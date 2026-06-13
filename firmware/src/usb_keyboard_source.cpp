#include "usb_keyboard_source.h"

#include "event_logger.h"

namespace echolocation {

void UsbKeyboardSource::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "USB keyboard source started.");
}

void UsbKeyboardSource::poll() {
  // TODO: poll MAX3421E and emit KeyEvent.
}

void UsbKeyboardSource::set_key_callback(KeyCallback callback) {
  key_callback_ = std::move(callback);
}

void UsbKeyboardSource::emit_event(const KeyEvent& event) {
  if (key_callback_) {
    key_callback_(event);
  }
}

}  // namespace echolocation
