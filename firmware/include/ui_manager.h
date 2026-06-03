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
  void setCurrentKeyLabel(const char* label);
  void setBatteryPercent(int battery_percent);
  void setErrorMessage(const char* message);
  void clearError();
  void draw();
  void handleTouch();

  UiScreen currentScreen() const;
  bool settingsChanged() const;
  DeviceSettings& editingSettings();
  void acknowledgeSettingsSaved();

 private:
  void drawMain();
  void drawSettings();
  void drawVolume();
  void drawBluetoothMenu();
  void drawBleKeyboard();
  void drawBleComputer();
  void drawHoldDuration();
  void drawFactoryReset();
  bool touchInRect(int x, int y, int w, int h, int touch_x, int touch_y) const;

  UiScreen screen_ = UiScreen::kMain;
  std::string current_key_label_ = "";
  std::string error_message_ = "";
  int battery_percent_ = -1;
  DeviceSettings editing_settings_{};
  bool settings_changed_ = false;
};

}  // namespace echo
