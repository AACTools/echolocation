#include "computer_output.h"

#include "ble_hid_computer.h"
#include "usb_hid_computer.h"

namespace echo {

void ComputerOutput::begin() {
  usbHidComputerBegin();
  bleHidComputerBegin("echolocation");
}

void ComputerOutput::tick() {
  usbHidComputerTick(usb_connected_);
  bleHidComputerTick(ble_connected_);
}

bool ComputerOutput::isComputerConnected() const {
  return usb_connected_ || ble_connected_;
}

void ComputerOutput::sendTap(uint8_t hid_usage, uint8_t modifier_mask) {
  if (!isComputerConnected()) {
    return;
  }
  if (usb_connected_) {
    usbHidComputerSendTap(hid_usage, modifier_mask);
  } else if (ble_connected_) {
    bleHidComputerSendTap(hid_usage, modifier_mask);
  }
}

}  // namespace echo
