#include "event_logger.h"

#include <chrono>
#include <iostream>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace echolocation {

EventLogger& EventLogger::instance() {
  static EventLogger logger;
  return logger;
}

void EventLogger::set_enabled(bool enabled) {
  enabled_ = enabled;
}

void EventLogger::set_log_level(LogLevel level) {
  level_ = level;
}

void EventLogger::log(LogLevel level, const std::string& message) {
  if (!enabled_ || level < level_) {
    return;
  }

  LogEntry entry{level, message, now_ms()};
  entries_.push_back(entry);
  if (entries_.size() > max_entries_) {
    entries_.pop_front();
  }

#ifdef ARDUINO
  Serial.printf("[%lu] %s\n", static_cast<unsigned long>(entry.timestamp_ms),
                entry.message.c_str());
#else
  std::cout << "[" << entry.timestamp_ms << "] " << entry.message << "\n";
#endif
}

const std::deque<LogEntry>& EventLogger::recent_entries() const {
  return entries_;
}

uint32_t EventLogger::now_ms() const {
#ifdef ARDUINO
  return millis();
#else
  auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
#endif
}

}  // namespace echolocation
