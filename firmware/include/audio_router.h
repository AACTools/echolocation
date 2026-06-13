#pragma once

#include <cstdint>

namespace echo {

class AudioRouter {
 public:
  void begin();
  void tick();
  bool isExternalOutputActive() const;

 private:
#ifndef NATIVE_TEST
  bool tryReadStatus(uint8_t* status_out);
#endif

  bool external_active_ = false;
  bool module_present_ = false;
  bool read_disabled_ = false;
  bool address_acknowledged_ = false;
  uint32_t last_poll_ms_ = 0;
  static constexpr uint8_t kModuleAudioStm32Address = 0x33;
  static constexpr uint32_t kPollIntervalMs = 100;
  static constexpr uint32_t kAbsentProbeIntervalMs = 30000;
};

}  // namespace echo
