#include "device_settings_store.h"

#include "ble_keyboard.h"
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
constexpr char kKeyBleComputerEnabled[] = "bt_pc_en";
constexpr char kKeyBleKeyboardEnabled[] = "bt_kb_en";
constexpr char kKeyBleKeyboardMac[] = "bt_kb_mac";
constexpr char kKeyBleKeyboardName[] = "bt_kb_name";
constexpr char kKeyBleKeyboardAddrType[] = "bt_kb_type";

Preferences prefs;

char ble_computer_name[16] = "echolocation";
bool ble_computer_enabled = true;
bool ble_keyboard_enabled = false;
uint8_t ble_keyboard_mac[6] = {};
uint8_t ble_keyboard_addr_type = 0;
char ble_keyboard_name[32] = "";

bool macIsValid(const uint8_t mac[6]) {
  for (int i = 0; i < 6; ++i) {
    if (mac[i] != 0) {
      return true;
    }
  }
  return false;
}

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
  prefs.getString(kKeyBleComputerName, ble_computer_name, sizeof(ble_computer_name));
  ble_computer_enabled = prefs.getBool(kKeyBleComputerEnabled, true);
  ble_keyboard_enabled = prefs.getBool(kKeyBleKeyboardEnabled, false);
  const size_t mac_len = prefs.getBytesLength(kKeyBleKeyboardMac);
  if (mac_len == 6) {
    prefs.getBytes(kKeyBleKeyboardMac, ble_keyboard_mac, sizeof(ble_keyboard_mac));
  } else {
    memset(ble_keyboard_mac, 0, sizeof(ble_keyboard_mac));
  }
  prefs.getString(kKeyBleKeyboardName, ble_keyboard_name, sizeof(ble_keyboard_name));
  ble_keyboard_addr_type = prefs.getUChar(kKeyBleKeyboardAddrType, 0);
  prefs.end();

  if (ble_computer_name[0] == '\0') {
    strncpy(ble_computer_name, kDefaultBleComputerName, sizeof(ble_computer_name) - 1);
    ble_computer_name[sizeof(ble_computer_name) - 1] = '\0';
  }

  uiSetVolume(volume);
  uiSetHoldDurationMs(hold_ms);
  uiSetBleComputerName(ble_computer_name);

  computerOutputBleSetDeviceName(ble_computer_name);
  computerOutputBleSetEnabled(ble_computer_enabled);

  bleKeyboardSetEnabled(ble_keyboard_enabled);
  if (ble_keyboard_enabled && macIsValid(ble_keyboard_mac)) {
    bleKeyboardSetSavedDevice(ble_keyboard_mac, ble_keyboard_name,
                              ble_keyboard_addr_type);
  }
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

void deviceSettingsSaveBleComputerEnabled(bool enabled) {
  ble_computer_enabled = enabled;
  prefs.begin(kNamespace, false);
  prefs.putBool(kKeyBleComputerEnabled, enabled);
  prefs.end();
}

bool deviceSettingsGetBleComputerEnabled() { return ble_computer_enabled; }

void deviceSettingsSaveBleKeyboardEnabled(bool enabled) {
  ble_keyboard_enabled = enabled;
  prefs.begin(kNamespace, false);
  prefs.putBool(kKeyBleKeyboardEnabled, enabled);
  prefs.end();
}

bool deviceSettingsGetBleKeyboardEnabled() { return ble_keyboard_enabled; }

void deviceSettingsSaveBleKeyboardDevice(const uint8_t address[6], const char* name,
                                         uint8_t addr_type) {
  if (address == nullptr) {
    return;
  }

  memcpy(ble_keyboard_mac, address, sizeof(ble_keyboard_mac));
  ble_keyboard_addr_type = addr_type;
  if (name != nullptr) {
    strncpy(ble_keyboard_name, name, sizeof(ble_keyboard_name) - 1);
    ble_keyboard_name[sizeof(ble_keyboard_name) - 1] = '\0';
  } else {
    ble_keyboard_name[0] = '\0';
  }

  prefs.begin(kNamespace, false);
  prefs.putBytes(kKeyBleKeyboardMac, ble_keyboard_mac, sizeof(ble_keyboard_mac));
  prefs.putString(kKeyBleKeyboardName, ble_keyboard_name);
  prefs.putUChar(kKeyBleKeyboardAddrType, ble_keyboard_addr_type);
  prefs.end();
}

bool deviceSettingsGetBleKeyboardAddress(uint8_t address_out[6]) {
  if (address_out == nullptr || !macIsValid(ble_keyboard_mac)) {
    return false;
  }
  memcpy(address_out, ble_keyboard_mac, 6);
  return true;
}

void deviceSettingsGetBleKeyboardName(char* name_out, size_t name_len) {
  if (name_out == nullptr || name_len == 0) {
    return;
  }
  strncpy(name_out, ble_keyboard_name, name_len - 1);
  name_out[name_len - 1] = '\0';
}

bool deviceSettingsHasBleKeyboardDevice() { return macIsValid(ble_keyboard_mac); }

uint8_t deviceSettingsGetBleKeyboardAddressType() { return ble_keyboard_addr_type; }
