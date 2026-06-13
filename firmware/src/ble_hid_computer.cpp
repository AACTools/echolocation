#include "ble_hid_computer.h"

#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HIDTypes.h>

#include <Arduino.h>
#include <string.h>

namespace {

typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

constexpr uint8_t kKeyboardReportId = 1;
constexpr char kDefaultDeviceName[] = "echolocation";

// Standard HID keyboard descriptor (boot-compatible).
const uint8_t kHidReportDescriptor[] = {
    USAGE_PAGE(1), 0x01,  USAGE(1), 0x06,  COLLECTION(1), 0x01,  REPORT_ID(1),
    kKeyboardReportId,  USAGE_PAGE(1), 0x07,  USAGE_MINIMUM(1), 0xE0,
    USAGE_MAXIMUM(1), 0xE7,  LOGICAL_MINIMUM(1), 0x00,  LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,  REPORT_COUNT(1), 0x08,  HIDINPUT(1), 0x02,  REPORT_COUNT(1),
    0x01,  REPORT_SIZE(1), 0x08,  HIDINPUT(1), 0x01,  REPORT_COUNT(1), 0x05,
    REPORT_SIZE(1), 0x01,  USAGE_PAGE(1), 0x08,  USAGE_MINIMUM(1), 0x01,
    USAGE_MAXIMUM(1), 0x05,  HIDOUTPUT(1), 0x02,  REPORT_COUNT(1), 0x01,
    REPORT_SIZE(1), 0x03,  HIDOUTPUT(1), 0x01,  REPORT_COUNT(1), 0x06,
    REPORT_SIZE(1), 0x08,  LOGICAL_MINIMUM(1), 0x00,  LOGICAL_MAXIMUM(1), 0x65,
    USAGE_PAGE(1), 0x07,  USAGE_MINIMUM(1), 0x00,  USAGE_MAXIMUM(1), 0x65,
    HIDINPUT(1), 0x00,  END_COLLECTION(0),
};

bool enabled = false;
bool advertising_active = false;
bool initialized = false;

class BleServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer* /*server*/) override { connected = true; }
  void onDisconnect(BLEServer* server) override {
    connected = false;
    if (advertising_active) {
      server->getAdvertising()->start();
    }
  }

  bool connected = false;
};

BLEHIDDevice* hid = nullptr;
BLECharacteristic* input_keyboard = nullptr;
BLEServer* server = nullptr;
BleServerCallbacks server_callbacks;
BLEAdvertising* advertising = nullptr;

char device_name[16] = "echolocation";

void sendReport(uint8_t mod, uint8_t key) {
  if (!server_callbacks.connected || input_keyboard == nullptr) {
    return;
  }

  KeyReport report = {};
  report.modifiers = mod;
  report.keys[0] = key;
  input_keyboard->setValue(reinterpret_cast<uint8_t*>(&report), sizeof(report));
  input_keyboard->notify();
}

void ensureInitialized() {
  if (initialized) {
    return;
  }

  BLEDevice::init(device_name);
  server = BLEDevice::createServer();
  server->setCallbacks(&server_callbacks);

  hid = new BLEHIDDevice(server);
  input_keyboard = hid->inputReport(kKeyboardReportId);
  hid->manufacturer()->setValue("echolocation");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x01);
  hid->reportMap(const_cast<uint8_t*>(kHidReportDescriptor),
                 sizeof(kHidReportDescriptor));
  hid->startServices();

  advertising = server->getAdvertising();
  advertising->setAppearance(0x03C1);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(true);

  initialized = true;
}

}  // namespace

void bleHidComputerBegin() {
  if (!enabled) {
    return;
  }
  ensureInitialized();
  bleHidComputerStartPairing();
}

void bleHidComputerTick() {}

void bleHidComputerSendKey(uint8_t mod, uint8_t key) {
  if (!enabled || key == 0) {
    return;
  }

  sendReport(mod, key);
  delay(8);

  KeyReport empty = {};
  if (input_keyboard != nullptr && server_callbacks.connected) {
    input_keyboard->setValue(reinterpret_cast<uint8_t*>(&empty), sizeof(empty));
    input_keyboard->notify();
  }
}

bool bleHidComputerIsConnected() { return server_callbacks.connected; }

bool bleHidComputerIsAdvertising() { return advertising_active; }

void bleHidComputerSetEnabled(bool value) {
  if (enabled == value) {
    return;
  }

  enabled = value;
  if (!enabled) {
    bleHidComputerStopPairing();
    return;
  }

  ensureInitialized();
  bleHidComputerStartPairing();
}

bool bleHidComputerIsEnabled() { return enabled; }

void bleHidComputerStartPairing() {
  if (!enabled) {
    return;
  }
  ensureInitialized();
  if (advertising != nullptr) {
    advertising->start();
    advertising_active = true;
  }
}

void bleHidComputerStopPairing() {
  if (advertising != nullptr) {
    advertising->stop();
  }
  advertising_active = false;
}

void bleHidComputerSetDeviceName(const char* name) {
  if (name == nullptr || name[0] == '\0') {
    strncpy(device_name, kDefaultDeviceName, sizeof(device_name) - 1);
  } else {
    strncpy(device_name, name, sizeof(device_name) - 1);
  }
  device_name[sizeof(device_name) - 1] = '\0';

  if (initialized) {
    BLEDevice::deinit(false);
    initialized = false;
    hid = nullptr;
    input_keyboard = nullptr;
    server = nullptr;
    advertising = nullptr;
    server_callbacks.connected = false;
    advertising_active = false;
    if (enabled) {
      ensureInitialized();
      bleHidComputerStartPairing();
    }
  }
}

const char* bleHidComputerGetDeviceName() { return device_name; }
