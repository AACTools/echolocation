#pragma once

#include <stdint.h>

void computerOutputBegin();
void computerOutputTick();
void computerOutputSendKey(uint8_t mod, uint8_t key);

bool computerOutputUsbReady();
bool computerOutputBleConnected();
bool computerOutputBleAdvertising();

void computerOutputBleSetEnabled(bool enabled);
bool computerOutputBleIsEnabled();
void computerOutputBleStartPairing();
void computerOutputBleStopPairing();
void computerOutputBleSetDeviceName(const char* name);
const char* computerOutputBleGetDeviceName();
