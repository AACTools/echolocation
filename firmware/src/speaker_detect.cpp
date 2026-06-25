#include "speaker_detect.h"

#include <M5Unified.h>

namespace {

constexpr uint8_t kModuleI2cAddr = 0x33;
constexpr uint8_t kHpInsertReg = 0x20;
constexpr uint32_t kPollIntervalMs = 200;
constexpr uint8_t kDebouncePolls = 2;
constexpr uint32_t kI2cSpeedHz = 400000;

bool module_present = false;
bool external_connected = false;
bool pending_connected = false;
uint8_t pending_count = 0;
uint32_t last_poll_ms = 0;

bool readHpInsertStatus(bool* inserted) {
  if (inserted == nullptr) {
    return false;
  }

  uint8_t status = 0;
  if (!M5.In_I2C.readRegister(kModuleI2cAddr, kHpInsertReg, &status, 1,
                                kI2cSpeedHz)) {
    return false;
  }

  // Register map: 0 = not inserted, 1 = inserted.
  *inserted = status != 0;
  return true;
}

}  // namespace

void speakerDetectBegin() {
  module_present = M5.In_I2C.scanID(kModuleI2cAddr, kI2cSpeedHz);

  bool inserted = false;
  if (module_present && readHpInsertStatus(&inserted)) {
    external_connected = inserted;
  } else {
    external_connected = false;
  }

  pending_connected = external_connected;
  pending_count = kDebouncePolls;
  last_poll_ms = millis();

#ifdef ECHOLOCATION_BLE_DEBUG
  Serial.printf("[speaker] module=%d inserted=%d\n", module_present,
                external_connected);
#endif
}

bool speakerDetectPoll() {
  if (!module_present) {
    return false;
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_poll_ms < kPollIntervalMs) {
    return false;
  }
  last_poll_ms = now_ms;

  bool inserted = false;
  if (!readHpInsertStatus(&inserted)) {
    return false;
  }

  const bool raw_connected = inserted;
  if (raw_connected == pending_connected) {
    if (pending_count < kDebouncePolls) {
      ++pending_count;
    }
  } else {
    pending_connected = raw_connected;
    pending_count = 1;
  }

  if (pending_count < kDebouncePolls) {
    return false;
  }

  if (external_connected == pending_connected) {
    return false;
  }

  external_connected = pending_connected;

#ifdef ECHOLOCATION_BLE_DEBUG
  Serial.printf("[speaker] route changed inserted=%d\n", external_connected);
#endif

  return true;
}

bool speakerDetectIsModulePresent() { return module_present; }

bool speakerDetectIsExternalConnected() { return external_connected; }
