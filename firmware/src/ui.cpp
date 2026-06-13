#include "ui.h"

#include <lvgl.h>

namespace {

enum class Screen { kMain, kSettings };

const lv_color_t kBgColor = lv_color_hex(0x1A1A1A);
const lv_color_t kHeaderColor = lv_color_hex(0x2A2A2A);
const lv_color_t kAccentColor = lv_color_hex(0x0066FF);

lv_obj_t* screen_main = nullptr;
lv_obj_t* screen_settings = nullptr;
lv_obj_t* keyboard_icon = nullptr;
lv_obj_t* pressed_key_label = nullptr;

void styleScreen(lv_obj_t* screen) {
  lv_obj_set_style_bg_color(screen, kBgColor, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

void showScreen(Screen screen) {
  lv_obj_t* target =
      (screen == Screen::kMain) ? screen_main : screen_settings;
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

  return header;
}

void onSettingsClicked(lv_event_t* event) {
  (void)event;
  showScreen(Screen::kSettings);
}

void buildScreens() {
  screen_main = lv_obj_create(nullptr);
  styleScreen(screen_main);

  keyboard_icon = lv_label_create(screen_main);
  lv_label_set_text(keyboard_icon, LV_SYMBOL_KEYBOARD);
  lv_obj_set_style_text_font(keyboard_icon, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(keyboard_icon, kAccentColor, 0);
  lv_obj_align(keyboard_icon, LV_ALIGN_TOP_LEFT, 12, 16);
  lv_obj_add_flag(keyboard_icon, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* title = lv_label_create(screen_main);
  lv_label_set_text(title, "echolocation");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

  pressed_key_label = lv_label_create(screen_main);
  lv_label_set_text(pressed_key_label, "");
  lv_obj_set_style_text_font(pressed_key_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(pressed_key_label, lv_color_white(), 0);
  lv_obj_align(pressed_key_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(pressed_key_label, LV_OBJ_FLAG_HIDDEN);

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
}

}  // namespace

void uiInit() {
  buildScreens();
  showScreen(Screen::kMain);
}

void uiSetKeyboardConnected(bool connected) {
  if (keyboard_icon == nullptr) {
    return;
  }
  if (connected) {
    lv_obj_remove_flag(keyboard_icon, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(keyboard_icon, LV_OBJ_FLAG_HIDDEN);
  }
}

void uiSetPressedKey(const char* label) {
  if (pressed_key_label == nullptr) {
    return;
  }
  if (label == nullptr || label[0] == '\0') {
    lv_label_set_text(pressed_key_label, "");
    lv_obj_add_flag(pressed_key_label, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_label_set_text(pressed_key_label, label);
  lv_obj_remove_flag(pressed_key_label, LV_OBJ_FLAG_HIDDEN);
}
