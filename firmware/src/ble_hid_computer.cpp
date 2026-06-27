#include "ble_hid_computer.h"

#include "ui.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <HIDTypes.h>

#include <esp_gap_ble_api.h>

#include <Arduino.h>
#include <string.h>

#ifdef ECHOLOCATION_BLE_DEBUG
#define BLE_LOG(fmt, ...) Serial.printf("[BLE] " fmt "\n", ##__VA_ARGS__)
#else
#define BLE_LOG(fmt, ...) ((void)0)
#endif

namespace {

typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

constexpr uint8_t kKeyboardReportId = 1;
constexpr char kDefaultDeviceName[] = "echolocation";

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
bool link_connected = false;
bool hid_ready = false;
bool have_server_peer = false;
bool pending_computer_ui = false;
esp_bd_addr_t server_peer_addr = {};

BLEHIDDevice* hid = nullptr;
BLECharacteristic* input_keyboard = nullptr;
BLECharacteristic* output_keyboard = nullptr;
BLEServer* server = nullptr;
BLEAdvertising* advertising = nullptr;

char device_name[16] = "echolocation";

#ifdef ECHOLOCATION_BLE_DEBUG
void clearBondedDevices() {
  int bond_count = esp_ble_get_bond_device_num();
  if (bond_count <= 0) {
    return;
  }

  esp_ble_bond_dev_t* bonds = static_cast<esp_ble_bond_dev_t*>(
      malloc(sizeof(esp_ble_bond_dev_t) * bond_count));
  if (bonds == nullptr) {
    return;
  }

  esp_ble_get_bond_device_list(&bond_count, bonds);
  for (int i = 0; i < bond_count; ++i) {
    esp_ble_remove_bond_device(bonds[i].bd_addr);
  }
  free(bonds);
  BLE_LOG("cleared %d bonded peer(s)", bond_count);
}
#endif

BLE2902* inputNotificationsDescriptor() {
  if (input_keyboard == nullptr) {
    return nullptr;
  }
  return static_cast<BLE2902*>(
      input_keyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902)));
}

void setInputNotifications(bool on) {
  BLE2902* cccd = inputNotificationsDescriptor();
  if (cccd != nullptr) {
    cccd->setNotifications(on);
  }
}

void logCccdState(const char* context) {
  BLE2902* cccd = inputNotificationsDescriptor();
  if (cccd == nullptr) {
    BLE_LOG("%s: no CCCD", context);
    return;
  }
  BLE_LOG("%s: notify=%d indicate=%d", context, cccd->getNotifications(),
          cccd->getIndications());
}

void clearServerPeer() {
  have_server_peer = false;
  memset(server_peer_addr, 0, sizeof(server_peer_addr));
}

void setServerPeer(const esp_bd_addr_t addr) {
  memcpy(server_peer_addr, addr, sizeof(server_peer_addr));
  have_server_peer = true;
}

bool isServerPeer(const esp_bd_addr_t addr) {
  return have_server_peer &&
         memcmp(addr, server_peer_addr, sizeof(server_peer_addr)) == 0;
}

void setHidReady(bool ready) {
  if (hid_ready == ready) {
    return;
  }
  hid_ready = ready;
  pending_computer_ui = true;
}

void processPendingComputerUi() {
  if (!pending_computer_ui) {
    return;
  }
  pending_computer_ui = false;
  uiRefreshComputerConnectionStatus();
  uiRefreshConnectionFlow();
}

class InputReportCallbacks : public BLECharacteristicCallbacks {
 public:
  void onStatus(BLECharacteristic* /*characteristic*/,
                BLECharacteristicCallbacks::Status status,
                uint32_t code) override {
    const char* label = "unknown";
    switch (status) {
      case BLECharacteristicCallbacks::Status::SUCCESS_NOTIFY:
        label = "SUCCESS_NOTIFY";
        break;
      case BLECharacteristicCallbacks::Status::ERROR_NO_CLIENT:
        label = "ERROR_NO_CLIENT";
        break;
      case BLECharacteristicCallbacks::Status::ERROR_NOTIFY_DISABLED:
        label = "ERROR_NOTIFY_DISABLED";
        break;
      case BLECharacteristicCallbacks::Status::ERROR_GATT:
        label = "ERROR_GATT";
        break;
      default:
        break;
    }
    BLE_LOG("notify status=%s code=%lu", label, static_cast<unsigned long>(code));
  }
};

class OutputReportCallbacks : public BLECharacteristicCallbacks {
 public:
  void onWrite(BLECharacteristic* characteristic) override {
    const std::string& value = characteristic->getValue();
    if (!value.empty()) {
      BLE_LOG("host LED state=0x%02x", static_cast<uint8_t>(value[0]));
    }
  }
};

InputReportCallbacks input_report_callbacks;
OutputReportCallbacks output_report_callbacks;

class BleSecurityCallbacks : public BLESecurityCallbacks {
 public:
  uint32_t onPassKeyRequest() override { return 0; }
  void onPassKeyNotify(uint32_t pass_key) override {
    BLE_LOG("passkey %06lu", static_cast<unsigned long>(pass_key));
  }
  bool onSecurityRequest() override { return true; }
  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) override {
    if (have_server_peer && !isServerPeer(auth_cmpl.bd_addr)) {
      BLE_LOG("auth ignored for non-host peer");
      return;
    }
    if (!have_server_peer && !link_connected) {
      BLE_LOG("auth ignored with no active host link");
      return;
    }
    if (!have_server_peer) {
      setServerPeer(auth_cmpl.bd_addr);
    }

    if (auth_cmpl.success) {
      setHidReady(true);
      setInputNotifications(true);
      logCccdState("authComplete");
      BLE_LOG("pairing complete");
    } else {
      setHidReady(false);
      BLE_LOG("pairing failed reason=%d", auth_cmpl.fail_reason);
    }
  }
  bool onConfirmPIN(uint32_t pin) override {
    BLE_LOG("confirm PIN %06lu", static_cast<unsigned long>(pin));
    return true;
  }
};

BleSecurityCallbacks security_callbacks;

class BleServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer* ble_server,
                 esp_ble_gatts_cb_param_t* param) override {
    link_connected = true;
    setHidReady(false);
    setInputNotifications(true);

    if (advertising != nullptr) {
      advertising->stop();
      advertising_active = false;
    }

    if (param != nullptr) {
      setServerPeer(param->connect.remote_bda);
      esp_ble_conn_update_params_t conn_params = {};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      conn_params.latency = 0;
      conn_params.max_int = 0x18;
      conn_params.min_int = 0x0c;
      conn_params.timeout = 400;
      esp_ble_gap_update_conn_params(&conn_params);
    }

    BLE_LOG("link up peers=%u", ble_server->getConnectedCount());
  }

  void onDisconnect(BLEServer* /*ble_server*/) override {
    link_connected = false;
    clearServerPeer();
    setHidReady(false);
    setInputNotifications(false);
    BLE_LOG("disconnected");

    if (enabled && advertising != nullptr) {
      advertising->start();
      advertising_active = true;
      BLE_LOG("advertising restarted");
    }
  }
};

BleServerCallbacks server_callbacks;

void primeInputNotifications() {
  BLE2902* cccd = inputNotificationsDescriptor();
  if (cccd == nullptr) {
    return;
  }
  // Bonded iOS hosts often skip rewriting CCCD on reconnect.
  cccd->setNotifications(true);
}

void configureInputReport(BLECharacteristic* input) {
  if (input == nullptr) {
    return;
  }
  input->setCallbacks(&input_report_callbacks);
  primeInputNotifications();
}

void sendReport(uint8_t mod, uint8_t key) {
  if (!hid_ready || input_keyboard == nullptr) {
    BLE_LOG("send skipped ready=%d connected=%d", hid_ready, link_connected);
    return;
  }

  if (server != nullptr && server->getConnectedCount() == 0) {
    BLE_LOG("send skipped no peers");
    return;
  }

  primeInputNotifications();

  KeyReport report = {};
  report.modifiers = mod;
  report.keys[0] = key;
  BLE_LOG("key mod=0x%02x code=0x%02x", mod, key);
  input_keyboard->setValue(reinterpret_cast<uint8_t*>(&report), sizeof(report));
  input_keyboard->notify();
}

void resetPointers() {
  hid = nullptr;
  input_keyboard = nullptr;
  output_keyboard = nullptr;
  server = nullptr;
  advertising = nullptr;
  link_connected = false;
  hid_ready = false;
  clearServerPeer();
  advertising_active = false;
}

void ensureInitialized() {
  if (initialized) {
    return;
  }

#ifdef ECHOLOCATION_BLE_DEBUG
  clearBondedDevices();
#endif

  BLEDevice::init(device_name);
  BLEDevice::setSecurityCallbacks(&security_callbacks);

  server = BLEDevice::createServer();
  server->setCallbacks(&server_callbacks);

  hid = new BLEHIDDevice(server);
  input_keyboard = hid->inputReport(kKeyboardReportId);
  output_keyboard = hid->outputReport(kKeyboardReportId);
  configureInputReport(input_keyboard);
  output_keyboard->setCallbacks(&output_report_callbacks);

  hid->manufacturer()->setValue("echolocation");
  // Bluetooth SIG vendor ID — required for iOS HID enumeration.
  hid->pnp(0x01, 0x02e5, 0x0001, 0x0100);
  hid->hidInfo(0x00, 0x01);
  hid->reportMap(const_cast<uint8_t*>(kHidReportDescriptor),
                 sizeof(kHidReportDescriptor));

  BLESecurity* security = new BLESecurity();
  security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  security->setCapability(ESP_IO_CAP_NONE);
  security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  hid->startServices();
  hid->setBatteryLevel(100);

  advertising = server->getAdvertising();
  advertising->setAppearance(0x03C1);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);

  initialized = true;
  BLE_LOG("initialized as \"%s\"", device_name);
}

void shutdownStack() {
  if (!initialized) {
    return;
  }

  BLEDevice::deinit(false);
  initialized = false;
  resetPointers();
}

}  // namespace

void bleHidComputerBegin() {
  if (!enabled) {
    return;
  }
  ensureInitialized();
  bleHidComputerStartPairing();
}

void bleHidComputerTick() { processPendingComputerUi(); }

void bleHidComputerSendKey(uint8_t mod, uint8_t key) {
  if (!enabled || key == 0) {
    return;
  }

  sendReport(mod, key);
  delay(8);

  KeyReport empty = {};
  if (hid_ready && input_keyboard != nullptr) {
    primeInputNotifications();
    input_keyboard->setValue(reinterpret_cast<uint8_t*>(&empty), sizeof(empty));
    input_keyboard->notify();
  }
}

bool bleHidComputerIsConnected() { return hid_ready; }

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
    BLE_LOG("advertising");
  }
}

void bleHidComputerStopPairing() {
  if (advertising != nullptr) {
    advertising->stop();
  }
  advertising_active = false;
}

void bleHidComputerSetDeviceName(const char* name) {
  char new_name[sizeof(device_name)] = {};
  if (name == nullptr || name[0] == '\0') {
    strncpy(new_name, kDefaultDeviceName, sizeof(new_name) - 1);
  } else {
    strncpy(new_name, name, sizeof(new_name) - 1);
  }
  new_name[sizeof(new_name) - 1] = '\0';

  if (strncmp(device_name, new_name, sizeof(device_name)) == 0) {
    return;
  }

  strncpy(device_name, new_name, sizeof(device_name) - 1);
  device_name[sizeof(device_name) - 1] = '\0';
  BLE_LOG("device name -> \"%s\"", device_name);

  if (initialized) {
    const bool was_enabled = enabled;
    shutdownStack();
    if (was_enabled) {
      ensureInitialized();
      bleHidComputerStartPairing();
    }
  }
}

const char* bleHidComputerGetDeviceName() { return device_name; }
