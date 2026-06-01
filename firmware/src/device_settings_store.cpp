#include "device_settings_store.h"

#ifndef NATIVE_TEST
#include <NimBLEDevice.h>
#include <Preferences.h>
#endif

namespace echo {

bool DeviceSettingsStore::begin() {
#ifndef NATIVE_TEST
  Preferences preferences;
  return preferences.begin(kNamespace, false);
#else
  return true;
#endif
}

void DeviceSettingsStore::load(DeviceSettings& settings) {
  settings = defaultDeviceSettings();
#ifndef NATIVE_TEST
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return;
  }
  settings.volume_percent =
      static_cast<uint8_t>(preferences.getUChar("volume", settings.volume_percent));
  settings.hold_duration_ms =
      preferences.getUInt("hold_ms", settings.hold_duration_ms);
  settings.ble_keyboard_enabled =
      preferences.getBool("ble_kbd_en", settings.ble_keyboard_enabled);
  settings.ble_computer_enabled =
      preferences.getBool("ble_pc_en", settings.ble_computer_enabled);
  preferences.getString("ble_kbd_name", settings.ble_keyboard_name,
                        sizeof(settings.ble_keyboard_name));
  preferences.getString("ble_pc_name", settings.ble_computer_name,
                        sizeof(settings.ble_computer_name));
  preferences.end();
#endif
  clampDeviceSettings(settings);
}

void DeviceSettingsStore::save(const DeviceSettings& settings) {
#ifndef NATIVE_TEST
  DeviceSettings to_save = settings;
  clampDeviceSettings(to_save);
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return;
  }
  preferences.putUChar("volume", to_save.volume_percent);
  preferences.putUInt("hold_ms", to_save.hold_duration_ms);
  preferences.putBool("ble_kbd_en", to_save.ble_keyboard_enabled);
  preferences.putBool("ble_pc_en", to_save.ble_computer_enabled);
  preferences.putString("ble_kbd_name", to_save.ble_keyboard_name);
  preferences.putString("ble_pc_name", to_save.ble_computer_name);
  preferences.end();
#endif
}

void DeviceSettingsStore::factoryReset() {
#ifndef NATIVE_TEST
  Preferences preferences;
  preferences.begin(kNamespace, false);
  preferences.clear();
  preferences.end();
  NimBLEDevice::deleteAllBonds();
#endif
}

}  // namespace echo
