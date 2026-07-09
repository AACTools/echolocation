#include <Arduino.h>
#include <M5Unified.h>

#include "device_settings_store.h"
#include "ble_keyboard_input.h"
#include "computer_output.h"
#include "key_audio.h"
#include "lvgl_port.h"
#include "speaker_detect.h"
#include "speaker_route.h"
#include "ui.h"
#include "usb_keyboard.h"

namespace {

constexpr uint32_t kBatteryUpdateIntervalMs = 2000;
constexpr uint32_t kLoadingPumpSliceMs = 30;

void pumpLoadingUi() {
  const uint32_t end_ms = millis() + kLoadingPumpSliceMs;
  while (millis() < end_ms) {
    uiPump();
    delay(5);
  }
}

void updateBatteryStatus() {
  static uint32_t last_update_ms = 0;
  const uint32_t now_ms = millis();
  if (now_ms - last_update_ms < kBatteryUpdateIntervalMs) {
    return;
  }
  last_update_ms = now_ms;

  const int level = M5.Power.getBatteryLevel();
  const bool charging =
      M5.Power.isCharging() == m5::Power_Class::is_charging;
  uiSetBattery(level, charging);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[boot] echolocation");
#ifdef ECHOLOCATION_DEBUG
  Serial.println("[boot] debug build");
#endif
  M5.Display.setBrightness(128);

  lvglPortInit();
  uiInit();

  uiSetLoadingStatus("Loading settings...");
  pumpLoadingUi();

  uiSetLoadingStatus("Starting computer output...");
  pumpLoadingUi();
  computerOutputBegin();

  uiSetLoadingStatus("Starting Bluetooth keyboard...");
  pumpLoadingUi();
  bleKeyboardInputBegin();

  uiSetLoadingStatus("Loading settings...");
  pumpLoadingUi();
  deviceSettingsLoad();

  uiSetLoadingStatus("Starting USB keyboard...");
  pumpLoadingUi();
  usbKeyboardBegin();

  uiSetLoadingStatus("Loading audio...");
  pumpLoadingUi();
  keyAudioRefresh();

  uiSetLoadingStatus("Starting speakers...");
  pumpLoadingUi();
  speakerDetectBegin();
  speakerRouteBegin();

  updateBatteryStatus();
  uiFinishLoading();
}

void loop() {
  M5.update();
  updateBatteryStatus();
  lvglPortTick();
  computerOutputTick();
  bleKeyboardInputTick();
  uiRefreshConnectionFlow();
  uiRefreshBluetoothOutputStatus();
  uiRefreshBluetoothKeyboardStatus();
  if (speakerDetectPoll()) {
    speakerRouteApply();
    uiRefreshSpeakerOutput();
  }
  usbKeyboardTick();
  delay(5);
}
