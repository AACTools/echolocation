#pragma once

#include <functional>
#include <string>

#include "key_event.h"

namespace echo {

class BleKeyboardSource {
 public:
  using KeyboardEventCallback = std::function<void(const KeyEvent&)>;

  void begin(KeyboardEventCallback callback);
  void tick(uint32_t now_ms);
  bool isKeyboardConnected() const;
  void startScan();
  void stopScan();
  bool isScanning() const;
  std::string statusMessage() const;
  void setConnected(bool connected);
  void setStatus(const char* message);
  void setScanning(bool scanning);
  void dispatchKeyEvent(const KeyEvent& event);

  KeyboardEventCallback callback_;

 private:
  bool connected_ = false;
  bool scanning_ = false;
  std::string status_message_ = "Bluetooth keyboard idle";
};

}  // namespace echo
