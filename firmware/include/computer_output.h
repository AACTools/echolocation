#pragma once

#include <cstdint>

namespace echo {

class ComputerOutput {
 public:
  void begin();
  void tick();
  bool isComputerConnected() const;
  void sendTap(uint8_t hid_usage, uint8_t modifier_mask);

 private:
  bool usb_connected_ = false;
  bool ble_connected_ = false;
};

}  // namespace echo
