#include "ble_computer_output.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <Arduino.h>

namespace {

constexpr char kDeviceName[] = "echolocation";
constexpr uint8_t kKeyboardReportId = 1;
constexpr size_t kKeyboardReportSize = 8;

// Standard HID keyboard report map with report ID 1.
static const uint8_t kKeyboardReportMap[] = {
    0x05, 0x01,  // Usage Page (Generic Desktop)
    0x09, 0x06,  // Usage (Keyboard)
    0xA1, 0x01,  // Collection (Application)
    0x85, kKeyboardReportId,  // Report ID (1)
    0x05, 0x07,  // Usage Page (Key Codes)
    0x19, 0x00,  // Usage Minimum (0)
    0x29, 0xE7,  // Usage Maximum (231)
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x01,  // Logical Maximum (1)
    0x75, 0x01,  // Report Size (1)
    0x95, 0x08,  // Report Count (8) modifiers
    0x81, 0x02,  // Input (Data, Var, Abs)
    0x95, 0x01,  // Report Count (1)
    0x75, 0x08,  // Report Size (8) reserved byte
    0x81, 0x01,  // Input (Const, Array)
    0x95, 0x06,  // Report Count (6)
    0x75, 0x08,  // Report Size (8) key slots
    0x15, 0x00,  // Logical Minimum (0)
    0x25, 0x65,  // Logical Maximum (101)
    0x05, 0x07,  // Usage Page (Key Codes)
    0x19, 0x00,  // Usage Minimum (0)
    0x29, 0x65,  // Usage Maximum (101)
    0x81, 0x00,  // Input (Data, Array)
    0xC0,        // End Collection
};

NimBLEServer* ble_server = nullptr;
NimBLEHIDDevice* ble_hid = nullptr;
NimBLECharacteristic* ble_input_report = nullptr;

bool stack_ready = false;
bool output_enabled = false;
bool host_connected = false;

class BleComputerOutputServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server, NimBLEConnInfo& conn_info) override {
    (void)server;
    (void)conn_info;
    host_connected = true;
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

void sendKeyboardReport(uint8_t mod, uint8_t key) {
  if (ble_input_report == nullptr) {
    return;
  }

  uint8_t report[kKeyboardReportSize + 1] = {};
  report[0] = kKeyboardReportId;
  report[1] = mod;
  report[3] = key;
  ble_input_report->setValue(report, sizeof(report));
  ble_input_report->notify();
}

void releaseAllKeys() {
  sendKeyboardReport(0, 0);
}

void stopAdvertising() {
  if (!stack_ready) {
    return;
  }
  NimBLEDevice::stopAdvertising();
}

void startAdvertising() {
  if (!stack_ready || !output_enabled || host_connected) {
    return;
  }

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->reset();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(ble_hid->getHidService()->getUUID());
  advertising->setName(kDeviceName);
  advertising->start();
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

  NimBLEDevice::init(kDeviceName);
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  ble_server = NimBLEDevice::createServer();
  ble_server->setCallbacks(&server_callbacks);

  ble_hid = new NimBLEHIDDevice(ble_server);
  ble_input_report = ble_hid->getInputReport(kKeyboardReportId);
  ble_hid->setReportMap(const_cast<uint8_t*>(kKeyboardReportMap),
                        sizeof(kKeyboardReportMap));
  ble_hid->setManufacturer("echolocation");
  ble_hid->setPnp(0x02, 0xE502, 0xA111, 0x0210);
  ble_hid->setHidInfo(0x00, 0x02);
  ble_hid->setBatteryLevel(100);

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

bool bleComputerOutputIsConnected() {
  if (!stack_ready) {
    return false;
  }
  return host_connected || ble_server->getConnectedCount() > 0;
}
