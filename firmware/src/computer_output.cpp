#include "computer_output.h"

#include "usb_hid_computer.h"

#include <Arduino.h>

void computerOutputBegin() {
#ifndef ECHOLOCATION_DEBUG
  usbHidComputerBegin();
#endif
}

void computerOutputTick() {
#ifndef ECHOLOCATION_DEBUG
  usbHidComputerTick();
#endif
}

void computerOutputSendKey(uint8_t mod, uint8_t key) {
#ifdef ECHOLOCATION_DEBUG
  Serial.printf("[out] send key 0x%02x mod 0x%02x\n", key, mod);
#else
  if (usbHidComputerIsReady()) {
    usbHidComputerSendKey(mod, key);
  }
#endif
}

bool computerOutputUsbReady() {
#ifndef ECHOLOCATION_DEBUG
  return usbHidComputerIsReady();
#else
  return false;
#endif
}

bool computerOutputBluetoothConnected() {
  return false;
}
