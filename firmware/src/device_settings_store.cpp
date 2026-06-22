#include "device_settings_store.h"

#include "computer_output.h"
#include "key_audio.h"
#include "ui.h"

#include <Preferences.h>
#include <string.h>

namespace {

constexpr char kNamespace[] = "echolocation";
constexpr char kKeyVolume[] = "volume";
constexpr char kKeyHoldMs[] = "hold_ms";
constexpr char kKeyBleComputerName[] = "bt_pc";

Preferences prefs;

char ble_computer_name[16] = "echolocation";

}  // namespace

void deviceSettingsLoad() {
  prefs.begin(kNamespace, true);
  const uint8_t volume = prefs.getUChar(kKeyVolume, kDefaultVolume);
  const uint32_t hold_ms = prefs.getUInt(kKeyHoldMs, kDefaultHoldDurationMs);
  prefs.getString(kKeyBleComputerName, ble_computer_name, sizeof(ble_computer_name));
  prefs.end();

  if (ble_computer_name[0] == '\0') {
    strncpy(ble_computer_name, kDefaultBleComputerName, sizeof(ble_computer_name) - 1);
    ble_computer_name[sizeof(ble_computer_name) - 1] = '\0';
  }

  uiSetVolume(volume);
  uiSetHoldDurationMs(hold_ms);
  uiSetBleComputerName(ble_computer_name);

  computerOutputBleSetDeviceName(ble_computer_name);
  computerOutputBleSetEnabled(true);
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

void deviceSettingsSaveBleComputerName(const char* name) {
  if (name == nullptr) {
    return;
  }
  strncpy(ble_computer_name, name, sizeof(ble_computer_name) - 1);
  ble_computer_name[sizeof(ble_computer_name) - 1] = '\0';

  prefs.begin(kNamespace, false);
  prefs.putString(kKeyBleComputerName, ble_computer_name);
  prefs.end();

  computerOutputBleSetDeviceName(ble_computer_name);
}

const char* deviceSettingsGetBleComputerName() { return ble_computer_name; }
