#include "app.h"
#include "event_logger.h"

#ifdef ARDUINO
#include <M5Unified.h>
#endif

using echolocation::App;
using echolocation::EventLogger;
using echolocation::LogLevel;

App app;

void setup() {
#ifdef ARDUINO
  auto config = M5.config();
  M5.begin(config);
  Serial.begin(115200);
#endif
  EventLogger::instance().set_log_level(LogLevel::kInfo);
  EventLogger::instance().log(LogLevel::kInfo, "Booting echolocation.");
  app.begin();
}

void loop() {
#ifdef ARDUINO
  M5.update();
#endif
  app.loop();
}
