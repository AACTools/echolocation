#pragma once

#include "echolocation_core/hold_detector.h"

namespace echolocation {

class ComputerOutput {
 public:
  virtual ~ComputerOutput() = default;
  virtual void begin() = 0;
  virtual void send_keypress(const HoldEvent& event) = 0;
};

}  // namespace echolocation
