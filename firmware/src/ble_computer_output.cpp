#include "ble_computer_output.h"

#include "ble_radio_policy.h"
#include "ble_stack.h"

#include <HIDTypes.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <Arduino.h>

namespace {

constexpr char kDeviceName[] = "echolocation";
constexpr uint8_t kKeyboardReportId = 1;

struct KeyReport {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
};

static_assert(sizeof(KeyReport) == 8, "HID keyboard input report must be 8 bytes");

// Standard keyboard report map (matches ESP32-BLE-Keyboard / USB HID appendix C).
static const uint8_t kKeyboardReportMap[] = {
    USAGE_PAGE(1), 0x01,       // Generic Desktop
    USAGE(1), 0x06,            // Keyboard
    COLLECTION(1), 0x01,       // Application
    REPORT_ID(1), kKeyboardReportId,
    USAGE_PAGE(1), 0x07,       // Keyboard/Keypad
    USAGE_MINIMUM(1), 0xE0,    // Left Control
    USAGE_MAXIMUM(1), 0xE7,    // Right GUI
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x08,     // 8 modifier bits
    HIDINPUT(1), 0x02,
    REPORT_COUNT(1), 0x01,     // 1 reserved byte
    REPORT_SIZE(1), 0x08,
    HIDINPUT(1), 0x01,
    REPORT_COUNT(1), 0x05,     // LED output bits
    REPORT_SIZE(1), 0x01,
    USAGE_PAGE(1), 0x08,       // LEDs
    USAGE_MINIMUM(1), 0x01,
    USAGE_MAXIMUM(1), 0x05,
    HIDOUTPUT(1), 0x02,
    REPORT_COUNT(1), 0x01,     // LED padding
    REPORT_SIZE(1), 0x03,
    HIDOUTPUT(1), 0x01,
    REPORT_COUNT(1), 0x06,     // 6 key bytes
    REPORT_SIZE(1), 0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(2), 0xE7, 0x00,
    USAGE_PAGE(1), 0x07,
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(2), 0xE7, 0x00,
    HIDINPUT(1), 0x00,
    END_COLLECTION(0),
};

NimBLEServer* ble_server = nullptr;
NimBLEHIDDevice* ble_hid = nullptr;
NimBLECharacteristic* ble_input_report = nullptr;

bool stack_ready = false;
bool output_enabled = false;
bool host_connected = false;
bool advertising_suspended = false;

void sendKeyboardReport(uint8_t mod, uint8_t key) {
  if (ble_input_report == nullptr) {
    return;
  }

  uint8_t report[sizeof(KeyReport)] = {};
  report[0] = mod;
  report[2] = key;
  ble_input_report->setValue(report, sizeof(report));
  ble_input_report->notify();
}

void releaseAllKeys() {
  if (ble_input_report == nullptr) {
    return;
  }

  uint8_t report[sizeof(KeyReport)] = {};
  ble_input_report->setValue(report, sizeof(report));
  ble_input_report->notify();
}

class BleComputerOutputServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& conn_info) override {
    (void)server;
    (void)conn_info;
    host_connected = true;
    releaseAllKeys();
    Serial.println("[ble-out] connected");
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& conn_info,
                    int reason) override {
    (void)conn_info;
    (void)reason;
    host_connected = false;
    Serial.println("[ble-out] disconnected");

    if (output_enabled) {
      server->startAdvertising();
    }
  }
};

BleComputerOutputServerCallbacks server_callbacks;

void stopAdvertising() {
  if (!stack_ready) {
    return;
  }
  NimBLEDevice::stopAdvertising();
}

void startAdvertising() {
  if (!stack_ready || !output_enabled || host_connected ||
      bleRadioPolicyIsScanActive()) {
    return;
  }

  if (NimBLEDevice::getAdvertising()->isAdvertising()) {
    advertising_suspended = false;
    return;
  }

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->reset();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(ble_hid->getHidService()->getUUID());
  advertising->setName(kDeviceName);
  advertising->start();
  advertising_suspended = false;
  Serial.println("[ble-out] advertising");
}

void disconnectHost() {
  if (ble_server == nullptr || !host_connected) {
    return;
  }

  releaseAllKeys();
  const std::vector<uint16_t> peers = ble_server->getPeerDevices();
  for (uint16_t handle : peers) {
    ble_server->disconnect(handle);
  }
}

}  // namespace

void bleComputerOutputBegin() {
  if (stack_ready) {
    return;
  }

  if (!bleStackEnsureInit(kDeviceName)) {
    return;
  }

  ble_server = NimBLEDevice::createServer();
  ble_server->setCallbacks(&server_callbacks);

  ble_hid = new NimBLEHIDDevice(ble_server);
  ble_input_report = ble_hid->getInputReport(kKeyboardReportId);
  ble_hid->getOutputReport(kKeyboardReportId);
  ble_hid->setReportMap(const_cast<uint8_t*>(kKeyboardReportMap),
                        sizeof(kKeyboardReportMap));
  ble_hid->setManufacturer("echolocation");
  ble_hid->setPnp(0x02, 0xE502, 0xA111, 0x0210);
  ble_hid->setHidInfo(0x00, 0x01);
  ble_hid->setBatteryLevel(100);

  releaseAllKeys();

  ble_server->start();
  stack_ready = true;
  Serial.println("[ble-out] stack ready");
}

void bleComputerOutputSetEnabled(bool enabled) {
  output_enabled = enabled;

  if (!stack_ready) {
    return;
  }

  if (enabled) {
    startAdvertising();
    return;
  }

  stopAdvertising();
  disconnectHost();
  host_connected = false;
}

void bleComputerOutputTick() {
  if (!stack_ready || !output_enabled || host_connected) {
    return;
  }

  if (bleRadioPolicyIsScanActive()) {
    if (NimBLEDevice::getAdvertising()->isAdvertising()) {
      stopAdvertising();
      advertising_suspended = true;
    }
    return;
  }

  if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
    startAdvertising();
  }
}

void bleComputerOutputSendKey(uint8_t mod, uint8_t key) {
  if (!host_connected || ble_input_report == nullptr || key == 0) {
    return;
  }

  sendKeyboardReport(mod, key);
  delay(8);
  releaseAllKeys();
}

void bleComputerOutputSendBootReport(const uint8_t report[8]) {
  if (!host_connected || ble_input_report == nullptr || report == nullptr) {
    return;
  }

  uint8_t out[sizeof(KeyReport)] = {};
  out[0] = report[0];
  uint8_t slot = 2;
  for (uint8_t i = 2; i < 8 && slot < sizeof(KeyReport); ++i) {
    const uint8_t key = report[i];
    if (key != 0 && key != 1) {
      out[slot++] = key;
    }
  }
  ble_input_report->setValue(out, sizeof(out));
  ble_input_report->notify();
}

bool bleComputerOutputIsConnected() {
  if (!stack_ready) {
    return false;
  }
  return host_connected || ble_server->getConnectedCount() > 0;
}
