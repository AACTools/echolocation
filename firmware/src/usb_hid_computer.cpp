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

bool usbHidComputerIsReady() { return tud_mounted(); }
