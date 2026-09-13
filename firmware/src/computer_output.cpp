#include "computer_output.h"

#include "ble_computer_output.h"
#include "usb_hid_computer.h"

#include <Arduino.h>

void computerOutputBegin() {
  bleComputerOutputBegin();
#ifndef ECHOLOCATION_DEBUG
  usbHidComputerBegin();
#endif
}

void computerOutputTick() {
  bleComputerOutputTick();
#ifndef ECHOLOCATION_DEBUG
  usbHidComputerTick();
#endif
}

void computerOutputSetBluetoothEnabled(bool enabled) {
  bleComputerOutputSetEnabled(enabled);
}

void computerOutputSendKey(uint8_t mod, uint8_t key) {
  if (key == 0) {
    return;
  }

#ifndef ECHOLOCATION_DEBUG
  if (usbHidComputerIsReady()) {
    usbHidComputerSendKey(mod, key);
    return;
  }
#endif

  if (bleComputerOutputIsConnected()) {
#ifdef ECHOLOCATION_DEBUG
    Serial.printf("[out] ble key 0x%02x mod 0x%02x\n", key, mod);
#endif
    bleComputerOutputSendKey(mod, key);
  }
#ifdef ECHOLOCATION_DEBUG
  else {
    Serial.printf("[out] send key 0x%02x mod 0x%02x\n", key, mod);
  }
#endif
}

void computerOutputSendBootReport(const uint8_t report[8]) {
  if (report == nullptr) {
    return;
  }

#ifndef ECHOLOCATION_DEBUG
  if (usbHidComputerIsReady()) {
    usbHidComputerSendBootReport(report);
    return;
  }
#endif

  if (bleComputerOutputIsConnected()) {
    bleComputerOutputSendBootReport(report);
  }
}

bool computerOutputUsbReady() {
#ifndef ECHOLOCATION_DEBUG
  return usbHidComputerIsReady();
#else
  return false;
#endif
}

bool computerOutputBluetoothConnected() {
  return bleComputerOutputIsConnected();
}
