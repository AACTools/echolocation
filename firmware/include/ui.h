#pragma once

#include <stdint.h>

void uiInit();
void uiSetLoadingStatus(const char* status);
void uiFinishLoading();
void uiPump();
void uiSetKeyboardConnected(bool connected);
void uiSetPressedKey(const char* label);
void uiSetKeyBoxOutline(bool show);
void uiSetVolume(uint8_t volume);
void uiSetBattery(int percent, bool charging);
uint32_t uiGetHoldDurationMs();
void uiSetHoldDurationMs(uint32_t ms);
void uiSetBluetoothOutput(bool enabled);

void uiRefreshConnectionFlow();
void uiRefreshBluetoothOutputStatus();
void uiRefreshSpeakerOutput();
