#pragma once

#include <string>

namespace echolocation {

enum class AudioOutputRoute {
  kInternalSpeaker,
  kExternalJack,
};

class AudioRouter {
 public:
  void begin();
  void set_external_connected(bool connected);
  AudioOutputRoute route() const;

 private:
  void apply_route(AudioOutputRoute route);

  AudioOutputRoute current_route_ = AudioOutputRoute::kInternalSpeaker;
};

}  // namespace echolocation
