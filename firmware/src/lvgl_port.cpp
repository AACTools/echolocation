#include "lvgl_port.h"

#include "event_logger.h"

namespace echolocation {

void LvglPort::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "LVGL port initialized.");
}

void LvglPort::tick() {
  // TODO: call lv_timer_handler and input drivers.
}

}  // namespace echolocation
