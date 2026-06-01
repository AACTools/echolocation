#include "key_event.h"

#include <cstdio>

namespace echo {

uint16_t makeKeySlotId(uint8_t hid_usage, uint8_t modifier_mask) {
  return static_cast<uint16_t>((static_cast<uint16_t>(hid_usage) << 8) | modifier_mask);
}

const char* keyLabelForUsage(uint8_t hid_usage) {
  if (hid_usage >= kModifierUsageBase && hid_usage <= kRightGui) {
    switch (static_cast<ModifierUsage>(hid_usage)) {
      case kLeftControl:
        return "Left Control";
      case kLeftShift:
        return "Left Shift";
      case kLeftAlt:
        return "Left Alt";
      case kLeftGui:
        return "Left Command";
      case kRightControl:
        return "Right Control";
      case kRightShift:
        return "Right Shift";
      case kRightAlt:
        return "Right Alt";
      case kRightGui:
        return "Right Command";
      default:
        break;
    }
  }

  if (hid_usage >= 0x04 && hid_usage <= 0x1D) {
    static char letter[2] = {'A', '\0'};
    letter[0] = static_cast<char>('A' + (hid_usage - 0x04));
    return letter;
  }

  switch (hid_usage) {
    case 0x28:
      return "Return";
    case 0x29:
      return "Escape";
    case 0x2A:
      return "Backspace";
    case 0x2B:
      return "Tab";
    case 0x2C:
      return "Space";
    case 0x3A:
      return "F1";
    case 0x3B:
      return "F2";
    case 0x3C:
      return "F3";
    case 0x3D:
      return "F4";
    case 0x3E:
      return "F5";
    case 0x3F:
      return "F6";
    case 0x40:
      return "F7";
    case 0x41:
      return "F8";
    case 0x42:
      return "F9";
    case 0x43:
      return "F10";
    case 0x44:
      return "F11";
    case 0x45:
      return "F12";
    case 0x4F:
      return "Right Arrow";
    case 0x50:
      return "Left Arrow";
    case 0x51:
      return "Down Arrow";
    case 0x52:
      return "Up Arrow";
    case 0x53:
      return "Num Lock";
    case 0x54:
      return "Keypad Slash";
    case 0x55:
      return "Keypad Asterisk";
    case 0x56:
      return "Keypad Minus";
    case 0x57:
      return "Keypad Plus";
    case 0x58:
      return "Keypad Enter";
    case 0x59:
      return "Keypad 1";
    case 0x5A:
      return "Keypad 2";
    case 0x5B:
      return "Keypad 3";
    case 0x5C:
      return "Keypad 4";
    case 0x5D:
      return "Keypad 5";
    case 0x5E:
      return "Keypad 6";
    case 0x5F:
      return "Keypad 7";
    case 0x60:
      return "Keypad 8";
    case 0x61:
      return "Keypad 9";
    case 0x62:
      return "Keypad 0";
    case 0x63:
      return "Keypad Period";
    default:
      break;
  }

  static char unknown[16];
  std::snprintf(unknown, sizeof(unknown), "Key 0x%02X", hid_usage);
  return unknown;
}

const char* audioPathForUsage(uint8_t hid_usage) {
  static char path[32];
  std::snprintf(path, sizeof(path), "/audio/keys/u%03x.wav", hid_usage);
  return path;
}

}  // namespace echo
