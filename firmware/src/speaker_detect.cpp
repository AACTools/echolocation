#include "speaker_detect.h"

#include <M5Unified.h>

namespace {

constexpr uint8_t kModuleI2cAddr = 0x33;
constexpr uint8_t kHpInsertReg = 0x20;
constexpr uint32_t kPollIntervalMs = 50;
constexpr uint8_t kConnectDebouncePolls = 1;
constexpr uint8_t kDisconnectDebouncePolls = 2;
constexpr uint32_t kI2cSpeedHz = 400000;

bool module_present = false;
bool external_connected = false;
bool pending_connected = false;
uint8_t pending_count = 0;
uint32_t last_poll_ms = 0;
uint32_t i2c_fail_count = 0;

bool readHpInsertStatus(bool* inserted) {
  if (inserted == nullptr) {
    return false;
  }

  uint8_t status = 0;
  if (!M5.In_I2C.readRegister(kModuleI2cAddr, kHpInsertReg, &status, 1,
                                kI2cSpeedHz)) {
    return false;
  }

  *inserted = status != 0;
  return true;
}

uint8_t requiredDebouncePolls(bool connected) {
  return connected ? kConnectDebouncePolls : kDisconnectDebouncePolls;
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
  pending_count = requiredDebouncePolls(external_connected);
  last_poll_ms = millis();
  i2c_fail_count = 0;
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
    ++i2c_fail_count;
    return false;
  }
  i2c_fail_count = 0;

  const bool raw_connected = inserted;
  if (raw_connected == pending_connected) {
    if (pending_count < requiredDebouncePolls(pending_connected)) {
      ++pending_count;
    }
  } else {
    pending_connected = raw_connected;
    pending_count = 1;
  }

  if (pending_count < requiredDebouncePolls(pending_connected)) {
    return false;
  }

  if (external_connected == pending_connected) {
    return false;
  }

  external_connected = pending_connected;

  return true;
}

bool speakerDetectIsModulePresent() { return module_present; }

bool speakerDetectIsExternalConnected() { return external_connected; }
