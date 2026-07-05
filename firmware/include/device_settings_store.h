#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t kDefaultVolume = 128;
constexpr uint32_t kDefaultHoldDurationMs = 500;

void deviceSettingsLoad();
void deviceSettingsResetToFactory();
void deviceSettingsSaveVolume(uint8_t volume);
void deviceSettingsSaveHoldDurationMs(uint32_t ms);
