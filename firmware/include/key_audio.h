#pragma once

#include <stddef.h>
#include <stdint.h>

struct KeyAudioDebugInfo {
  bool sd_mounted;
  bool audio_dir_exists;
  bool probe_a_wav;
  bool probe_b_wav;
  bool probe_c_wav;
  bool probe_space_wav;
  bool probe_enter_wav;
  int probe_files_found;
};

void keyAudioBegin();
void keyAudioRefresh();
void keyAudioGetDebugInfo(KeyAudioDebugInfo* info);
void keyAudioPlayForLabel(const char* label);
void keyAudioSetVolume(uint8_t volume);
uint8_t keyAudioGetVolume();
