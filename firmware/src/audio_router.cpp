#include "audio_router.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <Wire.h>
#endif

namespace echo {

#ifndef NATIVE_TEST
bool AudioRouter::tryReadStatus(uint8_t* status_out) {
  Wire.beginTransmission(kModuleAudioStm32Address);
  Wire.write(0x00);
  const uint8_t write_result = Wire.endTransmission(false);
  if (write_result != 0) {
    address_acknowledged_ = false;
    return false;
  }
  address_acknowledged_ = true;

  if (read_disabled_) {
    return false;
  }

  if (Wire.requestFrom(static_cast<uint8_t>(kModuleAudioStm32Address),
                       static_cast<uint8_t>(1)) != 1) {
    read_disabled_ = true;
    return false;
  }

  *status_out = Wire.read();
  return true;
}
#endif

void AudioRouter::begin() {
#ifndef NATIVE_TEST
  Wire.begin(12, 11);
  Wire.setTimeOut(2);

  uint8_t status = 0;
  if (tryReadStatus(&status)) {
    module_present_ = true;
    external_active_ = (status & 0x01) != 0;
    if (external_active_) {
      M5.Speaker.end();
    }
  } else {
    module_present_ = false;
  }
#endif
}

void AudioRouter::tick() {
#ifndef NATIVE_TEST
  const uint32_t now_ms = millis();
  const uint32_t interval =
      module_present_ ? kPollIntervalMs : kAbsentProbeIntervalMs;
  if (now_ms - last_poll_ms_ < interval) {
    return;
  }
  last_poll_ms_ = now_ms;

  if (!module_present_) {
    if (read_disabled_ && address_acknowledged_) {
      return;
    }

    uint8_t status = 0;
    if (!tryReadStatus(&status)) {
      return;
    }

    module_present_ = true;
    read_disabled_ = false;
    const bool headphone_inserted = (status & 0x01) != 0;
    external_active_ = headphone_inserted;
    if (external_active_) {
      M5.Speaker.end();
    }
    return;
  }

  uint8_t status = 0;
  if (!tryReadStatus(&status)) {
    module_present_ = false;
    return;
  }

  const bool headphone_inserted = (status & 0x01) != 0;
  if (headphone_inserted != external_active_) {
    external_active_ = headphone_inserted;
    if (external_active_) {
      M5.Speaker.end();
    } else {
      M5.Speaker.begin();
    }
  }
#endif
}

bool AudioRouter::isExternalOutputActive() const { return external_active_; }

}  // namespace echo
