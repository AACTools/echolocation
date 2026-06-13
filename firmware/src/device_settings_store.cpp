#include "device_settings_store.h"

#include "event_logger.h"

#ifdef ARDUINO
#include <Preferences.h>
#endif

namespace echolocation {

bool DeviceSettingsStore::load(SettingsModel& settings) {
#ifdef ARDUINO
  Preferences prefs;
  if (!prefs.begin("echolocation", true)) {
    EventLogger::instance().log(LogLevel::kError, "Settings load failed.");
    return false;
  }
  settings.volume = prefs.getUChar("volume", settings.volume);
  settings.hold_duration_ms = prefs.getUInt("hold_ms", settings.hold_duration_ms);
  settings.layout = static_cast<LayoutType>(
      prefs.getUChar("layout", static_cast<uint8_t>(settings.layout)));
  settings.bluetooth.keyboard_name = prefs.getString("bt_kb", "").c_str();
  settings.bluetooth.computer_name = prefs.getString("bt_pc", "").c_str();
  settings.debug_enabled = prefs.getBool("debug", settings.debug_enabled);
  prefs.end();
#else
  (void)settings;
#endif

  EventLogger::instance().log(LogLevel::kInfo, "Settings loaded.");
  return true;
}

bool DeviceSettingsStore::save(const SettingsModel& settings) {
#ifdef ARDUINO
  Preferences prefs;
  if (!prefs.begin("echolocation", false)) {
    EventLogger::instance().log(LogLevel::kError, "Settings save failed.");
    return false;
  }
  prefs.putUChar("volume", settings.volume);
  prefs.putUInt("hold_ms", settings.hold_duration_ms);
  prefs.putUChar("layout", static_cast<uint8_t>(settings.layout));
  prefs.putString("bt_kb", settings.bluetooth.keyboard_name.c_str());
  prefs.putString("bt_pc", settings.bluetooth.computer_name.c_str());
  prefs.putBool("debug", settings.debug_enabled);
  prefs.end();
#else
  (void)settings;
#endif

  EventLogger::instance().log(LogLevel::kInfo, "Settings saved.");
  return true;
}

bool DeviceSettingsStore::reset() {
#ifdef ARDUINO
  Preferences prefs;
  if (!prefs.begin("echolocation", false)) {
    EventLogger::instance().log(LogLevel::kError, "Settings reset failed.");
    return false;
  }
  prefs.clear();
  prefs.end();
#endif

  EventLogger::instance().log(LogLevel::kWarn, "Settings reset.");
  return true;
}

}  // namespace echolocation
