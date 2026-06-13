#pragma once

#include <functional>
#include <string>

#include "settings_model.h"

#ifndef NATIVE_TEST
#include <lvgl.h>
#endif

namespace echo {

enum class UiScreen {
  kMain,
  kSettings,
  kVolume,
  kBluetoothMenu,
  kBleKeyboard,
  kBleComputer,
  kHoldDuration,
  kDebug,
  kFactoryResetConfirm,
};

class UiManager {
 public:
  void begin();
  void setCurrentKeyLabel(const char* label);
  void requestImmediateKeyLabel(const char* label);
  void setBatteryPercent(int battery_percent);
  void setErrorMessage(const char* message);
  void clearError();
  void setDebugInfo(const char* info);
  void draw();
  void handleTouch();

  void setOnBleScanRequested(std::function<void()> callback);
  void setOnFactoryResetConfirmed(std::function<void()> callback);

  UiScreen currentScreen() const;
  bool settingsChanged() const;
  DeviceSettings& editingSettings();
  void acknowledgeSettingsSaved();

 private:
#ifndef NATIVE_TEST
  void buildScreens();
  void showScreen(UiScreen screen);
  lv_obj_t* createHeader(lv_obj_t* parent, const char* title, UiScreen back_to);
  void updateBatteryDisplay();
  void updateKeyLabel();
  void updateErrorDisplay();
  void updateVolumeDisplay();
  void updateHoldDisplay();
  void updateBleComputerDisplay();
  void updateDebugDisplay();

  static void onSettingsClicked(lv_event_t* event);
  static void onBackClicked(lv_event_t* event);
  static void onListNavigate(lv_event_t* event);
  static void onVolumeDecrease(lv_event_t* event);
  static void onVolumeIncrease(lv_event_t* event);
  static void onHoldDecrease(lv_event_t* event);
  static void onHoldIncrease(lv_event_t* event);
  static void onBleScanClicked(lv_event_t* event);
  static void onFactoryResetConfirm(lv_event_t* event);
  static void onFactoryResetCancel(lv_event_t* event);

  lv_obj_t* screen_main_ = nullptr;
  lv_obj_t* screen_settings_ = nullptr;
  lv_obj_t* screen_volume_ = nullptr;
  lv_obj_t* screen_bluetooth_menu_ = nullptr;
  lv_obj_t* screen_ble_keyboard_ = nullptr;
  lv_obj_t* screen_ble_computer_ = nullptr;
  lv_obj_t* screen_hold_duration_ = nullptr;
  lv_obj_t* screen_debug_ = nullptr;
  lv_obj_t* screen_factory_reset_ = nullptr;

  lv_obj_t* key_label_ = nullptr;
  lv_obj_t* battery_bar_ = nullptr;
  lv_obj_t* battery_label_ = nullptr;
  lv_obj_t* error_label_ = nullptr;
  lv_obj_t* volume_value_label_ = nullptr;
  lv_obj_t* hold_value_label_ = nullptr;
  lv_obj_t* ble_computer_label_ = nullptr;
  lv_obj_t* debug_label_ = nullptr;
#endif

  UiScreen screen_ = UiScreen::kMain;
  std::string current_key_label_ = "";
  std::string error_message_ = "";
  std::string debug_info_ = "Debug info unavailable";
  int battery_percent_ = -1;
  DeviceSettings editing_settings_{};
  bool settings_changed_ = false;
  std::function<void()> on_ble_scan_requested_;
  std::function<void()> on_factory_reset_confirmed_;
};

}  // namespace echo
