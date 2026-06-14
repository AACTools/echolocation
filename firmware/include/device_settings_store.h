#pragma once

#include <stdint.h>

constexpr uint8_t kDefaultVolume = 128;
constexpr uint32_t kDefaultHoldDurationMs = 500;
constexpr char kDefaultBleComputerName[] = "echolocation";
constexpr char kDefaultBleKeyboardName[] = "";

void deviceSettingsLoad();
void deviceSettingsSaveVolume(uint8_t volume);
void deviceSettingsSaveHoldDurationMs(uint32_t ms);

void deviceSettingsSaveBleComputerName(const char* name);
const char* deviceSettingsGetBleComputerName();
void deviceSettingsSaveBleKeyboardName(const char* name);
const char* deviceSettingsGetBleKeyboardName();
