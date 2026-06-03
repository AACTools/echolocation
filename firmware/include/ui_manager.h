#pragma once

#include <string>

#include "settings_model.h"

namespace echo {

enum class UiScreen {
  kMain,
  kSettings,
  kVolume,
  kBluetoothMenu,
  kBleKeyboard,
  kBleComputer,
  kHoldDuration,
  kFactoryResetConfirm,
};

class UiManager {
 public:
  void begin();
  void tick();

  void setCurrentKeyLabel(const char* label);
  void setBatteryPercent(int battery_percent);
  void setErrorMessage(const char* message);
  void clearError();

  UiScreen currentScreen() const;
  bool settingsChanged() const;
  DeviceSettings& editingSettings();
  void acknowledgeSettingsSaved();
  void refreshSettingsWidgets();

  bool bleScanRequested() const;
  void clearBleScanRequested();

  bool factoryResetConfirmed() const;
  void clearFactoryResetConfirmed();

  void navigateTo(UiScreen screen);
  void applyVolumePercent(uint8_t volume_percent);
  void applyHoldDurationMs(uint32_t hold_duration_ms);
  void triggerBleScan();
  void triggerFactoryResetConfirm();

 private:
#ifndef NATIVE_TEST
  void buildScreens();
  void refreshMainLabels();
  void refreshVolumeUi();
  void refreshHoldUi();
  void refreshBleComputerUi();
#endif

  UiScreen screen_ = UiScreen::kMain;
  std::string current_key_label_;
  std::string error_message_;
  int battery_percent_ = -1;
  DeviceSettings editing_settings_{};
  bool settings_changed_ = false;
  bool ble_scan_requested_ = false;
  bool factory_reset_confirmed_ = false;
};

}  // namespace echo
