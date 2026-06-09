#include "app.h"

#include <SD.h>
#include <cstring>

#include "audio_router.h"
#include "ble_keyboard_source.h"
#include "computer_output.h"
#include "device_settings_store.h"
#include "hold_detector.h"
#include "key_event.h"
#include "lvgl_port.h"
#include "speech_player.h"
#include "ui_manager.h"
#include "usb_keyboard_source.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace echo {

namespace {

DeviceSettingsStore settings_store;
DeviceSettings settings;
UiManager ui;
SpeechPlayer speech;
AudioRouter audio_router;
ComputerOutput computer_output;
UsbKeyboardSource usb_keyboard;
BleKeyboardSource ble_keyboard;
HoldDetector hold_detector;

bool sd_ready = false;

constexpr uint32_t kLvglTickIntervalMs = 5;
constexpr uint32_t kUsbKeyboardTickIntervalMs = 2;
constexpr uint32_t kBleKeyboardTickIntervalMs = 5;
constexpr uint32_t kComputerOutputTickIntervalMs = 5;

#ifndef NATIVE_TEST
TaskHandle_t ui_task_handle = nullptr;
TaskHandle_t worker_task_handle = nullptr;
portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;

struct UiMailbox {
  char key_label[32] = "-";
  bool has_error = false;
  char error_message[96] = "";
  bool settings_override_pending = false;
  DeviceSettings settings_override{};
};

struct WorkerMailbox {
  bool settings_pending = false;
  DeviceSettings pending_settings{};
  bool ble_scan_requested = false;
  bool factory_reset_requested = false;
};

UiMailbox ui_mailbox;
WorkerMailbox worker_mailbox;

void copyCString(char* destination, size_t destination_size, const char* source) {
  if (!destination || destination_size == 0) {
    return;
  }
  const char* safe_source = source ? source : "";
  std::strncpy(destination, safe_source, destination_size - 1);
  destination[destination_size - 1] = '\0';
}

void setUiKeyLabel(const char* label) {
  portENTER_CRITICAL(&state_lock);
  copyCString(ui_mailbox.key_label, sizeof(ui_mailbox.key_label), label);
  portEXIT_CRITICAL(&state_lock);
}

void setUiError(const char* message) {
  portENTER_CRITICAL(&state_lock);
  ui_mailbox.has_error = true;
  copyCString(ui_mailbox.error_message, sizeof(ui_mailbox.error_message), message);
  portEXIT_CRITICAL(&state_lock);
}

void clearUiError() {
  portENTER_CRITICAL(&state_lock);
  ui_mailbox.has_error = false;
  ui_mailbox.error_message[0] = '\0';
  portEXIT_CRITICAL(&state_lock);
}

void queueSettingsForWorker(const DeviceSettings& updated_settings) {
  portENTER_CRITICAL(&state_lock);
  worker_mailbox.pending_settings = updated_settings;
  worker_mailbox.settings_pending = true;
  portEXIT_CRITICAL(&state_lock);
}

bool takePendingSettings(DeviceSettings& out_settings) {
  bool has_pending = false;
  portENTER_CRITICAL(&state_lock);
  if (worker_mailbox.settings_pending) {
    out_settings = worker_mailbox.pending_settings;
    worker_mailbox.settings_pending = false;
    has_pending = true;
  }
  portEXIT_CRITICAL(&state_lock);
  return has_pending;
}

void queueUiSettingsOverride(const DeviceSettings& override_settings) {
  portENTER_CRITICAL(&state_lock);
  ui_mailbox.settings_override = override_settings;
  ui_mailbox.settings_override_pending = true;
  portEXIT_CRITICAL(&state_lock);
}

bool takeUiSettingsOverride(DeviceSettings& out_settings) {
  bool has_override = false;
  portENTER_CRITICAL(&state_lock);
  if (ui_mailbox.settings_override_pending) {
    out_settings = ui_mailbox.settings_override;
    ui_mailbox.settings_override_pending = false;
    has_override = true;
  }
  portEXIT_CRITICAL(&state_lock);
  return has_override;
}

void requestBleScan() {
  portENTER_CRITICAL(&state_lock);
  worker_mailbox.ble_scan_requested = true;
  portEXIT_CRITICAL(&state_lock);
}

bool takeBleScanRequest() {
  bool requested = false;
  portENTER_CRITICAL(&state_lock);
  requested = worker_mailbox.ble_scan_requested;
  worker_mailbox.ble_scan_requested = false;
  portEXIT_CRITICAL(&state_lock);
  return requested;
}

void requestFactoryReset() {
  portENTER_CRITICAL(&state_lock);
  worker_mailbox.factory_reset_requested = true;
  portEXIT_CRITICAL(&state_lock);
}

bool takeFactoryResetRequest() {
  bool requested = false;
  portENTER_CRITICAL(&state_lock);
  requested = worker_mailbox.factory_reset_requested;
  worker_mailbox.factory_reset_requested = false;
  portEXIT_CRITICAL(&state_lock);
  return requested;
}

void updateConnectivityError() {
  if (!usb_keyboard.isKeyboardConnected() && !ble_keyboard.isKeyboardConnected()) {
    setUiError("No keyboard connected");
  } else if (sd_ready) {
    clearUiError();
  }
}

void uiTaskMain(void* parameter) {
  (void)parameter;
  uint32_t last_lvgl_tick_ms = 0;
  uint32_t last_battery_poll_ms = 0;

  while (true) {
    const uint32_t now = millis();
    M5.update();

    if (now - last_lvgl_tick_ms >= kLvglTickIntervalMs) {
      lvglPortTick();
      last_lvgl_tick_ms = now;
    }

    DeviceSettings settings_override;
    if (takeUiSettingsOverride(settings_override)) {
      ui.editingSettings() = settings_override;
    }

    UiMailbox snapshot;
    portENTER_CRITICAL(&state_lock);
    snapshot = ui_mailbox;
    portEXIT_CRITICAL(&state_lock);

    ui.setCurrentKeyLabel(snapshot.key_label);
    if (snapshot.has_error) {
      ui.setErrorMessage(snapshot.error_message);
    } else {
      ui.clearError();
    }

    if (now - last_battery_poll_ms >= 1000) {
      ui.setBatteryPercent(M5.Power.getBatteryLevel());
      last_battery_poll_ms = now;
    }

    if (ui.settingsChanged()) {
      DeviceSettings updated_settings = ui.editingSettings();
      clampDeviceSettings(updated_settings);
      queueSettingsForWorker(updated_settings);
      ui.acknowledgeSettingsSaved();
    }

    ui.draw();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void workerTaskMain(void* parameter) {
  (void)parameter;
  uint32_t last_usb_keyboard_tick_ms = 0;
  uint32_t last_ble_keyboard_tick_ms = 0;
  uint32_t last_computer_output_tick_ms = 0;

  while (true) {
    const uint32_t now = millis();

    if (takeBleScanRequest()) {
      ble_keyboard.startScan();
    }

    if (takeFactoryResetRequest()) {
      settings_store.factoryReset();
      settings = defaultDeviceSettings();
      hold_detector.reset();
      hold_detector.setHoldDurationMs(settings.hold_duration_ms);
      speech.setVolumePercent(settings.volume_percent);
      queueUiSettingsOverride(settings);
      setUiError("Factory reset complete");
    }

    DeviceSettings updated_settings;
    if (takePendingSettings(updated_settings)) {
      settings = updated_settings;
      settings_store.save(settings);
      hold_detector.setHoldDurationMs(settings.hold_duration_ms);
      speech.setVolumePercent(settings.volume_percent);
    }

    if (now - last_usb_keyboard_tick_ms >= kUsbKeyboardTickIntervalMs) {
      usb_keyboard.tick(now);
      last_usb_keyboard_tick_ms = now;
    }

    if (now - last_ble_keyboard_tick_ms >= kBleKeyboardTickIntervalMs) {
      ble_keyboard.tick(now);
      last_ble_keyboard_tick_ms = now;
    }

    if (now - last_computer_output_tick_ms >= kComputerOutputTickIntervalMs) {
      computer_output.tick();
      last_computer_output_tick_ms = now;
    }

    audio_router.tick();
    speech.tick();
    hold_detector.tick(now);
    updateConnectivityError();

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
#endif

}  // namespace

void App::handleKeyEvent(const KeyEvent& event) {
  if (event.pressed) {
    speech.speakKey(event.hid_usage);
#ifndef NATIVE_TEST
    setUiKeyLabel(keyLabelForUsage(event.hid_usage));
#endif
  }
  hold_detector.onKeyEvent(event, millis());
}

void App::refreshUi() { ui.draw(); }

void App::applySettingsIfNeeded() {}

void App::setup() {
#ifndef NATIVE_TEST
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(128);
#endif

  settings_store.begin();
  settings_store.load(settings);
  ui.begin();
  ui.editingSettings() = settings;
  setUiKeyLabel("-");

  ui.setOnBleScanRequested([&]() { requestBleScan(); });
  ui.setOnFactoryResetConfirmed([&]() { requestFactoryReset(); });

  speech.begin([&](const char* message) { setUiError(message); });
  speech.setVolumePercent(settings.volume_percent);

  audio_router.begin();
  computer_output.begin();

  hold_detector.setHoldDurationMs(settings.hold_duration_ms);
  hold_detector.setTapCallback([&](const HoldTapAction& action) {
    computer_output.sendTap(action.hid_usage, action.modifier_mask);
  });

#ifndef NATIVE_TEST
  if (!SD.begin(GPIO_NUM_4)) {
    setUiError("microSD not found");
    sd_ready = false;
  } else {
    sd_ready = true;
    clearUiError();
  }
#endif

  usb_keyboard.begin([&](const KeyEvent& event) { handleKeyEvent(event); });
  ble_keyboard.begin([&](const KeyEvent& event) { handleKeyEvent(event); });

  xTaskCreatePinnedToCore(uiTaskMain, "ui_task", 8192, nullptr, 3, &ui_task_handle,
                          1);
  xTaskCreatePinnedToCore(workerTaskMain, "worker_task", 8192, nullptr, 2,
                          &worker_task_handle, 0);
}

#ifndef NATIVE_TEST
void App::loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
#else
void App::loop() {}
#endif

static App g_app;

void appSetup() { g_app.setup(); }
void appLoop() { g_app.loop(); }

}  // namespace echo
