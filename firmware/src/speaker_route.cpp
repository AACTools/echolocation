#include "speaker_route.h"

#include "speaker_detect.h"
#include "stack_module_audio.h"

#include <M5Unified.h>

#include <cstring>

namespace {

enum class ActiveRoute { kUnknown, kBuiltin, kExternal };
enum class I2sOwner { kNone, kBuiltin, kExternal };

ActiveRoute active_route = ActiveRoute::kUnknown;
I2sOwner i2s_owner = I2sOwner::kNone;
bool builtin_speaker_ended = false;
uint8_t stored_volume = 128;

struct WavPcmInfo {
  const uint8_t* pcm = nullptr;
  size_t pcm_len = 0;
  uint32_t sample_rate = 0;
  uint16_t num_channels = 0;
  uint16_t bits_per_sample = 0;
};

uint32_t readLe32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t readLe16(const uint8_t* data) {
  return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

bool parseWavPcm(const uint8_t* data, size_t len, WavPcmInfo* out) {
  if (out == nullptr || len < 44) {
    return false;
  }

  if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
    return false;
  }

  size_t offset = 12;
  uint32_t sample_rate = 0;
  uint16_t audio_format = 0;
  uint16_t num_channels = 0;
  uint16_t bits_per_sample = 0;
  const uint8_t* pcm = nullptr;
  size_t pcm_len = 0;

  while (offset + 8 <= len) {
    const uint8_t* chunk_id = data + offset;
    const uint32_t chunk_size = readLe32(data + offset + 4);
    offset += 8;
    if (offset + chunk_size > len) {
      break;
    }

    if (memcmp(chunk_id, "fmt ", 4) == 0 && chunk_size >= 16) {
      audio_format = readLe16(data + offset);
      num_channels = readLe16(data + offset + 2);
      sample_rate = readLe32(data + offset + 4);
      bits_per_sample = readLe16(data + offset + 14);
    } else if (memcmp(chunk_id, "data", 4) == 0) {
      pcm = data + offset;
      pcm_len = chunk_size;
    }

    offset += chunk_size + (chunk_size & 1u);
  }

  if (audio_format != 1 || pcm == nullptr || pcm_len == 0 || sample_rate == 0 ||
      num_channels == 0 || num_channels > 2 || bits_per_sample != 16) {
    return false;
  }

  out->pcm = pcm;
  out->pcm_len = pcm_len;
  out->sample_rate = sample_rate;
  out->num_channels = num_channels;
  out->bits_per_sample = bits_per_sample;
  return true;
}

uint8_t volumeToModule(uint8_t volume) {
  return static_cast<uint8_t>((static_cast<uint16_t>(volume) * 100u + 127u) / 255u);
}

bool useExternalRoute() {
  return speakerDetectIsModulePresent() && speakerDetectIsExternalConnected();
}

uint8_t* prepareStereoPcm(const WavPcmInfo& wav_info, size_t* out_len) {
  if (wav_info.pcm == nullptr || wav_info.pcm_len == 0 || out_len == nullptr) {
    return nullptr;
  }

  if (wav_info.num_channels == 2) {
    uint8_t* copy = static_cast<uint8_t*>(malloc(wav_info.pcm_len));
    if (copy == nullptr) {
      return nullptr;
    }
    memcpy(copy, wav_info.pcm, wav_info.pcm_len);
    *out_len = wav_info.pcm_len;
    return copy;
  }

  const size_t sample_count = wav_info.pcm_len / sizeof(int16_t);
  const size_t stereo_len = sample_count * 2 * sizeof(int16_t);
  uint8_t* stereo = static_cast<uint8_t*>(malloc(stereo_len));
  if (stereo == nullptr) {
    return nullptr;
  }

  const int16_t* mono = reinterpret_cast<const int16_t*>(wav_info.pcm);
  int16_t* out = reinterpret_cast<int16_t*>(stereo);
  for (size_t i = 0; i < sample_count; ++i) {
    out[i * 2] = mono[i];
    out[i * 2 + 1] = mono[i];
  }

  *out_len = stereo_len;
  return stereo;
}

void restoreBuiltinSpeakerIfNeeded() {
  if (!builtin_speaker_ended) {
    return;
  }

  if (M5.Speaker.begin()) {
    M5.Speaker.setVolume(stored_volume);
    builtin_speaker_ended = false;
    i2s_owner = I2sOwner::kBuiltin;
  }
}

void releaseBuiltinI2sIfNeeded() {
  if (i2s_owner != I2sOwner::kBuiltin) {
    return;
  }

  M5.Speaker.stop();
  M5.Speaker.end();
  builtin_speaker_ended = true;
  i2s_owner = I2sOwner::kNone;
}

bool acquireExternalI2s() {
  M5.Speaker.stop();

  if (i2s_owner == I2sOwner::kBuiltin) {
    releaseBuiltinI2sIfNeeded();
  }

  if (i2s_owner == I2sOwner::kExternal && stackModuleIsI2sActive()) {
    return true;
  }

  if (stackModuleIsI2sActive()) {
    stackModuleEndI2s();
  }

  if (!stackModuleEnsureI2s()) {
    return false;
  }

  i2s_owner = I2sOwner::kExternal;
  return true;
}

void acquireBuiltinI2s() {
  stackModuleStop();

  if (i2s_owner == I2sOwner::kExternal && stackModuleIsI2sActive()) {
    stackModuleEndI2s();
    i2s_owner = I2sOwner::kNone;
  }

  restoreBuiltinSpeakerIfNeeded();
  if (i2s_owner == I2sOwner::kNone && !builtin_speaker_ended) {
    i2s_owner = I2sOwner::kBuiltin;
  }
}

}  // namespace

void speakerRouteBegin() {
  stored_volume = M5.Speaker.getVolume();

  if (speakerDetectIsModulePresent()) {
    stackModuleAudioBegin();
    stackModuleSetVolume(volumeToModule(stored_volume));
  }

  active_route = ActiveRoute::kUnknown;
  speakerRouteApply();
}

void speakerRouteApply() {
  const ActiveRoute desired =
      useExternalRoute() ? ActiveRoute::kExternal : ActiveRoute::kBuiltin;

  if (desired == active_route) {
    return;
  }

  speakerRouteStop();
  active_route = desired;

#ifdef ECHOLOCATION_BLE_DEBUG
  Serial.printf("[speaker] route apply external=%d\n",
                desired == ActiveRoute::kExternal ? 1 : 0);
#endif
}

void speakerRouteStop() {
  M5.Speaker.stop();
  if (active_route == ActiveRoute::kExternal && stackModuleAudioReady()) {
    stackModuleStop();
  }
}

void speakerRoutePlayWav(const uint8_t* data, size_t len) {
  if (data == nullptr || len == 0) {
    return;
  }

  if (useExternalRoute()) {
    if (!stackModuleAudioReady()) {
      acquireBuiltinI2s();
      M5.Speaker.playWav(data, len, 1, -1, true);
      return;
    }

    WavPcmInfo wav_info;
    if (!parseWavPcm(data, len, &wav_info)) {
      return;
    }

    size_t pcm_len = 0;
    uint8_t* pcm = prepareStereoPcm(wav_info, &pcm_len);
    if (pcm == nullptr || pcm_len == 0) {
      free(pcm);
      return;
    }

    if (!acquireExternalI2s()) {
      free(pcm);
      acquireBuiltinI2s();
      M5.Speaker.playWav(data, len, 1, -1, true);
      return;
    }

    stackModulePreparePlayback();
    stackModuleSetVolume(volumeToModule(stored_volume));
    if (!stackModulePlayPcmAsync(pcm, pcm_len, wav_info.sample_rate)) {
      free(pcm);
    }
    return;
  }

  acquireBuiltinI2s();
  M5.Speaker.stop();
  M5.Speaker.playWav(data, len, 1, -1, true);
}

void speakerRouteSetVolume(uint8_t volume) {
  stored_volume = volume;
  M5.Speaker.setVolume(volume);
  stackModuleSetVolume(volumeToModule(volume));
}

uint8_t speakerRouteGetVolume() { return stored_volume; }
