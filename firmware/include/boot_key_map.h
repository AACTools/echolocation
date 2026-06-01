#pragma once

#include <cstdint>

#include "key_event.h"

namespace echo {

// Map USB boot keyboard scan codes (HID boot protocol) to HID usage codes.
uint8_t bootKeyToHidUsage(uint8_t boot_key);
uint8_t bootModifierToHidUsage(uint8_t modifier_bit_mask);

}  // namespace echo
