#include "ble_keyboard.h"

#include "ble_hid_computer.h"
#include "device_settings_store.h"
#include "keyboard_input.h"
#include "ui.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>

#include <Arduino.h>

#include <string.h>

#define KB_LOG(fmt, ...)                                                      \
  do {                                                                        \
    Serial.printf("[BLE-KB] " fmt "\n", ##__VA_ARGS__);                       \
  } while (0)

namespace {

constexpr uint16_t kHidServiceUuid = 0x1812;
constexpr uint16_t kBootKeyboardInputUuid = 0x2A22;
constexpr uint16_t kReportUuid = 0x2A4D;
constexpr uint16_t kProtocolModeUuid = 0x2A4E;

constexpr size_t kMaxDiscoveredDevices = 16;
constexpr uint8_t kAutoReconnectMaxAttempts = 3;
constexpr uint32_t kReconnectDelayMs = 2000;
constexpr uint32_t kConnectScanSeconds = 3;
constexpr uint32_t kListScanIntervalMs = 2000;
constexpr size_t kMaxPendingReportBytes = 20;

portMUX_TYPE pending_report_mux = portMUX_INITIALIZER_UNLOCKED;

bool enabled = false;
bool scanning = false;
bool scan_wanted = false;
bool targeted_connect_scan = false;
bool connecting = false;
bool connected = false;
bool last_connect_failed = false;
bool last_reported_connected = false;
bool auto_reconnect_pending = false;
bool connect_after_scan_pending = false;
bool pending_connection_ui = false;
bool pending_connecting_ui = false;
bool pending_report = false;
uint8_t pending_report_data[kMaxPendingReportBytes] = {};
size_t pending_report_len = 0;
uint32_t last_scan_start_ms = 0;
uint8_t auto_reconnect_attempts = 0;
uint32_t reconnect_after_ms = 0;

BLEClient* client = nullptr;
BLERemoteCharacteristic* input_report = nullptr;
uint8_t prev_report_state[8] = {};
uint8_t connected_address[6] = {};
uint8_t connected_addr_type = BLE_ADDR_TYPE_PUBLIC;
char connected_name[32] = "";

struct ConnectJob {
  uint8_t address[6];
  char name[32];
  uint8_t addr_type;
};

ConnectJob pending_connect_job = {};
BLEAdvertisedDevice* targeted_adv_copy = nullptr;

struct DiscoveredDevice {
  char name[32];
  uint8_t address[6];
  uint8_t addr_type;
  int rssi;
  bool valid;
};

DiscoveredDevice discovered_devices[kMaxDiscoveredDevices] = {};
size_t discovered_count = 0;

void logAddress(const char* label, const uint8_t address[6], uint8_t addr_type) {
  KB_LOG("%s %02x:%02x:%02x:%02x:%02x:%02x type=%u", label, address[0], address[1],
         address[2], address[3], address[4], address[5], addr_type);
}

bool addressesEqual(const uint8_t a[6], const uint8_t b[6]) {
  return memcmp(a, b, 6) == 0;
}

void copyAddress(uint8_t dest[6], const uint8_t src[6]) {
  memcpy(dest, src, 6);
}

void setConnectedName(const char* name) {
  if (name == nullptr || name[0] == '\0') {
    snprintf(connected_name, sizeof(connected_name), "Unknown keyboard");
    return;
  }
  strncpy(connected_name, name, sizeof(connected_name) - 1);
  connected_name[sizeof(connected_name) - 1] = '\0';
}

void notifyConnectionChanged() {
  if (connected == last_reported_connected) {
    return;
  }
  last_reported_connected = connected;
  pending_connection_ui = true;
}

void notifyConnectingChanged() { pending_connecting_ui = true; }

void processPendingBleKeyboardEvents() {
  if (pending_connection_ui) {
    pending_connection_ui = false;
    uiSetBleKeyboardConnected(connected);
  }
  if (pending_connecting_ui) {
    pending_connecting_ui = false;
    uiRefreshKeyboardConnectionStatus();
  }

  uint8_t report_copy[kMaxPendingReportBytes] = {};
  size_t report_len = 0;
  bool have_report = false;

  portENTER_CRITICAL(&pending_report_mux);
  if (pending_report) {
    report_len = pending_report_len;
    memcpy(report_copy, pending_report_data, report_len);
    pending_report = false;
    have_report = true;
  }
  portEXIT_CRITICAL(&pending_report_mux);

  if (have_report && connected) {
    keyboardInputProcessBootReport(KeyboardInputSource::kBle, prev_report_state,
                                   report_copy, report_len);
  }
}

void clearTargetedAdvCopy() {
  if (targeted_adv_copy != nullptr) {
    delete targeted_adv_copy;
    targeted_adv_copy = nullptr;
  }
}

void resetReportState() { memset(prev_report_state, 0, sizeof(prev_report_state)); }

void clearInputReport() {
  input_report = nullptr;
  resetReportState();
}

void resumeComputerAdvertising() {
  if (bleHidComputerIsEnabled() && !bleHidComputerIsConnected()) {
    bleHidComputerStartPairing();
  }
}

void pauseComputerAdvertising() {
  if (bleHidComputerIsAdvertising()) {
    bleHidComputerStopPairing();
  }
}

void releaseClient() {
  clearInputReport();
  if (client != nullptr) {
    if (client->isConnected()) {
      client->disconnect();
    }
    delete client;
    client = nullptr;
  }
}

void endConnectAttempt(bool success) {
  targeted_connect_scan = false;
  connecting = false;
  connect_after_scan_pending = false;
  clearTargetedAdvCopy();
  if (!success) {
    last_connect_failed = true;
    releaseClient();
    resumeComputerAdvertising();
  }
  notifyConnectingChanged();
}

void teardownClient() {
  releaseClient();
  connected = false;
  connecting = false;
  targeted_connect_scan = false;
  connect_after_scan_pending = false;
  clearTargetedAdvCopy();
  memset(connected_address, 0, sizeof(connected_address));
  connected_addr_type = BLE_ADDR_TYPE_PUBLIC;
  connected_name[0] = '\0';
  notifyConnectionChanged();
}

bool deviceLooksLikeKeyboard(BLEAdvertisedDevice& device) {
  const BLEUUID hid_uuid(static_cast<uint16_t>(kHidServiceUuid));
  if (device.isAdvertisingService(hid_uuid)) {
    return true;
  }
  for (int i = 0; i < device.getServiceUUIDCount(); ++i) {
    if (device.getServiceUUID(i).equals(hid_uuid)) {
      return true;
    }
  }
  if (device.getAppearance() == 0x03C1) {
    return true;
  }
  const std::string& name = device.getName();
  if (!name.empty()) {
    String lowered = String(name.c_str());
    lowered.toLowerCase();
    if (lowered.indexOf("keyboard") >= 0 || lowered.indexOf("keys") >= 0 ||
        lowered.indexOf("jlab") >= 0) {
      return true;
    }
  }
  return false;
}

bool isTargetDevice(BLEAdvertisedDevice& device) {
  uint8_t address[6] = {};
  memcpy(address, device.getAddress().getNative(), 6);
  return addressesEqual(address, pending_connect_job.address);
}

void captureTargetAdvertisement(BLEAdvertisedDevice& device) {
  if (!targeted_connect_scan || !isTargetDevice(device)) {
    return;
  }

  clearTargetedAdvCopy();
  targeted_adv_copy = new BLEAdvertisedDevice(device);
  pending_connect_job.addr_type = device.getAddressType();
  KB_LOG("saw target in scan name=\"%s\" type=%u rssi=%d", device.getName().c_str(),
         device.getAddressType(), device.getRSSI());
}

void upsertDiscoveredDevice(BLEAdvertisedDevice& device) {
  uint8_t address[6] = {};
  memcpy(address, device.getAddress().getNative(), 6);

  for (size_t i = 0; i < discovered_count; ++i) {
    if (discovered_devices[i].valid &&
        addressesEqual(discovered_devices[i].address, address)) {
      discovered_devices[i].rssi = device.getRSSI();
      discovered_devices[i].addr_type = device.getAddressType();
      if (!device.getName().empty()) {
        strncpy(discovered_devices[i].name, device.getName().c_str(),
                sizeof(discovered_devices[i].name) - 1);
        discovered_devices[i].name[sizeof(discovered_devices[i].name) - 1] = '\0';
      }
      return;
    }
  }

  if (discovered_count >= kMaxDiscoveredDevices) {
    return;
  }

  DiscoveredDevice& entry = discovered_devices[discovered_count++];
  entry.valid = true;
  copyAddress(entry.address, address);
  entry.addr_type = device.getAddressType();
  entry.rssi = device.getRSSI();
  if (device.getName().empty()) {
    snprintf(entry.name, sizeof(entry.name), "Keyboard %02X:%02X", address[4],
             address[5]);
  } else {
    strncpy(entry.name, device.getName().c_str(), sizeof(entry.name) - 1);
    entry.name[sizeof(entry.name) - 1] = '\0';
  }
}

void reportNotifyCallback(BLERemoteCharacteristic* /*characteristic*/, uint8_t* data,
                          size_t length, bool /*is_notify*/) {
  if (!connected || data == nullptr || length == 0) {
    return;
  }

  const size_t copy_len =
      length < kMaxPendingReportBytes ? length : kMaxPendingReportBytes;
  portENTER_CRITICAL(&pending_report_mux);
  memcpy(pending_report_data, data, copy_len);
  pending_report_len = copy_len;
  pending_report = true;
  portEXIT_CRITICAL(&pending_report_mux);
}

bool subscribeToInputReport(BLERemoteService* hid_service) {
  if (hid_service == nullptr) {
    KB_LOG("subscribe failed: no HID service");
    return false;
  }

  BLERemoteCharacteristic* protocol_mode =
      hid_service->getCharacteristic(BLEUUID(static_cast<uint16_t>(kProtocolModeUuid)));
  if (protocol_mode != nullptr && protocol_mode->canWrite()) {
    uint8_t boot_mode = 0;
    protocol_mode->writeValue(&boot_mode, 1, true);
    KB_LOG("set protocol mode to boot");
  }

  BLERemoteCharacteristic* boot_input = hid_service->getCharacteristic(
      BLEUUID(static_cast<uint16_t>(kBootKeyboardInputUuid)));
  if (boot_input != nullptr && boot_input->canNotify()) {
    input_report = boot_input;
    input_report->registerForNotify(reportNotifyCallback);
    KB_LOG("subscribed to boot keyboard input report");
    return true;
  }

  BLERemoteCharacteristic* report_char =
      hid_service->getCharacteristic(BLEUUID(static_cast<uint16_t>(kReportUuid)));
  if (report_char != nullptr && report_char->canNotify()) {
    input_report = report_char;
    input_report->registerForNotify(reportNotifyCallback);
    KB_LOG("subscribed to HID report characteristic");
    return true;
  }

  KB_LOG("subscribe failed: no notify characteristic");
  return false;
}

bool finishConnection(const ConnectJob& job) {
  if (client == nullptr || !client->isConnected()) {
    KB_LOG("finishConnection: client not connected");
    return false;
  }

  KB_LOG("discovering HID service");
  BLERemoteService* hid_service =
      client->getService(BLEUUID(static_cast<uint16_t>(kHidServiceUuid)));
  if (hid_service == nullptr) {
    KB_LOG("finishConnection: HID service not found");
    return false;
  }

  resetReportState();
  if (!subscribeToInputReport(hid_service)) {
    return false;
  }

  copyAddress(connected_address, job.address);
  connected_addr_type = job.addr_type;
  setConnectedName(job.name);
  connected = true;
  last_connect_failed = false;
  auto_reconnect_pending = false;
  auto_reconnect_attempts = 0;
  deviceSettingsSaveBleKeyboardDevice(job.address, connected_name, job.addr_type);
  notifyConnectionChanged();
  KB_LOG("connected to %s", connected_name);
  return true;
}

class KeyboardClientCallbacks : public BLEClientCallbacks {
 public:
  void onConnect(BLEClient* /*ble_client*/) override { KB_LOG("client onConnect"); }

  void onDisconnect(BLEClient* /*ble_client*/) override {
    KB_LOG("client onDisconnect");
    clearInputReport();
    connected = false;
    connecting = false;
    targeted_connect_scan = false;
    clearTargetedAdvCopy();
    memset(connected_address, 0, sizeof(connected_address));
    connected_addr_type = BLE_ADDR_TYPE_PUBLIC;
    connected_name[0] = '\0';
    notifyConnectionChanged();
    pending_connecting_ui = true;
    resumeComputerAdvertising();

    if (enabled && deviceSettingsHasBleKeyboardDevice()) {
      auto_reconnect_pending = true;
      auto_reconnect_attempts = 0;
      reconnect_after_ms = millis() + kReconnectDelayMs;
    }
  }
};

KeyboardClientCallbacks client_callbacks;

class KeyboardAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
 public:
  void onResult(BLEAdvertisedDevice advertised_device) override {
    if (!enabled) {
      return;
    }
    if (!scanning && !scan_wanted && !targeted_connect_scan) {
      return;
    }

    captureTargetAdvertisement(advertised_device);

    if (targeted_connect_scan && isTargetDevice(advertised_device)) {
      upsertDiscoveredDevice(advertised_device);
      return;
    }

    if (!deviceLooksLikeKeyboard(advertised_device)) {
      return;
    }
    upsertDiscoveredDevice(advertised_device);
  }
};

KeyboardAdvertisedDeviceCallbacks advertised_callbacks;

void ingestScanResults(BLEScanResults& results) {
  for (int i = 0; i < results.getCount(); ++i) {
    BLEAdvertisedDevice device = results.getDevice(i);
    if (deviceLooksLikeKeyboard(device)) {
      upsertDiscoveredDevice(device);
    }
  }
}

void onListScanComplete(BLEScanResults results) {
  scanning = false;
  ingestScanResults(results);
  KB_LOG("list scan complete, devices=%d list=%u", results.getCount(),
         discovered_count);
}

bool ensureClient() {
  if (client != nullptr) {
    return true;
  }
  if (BLEDevice::getScan() == nullptr) {
    KB_LOG("ensureClient: BLE scan unavailable");
    return false;
  }
  client = BLEDevice::createClient();
  if (client == nullptr) {
    KB_LOG("ensureClient: createClient failed");
    return false;
  }
  client->setClientCallbacks(&client_callbacks);
  return true;
}

bool tryAddressTypes(const ConnectJob& job) {
  uint8_t address_copy[6];
  memcpy(address_copy, job.address, sizeof(address_copy));
  const BLEAddress ble_address(address_copy);

  const esp_ble_addr_type_t types[] = {static_cast<esp_ble_addr_type_t>(job.addr_type),
                                       BLE_ADDR_TYPE_RANDOM, BLE_ADDR_TYPE_PUBLIC};

  for (esp_ble_addr_type_t addr_type : types) {
    KB_LOG("trying address connect type=%u", addr_type);
    if (client->connect(ble_address, addr_type) && client->isConnected()) {
      pending_connect_job.addr_type = static_cast<uint8_t>(addr_type);
      return true;
    }
    if (client != nullptr && client->isConnected()) {
      client->disconnect();
    }
  }
  return false;
}

bool tryLinkClient(const ConnectJob& job) {
  if (targeted_adv_copy != nullptr) {
    KB_LOG("connecting via captured advertisement type=%u",
           targeted_adv_copy->getAddressType());
    auto* heap_device = new BLEAdvertisedDevice(*targeted_adv_copy);
    const bool linked = client->connect(heap_device);
    delete heap_device;
    if (linked && client->isConnected()) {
      pending_connect_job.addr_type = targeted_adv_copy->getAddressType();
      return true;
    }
    KB_LOG("advertisement connect failed");
    if (client != nullptr && client->isConnected()) {
      client->disconnect();
    }
  }

  BLEScan* scan = BLEDevice::getScan();
  if (scan != nullptr) {
    BLEScanResults results = scan->getResults();
    KB_LOG("scan cache has %d device(s)", results.getCount());
    for (int i = 0; i < results.getCount(); ++i) {
      BLEAdvertisedDevice candidate = results.getDevice(i);
      uint8_t candidate_addr[6] = {};
      memcpy(candidate_addr, candidate.getAddress().getNative(), 6);
      if (!addressesEqual(candidate_addr, job.address)) {
        continue;
      }

      KB_LOG("connecting via scan result name=\"%s\" type=%u",
             candidate.getName().c_str(), candidate.getAddressType());
      auto* heap_device = new BLEAdvertisedDevice(candidate);
      const bool linked = client->connect(heap_device);
      delete heap_device;
      if (linked && client->isConnected()) {
        pending_connect_job.addr_type = candidate.getAddressType();
        return true;
      }
      KB_LOG("scan result connect failed");
      if (client != nullptr && client->isConnected()) {
        client->disconnect();
      }
    }
  }

  return tryAddressTypes(job);
}

bool performConnect(const ConnectJob& job) {
  if (!enabled) {
    return false;
  }

  logAddress("connect target", job.address, job.addr_type);
  KB_LOG("performConnect name=\"%s\"", job.name);

  releaseClient();
  if (!ensureClient()) {
    return false;
  }

  if (!tryLinkClient(job)) {
    KB_LOG("link failed");
    releaseClient();
    resumeComputerAdvertising();
    return false;
  }

  KB_LOG("link established, setting up HID");
  if (!finishConnection(job)) {
    KB_LOG("HID setup failed");
    releaseClient();
    resumeComputerAdvertising();
    return false;
  }

  resumeComputerAdvertising();
  return true;
}

void findTargetInScanResults(BLEScanResults& results) {
  if (targeted_adv_copy != nullptr) {
    return;
  }

  for (int i = 0; i < results.getCount(); ++i) {
    BLEAdvertisedDevice candidate = results.getDevice(i);
    if (!isTargetDevice(candidate)) {
      continue;
    }
    targeted_adv_copy = new BLEAdvertisedDevice(candidate);
    pending_connect_job.addr_type = candidate.getAddressType();
    KB_LOG("found target in scan results type=%u", candidate.getAddressType());
    return;
  }
}

void onTargetedScanComplete(BLEScanResults results) {
  scanning = false;
  targeted_connect_scan = false;
  KB_LOG("connect scan complete, cache=%d", results.getCount());

  findTargetInScanResults(results);
  connect_after_scan_pending = true;
}

void clearDiscoveredDevices() {
  discovered_count = 0;
  memset(discovered_devices, 0, sizeof(discovered_devices));
}

bool startTargetedConnectScan() {
  BLEScan* scan = BLEDevice::getScan();
  if (scan == nullptr) {
    KB_LOG("targeted scan unavailable");
    return false;
  }

  scan_wanted = false;
  scan->setAdvertisedDeviceCallbacks(&advertised_callbacks, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);
  targeted_connect_scan = true;
  scanning = true;
  KB_LOG("starting connect scan for \"%s\"", pending_connect_job.name);
  return scan->start(kConnectScanSeconds, onTargetedScanComplete, false);
}

void queueAutoReconnect() {
  uint8_t saved_address[6] = {};
  if (!deviceSettingsGetBleKeyboardAddress(saved_address)) {
    return;
  }
  auto_reconnect_pending = true;
  auto_reconnect_attempts = 0;
  reconnect_after_ms = millis() + 500;
}

bool beginConnect(const uint8_t address[6], const char* name, uint8_t addr_type) {
  if (!enabled || address == nullptr) {
    KB_LOG("beginConnect rejected: not enabled or null address");
    return false;
  }
  if (connecting || targeted_connect_scan) {
    KB_LOG("beginConnect rejected: already connecting");
    return false;
  }

  pending_connect_job = {};
  memcpy(pending_connect_job.address, address, 6);
  pending_connect_job.addr_type = addr_type;
  if (name != nullptr) {
    strncpy(pending_connect_job.name, name, sizeof(pending_connect_job.name) - 1);
    pending_connect_job.name[sizeof(pending_connect_job.name) - 1] = '\0';
  } else {
    snprintf(pending_connect_job.name, sizeof(pending_connect_job.name),
             "Keyboard %02X:%02X", address[4], address[5]);
  }

  last_connect_failed = false;
  connecting = true;
  clearTargetedAdvCopy();
  bleKeyboardStopScan();
  pauseComputerAdvertising();
  notifyConnectingChanged();
  KB_LOG("begin connect to \"%s\"", pending_connect_job.name);

  if (!startTargetedConnectScan()) {
    endConnectAttempt(false);
    return false;
  }
  return true;
}

}  // namespace

void bleKeyboardBegin() {
  // Auto-reconnect only runs after an unexpected disconnect, not on boot.
}

void bleKeyboardTick() {
  processPendingBleKeyboardEvents();

  if (!enabled) {
    return;
  }

  if (scan_wanted && !scanning && !connected && !targeted_connect_scan) {
    BLEScan* scan = BLEDevice::getScan();
    if (scan != nullptr && millis() - last_scan_start_ms >= kListScanIntervalMs) {
      last_scan_start_ms = millis();
      scan->setAdvertisedDeviceCallbacks(&advertised_callbacks, true);
      scan->setActiveScan(true);
      scan->setInterval(100);
      scan->setWindow(99);
      scanning = true;
      if (!scan->start(1, onListScanComplete, true)) {
        scanning = false;
        KB_LOG("list scan start failed");
      } else {
        KB_LOG("list scan started");
      }
    }
  }

  if (connect_after_scan_pending && connecting && !scanning) {
    connect_after_scan_pending = false;
    const bool ok = performConnect(pending_connect_job);
    endConnectAttempt(ok);
    if (!ok) {
      KB_LOG("connect failed");
    }
  }

  if (auto_reconnect_pending && !connected && !connecting && !targeted_connect_scan &&
      !scanning && millis() >= reconnect_after_ms) {
    uint8_t saved_address[6] = {};
    char saved_name[32] = {};
    if (deviceSettingsGetBleKeyboardAddress(saved_address)) {
      deviceSettingsGetBleKeyboardName(saved_name, sizeof(saved_name));
      const uint8_t addr_type = deviceSettingsGetBleKeyboardAddressType();
      KB_LOG("auto-reconnect attempt %u", auto_reconnect_attempts + 1);
      if (beginConnect(saved_address, saved_name, addr_type)) {
        auto_reconnect_pending = false;
      } else {
        auto_reconnect_attempts++;
        if (auto_reconnect_attempts >= kAutoReconnectMaxAttempts) {
          auto_reconnect_pending = false;
          KB_LOG("auto-reconnect gave up");
        } else {
          reconnect_after_ms = millis() + kReconnectDelayMs;
        }
      }
    } else {
      auto_reconnect_pending = false;
    }
  }
}

void bleKeyboardSetEnabled(bool value) {
  if (enabled == value) {
    return;
  }

  enabled = value;

  if (!enabled) {
    bleKeyboardStopScan();
    bleKeyboardDisconnect();
    auto_reconnect_pending = false;
    connect_after_scan_pending = false;
    return;
  }
}

bool bleKeyboardIsEnabled() { return enabled; }

bool bleKeyboardIsConnected() { return connected; }

bool bleKeyboardLastConnectFailed() { return last_connect_failed; }

void bleKeyboardStartScan() {
  if (!enabled || connected || targeted_connect_scan) {
    return;
  }
  if (BLEDevice::getScan() == nullptr) {
    return;
  }

  clearDiscoveredDevices();
  scan_wanted = true;
  last_scan_start_ms = millis() - kListScanIntervalMs;
}

void bleKeyboardStopScan() {
  scan_wanted = false;

  if (!scanning) {
    return;
  }

  BLEScan* scan = BLEDevice::getScan();
  if (scan != nullptr) {
    scan->stop();
  }
  scanning = false;
  targeted_connect_scan = false;
}

bool bleKeyboardIsScanning() {
  return scanning || scan_wanted || targeted_connect_scan;
}

bool bleKeyboardIsConnecting() { return connecting || targeted_connect_scan; }

size_t bleKeyboardGetDeviceCount() { return discovered_count; }

bool bleKeyboardGetDevice(size_t index, BleKeyboardDevice* out) {
  if (out == nullptr || index >= discovered_count ||
      !discovered_devices[index].valid) {
    return false;
  }

  strncpy(out->name, discovered_devices[index].name, sizeof(out->name) - 1);
  out->name[sizeof(out->name) - 1] = '\0';
  copyAddress(out->address, discovered_devices[index].address);
  out->addr_type = discovered_devices[index].addr_type;
  out->rssi = discovered_devices[index].rssi;
  return true;
}

void bleKeyboardConnect(const uint8_t address[6]) {
  if (!enabled || address == nullptr) {
    return;
  }

  const char* name_hint = nullptr;
  char fallback_name[32];
  uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;
  for (size_t i = 0; i < discovered_count; ++i) {
    if (discovered_devices[i].valid &&
        addressesEqual(discovered_devices[i].address, address)) {
      name_hint = discovered_devices[i].name;
      addr_type = discovered_devices[i].addr_type;
      break;
    }
  }
  if (name_hint == nullptr) {
    snprintf(fallback_name, sizeof(fallback_name), "Keyboard %02X:%02X", address[4],
             address[5]);
    name_hint = fallback_name;
    addr_type = deviceSettingsGetBleKeyboardAddressType();
  }

  deviceSettingsSaveBleKeyboardDevice(address, name_hint, addr_type);
  beginConnect(address, name_hint, addr_type);
}

void bleKeyboardDisconnect() {
  auto_reconnect_pending = false;
  teardownClient();
  resumeComputerAdvertising();
}

bool bleKeyboardGetConnectedDevice(BleKeyboardDevice* out) {
  if (out == nullptr || !connected) {
    return false;
  }

  strncpy(out->name, connected_name, sizeof(out->name) - 1);
  out->name[sizeof(out->name) - 1] = '\0';
  copyAddress(out->address, connected_address);
  out->addr_type = connected_addr_type;
  out->rssi = 0;
  return true;
}

void bleKeyboardSetSavedDevice(const uint8_t address[6], const char* name,
                               uint8_t addr_type) {
  if (address == nullptr) {
    return;
  }
  deviceSettingsSaveBleKeyboardDevice(address, name, addr_type);
}

bool bleKeyboardAddressesEqual(const uint8_t a[6], const uint8_t b[6]) {
  return addressesEqual(a, b);
}
