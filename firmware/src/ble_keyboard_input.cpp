#include "ble_keyboard_input.h"

#include "ble_radio_policy.h"
#include "ble_stack.h"
#include "keyboard_input.h"
#include "usb_keyboard.h"

#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEUUID.h>

#include <Preferences.h>

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr char kPrefsNamespace[] = "echolocation";
constexpr char kKeyBondCount[] = "kb_bond_n";
constexpr char kKeyBondPrefix[] = "kb_bond_";
constexpr char kKeyLastAddress[] = "kb_last";
constexpr size_t kMaxBondedKeyboards = 8;
constexpr size_t kMaxScanResults = 8;
constexpr uint16_t kAppearanceGenericHid = 0x03C0;
constexpr uint16_t kAppearanceKeyboard = 0x03C1;
constexpr uint32_t kScanDurationSec = 0;
constexpr uint32_t kConnectTimeoutMs = 5000;
constexpr uint32_t kReconnectIntervalMs = 10000;

struct ScanEntry {
  char name[32];
  char address[18];
  uint8_t address_type;
};

struct BondedKeyboard {
  char name[32];
  char address[18];
  uint8_t address_type;
};

NimBLEClient* ble_client = nullptr;
bool module_ready = false;
bool input_enabled = false;
bool scan_requested = false;
bool scanning = false;
bool connecting = false;
bool keyboard_connected = false;
uint32_t connect_started_ms = 0;
uint32_t last_reconnect_attempt_ms = 0;
char connected_name[32] = {};
char connected_address[18] = {};
char selected_address[18] = {};
uint8_t prev_report_state[8] = {};
ScanEntry scan_results[kMaxScanResults];
size_t scan_result_count = 0;
char bonded_addresses[kMaxBondedKeyboards][18];
char bonded_names[kMaxBondedKeyboards][32];
uint8_t bonded_address_types[kMaxBondedKeyboards];
size_t bonded_count = 0;
bool reconnect_scan_active = false;

struct PendingConnect {
  char address[18];
  char name[32];
  uint8_t address_type;
  uint8_t attempt;
  bool connect_in_flight;
};

PendingConnect pending_connect = {};
bool pending_advertised_connect = false;
char pending_advertised_address[18] = {};
char pending_advertised_name[32] = {};
uint8_t pending_advertised_address_type = BLE_ADDR_PUBLIC;

bool ensureClient();
bool finishConnection(const char* name, const char* address, uint8_t address_type);
bool startScanInternal();
void stopScanInternal();
void beginConnectAttempt(const char* address, const char* name,
                         uint8_t address_type);

void formatAddress(const NimBLEAddress& addr, char* out, size_t out_len) {
  snprintf(out, out_len, "%s", addr.toString().c_str());
}

void copyName(const char* source, char* dest, size_t dest_len) {
  if (source == nullptr || source[0] == '\0') {
    snprintf(dest, dest_len, "Unknown keyboard");
    return;
  }
  strncpy(dest, source, dest_len - 1);
  dest[dest_len - 1] = '\0';
}

NimBLEAddress addressFromStored(const char* address, uint8_t address_type) {
  return NimBLEAddress(std::string(address), address_type);
}

bool nameLooksLikeKeyboard(const char* name) {
  if (name == nullptr || name[0] == '\0') {
    return false;
  }

  char lowered[32];
  strncpy(lowered, name, sizeof(lowered) - 1);
  lowered[sizeof(lowered) - 1] = '\0';
  for (char* c = lowered; *c != '\0'; ++c) {
    if (*c >= 'A' && *c <= 'Z') {
      *c = static_cast<char>(*c - 'A' + 'a');
    }
  }
  return strstr(lowered, "keyboard") != nullptr;
}

bool isDiscoverableKeyboard(const NimBLEAdvertisedDevice* device) {
  if (device == nullptr) {
    return false;
  }

  if (device->isAdvertisingService(NimBLEUUID((uint16_t)0x1812))) {
    return true;
  }

  if (device->haveAppearance()) {
    const uint16_t appearance = device->getAppearance();
    if (appearance == kAppearanceGenericHid || appearance == kAppearanceKeyboard) {
      return true;
    }
  }

  return nameLooksLikeKeyboard(device->getName().c_str());
}

int findBondIndex(const char* address) {
  if (address == nullptr) {
    return -1;
  }
  for (size_t i = 0; i < bonded_count; ++i) {
    if (strcmp(bonded_addresses[i], address) == 0) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

uint8_t lookupAddressType(const char* address) {
  if (address == nullptr) {
    return BLE_ADDR_PUBLIC;
  }

  for (size_t i = 0; i < scan_result_count; ++i) {
    if (strcmp(scan_results[i].address, address) == 0) {
      return scan_results[i].address_type;
    }
  }

  const int bond_index = findBondIndex(address);
  if (bond_index >= 0) {
    return bonded_address_types[bond_index];
  }

  return BLE_ADDR_PUBLIC;
}

bool namesMatch(const char* left, const char* right) {
  if (left == nullptr || right == nullptr) {
    return false;
  }
  while (*left != '\0' && *right != '\0') {
    char a = *left;
    char b = *right;
    if (a >= 'A' && a <= 'Z') {
      a = static_cast<char>(a - 'A' + 'a');
    }
    if (b >= 'A' && b <= 'Z') {
      b = static_cast<char>(b - 'A' + 'a');
    }
    if (a != b) {
      return false;
    }
    ++left;
    ++right;
  }
  return *left == '\0' && *right == '\0';
}

bool matchesBondedKeyboard(const NimBLEAdvertisedDevice* device) {
  if (device == nullptr || !isDiscoverableKeyboard(device)) {
    return false;
  }

  char address[18];
  formatAddress(device->getAddress(), address, sizeof(address));
  if (findBondIndex(address) >= 0) {
    return true;
  }

  const char* name = device->getName().c_str();
  for (size_t i = 0; i < bonded_count; ++i) {
    if (namesMatch(name, bonded_names[i])) {
      return true;
    }
  }
  return false;
}

void resetPendingConnect() { memset(&pending_connect, 0, sizeof(pending_connect)); }

void cancelConnectAttempt() {
  if (ble_client != nullptr && ble_client->isConnected()) {
    ble_client->disconnect();
  }
  connecting = false;
  resetPendingConnect();
  connected_name[0] = '\0';
  connected_address[0] = '\0';
}

void startReconnectScan() {
  if (!module_ready || !input_enabled || keyboard_connected || connecting ||
      scan_requested) {
    return;
  }

  reconnect_scan_active = true;
  if (!startScanInternal()) {
    reconnect_scan_active = false;
  } else {
    Serial.println("[ble-kb] scanning for bonded keyboard");
  }
}

void queueConnectFromAdvertised(const NimBLEAdvertisedDevice* device) {
  if (device == nullptr) {
    return;
  }

  formatAddress(device->getAddress(), pending_advertised_address,
                sizeof(pending_advertised_address));
  copyName(device->getName().c_str(), pending_advertised_name,
           sizeof(pending_advertised_name));
  pending_advertised_address_type = device->getAddress().getType();
  pending_advertised_connect = true;
  reconnect_scan_active = false;
  stopScanInternal();
}

void processPendingAdvertisedConnect() {
  if (!pending_advertised_connect || connecting || keyboard_connected) {
    return;
  }

  pending_advertised_connect = false;
  Serial.printf("[ble-kb] reconnect found: %s (%s)\n", pending_advertised_name,
                pending_advertised_address);
  beginConnectAttempt(pending_advertised_address, pending_advertised_name,
                      pending_advertised_address_type);
}

void beginConnectAttempt(const char* address, const char* name,
                         uint8_t address_type) {
  stopScanInternal();
  reconnect_scan_active = false;

  if (!ensureClient()) {
    return;
  }

  if (ble_client->isConnected()) {
    ble_client->disconnect();
    delay(50);
  }

  resetPendingConnect();
  strncpy(pending_connect.address, address, sizeof(pending_connect.address) - 1);
  pending_connect.address[sizeof(pending_connect.address) - 1] = '\0';
  copyName(name, pending_connect.name, sizeof(pending_connect.name));
  pending_connect.address_type = address_type;
  pending_connect.attempt = 0;
  pending_connect.connect_in_flight = false;

  copyName(name, connected_name, sizeof(connected_name));
  strncpy(connected_address, address, sizeof(connected_address) - 1);
  connected_address[sizeof(connected_address) - 1] = '\0';
  connecting = true;
  connect_started_ms = millis();
}

void processConnectTick() {
  if (!connecting || ble_client == nullptr || pending_connect.address[0] == '\0') {
    return;
  }

  if (ble_client->isConnected()) {
    if (!keyboard_connected &&
        !finishConnection(connected_name, connected_address,
                          pending_connect.address_type)) {
      cancelConnectAttempt();
      startReconnectScan();
    }
    resetPendingConnect();
    return;
  }

  if (pending_connect.connect_in_flight) {
    return;
  }

  pending_connect.connect_in_flight = true;
  connect_started_ms = millis();
  NimBLEAddress peer =
      addressFromStored(pending_connect.address, pending_connect.address_type);
  Serial.printf("[ble-kb] connecting to %s (%s, type=%u)\n",
                pending_connect.name, pending_connect.address,
                static_cast<unsigned>(pending_connect.address_type));

  const bool connected = ble_client->connect(peer);
  pending_connect.connect_in_flight = false;

  if (connected && ble_client->isConnected()) {
    if (!finishConnection(pending_connect.name, pending_connect.address,
                          pending_connect.address_type)) {
      cancelConnectAttempt();
      startReconnectScan();
    }
    resetPendingConnect();
    return;
  }

  if (pending_connect.attempt == 0) {
    pending_connect.attempt = 1;
    pending_connect.address_type = pending_connect.address_type == BLE_ADDR_PUBLIC
                                       ? BLE_ADDR_RANDOM
                                       : BLE_ADDR_PUBLIC;
    Serial.printf("[ble-kb] retrying connect with address type %u\n",
                  static_cast<unsigned>(pending_connect.address_type));
    return;
  }

  Serial.println("[ble-kb] connect failed");
  cancelConnectAttempt();
  startReconnectScan();
}

void loadBondedKeyboards() {
  bonded_count = 0;
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  const size_t count = prefs.getUInt(kKeyBondCount, 0);
  for (size_t i = 0; i < count && i < kMaxBondedKeyboards; ++i) {
    char key[16];
    snprintf(key, sizeof(key), "%s%u", kKeyBondPrefix, static_cast<unsigned>(i));
    String value = prefs.getString(key, "");
    if (value.length() == 0) {
      continue;
    }
    const int sep = value.indexOf('|');
    if (sep <= 0) {
      continue;
    }
    value.substring(0, sep).toCharArray(bonded_addresses[bonded_count],
                                        sizeof(bonded_addresses[0]));
    const int name_sep = value.indexOf('|', sep + 1);
    if (name_sep > sep) {
      value.substring(sep + 1, name_sep)
          .toCharArray(bonded_names[bonded_count], sizeof(bonded_names[0]));
      bonded_address_types[bonded_count] =
          static_cast<uint8_t>(value.substring(name_sep + 1).toInt());
    } else {
      value.substring(sep + 1).toCharArray(bonded_names[bonded_count],
                                           sizeof(bonded_names[0]));
      bonded_address_types[bonded_count] = BLE_ADDR_PUBLIC;
    }
    bonded_count++;
  }
  prefs.end();
}

void saveBondedKeyboards() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putUInt(kKeyBondCount, bonded_count);
  for (size_t i = 0; i < kMaxBondedKeyboards; ++i) {
    char key[16];
    snprintf(key, sizeof(key), "%s%u", kKeyBondPrefix, static_cast<unsigned>(i));
    if (i < bonded_count) {
      String value = String(bonded_addresses[i]) + "|" + bonded_names[i] + "|" +
                     String(bonded_address_types[i]);
      prefs.putString(key, value);
    } else {
      prefs.remove(key);
    }
  }
  prefs.end();
}

void saveLastAddress(const char* address) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  if (address == nullptr || address[0] == '\0') {
    prefs.remove(kKeyLastAddress);
  } else {
    prefs.putString(kKeyLastAddress, address);
  }
  prefs.end();
}

void loadLastAddress(char* out, size_t out_len) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  String value = prefs.getString(kKeyLastAddress, "");
  prefs.end();
  if (value.length() == 0) {
    out[0] = '\0';
    return;
  }
  value.toCharArray(out, out_len);
}

void rememberBondedKeyboard(const char* address, const char* name,
                            uint8_t address_type) {
  if (address == nullptr || address[0] == '\0') {
    return;
  }

  const int existing = findBondIndex(address);
  if (existing >= 0) {
    copyName(name, bonded_names[existing], sizeof(bonded_names[0]));
    bonded_address_types[existing] = address_type;
    saveBondedKeyboards();
    return;
  }

  if (bonded_count >= kMaxBondedKeyboards) {
    bonded_count = kMaxBondedKeyboards - 1;
  }

  strncpy(bonded_addresses[bonded_count], address,
          sizeof(bonded_addresses[0]) - 1);
  bonded_addresses[bonded_count][sizeof(bonded_addresses[0]) - 1] = '\0';
  copyName(name, bonded_names[bonded_count], sizeof(bonded_names[0]));
  bonded_address_types[bonded_count] = address_type;
  bonded_count++;
  saveBondedKeyboards();
}

void removeBondedKeyboard(const char* address) {
  const int index = findBondIndex(address);
  if (index < 0) {
    return;
  }

  for (size_t i = static_cast<size_t>(index) + 1; i < bonded_count; ++i) {
    strncpy(bonded_addresses[i - 1], bonded_addresses[i],
            sizeof(bonded_addresses[0]) - 1);
    strncpy(bonded_names[i - 1], bonded_names[i], sizeof(bonded_names[0]) - 1);
    bonded_address_types[i - 1] = bonded_address_types[i];
  }
  bonded_count--;
  saveBondedKeyboards();
}

void clearConnectedState() {
  keyboard_connected = false;
  connected_name[0] = '\0';
  connected_address[0] = '\0';
  memset(prev_report_state, 0, sizeof(prev_report_state));
}

void updateScanResultsFromDevice(const NimBLEAdvertisedDevice* device) {
  if (device == nullptr) {
    return;
  }

  char address[18];
  formatAddress(device->getAddress(), address, sizeof(address));

  int existing = -1;
  for (size_t i = 0; i < scan_result_count; ++i) {
    if (strcmp(scan_results[i].address, address) == 0) {
      existing = static_cast<int>(i);
      break;
    }
  }

  ScanEntry* entry = nullptr;
  if (existing >= 0) {
    entry = &scan_results[existing];
  } else if (scan_result_count < kMaxScanResults) {
    entry = &scan_results[scan_result_count++];
  } else {
    return;
  }

  formatAddress(device->getAddress(), entry->address, sizeof(entry->address));
  entry->address_type = device->getAddress().getType();
  copyName(device->getName().c_str(), entry->name, sizeof(entry->name));
}

bool addScanResultFromDevice(const NimBLEAdvertisedDevice* device) {
  if (device == nullptr) {
    return false;
  }

  char address[18];
  formatAddress(device->getAddress(), address, sizeof(address));
  for (size_t i = 0; i < scan_result_count; ++i) {
    if (strcmp(scan_results[i].address, address) == 0) {
      updateScanResultsFromDevice(device);
      return false;
    }
  }

  if (scan_result_count >= kMaxScanResults) {
    return false;
  }

  updateScanResultsFromDevice(device);
  return true;
}

class KeyboardScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertised_device) override {
    if (advertised_device == nullptr) {
      return;
    }

    if (reconnect_scan_active && matchesBondedKeyboard(advertised_device)) {
      queueConnectFromAdvertised(advertised_device);
      return;
    }

    if (!scan_requested) {
      return;
    }

    if (!isDiscoverableKeyboard(advertised_device)) {
      return;
    }

    if (addScanResultFromDevice(advertised_device)) {
      Serial.printf("[ble-kb] found: %s (%s)\n",
                    advertised_device->getName().c_str(),
                    advertised_device->getAddress().toString().c_str());
    }
  }
};

KeyboardScanCallbacks scan_callbacks;

class KeyboardClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* client) override {
    (void)client;
    Serial.println("[ble-kb] connected");
  }

  void onDisconnect(NimBLEClient* client, int reason) override {
    (void)client;
    (void)reason;
    Serial.println("[ble-kb] disconnected");
    clearConnectedState();
    connecting = false;
  }
};

KeyboardClientCallbacks client_callbacks;

void processHidReport(NimBLERemoteCharacteristic* characteristic,
                        uint8_t* data, size_t length, bool is_notify) {
  (void)characteristic;
  (void)is_notify;
  if (!keyboard_connected || usbKeyboardIsConnected() || data == nullptr) {
    return;
  }

  uint8_t report[8] = {};
  if (length == 8) {
    memcpy(report, data, 8);
  } else if (length >= 9 && data[0] == 1) {
    memcpy(report, data + 1, 8);
  } else if (length > 8) {
    memcpy(report, data + (length - 8), 8);
  } else {
    return;
  }

  keyboardInputProcessBootReport(prev_report_state, report, sizeof(report));
}

bool subscribeToKeyboardReports() {
  if (ble_client == nullptr || !ble_client->isConnected()) {
    return false;
  }

  NimBLERemoteService* hid_service =
      ble_client->getService(NimBLEUUID((uint16_t)0x1812));
  if (hid_service == nullptr) {
    Serial.println("[ble-kb] HID service missing");
    return false;
  }

  const std::vector<NimBLERemoteCharacteristic*>& characteristics =
      hid_service->getCharacteristics(true);
  for (NimBLERemoteCharacteristic* characteristic : characteristics) {
    if (characteristic == nullptr || !characteristic->canNotify()) {
      continue;
    }

    const NimBLEUUID uuid = characteristic->getUUID();
    if (uuid.equals(NimBLEUUID((uint16_t)0x2A4D)) ||
        uuid.equals(NimBLEUUID((uint16_t)0x2A22))) {
      if (!characteristic->subscribe(true, processHidReport)) {
        continue;
      }
      Serial.println("[ble-kb] subscribed to HID reports");
      return true;
    }
  }

  for (NimBLERemoteCharacteristic* characteristic : characteristics) {
    if (characteristic != nullptr && characteristic->canNotify() &&
        characteristic->subscribe(true, processHidReport)) {
      Serial.println("[ble-kb] subscribed to fallback notify characteristic");
      return true;
    }
  }

  Serial.println("[ble-kb] no notify characteristic found");
  return false;
}

bool finishConnection(const char* name, const char* address,
                      uint8_t address_type) {
  if (ble_client == nullptr || !ble_client->isConnected()) {
    return false;
  }

  if (!ble_client->secureConnection()) {
    Serial.println("[ble-kb] secure connection failed");
  }

  if (!ble_client->discoverAttributes()) {
    Serial.println("[ble-kb] attribute discovery failed");
    ble_client->disconnect();
    return false;
  }

  if (!subscribeToKeyboardReports()) {
    ble_client->disconnect();
    return false;
  }

  keyboard_connected = true;
  connecting = false;
  copyName(name, connected_name, sizeof(connected_name));
  strncpy(connected_address, address, sizeof(connected_address) - 1);
  connected_address[sizeof(connected_address) - 1] = '\0';
  rememberBondedKeyboard(address, connected_name, address_type);
  saveLastAddress(address);
  memset(prev_report_state, 0, sizeof(prev_report_state));
  Serial.printf("[ble-kb] ready: %s (%s)\n", connected_name, connected_address);
  return true;
}

bool ensureClient() {
  if (ble_client != nullptr) {
    return true;
  }

  ble_client = NimBLEDevice::createClient();
  if (ble_client == nullptr) {
    return false;
  }

  ble_client->setClientCallbacks(&client_callbacks, false);
  ble_client->setConnectTimeout(kConnectTimeoutMs);
  return true;
}

void disconnectClient() {
  if (ble_client != nullptr && ble_client->isConnected()) {
    ble_client->disconnect();
  }
  clearConnectedState();
  cancelConnectAttempt();
}

bool startScanInternal() {
  if (!module_ready || !input_enabled || keyboard_connected || connecting) {
    return false;
  }

  if (!scan_requested && !reconnect_scan_active) {
    return false;
  }

  NimBLEScan* scan = NimBLEDevice::getScan();
  if (scan == nullptr) {
    return false;
  }

  bleRadioPolicySetScanActive(true);
  if (scan->isScanning()) {
    scanning = true;
    return true;
  }

  if (scan_requested) {
    scan_result_count = 0;
  }
  scan->setScanCallbacks(&scan_callbacks, false);
  scan->setActiveScan(true);
  scan->setInterval(45);
  scan->setWindow(30);
  scan->setDuplicateFilter(false);

  scanning = true;
  const bool started = scan->start(kScanDurationSec, false, true);
  if (!started) {
    scanning = false;
    bleRadioPolicySetScanActive(false);
    return false;
  }

  Serial.println("[ble-kb] scan started");
  return true;
}

void stopScanInternal() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  if (scan != nullptr && scan->isScanning()) {
    scan->stop();
  }
  scanning = false;
  bleRadioPolicySetScanActive(false);
}

bool connectToAddress(const char* address, const char* name,
                      uint8_t address_type) {
  if (!module_ready || !input_enabled || address == nullptr ||
      address[0] == '\0' || keyboard_connected) {
    return false;
  }

  beginConnectAttempt(address, name, address_type);
  return connecting;
}

void attemptAutoReconnect() {
  if (!input_enabled || keyboard_connected || connecting || scan_requested) {
    return;
  }

  if (scanning || reconnect_scan_active) {
    return;
  }

  const uint32_t now_ms = millis();
  if (now_ms - last_reconnect_attempt_ms < kReconnectIntervalMs) {
    return;
  }
  last_reconnect_attempt_ms = now_ms;

  if (bonded_count == 0) {
    return;
  }

  char last_address[18];
  loadLastAddress(last_address, sizeof(last_address));
  if (last_address[0] == '\0') {
    strncpy(last_address, bonded_addresses[0], sizeof(last_address) - 1);
    last_address[sizeof(last_address) - 1] = '\0';
  }

  const int bond_index = findBondIndex(last_address);
  const char* name = bond_index >= 0 ? bonded_names[bond_index] : "Keyboard";
  const uint8_t address_type =
      bond_index >= 0 ? bonded_address_types[bond_index]
                      : lookupAddressType(last_address);
  beginConnectAttempt(last_address, name, address_type);
}

}  // namespace

void bleKeyboardInputBegin() {
  if (module_ready) {
    return;
  }

  if (!bleStackEnsureInit("echolocation")) {
    return;
  }

  loadBondedKeyboards();
  module_ready = true;
  Serial.println("[ble-kb] module ready");
}

void bleKeyboardInputSetEnabled(bool enabled) {
  input_enabled = enabled;
  if (!module_ready) {
    return;
  }

  if (!enabled) {
    bleKeyboardInputStopScan();
    reconnect_scan_active = false;
    cancelConnectAttempt();
    disconnectClient();
    return;
  }

  last_reconnect_attempt_ms = millis();
}

void bleKeyboardInputTick() {
  if (!module_ready || !input_enabled) {
    return;
  }

  if (connecting) {
    processConnectTick();
  }

  processPendingAdvertisedConnect();

  if (scan_requested && !keyboard_connected && !connecting) {
    startScanInternal();
  } else if (reconnect_scan_active && !keyboard_connected && !connecting) {
    startScanInternal();
  } else if (!scan_requested && !reconnect_scan_active && scanning) {
    stopScanInternal();
  }

  if (!keyboard_connected && !connecting && !scanning && !scan_requested &&
      !reconnect_scan_active) {
    attemptAutoReconnect();
  }
}

void bleKeyboardInputStartScan() {
  if (!module_ready || !input_enabled || keyboard_connected) {
    return;
  }
  scan_requested = true;
  bleKeyboardInputTick();
}

void bleKeyboardInputStopScan() {
  scan_requested = false;
  reconnect_scan_active = false;
  stopScanInternal();
  scan_result_count = 0;
}

bool bleKeyboardInputConnectByAddress(const char* address) {
  if (address == nullptr) {
    return false;
  }

  const char* name = "Keyboard";
  for (size_t i = 0; i < scan_result_count; ++i) {
    if (strcmp(scan_results[i].address, address) == 0) {
      name = scan_results[i].name;
      break;
    }
  }

  const int bond_index = findBondIndex(address);
  if (bond_index >= 0) {
    name = bonded_names[bond_index];
  }

  return connectToAddress(address, name, lookupAddressType(address));
}

void bleKeyboardInputForgetByAddress(const char* address) {
  if (address == nullptr || address[0] == '\0') {
    return;
  }

  if (keyboard_connected && strcmp(connected_address, address) == 0) {
    disconnectClient();
  }

  NimBLEAddress peer =
      addressFromStored(address, lookupAddressType(address));
  NimBLEDevice::deleteBond(peer);
  removeBondedKeyboard(address);

  char last_address[18];
  loadLastAddress(last_address, sizeof(last_address));
  if (strcmp(last_address, address) == 0) {
    saveLastAddress(nullptr);
  }
}

void bleKeyboardInputForgetSelected() {
  if (selected_address[0] == '\0') {
    return;
  }
  bleKeyboardInputForgetByAddress(selected_address);
  selected_address[0] = '\0';
}

void bleKeyboardInputSetSelectedAddress(const char* address) {
  if (address == nullptr) {
    selected_address[0] = '\0';
    return;
  }
  strncpy(selected_address, address, sizeof(selected_address) - 1);
  selected_address[sizeof(selected_address) - 1] = '\0';
}

bool bleKeyboardInputIsConnected() { return keyboard_connected; }

bool bleKeyboardInputIsScanning() { return scanning; }

bool bleKeyboardInputIsConnecting() { return connecting; }

const char* bleKeyboardInputGetConnectedName() { return connected_name; }

const char* bleKeyboardInputGetConnectedAddress() { return connected_address; }

size_t bleKeyboardInputGetPairedDevices(BleKeyboardDeviceInfo* out, size_t max) {
  if (out == nullptr || max == 0) {
    return 0;
  }

  const size_t count = bonded_count < max ? bonded_count : max;
  for (size_t i = 0; i < count; ++i) {
    copyName(bonded_names[i], out[i].name, sizeof(out[i].name));
    strncpy(out[i].address, bonded_addresses[i], sizeof(out[i].address) - 1);
    out[i].address[sizeof(out[i].address) - 1] = '\0';
    out[i].connected =
        keyboard_connected && strcmp(connected_address, out[i].address) == 0;
  }
  return count;
}

size_t bleKeyboardInputGetScanResults(BleKeyboardDeviceInfo* out, size_t max) {
  if (out == nullptr || max == 0) {
    return 0;
  }

  const size_t count = scan_result_count < max ? scan_result_count : max;
  for (size_t i = 0; i < count; ++i) {
    copyName(scan_results[i].name, out[i].name, sizeof(out[i].name));
    strncpy(out[i].address, scan_results[i].address, sizeof(out[i].address) - 1);
    out[i].address[sizeof(out[i].address) - 1] = '\0';
    out[i].connected = false;
  }
  return count;
}

void bleKeyboardInputClearKeyboardBonds() {
  for (size_t i = 0; i < bonded_count; ++i) {
    NimBLEAddress peer =
        addressFromStored(bonded_addresses[i], bonded_address_types[i]);
    NimBLEDevice::deleteBond(peer);
  }

  bonded_count = 0;
  saveBondedKeyboards();
  saveLastAddress(nullptr);
  selected_address[0] = '\0';
  disconnectClient();
  bleKeyboardInputStopScan();
}
