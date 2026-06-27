#include "keyboard_input.h"

#include "computer_output.h"
#include "key_audio.h"
#include "ui.h"
#include "usb_keyboard.h"

#include <Arduino.h>

#include <string.h>

namespace {

uint8_t displayed_key = 0;
uint8_t displayed_mod = 0;
uint8_t held_key = 0;
uint8_t held_mod = 0;
unsigned long key_pressed_at = 0;
bool box_shown = false;
bool key_sent_to_computer = false;

const char* hidKeyName(uint8_t key) {
  switch (key) {
    case 0x28:
      return "Enter";
    case 0x29:
      return "Esc";
    case 0x2A:
      return "Backspace";
    case 0x2B:
      return "Tab";
    case 0x2C:
      return "Space";
    case 0x39:
      return "Caps";
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
      return "Right";
    case 0x50:
      return "Left";
    case 0x51:
      return "Down";
    case 0x52:
      return "Up";
    case 0x53:
      return "Num";
    default:
      return nullptr;
  }
}

uint8_t hidKeyToAscii(uint8_t mod, uint8_t key) {
  const bool shift = (mod & 0x22) != 0;

  if (key >= 0x04 && key <= 0x1D) {
    const char base = static_cast<char>('a' + (key - 0x04));
    return shift ? static_cast<uint8_t>(base - 32) : static_cast<uint8_t>(base);
  }
  if (key >= 0x1E && key <= 0x27) {
    const char digits[] = "1234567890";
    if (!shift) {
      return static_cast<uint8_t>(digits[key - 0x1E]);
    }
    const char shifted[] = "!@#$%^&*()";
    return static_cast<uint8_t>(shifted[key - 0x1E]);
  }

  switch (key) {
    case 0x2C:
      return ' ';
    case 0x2D:
      return shift ? '_' : '-';
    case 0x2E:
      return shift ? '+' : '=';
    case 0x2F:
      return shift ? '{' : '[';
    case 0x30:
      return shift ? '}' : ']';
    case 0x31:
      return shift ? '|' : '\\';
    case 0x33:
      return shift ? '"' : '\'';
    case 0x34:
      return shift ? ':' : ';';
    case 0x35:
      return shift ? '?' : '/';
    case 0x36:
      return shift ? '~' : '`';
  case 0x37:
      return shift ? '>' : ',';
    case 0x38:
      return shift ? '<' : '.';
    default:
      return 0;
  }
}

bool sourceAllowed(KeyboardInputSource source) {
  if (source == KeyboardInputSource::kBle && usbKeyboardIsConnected()) {
    return false;
  }
  return true;
}

void normalizeBootReport(const uint8_t* report, size_t len, uint8_t normalized[8]) {
  memset(normalized, 0, 8);

  const bool has_report_id =
      len > 8 || (len >= 8 && report[0] != 0 && report[2] == 0 && report[3] != 0);

  if (has_report_id) {
    normalized[0] = report[1];
    normalized[1] = report[2];
    for (uint8_t i = 0; i < 6; ++i) {
      const uint8_t src = static_cast<uint8_t>(3 + i);
      normalized[static_cast<uint8_t>(2 + i)] = src < len ? report[src] : 0;
    }
  } else {
    const size_t copy_len = len < 8 ? len : 8;
    memcpy(normalized, report, copy_len);
  }
}

}  // namespace

bool keyboardInputKeyToLabel(uint8_t mod, uint8_t key, char* out, size_t out_len) {
  if (key == 0 || out_len == 0) {
    if (out_len > 0) {
      out[0] = '\0';
    }
    return false;
  }

  const char* name = hidKeyName(key);
  if (name != nullptr) {
    strncpy(out, name, out_len - 1);
    out[out_len - 1] = '\0';
    return true;
  }

  const uint8_t ascii = hidKeyToAscii(mod, key);
  if (ascii >= 0x20 && ascii <= 0x7E) {
    out[0] = static_cast<char>(ascii);
    out[1] = '\0';
    return true;
  }

  out[0] = '\0';
  return false;
}

void keyboardInputOnKeyDown(KeyboardInputSource source, uint8_t mod, uint8_t key) {
  if (!sourceAllowed(source) || key == 0) {
    return;
  }

  char label[16];
  if (!keyboardInputKeyToLabel(mod, key, label, sizeof(label))) {
    return;
  }

  const bool is_new_key = (key != displayed_key || mod != displayed_mod);
  if (is_new_key) {
    displayed_key = key;
    displayed_mod = mod;
    box_shown = false;
    key_sent_to_computer = false;
    uiSetKeyBoxOutline(false);
    uiSetPressedKey(label);
  }

  if (held_key != key || held_mod != mod) {
    held_key = key;
    held_mod = mod;
    key_pressed_at = millis();
    if (!is_new_key) {
      box_shown = false;
      key_sent_to_computer = false;
      uiSetKeyBoxOutline(false);
    }
  }

  keyAudioPlayForLabel(label);
}

void keyboardInputOnKeyUp(KeyboardInputSource source, uint8_t mod, uint8_t key) {
  if (!sourceAllowed(source)) {
    return;
  }

  if (key == held_key && mod == held_mod) {
    held_key = 0;
    held_mod = 0;
  }
}

void keyboardInputProcessBootReport(KeyboardInputSource source, uint8_t* prev_state,
                                    const uint8_t* report, size_t len) {
  if (!sourceAllowed(source)) {
    return;
  }

  uint8_t normalized[8] = {};
  normalizeBootReport(report, len, normalized);

  if (normalized[2] == 1) {
    return;
  }

  for (uint8_t i = 2; i < 8; i++) {
    bool down = false;
    bool up = false;

    for (uint8_t j = 2; j < 8; j++) {
      if (normalized[i] == prev_state[j] && normalized[i] != 1) {
        down = true;
      }
      if (normalized[j] == prev_state[i] && prev_state[i] != 1) {
        up = true;
      }
    }
    if (!down && normalized[i] != 0) {
      keyboardInputOnKeyDown(source, normalized[0], normalized[i]);
    }
    if (!up && prev_state[i] != 0) {
      keyboardInputOnKeyUp(source, prev_state[0], prev_state[i]);
    }
  }

  memcpy(prev_state, normalized, 8);
}

void keyboardInputTick() {
  if (held_key != 0 && !box_shown && held_key == displayed_key &&
      held_mod == displayed_mod &&
      millis() - key_pressed_at >= uiGetHoldDurationMs()) {
    box_shown = true;
    uiSetKeyBoxOutline(true);
    if (!key_sent_to_computer) {
      computerOutputSendKey(held_mod, held_key);
      key_sent_to_computer = true;
    }
  }
}
