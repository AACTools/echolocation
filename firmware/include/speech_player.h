#pragma once

#include <string>

#include "audio_router.h"

namespace echolocation {

class SpeechPlayer {
 public:
  void begin(AudioRouter* router);
  void set_volume(uint8_t volume);
  void play_token(const std::string& token);
  void stop();

 private:
  std::string current_token_;
  AudioRouter* router_ = nullptr;
  uint8_t volume_ = 80;
};

}  // namespace echolocation
