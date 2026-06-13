#include <Arduino.h>
#include <M5Unified.h>

#include "device_settings_store.h"
#include "key_audio.h"
#include "lvgl_port.h"
#include "ui.h"
#include "usb_keyboard.h"

namespace {

constexpr uint32_t kBatteryUpdateIntervalMs = 2000;

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
  M5.Display.setBrightness(128);

  lvglPortInit();
  uiInit();
  deviceSettingsLoad();
  keyAudioBegin();
  usbKeyboardBegin();
  updateBatteryStatus();
}

void loop() {
  M5.update();
  updateBatteryStatus();
  lvglPortTick();
  usbKeyboardTick();
  delay(5);
}
