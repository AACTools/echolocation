#include "ui_manager.h"

#include "lvgl_port.h"

#ifndef NATIVE_TEST
#include <cstdio>
#include <cstring>
#include <lvgl.h>
#endif

namespace echo {

namespace {

#ifndef NATIVE_TEST
const lv_color_t kBgColor = lv_color_hex(0x1A1A1A);
const lv_color_t kHeaderColor = lv_color_hex(0x2A2A2A);
const lv_color_t kAccentColor = lv_color_hex(0x0066FF);
const lv_color_t kErrorColor = lv_color_hex(0xFF4444);

void styleScreen(lv_obj_t* screen) {
  lv_obj_set_style_bg_color(screen, kBgColor, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* createStepperButton(lv_obj_t* parent, const char* text, int x_ofs) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_size(button, 56, 56);
  lv_obj_align(button, LV_ALIGN_CENTER, x_ofs, 24);
  lv_obj_set_style_radius(button, 12, 0);
  lv_obj_set_style_bg_color(button, kAccentColor, 0);

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return button;
}
#endif

}  // namespace

void UiManager::begin() {
  editing_settings_ = defaultDeviceSettings();
#ifndef NATIVE_TEST
  lvglPortInit();
  buildScreens();
  showScreen(UiScreen::kLoading);
#endif
}

void UiManager::setLoadingProgress(int loaded, int total) {
  if (loaded == loading_loaded_ && total == loading_total_) {
    return;
  }
  loading_loaded_ = loaded;
  loading_total_ = total;
#ifndef NATIVE_TEST
  updateLoadingDisplay();
#endif
}

void UiManager::finishLoading() {
#ifndef NATIVE_TEST
  showScreen(UiScreen::kMain);
#endif
}

void UiManager::setCurrentKeyLabel(const char* label) {
  const std::string new_label = label ? label : "";
  if (new_label == current_key_label_) {
    return;
  }
  current_key_label_ = new_label;
#ifndef NATIVE_TEST
  updateKeyLabel();
#endif
}

#ifndef NATIVE_TEST
namespace {

struct ImmediateKeyLabelRequest {
  UiManager* manager = nullptr;
  char label[32] = {};
};

void onImmediateKeyLabelAsync(void* user_data) {
  auto* request = static_cast<ImmediateKeyLabelRequest*>(user_data);
  if (request != nullptr && request->manager != nullptr) {
    request->manager->setCurrentKeyLabel(request->label);
  }
  delete request;
}

}  // namespace
#endif

void UiManager::requestImmediateKeyLabel(const char* label) {
#ifndef NATIVE_TEST
  auto* request = new ImmediateKeyLabelRequest();
  request->manager = this;
  const char* safe_label = label ? label : "";
  std::strncpy(request->label, safe_label, sizeof(request->label) - 1);
  request->label[sizeof(request->label) - 1] = '\0';
  if (lv_async_call(onImmediateKeyLabelAsync, request) != LV_RESULT_OK) {
    delete request;
    setCurrentKeyLabel(label);
  }
#else
  setCurrentKeyLabel(label);
#endif
}

void UiManager::setBatteryPercent(int battery_percent) {
  if (battery_percent == battery_percent_) {
    return;
  }
  battery_percent_ = battery_percent;
#ifndef NATIVE_TEST
  updateBatteryDisplay();
#endif
}

void UiManager::setErrorMessage(const char* message) {
  const std::string new_message = message ? message : "";
  if (new_message == error_message_) {
    return;
  }
  error_message_ = new_message;
#ifndef NATIVE_TEST
  updateErrorDisplay();
#endif
}

void UiManager::clearError() {
  if (error_message_.empty()) {
    return;
  }
  error_message_.clear();
#ifndef NATIVE_TEST
  updateErrorDisplay();
#endif
}

void UiManager::setDebugInfo(const char* info) {
  const std::string new_info = info ? info : "";
  if (new_info == debug_info_) {
    return;
  }
  debug_info_ = new_info;
#ifndef NATIVE_TEST
  updateDebugDisplay();
#endif
}

void UiManager::draw() {
#ifndef NATIVE_TEST
  // UI widgets are updated by state setters to avoid redundant redraw work.
#endif
}

void UiManager::handleTouch() {}

void UiManager::setOnBleScanRequested(std::function<void()> callback) {
  on_ble_scan_requested_ = std::move(callback);
}

void UiManager::setOnFactoryResetConfirmed(std::function<void()> callback) {
  on_factory_reset_confirmed_ = std::move(callback);
}

UiScreen UiManager::currentScreen() const { return screen_; }

bool UiManager::settingsChanged() const { return settings_changed_; }

DeviceSettings& UiManager::editingSettings() { return editing_settings_; }

void UiManager::acknowledgeSettingsSaved() { settings_changed_ = false; }

#ifndef NATIVE_TEST

void UiManager::showScreen(UiScreen screen) {
  screen_ = screen;
  lv_obj_t* target = nullptr;
  switch (screen) {
    case UiScreen::kLoading:
      target = screen_loading_;
      updateLoadingDisplay();
      break;
    case UiScreen::kMain:
      target = screen_main_;
      break;
    case UiScreen::kSettings:
      target = screen_settings_;
      break;
    case UiScreen::kVolume:
      target = screen_volume_;
      updateVolumeDisplay();
      break;
    case UiScreen::kBluetoothMenu:
      target = screen_bluetooth_menu_;
      break;
    case UiScreen::kBleKeyboard:
      target = screen_ble_keyboard_;
      break;
    case UiScreen::kBleComputer:
      target = screen_ble_computer_;
      updateBleComputerDisplay();
      break;
    case UiScreen::kHoldDuration:
      target = screen_hold_duration_;
      updateHoldDisplay();
      break;
    case UiScreen::kDebug:
      target = screen_debug_;
      updateDebugDisplay();
      break;
    case UiScreen::kFactoryResetConfirm:
      target = screen_factory_reset_;
      break;
  }
  if (target != nullptr) {
    lv_screen_load(target);
  }
}

lv_obj_t* UiManager::createHeader(lv_obj_t* parent, const char* title,
                                  UiScreen back_to) {
  lv_obj_t* header = lv_obj_create(parent);
  lv_obj_set_size(header, LV_PCT(100), 40);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, kHeaderColor, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_pad_hor(header, 8, 0);
  lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* back_button = lv_btn_create(header);
  lv_obj_set_size(back_button, 64, 28);
  lv_obj_align(back_button, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_radius(back_button, 8, 0);
  lv_obj_set_style_bg_color(back_button, lv_color_hex(0x3A3A3A), 0);

  lv_obj_t* back_label = lv_label_create(back_button);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);

  lv_obj_add_event_cb(back_button, onBackClicked, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(
      back_button,
      reinterpret_cast<void*>(static_cast<intptr_t>(back_to)));

  lv_obj_t* title_label = lv_label_create(header);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
  lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

  return header;
}

void UiManager::buildScreens() {
  screen_loading_ = lv_obj_create(nullptr);
  styleScreen(screen_loading_);

  lv_obj_t* loading_title = lv_label_create(screen_loading_);
  lv_label_set_text(loading_title, "echolocation");
  lv_obj_set_style_text_font(loading_title, &lv_font_montserrat_16, 0);
  lv_obj_align(loading_title, LV_ALIGN_TOP_MID, 0, 24);

  lv_obj_t* loading_status = lv_label_create(screen_loading_);
  lv_label_set_text(loading_status, "Loading audio...");
  lv_obj_set_style_text_font(loading_status, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(loading_status, lv_color_white(), 0);
  lv_obj_align(loading_status, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* loading_spinner = lv_spinner_create(screen_loading_);
  lv_obj_set_size(loading_spinner, 48, 48);
  lv_obj_align(loading_spinner, LV_ALIGN_CENTER, 0, 24);
  lv_obj_set_style_arc_color(loading_spinner, kAccentColor, LV_PART_INDICATOR);

  loading_bar_ = lv_bar_create(screen_loading_);
  lv_obj_set_size(loading_bar_, 220, 12);
  lv_obj_align(loading_bar_, LV_ALIGN_CENTER, 0, 72);
  lv_bar_set_range(loading_bar_, 0, 100);
  lv_obj_set_style_bg_color(loading_bar_, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(loading_bar_, kAccentColor, LV_PART_INDICATOR);

  loading_count_label_ = lv_label_create(screen_loading_);
  lv_label_set_text(loading_count_label_, "0 / 0");
  lv_obj_set_style_text_color(loading_count_label_, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(loading_count_label_, LV_ALIGN_CENTER, 0, 96);

  screen_main_ = lv_obj_create(nullptr);
  styleScreen(screen_main_);

  lv_obj_t* main_header = lv_obj_create(screen_main_);
  lv_obj_set_size(main_header, LV_PCT(100), 40);
  lv_obj_align(main_header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(main_header, kHeaderColor, 0);
  lv_obj_set_style_border_width(main_header, 0, 0);
  lv_obj_set_style_radius(main_header, 0, 0);
  lv_obj_set_style_pad_hor(main_header, 8, 0);
  lv_obj_remove_flag(main_header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* app_title = lv_label_create(main_header);
  lv_label_set_text(app_title, "echolocation");
  lv_obj_set_style_text_font(app_title, &lv_font_montserrat_16, 0);
  lv_obj_align(app_title, LV_ALIGN_LEFT_MID, 0, 0);

  battery_bar_ = lv_bar_create(main_header);
  lv_obj_set_size(battery_bar_, 80, 12);
  lv_obj_align(battery_bar_, LV_ALIGN_RIGHT_MID, -44, 0);
  lv_bar_set_range(battery_bar_, 0, 100);
  lv_obj_set_style_bg_color(battery_bar_, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
  lv_obj_set_style_bg_color(battery_bar_, kAccentColor, LV_PART_INDICATOR);

  battery_label_ = lv_label_create(main_header);
  lv_label_set_text(battery_label_, "--%");
  lv_obj_align(battery_label_, LV_ALIGN_RIGHT_MID, 0, 0);

  key_label_ = lv_label_create(screen_main_);
  lv_label_set_text(key_label_, "-");
  lv_obj_set_style_text_font(key_label_, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(key_label_, lv_color_white(), 0);
  lv_obj_align(key_label_, LV_ALIGN_CENTER, 0, -10);

  error_label_ = lv_label_create(screen_main_);
  lv_label_set_text(error_label_, "");
  lv_obj_set_width(error_label_, LV_PCT(90));
  lv_label_set_long_mode(error_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(error_label_, kErrorColor, 0);
  lv_obj_set_style_text_align(error_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(error_label_, LV_ALIGN_CENTER, 0, 48);
  lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* settings_button = lv_btn_create(screen_main_);
  lv_obj_set_size(settings_button, 96, 40);
  lv_obj_align(settings_button, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_set_style_radius(settings_button, 10, 0);
  lv_obj_set_style_bg_color(settings_button, kAccentColor, 0);
  lv_obj_add_event_cb(settings_button, onSettingsClicked, LV_EVENT_CLICKED,
                      this);

  lv_obj_t* settings_label = lv_label_create(settings_button);
  lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS " Settings");
  lv_obj_center(settings_label);

  screen_settings_ = lv_obj_create(nullptr);
  styleScreen(screen_settings_);
  createHeader(screen_settings_, "Settings", UiScreen::kMain);

  lv_obj_t* settings_list = lv_list_create(screen_settings_);
  lv_obj_set_width(settings_list, LV_PCT(100));
  lv_obj_set_height(settings_list, 200);
  lv_obj_align(settings_list, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(settings_list, kBgColor, 0);
  lv_obj_set_style_border_width(settings_list, 0, 0);

  lv_obj_t* volume_item =
      lv_list_add_button(settings_list, LV_SYMBOL_VOLUME_MAX, "Volume");
  lv_obj_add_event_cb(volume_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(volume_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kVolume)));

  lv_obj_t* bluetooth_item =
      lv_list_add_button(settings_list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
  lv_obj_add_event_cb(bluetooth_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(bluetooth_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kBluetoothMenu)));

  lv_obj_t* hold_item = lv_list_add_button(settings_list, LV_SYMBOL_SETTINGS,
                                           "Hold duration");
  lv_obj_add_event_cb(hold_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(hold_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kHoldDuration)));

  lv_obj_t* debug_item =
      lv_list_add_button(settings_list, LV_SYMBOL_EDIT, "Debug");
  lv_obj_add_event_cb(debug_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(debug_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kDebug)));

  lv_obj_t* reset_item =
      lv_list_add_button(settings_list, LV_SYMBOL_WARNING, "Factory reset");
  lv_obj_set_style_text_color(reset_item, kErrorColor, 0);
  lv_obj_add_event_cb(reset_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(reset_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kFactoryResetConfirm)));

  screen_volume_ = lv_obj_create(nullptr);
  styleScreen(screen_volume_);
  createHeader(screen_volume_, "Volume", UiScreen::kSettings);

  volume_value_label_ = lv_label_create(screen_volume_);
  lv_label_set_text(volume_value_label_, "Volume: 80%");
  lv_obj_set_style_text_font(volume_value_label_, &lv_font_montserrat_20, 0);
  lv_obj_align(volume_value_label_, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* minus_button = createStepperButton(screen_volume_, "-", -72);
  lv_obj_add_event_cb(minus_button, onVolumeDecrease, LV_EVENT_CLICKED, this);

  lv_obj_t* plus_button = createStepperButton(screen_volume_, "+", 72);
  lv_obj_add_event_cb(plus_button, onVolumeIncrease, LV_EVENT_CLICKED, this);

  screen_bluetooth_menu_ = lv_obj_create(nullptr);
  styleScreen(screen_bluetooth_menu_);
  createHeader(screen_bluetooth_menu_, "Bluetooth", UiScreen::kSettings);

  lv_obj_t* bluetooth_list = lv_list_create(screen_bluetooth_menu_);
  lv_obj_set_width(bluetooth_list, LV_PCT(100));
  lv_obj_set_height(bluetooth_list, 200);
  lv_obj_align(bluetooth_list, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(bluetooth_list, kBgColor, 0);
  lv_obj_set_style_border_width(bluetooth_list, 0, 0);

  lv_obj_t* keyboard_item =
      lv_list_add_button(bluetooth_list, LV_SYMBOL_KEYBOARD, "Keyboard");
  lv_obj_add_event_cb(keyboard_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(keyboard_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kBleKeyboard)));

  lv_obj_t* computer_item =
      lv_list_add_button(bluetooth_list, LV_SYMBOL_WIFI, "Computer");
  lv_obj_add_event_cb(computer_item, onListNavigate, LV_EVENT_CLICKED, this);
  lv_obj_set_user_data(computer_item,
                       reinterpret_cast<void*>(static_cast<intptr_t>(
                           UiScreen::kBleComputer)));

  screen_ble_keyboard_ = lv_obj_create(nullptr);
  styleScreen(screen_ble_keyboard_);
  createHeader(screen_ble_keyboard_, "BLE Keyboard", UiScreen::kBluetoothMenu);

  lv_obj_t* scan_hint = lv_label_create(screen_ble_keyboard_);
  lv_label_set_text(scan_hint, "Scan for nearby BLE keyboards");
  lv_obj_set_width(scan_hint, LV_PCT(90));
  lv_label_set_long_mode(scan_hint, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(scan_hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(scan_hint, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* scan_button = lv_btn_create(screen_ble_keyboard_);
  lv_obj_set_size(scan_button, 160, 44);
  lv_obj_align(scan_button, LV_ALIGN_CENTER, 0, 32);
  lv_obj_set_style_radius(scan_button, 10, 0);
  lv_obj_set_style_bg_color(scan_button, kAccentColor, 0);
  lv_obj_add_event_cb(scan_button, onBleScanClicked, LV_EVENT_CLICKED, this);

  lv_obj_t* scan_label = lv_label_create(scan_button);
  lv_label_set_text(scan_label, "Start scan");
  lv_obj_center(scan_label);

  screen_ble_computer_ = lv_obj_create(nullptr);
  styleScreen(screen_ble_computer_);
  createHeader(screen_ble_computer_, "BLE Computer", UiScreen::kBluetoothMenu);

  lv_obj_t* computer_hint = lv_label_create(screen_ble_computer_);
  lv_label_set_text(computer_hint, "Device advertises as:");
  lv_obj_align(computer_hint, LV_ALIGN_CENTER, 0, -24);

  ble_computer_label_ = lv_label_create(screen_ble_computer_);
  lv_label_set_text(ble_computer_label_, editing_settings_.ble_computer_name);
  lv_obj_set_style_text_font(ble_computer_label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(ble_computer_label_, kAccentColor, 0);
  lv_obj_align(ble_computer_label_, LV_ALIGN_CENTER, 0, 16);

  screen_hold_duration_ = lv_obj_create(nullptr);
  styleScreen(screen_hold_duration_);
  createHeader(screen_hold_duration_, "Hold duration", UiScreen::kSettings);

  hold_value_label_ = lv_label_create(screen_hold_duration_);
  lv_label_set_text(hold_value_label_, "Hold: 500 ms");
  lv_obj_set_style_text_font(hold_value_label_, &lv_font_montserrat_20, 0);
  lv_obj_align(hold_value_label_, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* hold_minus = createStepperButton(screen_hold_duration_, "-", -72);
  lv_obj_add_event_cb(hold_minus, onHoldDecrease, LV_EVENT_CLICKED, this);

  lv_obj_t* hold_plus = createStepperButton(screen_hold_duration_, "+", 72);
  lv_obj_add_event_cb(hold_plus, onHoldIncrease, LV_EVENT_CLICKED, this);

  screen_debug_ = lv_obj_create(nullptr);
  styleScreen(screen_debug_);
  createHeader(screen_debug_, "Debug", UiScreen::kSettings);

  debug_label_ = lv_label_create(screen_debug_);
  lv_obj_set_width(debug_label_, LV_PCT(92));
  lv_label_set_long_mode(debug_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(debug_label_, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(debug_label_, &lv_font_montserrat_14, 0);
  lv_obj_align(debug_label_, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_label_set_text(debug_label_, debug_info_.c_str());

  screen_factory_reset_ = lv_obj_create(nullptr);
  styleScreen(screen_factory_reset_);
  createHeader(screen_factory_reset_, "Factory reset", UiScreen::kSettings);

  lv_obj_t* warning_label = lv_label_create(screen_factory_reset_);
  lv_label_set_text(warning_label, "Reset all settings\nto factory defaults?");
  lv_obj_set_style_text_color(warning_label, kErrorColor, 0);
  lv_obj_set_style_text_align(warning_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(warning_label, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* confirm_button = lv_btn_create(screen_factory_reset_);
  lv_obj_set_size(confirm_button, 120, 44);
  lv_obj_align(confirm_button, LV_ALIGN_CENTER, -70, 40);
  lv_obj_set_style_radius(confirm_button, 10, 0);
  lv_obj_set_style_bg_color(confirm_button, kErrorColor, 0);
  lv_obj_add_event_cb(confirm_button, onFactoryResetConfirm, LV_EVENT_CLICKED,
                      this);

  lv_obj_t* confirm_label = lv_label_create(confirm_button);
  lv_label_set_text(confirm_label, "Confirm");
  lv_obj_center(confirm_label);

  lv_obj_t* cancel_button = lv_btn_create(screen_factory_reset_);
  lv_obj_set_size(cancel_button, 120, 44);
  lv_obj_align(cancel_button, LV_ALIGN_CENTER, 70, 40);
  lv_obj_set_style_radius(cancel_button, 10, 0);
  lv_obj_set_style_bg_color(cancel_button, lv_color_hex(0x3A3A3A), 0);
  lv_obj_add_event_cb(cancel_button, onFactoryResetCancel, LV_EVENT_CLICKED,
                      this);

  lv_obj_t* cancel_label = lv_label_create(cancel_button);
  lv_label_set_text(cancel_label, "Cancel");
  lv_obj_center(cancel_label);
}

void UiManager::updateLoadingDisplay() {
  if (loading_bar_ == nullptr || loading_count_label_ == nullptr) {
    return;
  }
  if (loading_total_ > 0) {
    const int percent = (loading_loaded_ * 100) / loading_total_;
    lv_bar_set_value(loading_bar_, percent, LV_ANIM_OFF);
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%d / %d", loading_loaded_,
                  loading_total_);
    lv_label_set_text(loading_count_label_, buffer);
  } else {
    lv_bar_set_value(loading_bar_, 0, LV_ANIM_OFF);
    lv_label_set_text(loading_count_label_, "Scanning...");
  }
}

void UiManager::updateBatteryDisplay() {
  if (battery_bar_ == nullptr || battery_label_ == nullptr) {
    return;
  }
  const int percent = battery_percent_ < 0 ? 0 : battery_percent_;
  lv_bar_set_value(battery_bar_, percent, LV_ANIM_OFF);
  char buffer[8];
  std::snprintf(buffer, sizeof(buffer), "%d%%", percent);
  lv_label_set_text(battery_label_, buffer);
}

void UiManager::updateKeyLabel() {
  if (key_label_ == nullptr) {
    return;
  }
  lv_label_set_text(key_label_,
                    current_key_label_.empty() ? "-" : current_key_label_.c_str());
}

void UiManager::updateErrorDisplay() {
  if (error_label_ == nullptr) {
    return;
  }
  if (error_message_.empty()) {
    lv_obj_add_flag(error_label_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(error_label_, "");
  } else {
    lv_label_set_text(error_label_, error_message_.c_str());
    lv_obj_remove_flag(error_label_, LV_OBJ_FLAG_HIDDEN);
  }
}

void UiManager::updateVolumeDisplay() {
  if (volume_value_label_ == nullptr) {
    return;
  }
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "Volume: %u%%",
                editing_settings_.volume_percent);
  lv_label_set_text(volume_value_label_, buffer);
}

void UiManager::updateHoldDisplay() {
  if (hold_value_label_ == nullptr) {
    return;
  }
  char buffer[24];
  std::snprintf(buffer, sizeof(buffer), "Hold: %lu ms",
                static_cast<unsigned long>(editing_settings_.hold_duration_ms));
  lv_label_set_text(hold_value_label_, buffer);
}

void UiManager::updateBleComputerDisplay() {
  if (ble_computer_label_ == nullptr) {
    return;
  }
  lv_label_set_text(ble_computer_label_, editing_settings_.ble_computer_name);
}

void UiManager::updateDebugDisplay() {
  if (debug_label_ == nullptr) {
    return;
  }
  lv_label_set_text(debug_label_, debug_info_.c_str());
}

void UiManager::onSettingsClicked(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  self->showScreen(UiScreen::kSettings);
}

void UiManager::onBackClicked(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  lv_obj_t* button = lv_event_get_current_target_obj(event);
  const auto back_to = static_cast<UiScreen>(
      reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));
  self->showScreen(back_to);
}

void UiManager::onListNavigate(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  lv_obj_t* item = lv_event_get_current_target_obj(event);
  const auto target = static_cast<UiScreen>(
      reinterpret_cast<intptr_t>(lv_obj_get_user_data(item)));
  self->showScreen(target);
}

void UiManager::onVolumeDecrease(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->editing_settings_.volume_percent >= 10) {
    self->editing_settings_.volume_percent -= 10;
    self->settings_changed_ = true;
    self->updateVolumeDisplay();
  }
}

void UiManager::onVolumeIncrease(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->editing_settings_.volume_percent <= 90) {
    self->editing_settings_.volume_percent += 10;
    self->settings_changed_ = true;
    self->updateVolumeDisplay();
  }
}

void UiManager::onHoldDecrease(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->editing_settings_.hold_duration_ms > kMinHoldDurationMs) {
    self->editing_settings_.hold_duration_ms -= 100;
    self->settings_changed_ = true;
    self->updateHoldDisplay();
  }
}

void UiManager::onHoldIncrease(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->editing_settings_.hold_duration_ms < kMaxHoldDurationMs) {
    self->editing_settings_.hold_duration_ms += 100;
    self->settings_changed_ = true;
    self->updateHoldDisplay();
  }
}

void UiManager::onBleScanClicked(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->on_ble_scan_requested_) {
    self->on_ble_scan_requested_();
  }
}

void UiManager::onFactoryResetConfirm(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  if (self->on_factory_reset_confirmed_) {
    self->on_factory_reset_confirmed_();
  }
  self->showScreen(UiScreen::kSettings);
}

void UiManager::onFactoryResetCancel(lv_event_t* event) {
  auto* self = static_cast<UiManager*>(lv_event_get_user_data(event));
  self->showScreen(UiScreen::kSettings);
}

#endif  // NATIVE_TEST

}  // namespace echo
