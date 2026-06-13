#include "audio_router.h"

#include "event_logger.h"

namespace echolocation {

void AudioRouter::begin() {
  apply_route(current_route_);
}

void AudioRouter::set_external_connected(bool connected) {
  const AudioOutputRoute target =
      connected ? AudioOutputRoute::kExternalJack : AudioOutputRoute::kInternalSpeaker;
  if (target == current_route_) {
    return;
  }
  apply_route(target);
}

AudioOutputRoute AudioRouter::route() const {
  return current_route_;
}

void AudioRouter::apply_route(AudioOutputRoute route) {
  current_route_ = route;
  if (route == AudioOutputRoute::kExternalJack) {
    EventLogger::instance().log(LogLevel::kInfo, "Audio route: external jack.");
  } else {
    EventLogger::instance().log(LogLevel::kInfo, "Audio route: internal speaker.");
  }
}

}  // namespace echolocation
