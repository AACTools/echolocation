#pragma once

#include <stdint.h>
#include <string>

namespace echolocation {

enum class LayoutType : uint8_t {
  kUnknown = 0,
  kUS = 1,
  kUK = 2,
};

struct BluetoothConfig {
  std::string keyboard_name;
  std::string computer_name;
};

struct SettingsModel {
  uint8_t volume = 80;
  uint32_t hold_duration_ms = 600;
  LayoutType layout = LayoutType::kUnknown;
  BluetoothConfig bluetooth;
  bool debug_enabled = false;
};

}  // namespace echolocation
