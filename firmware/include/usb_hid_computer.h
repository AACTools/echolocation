#pragma once

#include <stdint.h>

void usbHidComputerBegin();
void usbHidComputerTick();
void usbHidComputerSendKey(uint8_t mod, uint8_t key);
void usbHidComputerSendBootReport(const uint8_t report[8]);
bool usbHidComputerIsReady();
