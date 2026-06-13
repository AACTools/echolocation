#pragma once

#include <deque>
#include <string>

namespace echolocation {

enum class LogLevel {
  kDebug,
  kInfo,
  kWarn,
  kError,
};

struct LogEntry {
  LogLevel level;
  std::string message;
  uint32_t timestamp_ms;
};

class EventLogger {
 public:
  static EventLogger& instance();

  void set_enabled(bool enabled);
  void set_log_level(LogLevel level);
  void log(LogLevel level, const std::string& message);

  const std::deque<LogEntry>& recent_entries() const;

 private:
  EventLogger() = default;

  uint32_t now_ms() const;

  bool enabled_ = true;
  LogLevel level_ = LogLevel::kInfo;
  std::deque<LogEntry> entries_;
  size_t max_entries_ = 200;
};

}  // namespace echolocation
