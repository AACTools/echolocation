#include <unity.h>

#include "key_config.h"
#include "key_config_parse.h"
#include "keyboard_layout.h"

#include <string.h>

void setUp(void) {
  keyboardLayoutSetDetected(KeyboardDetectedLayout::kUnknown);
}

void tearDown(void) {}

void test_parse_space_passthrough(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(
      keyConfigParseLine("space echo=off hold=off", name, sizeof(name), &behavior));
  TEST_ASSERT_EQUAL_STRING("space", name);
  TEST_ASSERT_FALSE(behavior.echo_enabled);
  TEST_ASSERT_FALSE(behavior.hold_enabled);
}

void test_parse_case_insensitive_name(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(
      keyConfigParseLine("SPACE echo=off", name, sizeof(name), &behavior));
  TEST_ASSERT_EQUAL_STRING("SPACE", name);
  TEST_ASSERT_FALSE(behavior.echo_enabled);
  TEST_ASSERT_TRUE(behavior.hold_enabled);
}

void test_parse_ignores_comments_and_blanks(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_FALSE(keyConfigParseLine("# comment", name, sizeof(name), &behavior));
  TEST_ASSERT_FALSE(keyConfigParseLine("", name, sizeof(name), &behavior));
  TEST_ASSERT_FALSE(keyConfigParseLine("   ", name, sizeof(name), &behavior));
}

void test_parse_rejects_all_defaults(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_FALSE(
      keyConfigParseLine("space echo=on hold=on", name, sizeof(name), &behavior));
}

void test_token_space(void) {
  uint8_t key = 0;
  uint8_t mod_bit = 0;
  TEST_ASSERT_TRUE(keyboardLayoutResolveToken("space", &key, &mod_bit));
  TEST_ASSERT_EQUAL_UINT8(0x2C, key);
  TEST_ASSERT_EQUAL_UINT8(0, mod_bit);
}

void test_token_letter_a(void) {
  uint8_t key = 0;
  uint8_t mod_bit = 0;
  TEST_ASSERT_TRUE(keyboardLayoutResolveToken("a", &key, &mod_bit));
  TEST_ASSERT_EQUAL_UINT8(0x04, key);
}

void test_token_left_shift_modifier(void) {
  uint8_t key = 0;
  uint8_t mod_bit = 0;
  TEST_ASSERT_TRUE(keyboardLayoutResolveToken("left_shift", &key, &mod_bit));
  TEST_ASSERT_EQUAL_UINT8(0, key);
  TEST_ASSERT_EQUAL_UINT8(0x02, mod_bit);
}

void test_override_summary_both_off(void) {
  KeyBehavior behavior;
  behavior.echo_enabled = false;
  behavior.hold_enabled = false;
  char summary[48];
  keyConfigFormatOverrideSummary(behavior, summary, sizeof(summary));
  TEST_ASSERT_EQUAL_STRING("echo off, hold off", summary);
}

void test_override_summary_echo_only(void) {
  KeyBehavior behavior;
  behavior.echo_enabled = false;
  behavior.hold_enabled = true;
  char summary[48];
  keyConfigFormatOverrideSummary(behavior, summary, sizeof(summary));
  TEST_ASSERT_EQUAL_STRING("echo off", summary);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parse_space_passthrough);
  RUN_TEST(test_parse_case_insensitive_name);
  RUN_TEST(test_parse_ignores_comments_and_blanks);
  RUN_TEST(test_parse_rejects_all_defaults);
  RUN_TEST(test_token_space);
  RUN_TEST(test_token_letter_a);
  RUN_TEST(test_token_left_shift_modifier);
  RUN_TEST(test_override_summary_both_off);
  RUN_TEST(test_override_summary_echo_only);
  return UNITY_END();
}
