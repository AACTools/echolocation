#include "speaker_route.h"

#include "speaker_detect.h"

#include <M5Module_Audio.h>
#include <M5Unified.h>
#include <Wire.h>

#include <cstring>

namespace {

enum class ActiveRoute { kUnknown, kBuiltin, kExternal };

struct ExternalPlayJob {
  uint8_t* pcm = nullptr;
  size_t pcm_len = 0;
  es_sample_rate_t sample_rate = SAMPLE_RATE_44K;
};

M5ModuleAudio module_audio;
bool module_audio_ready = false;
ActiveRoute active_route = ActiveRoute::kUnknown;
uint8_t stored_volume = 128;
QueueHandle_t external_play_queue = nullptr;
TaskHandle_t external_play_task = nullptr;

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

es_sample_rate_t sampleRateToEs(uint32_t hz) {
  if (hz <= 8000) {
    return SAMPLE_RATE_8K;
  }
  if (hz <= 11025) {
    return SAMPLE_RATE_11K;
  }
  if (hz <= 16000) {
    return SAMPLE_RATE_16K;
  }
  if (hz <= 24000) {
    return SAMPLE_RATE_24K;
  }
  if (hz <= 32000) {
    return SAMPLE_RATE_32K;
  }
  if (hz <= 44100) {
    return SAMPLE_RATE_44K;
  }
  return SAMPLE_RATE_48K;
}

uint8_t volumeToModule(uint8_t volume) {
  return static_cast<uint8_t>((static_cast<uint16_t>(volume) * 100u + 127u) / 255u);
}

bool useExternalRoute() {
  return speakerDetectIsModulePresent() && speakerDetectIsExternalConnected();
}

void applyModuleVolume() {
  if (!module_audio_ready) {
    return;
  }
  module_audio.setSpeakerVolume(volumeToModule(stored_volume));
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

void freeQueuedJob(ExternalPlayJob* job) {
  if (job == nullptr) {
    return;
  }
  free(job->pcm);
  job->pcm = nullptr;
  job->pcm_len = 0;
}

void externalPlayWorker(void* /*arg*/) {
  for (;;) {
    ExternalPlayJob job;
    if (xQueueReceive(external_play_queue, &job, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    ExternalPlayJob newer;
    while (xQueueReceive(external_play_queue, &newer, 0) == pdTRUE) {
      freeQueuedJob(&job);
      job = newer;
    }

    module_audio.setSampleRate(job.sample_rate);
    applyModuleVolume();
    module_audio.setMute(false);
    module_audio.play(job.pcm, static_cast<int>(job.pcm_len));
    freeQueuedJob(&job);
  }
}

void ensureExternalPlayWorker() {
  if (external_play_queue == nullptr) {
    external_play_queue = xQueueCreate(1, sizeof(ExternalPlayJob));
  }
  if (external_play_queue == nullptr) {
    return;
  }

  if (external_play_task == nullptr) {
    xTaskCreate(externalPlayWorker, "ext_audio", 8192, nullptr, 5, &external_play_task);
  }
}

bool startExternalPlayAsync(const WavPcmInfo& wav_info) {
  size_t pcm_len = 0;
  uint8_t* pcm = prepareStereoPcm(wav_info, &pcm_len);
  if (pcm == nullptr || pcm_len == 0) {
    free(pcm);
    return false;
  }

  ensureExternalPlayWorker();
  if (external_play_queue == nullptr) {
    free(pcm);
    return false;
  }

  ExternalPlayJob job;
  job.pcm = pcm;
  job.pcm_len = pcm_len;
  job.sample_rate = sampleRateToEs(wav_info.sample_rate);

  ExternalPlayJob dropped;
  if (xQueueReceive(external_play_queue, &dropped, 0) == pdTRUE) {
    freeQueuedJob(&dropped);
  }

  if (xQueueSend(external_play_queue, &job, 0) != pdTRUE) {
    freeQueuedJob(&job);
    return false;
  }

  return true;
}

}  // namespace

void speakerRouteBegin() {
  stored_volume = M5.Speaker.getVolume();

  if (!speakerDetectIsModulePresent()) {
    module_audio_ready = false;
    active_route = ActiveRoute::kBuiltin;
    return;
  }

  if (!module_audio.begin(Wire)) {
#ifdef ECHOLOCATION_BLE_DEBUG
    Serial.println("[speaker] module audio init failed");
#endif
    module_audio_ready = false;
    active_route = ActiveRoute::kBuiltin;
    return;
  }

  module_audio.setHPMode(AUDIO_HPMODE_NATIONAL);
  module_audio.setSpeakerOutput(DAC_OUTPUT_OUT1);
  module_audio.setBitsSample(ES_MODULE_DAC, BIT_LENGTH_16BITS);
  module_audio.setSampleRate(SAMPLE_RATE_44K);
  applyModuleVolume();
  module_audio_ready = true;

  ensureExternalPlayWorker();
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
  if (module_audio_ready && useExternalRoute()) {
    module_audio.setMute(true);
  }
}

void speakerRoutePlayWav(const uint8_t* data, size_t len) {
  if (data == nullptr || len == 0) {
    return;
  }

  if (useExternalRoute() && module_audio_ready) {
    WavPcmInfo wav_info;
    if (!parseWavPcm(data, len, &wav_info)) {
      return;
    }

    M5.Speaker.stop();
    if (!startExternalPlayAsync(wav_info)) {
      module_audio.setMute(false);
      return;
    }
    return;
  }

  speakerRouteStop();
  M5.Speaker.playWav(data, len, 1, -1, true);
}

void speakerRouteSetVolume(uint8_t volume) {
  stored_volume = volume;
  M5.Speaker.setVolume(volume);
  applyModuleVolume();
}

uint8_t speakerRouteGetVolume() { return stored_volume; }
