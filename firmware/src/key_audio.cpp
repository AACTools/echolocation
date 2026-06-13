#include "key_audio.h"

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

bool sd_ready = false;
uint8_t* wav_buffer = nullptr;
size_t wav_buffer_capacity = 0;

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

bool audioFileExists(const char* filename) {
  char path[48];
  snprintf(path, sizeof(path), "%s/%s", kAudioDir, filename);
  return SD.exists(path);
}

}  // namespace

void keyAudioBegin() {
  sd_ready = mountSdCard();
}

void keyAudioRefresh() {
  SD.end();
  sd_ready = mountSdCard();
}

void keyAudioGetDebugInfo(KeyAudioDebugInfo* info) {
  if (info == nullptr) {
    return;
  }

  *info = {};

  prepareSdBus();
  if (!sd_ready) {
    sd_ready = mountSdCard();
  }

  info->sd_mounted = sd_ready;
  if (!sd_ready) {
    return;
  }

  info->audio_dir_exists = SD.exists(kAudioDir);
  info->probe_a_wav = audioFileExists("a.wav");
  info->probe_b_wav = audioFileExists("b.wav");
  info->probe_c_wav = audioFileExists("c.wav");
  info->probe_space_wav = audioFileExists("space.wav");
  info->probe_enter_wav = audioFileExists("enter.wav");

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
  if (!sd_ready || label == nullptr || label[0] == '\0') {
    return;
  }

  prepareSdBus();

  char basename[16];
  if (!labelToBasename(label, basename, sizeof(basename))) {
    return;
  }

  char path[48];
  snprintf(path, sizeof(path), "%s/%s.wav", kAudioDir, basename);
  if (!SD.exists(path)) {
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return;
  }

  const size_t file_size = file.size();
  if (file_size == 0) {
    file.close();
    return;
  }

  if (file_size > wav_buffer_capacity) {
    uint8_t* new_buffer = allocBuffer(file_size);
    if (new_buffer == nullptr) {
      file.close();
      return;
    }
    free(wav_buffer);
    wav_buffer = new_buffer;
    wav_buffer_capacity = file_size;
  }

  const size_t bytes_read = file.read(wav_buffer, file_size);
  file.close();
  if (bytes_read != file_size) {
    return;
  }

  M5.Speaker.stop();
  M5.Speaker.playWav(wav_buffer, file_size, 1, -1, true);
}
