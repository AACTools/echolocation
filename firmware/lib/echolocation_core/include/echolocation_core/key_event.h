#pragma once

#include <stdint.h>

namespace echolocation {

struct KeyEvent {
  uint16_t usage = 0;
  uint8_t modifiers = 0;
  bool pressed = false;
  uint32_t timestamp_ms = 0;
};

}  // namespace echolocation
