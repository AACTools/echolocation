#include <Arduino.h>
#include <M5Unified.h>

#include "lvgl_port.h"
#include "ui.h"

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(128);

  lvglPortInit();
  uiInit();
}

void loop() {
  M5.update();
  lvglPortTick();
  delay(5);
}
