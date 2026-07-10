#include "key_audio.h"

#include "speaker_route.h"

#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>

#include <cstring>

#include <esp_heap_caps.h>
#include <soc/gpio_reg.h>

namespace {

constexpr int kLcdCs = 3;
constexpr int kSdCs = 4;
constexpr char kAudioDir[] = "/audio";
constexpr uint32_t kFspiQOutIdx = 102;
// Keep a small hot cache so boot does not load every WAV into RAM.
// CoreS3 PSRAM is ~8MB; the full speech set is larger than that.
constexpr size_t kMaxCachedFiles = 48;
constexpr size_t kBasenameLen = 32;

struct CachedWav {
  char basename[kBasenameLen];
  uint8_t* data = nullptr;
  size_t size = 0;
  uint32_t last_used_ms = 0;
};

bool sd_ready = false;
CachedWav cached_wavs[kMaxCachedFiles];
size_t cached_wav_count = 0;

void enableSdPower() {
  M5.In_I2C.bitOn(0x58, 0x02, 0b00010000, 400000);
}

void prepareSdBus() {
  pinMode(kLcdCs, OUTPUT);
  digitalWrite(kLcdCs, HIGH);

  *(volatile uint32_t*)GPIO_FUNC35_OUT_SEL_CFG_REG = kFspiQOutIdx;
  *(volatile uint32_t*)GPIO_ENABLE1_W1TC_REG = 1u << (GPIO_NUM_35 & 31);
}

bool mountSdCard() {
  enableSdPower();
  prepareSdBus();
  return SD.begin(kSdCs, SPI, 25000000);
}

bool tokenToBasename(const char* token, char* out, size_t out_len) {
  if (token == nullptr || token[0] == '\0' || out_len < 2) {
    return false;
  }

  size_t i = 0;
  for (; i + 1 < out_len && token[i] != '\0'; ++i) {
    const unsigned char c = static_cast<unsigned char>(token[i]);
    if (c >= 'A' && c <= 'Z') {
      out[i] = static_cast<char>(c - 'A' + 'a');
    } else {
      out[i] = static_cast<char>(c);
    }
  }
  out[i] = '\0';
  return i > 0;
}

uint8_t* allocWavBuffer(size_t size) {
  // WAV data must stay in PSRAM. Falling back to internal RAM starves LVGL
  // and can leave the screen stuck on a blank/gray frame.
  return static_cast<uint8_t*>(
      heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}

void freeCachedEntry(CachedWav* entry) {
  if (entry == nullptr) {
    return;
  }
  free(entry->data);
  entry->data = nullptr;
  entry->size = 0;
  entry->last_used_ms = 0;
  entry->basename[0] = '\0';
}

void clearWavCache() {
  for (size_t i = 0; i < cached_wav_count; ++i) {
    freeCachedEntry(&cached_wavs[i]);
  }
  cached_wav_count = 0;
}

CachedWav* findCachedWavMutable(const char* basename) {
  if (basename == nullptr) {
    return nullptr;
  }

  for (size_t i = 0; i < cached_wav_count; ++i) {
    if (strcmp(cached_wavs[i].basename, basename) == 0) {
      return &cached_wavs[i];
    }
  }
  return nullptr;
}

CachedWav* findOldestCachedWav() {
  if (cached_wav_count == 0) {
    return nullptr;
  }

  CachedWav* oldest = &cached_wavs[0];
  for (size_t i = 1; i < cached_wav_count; ++i) {
    if (cached_wavs[i].last_used_ms < oldest->last_used_ms) {
      oldest = &cached_wavs[i];
    }
  }
  return oldest;
}

CachedWav* allocateCacheSlot() {
  if (cached_wav_count < kMaxCachedFiles) {
    return &cached_wavs[cached_wav_count++];
  }

  CachedWav* oldest = findOldestCachedWav();
  freeCachedEntry(oldest);
  return oldest;
}

bool loadWavIntoCache(const char* basename) {
  if (basename == nullptr || basename[0] == '\0' || !sd_ready) {
    return false;
  }

  if (findCachedWavMutable(basename) != nullptr) {
    return true;
  }

  char path[48];
  snprintf(path, sizeof(path), "%s/%s.wav", kAudioDir, basename);

  prepareSdBus();
  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  const size_t file_size = file.size();
  if (file_size == 0) {
    file.close();
    return false;
  }

  uint8_t* buffer = allocWavBuffer(file_size);
  if (buffer == nullptr) {
    CachedWav* oldest = findOldestCachedWav();
    if (oldest != nullptr) {
      freeCachedEntry(oldest);
    }
    buffer = allocWavBuffer(file_size);
    if (buffer == nullptr) {
      file.close();
      return false;
    }
  }

  const size_t bytes_read = file.read(buffer, file_size);
  file.close();
  if (bytes_read != file_size) {
    free(buffer);
    return false;
  }

  CachedWav* entry = findCachedWavMutable(basename);
  if (entry == nullptr) {
    entry = allocateCacheSlot();
  }
  if (entry == nullptr) {
    free(buffer);
    return false;
  }

  // Slot may already hold an evicted entry's leftover pointers.
  free(entry->data);
  strncpy(entry->basename, basename, sizeof(entry->basename) - 1);
  entry->basename[sizeof(entry->basename) - 1] = '\0';
  entry->data = buffer;
  entry->size = file_size;
  entry->last_used_ms = millis();
  return true;
}

bool probeWavOnSd(const char* basename) {
  if (!sd_ready || basename == nullptr || basename[0] == '\0') {
    return false;
  }

  char path[48];
  snprintf(path, sizeof(path), "%s/%s.wav", kAudioDir, basename);
  prepareSdBus();
  return SD.exists(path);
}

void ensureSdReady() {
  prepareSdBus();
  if (!sd_ready) {
    sd_ready = mountSdCard();
  }
}

}  // namespace

void keyAudioBegin() {
  sd_ready = mountSdCard();
  clearWavCache();
}

void keyAudioRefresh() {
  SD.end();
  clearWavCache();
  sd_ready = mountSdCard();
  Serial.printf("[audio] sd %s (lazy cache, max %u clips)\n",
                sd_ready ? "ready" : "missing",
                static_cast<unsigned>(kMaxCachedFiles));
}

void keyAudioGetDebugInfo(KeyAudioDebugInfo* info) {
  if (info == nullptr) {
    return;
  }

  *info = {};

  ensureSdReady();

  info->sd_mounted = sd_ready;
  info->cached_wav_count = static_cast<int>(cached_wav_count);
  if (!sd_ready) {
    return;
  }

  info->audio_dir_exists = SD.exists(kAudioDir);
  info->probe_a_wav = probeWavOnSd("a");
  info->probe_b_wav = probeWavOnSd("b");
  info->probe_c_wav = probeWavOnSd("c");
  info->probe_space_wav = probeWavOnSd("space");
  info->probe_enter_wav = probeWavOnSd("enter");

  info->probe_files_found = 0;
  if (info->probe_a_wav) {
    ++info->probe_files_found;
  }
  if (info->probe_b_wav) {
    ++info->probe_files_found;
  }
  if (info->probe_c_wav) {
    ++info->probe_files_found;
  }
  if (info->probe_space_wav) {
    ++info->probe_files_found;
  }
  if (info->probe_enter_wav) {
    ++info->probe_files_found;
  }
}

void keyAudioPlayForToken(const char* token) {
  if (token == nullptr || token[0] == '\0') {
    return;
  }

  char basename[32];
  if (!tokenToBasename(token, basename, sizeof(basename))) {
    return;
  }

  ensureSdReady();
  if (!sd_ready) {
    return;
  }

  CachedWav* cached = findCachedWavMutable(basename);
  if (cached == nullptr) {
    if (!loadWavIntoCache(basename)) {
      return;
    }
    cached = findCachedWavMutable(basename);
  }
  if (cached == nullptr || cached->data == nullptr || cached->size == 0) {
    return;
  }

  cached->last_used_ms = millis();
  speakerRouteStop();
  speakerRoutePlayWav(cached->data, cached->size);
}

void keyAudioPlayForLabel(const char* label) { keyAudioPlayForToken(label); }

void keyAudioSetVolume(uint8_t volume) { speakerRouteSetVolume(volume); }

uint8_t keyAudioGetVolume() { return speakerRouteGetVolume(); }
