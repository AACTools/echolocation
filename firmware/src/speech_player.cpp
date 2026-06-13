#include "speech_player.h"

#include "key_event.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <SD.h>
#include <cstdio>
#include <cstdlib>

#include <esp_heap_caps.h>
#endif

namespace echo {

size_t SpeechPlayer::cacheIndex(uint8_t hid_usage) const {
  if (hid_usage < 0x04 || hid_usage > 0xE7) {
    return kCacheCapacity;
  }
  return hid_usage - 0x04;
}

const SpeechPlayer::CachedAudio* SpeechPlayer::cachedAudio(
    uint8_t hid_usage) const {
  const size_t index = cacheIndex(hid_usage);
  if (index >= kCacheCapacity || cache_[index].data == nullptr) {
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
  if (index >= kCacheCapacity || cache_[index].data != nullptr) {
    return cache_[index].data != nullptr;
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

  cache_[index].data = buffer;
  cache_[index].size = file_size;
  return true;
#else
  (void)hid_usage;
  return true;
#endif
}

void SpeechPlayer::preloadFromSd() {
#ifndef NATIVE_TEST
  File dir = SD.open("/audio/keys");
  if (!dir || !dir.isDirectory()) {
    return;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    const char* name = entry.name();
    unsigned int usage = 0;
    if (std::sscanf(name, "u%03x.wav", &usage) == 1 ||
        std::sscanf(name, "u%x.wav", &usage) == 1) {
      loadIntoCache(static_cast<uint8_t>(usage));
    }
    entry.close();
  }
  dir.close();
#endif
}

void SpeechPlayer::speakKey(uint8_t hid_usage) {
#ifndef NATIVE_TEST
  if (!cachedAudio(hid_usage) && !loadIntoCache(hid_usage)) {
    return;
  }

  const CachedAudio* cached = cachedAudio(hid_usage);
  if (!cached) {
    return;
  }

  stop();
  M5.Speaker.setVolume(volume_percent_);
  if (!M5.Speaker.playWav(cached->data, cached->size, /*repeat=*/1,
                           /*channel=*/-1, /*stop_current=*/true)) {
    if (on_error_) {
      on_error_("Could not play audio file");
    }
  } else {
    playing_ = true;
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
