#pragma once

#include "computer_output.h"

namespace echolocation {

class BleHidComputer : public ComputerOutput {
 public:
  void begin() override;
  void send_keypress(const HoldEvent& event) override;
};

}  // namespace echolocation
