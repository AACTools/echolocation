#include "usb_hid_computer.h"

#include "event_logger.h"

namespace echolocation {

void UsbHidComputer::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "USB HID computer output started.");
}

void UsbHidComputer::send_keypress(const HoldEvent& event) {
  EventLogger::instance().log(
      LogLevel::kInfo,
      "USB HID send usage=" + std::to_string(event.usage) +
          " held=" + std::to_string(event.held_ms) + "ms");
  // TODO: send HID keypress to host computer.
}

}  // namespace echolocation
