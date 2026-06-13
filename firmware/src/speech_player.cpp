#include "speech_player.h"

#include "key_event.h"
#include "latency_log.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <SD.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <esp_heap_caps.h>
#endif

namespace echo {

namespace {

#ifndef NATIVE_TEST
struct WavHeaderInfo {
  uint32_t sample_rate = 0;
  uint16_t channels = 0;
  uint16_t bits_per_sample = 0;
  size_t data_offset = 0;
  size_t data_size = 0;
};

uint16_t readLe16(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
}

uint32_t readLe32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

bool parseWavHeader(const uint8_t* file_data, size_t file_size,
                    WavHeaderInfo& info) {
  if (file_size < 44 || std::memcmp(file_data, "RIFF", 4) != 0 ||
      std::memcmp(file_data + 8, "WAVE", 4) != 0) {
    return false;
  }

  size_t offset = 12;
  bool found_fmt = false;
  bool found_data = false;
  while (offset + 8 <= file_size) {
    const uint8_t* chunk_id = file_data + offset;
    const uint32_t chunk_size = readLe32(file_data + offset + 4);
    offset += 8;
    if (offset + chunk_size > file_size) {
      return false;
    }

    if (std::memcmp(chunk_id, "fmt ", 4) == 0) {
      if (chunk_size < 16) {
        return false;
      }
      const uint16_t audio_format = readLe16(file_data + offset);
      if (audio_format != 1) {
        return false;
      }
      info.channels = readLe16(file_data + offset + 2);
      info.sample_rate = readLe32(file_data + offset + 4);
      info.bits_per_sample = readLe16(file_data + offset + 14);
      found_fmt = true;
    } else if (std::memcmp(chunk_id, "data", 4) == 0) {
      info.data_offset = offset;
      info.data_size = chunk_size;
      found_data = true;
    }

    offset += chunk_size + (chunk_size & 1U);
  }

  return found_fmt && found_data && info.channels > 0 &&
         info.bits_per_sample == 16 && info.sample_rate > 0 &&
         info.data_size >= sizeof(int16_t);
}

struct ParsedPcm {
  int16_t* pcm_data = nullptr;
  size_t sample_count = 0;
  uint32_t sample_rate = 16000;
  bool stereo = false;
};

bool adoptWavFileAsPcm(uint8_t* file_data, size_t file_size, ParsedPcm& out) {
  WavHeaderInfo header;
  if (!parseWavHeader(file_data, file_size, header)) {
    free(file_data);
    return false;
  }

  const size_t sample_count = header.data_size / sizeof(int16_t);
  int16_t* pcm_data = static_cast<int16_t*>(
      heap_caps_malloc(header.data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!pcm_data) {
    pcm_data = static_cast<int16_t*>(malloc(header.data_size));
  }
  if (!pcm_data) {
    free(file_data);
    return false;
  }

  std::memcpy(pcm_data, file_data + header.data_offset, header.data_size);
  free(file_data);

  out.pcm_data = pcm_data;
  out.sample_count = sample_count;
  out.sample_rate = header.sample_rate;
  out.stereo = header.channels > 1;
  return true;
}
#endif

}  // namespace

size_t SpeechPlayer::cacheIndex(uint8_t hid_usage) const {
  if (hid_usage < 0x04 || hid_usage > 0xE7) {
    return kCacheCapacity;
  }
  return hid_usage - 0x04;
}

const SpeechPlayer::CachedAudio* SpeechPlayer::cachedAudio(
    uint8_t hid_usage) const {
  const size_t index = cacheIndex(hid_usage);
  if (index >= kCacheCapacity || cache_[index].pcm_data == nullptr) {
    return nullptr;
  }
  return &cache_[index];
}

void SpeechPlayer::begin(SpeechErrorCallback on_error) {
  on_error_ = std::move(on_error);
}

void SpeechPlayer::setVolumePercent(uint8_t volume_percent) {
  volume_percent_ = volume_percent;
#ifndef NATIVE_TEST
  M5.Speaker.setVolume(volume_percent);
#endif
}

void SpeechPlayer::stop() {
#ifndef NATIVE_TEST
  M5.Speaker.stop();
#endif
  playing_ = false;
}

bool SpeechPlayer::isPlaying() const {
#ifndef NATIVE_TEST
  return M5.Speaker.isPlaying();
#else
  return playing_;
#endif
}

bool SpeechPlayer::loadIntoCache(uint8_t hid_usage) {
#ifndef NATIVE_TEST
  const size_t index = cacheIndex(hid_usage);
  if (index >= kCacheCapacity || cache_[index].pcm_data != nullptr) {
    return cache_[index].pcm_data != nullptr;
  }

  const char* path = audioPathForUsage(hid_usage);
  if (!SD.exists(path)) {
    if (on_error_) {
      on_error_("Audio file missing on microSD");
    }
    return false;
  }

  File wav_file = SD.open(path, FILE_READ);
  if (!wav_file) {
    if (on_error_) {
      on_error_("Could not open audio file");
    }
    return false;
  }

  const size_t file_size = wav_file.size();
  uint8_t* buffer = static_cast<uint8_t*>(
      heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!buffer) {
    buffer = static_cast<uint8_t*>(malloc(file_size));
  }
  if (!buffer) {
    wav_file.close();
    if (on_error_) {
      on_error_("Not enough memory for audio");
    }
    return false;
  }

  const size_t read_bytes = wav_file.read(buffer, file_size);
  wav_file.close();
  if (read_bytes != file_size) {
    free(buffer);
    if (on_error_) {
      on_error_("Could not read audio file");
    }
    return false;
  }

  ParsedPcm parsed;
  if (!adoptWavFileAsPcm(buffer, file_size, parsed)) {
    if (on_error_) {
      on_error_("Unsupported audio file format");
    }
    return false;
  }
  cache_[index].pcm_data = parsed.pcm_data;
  cache_[index].sample_count = parsed.sample_count;
  cache_[index].sample_rate = parsed.sample_rate;
  cache_[index].stereo = parsed.stereo;
  return true;
#else
  (void)hid_usage;
  return true;
#endif
}

void SpeechPlayer::preloadFromSd(PreloadProgressCallback on_progress) {
#ifndef NATIVE_TEST
  File dir = SD.open("/audio/keys");
  if (!dir || !dir.isDirectory()) {
    return;
  }

  int total = 0;
  File entry;
  while ((entry = dir.openNextFile())) {
    const char* name = entry.name();
    unsigned int usage = 0;
    if (std::sscanf(name, "u%03x.wav", &usage) == 1 ||
        std::sscanf(name, "u%x.wav", &usage) == 1) {
      total++;
    }
    entry.close();
  }
  dir.close();

  if (on_progress) {
    on_progress(0, total);
  }

  dir = SD.open("/audio/keys");
  if (!dir || !dir.isDirectory()) {
    return;
  }

  int loaded = 0;
  while ((entry = dir.openNextFile())) {
    const char* name = entry.name();
    unsigned int usage = 0;
    if (std::sscanf(name, "u%03x.wav", &usage) == 1 ||
        std::sscanf(name, "u%x.wav", &usage) == 1) {
      loadIntoCache(static_cast<uint8_t>(usage));
      loaded++;
      if (on_progress) {
        on_progress(loaded, total);
      }
    }
    entry.close();
  }
  dir.close();
#else
  (void)on_progress;
#endif
}

void SpeechPlayer::speakKey(uint8_t hid_usage) {
#ifndef NATIVE_TEST
  const bool cache_hit = cachedAudio(hid_usage) != nullptr;
  latencyLog("speech", "speakKey 0x%02X cache_hit=%s", hid_usage,
             cache_hit ? "yes" : "no");

  if (!cache_hit) {
    const uint32_t load_start = millis();
    if (!loadIntoCache(hid_usage)) {
      latencyLog("speech", "loadIntoCache failed after %lums",
                 static_cast<unsigned long>(millis() - load_start));
      return;
    }
    latencyLog("speech", "loadIntoCache ok in %lums",
               static_cast<unsigned long>(millis() - load_start));
  }

  const CachedAudio* cached = cachedAudio(hid_usage);
  if (!cached) {
    return;
  }

  M5.Speaker.setVolume(volume_percent_);
  const uint32_t play_start = millis();
  if (!M5.Speaker.playRaw(cached->pcm_data, cached->sample_count,
                          cached->sample_rate, cached->stereo,
                          /*repeat=*/1, /*channel=*/-1,
                          /*stop_current_sound=*/true)) {
    latencyLog("speech", "playRaw failed after %lums",
               static_cast<unsigned long>(millis() - play_start));
    if (on_error_) {
      on_error_("Could not play audio file");
    }
  } else {
    playing_ = true;
    latencyLog("speech", "playRaw started in %lums", 
               static_cast<unsigned long>(millis() - play_start));
  }
#else
  (void)hid_usage;
  playing_ = true;
#endif
}

void SpeechPlayer::tick() {
#ifndef NATIVE_TEST
  if (playing_ && !M5.Speaker.isPlaying()) {
    playing_ = false;
  }
#endif
}

}  // namespace echo
