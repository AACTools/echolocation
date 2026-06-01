#pragma once

#include <cstdint>
#include <functional>

namespace echo {

using SpeechErrorCallback = std::function<void(const char* message)>;

class SpeechPlayer {
 public:
  void begin(SpeechErrorCallback on_error);
  void setVolumePercent(uint8_t volume_percent);
  void speakKey(uint8_t hid_usage);
  void stop();
  void tick();
  bool isPlaying() const;

 private:
  SpeechErrorCallback on_error_;
  uint8_t volume_percent_ = 80;
  bool playing_ = false;
};

}  // namespace echo
