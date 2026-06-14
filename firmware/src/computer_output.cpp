#include "computer_output.h"

#include "ble_hid_computer.h"
#include "usb_hid_computer.h"

void computerOutputBegin() {
  usbHidComputerBegin();
  bleHidComputerSetEnabled(true);
  bleHidComputerBegin();
}

void computerOutputTick() {
  usbHidComputerTick();
  bleHidComputerTick();
}

void computerOutputSendKey(uint8_t mod, uint8_t key) {
  if (usbHidComputerIsReady()) {
    usbHidComputerSendKey(mod, key);
  }
  if (bleHidComputerIsEnabled() && bleHidComputerIsConnected()) {
    bleHidComputerSendKey(mod, key);
  }
}

bool computerOutputUsbReady() { return usbHidComputerIsReady(); }

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
