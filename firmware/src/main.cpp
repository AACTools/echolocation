#include <Arduino.h>
#include <M5Unified.h>

#include "lvgl_port.h"
#include "ui.h"
#include "key_audio.h"
#include "usb_keyboard.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(128);

  lvglPortInit();
  uiInit();
  keyAudioBegin();
  usbKeyboardBegin();
}

void loop() {
  M5.update();
  lvglPortTick();
  usbKeyboardTick();
  delay(5);
}
