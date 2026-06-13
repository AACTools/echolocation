#include "ble_keyboard_source.h"

#include "event_logger.h"

namespace echolocation {

void BleKeyboardSource::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "BLE keyboard source started.");
}

void BleKeyboardSource::poll() {
  // TODO: poll BLE HID host and emit KeyEvent.
}

void BleKeyboardSource::set_key_callback(KeyCallback callback) {
  key_callback_ = std::move(callback);
}

void BleKeyboardSource::emit_event(const KeyEvent& event) {
  if (key_callback_) {
    key_callback_(event);
  }
}

}  // namespace echolocation
