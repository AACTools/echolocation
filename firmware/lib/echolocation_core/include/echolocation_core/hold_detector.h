#pragma once

#include <functional>
#include <unordered_map>

#include "echolocation_core/key_event.h"

namespace echolocation {

struct HoldEvent {
  uint16_t usage = 0;
  uint8_t modifiers = 0;
  uint32_t held_ms = 0;
};

class HoldDetector {
 public:
  using HoldCallback = std::function<void(const HoldEvent&)>;

  explicit HoldDetector(uint32_t hold_duration_ms = 600);

  void set_hold_duration_ms(uint32_t hold_duration_ms);
  void set_hold_callback(HoldCallback callback);

  void process_key_event(const KeyEvent& event);
  void update(uint32_t now_ms);

 private:
  struct HoldState {
    uint32_t start_ms = 0;
    bool emitted = false;
    uint8_t modifiers = 0;
  };

  uint32_t hold_duration_ms_;
  HoldCallback hold_callback_;
  std::unordered_map<uint16_t, HoldState> active_keys_;
};

}  // namespace echolocation
