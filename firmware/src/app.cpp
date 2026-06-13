#include "app.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace {

uint32_t now_ms() {
#ifdef ARDUINO
  return millis();
#else
  return 0;
#endif
}

}  // namespace

namespace echolocation {

void App::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "App begin.");

  settings_store_.load(settings_);
  EventLogger::instance().set_enabled(settings_.debug_enabled);
  layout_mapper_.set_layout(settings_.layout);
  hold_detector_.set_hold_duration_ms(settings_.hold_duration_ms);
  hold_detector_.set_hold_callback(
      [this](const HoldEvent& event) { on_hold_event(event); });

  audio_router_.begin();
  speech_player_.begin(&audio_router_);
  speech_player_.set_volume(settings_.volume);

  ui_manager_.begin();
  if (settings_.debug_enabled) {
    ui_manager_.show_debug_page(EventLogger::instance().recent_entries());
  }

  usb_keyboard_.set_key_callback(
      [this](const KeyEvent& event) { on_key_event(event); });
  ble_keyboard_.set_key_callback(
      [this](const KeyEvent& event) { on_key_event(event); });

  usb_keyboard_.begin();
  ble_keyboard_.begin();
  usb_computer_.begin();
  ble_computer_.begin();
}

void App::loop() {
  usb_keyboard_.poll();
  ble_keyboard_.poll();

  hold_detector_.update(::now_ms());
}

void App::on_key_event(const KeyEvent& event) {
  hold_detector_.process_key_event(event);

  if (!event.pressed) {
    return;
  }

  LayoutResult mapped = layout_mapper_.map_key(event);
  ui_manager_.update_current_key(mapped.spoken_token);
  speech_player_.play_token(mapped.spoken_token);

  EventLogger::instance().log(LogLevel::kDebug,
                              "Key event usage=" + std::to_string(event.usage) +
                                  (mapped.used_fallback ? " fallback" : ""));
}

void App::on_hold_event(const HoldEvent& event) {
  usb_computer_.send_keypress(event);
  ble_computer_.send_keypress(event);
  EventLogger::instance().log(LogLevel::kInfo,
                              "Hold event usage=" + std::to_string(event.usage));
}

}  // namespace echolocation
