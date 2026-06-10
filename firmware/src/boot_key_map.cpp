#include "boot_key_map.h"

#include "key_event.h"

namespace echo {

uint8_t bootKeyToHidUsage(uint8_t boot_key) {
  // Boot protocol report bytes are already HID keyboard usages (usage page 0x07).
  if (boot_key >= 0x04 && boot_key <= 0xE7) {
    return boot_key;
  }
  return 0;
}

uint8_t bootModifierToHidUsage(uint8_t modifier_bit_mask) {
  switch (modifier_bit_mask) {
    case 0x01:
      return kLeftControl;
    case 0x02:
      return kLeftShift;
    case 0x04:
      return kLeftAlt;
    case 0x08:
      return kLeftGui;
    case 0x10:
      return kRightControl;
    case 0x20:
      return kRightShift;
    case 0x40:
      return kRightAlt;
    case 0x80:
      return kRightGui;
    default:
      return 0;
  }
}

}  // namespace echo
