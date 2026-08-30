#pragma once

#include <stdint.h>

void computerOutputBegin();
void computerOutputTick();
void computerOutputSetBluetoothEnabled(bool enabled);
void computerOutputSendKey(uint8_t mod, uint8_t key);
void computerOutputSendBootReport(const uint8_t report[8]);

bool computerOutputUsbReady();
bool computerOutputBluetoothConnected();
