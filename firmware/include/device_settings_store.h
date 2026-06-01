#pragma once

#include "settings_model.h"

namespace echo {

class DeviceSettingsStore {
 public:
  bool begin();
  void load(DeviceSettings& settings);
  void save(const DeviceSettings& settings);
  void factoryReset();

 private:
  static constexpr const char* kNamespace = "echolocation";
};

}  // namespace echo
