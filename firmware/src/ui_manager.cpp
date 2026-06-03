#include "ui_manager.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#endif

namespace echo {

void UiManager::begin() {
#ifndef NATIVE_TEST
  auto& display = M5.Display;
  display.setRotation(1);
  display.setTextSize(2);
  display.fillScreen(TFT_BLACK);
#endif
  editing_settings_ = defaultDeviceSettings();
}

void UiManager::setCurrentKeyLabel(const char* label) {
  current_key_label_ = label ? label : "";
}

void UiManager::setBatteryPercent(int battery_percent) {
  battery_percent_ = battery_percent;
}

void UiManager::setErrorMessage(const char* message) {
  error_message_ = message ? message : "";
}

void UiManager::clearError() { error_message_.clear(); }

UiScreen UiManager::currentScreen() const { return screen_; }

bool UiManager::settingsChanged() const { return settings_changed_; }

DeviceSettings& UiManager::editingSettings() { return editing_settings_; }

void UiManager::acknowledgeSettingsSaved() { settings_changed_ = false; }

bool UiManager::touchInRect(int x, int y, int w, int h, int touch_x,
                            int touch_y) const {
  return touch_x >= x && touch_x <= x + w && touch_y >= y && touch_y <= y + h;
}

void UiManager::draw() {
#ifndef NATIVE_TEST
  switch (screen_) {
    case UiScreen::kMain:
      drawMain();
      break;
    case UiScreen::kSettings:
      drawSettings();
      break;
    case UiScreen::kVolume:
      drawVolume();
      break;
    case UiScreen::kBluetoothMenu:
      drawBluetoothMenu();
      break;
    case UiScreen::kBleKeyboard:
      drawBleKeyboard();
      break;
    case UiScreen::kBleComputer:
      drawBleComputer();
      break;
    case UiScreen::kHoldDuration:
      drawHoldDuration();
      break;
    case UiScreen::kFactoryResetConfirm:
      drawFactoryReset();
      break;
  }
#endif
}

void UiManager::handleTouch() {
#ifndef NATIVE_TEST
  M5.update();
  if (!M5.Touch.getCount()) {
    return;
  }
  auto touch = M5.Touch.getDetail(0);
  if (touch.state != m5::touch_state_t::touch_begin) {
    return;
  }
  const int x = touch.x;
  const int y = touch.y;

  if (screen_ == UiScreen::kMain && touchInRect(250, 200, 60, 35, x, y)) {
    screen_ = UiScreen::kSettings;
    return;
  }

  if (screen_ != UiScreen::kMain && touchInRect(5, 5, 80, 30, x, y)) {
    screen_ = screen_ == UiScreen::kSettings ? UiScreen::kMain
                                               : UiScreen::kSettings;
    return;
  }

  if (screen_ == UiScreen::kSettings) {
    if (touchInRect(10, 50, 300, 30, x, y)) screen_ = UiScreen::kVolume;
    if (touchInRect(10, 90, 300, 30, x, y))
      screen_ = UiScreen::kBluetoothMenu;
    if (touchInRect(10, 130, 300, 30, x, y))
      screen_ = UiScreen::kHoldDuration;
    if (touchInRect(10, 170, 300, 30, x, y))
      screen_ = UiScreen::kFactoryResetConfirm;
    return;
  }

  if (screen_ == UiScreen::kVolume) {
    if (touchInRect(10, 80, 40, 40, x, y)) {
      if (editing_settings_.volume_percent >= 10) {
        editing_settings_.volume_percent -= 10;
        settings_changed_ = true;
      }
    }
    if (touchInRect(270, 80, 40, 40, x, y)) {
      if (editing_settings_.volume_percent <= 90) {
        editing_settings_.volume_percent += 10;
        settings_changed_ = true;
      }
    }
    return;
  }

  if (screen_ == UiScreen::kBluetoothMenu) {
    if (touchInRect(10, 50, 300, 30, x, y))
      screen_ = UiScreen::kBleKeyboard;
    if (touchInRect(10, 90, 300, 30, x, y))
      screen_ = UiScreen::kBleComputer;
    return;
  }

  if (screen_ == UiScreen::kHoldDuration) {
    if (touchInRect(10, 80, 40, 40, x, y) &&
        editing_settings_.hold_duration_ms > kMinHoldDurationMs) {
      editing_settings_.hold_duration_ms -= 100;
      settings_changed_ = true;
    }
    if (touchInRect(270, 80, 40, 40, x, y) &&
        editing_settings_.hold_duration_ms < kMaxHoldDurationMs) {
      editing_settings_.hold_duration_ms += 100;
      settings_changed_ = true;
    }
  }
#endif
}

void UiManager::drawMain() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setCursor(10, 10);
  d.printf("Battery: %d%%", battery_percent_ < 0 ? 0 : battery_percent_);
  d.setCursor(10, 60);
  d.setTextSize(4);
  d.println(current_key_label_.empty() ? "-" : current_key_label_.c_str());
  d.setTextSize(2);
  if (!error_message_.empty()) {
    d.setTextColor(TFT_RED, TFT_BLACK);
    d.setCursor(10, 150);
    d.println(error_message_.c_str());
    d.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  d.fillRect(250, 200, 60, 35, TFT_BLUE);
  d.setCursor(255, 208);
  d.print("Set");
#endif
}

void UiManager::drawSettings() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.println("Settings");
  d.setCursor(10, 50);
  d.println("> Volume");
  d.setCursor(10, 90);
  d.println("> Bluetooth");
  d.setCursor(10, 130);
  d.println("> Hold duration");
  d.setCursor(10, 170);
  d.println("> Factory reset");
  d.setCursor(5, 5);
  d.print("< Back");
#endif
}

void UiManager::drawVolume() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.printf("Volume: %u%%", editing_settings_.volume_percent);
  d.drawRect(10, 80, 40, 40, TFT_WHITE);
  d.drawRect(270, 80, 40, 40, TFT_WHITE);
  d.setCursor(20, 92);
  d.print("-");
  d.setCursor(285, 92);
  d.print("+");
#endif
}

void UiManager::drawBluetoothMenu() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.println("Bluetooth");
  d.setCursor(10, 50);
  d.println("> Keyboard connection");
  d.setCursor(10, 90);
  d.println("> Computer connection");
#endif
}

void UiManager::drawBleKeyboard() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.println("BLE Keyboard");
  d.setCursor(10, 50);
  d.println("Tap screen to start scan");
#endif
}

void UiManager::drawBleComputer() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.println("BLE Computer");
  d.setCursor(10, 50);
  d.println("Device advertises as");
  d.setCursor(10, 80);
  d.println(editing_settings_.ble_computer_name);
#endif
}

void UiManager::drawHoldDuration() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setCursor(10, 10);
  d.printf("Hold: %lu ms", editing_settings_.hold_duration_ms);
  d.drawRect(10, 80, 40, 40, TFT_WHITE);
  d.drawRect(270, 80, 40, 40, TFT_WHITE);
#endif
}

void UiManager::drawFactoryReset() {
#ifndef NATIVE_TEST
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_RED, TFT_BLACK);
  d.setCursor(10, 40);
  d.println("Factory reset?");
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.fillRect(10, 120, 130, 40, TFT_RED);
  d.setCursor(20, 132);
  d.print("Confirm");
#endif
}

}  // namespace echo
