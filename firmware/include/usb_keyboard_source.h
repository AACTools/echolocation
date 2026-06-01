#pragma once

#include <functional>

#include "key_event.h"

namespace echo {

using KeyboardEventCallback = std::function<void(const KeyEvent&)>;

class UsbKeyboardSource {
 public:
  void begin(KeyboardEventCallback callback);
  void tick(uint32_t now_ms);
  bool isKeyboardConnected() const;

 private:
  KeyboardEventCallback callback_;
  bool connected_ = false;
};

}  // namespace echo
