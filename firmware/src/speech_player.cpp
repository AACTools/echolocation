#include "speech_player.h"

#include "event_logger.h"

namespace echolocation {

void SpeechPlayer::begin(AudioRouter* router) {
  router_ = router;
}

void SpeechPlayer::set_volume(uint8_t volume) {
  volume_ = volume;
  EventLogger::instance().log(LogLevel::kInfo,
                              "Volume set to " + std::to_string(volume_));
}

void SpeechPlayer::play_token(const std::string& token) {
  if (token == current_token_) {
    return;
  }
  stop();
  current_token_ = token;
  EventLogger::instance().log(LogLevel::kInfo, "Play token: " + token);
  // TODO: stream WAV from microSD using ES8388.
}

void SpeechPlayer::stop() {
  if (current_token_.empty()) {
    return;
  }
  EventLogger::instance().log(LogLevel::kInfo, "Stop token: " + current_token_);
  current_token_.clear();
}

}  // namespace echolocation
