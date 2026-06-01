#include "hold_detector.h"

namespace echo {

void HoldDetector::setHoldDurationMs(uint32_t hold_duration_ms) {
  hold_duration_ms_ = hold_duration_ms;
}

void HoldDetector::setTapCallback(HoldTapCallback callback) {
  tap_callback_ = std::move(callback);
}

HoldDetector::TrackedKey* HoldDetector::find(uint16_t slot_id) {
  for (auto& key : tracked_) {
    if (key.slot_id == slot_id) {
      return &key;
    }
  }
  return nullptr;
}

HoldDetector::TrackedKey* HoldDetector::findOrAllocate(uint16_t slot_id) {
  if (TrackedKey* existing = find(slot_id)) {
    return existing;
  }
  for (auto& key : tracked_) {
    if (key.slot_id == 0) {
      key.slot_id = slot_id;
      return &key;
    }
  }
  return nullptr;
}

void HoldDetector::emitTapIfNeeded(TrackedKey& key, uint32_t now_ms) {
  if (!key.pressed || key.hold_sent || !tap_callback_) {
    return;
  }
  if (now_ms - key.press_started_ms < hold_duration_ms_) {
    return;
  }
  key.hold_sent = true;
  tap_callback_(HoldTapAction{key.hid_usage, key.modifier_mask});
}

void HoldDetector::onKeyEvent(const KeyEvent& event, uint32_t now_ms) {
  const uint16_t slot_id = makeKeySlotId(event.hid_usage, event.modifier_mask);
  TrackedKey* key = findOrAllocate(slot_id);
  if (!key) {
    return;
  }

  if (event.pressed) {
    key->hid_usage = event.hid_usage;
    key->modifier_mask = event.modifier_mask;
    key->pressed = true;
    key->hold_sent = false;
    key->press_started_ms = event.timestamp_ms ? event.timestamp_ms : now_ms;
    emitTapIfNeeded(*key, now_ms);
    return;
  }

  key->pressed = false;
  key->hold_sent = false;
  key->press_started_ms = 0;
}

void HoldDetector::tick(uint32_t now_ms) {
  for (auto& key : tracked_) {
    if (key.pressed) {
      emitTapIfNeeded(key, now_ms);
    }
  }
}

void HoldDetector::reset() {
  for (auto& key : tracked_) {
    key = TrackedKey{};
  }
}

}  // namespace echo
