#include "ui.h"

#include "computer_output.h"
#include "device_settings_store.h"
#include "key_audio.h"
#include "usb_keyboard.h"

#include <lvgl.h>

#include <stdio.h>
#include <string.h>

namespace {

enum class Screen {
  kMain,
  kSettings,
  kDebug,
  kVolume,
  kHoldDuration,
  kBluetooth,
  kComputerConnection,
  kKeyboardConnection,
};

const lv_color_t kBgColor = lv_color_hex(0x1A1A1A);
const lv_color_t kHeaderColor = lv_color_hex(0x2A2A2A);
const lv_color_t kAccentColor = lv_color_hex(0x0066FF);

constexpr uint32_t kMinHoldDurationMs = 100;
constexpr uint32_t kMaxHoldDurationMs = 3000;

lv_obj_t* screen_main = nullptr;
lv_obj_t* screen_settings = nullptr;
lv_obj_t* screen_debug = nullptr;
lv_obj_t* screen_volume = nullptr;
lv_obj_t* screen_hold_duration = nullptr;
lv_obj_t* screen_bluetooth = nullptr;
lv_obj_t* screen_computer_connection = nullptr;
lv_obj_t* screen_keyboard_connection = nullptr;
lv_obj_t* connection_flow_label = nullptr;
lv_obj_t* pressed_key_box = nullptr;
lv_obj_t* pressed_key_label = nullptr;
lv_obj_t* audio_debug_label = nullptr;
lv_obj_t* volume_slider = nullptr;
lv_obj_t* volume_value_label = nullptr;
lv_obj_t* hold_duration_slider = nullptr;
lv_obj_t* hold_duration_value_label = nullptr;
lv_obj_t* computer_usb_status_label = nullptr;
lv_obj_t* computer_name_textarea = nullptr;
lv_obj_t* computer_ble_status_label = nullptr;
lv_obj_t* keyboard_status_label = nullptr;
lv_obj_t* keyboard_usb_status_label = nullptr;
lv_obj_t* keyboard_name_textarea = nullptr;

constexpr size_t kMaxBatteryLabels = 8;
lv_obj_t* battery_labels[kMaxBatteryLabels] = {};
size_t battery_label_count = 0;

uint32_t hold_duration_ms = kDefaultHoldDurationMs;  // from device_settings_store.h
char ble_computer_name[16] = "echolocation";
char ble_keyboard_name[16] = "";

void showScreen(Screen screen);

void registerBatteryLabel(lv_obj_t* label) {
  if (battery_label_count < kMaxBatteryLabels) {
    battery_labels[battery_label_count++] = label;
  }
}

const char* batterySymbolForLevel(int percent) {
  if (percent >= 80) {
    return LV_SYMBOL_BATTERY_FULL;
  }
  if (percent >= 60) {
    return LV_SYMBOL_BATTERY_3;
  }
  if (percent >= 40) {
    return LV_SYMBOL_BATTERY_2;
  }
  if (percent >= 20) {
    return LV_SYMBOL_BATTERY_1;
  }
  return LV_SYMBOL_BATTERY_EMPTY;
}

void refreshBatteryLabels(int percent, bool charging) {
  if (percent < 0) {
    percent = 0;
  } else if (percent > 100) {
    percent = 100;
  }

  const char* symbol = batterySymbolForLevel(percent);
  char text[32];
  if (charging) {
    snprintf(text, sizeof(text), "%s %s %d%%", LV_SYMBOL_CHARGE, symbol, percent);
  } else {
    snprintf(text, sizeof(text), "%s %d%%", symbol, percent);
  }

  lv_color_t color;
  if (charging) {
    color = lv_color_hex(0x44DD66);
  } else if (percent <= 10) {
    color = lv_color_hex(0xFF4444);
  } else if (percent <= 20) {
    color = lv_color_hex(0xFFAA00);
  } else {
    color = lv_color_hex(0xAAAAAA);
  }

  for (size_t i = 0; i < battery_label_count; ++i) {
    if (battery_labels[i] == nullptr) {
      continue;
    }
    lv_label_set_text(battery_labels[i], text);
    lv_obj_set_style_text_color(battery_labels[i], color, 0);
  }
}

lv_obj_t* createBatteryLabel(lv_obj_t* parent, lv_align_t align, int x_ofs,
                             int y_ofs) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_label_set_text(label, LV_SYMBOL_BATTERY_EMPTY " --%");
  lv_obj_align(label, align, x_ofs, y_ofs);
  registerBatteryLabel(label);
  return label;
}

void styleScreen(lv_obj_t* screen) {
  lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(screen, kBgColor, 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

void showScreen(Screen screen) {
  lv_obj_t* target = nullptr;
  switch (screen) {
    case Screen::kMain:
      target = screen_main;
      break;
    case Screen::kSettings:
      target = screen_settings;
      break;
    case Screen::kDebug:
      target = screen_debug;
      break;
    case Screen::kVolume:
      target = screen_volume;
      break;
    case Screen::kHoldDuration:
      target = screen_hold_duration;
      break;
    case Screen::kBluetooth:
      target = screen_bluetooth;
      break;
    case Screen::kComputerConnection:
      target = screen_computer_connection;
      break;
    case Screen::kKeyboardConnection:
      target = screen_keyboard_connection;
      break;
  }
  if (target != nullptr) {
    lv_screen_load(target);
  }
}

void onBackClicked(lv_event_t* event) {
  lv_obj_t* button = lv_event_get_current_target_obj(event);
  const auto back_to = static_cast<Screen>(
      reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));
  showScreen(back_to);
}

lv_obj_t* createHeader(lv_obj_t* parent, const char* title, Screen back_to) {
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
  lv_obj_set_user_data(back_button,
                       reinterpret_cast<void*>(static_cast<intptr_t>(back_to)));
  lv_obj_add_event_cb(back_button, onBackClicked, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* back_label = lv_label_create(back_button);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);

  lv_obj_t* title_label = lv_label_create(header);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
  lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

  createBatteryLabel(header, LV_ALIGN_RIGHT_MID, -4, 0);

  return header;
}

lv_obj_t* createMenuButton(lv_obj_t* parent, const char* label, int y,
                           lv_event_cb_t on_click) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_size(button, 296, 44);
  lv_obj_align(button, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_set_style_radius(button, 8, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x2A2A2A), 0);
  lv_obj_add_event_cb(button, on_click, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* button_label = lv_label_create(button);
  lv_label_set_text(button_label, label);
  lv_obj_set_style_text_font(button_label, &lv_font_montserrat_16, 0);
  lv_obj_align(button_label, LV_ALIGN_LEFT_MID, 12, 0);

  lv_obj_t* chevron = lv_label_create(button);
  lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
  lv_obj_set_style_text_color(chevron, lv_color_hex(0x888888), 0);
  lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -12, 0);

  return button;
}

void refreshAudioDebugLabel() {
  if (audio_debug_label == nullptr) {
    return;
  }

  KeyAudioDebugInfo info;
  keyAudioGetDebugInfo(&info);

  char text[320];
  snprintf(text, sizeof(text),
           "SD card: %s\n"
           "/audio folder: %s\n\n"
           "Sample files:\n"
           "  a.wav: %s\n"
           "  b.wav: %s\n"
           "  c.wav: %s\n"
           "  space.wav: %s\n"
           "  enter.wav: %s\n\n"
           "Found: %d/5",
           info.sd_mounted ? "yes" : "no",
           info.audio_dir_exists ? "yes" : "no",
           info.probe_a_wav ? "yes" : "no", info.probe_b_wav ? "yes" : "no",
           info.probe_c_wav ? "yes" : "no", info.probe_space_wav ? "yes" : "no",
           info.probe_enter_wav ? "yes" : "no", info.probe_files_found);
  lv_label_set_text(audio_debug_label, text);
}

void updateVolumeLabel() {
  if (volume_slider == nullptr || volume_value_label == nullptr) {
    return;
  }

  const int32_t value = lv_slider_get_value(volume_slider);
  const int percent = (value * 100) / 255;
  char text[24];
  snprintf(text, sizeof(text), "Volume: %d%%", percent);
  lv_label_set_text(volume_value_label, text);
}

void updateHoldDurationLabel() {
  if (hold_duration_slider == nullptr || hold_duration_value_label == nullptr) {
    return;
  }

  const int32_t value = lv_slider_get_value(hold_duration_slider);
  char text[32];
  snprintf(text, sizeof(text), "Hold Duration: %ld ms", static_cast<long>(value));
  lv_label_set_text(hold_duration_value_label, text);
}

void onRefreshAudioDebugClicked(lv_event_t* event) {
  (void)event;
  keyAudioRefresh();
  refreshAudioDebugLabel();
}

void onSettingsClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kSettings);
}

void onDebugMenuClicked(lv_event_t* event) {
  (void)event;
  refreshAudioDebugLabel();
  showScreen(Screen::kDebug);
}

void onVolumeMenuClicked(lv_event_t* event) {
  (void)event;
  if (volume_slider != nullptr) {
    lv_slider_set_value(volume_slider, keyAudioGetVolume(), LV_ANIM_OFF);
  }
  updateVolumeLabel();
  showScreen(Screen::kVolume);
}

void onVolumeSliderChanged(lv_event_t* event) {
  lv_obj_t* slider = lv_event_get_target_obj(event);
  const int32_t value = lv_slider_get_value(slider);
  keyAudioSetVolume(static_cast<uint8_t>(value));
  deviceSettingsSaveVolume(static_cast<uint8_t>(value));
  updateVolumeLabel();
}

void onHoldDurationMenuClicked(lv_event_t* event) {
  (void)event;
  if (hold_duration_slider != nullptr) {
    lv_slider_set_value(hold_duration_slider, static_cast<int32_t>(hold_duration_ms),
                        LV_ANIM_OFF);
  }
  updateHoldDurationLabel();
  showScreen(Screen::kHoldDuration);
}

void onHoldDurationSliderChanged(lv_event_t* event) {
  lv_obj_t* slider = lv_event_get_target_obj(event);
  hold_duration_ms = static_cast<uint32_t>(lv_slider_get_value(slider));
  deviceSettingsSaveHoldDurationMs(hold_duration_ms);
  updateHoldDurationLabel();
}

void refreshComputerConnectionStatus() {
  if (computer_usb_status_label == nullptr || computer_ble_status_label == nullptr) {
    return;
  }

  if (computerOutputUsbReady()) {
    lv_label_set_text(computer_usb_status_label, "USB: Connected");
    lv_obj_set_style_text_color(computer_usb_status_label, lv_color_hex(0x44DD66), 0);
  } else {
    lv_label_set_text(computer_usb_status_label, "USB: Not connected");
    lv_obj_set_style_text_color(computer_usb_status_label, lv_color_hex(0xAAAAAA), 0);
  }

  if (computer_name_textarea != nullptr) {
    lv_textarea_set_text(computer_name_textarea, computerOutputBleGetDeviceName());
  }

  if (computerOutputBleConnected()) {
    lv_label_set_text(computer_ble_status_label, "Bluetooth: Connected");
    lv_obj_set_style_text_color(computer_ble_status_label, lv_color_hex(0x44DD66), 0);
  } else {
    lv_label_set_text(computer_ble_status_label, "Bluetooth: Not connected");
    lv_obj_set_style_text_color(computer_ble_status_label, lv_color_hex(0xAAAAAA), 0);
  }
}

void refreshKeyboardConnectionStatus() {
  if (keyboard_usb_status_label == nullptr) {
    return;
  }

  if (usbKeyboardIsConnected()) {
    lv_label_set_text(keyboard_usb_status_label, "USB keyboard: Connected");
    lv_obj_set_style_text_color(keyboard_usb_status_label, lv_color_hex(0x44DD66), 0);
  } else {
    lv_label_set_text(keyboard_usb_status_label, "USB keyboard: Not connected");
    lv_obj_set_style_text_color(keyboard_usb_status_label, lv_color_hex(0xAAAAAA), 0);
  }
}

void refreshConnectionFlowIndicator() {
  if (connection_flow_label == nullptr) {
    return;
  }

  const bool input_usb = usbKeyboardIsConnected();
  const bool output_usb = computerOutputUsbReady();
  const bool output_ble = computerOutputBleConnected();

  const char* input_icon = input_usb ? LV_SYMBOL_USB : LV_SYMBOL_CLOSE;
  const char* output_icon;
  if (output_usb) {
    output_icon = LV_SYMBOL_USB;
  } else if (output_ble) {
    output_icon = LV_SYMBOL_BLUETOOTH;
  } else {
    output_icon = LV_SYMBOL_CLOSE;
  }

  char text[32];
  snprintf(text, sizeof(text), "%s %s %s", input_icon, LV_SYMBOL_RIGHT, output_icon);
  lv_label_set_text(connection_flow_label, text);

  const bool active = input_usb || output_usb || output_ble;
  lv_obj_set_style_text_color(connection_flow_label,
                              active ? kAccentColor : lv_color_hex(0x666666), 0);
}

void onBluetoothMenuClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kBluetooth);
}

void onComputerConnectionMenuClicked(lv_event_t* event) {
  (void)event;
  refreshComputerConnectionStatus();
  showScreen(Screen::kComputerConnection);
}

void onKeyboardConnectionMenuClicked(lv_event_t* event) {
  (void)event;
  if (keyboard_name_textarea != nullptr) {
    lv_textarea_set_text(keyboard_name_textarea, ble_keyboard_name);
  }
  refreshKeyboardConnectionStatus();
  showScreen(Screen::kKeyboardConnection);
}

void onKeyboardNameChanged(lv_event_t* event) {
  lv_obj_t* textarea = lv_event_get_target_obj(event);
  const char* text = lv_textarea_get_text(textarea);
  if (text == nullptr) {
    return;
  }
  strncpy(ble_keyboard_name, text, sizeof(ble_keyboard_name) - 1);
  ble_keyboard_name[sizeof(ble_keyboard_name) - 1] = '\0';
  deviceSettingsSaveBleKeyboardName(ble_keyboard_name);
}

void buildScreens() {
  screen_main = lv_obj_create(nullptr);
  styleScreen(screen_main);

  connection_flow_label = lv_label_create(screen_main);
  lv_label_set_text(connection_flow_label, LV_SYMBOL_CLOSE " " LV_SYMBOL_RIGHT " "
                                              LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_font(connection_flow_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(connection_flow_label, lv_color_hex(0x666666), 0);
  lv_obj_align(connection_flow_label, LV_ALIGN_TOP_LEFT, 12, 16);

  createBatteryLabel(screen_main, LV_ALIGN_TOP_RIGHT, -12, 16);

  lv_obj_t* title = lv_label_create(screen_main);
  lv_label_set_text(title, "echolocation");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  pressed_key_box = lv_obj_create(screen_main);
  lv_obj_set_size(pressed_key_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(pressed_key_box, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(pressed_key_box, kAccentColor, 0);
  lv_obj_set_style_border_width(pressed_key_box, 0, 0);
  lv_obj_set_style_radius(pressed_key_box, 8, 0);
  lv_obj_set_style_pad_hor(pressed_key_box, 16, 0);
  lv_obj_set_style_pad_ver(pressed_key_box, 8, 0);
  lv_obj_set_style_shadow_width(pressed_key_box, 0, 0);
  lv_obj_align(pressed_key_box, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(pressed_key_box, LV_OBJ_FLAG_HIDDEN);

  pressed_key_label = lv_label_create(pressed_key_box);
  lv_label_set_text(pressed_key_label, "");
  lv_obj_set_style_text_font(pressed_key_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(pressed_key_label, lv_color_white(), 0);
  lv_obj_center(pressed_key_label);

  lv_obj_t* settings_button = lv_btn_create(screen_main);
  lv_obj_set_size(settings_button, 100, 40);
  lv_obj_align(settings_button, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_set_style_radius(settings_button, 8, 0);
  lv_obj_set_style_bg_color(settings_button, kAccentColor, 0);
  lv_obj_add_event_cb(settings_button, onSettingsClicked, LV_EVENT_CLICKED,
                      nullptr);

  lv_obj_t* settings_label = lv_label_create(settings_button);
  lv_label_set_text(settings_label, "settings");
  lv_obj_center(settings_label);

  screen_settings = lv_obj_create(nullptr);
  styleScreen(screen_settings);
  createHeader(screen_settings, "Settings", Screen::kMain);
  createMenuButton(screen_settings, "Debug", 56, onDebugMenuClicked);
  createMenuButton(screen_settings, "Volume", 100, onVolumeMenuClicked);
  createMenuButton(screen_settings, "Hold Duration", 144, onHoldDurationMenuClicked);
  createMenuButton(screen_settings, "Bluetooth", 188, onBluetoothMenuClicked);

  screen_debug = lv_obj_create(nullptr);
  styleScreen(screen_debug);
  createHeader(screen_debug, "Debug", Screen::kSettings);

  audio_debug_label = lv_label_create(screen_debug);
  lv_label_set_text(audio_debug_label, "Checking...");
  lv_obj_set_style_text_font(audio_debug_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(audio_debug_label, lv_color_white(), 0);
  lv_obj_set_width(audio_debug_label, 296);
  lv_label_set_long_mode(audio_debug_label, LV_LABEL_LONG_WRAP);
  lv_obj_align(audio_debug_label, LV_ALIGN_TOP_LEFT, 12, 52);

  lv_obj_t* refresh_button = lv_btn_create(screen_debug);
  lv_obj_set_size(refresh_button, 120, 36);
  lv_obj_align(refresh_button, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_obj_set_style_radius(refresh_button, 8, 0);
  lv_obj_set_style_bg_color(refresh_button, kAccentColor, 0);
  lv_obj_add_event_cb(refresh_button, onRefreshAudioDebugClicked,
                      LV_EVENT_CLICKED, nullptr);

  lv_obj_t* refresh_label = lv_label_create(refresh_button);
  lv_label_set_text(refresh_label, "Refresh");
  lv_obj_center(refresh_label);

  screen_volume = lv_obj_create(nullptr);
  styleScreen(screen_volume);
  createHeader(screen_volume, "Volume", Screen::kSettings);

  volume_value_label = lv_label_create(screen_volume);
  lv_label_set_text(volume_value_label, "Volume: 0%");
  lv_obj_set_style_text_font(volume_value_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(volume_value_label, lv_color_white(), 0);
  lv_obj_align(volume_value_label, LV_ALIGN_CENTER, 0, -24);

  volume_slider = lv_slider_create(screen_volume);
  lv_obj_set_size(volume_slider, 260, 12);
  lv_obj_align(volume_slider, LV_ALIGN_CENTER, 0, 16);
  lv_slider_set_range(volume_slider, 0, 255);
  lv_slider_set_value(volume_slider, keyAudioGetVolume(), LV_ANIM_OFF);
  lv_obj_add_event_cb(volume_slider, onVolumeSliderChanged, LV_EVENT_VALUE_CHANGED,
                      nullptr);
  updateVolumeLabel();

  screen_hold_duration = lv_obj_create(nullptr);
  styleScreen(screen_hold_duration);
  createHeader(screen_hold_duration, "Hold Duration", Screen::kSettings);

  hold_duration_value_label = lv_label_create(screen_hold_duration);
  lv_label_set_text(hold_duration_value_label, "Hold Duration: 500 ms");
  lv_obj_set_style_text_font(hold_duration_value_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(hold_duration_value_label, lv_color_white(), 0);
  lv_obj_align(hold_duration_value_label, LV_ALIGN_CENTER, 0, -24);

  hold_duration_slider = lv_slider_create(screen_hold_duration);
  lv_obj_set_size(hold_duration_slider, 260, 12);
  lv_obj_align(hold_duration_slider, LV_ALIGN_CENTER, 0, 16);
  lv_slider_set_range(hold_duration_slider, static_cast<int32_t>(kMinHoldDurationMs),
                      static_cast<int32_t>(kMaxHoldDurationMs));
  lv_slider_set_value(hold_duration_slider,
                      static_cast<int32_t>(kDefaultHoldDurationMs), LV_ANIM_OFF);
  lv_obj_add_event_cb(hold_duration_slider, onHoldDurationSliderChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);
  updateHoldDurationLabel();

  screen_bluetooth = lv_obj_create(nullptr);
  styleScreen(screen_bluetooth);
  createHeader(screen_bluetooth, "Bluetooth", Screen::kSettings);
  createMenuButton(screen_bluetooth, "Computer Connection", 56,
                   onComputerConnectionMenuClicked);
  createMenuButton(screen_bluetooth, "Keyboard Connection", 100,
                   onKeyboardConnectionMenuClicked);

  screen_computer_connection = lv_obj_create(nullptr);
  styleScreen(screen_computer_connection);
  createHeader(screen_computer_connection, "Computer", Screen::kBluetooth);

  lv_obj_t* content_panel = lv_obj_create(screen_computer_connection);
  lv_obj_set_size(content_panel, 296, 184);
  lv_obj_align(content_panel, LV_ALIGN_TOP_MID, 0, 48);
  lv_obj_set_style_bg_color(content_panel, lv_color_hex(0x2A2A2A), 0);
  lv_obj_set_style_bg_opa(content_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(content_panel, 0, 0);
  lv_obj_set_style_radius(content_panel, 8, 0);
  lv_obj_set_style_pad_all(content_panel, 12, 0);
  lv_obj_remove_flag(content_panel, LV_OBJ_FLAG_SCROLLABLE);

  computer_usb_status_label = lv_label_create(content_panel);
  lv_label_set_text(computer_usb_status_label, "USB: --");
  lv_obj_set_style_text_font(computer_usb_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(computer_usb_status_label, LV_ALIGN_TOP_LEFT, 0, 0);

  lv_obj_t* ble_name_heading = lv_label_create(content_panel);
  lv_label_set_text(ble_name_heading, "Bluetooth name");
  lv_obj_set_style_text_font(ble_name_heading, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(ble_name_heading, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(ble_name_heading, LV_ALIGN_TOP_LEFT, 0, 32);

  computer_name_textarea = lv_textarea_create(content_panel);
  lv_obj_set_size(computer_name_textarea, LV_PCT(100), 36);
  lv_obj_align(computer_name_textarea, LV_ALIGN_TOP_MID, 0, 52);
  lv_textarea_set_one_line(computer_name_textarea, true);
  lv_textarea_set_max_length(computer_name_textarea, 15);
  lv_textarea_set_text(computer_name_textarea, kDefaultBleComputerName);
  lv_obj_remove_flag(computer_name_textarea, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(computer_name_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);

  computer_ble_status_label = lv_label_create(content_panel);
  lv_label_set_text(computer_ble_status_label, "Bluetooth: --");
  lv_obj_set_style_text_font(computer_ble_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(computer_ble_status_label, LV_ALIGN_TOP_LEFT, 0, 96);

  screen_keyboard_connection = lv_obj_create(nullptr);
  styleScreen(screen_keyboard_connection);
  createHeader(screen_keyboard_connection, "Keyboard", Screen::kBluetooth);

  keyboard_status_label = lv_label_create(screen_keyboard_connection);
  lv_label_set_text(keyboard_status_label, "Connect a keyboard to read keypresses");
  lv_obj_set_style_text_font(keyboard_status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(keyboard_status_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_align(keyboard_status_label, LV_ALIGN_TOP_LEFT, 12, 48);

  keyboard_usb_status_label = lv_label_create(screen_keyboard_connection);
  lv_label_set_text(keyboard_usb_status_label, "USB keyboard: --");
  lv_obj_set_style_text_font(keyboard_usb_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(keyboard_usb_status_label, LV_ALIGN_TOP_LEFT, 12, 72);

  lv_obj_t* kb_name_label = lv_label_create(screen_keyboard_connection);
  lv_label_set_text(kb_name_label, "Bluetooth keyboard name");
  lv_obj_set_style_text_font(kb_name_label, &lv_font_montserrat_14, 0);
  lv_obj_align(kb_name_label, LV_ALIGN_TOP_LEFT, 12, 108);

  keyboard_name_textarea = lv_textarea_create(screen_keyboard_connection);
  lv_obj_set_size(keyboard_name_textarea, 296, 36);
  lv_obj_align(keyboard_name_textarea, LV_ALIGN_TOP_MID, 0, 128);
  lv_textarea_set_one_line(keyboard_name_textarea, true);
  lv_textarea_set_max_length(keyboard_name_textarea, 15);
  lv_textarea_set_placeholder_text(keyboard_name_textarea, "Optional filter");
  lv_obj_add_event_cb(keyboard_name_textarea, onKeyboardNameChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);
}

}  // namespace

void uiInit() {
  buildScreens();
  showScreen(Screen::kMain);
  refreshConnectionFlowIndicator();
}

void uiSetKeyboardConnected(bool connected) {
  (void)connected;
  refreshConnectionFlowIndicator();
}

void uiSetPressedKey(const char* label) {
  if (pressed_key_label == nullptr || pressed_key_box == nullptr) {
    return;
  }
  if (label == nullptr || label[0] == '\0') {
    lv_label_set_text(pressed_key_label, "");
    lv_obj_add_flag(pressed_key_box, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_label_set_text(pressed_key_label, label);
  lv_obj_remove_flag(pressed_key_box, LV_OBJ_FLAG_HIDDEN);
}

void uiSetKeyBoxOutline(bool show) {
  if (pressed_key_box == nullptr) {
    return;
  }
  lv_obj_set_style_border_width(pressed_key_box, show ? 3 : 0, 0);
}

void uiSetVolume(uint8_t volume) {
  keyAudioSetVolume(volume);
  if (volume_slider != nullptr) {
    lv_slider_set_value(volume_slider, volume, LV_ANIM_OFF);
  }
  updateVolumeLabel();
}

void uiSetBattery(int percent, bool charging) {
  refreshBatteryLabels(percent, charging);
}

uint32_t uiGetHoldDurationMs() { return hold_duration_ms; }

void uiSetHoldDurationMs(uint32_t ms) {
  if (ms < kMinHoldDurationMs) {
    ms = kMinHoldDurationMs;
  } else if (ms > kMaxHoldDurationMs) {
    ms = kMaxHoldDurationMs;
  }
  hold_duration_ms = ms;
  if (hold_duration_slider != nullptr) {
    lv_slider_set_value(hold_duration_slider, static_cast<int32_t>(ms), LV_ANIM_OFF);
  }
  updateHoldDurationLabel();
}

void uiSetBleComputerName(const char* name) {
  if (name == nullptr) {
    return;
  }
  strncpy(ble_computer_name, name, sizeof(ble_computer_name) - 1);
  ble_computer_name[sizeof(ble_computer_name) - 1] = '\0';
  if (computer_name_textarea != nullptr) {
    lv_textarea_set_text(computer_name_textarea, ble_computer_name);
  }
}

void uiSetBleKeyboardName(const char* name) {
  if (name == nullptr) {
    return;
  }
  strncpy(ble_keyboard_name, name, sizeof(ble_keyboard_name) - 1);
  ble_keyboard_name[sizeof(ble_keyboard_name) - 1] = '\0';
  if (keyboard_name_textarea != nullptr) {
    lv_textarea_set_text(keyboard_name_textarea, ble_keyboard_name);
  }
}

void uiRefreshComputerConnectionStatus() {
  refreshComputerConnectionStatus();
  refreshConnectionFlowIndicator();
}

void uiRefreshKeyboardConnectionStatus() {
  refreshKeyboardConnectionStatus();
  refreshConnectionFlowIndicator();
}

void uiRefreshConnectionFlow() { refreshConnectionFlowIndicator(); }
