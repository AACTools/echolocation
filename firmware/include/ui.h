#pragma once

#include <stddef.h>
#include <stdint.h>

struct KeyBehavior;

void uiInit();
void uiSetLoadingStatus(const char* status);
void uiFinishLoading();
void uiPump();
void uiSetKeyboardConnected(bool connected);
void uiSetPressedKey(const char* label, const KeyBehavior* behavior = nullptr);
void uiSetKeyBoxOutline(bool show);
void uiSetVolume(uint8_t volume);
void uiSetBattery(int percent, bool charging);
uint32_t uiGetHoldDurationMs();
void uiSetHoldDurationMs(uint32_t ms);
void uiSetBluetoothOutput(bool enabled);
void uiSetBluetoothKeyboard(bool enabled);

void uiRefreshConnectionFlow();
void uiRefreshBluetoothOutputStatus();
void uiRefreshBluetoothKeyboardStatus();
void uiRefreshSpeakerOutput();
