#pragma once

#include <cstdint>

namespace echo {

class AudioRouter {
 public:
  void begin();
  void tick();
  bool isExternalOutputActive() const;

 private:
  bool external_active_ = false;
  uint32_t last_poll_ms_ = 0;
  static constexpr uint8_t kModuleAudioStm32Address = 0x33;
  static constexpr uint32_t kPollIntervalMs = 100;
};

}  // namespace echo
