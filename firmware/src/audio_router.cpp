#include "audio_router.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <Wire.h>
#endif

namespace echo {

void AudioRouter::begin() {
#ifndef NATIVE_TEST
  Wire.begin(12, 11);
#endif
}

void AudioRouter::tick() {
#ifndef NATIVE_TEST
  Wire.beginTransmission(kModuleAudioStm32Address);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    return;
  }
  if (Wire.requestFrom(static_cast<uint8_t>(kModuleAudioStm32Address),
                       static_cast<uint8_t>(1)) != 1) {
    return;
  }
  const uint8_t status = Wire.read();
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
