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

void test_parse_text_only(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(keyConfigParseLine("arrow_up text=Next", name, sizeof(name),
                                      &behavior));
  TEST_ASSERT_EQUAL_STRING("arrow_up", name);
  TEST_ASSERT_TRUE(behavior.echo_enabled);
  TEST_ASSERT_TRUE(behavior.hold_enabled);
  TEST_ASSERT_EQUAL_STRING("Next", behavior.display_text);
  TEST_ASSERT_EQUAL_STRING("", behavior.audio_file);
}

void test_parse_audio_only(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(keyConfigParseLine("arrow_up audio=next.wav", name,
                                      sizeof(name), &behavior));
  TEST_ASSERT_EQUAL_STRING("next.wav", behavior.audio_file);
  TEST_ASSERT_EQUAL_STRING("", behavior.display_text);
}

void test_parse_all_optional_params(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(keyConfigParseLine(
      "arrow_up echo=off hold=off text=Next audio=next.wav", name, sizeof(name),
      &behavior));
  TEST_ASSERT_EQUAL_STRING("arrow_up", name);
  TEST_ASSERT_FALSE(behavior.echo_enabled);
  TEST_ASSERT_FALSE(behavior.hold_enabled);
  TEST_ASSERT_EQUAL_STRING("Next", behavior.display_text);
  TEST_ASSERT_EQUAL_STRING("next.wav", behavior.audio_file);
}

void test_parse_rejects_empty_text(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_FALSE(
      keyConfigParseLine("arrow_up text=", name, sizeof(name), &behavior));
}

void test_parse_quoted_text_with_spaces(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(keyConfigParseLine(
      "backspace hold=off text=\"Delete Letter\" audio=delete_letter.wav", name,
      sizeof(name), &behavior));
  TEST_ASSERT_EQUAL_STRING("backspace", name);
  TEST_ASSERT_FALSE(behavior.hold_enabled);
  TEST_ASSERT_TRUE(behavior.echo_enabled);
  TEST_ASSERT_EQUAL_STRING("Delete Letter", behavior.display_text);
  TEST_ASSERT_EQUAL_STRING("delete_letter.wav", behavior.audio_file);
}

void test_parse_unquoted_text_still_works(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_TRUE(
      keyConfigParseLine("right_alt text=Home", name, sizeof(name), &behavior));
  TEST_ASSERT_EQUAL_STRING("Home", behavior.display_text);
}

void test_parse_rejects_unclosed_quote(void) {
  char name[24];
  KeyBehavior behavior;
  TEST_ASSERT_FALSE(keyConfigParseLine("backspace text=\"Delete Letter", name,
                                       sizeof(name), &behavior));
}

void test_audio_basename_strips_wav_suffix(void) {
  KeyBehavior behavior;
  strncpy(behavior.audio_file, "next.wav", sizeof(behavior.audio_file) - 1);
  behavior.audio_file[sizeof(behavior.audio_file) - 1] = '\0';
  char basename[32];
  keyConfigAudioBasename(behavior, basename, sizeof(basename));
  TEST_ASSERT_EQUAL_STRING("next", basename);
}

void test_audio_basename_accepts_name_without_extension(void) {
  KeyBehavior behavior;
  strncpy(behavior.audio_file, "next", sizeof(behavior.audio_file) - 1);
  behavior.audio_file[sizeof(behavior.audio_file) - 1] = '\0';
  char basename[32];
  keyConfigAudioBasename(behavior, basename, sizeof(basename));
  TEST_ASSERT_EQUAL_STRING("next", basename);
}

void test_override_summary_text_and_audio(void) {
  KeyBehavior behavior;
  behavior.echo_enabled = false;
  behavior.hold_enabled = false;
  strncpy(behavior.display_text, "Next", sizeof(behavior.display_text) - 1);
  behavior.display_text[sizeof(behavior.display_text) - 1] = '\0';
  strncpy(behavior.audio_file, "next.wav", sizeof(behavior.audio_file) - 1);
  behavior.audio_file[sizeof(behavior.audio_file) - 1] = '\0';
  char summary[80];
  keyConfigFormatOverrideSummary(behavior, summary, sizeof(summary));
  TEST_ASSERT_EQUAL_STRING("echo off, hold off, text Next, audio next.wav",
                           summary);
}

void test_override_summary_text_only(void) {
  KeyBehavior behavior;
  strncpy(behavior.display_text, "Next", sizeof(behavior.display_text) - 1);
  behavior.display_text[sizeof(behavior.display_text) - 1] = '\0';
  char summary[80];
  keyConfigFormatOverrideSummary(behavior, summary, sizeof(summary));
  TEST_ASSERT_EQUAL_STRING("text Next", summary);
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

void test_entry_summary_uses_ascii_dash(void) {
  KeyConfigEntry entry;
  strncpy(entry.name, "arrow_down", sizeof(entry.name) - 1);
  entry.name[sizeof(entry.name) - 1] = '\0';
  entry.behavior.echo_enabled = false;
  entry.behavior.hold_enabled = false;
  char line[64];
  keyConfigFormatEntrySummary(entry, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING("arrow_down - echo off, hold off", line);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parse_space_passthrough);
  RUN_TEST(test_parse_case_insensitive_name);
  RUN_TEST(test_parse_ignores_comments_and_blanks);
  RUN_TEST(test_parse_rejects_all_defaults);
  RUN_TEST(test_parse_text_only);
  RUN_TEST(test_parse_audio_only);
  RUN_TEST(test_parse_all_optional_params);
  RUN_TEST(test_parse_rejects_empty_text);
  RUN_TEST(test_parse_quoted_text_with_spaces);
  RUN_TEST(test_parse_unquoted_text_still_works);
  RUN_TEST(test_parse_rejects_unclosed_quote);
  RUN_TEST(test_token_space);
  RUN_TEST(test_token_letter_a);
  RUN_TEST(test_token_left_shift_modifier);
  RUN_TEST(test_override_summary_both_off);
  RUN_TEST(test_override_summary_echo_only);
  RUN_TEST(test_override_summary_text_and_audio);
  RUN_TEST(test_override_summary_text_only);
  RUN_TEST(test_audio_basename_strips_wav_suffix);
  RUN_TEST(test_audio_basename_accepts_name_without_extension);
  RUN_TEST(test_entry_summary_uses_ascii_dash);
  return UNITY_END();
}
