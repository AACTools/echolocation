#pragma once

#include <stddef.h>
#include <stdint.h>

struct BleKeyboardDevice {
  char name[32];
  uint8_t address[6];
  uint8_t addr_type;
  int rssi;
};

void bleKeyboardBegin();
void bleKeyboardTick();
void bleKeyboardSetEnabled(bool enabled);
bool bleKeyboardIsEnabled();
bool bleKeyboardIsConnected();
void bleKeyboardStartScan();
void bleKeyboardStopScan();
bool bleKeyboardIsScanning();
bool bleKeyboardIsConnecting();
bool bleKeyboardLastConnectFailed();
size_t bleKeyboardGetDeviceCount();
bool bleKeyboardGetDevice(size_t index, BleKeyboardDevice* out);
void bleKeyboardConnect(const uint8_t address[6]);
void bleKeyboardDisconnect();
bool bleKeyboardGetConnectedDevice(BleKeyboardDevice* out);
void bleKeyboardSetSavedDevice(const uint8_t address[6], const char* name,
                               uint8_t addr_type);
bool bleKeyboardAddressesEqual(const uint8_t a[6], const uint8_t b[6]);
