#pragma once

#include <functional>

#include "echolocation_core/key_event.h"

namespace echolocation {

class BleKeyboardSource {
 public:
  using KeyCallback = std::function<void(const KeyEvent&)>;

  void begin();
  void poll();
  void set_key_callback(KeyCallback callback);
  void emit_event(const KeyEvent& event);

 private:
  KeyCallback key_callback_;
};

}  // namespace echolocation
