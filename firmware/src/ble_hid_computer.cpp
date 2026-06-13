#include "ble_hid_computer.h"

#include "event_logger.h"

namespace echolocation {

void BleHidComputer::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "BLE HID computer output started.");
}

void BleHidComputer::send_keypress(const HoldEvent& event) {
  EventLogger::instance().log(
      LogLevel::kInfo,
      "BLE HID send usage=" + std::to_string(event.usage) +
          " held=" + std::to_string(event.held_ms) + "ms");
  // TODO: send BLE HID keypress to host computer.
}

}  // namespace echolocation
