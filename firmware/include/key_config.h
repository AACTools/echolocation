#pragma once

#include <stddef.h>
#include <stdint.h>

struct KeyBehavior {
  bool echo_enabled = true;
  bool hold_enabled = true;
};

struct KeyConfigEntry {
  char name[24];
  KeyBehavior behavior;
};

enum class KeyConfigLoadStatus : uint8_t {
  kOk = 0,
  kNoSd,
  kFileMissing,
  kParseError,
};

void keyConfigLoad();
KeyBehavior keyConfigForKey(uint8_t key);
KeyBehavior keyConfigForModifier(uint8_t mod_bit);
int keyConfigLoadedEntryCount();
size_t keyConfigGetEntries(KeyConfigEntry* out, size_t max);
KeyConfigLoadStatus keyConfigGetLoadStatus();

bool keyConfigHasOverrides(const KeyBehavior& behavior);
void keyConfigFormatOverrideSummary(const KeyBehavior& behavior, char* out,
                                    size_t out_len);
void keyConfigFormatEntrySummary(const KeyConfigEntry& entry, char* out,
                                 size_t out_len);
