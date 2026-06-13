#pragma once

#include <stdint.h>

void usbHidComputerBegin();
void usbHidComputerTick();
void usbHidComputerSendKey(uint8_t mod, uint8_t key);
bool usbHidComputerIsReady();
