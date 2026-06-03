#include "ui_manager.h"

#include "lvgl_port.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <lvgl.h>
#endif

namespace echo {

#ifndef NATIVE_TEST

namespace {

const lv_color_t kBgColor = lv_color_hex(0x101010);
const lv_color_t kAccentColor = lv_color_hex(0x2060d0);
const lv_color_t kDangerColor = lv_color_hex(0xc03030);
const lv_color_t kTextColor = lv_color_hex(0xffffff);

void styleTextLight(lv_obj_t* obj) {
  lv_obj_set_style_text_color(obj, kTextColor, 0);
}

lv_obj_t* scr_main = nullptr;
lv_obj_t* scr_settings = nullptr;
lv_obj_t* scr_volume = nullptr;
lv_obj_t* scr_bluetooth = nullptr;
lv_obj_t* scr_ble_keyboard = nullptr;
lv_obj_t* scr_ble_computer = nullptr;
lv_obj_t* scr_hold = nullptr;
lv_obj_t* scr_factory = nullptr;

lv_obj_t* lbl_battery = nullptr;
lv_obj_t* lbl_key = nullptr;
lv_obj_t* lbl_error = nullptr;
lv_obj_t* slider_volume = nullptr;
lv_obj_t* lbl_volume_value = nullptr;
lv_obj_t* slider_hold = nullptr;
lv_obj_t* lbl_hold_value = nullptr;
lv_obj_t* lbl_ble_computer_name = nullptr;

void applyTheme(lv_obj_t* screen) {
  lv_obj_set_style_bg_color(screen, kBgColor, 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

UiManager* uiFromEvent(lv_event_t* e) {
  return static_cast<UiManager*>(lv_event_get_user_data(e));
}

lv_obj_t* createHeader(lv_obj_t* parent, UiManager* ui, const char* title,
                       UiScreen back_to) {
  lv_obj_t* header = lv_obj_create(parent);
  lv_obj_set_size(header, lv_pct(100), 44);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, kBgColor, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 4, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* back = lv_btn_create(header);
  lv_obj_set_size(back, 72, 32);
  lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_color(back, kAccentColor, 0);
  lv_obj_t* back_lbl = lv_label_create(back);
  lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_lbl);
  lv_obj_add_event_cb(
      back,
      [](lv_event_t* e) {
        auto* mgr = static_cast<UiManager*>(lv_event_get_user_data(e));
        const auto back_target = static_cast<UiScreen>(reinterpret_cast<intptr_t>(
            lv_obj_get_user_data(lv_event_get_target(e))));
        if (mgr != nullptr) {
          mgr->navigateTo(back_target);
        }
      },
      LV_EVENT_CLICKED, ui);
  lv_obj_set_user_data(back,
                      reinterpret_cast<void*>(static_cast<intptr_t>(back_to)));

  lv_obj_t* hdr_title = lv_label_create(header);
  lv_label_set_text(hdr_title, title);
  lv_obj_set_style_text_font(hdr_title, &lv_font_montserrat_14, 0);
  lv_obj_align(hdr_title, LV_ALIGN_CENTER, 0, 0);

  return header;
}

lv_obj_t* createMenuButton(lv_obj_t* parent, UiManager* ui, const char* text,
                           UiScreen target) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_width(btn, lv_pct(92));
  lv_obj_set_height(btn, 44);
  lv_obj_set_style_bg_color(btn, kAccentColor, 0);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, text);
  lv_obj_center(lbl);
  lv_obj_add_event_cb(
      btn,
      [](lv_event_t* e) {
        auto* mgr = static_cast<UiManager*>(lv_event_get_user_data(e));
        const auto screen = static_cast<UiScreen>(reinterpret_cast<intptr_t>(
            lv_obj_get_user_data(lv_event_get_target(e))));
        if (mgr != nullptr) {
          mgr->navigateTo(screen);
        }
      },
      LV_EVENT_CLICKED, ui);
  lv_obj_set_user_data(btn,
                       reinterpret_cast<void*>(static_cast<intptr_t>(target)));
  return btn;
}

}  // namespace

void UiManager::navigateTo(UiScreen screen) {
  screen_ = screen;
  lv_obj_t* scr = scr_main;
  switch (screen) {
    case UiScreen::kMain:
      scr = scr_main;
      refreshMainLabels();
      break;
    case UiScreen::kSettings:
      scr = scr_settings;
      break;
    case UiScreen::kVolume:
      scr = scr_volume;
      refreshVolumeUi();
      break;
    case UiScreen::kBluetoothMenu:
      scr = scr_bluetooth;
      break;
    case UiScreen::kBleKeyboard:
      scr = scr_ble_keyboard;
      break;
    case UiScreen::kBleComputer:
      scr = scr_ble_computer;
      refreshBleComputerUi();
      break;
    case UiScreen::kHoldDuration:
      scr = scr_hold;
      refreshHoldUi();
      break;
    case UiScreen::kFactoryResetConfirm:
      scr = scr_factory;
      break;
  }
  lv_scr_load(scr);
  lvglPortForceRefresh();
}

void UiManager::refreshMainLabels() {
  if (lbl_battery == nullptr) {
    return;
  }
  const int pct = battery_percent_ < 0 ? 0 : battery_percent_;
  lv_label_set_text_fmt(lbl_battery, LV_SYMBOL_BATTERY_FULL "  %d%%", pct);
  lv_label_set_text(lbl_key,
                    current_key_label_.empty() ? "-" : current_key_label_.c_str());
  if (error_message_.empty()) {
    lv_obj_add_flag(lbl_error, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(lbl_error, error_message_.c_str());
    lv_obj_clear_flag(lbl_error, LV_OBJ_FLAG_HIDDEN);
  }
}

void UiManager::refreshVolumeUi() {
  if (slider_volume == nullptr) {
    return;
  }
  lv_slider_set_value(slider_volume, editing_settings_.volume_percent, LV_ANIM_OFF);
  lv_label_set_text_fmt(lbl_volume_value, "%u%%", editing_settings_.volume_percent);
}

void UiManager::refreshHoldUi() {
  if (slider_hold == nullptr) {
    return;
  }
  lv_slider_set_value(slider_hold, editing_settings_.hold_duration_ms, LV_ANIM_OFF);
  lv_label_set_text_fmt(lbl_hold_value, "%lu ms",
                        static_cast<unsigned long>(editing_settings_.hold_duration_ms));
}

void UiManager::refreshBleComputerUi() {
  if (lbl_ble_computer_name == nullptr) {
    return;
  }
  lv_label_set_text(lbl_ble_computer_name, editing_settings_.ble_computer_name);
}

void UiManager::applyVolumePercent(uint8_t volume_percent) {
  editing_settings_.volume_percent = volume_percent;
  settings_changed_ = true;
  refreshVolumeUi();
}

void UiManager::applyHoldDurationMs(uint32_t hold_duration_ms) {
  editing_settings_.hold_duration_ms = hold_duration_ms;
  settings_changed_ = true;
  refreshHoldUi();
}

void UiManager::triggerBleScan() { ble_scan_requested_ = true; }

void UiManager::triggerFactoryResetConfirm() {
  factory_reset_confirmed_ = true;
}

void UiManager::buildScreens() {
  scr_main = lv_obj_create(nullptr);
  applyTheme(scr_main);

  lbl_battery = lv_label_create(scr_main);
  lv_obj_set_style_text_font(lbl_battery, &lv_font_montserrat_14, 0);
  styleTextLight(lbl_battery);
  lv_obj_align(lbl_battery, LV_ALIGN_TOP_LEFT, 12, 8);

  lbl_key = lv_label_create(scr_main);
  lv_obj_set_style_text_font(lbl_key, &lv_font_montserrat_14, 0);
  styleTextLight(lbl_key);
  lv_obj_align(lbl_key, LV_ALIGN_CENTER, 0, -10);

  lbl_error = lv_label_create(scr_main);
  lv_obj_set_style_text_color(lbl_error, lv_color_hex(0xff6060), 0);
  lv_obj_set_width(lbl_error, lv_pct(90));
  lv_label_set_long_mode(lbl_error, LV_LABEL_LONG_WRAP);
  lv_obj_align(lbl_error, LV_ALIGN_BOTTOM_MID, 0, -72);

  lv_obj_t* settings_btn = lv_btn_create(scr_main);
  lv_obj_set_size(settings_btn, 88, 40);
  lv_obj_align(settings_btn, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_set_style_bg_color(settings_btn, kAccentColor, 0);
  lv_obj_t* settings_lbl = lv_label_create(settings_btn);
  lv_label_set_text(settings_lbl, LV_SYMBOL_SETTINGS " Settings");
  lv_obj_center(settings_lbl);
  lv_obj_add_event_cb(
      settings_btn,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          mgr->navigateTo(UiScreen::kSettings);
        }
      },
      LV_EVENT_CLICKED, this);

  scr_settings = lv_obj_create(nullptr);
  applyTheme(scr_settings);
  createHeader(scr_settings, this, "Settings", UiScreen::kMain);

  lv_obj_t* menu = lv_obj_create(scr_settings);
  lv_obj_set_size(menu, lv_pct(100), lv_pct(100));
  lv_obj_align(menu, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(menu, 0, 0);
  lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(menu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(menu, 10, 0);
  lv_obj_set_style_pad_top(menu, 52, 0);
  lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);

  createMenuButton(menu, this, LV_SYMBOL_VOLUME_MAX "  Volume", UiScreen::kVolume);
  createMenuButton(menu, this, LV_SYMBOL_BLUETOOTH "  Bluetooth",
                   UiScreen::kBluetoothMenu);
  createMenuButton(menu, this, LV_SYMBOL_KEYBOARD "  Hold duration",
                   UiScreen::kHoldDuration);
  createMenuButton(menu, this, LV_SYMBOL_WARNING "  Factory reset",
                   UiScreen::kFactoryResetConfirm);

  scr_volume = lv_obj_create(nullptr);
  applyTheme(scr_volume);
  createHeader(scr_volume, this, "Volume", UiScreen::kSettings);

  lbl_volume_value = lv_label_create(scr_volume);
  lv_obj_set_style_text_font(lbl_volume_value, &lv_font_montserrat_14, 0);
  styleTextLight(lbl_volume_value);
  lv_obj_align(lbl_volume_value, LV_ALIGN_TOP_MID, 0, 64);

  slider_volume = lv_slider_create(scr_volume);
  lv_obj_set_width(slider_volume, lv_pct(85));
  lv_slider_set_range(slider_volume, kMinVolumePercent, kMaxVolumePercent);
  lv_obj_align(slider_volume, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_event_cb(
      slider_volume,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          const int value = lv_slider_get_value(lv_event_get_target(e));
          mgr->applyVolumePercent(static_cast<uint8_t>(value));
        }
      },
      LV_EVENT_VALUE_CHANGED, this);

  scr_bluetooth = lv_obj_create(nullptr);
  applyTheme(scr_bluetooth);
  createHeader(scr_bluetooth, this, "Bluetooth", UiScreen::kSettings);

  lv_obj_t* bt_menu = lv_obj_create(scr_bluetooth);
  lv_obj_set_size(bt_menu, lv_pct(100), lv_pct(100));
  lv_obj_align(bt_menu, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(bt_menu, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bt_menu, 0, 0);
  lv_obj_set_flex_flow(bt_menu, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(bt_menu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(bt_menu, 10, 0);
  lv_obj_set_style_pad_top(bt_menu, 52, 0);
  lv_obj_clear_flag(bt_menu, LV_OBJ_FLAG_SCROLLABLE);

  createMenuButton(bt_menu, this, LV_SYMBOL_KEYBOARD "  Keyboard connection",
                   UiScreen::kBleKeyboard);
  createMenuButton(bt_menu, this, LV_SYMBOL_USB "  Computer connection",
                   UiScreen::kBleComputer);

  scr_ble_keyboard = lv_obj_create(nullptr);
  applyTheme(scr_ble_keyboard);
  createHeader(scr_ble_keyboard, this, "BLE Keyboard", UiScreen::kBluetoothMenu);

  lv_obj_t* kb_hint = lv_label_create(scr_ble_keyboard);
  lv_label_set_text(kb_hint,
                    "Scan for a Bluetooth keyboard\nto pair with this device.");
  lv_obj_set_width(kb_hint, lv_pct(88));
  lv_label_set_long_mode(kb_hint, LV_LABEL_LONG_WRAP);
  lv_obj_align(kb_hint, LV_ALIGN_TOP_MID, 0, 56);

  lv_obj_t* scan_btn = lv_btn_create(scr_ble_keyboard);
  lv_obj_set_size(scan_btn, lv_pct(70), 44);
  lv_obj_align(scan_btn, LV_ALIGN_CENTER, 0, 24);
  lv_obj_set_style_bg_color(scan_btn, kAccentColor, 0);
  lv_obj_t* scan_lbl = lv_label_create(scan_btn);
  lv_label_set_text(scan_lbl, LV_SYMBOL_REFRESH "  Start scan");
  lv_obj_center(scan_lbl);
  lv_obj_add_event_cb(
      scan_btn,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          mgr->triggerBleScan();
        }
      },
      LV_EVENT_CLICKED, this);

  scr_ble_computer = lv_obj_create(nullptr);
  applyTheme(scr_ble_computer);
  createHeader(scr_ble_computer, this, "BLE Computer", UiScreen::kBluetoothMenu);

  lv_obj_t* comp_hint = lv_label_create(scr_ble_computer);
  lv_label_set_text(comp_hint, "This device advertises to the computer as:");
  lv_obj_align(comp_hint, LV_ALIGN_TOP_MID, 0, 56);

  lbl_ble_computer_name = lv_label_create(scr_ble_computer);
  lv_obj_set_style_text_font(lbl_ble_computer_name, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_ble_computer_name, lv_color_hex(0x80c0ff), 0);
  lv_obj_align(lbl_ble_computer_name, LV_ALIGN_CENTER, 0, -8);

  scr_hold = lv_obj_create(nullptr);
  applyTheme(scr_hold);
  createHeader(scr_hold, this, "Hold duration", UiScreen::kSettings);

  lbl_hold_value = lv_label_create(scr_hold);
  lv_obj_set_style_text_font(lbl_hold_value, &lv_font_montserrat_14, 0);
  styleTextLight(lbl_hold_value);
  lv_obj_align(lbl_hold_value, LV_ALIGN_TOP_MID, 0, 64);

  slider_hold = lv_slider_create(scr_hold);
  lv_obj_set_width(slider_hold, lv_pct(85));
  lv_slider_set_range(slider_hold, kMinHoldDurationMs, kMaxHoldDurationMs);
  lv_obj_align(slider_hold, LV_ALIGN_CENTER, 0, 20);
  lv_obj_add_event_cb(
      slider_hold,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          const int value = lv_slider_get_value(lv_event_get_target(e));
          mgr->applyHoldDurationMs(static_cast<uint32_t>(value));
        }
      },
      LV_EVENT_VALUE_CHANGED, this);

  scr_factory = lv_obj_create(nullptr);
  applyTheme(scr_factory);
  createHeader(scr_factory, this, "Factory reset", UiScreen::kSettings);

  lv_obj_t* warn = lv_label_create(scr_factory);
  lv_label_set_text(warn, "Erase all saved settings\nand restore defaults?");
  lv_obj_set_style_text_color(warn, lv_color_hex(0xff8080), 0);
  lv_obj_set_style_text_align(warn, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(warn, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t* confirm_btn = lv_btn_create(scr_factory);
  lv_obj_set_size(confirm_btn, 130, 44);
  lv_obj_align(confirm_btn, LV_ALIGN_BOTTOM_LEFT, 16, -16);
  lv_obj_set_style_bg_color(confirm_btn, kDangerColor, 0);
  lv_obj_t* confirm_lbl = lv_label_create(confirm_btn);
  lv_label_set_text(confirm_lbl, "Confirm");
  lv_obj_center(confirm_lbl);
  lv_obj_add_event_cb(
      confirm_btn,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          mgr->triggerFactoryResetConfirm();
          mgr->navigateTo(UiScreen::kSettings);
        }
      },
      LV_EVENT_CLICKED, this);

  lv_obj_t* cancel_btn = lv_btn_create(scr_factory);
  lv_obj_set_size(cancel_btn, 130, 44);
  lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
  lv_obj_set_style_bg_color(cancel_btn, kAccentColor, 0);
  lv_obj_t* cancel_lbl = lv_label_create(cancel_btn);
  lv_label_set_text(cancel_lbl, "Cancel");
  lv_obj_center(cancel_lbl);
  lv_obj_add_event_cb(
      cancel_btn,
      [](lv_event_t* e) {
        if (auto* mgr = uiFromEvent(e)) {
          mgr->navigateTo(UiScreen::kSettings);
        }
      },
      LV_EVENT_CLICKED, this);

  refreshMainLabels();
  refreshVolumeUi();
  refreshHoldUi();
  refreshBleComputerUi();
}

#endif  // NATIVE_TEST

void UiManager::begin() {
#ifndef NATIVE_TEST
  auto& display = M5.Display;
  display.setRotation(1);
  display.fillScreen(TFT_BLACK);
  lvglPortInit();
  buildScreens();
  navigateTo(UiScreen::kMain);
  lvglPortForceRefresh();
#else
  editing_settings_ = defaultDeviceSettings();
#endif
}

void UiManager::tick() {
#ifndef NATIVE_TEST
  lvglPortTick();
#endif
}

void UiManager::setCurrentKeyLabel(const char* label) {
  current_key_label_ = label ? label : "";
#ifndef NATIVE_TEST
  refreshMainLabels();
#endif
}

void UiManager::setBatteryPercent(int battery_percent) {
  battery_percent_ = battery_percent;
#ifndef NATIVE_TEST
  refreshMainLabels();
#endif
}

void UiManager::setErrorMessage(const char* message) {
  error_message_ = message ? message : "";
#ifndef NATIVE_TEST
  refreshMainLabels();
#endif
}

void UiManager::clearError() {
  error_message_.clear();
#ifndef NATIVE_TEST
  refreshMainLabels();
#endif
}

UiScreen UiManager::currentScreen() const { return screen_; }

bool UiManager::settingsChanged() const { return settings_changed_; }

DeviceSettings& UiManager::editingSettings() { return editing_settings_; }

void UiManager::acknowledgeSettingsSaved() { settings_changed_ = false; }

bool UiManager::bleScanRequested() const { return ble_scan_requested_; }

void UiManager::clearBleScanRequested() { ble_scan_requested_ = false; }

bool UiManager::factoryResetConfirmed() const {
  return factory_reset_confirmed_;
}

void UiManager::clearFactoryResetConfirmed() {
  factory_reset_confirmed_ = false;
}

void UiManager::refreshSettingsWidgets() {
#ifndef NATIVE_TEST
  refreshVolumeUi();
  refreshHoldUi();
  refreshBleComputerUi();
#endif
}

}  // namespace echo
