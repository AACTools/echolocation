#pragma once

#include <cstdint>
#include <functional>

#include "key_event.h"

namespace echo {

using KeyboardEventCallback = std::function<void(const KeyEvent&)>;

class UsbKeyboardSource {
 public:
  void begin(KeyboardEventCallback callback);
  void tick(uint32_t now_ms);
  bool isKeyboardConnected() const;
  uint8_t usbTaskState() const;
  bool isHidReady() const;
  uint32_t lastActivityMs() const;
  bool usbHostInitOk() const;
  uint8_t usbVbusState() const;

 private:
  KeyboardEventCallback callback_;
  bool connected_ = false;
  uint32_t last_activity_ms_ = 0;
  uint32_t last_reinit_ms_ = 0;
  uint8_t usb_task_state_ = 0;
  bool hid_ready_ = false;
  bool usb_host_init_ok_ = false;
  uint8_t usb_vbus_state_ = 0;
  static constexpr uint32_t kDisconnectGraceMs = 3000;
  static constexpr uint32_t kIllegalStateReinitMs = 1000;
};

}  // namespace echo
