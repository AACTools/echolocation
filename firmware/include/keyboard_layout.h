#pragma once

#include <stddef.h>
#include <stdint.h>

// Layout inferred from a connected USB keyboard HID country code.
enum class KeyboardDetectedLayout : uint8_t {
  kUnknown = 0,
  kUs = 1,
  kUk = 2,
};

struct KeyLabel {
  char speech_token[24];
  char display[8];
};

void keyboardLayoutSetDetected(KeyboardDetectedLayout layout);
KeyboardDetectedLayout keyboardLayoutGetDetected();

// USB HID bCountryCode values (HID Usage Tables).
void keyboardLayoutApplyUsbCountryCode(uint8_t country_code);

bool keyboardLayoutResolveKey(uint8_t mod, uint8_t key, KeyLabel* out);
bool keyboardLayoutResolveModifier(uint8_t mod_mask, KeyLabel* out);

// Resolve a config token (e.g. "space", "a", "left_shift") to HID usage or
// modifier bit. Exactly one of out_key or out_mod_bit is set on success.
bool keyboardLayoutResolveToken(const char* token, uint8_t* out_key,
                                uint8_t* out_mod_bit);

// Single modifier bit: 0x01 LCtrl, 0x02 LShift, 0x04 LAlt, 0x08 LGui,
// 0x10 RCtrl, 0x20 RShift, 0x40 RAlt, 0x80 RGui.
bool keyboardLayoutIsModifierMask(uint8_t mod_mask);
