#include "usb_hid_computer.h"

#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include "tusb.h"

namespace {

USBHIDKeyboard hid_keyboard;

void sendReport(uint8_t mod, uint8_t key) {
  KeyReport report = {};
  report.modifiers = mod;
  report.keys[0] = key;
  hid_keyboard.sendReport(&report);
}

}  // namespace

void usbHidComputerBegin() {
  hid_keyboard.begin();
}

void usbHidComputerTick() {}

void usbHidComputerSendKey(uint8_t mod, uint8_t key) {
  if (key == 0) {
    return;
  }

  sendReport(mod, key);
  delay(8);
  hid_keyboard.releaseAll();
}

void usbHidComputerSendBootReport(const uint8_t report[8]) {
  if (report == nullptr) {
    return;
  }

  KeyReport out = {};
  out.modifiers = report[0];
  uint8_t slot = 0;
  for (uint8_t i = 2; i < 8 && slot < 6; ++i) {
    const uint8_t key = report[i];
    if (key != 0 && key != 1) {
      out.keys[slot++] = key;
    }
  }
  hid_keyboard.sendReport(&out);
}

bool usbHidComputerIsReady() { return tud_mounted(); }
