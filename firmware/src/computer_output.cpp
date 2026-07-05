#include "computer_output.h"

#include "usb_hid_computer.h"

void computerOutputBegin() { usbHidComputerBegin(); }

void computerOutputTick() { usbHidComputerTick(); }

void computerOutputSendKey(uint8_t mod, uint8_t key) {
  if (usbHidComputerIsReady()) {
    usbHidComputerSendKey(mod, key);
  }
}

bool computerOutputUsbReady() { return usbHidComputerIsReady(); }
