#include "key_config.h"

#include "key_config_parse.h"
#include "keyboard_layout.h"
#include "key_audio.h"

#include <SD.h>

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr char kConfigPath[] = "/config/keys.txt";
constexpr size_t kMaxEntries = 64;

struct StoredEntry {
  char name[24];
  uint8_t key = 0;
  uint8_t mod_bit = 0;
  KeyBehavior behavior;
};

StoredEntry entries[kMaxEntries];
size_t entry_count = 0;
KeyConfigLoadStatus load_status = KeyConfigLoadStatus::kNoSd;

KeyBehavior defaultBehavior() { return KeyBehavior{}; }

const StoredEntry* findKeyEntry(uint8_t key) {
  for (size_t i = 0; i < entry_count; ++i) {
    if (entries[i].mod_bit == 0 && entries[i].key == key) {
      return &entries[i];
    }
  }
  return nullptr;
}

const StoredEntry* findModifierEntry(uint8_t mod_bit) {
  for (size_t i = 0; i < entry_count; ++i) {
    if (entries[i].mod_bit == mod_bit) {
      return &entries[i];
    }
  }
  return nullptr;
}

bool addEntry(const char* name, uint8_t key, uint8_t mod_bit,
              const KeyBehavior& behavior) {
  if (entry_count >= kMaxEntries || name == nullptr) {
    return false;
  }

  StoredEntry& entry = entries[entry_count++];
  strncpy(entry.name, name, sizeof(entry.name) - 1);
  entry.name[sizeof(entry.name) - 1] = '\0';
  entry.key = key;
  entry.mod_bit = mod_bit;
  entry.behavior = behavior;
  return true;
}

void clearEntries() {
  entry_count = 0;
}

}  // namespace

void keyConfigLoad() {
  clearEntries();

  KeyAudioDebugInfo audio_info;
  keyAudioGetDebugInfo(&audio_info);
  if (!audio_info.sd_mounted) {
    load_status = KeyConfigLoadStatus::kNoSd;
    Serial.println("[keycfg] SD not available");
    return;
  }

  if (!SD.exists(kConfigPath)) {
    load_status = KeyConfigLoadStatus::kFileMissing;
    Serial.println("[keycfg] /config/keys.txt not found (defaults)");
    return;
  }

  File file = SD.open(kConfigPath, FILE_READ);
  if (!file) {
    load_status = KeyConfigLoadStatus::kParseError;
    Serial.println("[keycfg] failed to open /config/keys.txt");
    return;
  }

  bool saw_error = false;
  char line[128];

  while (file.available()) {
    const size_t len = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';
    if (len > 0 && line[len - 1] == '\r') {
      line[len - 1] = '\0';
    }

    char name[24];
    KeyBehavior behavior;
    if (!keyConfigParseLine(line, name, sizeof(name), &behavior)) {
      continue;
    }

    uint8_t key = 0;
    uint8_t mod_bit = 0;
    if (!keyboardLayoutResolveToken(name, &key, &mod_bit)) {
      Serial.printf("[keycfg] unknown key token: %s\n", name);
      saw_error = true;
      continue;
    }

    if (!addEntry(name, key, mod_bit, behavior)) {
      Serial.println("[keycfg] override table full");
      saw_error = true;
      break;
    }
  }

  file.close();
  load_status = saw_error ? KeyConfigLoadStatus::kParseError : KeyConfigLoadStatus::kOk;
  Serial.printf("[keycfg] loaded %u overrides\n", static_cast<unsigned>(entry_count));
}

KeyBehavior keyConfigForKey(uint8_t key) {
  const StoredEntry* entry = findKeyEntry(key);
  if (entry == nullptr) {
    return defaultBehavior();
  }
  return entry->behavior;
}

KeyBehavior keyConfigForModifier(uint8_t mod_bit) {
  const StoredEntry* entry = findModifierEntry(mod_bit);
  if (entry == nullptr) {
    return defaultBehavior();
  }
  return entry->behavior;
}

int keyConfigLoadedEntryCount() { return static_cast<int>(entry_count); }

size_t keyConfigGetEntries(KeyConfigEntry* out, size_t max) {
  if (out == nullptr || max == 0) {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i < entry_count && count < max; ++i) {
    strncpy(out[count].name, entries[i].name, sizeof(out[count].name) - 1);
    out[count].name[sizeof(out[count].name) - 1] = '\0';
    out[count].behavior = entries[i].behavior;
    ++count;
  }
  return count;
}

KeyConfigLoadStatus keyConfigGetLoadStatus() { return load_status; }
