#include "app.h"

#include <SD.h>

#include "audio_router.h"
#include "ble_keyboard_source.h"
#include "computer_output.h"
#include "device_settings_store.h"
#include "hold_detector.h"
#include "key_event.h"
#include "speech_player.h"
#include "ui_manager.h"
#include "usb_keyboard_source.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
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
bool factory_reset_requested = false;

}  // namespace

void App::handleKeyEvent(const KeyEvent& event) {
  if (event.pressed) {
    speech.speakKey(event.hid_usage);
    ui.setCurrentKeyLabel(keyLabelForUsage(event.hid_usage));
  }
  hold_detector.onKeyEvent(event, millis());
}

void App::refreshUi() {
#ifndef NATIVE_TEST
  int battery = M5.Power.getBatteryLevel();
  ui.setBatteryPercent(battery);
#endif
}

void App::applySettingsIfNeeded() {
  if (!ui.settingsChanged()) {
    return;
  }
  settings = ui.editingSettings();
  clampDeviceSettings(settings);
  settings_store.save(settings);
  hold_detector.setHoldDurationMs(settings.hold_duration_ms);
  speech.setVolumePercent(settings.volume_percent);
  ui.acknowledgeSettingsSaved();
}

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
  ui.refreshSettingsWidgets();

  speech.begin([&](const char* message) { ui.setErrorMessage(message); });
  speech.setVolumePercent(settings.volume_percent);

  audio_router.begin();
  computer_output.begin();

  hold_detector.setHoldDurationMs(settings.hold_duration_ms);
  hold_detector.setTapCallback([&](const HoldTapAction& action) {
    computer_output.sendTap(action.hid_usage, action.modifier_mask);
  });

#ifndef NATIVE_TEST
  if (!SD.begin(GPIO_NUM_4)) {
    ui.setErrorMessage("microSD not found");
    sd_ready = false;
  } else {
    sd_ready = true;
    ui.clearError();
  }
#endif

  usb_keyboard.begin([&](const KeyEvent& event) { handleKeyEvent(event); });
  ble_keyboard.begin([&](const KeyEvent& event) { handleKeyEvent(event); });

  refreshUi();
}

void App::loop() {
#ifndef NATIVE_TEST
  M5.update();
#endif

  const uint32_t now = millis();
  usb_keyboard.tick(now);
  ble_keyboard.tick(now);
  computer_output.tick();
  audio_router.tick();
  speech.tick();
  hold_detector.tick(now);

  ui.tick();

  if (ui.bleScanRequested()) {
    ble_keyboard.startScan();
    ui.clearBleScanRequested();
  }

  if (ui.factoryResetConfirmed()) {
    ui.clearFactoryResetConfirmed();
    settings_store.factoryReset();
    settings = defaultDeviceSettings();
    ui.editingSettings() = settings;
    hold_detector.setHoldDurationMs(settings.hold_duration_ms);
    speech.setVolumePercent(settings.volume_percent);
    hold_detector.reset();
    ui.setErrorMessage("Factory reset complete");
  }

  applySettingsIfNeeded();

  if (!usb_keyboard.isKeyboardConnected() &&
      !ble_keyboard.isKeyboardConnected()) {
    ui.setErrorMessage("No keyboard connected");
  } else if (sd_ready) {
    ui.clearError();
  }

  static uint32_t last_battery_ui = 0;
  if (now - last_battery_ui > 1000) {
    refreshUi();
    last_battery_ui = now;
  }
}

static App g_app;

void appSetup() { g_app.setup(); }
void appLoop() { g_app.loop(); }

}  // namespace echo
