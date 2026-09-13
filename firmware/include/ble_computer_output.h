#pragma once

#include <stdint.h>

void bleComputerOutputBegin();
void bleComputerOutputSetEnabled(bool enabled);
void bleComputerOutputTick();
void bleComputerOutputSendKey(uint8_t mod, uint8_t key);
void bleComputerOutputSendBootReport(const uint8_t report[8]);
bool bleComputerOutputIsConnected();
