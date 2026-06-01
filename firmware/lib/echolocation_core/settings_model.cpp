#include "settings_model.h"

namespace echo {

DeviceSettings defaultDeviceSettings() {
  return DeviceSettings{};
}

bool isValidVolume(uint8_t volume_percent) {
  return volume_percent <= kMaxVolumePercent;
}

bool isValidHoldDuration(uint32_t hold_duration_ms) {
  return hold_duration_ms >= kMinHoldDurationMs &&
         hold_duration_ms <= kMaxHoldDurationMs;
}

void clampDeviceSettings(DeviceSettings& settings) {
  if (!isValidVolume(settings.volume_percent)) {
    settings.volume_percent = defaultDeviceSettings().volume_percent;
  }
  if (!isValidHoldDuration(settings.hold_duration_ms)) {
    settings.hold_duration_ms = defaultDeviceSettings().hold_duration_ms;
  }
}

}  // namespace echo
