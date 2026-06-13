#pragma once

#include "echolocation_core/settings_model.h"

namespace echolocation {

class DeviceSettingsStore {
 public:
  bool load(SettingsModel& settings);
  bool save(const SettingsModel& settings);
  bool reset();
};

}  // namespace echolocation
