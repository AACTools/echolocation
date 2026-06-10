#pragma once

#include <cstdint>

#include "key_event.h"

namespace echo {

// Boot protocol key bytes are HID keyboard usages; modifiers use a separate bitmask.
uint8_t bootKeyToHidUsage(uint8_t boot_key);
uint8_t bootModifierToHidUsage(uint8_t modifier_bit_mask);

}  // namespace echo
