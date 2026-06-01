#include "speech_player.h"

#include "key_event.h"

#ifndef NATIVE_TEST
#include <M5Unified.h>
#include <SD.h>
#include <cstdlib>
#endif

namespace echo {

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

void SpeechPlayer::speakKey(uint8_t hid_usage) {
#ifndef NATIVE_TEST
  const char* path = audioPathForUsage(hid_usage);
  if (!SD.exists(path)) {
    if (on_error_) {
      on_error_("Audio file missing on microSD");
    }
    return;
  }

  File wav_file = SD.open(path, FILE_READ);
  if (!wav_file) {
    if (on_error_) {
      on_error_("Could not open audio file");
    }
    return;
  }

  const size_t file_size = wav_file.size();
  uint8_t* buffer = static_cast<uint8_t*>(malloc(file_size));
  if (!buffer) {
    wav_file.close();
    if (on_error_) {
      on_error_("Not enough memory for audio");
    }
    return;
  }

  const size_t read_bytes = wav_file.read(buffer, file_size);
  wav_file.close();
  if (read_bytes != file_size) {
    free(buffer);
    if (on_error_) {
      on_error_("Could not read audio file");
    }
    return;
  }

  stop();
  M5.Speaker.setVolume(volume_percent_);
  if (!M5.Speaker.playWav(buffer, file_size, /*repeat=*/1, /*channel=*/-1,
                           /*stop_current=*/true)) {
    if (on_error_) {
      on_error_("Could not play audio file");
    }
  } else {
    playing_ = true;
  }
  free(buffer);
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
