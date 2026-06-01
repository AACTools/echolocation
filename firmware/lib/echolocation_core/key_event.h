#pragma once

#include <cstdint>

namespace echo {

// USB HID keyboard usage page (0x07) key codes and modifier pseudo-usages.
constexpr uint8_t kModifierUsageBase = 0xE0;

enum ModifierUsage : uint8_t {
  kLeftControl = 0xE0,
  kLeftShift = 0xE1,
  kLeftAlt = 0xE2,
  kLeftGui = 0xE3,
  kRightControl = 0xE4,
  kRightShift = 0xE5,
  kRightAlt = 0xE6,
  kRightGui = 0xE7,
};

struct KeyEvent {
  uint8_t hid_usage = 0;
  uint8_t modifier_mask = 0;
  bool pressed = false;
  uint32_t timestamp_ms = 0;
};

uint16_t makeKeySlotId(uint8_t hid_usage, uint8_t modifier_mask);
const char* keyLabelForUsage(uint8_t hid_usage);
const char* audioPathForUsage(uint8_t hid_usage);

}  // namespace echo
