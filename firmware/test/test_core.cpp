#include <unity.h>

#include "echolocation_core/hold_detector.h"
#include "event_logger.h"
#include "layout_mapper.h"

using echolocation::HoldDetector;
using echolocation::HoldEvent;
using echolocation::KeyEvent;
using echolocation::LayoutMapper;
using echolocation::LayoutResult;
using echolocation::LayoutType;
using echolocation::EventLogger;
using echolocation::LogLevel;

void test_hold_detector_emits_once() {
  HoldDetector detector(100);
  int hold_count = 0;
  detector.set_hold_callback([&hold_count](const HoldEvent&) { hold_count++; });

  KeyEvent down{0x04, 0, true, 0};
  detector.process_key_event(down);
  detector.update(150);
  detector.update(250);
  TEST_ASSERT_EQUAL(1, hold_count);

  KeyEvent up{0x04, 0, false, 300};
  detector.process_key_event(up);
}

void test_layout_mapper_us_letter() {
  LayoutMapper mapper(LayoutType::kUS);
  KeyEvent event{0x04, 0, true, 0};
  LayoutResult result = mapper.map_key(event);
  TEST_ASSERT_EQUAL_STRING("a", result.spoken_token.c_str());
  TEST_ASSERT_FALSE(result.used_fallback);
}

void test_layout_mapper_fallback() {
  LayoutMapper mapper(LayoutType::kUnknown);
  KeyEvent event{0x99, 0, true, 0};
  LayoutResult result = mapper.map_key(event);
  TEST_ASSERT_TRUE(result.used_fallback);
}

void test_event_logger_disable() {
  auto& logger = EventLogger::instance();
  logger.set_enabled(false);
  const size_t before = logger.recent_entries().size();
  logger.log(LogLevel::kInfo, "should not log");
  TEST_ASSERT_EQUAL(before, logger.recent_entries().size());
  logger.set_enabled(true);
}

void setup() {}

void loop() {
  UNITY_BEGIN();
  RUN_TEST(test_hold_detector_emits_once);
  RUN_TEST(test_layout_mapper_us_letter);
  RUN_TEST(test_layout_mapper_fallback);
  RUN_TEST(test_event_logger_disable);
  UNITY_END();
}
