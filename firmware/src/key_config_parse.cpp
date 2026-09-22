#include "key_config_parse.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

namespace {

const char* skipWhitespace(const char* p) {
  while (*p != '\0' && isspace(static_cast<unsigned char>(*p))) {
    ++p;
  }
  return p;
}

bool equalsIgnoreCase(const char* a, const char* b) {
  if (a == nullptr || b == nullptr) {
    return false;
  }
  while (*a != '\0' && *b != '\0') {
    if (tolower(static_cast<unsigned char>(*a)) !=
        tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parseOnOffValue(const char* value, bool* out_enabled) {
  if (value == nullptr || out_enabled == nullptr) {
    return false;
  }
  if (equalsIgnoreCase(value, "on") || equalsIgnoreCase(value, "1") ||
      equalsIgnoreCase(value, "true") || equalsIgnoreCase(value, "yes")) {
    *out_enabled = true;
    return true;
  }
  if (equalsIgnoreCase(value, "off") || equalsIgnoreCase(value, "0") ||
      equalsIgnoreCase(value, "false") || equalsIgnoreCase(value, "no")) {
    *out_enabled = false;
    return true;
  }
  return false;
}

bool copyBoundedValue(const char* value, char* out, size_t out_len) {
  if (value == nullptr || value[0] == '\0' || out == nullptr || out_len == 0) {
    return false;
  }
  if (strlen(value) >= out_len) {
    return false;
  }
  memcpy(out, value, strlen(value) + 1);
  return true;
}

void appendSummaryPart(char* out, size_t out_len, bool* first, const char* part) {
  if (out == nullptr || out_len == 0 || first == nullptr || part == nullptr) {
    return;
  }
  if (*first) {
    strncpy(out, part, out_len - 1);
    out[out_len - 1] = '\0';
    *first = false;
    return;
  }
  strncat(out, ", ", out_len - strlen(out) - 1);
  strncat(out, part, out_len - strlen(out) - 1);
}

bool unquoteValue(const char* value, char* out, size_t out_len) {
  if (value == nullptr || out == nullptr || out_len == 0) {
    return false;
  }
  if (value[0] != '"') {
    if (strlen(value) >= out_len) {
      return false;
    }
    memcpy(out, value, strlen(value) + 1);
    return true;
  }

  const size_t vlen = strlen(value);
  if (vlen < 2 || value[vlen - 1] != '"') {
    return false;
  }
  const size_t inner_len = vlen - 2;
  if (inner_len >= out_len) {
    return false;
  }
  memcpy(out, value + 1, inner_len);
  out[inner_len] = '\0';
  return true;
}

bool readSettingToken(const char** cursor, char* token, size_t token_len) {
  if (cursor == nullptr || *cursor == nullptr || token == nullptr ||
      token_len == 0) {
    return false;
  }

  const char* p = *cursor;
  const char* start = p;

  while (*p != '\0' && *p != '=' && *p != '#' &&
         !isspace(static_cast<unsigned char>(*p))) {
    ++p;
  }
  if (*p != '=') {
    return false;
  }
  ++p;

  if (*p == '"') {
    ++p;
    while (*p != '\0' && *p != '"') {
      ++p;
    }
    if (*p != '"') {
      return false;
    }
    ++p;
  } else {
    while (*p != '\0' && *p != '#' && !isspace(static_cast<unsigned char>(*p))) {
      ++p;
    }
  }

  const size_t len = static_cast<size_t>(p - start);
  if (len == 0 || len >= token_len) {
    return false;
  }
  memcpy(token, start, len);
  token[len] = '\0';
  *cursor = p;
  return true;
}

bool parseSettingToken(const char* token, KeyBehavior* behavior) {
  if (token == nullptr || behavior == nullptr) {
    return false;
  }

  const char* eq = strchr(token, '=');
  if (eq == nullptr) {
    return false;
  }

  char key[16];
  const size_t key_len = static_cast<size_t>(eq - token);
  if (key_len == 0 || key_len >= sizeof(key)) {
    return false;
  }
  memcpy(key, token, key_len);
  key[key_len] = '\0';

  char value[64];
  if (!unquoteValue(eq + 1, value, sizeof(value))) {
    return false;
  }

  if (equalsIgnoreCase(key, "echo")) {
    bool enabled = true;
    if (!parseOnOffValue(value, &enabled)) {
      return false;
    }
    behavior->echo_enabled = enabled;
    return true;
  }
  if (equalsIgnoreCase(key, "hold")) {
    bool enabled = true;
    if (!parseOnOffValue(value, &enabled)) {
      return false;
    }
    behavior->hold_enabled = enabled;
    return true;
  }
  if (equalsIgnoreCase(key, "text")) {
    return copyBoundedValue(value, behavior->display_text,
                            sizeof(behavior->display_text));
  }
  if (equalsIgnoreCase(key, "audio")) {
    return copyBoundedValue(value, behavior->audio_file,
                            sizeof(behavior->audio_file));
  }
  return false;
}

}  // namespace

bool keyConfigParseLine(const char* line, char* out_name, size_t out_name_len,
                        KeyBehavior* out_behavior) {
  if (line == nullptr || out_name == nullptr || out_name_len == 0 ||
      out_behavior == nullptr) {
    return false;
  }

  const char* p = skipWhitespace(line);
  if (*p == '\0' || *p == '#') {
    return false;
  }

  size_t name_len = 0;
  while (p[name_len] != '\0' && !isspace(static_cast<unsigned char>(p[name_len]))) {
    ++name_len;
  }
  if (name_len == 0 || name_len >= out_name_len) {
    return false;
  }

  memcpy(out_name, p, name_len);
  out_name[name_len] = '\0';
  p += name_len;

  KeyBehavior behavior;
  bool saw_setting = false;

  while (true) {
    p = skipWhitespace(p);
    if (*p == '\0' || *p == '#') {
      break;
    }

    char token[64];
    if (!readSettingToken(&p, token, sizeof(token))) {
      return false;
    }

    if (!parseSettingToken(token, &behavior)) {
      return false;
    }
    saw_setting = true;
  }

  if (!saw_setting) {
    return false;
  }
  if (!keyConfigHasOverrides(behavior)) {
    return false;
  }

  *out_behavior = behavior;
  return true;
}

bool keyConfigHasOverrides(const KeyBehavior& behavior) {
  return !behavior.echo_enabled || !behavior.hold_enabled ||
         behavior.display_text[0] != '\0' || behavior.audio_file[0] != '\0';
}

void keyConfigFormatOverrideSummary(const KeyBehavior& behavior, char* out,
                                    size_t out_len) {
  if (out == nullptr || out_len == 0) {
    return;
  }

  out[0] = '\0';
  if (!keyConfigHasOverrides(behavior)) {
    return;
  }

  bool first = true;
  if (!behavior.echo_enabled) {
    appendSummaryPart(out, out_len, &first, "echo off");
  }
  if (!behavior.hold_enabled) {
    appendSummaryPart(out, out_len, &first, "hold off");
  }
  if (behavior.display_text[0] != '\0') {
    char part[32];
    snprintf(part, sizeof(part), "text %s", behavior.display_text);
    appendSummaryPart(out, out_len, &first, part);
  }
  if (behavior.audio_file[0] != '\0') {
    char part[40];
    snprintf(part, sizeof(part), "audio %s", behavior.audio_file);
    appendSummaryPart(out, out_len, &first, part);
  }
}

void keyConfigFormatEntrySummary(const KeyConfigEntry& entry, char* out,
                                 size_t out_len) {
  if (out == nullptr || out_len == 0) {
    return;
  }

  char summary[80];
  keyConfigFormatOverrideSummary(entry.behavior, summary, sizeof(summary));
  snprintf(out, out_len, "%s - %s", entry.name, summary);
}

void keyConfigAudioBasename(const KeyBehavior& behavior, char* out,
                            size_t out_len) {
  if (out == nullptr || out_len == 0) {
    return;
  }

  out[0] = '\0';
  if (behavior.audio_file[0] == '\0') {
    return;
  }

  const char* name = behavior.audio_file;
  const char* slash = strrchr(name, '/');
  if (slash != nullptr && slash[1] != '\0') {
    name = slash + 1;
  }

  strncpy(out, name, out_len - 1);
  out[out_len - 1] = '\0';

  const size_t len = strlen(out);
  if (len >= 4 && equalsIgnoreCase(out + len - 4, ".wav")) {
    out[len - 4] = '\0';
  }
}
