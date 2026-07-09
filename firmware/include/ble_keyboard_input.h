#pragma once

#include <stddef.h>
#include <stdint.h>

struct BleKeyboardDeviceInfo {
  char name[32];
  char address[18];
  bool connected;
};

void bleKeyboardInputBegin();
void bleKeyboardInputSetEnabled(bool enabled);
void bleKeyboardInputTick();
void bleKeyboardInputStartScan();
void bleKeyboardInputStopScan();
bool bleKeyboardInputConnectByAddress(const char* address);
void bleKeyboardInputForgetByAddress(const char* address);
void bleKeyboardInputForgetSelected();
void bleKeyboardInputSetSelectedAddress(const char* address);
bool bleKeyboardInputIsConnected();
bool bleKeyboardInputIsScanning();
bool bleKeyboardInputIsConnecting();
const char* bleKeyboardInputGetConnectedName();
const char* bleKeyboardInputGetConnectedAddress();
size_t bleKeyboardInputGetPairedDevices(BleKeyboardDeviceInfo* out, size_t max);
size_t bleKeyboardInputGetScanResults(BleKeyboardDeviceInfo* out, size_t max);
void bleKeyboardInputClearKeyboardBonds();
