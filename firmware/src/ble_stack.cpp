#include "ble_stack.h"

#include <NimBLEDevice.h>

#include <Arduino.h>

namespace {

bool stack_initialized = false;

}  // namespace

bool bleStackEnsureInit(const char* device_name) {
  if (stack_initialized || NimBLEDevice::isInitialized()) {
    stack_initialized = true;
    return true;
  }

  if (device_name == nullptr || !NimBLEDevice::init(device_name)) {
    return false;
  }

  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  stack_initialized = true;
  Serial.println("[ble] stack initialized");
  return true;
}

bool bleStackIsInitialized() {
  return stack_initialized || NimBLEDevice::isInitialized();
}
