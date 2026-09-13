#include "device_settings_store.h"

#include "ble_keyboard_input.h"
#include "key_audio.h"
#include "ui.h"

#include <Preferences.h>

namespace {

constexpr char kNamespace[] = "echolocation";
constexpr char kKeyVolume[] = "volume";
constexpr char kKeyHoldMs[] = "hold_ms";
constexpr char kKeyBluetoothOutput[] = "bt_output";
constexpr char kKeyBluetoothKeyboard[] = "bt_keyboard";
constexpr char kKeyShowKeyName[] = "show_key_name";

Preferences prefs;

}  // namespace

void deviceSettingsResetToFactory() {
  bleKeyboardInputClearKeyboardBonds();
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();
  deviceSettingsLoad();
}

void deviceSettingsLoad() {
  prefs.begin(kNamespace, true);
  const uint8_t volume = prefs.getUChar(kKeyVolume, kDefaultVolume);
  const uint32_t hold_ms = prefs.getUInt(kKeyHoldMs, kDefaultHoldDurationMs);
  const bool bluetooth_output =
      prefs.getBool(kKeyBluetoothOutput, kDefaultBluetoothOutput);
  const bool bluetooth_keyboard =
      prefs.getBool(kKeyBluetoothKeyboard, kDefaultBluetoothKeyboard);
  const bool show_key_name = prefs.getBool(kKeyShowKeyName, kDefaultShowKeyName);
  prefs.end();

  uiSetVolume(volume);
  uiSetHoldDurationMs(hold_ms);
  uiSetBluetoothOutput(bluetooth_output);
  uiSetBluetoothKeyboard(bluetooth_keyboard);
  uiSetShowKeyName(show_key_name);
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

void deviceSettingsSaveBluetoothOutput(bool enabled) {
  prefs.begin(kNamespace, false);
  prefs.putBool(kKeyBluetoothOutput, enabled);
  prefs.end();
}

void deviceSettingsSaveBluetoothKeyboard(bool enabled) {
  prefs.begin(kNamespace, false);
  prefs.putBool(kKeyBluetoothKeyboard, enabled);
  prefs.end();
}

void deviceSettingsSaveShowKeyName(bool enabled) {
  prefs.begin(kNamespace, false);
  prefs.putBool(kKeyShowKeyName, enabled);
  prefs.end();
}
