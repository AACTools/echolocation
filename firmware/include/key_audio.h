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
  int cached_wav_count;
};

void keyAudioBegin();
void keyAudioRefresh();
void keyAudioGetDebugInfo(KeyAudioDebugInfo* info);
void keyAudioPlayForToken(const char* token);
void keyAudioPlayForLabel(const char* label);
void keyAudioSetVolume(uint8_t volume);
uint8_t keyAudioGetVolume();
