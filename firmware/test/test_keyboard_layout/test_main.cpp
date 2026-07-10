#include <unity.h>

#include "keyboard_layout.h"

#include <string.h>

void setUp(void) {
  keyboardLayoutSetDetected(KeyboardDetectedLayout::kUnknown);
}

void tearDown(void) {}

static void expectKey(uint8_t mod, uint8_t key, KeyboardDetectedLayout layout,
                      const char* expected_token, const char* expected_display) {
  keyboardLayoutSetDetected(layout);
  KeyLabel label;
  TEST_ASSERT_TRUE(keyboardLayoutResolveKey(mod, key, &label));
  TEST_ASSERT_EQUAL_STRING(expected_token, label.speech_token);
  TEST_ASSERT_EQUAL_STRING(expected_display, label.display);
}

void test_us_letter_unshifted(void) {
  expectKey(0, 0x04, KeyboardDetectedLayout::kUs, "a", "a");
}

void test_us_letter_shifted(void) {
  expectKey(0x02, 0x04, KeyboardDetectedLayout::kUs, "a", "A");
}

void test_us_digit_shifted_exclamation(void) {
  expectKey(0x02, 0x1E, KeyboardDetectedLayout::kUs, "exclamation", "!");
}

void test_us_named_enter(void) {
  expectKey(0, 0x28, KeyboardDetectedLayout::kUs, "enter", "Enter");
}

void test_uk_shift_three_pound(void) {
  expectKey(0x02, 0x20, KeyboardDetectedLayout::kUk, "pound", "\xC2\xA3");
}

void test_uk_shift_two_double_quote(void) {
  expectKey(0x02, 0x1F, KeyboardDetectedLayout::kUk, "double_quote", "\"");
}

void test_physical_unknown_layout(void) {
  keyboardLayoutSetDetected(KeyboardDetectedLayout::kUnknown);
  KeyLabel label;
  TEST_ASSERT_TRUE(keyboardLayoutResolveKey(0, 0x2F, &label));
  TEST_ASSERT_EQUAL_STRING("key_left_bracket", label.speech_token);
  TEST_ASSERT_EQUAL_STRING("[", label.display);
}

void test_modifier_left_shift(void) {
  KeyLabel label;
  TEST_ASSERT_TRUE(keyboardLayoutResolveModifier(0x02, &label));
  TEST_ASSERT_EQUAL_STRING("left_shift", label.speech_token);
  TEST_ASSERT_EQUAL_STRING("LShift", label.display);
}

void test_usb_country_us(void) {
  keyboardLayoutApplyUsbCountryCode(33);
  TEST_ASSERT_EQUAL(KeyboardDetectedLayout::kUs, keyboardLayoutGetDetected());
}

void test_usb_country_uk(void) {
  keyboardLayoutApplyUsbCountryCode(32);
  TEST_ASSERT_EQUAL(KeyboardDetectedLayout::kUk, keyboardLayoutGetDetected());
}

void test_auto_uses_detected_us(void) {
  keyboardLayoutSetDetected(KeyboardDetectedLayout::kUs);
  KeyLabel label;
  TEST_ASSERT_TRUE(keyboardLayoutResolveKey(0, 0x2D, &label));
  TEST_ASSERT_EQUAL_STRING("minus", label.speech_token);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_us_letter_unshifted);
  RUN_TEST(test_us_letter_shifted);
  RUN_TEST(test_us_digit_shifted_exclamation);
  RUN_TEST(test_us_named_enter);
  RUN_TEST(test_uk_shift_three_pound);
  RUN_TEST(test_uk_shift_two_double_quote);
  RUN_TEST(test_physical_unknown_layout);
  RUN_TEST(test_modifier_left_shift);
  RUN_TEST(test_usb_country_us);
  RUN_TEST(test_usb_country_uk);
  RUN_TEST(test_auto_uses_detected_us);
  return UNITY_END();
}
