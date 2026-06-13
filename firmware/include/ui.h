#pragma once

#include <stdint.h>

void uiInit();
void uiSetKeyboardConnected(bool connected);
void uiSetPressedKey(const char* label);
void uiSetKeyBoxOutline(bool show);
void uiSetVolume(uint8_t volume);
uint32_t uiGetHoldDurationMs();
void uiSetHoldDurationMs(uint32_t ms);
