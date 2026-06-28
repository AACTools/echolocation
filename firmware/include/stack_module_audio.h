#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool stackModuleAudioBegin();
bool stackModuleAudioReady();
bool stackModuleEnsureI2s();
void stackModuleEndI2s();
bool stackModuleIsI2sActive();
void stackModulePreparePlayback();
void stackModuleSetVolume(uint8_t volume);
void stackModuleStop();
bool stackModulePlayPcmAsync(uint8_t* pcm, size_t len, uint32_t sample_rate_hz);
