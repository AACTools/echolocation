#include "ui.h"

#include "computer_output.h"
#include "device_settings_store.h"
#include "key_audio.h"
#include "lvgl_port.h"
#include "speaker_detect.h"
#include "usb_keyboard.h"

#include <lvgl.h>

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_14_bold);

namespace {

enum class Screen {
  kLoading,
  kMain,
  kSettings,
  kBluetooth,
  kComputerOutput,
#ifdef ECHOLOCATION_DEBUG
  kDebug,
#endif
  kVolume,
  kHoldDuration,
  kFactoryReset,
};

const lv_color_t kBgColor = lv_color_hex(0x1A1A1A);
const lv_color_t kHeaderColor = lv_color_hex(0x2A2A2A);
const lv_color_t kAccentColor = lv_color_hex(0x0066FF);
const lv_color_t kConnectedColor = lv_color_hex(0x44DD66);

constexpr uint32_t kMinHoldDurationMs = 100;
constexpr uint32_t kMaxHoldDurationMs = 3000;
constexpr uint32_t kBluetoothOutputDotsIntervalMs = 400;
constexpr uint8_t kBluetoothOutputMaxDots = 9;

lv_obj_t* screen_loading = nullptr;
lv_obj_t* loading_status_label = nullptr;
lv_obj_t* screen_main = nullptr;
lv_obj_t* screen_settings = nullptr;
lv_obj_t* screen_bluetooth = nullptr;
lv_obj_t* screen_computer_output = nullptr;
#ifdef ECHOLOCATION_DEBUG
lv_obj_t* screen_debug = nullptr;
#endif
lv_obj_t* screen_volume = nullptr;
lv_obj_t* screen_hold_duration = nullptr;
lv_obj_t* screen_factory_reset = nullptr;
lv_obj_t* connection_flow_label = nullptr;
lv_obj_t* speaker_output_label = nullptr;
lv_obj_t* speaker_error_label = nullptr;
lv_obj_t* pressed_key_box = nullptr;
lv_obj_t* pressed_key_label = nullptr;
#ifdef ECHOLOCATION_DEBUG
lv_obj_t* audio_debug_label = nullptr;
#endif
lv_obj_t* volume_slider = nullptr;
lv_obj_t* volume_value_label = nullptr;
lv_obj_t* hold_duration_slider = nullptr;
lv_obj_t* hold_duration_value_label = nullptr;
lv_obj_t* bluetooth_output_switch = nullptr;
lv_obj_t* bluetooth_output_status_label = nullptr;
lv_timer_t* bluetooth_output_dots_timer = nullptr;

Screen current_screen = Screen::kLoading;

constexpr size_t kMaxBatteryLabels = 8;
lv_obj_t* battery_labels[kMaxBatteryLabels] = {};
size_t battery_label_count = 0;

uint32_t hold_duration_ms = kDefaultHoldDurationMs;
bool bluetooth_output_enabled = kDefaultBluetoothOutput;
uint8_t bluetooth_output_dot_count = 1;

enum class BluetoothOutputDisplayState {
  kHidden,
  kReady,
  kConnected,
};

BluetoothOutputDisplayState displayed_bluetooth_output_state =
    BluetoothOutputDisplayState::kHidden;

void showScreen(Screen screen);

void refreshBluetoothOutputStatus();

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
    case Screen::kLoading:
      target = screen_loading;
      break;
    case Screen::kMain:
      target = screen_main;
      break;
    case Screen::kSettings:
      target = screen_settings;
      break;
    case Screen::kBluetooth:
      target = screen_bluetooth;
      break;
    case Screen::kComputerOutput:
      target = screen_computer_output;
      break;
#ifdef ECHOLOCATION_DEBUG
    case Screen::kDebug:
      target = screen_debug;
      break;
#endif
    case Screen::kVolume:
      target = screen_volume;
      break;
    case Screen::kHoldDuration:
      target = screen_hold_duration;
      break;
    case Screen::kFactoryReset:
      target = screen_factory_reset;
      break;
  }
  if (target != nullptr) {
    lv_screen_load(target);
    current_screen = screen;
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

lv_obj_t* createMenuListButton(lv_obj_t* parent, const char* label,
                               lv_event_cb_t on_click) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_width(button, LV_PCT(100));
  lv_obj_set_height(button, 44);
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

lv_obj_t* createScrollableMenuList(lv_obj_t* parent) {
  lv_obj_t* list = lv_obj_create(parent);
  lv_obj_set_size(list, LV_PCT(100), 200);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 12, 0);
  lv_obj_set_style_pad_row(list, 8, 0);
  lv_obj_set_style_pad_right(list, 16, 0);
  lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START);
  lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ON);
  lv_obj_set_style_width(list, 8, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(list, kAccentColor, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(list, LV_OPA_70, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(list, 4, LV_PART_SCROLLBAR);
  return list;
}

#ifdef ECHOLOCATION_DEBUG
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
           "Found: %d/5\n"
           "Cached: %d",
           info.sd_mounted ? "yes" : "no",
           info.audio_dir_exists ? "yes" : "no",
           info.probe_a_wav ? "yes" : "no", info.probe_b_wav ? "yes" : "no",
           info.probe_c_wav ? "yes" : "no", info.probe_space_wav ? "yes" : "no",
           info.probe_enter_wav ? "yes" : "no", info.probe_files_found,
           info.cached_wav_count);
  lv_label_set_text(audio_debug_label, text);
}

void onRefreshAudioDebugClicked(lv_event_t* event) {
  (void)event;
  keyAudioRefresh();
  refreshAudioDebugLabel();
}

void onDebugMenuClicked(lv_event_t* event) {
  (void)event;
  refreshAudioDebugLabel();
  showScreen(Screen::kDebug);
}
#endif

void onSettingsClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kSettings);
}

void onBluetoothMenuClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kBluetooth);
}

void onComputerOutputMenuClicked(lv_event_t* event) {
  (void)event;
  refreshBluetoothOutputStatus();
  showScreen(Screen::kComputerOutput);
}

void updateReadyForConnectionLabel() {
  if (bluetooth_output_status_label == nullptr) {
    return;
  }

  char text[48];
  const char* base = "Ready for connection";
  const size_t base_len = strlen(base);
  memcpy(text, base, base_len);
  for (uint8_t i = 0; i < bluetooth_output_dot_count; ++i) {
    text[base_len + i] = '.';
  }
  text[base_len + bluetooth_output_dot_count] = '\0';
  lv_label_set_text(bluetooth_output_status_label, text);
}

void onBluetoothOutputDotsTimer(lv_timer_t* timer) {
  (void)timer;
  if (bluetooth_output_status_label == nullptr ||
      !bluetooth_output_enabled || computerOutputBluetoothConnected()) {
    return;
  }

  bluetooth_output_dot_count++;
  if (bluetooth_output_dot_count > kBluetoothOutputMaxDots) {
    bluetooth_output_dot_count = 1;
  }
  updateReadyForConnectionLabel();
}

void refreshBluetoothOutputStatus() {
  if (bluetooth_output_status_label == nullptr) {
    return;
  }

  BluetoothOutputDisplayState state;
  if (!bluetooth_output_enabled) {
    state = BluetoothOutputDisplayState::kHidden;
  } else if (computerOutputBluetoothConnected()) {
    state = BluetoothOutputDisplayState::kConnected;
  } else {
    state = BluetoothOutputDisplayState::kReady;
  }

  if (state == displayed_bluetooth_output_state) {
    return;
  }
  displayed_bluetooth_output_state = state;

  switch (state) {
    case BluetoothOutputDisplayState::kHidden:
      lv_obj_add_flag(bluetooth_output_status_label, LV_OBJ_FLAG_HIDDEN);
      if (bluetooth_output_dots_timer != nullptr) {
        lv_timer_pause(bluetooth_output_dots_timer);
      }
      break;

    case BluetoothOutputDisplayState::kConnected:
      lv_obj_remove_flag(bluetooth_output_status_label, LV_OBJ_FLAG_HIDDEN);
      if (bluetooth_output_dots_timer != nullptr) {
        lv_timer_pause(bluetooth_output_dots_timer);
      }
      lv_label_set_text(bluetooth_output_status_label, "Connected");
      lv_obj_set_style_text_color(bluetooth_output_status_label, kConnectedColor, 0);
      break;

    case BluetoothOutputDisplayState::kReady:
      lv_obj_remove_flag(bluetooth_output_status_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_color(bluetooth_output_status_label, kAccentColor, 0);
      bluetooth_output_dot_count = 1;
      updateReadyForConnectionLabel();

      if (bluetooth_output_dots_timer == nullptr) {
        bluetooth_output_dots_timer = lv_timer_create(onBluetoothOutputDotsTimer,
                                                      kBluetoothOutputDotsIntervalMs,
                                                      nullptr);
      } else {
        lv_timer_reset(bluetooth_output_dots_timer);
        lv_timer_resume(bluetooth_output_dots_timer);
      }
      break;
  }
}

void onBluetoothOutputSwitchChanged(lv_event_t* event) {
  lv_obj_t* sw = lv_event_get_target_obj(event);
  bluetooth_output_enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
  deviceSettingsSaveBluetoothOutput(bluetooth_output_enabled);
  refreshBluetoothOutputStatus();
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

void refreshConnectionFlowIndicator() {
  if (connection_flow_label == nullptr) {
    return;
  }

  const bool input_usb = usbKeyboardIsConnected();
  const bool output_usb = computerOutputUsbReady();

  const char* input_icon = input_usb ? LV_SYMBOL_USB : LV_SYMBOL_CLOSE;
  const char* output_icon = output_usb ? LV_SYMBOL_USB : LV_SYMBOL_CLOSE;

  char text[32];
  snprintf(text, sizeof(text), "%s %s %s", input_icon, LV_SYMBOL_RIGHT, output_icon);
  lv_label_set_text(connection_flow_label, text);

  const bool active = input_usb || output_usb;
  lv_obj_set_style_text_color(connection_flow_label,
                              active ? kAccentColor : lv_color_hex(0x666666), 0);
}

enum class SpeakerDisplayState {
  kUnknown,
  kExternal,
  kBuiltin,
  kModuleAbsent,
};

SpeakerDisplayState displayed_speaker_state = SpeakerDisplayState::kUnknown;

void refreshSpeakerOutputIndicator() {
  if (speaker_output_label == nullptr || speaker_error_label == nullptr) {
    return;
  }

  SpeakerDisplayState state;
  if (!speakerDetectIsModulePresent()) {
    state = SpeakerDisplayState::kModuleAbsent;
  } else if (speakerDetectIsExternalConnected()) {
    state = SpeakerDisplayState::kExternal;
  } else {
    state = SpeakerDisplayState::kBuiltin;
  }

  if (state == displayed_speaker_state) {
    return;
  }
  displayed_speaker_state = state;

  switch (state) {
    case SpeakerDisplayState::kExternal:
      lv_obj_add_flag(speaker_error_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(speaker_output_label, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(speaker_output_label, LV_SYMBOL_VOLUME_MAX);
      lv_obj_set_style_text_color(speaker_output_label, kAccentColor, 0);
      break;
    case SpeakerDisplayState::kBuiltin:
      lv_obj_add_flag(speaker_error_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(speaker_output_label, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(speaker_output_label, LV_SYMBOL_VOLUME_MAX);
      lv_obj_set_style_text_color(speaker_output_label, lv_color_hex(0x666666), 0);
      break;
    case SpeakerDisplayState::kModuleAbsent:
    default:
      lv_obj_add_flag(speaker_output_label, LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(speaker_error_label, LV_OBJ_FLAG_HIDDEN);
      break;
  }
}

void onFactoryResetMenuClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kFactoryReset);
}

void onFactoryResetConfirmed(lv_event_t* event) {
  (void)event;
  deviceSettingsResetToFactory();
  showScreen(Screen::kSettings);
}

void buildLoadingScreen() {
  screen_loading = lv_obj_create(nullptr);
  styleScreen(screen_loading);

  lv_obj_t* title = lv_label_create(screen_loading);
  lv_label_set_text(title, "echolocation");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -48);

  lv_obj_t* spinner = lv_spinner_create(screen_loading);
  lv_obj_set_size(spinner, 48, 48);
  lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0);
  lv_spinner_set_anim_params(spinner, 1000, 200);

  loading_status_label = lv_label_create(screen_loading);
  lv_label_set_text(loading_status_label, "Starting up...");
  lv_obj_set_style_text_font(loading_status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(loading_status_label, lv_color_hex(0xAAAAAA), 0);
  lv_obj_set_width(loading_status_label, 280);
  lv_label_set_long_mode(loading_status_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(loading_status_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(loading_status_label, LV_ALIGN_CENTER, 0, 56);
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

  speaker_output_label = lv_label_create(screen_main);
  lv_label_set_text(speaker_output_label, LV_SYMBOL_VOLUME_MAX);
  lv_obj_set_style_text_font(speaker_output_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(speaker_output_label, lv_color_hex(0x666666), 0);
  lv_obj_align(speaker_output_label, LV_ALIGN_BOTTOM_LEFT, 12, -12);

  speaker_error_label = lv_label_create(screen_main);
  lv_label_set_text(speaker_error_label,
                     LV_SYMBOL_WARNING " audio module missing");
  lv_obj_set_style_text_font(speaker_error_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(speaker_error_label, lv_color_hex(0xFF4444), 0);
  lv_obj_align(speaker_error_label, LV_ALIGN_BOTTOM_LEFT, 12, -12);
  lv_obj_add_flag(speaker_error_label, LV_OBJ_FLAG_HIDDEN);

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

  lv_obj_t* settings_menu_list = createScrollableMenuList(screen_settings);
  createMenuListButton(settings_menu_list, "Bluetooth", onBluetoothMenuClicked);
#ifdef ECHOLOCATION_DEBUG
  createMenuListButton(settings_menu_list, "Debug", onDebugMenuClicked);
#endif
  createMenuListButton(settings_menu_list, "Volume", onVolumeMenuClicked);
  createMenuListButton(settings_menu_list, "Hold Duration", onHoldDurationMenuClicked);
  createMenuListButton(settings_menu_list, "Factory Defaults", onFactoryResetMenuClicked);

  screen_bluetooth = lv_obj_create(nullptr);
  styleScreen(screen_bluetooth);
  createHeader(screen_bluetooth, "Bluetooth", Screen::kSettings);

  lv_obj_t* bluetooth_menu_list = createScrollableMenuList(screen_bluetooth);
  createMenuListButton(bluetooth_menu_list, "Computer / Output",
                         onComputerOutputMenuClicked);

  screen_computer_output = lv_obj_create(nullptr);
  styleScreen(screen_computer_output);
  createHeader(screen_computer_output, "Computer / Output", Screen::kBluetooth);

  lv_obj_t* bluetooth_output_row = lv_obj_create(screen_computer_output);
  lv_obj_set_size(bluetooth_output_row, LV_PCT(100), 44);
  lv_obj_align(bluetooth_output_row, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_bg_opa(bluetooth_output_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bluetooth_output_row, 0, 0);
  lv_obj_set_style_pad_hor(bluetooth_output_row, 12, 0);
  lv_obj_remove_flag(bluetooth_output_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(bluetooth_output_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bluetooth_output_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* bluetooth_output_label = lv_label_create(bluetooth_output_row);
  lv_label_set_text(bluetooth_output_label, "Bluetooth Output");
  lv_obj_set_style_text_font(bluetooth_output_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(bluetooth_output_label, lv_color_white(), 0);

  bluetooth_output_switch = lv_switch_create(bluetooth_output_row);
  if (bluetooth_output_enabled) {
    lv_obj_add_state(bluetooth_output_switch, LV_STATE_CHECKED);
  }
  lv_obj_add_event_cb(bluetooth_output_switch, onBluetoothOutputSwitchChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);

  bluetooth_output_status_label = lv_label_create(screen_computer_output);
  lv_label_set_text(bluetooth_output_status_label, "Ready for connection.");
  lv_obj_set_style_text_font(bluetooth_output_status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(bluetooth_output_status_label, kAccentColor, 0);
  lv_obj_align(bluetooth_output_status_label, LV_ALIGN_TOP_LEFT, 12, 88);
  lv_obj_add_flag(bluetooth_output_status_label, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* connect_instructions = lv_spangroup_create(screen_computer_output);
  lv_obj_set_width(connect_instructions, 296);
  lv_obj_set_style_text_color(connect_instructions, lv_color_white(), 0);
  lv_obj_set_style_text_font(connect_instructions, &lv_font_montserrat_14, 0);
  lv_spangroup_set_mode(connect_instructions, LV_SPAN_MODE_BREAK);
  lv_obj_align(connect_instructions, LV_ALIGN_TOP_LEFT, 12, 116);

  lv_span_t* instructions_prefix = lv_spangroup_add_span(connect_instructions);
  lv_span_set_text(instructions_prefix,
                   "To connect, go to the bluetooth settings on your chosen "
                   "device and search for new devices to connect to. Click on '");

  lv_span_t* instructions_device_name = lv_spangroup_add_span(connect_instructions);
  lv_span_set_text(instructions_device_name, "echolocation");
  lv_style_set_text_font(lv_span_get_style(instructions_device_name),
                         &lv_font_montserrat_14_bold);

  lv_span_t* instructions_suffix = lv_spangroup_add_span(connect_instructions);
  lv_span_set_text(instructions_suffix, "' to connect to this device");

  refreshBluetoothOutputStatus();

#ifdef ECHOLOCATION_DEBUG
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
  lv_obj_add_event_cb(refresh_button, onRefreshAudioDebugClicked, LV_EVENT_CLICKED,
                      nullptr);

  lv_obj_t* refresh_label = lv_label_create(refresh_button);
  lv_label_set_text(refresh_label, "Refresh");
  lv_obj_center(refresh_label);
#endif

  screen_factory_reset = lv_obj_create(nullptr);
  styleScreen(screen_factory_reset);
  createHeader(screen_factory_reset, "Factory Defaults", Screen::kSettings);

  lv_obj_t* factory_reset_message = lv_label_create(screen_factory_reset);
  lv_label_set_text(factory_reset_message,
                    "Reset all settings to factory defaults?\n\n"
                    "Volume and hold duration will be cleared.");
  lv_obj_set_style_text_font(factory_reset_message, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(factory_reset_message, lv_color_white(), 0);
  lv_obj_set_width(factory_reset_message, 280);
  lv_label_set_long_mode(factory_reset_message, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(factory_reset_message, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(factory_reset_message, LV_ALIGN_CENTER, 0, -24);

  lv_obj_t* confirm_reset_button = lv_btn_create(screen_factory_reset);
  lv_obj_set_size(confirm_reset_button, 160, 44);
  lv_obj_align(confirm_reset_button, LV_ALIGN_BOTTOM_MID, 0, -16);
  lv_obj_set_style_radius(confirm_reset_button, 8, 0);
  lv_obj_set_style_bg_color(confirm_reset_button, lv_color_hex(0xCC3333), 0);
  lv_obj_add_event_cb(confirm_reset_button, onFactoryResetConfirmed, LV_EVENT_CLICKED,
                      nullptr);

  lv_obj_t* confirm_reset_label = lv_label_create(confirm_reset_button);
  lv_label_set_text(confirm_reset_label, "Reset");
  lv_obj_center(confirm_reset_label);

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
}

}  // namespace

void uiSetBluetoothOutput(bool enabled) {
  bluetooth_output_enabled = enabled;
  if (bluetooth_output_switch != nullptr) {
    if (enabled) {
      lv_obj_add_state(bluetooth_output_switch, LV_STATE_CHECKED);
    } else {
      lv_obj_remove_state(bluetooth_output_switch, LV_STATE_CHECKED);
    }
  }
  refreshBluetoothOutputStatus();
}

void uiInit() {
  buildLoadingScreen();
  showScreen(Screen::kLoading);
  uiPump();
  buildScreens();
}

void uiSetLoadingStatus(const char* status) {
  if (loading_status_label == nullptr || status == nullptr) {
    return;
  }
  lv_label_set_text(loading_status_label, status);
  uiPump();
}

void uiFinishLoading() {
  showScreen(Screen::kMain);
  refreshConnectionFlowIndicator();
  refreshSpeakerOutputIndicator();
}

void uiPump() { lvglPortTick(); }

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

void uiRefreshConnectionFlow() { refreshConnectionFlowIndicator(); }

void uiRefreshBluetoothOutputStatus() { refreshBluetoothOutputStatus(); }

void uiRefreshSpeakerOutput() { refreshSpeakerOutputIndicator(); }
