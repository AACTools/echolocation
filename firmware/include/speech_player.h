#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace echo {

using SpeechErrorCallback = std::function<void(const char* message)>;

class SpeechPlayer {
 public:
  void begin(SpeechErrorCallback on_error);
  void preloadFromSd();
  void setVolumePercent(uint8_t volume_percent);
  void speakKey(uint8_t hid_usage);
  void stop();
  void tick();
  bool isPlaying() const;

 private:
  struct CachedAudio {
    int16_t* pcm_data = nullptr;
    size_t sample_count = 0;
    uint32_t sample_rate = 16000;
    bool stereo = false;
  };

  static constexpr size_t kCacheCapacity = 0xE8 - 0x04;

  size_t cacheIndex(uint8_t hid_usage) const;
  bool loadIntoCache(uint8_t hid_usage);
  const CachedAudio* cachedAudio(uint8_t hid_usage) const;

  SpeechErrorCallback on_error_;
  CachedAudio cache_[kCacheCapacity]{};
  uint8_t volume_percent_ = 80;
  bool playing_ = false;
};

}  // namespace echo
