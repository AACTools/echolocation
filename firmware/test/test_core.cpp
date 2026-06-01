#include <unity.h>

#include "hold_detector.h"
#include "key_event.h"
#include "settings_model.h"

using namespace echo;

static int tap_count = 0;
static HoldTapAction last_tap{};

static void onTap(const HoldTapAction& action) {
  ++tap_count;
  last_tap = action;
}

void setUp(void) {
  tap_count = 0;
  last_tap = {};
}

void tearDown(void) {}

void test_hold_emits_once_after_duration(void) {
  HoldDetector detector;
  detector.setHoldDurationMs(500);
  detector.setTapCallback(onTap);

  KeyEvent press;
  press.hid_usage = 0x04;
  press.modifier_mask = 0;
  press.pressed = true;
  press.timestamp_ms = 1000;

  detector.onKeyEvent(press, 1000);
  TEST_ASSERT_EQUAL(0, tap_count);

  detector.tick(1500);
  TEST_ASSERT_EQUAL(1, tap_count);
  TEST_ASSERT_EQUAL(0x04, last_tap.hid_usage);

  detector.onKeyEvent(KeyEvent{0x04, 0, false, 1600}, 1600);
  detector.tick(2000);
  TEST_ASSERT_EQUAL(1, tap_count);
}

void test_release_resets_hold_state(void) {
  HoldDetector detector;
  detector.setHoldDurationMs(200);
  detector.setTapCallback(onTap);

  KeyEvent press{0x05, 0, true, 0};
  detector.onKeyEvent(press, 0);
  detector.tick(250);
  TEST_ASSERT_EQUAL(1, tap_count);

  KeyEvent release{0x05, 0, false, 300};
  detector.onKeyEvent(release, 300);

  press.timestamp_ms = 400;
  detector.onKeyEvent(press, 400);
  detector.tick(700);
  TEST_ASSERT_EQUAL(2, tap_count);
}

void test_default_settings_are_valid(void) {
  DeviceSettings settings = defaultDeviceSettings();
  TEST_ASSERT_TRUE(isValidVolume(settings.volume_percent));
  TEST_ASSERT_TRUE(isValidHoldDuration(settings.hold_duration_ms));
}

void test_clamp_fixes_invalid_values(void) {
  DeviceSettings settings = defaultDeviceSettings();
  settings.volume_percent = 200;
  settings.hold_duration_ms = 50;
  clampDeviceSettings(settings);
  TEST_ASSERT_EQUAL(80, settings.volume_percent);
  TEST_ASSERT_EQUAL(500, settings.hold_duration_ms);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_hold_emits_once_after_duration);
  RUN_TEST(test_release_resets_hold_state);
  RUN_TEST(test_default_settings_are_valid);
  RUN_TEST(test_clamp_fixes_invalid_values);
  return UNITY_END();
}
