#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint8_t kDefaultVolume = 128;
constexpr uint32_t kDefaultHoldDurationMs = 500;
constexpr char kDefaultBleComputerName[] = "echolocation";

void deviceSettingsLoad();
void deviceSettingsResetToFactory();
void deviceSettingsSaveVolume(uint8_t volume);
void deviceSettingsSaveHoldDurationMs(uint32_t ms);

void deviceSettingsSaveBleComputerName(const char* name);
const char* deviceSettingsGetBleComputerName();
void deviceSettingsSaveBleComputerEnabled(bool enabled);
bool deviceSettingsGetBleComputerEnabled();

void deviceSettingsSaveBleKeyboardEnabled(bool enabled);
bool deviceSettingsGetBleKeyboardEnabled();
void deviceSettingsSaveBleKeyboardDevice(const uint8_t address[6], const char* name,
                                         uint8_t addr_type);
bool deviceSettingsGetBleKeyboardAddress(uint8_t address_out[6]);
uint8_t deviceSettingsGetBleKeyboardAddressType();
void deviceSettingsGetBleKeyboardName(char* name_out, size_t name_len);
bool deviceSettingsHasBleKeyboardDevice();
