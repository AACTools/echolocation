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
  static constexpr uint8_t kModuleAudioStm32Address = 0x33;
};

}  // namespace echo
