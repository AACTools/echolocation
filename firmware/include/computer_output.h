#pragma once

#include <stdint.h>

void computerOutputBegin();
void computerOutputTick();
void computerOutputSendKey(uint8_t mod, uint8_t key);

bool computerOutputUsbReady();
