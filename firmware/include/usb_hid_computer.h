#pragma once

#include <cstdint>

namespace echo {

void usbHidComputerBegin();
void usbHidComputerTick(bool& connected);
void usbHidComputerSendTap(uint8_t hid_usage, uint8_t modifier_mask);

}  // namespace echo
