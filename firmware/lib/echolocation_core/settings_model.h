#pragma once

#include <cstdint>

namespace echo {

struct DeviceSettings {
  uint8_t volume_percent = 80;
  uint32_t hold_duration_ms = 500;
  bool ble_keyboard_enabled = true;
  bool ble_computer_enabled = true;
  char ble_keyboard_name[32] = "";
  char ble_computer_name[32] = "echolocation";
};

constexpr uint8_t kMinVolumePercent = 0;
constexpr uint8_t kMaxVolumePercent = 100;
constexpr uint32_t kMinHoldDurationMs = 200;
constexpr uint32_t kMaxHoldDurationMs = 2000;

DeviceSettings defaultDeviceSettings();
bool isValidVolume(uint8_t volume_percent);
bool isValidHoldDuration(uint32_t hold_duration_ms);
void clampDeviceSettings(DeviceSettings& settings);

}  // namespace echo
