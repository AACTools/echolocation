#pragma once

#include <cstdint>
#include <functional>

#include "key_event.h"

namespace echo {

struct HoldTapAction {
  uint8_t hid_usage = 0;
  uint8_t modifier_mask = 0;

  HoldTapAction() = default;
  HoldTapAction(uint8_t usage, uint8_t modifiers)
      : hid_usage(usage), modifier_mask(modifiers) {}
};

using HoldTapCallback = std::function<void(const HoldTapAction&)>;

class HoldDetector {
 public:
  static constexpr size_t kMaxTrackedKeys = 16;
  static constexpr uint32_t kDefaultHoldDurationMs = 500;

  void setHoldDurationMs(uint32_t hold_duration_ms);
  void setTapCallback(HoldTapCallback callback);

  void onKeyEvent(const KeyEvent& event, uint32_t now_ms);
  void tick(uint32_t now_ms);
  void reset();

 private:
  struct TrackedKey {
    uint16_t slot_id = 0;
    uint8_t hid_usage = 0;
    uint8_t modifier_mask = 0;
    bool pressed = false;
    bool hold_sent = false;
    uint32_t press_started_ms = 0;
  };

  TrackedKey* findOrAllocate(uint16_t slot_id);
  TrackedKey* find(uint16_t slot_id);
  void emitTapIfNeeded(TrackedKey& key, uint32_t now_ms);

  uint32_t hold_duration_ms_ = kDefaultHoldDurationMs;
  HoldTapCallback tap_callback_;
  TrackedKey tracked_[kMaxTrackedKeys]{};
};

}  // namespace echo
