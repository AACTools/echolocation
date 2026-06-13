#pragma once

#include "audio_router.h"
#include "ble_hid_computer.h"
#include "ble_keyboard_source.h"
#include "device_settings_store.h"
#include "event_logger.h"
#include "layout_mapper.h"
#include "speech_player.h"
#include "ui_manager.h"
#include "usb_hid_computer.h"
#include "usb_keyboard_source.h"

namespace echolocation {

class App {
 public:
  void begin();
  void loop();

 private:
  void on_key_event(const KeyEvent& event);
  void on_hold_event(const HoldEvent& event);

  SettingsModel settings_;
  DeviceSettingsStore settings_store_;
  LayoutMapper layout_mapper_{LayoutType::kUnknown};
  HoldDetector hold_detector_{600};

  UsbKeyboardSource usb_keyboard_;
  BleKeyboardSource ble_keyboard_;
  UsbHidComputer usb_computer_;
  BleHidComputer ble_computer_;

  AudioRouter audio_router_;
  SpeechPlayer speech_player_;
  UIManager ui_manager_;
};

}  // namespace echolocation
