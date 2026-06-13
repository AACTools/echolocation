#include "ui_manager.h"

namespace echolocation {

void UIManager::begin() {
  EventLogger::instance().log(LogLevel::kInfo, "UI initialized.");
}

void UIManager::update_current_key(const std::string& key_name) {
  EventLogger::instance().log(LogLevel::kDebug, "UI key: " + key_name);
}

void UIManager::update_battery(int percent) {
  EventLogger::instance().log(LogLevel::kDebug,
                              "Battery: " + std::to_string(percent) + "%");
}

void UIManager::show_error(const std::string& message) {
  EventLogger::instance().log(LogLevel::kError, "UI error: " + message);
}

void UIManager::show_debug_page(const std::deque<LogEntry>& logs) {
  EventLogger::instance().log(LogLevel::kInfo,
                              "Debug page logs: " + std::to_string(logs.size()));
}

}  // namespace echolocation
