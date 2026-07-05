#include "device_settings_store.h"

#include "key_audio.h"
#include "ui.h"

#include <Preferences.h>

namespace {

constexpr char kNamespace[] = "echolocation";
constexpr char kKeyVolume[] = "volume";
constexpr char kKeyHoldMs[] = "hold_ms";

Preferences prefs;

}  // namespace

void deviceSettingsResetToFactory() {
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();
  deviceSettingsLoad();
}

void deviceSettingsLoad() {
  prefs.begin(kNamespace, true);
  const uint8_t volume = prefs.getUChar(kKeyVolume, kDefaultVolume);
  const uint32_t hold_ms = prefs.getUInt(kKeyHoldMs, kDefaultHoldDurationMs);
  prefs.end();

  uiSetVolume(volume);
  uiSetHoldDurationMs(hold_ms);
}

void deviceSettingsSaveVolume(uint8_t volume) {
  prefs.begin(kNamespace, false);
  prefs.putUChar(kKeyVolume, volume);
  prefs.end();
}

void deviceSettingsSaveHoldDurationMs(uint32_t ms) {
  prefs.begin(kNamespace, false);
  prefs.putUInt(kKeyHoldMs, ms);
  prefs.end();
}
