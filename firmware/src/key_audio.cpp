#include "key_audio.h"

#include "speaker_route.h"

#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>

#include <cctype>
#include <cstring>

#include <esp_heap_caps.h>
#include <soc/gpio_reg.h>

namespace {

constexpr int kLcdCs = 3;
constexpr int kSdCs = 4;
constexpr char kAudioDir[] = "/audio";
constexpr uint32_t kFspiQOutIdx = 102;
constexpr size_t kMaxCachedFiles = 128;
constexpr size_t kBasenameLen = 16;

struct CachedWav {
  char basename[kBasenameLen];
  uint8_t* data = nullptr;
  size_t size = 0;
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

bool labelToBasename(const char* label, char* out, size_t out_len) {
  if (label == nullptr || label[0] == '\0' || out_len < 2) {
    return false;
  }

  const size_t label_len = strlen(label);
  if (label_len == 1) {
    char c = label[0];
    if (isalpha(static_cast<unsigned char>(c))) {
      c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    out[0] = c;
    out[1] = '\0';
    return true;
  }

  size_t i = 0;
  for (; i + 1 < out_len && label[i] != '\0'; ++i) {
    out[i] = static_cast<char>(tolower(static_cast<unsigned char>(label[i])));
  }
  out[i] = '\0';
  return i > 0;
}

uint8_t* allocBuffer(size_t size) {
  uint8_t* buffer = static_cast<uint8_t*>(
      heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (buffer == nullptr) {
    buffer = static_cast<uint8_t*>(malloc(size));
  }
  return buffer;
}

void clearWavCache() {
  for (size_t i = 0; i < cached_wav_count; ++i) {
    free(cached_wavs[i].data);
    cached_wavs[i].data = nullptr;
    cached_wavs[i].size = 0;
    cached_wavs[i].basename[0] = '\0';
  }
  cached_wav_count = 0;
}

bool cacheWavFile(const char* basename) {
  if (basename == nullptr || basename[0] == '\0' ||
      cached_wav_count >= kMaxCachedFiles) {
    return false;
  }

  char path[48];
  snprintf(path, sizeof(path), "%s/%s.wav", kAudioDir, basename);
  if (!SD.exists(path)) {
    return false;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  const size_t file_size = file.size();
  if (file_size == 0) {
    file.close();
    return false;
  }

  uint8_t* buffer = allocBuffer(file_size);
  if (buffer == nullptr) {
    file.close();
    return false;
  }

  const size_t bytes_read = file.read(buffer, file_size);
  file.close();
  if (bytes_read != file_size) {
    free(buffer);
    return false;
  }

  CachedWav* entry = &cached_wavs[cached_wav_count++];
  strncpy(entry->basename, basename, sizeof(entry->basename) - 1);
  entry->basename[sizeof(entry->basename) - 1] = '\0';
  entry->data = buffer;
  entry->size = file_size;
  return true;
}

void loadWavCacheFromSd() {
  clearWavCache();

  File dir = SD.open(kAudioDir);
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    return;
  }

  for (File entry = dir.openNextFile(); entry && cached_wav_count < kMaxCachedFiles;
       entry = dir.openNextFile()) {
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    const char* filename = entry.name();
    const size_t name_len = strlen(filename);
    if (name_len < 5 || strcasecmp(filename + name_len - 4, ".wav") != 0) {
      entry.close();
      continue;
    }

    char basename[kBasenameLen];
    const size_t base_len = name_len - 4;
    if (base_len >= sizeof(basename)) {
      entry.close();
      continue;
    }
    memcpy(basename, filename, base_len);
    basename[base_len] = '\0';
    entry.close();

    cacheWavFile(basename);
  }

  dir.close();
}

const CachedWav* findCachedWav(const char* basename) {
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

bool audioFileExists(const char* filename) {
  return findCachedWav(filename) != nullptr;
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
  if (sd_ready) {
    loadWavCacheFromSd();
  }
}

void keyAudioRefresh() {
  SD.end();
  sd_ready = mountSdCard();
  if (sd_ready) {
    loadWavCacheFromSd();
  } else {
    clearWavCache();
  }
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
  info->probe_a_wav = audioFileExists("a");
  info->probe_b_wav = audioFileExists("b");
  info->probe_c_wav = audioFileExists("c");
  info->probe_space_wav = audioFileExists("space");
  info->probe_enter_wav = audioFileExists("enter");

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

void keyAudioPlayForLabel(const char* label) {
  if (label == nullptr || label[0] == '\0') {
    return;
  }

  char basename[16];
  if (!labelToBasename(label, basename, sizeof(basename))) {
    return;
  }

  const CachedWav* cached = findCachedWav(basename);
  if (cached == nullptr || cached->data == nullptr || cached->size == 0) {
    return;
  }

  speakerRoutePlayWav(cached->data, cached->size);
}

void keyAudioSetVolume(uint8_t volume) { speakerRouteSetVolume(volume); }

uint8_t keyAudioGetVolume() { return speakerRouteGetVolume(); }
