#include <Arduino.h>
#include <M5Unified.h>

#include "device_settings_store.h"
#include "key_audio.h"
#include "lvgl_port.h"
#include "ui.h"
#include "usb_keyboard.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(128);

  lvglPortInit();
  uiInit();
  deviceSettingsLoad();
  keyAudioBegin();
  usbKeyboardBegin();
}

void loop() {
  M5.update();
  lvglPortTick();
  usbKeyboardTick();
  delay(5);
}
