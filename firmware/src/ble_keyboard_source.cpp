#include "ble_keyboard_source.h"

#include "boot_key_map.h"

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <NimBLEDevice.h>
#endif

namespace echo {

#ifndef NATIVE_TEST

static BleKeyboardSource* g_owner = nullptr;

static void notifyKeyboardReport(NimBLERemoteCharacteristic* characteristic,
                                 uint8_t* data, size_t length, bool is_notify) {
  (void)characteristic;
  (void)is_notify;
  if (!g_owner || !g_owner->callback_ || length < 8) {
    return;
  }

  const uint8_t modifier = data[0];
  for (uint8_t bit = 0x01; bit != 0; bit <<= 1) {
    const uint8_t usage = bootModifierToHidUsage(bit);
    if (usage == 0) {
      continue;
    }
    KeyEvent event;
    event.hid_usage = usage;
    event.modifier_mask = modifier;
    event.pressed = (modifier & bit) != 0;
    event.timestamp_ms = millis();
    g_owner->dispatchKeyEvent(event);
  }

  for (size_t i = 2; i < 8; ++i) {
    if (data[i] == 0) {
      continue;
    }
    const uint8_t usage = bootKeyToHidUsage(data[i]);
    if (usage == 0) {
      continue;
    }
    KeyEvent event;
    event.hid_usage = usage;
    event.modifier_mask = modifier;
    event.pressed = true;
    event.timestamp_ms = millis();
    g_owner->dispatchKeyEvent(event);
  }
}

class BleKeyboardClientCallbacks : public NimBLEClientCallbacks {
 public:
  void onConnect(NimBLEClient* client) override {
    if (g_owner) {
      g_owner->setConnected(true);
      g_owner->setStatus("Bluetooth keyboard connected");
      g_owner->setScanning(false);
    }

    for (const auto& service : client->getServices(true)) {
      if (!service->getUUID().equals(NimBLEUUID((uint16_t)0x1812))) {
        continue;
      }
      for (const auto& characteristic : service->getCharacteristics(true)) {
        if (characteristic->canNotify()) {
          characteristic->subscribe(true, notifyKeyboardReport);
        }
      }
    }
  }

  void onDisconnect(NimBLEClient* client, int reason) override {
    (void)client;
    (void)reason;
    if (g_owner) {
      g_owner->setConnected(false);
      g_owner->setStatus("Bluetooth keyboard disconnected");
      g_owner->startScan();
    }
  }
};

class BleKeyboardScanCallbacks : public NimBLEScanCallbacks {
 public:
  void onResult(const NimBLEAdvertisedDevice* advertised_device) override {
    if (!advertised_device->isAdvertisingService(NimBLEUUID((uint16_t)0x1812))) {
      return;
    }
    NimBLEDevice::getScan()->stop();
    if (g_owner) {
      g_owner->setScanning(false);
      g_owner->setStatus("Connecting to Bluetooth keyboard");
    }
    NimBLEClient* client = NimBLEDevice::createClient();
    client->setClientCallbacks(new BleKeyboardClientCallbacks(), false);
    client->connect(advertised_device);
  }
};

#endif

void BleKeyboardSource::begin(KeyboardEventCallback callback) {
  callback_ = std::move(callback);
#ifndef NATIVE_TEST
  g_owner = this;
  NimBLEDevice::init("echolocation");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
#endif
}

void BleKeyboardSource::tick(uint32_t now_ms) { (void)now_ms; }

bool BleKeyboardSource::isKeyboardConnected() const { return connected_; }

void BleKeyboardSource::startScan() {
#ifndef NATIVE_TEST
  setStatus("Scanning for Bluetooth keyboard");
  setScanning(true);
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new BleKeyboardScanCallbacks(), false);
  scan->setActiveScan(true);
  scan->start(0, false, true);
#endif
}

void BleKeyboardSource::stopScan() {
#ifndef NATIVE_TEST
  NimBLEDevice::getScan()->stop();
#endif
  setScanning(false);
  setStatus("Bluetooth keyboard scan stopped");
}

bool BleKeyboardSource::isScanning() const { return scanning_; }

std::string BleKeyboardSource::statusMessage() const { return status_message_; }

void BleKeyboardSource::dispatchKeyEvent(const KeyEvent& event) {
  if (callback_) {
    callback_(event);
  }
}

void BleKeyboardSource::setConnected(bool connected) { connected_ = connected; }

void BleKeyboardSource::setStatus(const char* message) {
  status_message_ = message ? message : "";
}

void BleKeyboardSource::setScanning(bool scanning) { scanning_ = scanning; }

}  // namespace echo
