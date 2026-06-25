#pragma once

#include <stddef.h>
#include <stdint.h>

void speakerRouteBegin();
void speakerRouteApply();
void speakerRoutePlayWav(const uint8_t* data, size_t len);
void speakerRouteStop();
void speakerRouteSetVolume(uint8_t volume);
uint8_t speakerRouteGetVolume();
