#pragma once

#include <stddef.h>
#include <stdint.h>

void keyboardInputOnKeyDown(uint8_t mod, uint8_t key);
void keyboardInputOnKeyUp(uint8_t mod, uint8_t key);
void keyboardInputTick();
void keyboardInputProcessBootReport(uint8_t* prev_state, const uint8_t* report,
                                    size_t len);
bool keyboardInputKeyToLabel(uint8_t mod, uint8_t key, char* out, size_t out_len);
