#pragma once

#include <cstdint>

namespace echo {

void bleHidComputerBegin(const char* device_name);
void bleHidComputerTick(bool& connected);
void bleHidComputerSendTap(uint8_t hid_usage, uint8_t modifier_mask);

}  // namespace echo
