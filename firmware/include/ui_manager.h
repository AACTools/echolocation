#pragma once

#include <string>

#include "event_logger.h"

namespace echolocation {

class UIManager {
 public:
  void begin();
  void update_current_key(const std::string& key_name);
  void update_battery(int percent);
  void show_error(const std::string& message);
  void show_debug_page(const std::deque<LogEntry>& logs);
};

}  // namespace echolocation
