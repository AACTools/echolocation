#include "hold_detector.h"

namespace echolocation {

HoldDetector::HoldDetector(uint32_t hold_duration_ms)
    : hold_duration_ms_(hold_duration_ms) {}

void HoldDetector::set_hold_duration_ms(uint32_t hold_duration_ms) {
  hold_duration_ms_ = hold_duration_ms;
}

void HoldDetector::set_hold_callback(HoldCallback callback) {
  hold_callback_ = std::move(callback);
}

void HoldDetector::process_key_event(const KeyEvent& event) {
  if (event.pressed) {
    HoldState state;
    state.start_ms = event.timestamp_ms;
    state.emitted = false;
    state.modifiers = event.modifiers;
    active_keys_[event.usage] = state;
    return;
  }

  active_keys_.erase(event.usage);
}

void HoldDetector::update(uint32_t now_ms) {
  if (!hold_callback_) {
    return;
  }

  for (auto& entry : active_keys_) {
    HoldState& state = entry.second;
    if (state.emitted) {
      continue;
    }

    const uint32_t elapsed = now_ms - state.start_ms;
    if (elapsed >= hold_duration_ms_) {
      HoldEvent event;
      event.usage = entry.first;
      event.modifiers = state.modifiers;
      event.held_ms = elapsed;
      state.emitted = true;
      hold_callback_(event);
    }
  }
}

}  // namespace echolocation
