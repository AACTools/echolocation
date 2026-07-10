#include "keyboard_input.h"

#include "computer_output.h"
#include "key_audio.h"
#include "keyboard_layout.h"
#include "ui.h"

#include <Arduino.h>
#include <string.h>

namespace {

uint8_t displayed_key = 0;
uint8_t displayed_mod = 0;
uint8_t held_key = 0;
uint8_t held_mod = 0;
bool box_shown = false;
bool key_sent_to_computer = false;
uint8_t current_report[8] = {};
uint8_t report_snapshot[8] = {};
uint8_t prev_modifiers = 0;
uint32_t last_report_change_ms = 0;

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

uint8_t activeKeyCountInReport(const uint8_t report[8]) {
  uint8_t count = 0;
  for (uint8_t i = 2; i < 8; ++i) {
    if (report[i] != 0 && report[i] != 1) {
      count++;
    }
  }
  return count;
}

bool onlyKeyInReport(const uint8_t report[8], uint8_t key) {
  if (activeKeyCountInReport(report) != 1) {
    return false;
  }
  for (uint8_t i = 2; i < 8; ++i) {
    if (report[i] != 0 && report[i] != 1) {
      return report[i] == key;
    }
  }
  return false;
}

void noteReportChanged(const uint8_t report[8]) {
  if (memcmp(report, report_snapshot, 8) == 0) {
    return;
  }
  memcpy(report_snapshot, report, 8);
  last_report_change_ms = millis();
  box_shown = false;
  uiSetKeyBoxOutline(false);
}

void announceLabel(const KeyLabel& label) {
  keyAudioPlayForToken(label.speech_token);
  uiSetPressedKey(label.display);
}

void keyboardInputOnModifierChange(uint8_t old_mod, uint8_t new_mod) {
  static const uint8_t kModifierBits[] = {0x01, 0x02, 0x04, 0x08,
                                          0x10, 0x20, 0x40, 0x80};
  for (size_t i = 0; i < sizeof(kModifierBits); ++i) {
    const uint8_t bit = kModifierBits[i];
    if ((old_mod & bit) == 0 && (new_mod & bit) != 0) {
      KeyLabel label;
      if (keyboardLayoutResolveModifier(bit, &label)) {
        announceLabel(label);
      }
    }
  }
}

}  // namespace

bool keyboardInputKeyToLabel(uint8_t mod, uint8_t key, char* out, size_t out_len) {
  KeyLabel label;
  if (!keyboardLayoutResolveKey(mod, key, &label)) {
    if (out_len > 0) {
      out[0] = '\0';
    }
    return false;
  }

  if (out == nullptr || out_len == 0) {
    return true;
  }

  strncpy(out, label.display, out_len - 1);
  out[out_len - 1] = '\0';
  return true;
}

void keyboardInputOnKeyDown(uint8_t mod, uint8_t key) {
  if (key == 0) {
    return;
  }

  KeyLabel label;
  if (!keyboardLayoutResolveKey(mod, key, &label)) {
    return;
  }

  const bool is_new_key = (key != displayed_key || mod != displayed_mod);
  if (is_new_key) {
    displayed_key = key;
    displayed_mod = mod;
    box_shown = false;
    uiSetKeyBoxOutline(false);
    uiSetPressedKey(label.display);
  }

  if (held_key != key || held_mod != mod) {
    held_key = key;
    held_mod = mod;
    if (!is_new_key) {
      box_shown = false;
      uiSetKeyBoxOutline(false);
    }
  }

  keyAudioPlayForToken(label.speech_token);
}

void keyboardInputOnKeyUp(uint8_t mod, uint8_t key) {
  if (key == held_key && mod == held_mod) {
    held_key = 0;
    held_mod = 0;
  }
}

void keyboardInputProcessBootReport(uint8_t* prev_state, const uint8_t* report,
                                    size_t len) {
  uint8_t normalized[8] = {};
  normalizeBootReport(report, len, normalized);

  if (normalized[2] == 1) {
    return;
  }

  const uint8_t modifiers = normalized[0];
  if (modifiers != prev_modifiers) {
    keyboardInputOnModifierChange(prev_modifiers, modifiers);
    prev_modifiers = modifiers;
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
      keyboardInputOnKeyDown(modifiers, normalized[i]);
    }
    if (!up && prev_state[i] != 0) {
      keyboardInputOnKeyUp(prev_state[0], prev_state[i]);
    }
  }

  memcpy(prev_state, normalized, 8);
  memcpy(current_report, normalized, 8);
  noteReportChanged(normalized);

  if (activeKeyCountInReport(normalized) == 0) {
    held_key = 0;
    held_mod = 0;
    box_shown = false;
    key_sent_to_computer = false;
    uiSetKeyBoxOutline(false);
  }
}

void keyboardInputTick() {
  if (held_key == 0 || held_key != displayed_key || held_mod != displayed_mod) {
    return;
  }
  if (!onlyKeyInReport(current_report, held_key)) {
    return;
  }
  if (millis() - last_report_change_ms < uiGetHoldDurationMs()) {
    return;
  }

  if (!box_shown) {
    box_shown = true;
    uiSetKeyBoxOutline(true);
  }
  if (key_sent_to_computer) {
    return;
  }

  key_sent_to_computer = true;
  computerOutputSendKey(held_mod, held_key);
}
