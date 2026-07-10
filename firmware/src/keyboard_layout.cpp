#include "keyboard_layout.h"

#include <stdio.h>
#include <string.h>

namespace {

KeyboardDetectedLayout detected_layout = KeyboardDetectedLayout::kUnknown;

constexpr uint8_t kHidCountryUs = 33;
constexpr uint8_t kHidCountryUk = 32;
constexpr uint8_t kHidCountryCanada = 7;
constexpr uint8_t kHidCountryIreland = 18;

struct CharMap {
  const char* unshifted_token;
  const char* shifted_token;
  char unshifted_display;
  char shifted_display;
};

struct NamedKey {
  uint8_t usage;
  const char* token;
  const char* display;
};

struct PhysicalKey {
  uint8_t usage;
  const char* suffix;
  const char* display;
};

void copyToken(char* out, size_t out_len, const char* token) {
  if (out == nullptr || out_len == 0 || token == nullptr) {
    return;
  }
  strncpy(out, token, out_len - 1);
  out[out_len - 1] = '\0';
}

void copyDisplay(char* out, size_t out_len, const char* display) {
  if (out == nullptr || out_len == 0 || display == nullptr) {
    return;
  }
  strncpy(out, display, out_len - 1);
  out[out_len - 1] = '\0';
}

bool shiftHeld(uint8_t mod) { return (mod & 0x22) != 0; }

bool applyCharMap(const CharMap& map, bool shift, KeyLabel* out) {
  if (shift) {
    copyToken(out->speech_token, sizeof(out->speech_token), map.shifted_token);
    out->display[0] = map.shifted_display;
    out->display[1] = '\0';
  } else {
    copyToken(out->speech_token, sizeof(out->speech_token), map.unshifted_token);
    out->display[0] = map.unshifted_display;
    out->display[1] = '\0';
  }
  return true;
}

bool usCharMapping(uint8_t key, bool shift, KeyLabel* out) {
  static const CharMap kLetters[] = {
      {"a", "a", 'a', 'A'}, {"b", "b", 'b', 'B'}, {"c", "c", 'c', 'C'},
      {"d", "d", 'd', 'D'}, {"e", "e", 'e', 'E'}, {"f", "f", 'f', 'F'},
      {"g", "g", 'g', 'G'}, {"h", "h", 'h', 'H'}, {"i", "i", 'i', 'I'},
      {"j", "j", 'j', 'J'}, {"k", "k", 'k', 'K'}, {"l", "l", 'l', 'L'},
      {"m", "m", 'm', 'M'}, {"n", "n", 'n', 'N'}, {"o", "o", 'o', 'O'},
      {"p", "p", 'p', 'P'}, {"q", "q", 'q', 'Q'}, {"r", "r", 'r', 'R'},
      {"s", "s", 's', 'S'}, {"t", "t", 't', 'T'}, {"u", "u", 'u', 'U'},
      {"v", "v", 'v', 'V'}, {"w", "w", 'w', 'W'}, {"x", "x", 'x', 'X'},
      {"y", "y", 'y', 'Y'}, {"z", "z", 'z', 'Z'},
  };
  static const CharMap kDigitRow[] = {
      {"1", "exclamation", '1', '!'}, {"2", "at", '2', '@'},
      {"3", "hash", '3', '#'},       {"4", "dollar", '4', '$'},
      {"5", "percent", '5', '%'},    {"6", "caret", '6', '^'},
      {"7", "ampersand", '7', '&'},  {"8", "asterisk", '8', '*'},
      {"9", "left_paren", '9', '('}, {"0", "right_paren", '0', ')'},
  };

  if (key >= 0x04 && key <= 0x1D) {
    return applyCharMap(kLetters[key - 0x04], shift, out);
  }
  if (key >= 0x1E && key <= 0x27) {
    return applyCharMap(kDigitRow[key - 0x1E], shift, out);
  }

  switch (key) {
    case 0x2D:
      return applyCharMap({"minus", "underscore", '-', '_'}, shift, out);
    case 0x2E:
      return applyCharMap({"equals", "plus", '=', '+'}, shift, out);
    case 0x2F:
      return applyCharMap({"left_bracket", "left_brace", '[', '{'}, shift, out);
    case 0x30:
      return applyCharMap({"right_bracket", "right_brace", ']', '}'}, shift, out);
    case 0x31:
      return applyCharMap({"backslash", "pipe", '\\', '|'}, shift, out);
    case 0x32:
      return applyCharMap({"hash", "tilde", '#', '~'}, shift, out);
    case 0x33:
      return applyCharMap({"single_quote", "double_quote", '\'', '"'}, shift, out);
    case 0x34:
      return applyCharMap({"semicolon", "colon", ';', ':'}, shift, out);
    case 0x35:
      return applyCharMap({"slash", "question", '/', '?'}, shift, out);
    case 0x36:
      return applyCharMap({"backtick", "tilde", '`', '~'}, shift, out);
    case 0x37:
      return applyCharMap({"comma", "less_than", ',', '<'}, shift, out);
    case 0x38:
      return applyCharMap({"period", "greater_than", '.', '>'}, shift, out);
    default:
      return false;
  }
}

bool ukCharMapping(uint8_t key, bool shift, KeyLabel* out) {
  static const CharMap kLetters[] = {
      {"a", "a", 'a', 'A'}, {"b", "b", 'b', 'B'}, {"c", "c", 'c', 'C'},
      {"d", "d", 'd', 'D'}, {"e", "e", 'e', 'E'}, {"f", "f", 'f', 'F'},
      {"g", "g", 'g', 'G'}, {"h", "h", 'h', 'H'}, {"i", "i", 'i', 'I'},
      {"j", "j", 'j', 'J'}, {"k", "k", 'k', 'K'}, {"l", "l", 'l', 'L'},
      {"m", "m", 'm', 'M'}, {"n", "n", 'n', 'N'}, {"o", "o", 'o', 'O'},
      {"p", "p", 'p', 'P'}, {"q", "q", 'q', 'Q'}, {"r", "r", 'r', 'R'},
      {"s", "s", 's', 'S'}, {"t", "t", 't', 'T'}, {"u", "u", 'u', 'U'},
      {"v", "v", 'v', 'V'}, {"w", "w", 'w', 'W'}, {"x", "x", 'x', 'X'},
      {"y", "y", 'y', 'Y'}, {"z", "z", 'z', 'Z'},
  };

  if (key >= 0x04 && key <= 0x1D) {
    return applyCharMap(kLetters[key - 0x04], shift, out);
  }

  switch (key) {
    case 0x1E:
      return applyCharMap({"1", "exclamation", '1', '!'}, shift, out);
    case 0x1F:
      return applyCharMap({"2", "double_quote", '2', '"'}, shift, out);
    case 0x20:
      if (shift) {
        copyToken(out->speech_token, sizeof(out->speech_token), "pound");
        copyDisplay(out->display, sizeof(out->display), "\xC2\xA3");
      } else {
        copyToken(out->speech_token, sizeof(out->speech_token), "3");
        out->display[0] = '3';
        out->display[1] = '\0';
      }
      return true;
    case 0x21:
      return applyCharMap({"4", "dollar", '4', '$'}, shift, out);
    case 0x22:
      return applyCharMap({"5", "percent", '5', '%'}, shift, out);
    case 0x23:
      return applyCharMap({"6", "caret", '6', '^'}, shift, out);
    case 0x24:
      return applyCharMap({"7", "ampersand", '7', '&'}, shift, out);
    case 0x25:
      return applyCharMap({"8", "asterisk", '8', '*'}, shift, out);
    case 0x26:
      return applyCharMap({"9", "left_paren", '9', '('}, shift, out);
    case 0x27:
      return applyCharMap({"0", "right_paren", '0', ')'}, shift, out);
    case 0x2D:
      return applyCharMap({"minus", "underscore", '-', '_'}, shift, out);
    case 0x2E:
      return applyCharMap({"equals", "plus", '=', '+'}, shift, out);
    case 0x2F:
      return applyCharMap({"left_bracket", "left_brace", '[', '{'}, shift, out);
    case 0x30:
      return applyCharMap({"right_bracket", "right_brace", ']', '}'}, shift, out);
    case 0x31:
      return applyCharMap({"backslash", "pipe", '\\', '|'}, shift, out);
    case 0x32:
      return applyCharMap({"hash", "tilde", '#', '~'}, shift, out);
    case 0x33:
      return applyCharMap({"single_quote", "at", '\'', '@'}, shift, out);
    case 0x34:
      return applyCharMap({"semicolon", "colon", ';', ':'}, shift, out);
    case 0x35:
      return applyCharMap({"slash", "question", '/', '?'}, shift, out);
    case 0x36:
      if (shift) {
        copyToken(out->speech_token, sizeof(out->speech_token), "not_sign");
        copyDisplay(out->display, sizeof(out->display), "\xC2\xAC");
      } else {
        copyToken(out->speech_token, sizeof(out->speech_token), "backtick");
        out->display[0] = '`';
        out->display[1] = '\0';
      }
      return true;
    case 0x37:
      return applyCharMap({"comma", "less_than", ',', '<'}, shift, out);
    case 0x38:
      return applyCharMap({"period", "greater_than", '.', '>'}, shift, out);
    case 0x64:
      return applyCharMap({"backslash", "pipe", '\\', '|'}, shift, out);
    default:
      return false;
  }
}

const NamedKey kNamedKeys[] = {
    {0x28, "enter", "Enter"},
    {0x29, "escape", "Esc"},
    {0x2A, "backspace", "Bksp"},
    {0x2B, "tab", "Tab"},
    {0x2C, "space", "Space"},
    {0x39, "caps_lock", "Caps"},
    {0x3A, "f1", "F1"},
    {0x3B, "f2", "F2"},
    {0x3C, "f3", "F3"},
    {0x3D, "f4", "F4"},
    {0x3E, "f5", "F5"},
    {0x3F, "f6", "F6"},
    {0x40, "f7", "F7"},
    {0x41, "f8", "F8"},
    {0x42, "f9", "F9"},
    {0x43, "f10", "F10"},
    {0x44, "f11", "F11"},
    {0x45, "f12", "F12"},
    {0x46, "print_screen", "PrtSc"},
    {0x47, "scroll_lock", "ScrLk"},
    {0x48, "pause", "Pause"},
    {0x49, "insert", "Ins"},
    {0x4A, "home", "Home"},
    {0x4B, "page_up", "PgUp"},
    {0x4C, "delete", "Del"},
    {0x4D, "end", "End"},
    {0x4E, "page_down", "PgDn"},
    {0x4F, "arrow_right", "Right"},
    {0x50, "arrow_left", "Left"},
    {0x51, "arrow_down", "Down"},
    {0x52, "arrow_up", "Up"},
    {0x53, "num_lock", "Num"},
    {0x54, "numpad_divide", "Num/"},
    {0x55, "numpad_multiply", "Num*"},
    {0x56, "numpad_minus", "Num-"},
    {0x57, "numpad_plus", "Num+"},
    {0x58, "numpad_enter", "NumEnt"},
    {0x59, "numpad_1", "Num1"},
    {0x5A, "numpad_2", "Num2"},
    {0x5B, "numpad_3", "Num3"},
    {0x5C, "numpad_4", "Num4"},
    {0x5D, "numpad_5", "Num5"},
    {0x5E, "numpad_6", "Num6"},
    {0x5F, "numpad_7", "Num7"},
    {0x60, "numpad_8", "Num8"},
    {0x61, "numpad_9", "Num9"},
    {0x62, "numpad_0", "Num0"},
    {0x63, "numpad_decimal", "Num."},
    {0x64, "backslash", "\\"},
    {0x65, "menu", "Menu"},
    {0x67, "numpad_equals", "Num="},
    {0x68, "f13", "F13"},
    {0x69, "f14", "F14"},
    {0x6A, "f15", "F15"},
    {0x6B, "f16", "F16"},
    {0x6C, "f17", "F17"},
    {0x6D, "f18", "F18"},
    {0x6E, "f19", "F19"},
    {0x6F, "f20", "F20"},
    {0x70, "f21", "F21"},
    {0x71, "f22", "F22"},
    {0x72, "f23", "F23"},
    {0x73, "f24", "F24"},
};

const PhysicalKey kPhysicalKeys[] = {
    {0x04, "a", "A"},           {0x05, "b", "B"},           {0x06, "c", "C"},
    {0x07, "d", "D"},           {0x08, "e", "E"},           {0x09, "f", "F"},
    {0x0A, "g", "G"},           {0x0B, "h", "H"},           {0x0C, "i", "I"},
    {0x0D, "j", "J"},           {0x0E, "k", "K"},           {0x0F, "l", "L"},
    {0x10, "m", "M"},           {0x11, "n", "N"},           {0x12, "o", "O"},
    {0x13, "p", "P"},           {0x14, "q", "Q"},           {0x15, "r", "R"},
    {0x16, "s", "S"},           {0x17, "t", "T"},           {0x18, "u", "U"},
    {0x19, "v", "V"},           {0x1A, "w", "W"},           {0x1B, "x", "X"},
    {0x1C, "y", "Y"},           {0x1D, "z", "Z"},
    {0x1E, "1", "1"},           {0x1F, "2", "2"},           {0x20, "3", "3"},
    {0x21, "4", "4"},           {0x22, "5", "5"},           {0x23, "6", "6"},
    {0x24, "7", "7"},           {0x25, "8", "8"},           {0x26, "9", "9"},
    {0x27, "0", "0"},
    {0x2D, "minus", "-"},       {0x2E, "equals", "="},
    {0x2F, "left_bracket", "["}, {0x30, "right_bracket", "]"},
    {0x31, "backslash", "\\"},  {0x32, "hash", "#"},
    {0x33, "single_quote", "'"}, {0x34, "semicolon", ";"},
    {0x35, "slash", "/"},       {0x36, "backtick", "`"},
    {0x37, "comma", ","},       {0x38, "period", "."},
    {0x28, "enter", "Enter"},    {0x29, "escape", "Esc"},
    {0x2A, "backspace", "Bksp"}, {0x2B, "tab", "Tab"},
    {0x2C, "space", "Space"},    {0x39, "caps_lock", "Caps"},
    {0x3A, "f1", "F1"},         {0x3B, "f2", "F2"},
    {0x3C, "f3", "F3"},         {0x3D, "f4", "F4"},
    {0x3E, "f5", "F5"},         {0x3F, "f6", "F6"},
    {0x40, "f7", "F7"},         {0x41, "f8", "F8"},
    {0x42, "f9", "F9"},         {0x43, "f10", "F10"},
    {0x44, "f11", "F11"},       {0x45, "f12", "F12"},
    {0x46, "print_screen", "PrtSc"}, {0x47, "scroll_lock", "ScrLk"},
    {0x48, "pause", "Pause"},    {0x49, "insert", "Ins"},
    {0x4A, "home", "Home"},      {0x4B, "page_up", "PgUp"},
    {0x4C, "delete", "Del"},     {0x4D, "end", "End"},
    {0x4E, "page_down", "PgDn"}, {0x4F, "arrow_right", "Right"},
    {0x50, "arrow_left", "Left"}, {0x51, "arrow_down", "Down"},
    {0x52, "arrow_up", "Up"},    {0x53, "num_lock", "Num"},
    {0x54, "numpad_divide", "Num/"}, {0x55, "numpad_multiply", "Num*"},
    {0x56, "numpad_minus", "Num-"}, {0x57, "numpad_plus", "Num+"},
    {0x58, "numpad_enter", "NumEnt"}, {0x59, "numpad_1", "Num1"},
    {0x5A, "numpad_2", "Num2"}, {0x5B, "numpad_3", "Num3"},
    {0x5C, "numpad_4", "Num4"}, {0x5D, "numpad_5", "Num5"},
    {0x5E, "numpad_6", "Num6"}, {0x5F, "numpad_7", "Num7"},
    {0x60, "numpad_8", "Num8"}, {0x61, "numpad_9", "Num9"},
    {0x62, "numpad_0", "Num0"}, {0x63, "numpad_decimal", "Num."},
    {0x64, "backslash", "\\"},  {0x65, "menu", "Menu"},
    {0x67, "numpad_equals", "Num="},
    {0x68, "f13", "F13"},       {0x69, "f14", "F14"},
    {0x6A, "f15", "F15"},       {0x6B, "f16", "F16"},
    {0x6C, "f17", "F17"},       {0x6D, "f18", "F18"},
    {0x6E, "f19", "F19"},       {0x6F, "f20", "F20"},
    {0x70, "f21", "F21"},       {0x71, "f22", "F22"},
    {0x72, "f23", "F23"},       {0x73, "f24", "F24"},
};

const NamedKey kModifiers[] = {
    {0x01, "left_control", "LCtrl"},
    {0x02, "left_shift", "LShift"},
    {0x04, "left_alt", "LAlt"},
    {0x08, "left_gui", "LGui"},
    {0x10, "right_control", "RCtrl"},
    {0x20, "right_shift", "RShift"},
    {0x40, "right_alt", "RAlt"},
    {0x80, "right_gui", "RGui"},
};

bool lookupNamedKey(uint8_t usage, KeyLabel* out) {
  for (size_t i = 0; i < sizeof(kNamedKeys) / sizeof(kNamedKeys[0]); ++i) {
    if (kNamedKeys[i].usage == usage) {
      copyToken(out->speech_token, sizeof(out->speech_token), kNamedKeys[i].token);
      copyDisplay(out->display, sizeof(out->display), kNamedKeys[i].display);
      return true;
    }
  }
  return false;
}

bool lookupPhysicalKey(uint8_t usage, KeyLabel* out) {
  for (size_t i = 0; i < sizeof(kPhysicalKeys) / sizeof(kPhysicalKeys[0]); ++i) {
    if (kPhysicalKeys[i].usage == usage) {
      char token[24];
      snprintf(token, sizeof(token), "key_%s", kPhysicalKeys[i].suffix);
      copyToken(out->speech_token, sizeof(out->speech_token), token);
      copyDisplay(out->display, sizeof(out->display), kPhysicalKeys[i].display);
      return true;
    }
  }

  char fallback_token[24];
  snprintf(fallback_token, sizeof(fallback_token), "key_0x%02x", usage);
  copyToken(out->speech_token, sizeof(out->speech_token), fallback_token);
  snprintf(out->display, sizeof(out->display), "0x%02X", usage);
  return true;
}

KeyboardDetectedLayout effectiveLayout() { return detected_layout; }

bool layoutCharMapping(uint8_t key, bool shift, KeyboardDetectedLayout layout,
                       KeyLabel* out) {
  if (layout == KeyboardDetectedLayout::kUk) {
    return ukCharMapping(key, shift, out);
  }
  if (layout == KeyboardDetectedLayout::kUs) {
    return usCharMapping(key, shift, out);
  }
  return false;
}

}  // namespace

void keyboardLayoutSetDetected(KeyboardDetectedLayout layout) {
  detected_layout = layout;
}

KeyboardDetectedLayout keyboardLayoutGetDetected() { return detected_layout; }

void keyboardLayoutApplyUsbCountryCode(uint8_t country_code) {
  switch (country_code) {
    case kHidCountryUs:
    case kHidCountryCanada:
      detected_layout = KeyboardDetectedLayout::kUs;
      break;
    case kHidCountryUk:
    case kHidCountryIreland:
      detected_layout = KeyboardDetectedLayout::kUk;
      break;
    default:
      break;
  }
}

bool keyboardLayoutIsModifierMask(uint8_t mod_mask) {
  return mod_mask == 0x01 || mod_mask == 0x02 || mod_mask == 0x04 ||
         mod_mask == 0x08 || mod_mask == 0x10 || mod_mask == 0x20 ||
         mod_mask == 0x40 || mod_mask == 0x80;
}

bool keyboardLayoutResolveModifier(uint8_t mod_mask, KeyLabel* out) {
  if (out == nullptr || !keyboardLayoutIsModifierMask(mod_mask)) {
    return false;
  }

  for (size_t i = 0; i < sizeof(kModifiers) / sizeof(kModifiers[0]); ++i) {
    if (kModifiers[i].usage == mod_mask) {
      copyToken(out->speech_token, sizeof(out->speech_token), kModifiers[i].token);
      copyDisplay(out->display, sizeof(out->display), kModifiers[i].display);
      return true;
    }
  }
  return false;
}

bool keyboardLayoutResolveKey(uint8_t mod, uint8_t key, KeyLabel* out) {
  if (out == nullptr || key == 0) {
    return false;
  }

  memset(out, 0, sizeof(*out));

  const KeyboardDetectedLayout layout = effectiveLayout();
  const bool shift = shiftHeld(mod);

  if (layout != KeyboardDetectedLayout::kUnknown) {
    if (layoutCharMapping(key, shift, layout, out)) {
      return true;
    }
    if (lookupNamedKey(key, out)) {
      return true;
    }
  }

  return lookupPhysicalKey(key, out);
}
