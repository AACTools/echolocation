#pragma once

#include <stdint.h>

void bleHidComputerBegin();
void bleHidComputerTick();
void bleHidComputerSendKey(uint8_t mod, uint8_t key);

bool bleHidComputerIsConnected();
bool bleHidComputerIsAdvertising();

void bleHidComputerSetEnabled(bool enabled);
bool bleHidComputerIsEnabled();
void bleHidComputerStartPairing();
void bleHidComputerStopPairing();
void bleHidComputerSetDeviceName(const char* name);
const char* bleHidComputerGetDeviceName();
