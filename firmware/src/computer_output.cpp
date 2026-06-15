#include "computer_output.h"

#include "ble_hid_computer.h"
#ifndef ECHOLOCATION_BLE_DEBUG
#include "usb_hid_computer.h"
#endif

#include <Arduino.h>

void computerOutputBegin() {
#ifndef ECHOLOCATION_BLE_DEBUG
  usbHidComputerBegin();
#endif
  bleHidComputerSetEnabled(true);
  bleHidComputerBegin();
}

void computerOutputTick() {
#ifndef ECHOLOCATION_BLE_DEBUG
  usbHidComputerTick();
#endif
  bleHidComputerTick();
}

void computerOutputSendKey(uint8_t mod, uint8_t key) {
#ifdef ECHOLOCATION_BLE_DEBUG
  const bool ble_enabled = bleHidComputerIsEnabled();
  const bool ble_connected = bleHidComputerIsConnected();
  Serial.printf("[out] send key 0x%02x mod 0x%02x ble_en=%d ble_conn=%d\n", key, mod,
                ble_enabled, ble_connected);
#else
  if (usbHidComputerIsReady()) {
    usbHidComputerSendKey(mod, key);
  }
#endif
  if (bleHidComputerIsEnabled() && bleHidComputerIsConnected()) {
    bleHidComputerSendKey(mod, key);
  }
}

bool computerOutputUsbReady() {
#ifndef ECHOLOCATION_BLE_DEBUG
  return usbHidComputerIsReady();
#else
  return false;
#endif
}

bool computerOutputBleConnected() { return bleHidComputerIsConnected(); }

bool computerOutputBleAdvertising() { return bleHidComputerIsAdvertising(); }

void computerOutputBleSetEnabled(bool enabled) {
  bleHidComputerSetEnabled(enabled);
}

bool computerOutputBleIsEnabled() { return bleHidComputerIsEnabled(); }

void computerOutputBleStartPairing() { bleHidComputerStartPairing(); }

void computerOutputBleStopPairing() { bleHidComputerStopPairing(); }

void computerOutputBleSetDeviceName(const char* name) {
  bleHidComputerSetDeviceName(name);
}

const char* computerOutputBleGetDeviceName() {
  return bleHidComputerGetDeviceName();
}
