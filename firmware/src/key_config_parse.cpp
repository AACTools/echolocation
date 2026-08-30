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

  const char* value = eq + 1;
  bool enabled = true;
  if (!parseOnOffValue(value, &enabled)) {
    return false;
  }

  if (equalsIgnoreCase(key, "echo")) {
    behavior->echo_enabled = enabled;
    return true;
  }
  if (equalsIgnoreCase(key, "hold")) {
    behavior->hold_enabled = enabled;
    return true;
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

    const char* start = p;
    while (*p != '\0' && !isspace(static_cast<unsigned char>(*p)) && *p != '#') {
      ++p;
    }

    char token[32];
    const size_t token_len = static_cast<size_t>(p - start);
    if (token_len == 0 || token_len >= sizeof(token)) {
      return false;
    }
    memcpy(token, start, token_len);
    token[token_len] = '\0';

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
  return !behavior.echo_enabled || !behavior.hold_enabled;
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
    strncpy(out, "echo off", out_len - 1);
    out[out_len - 1] = '\0';
    first = false;
  }
  if (!behavior.hold_enabled) {
    if (first) {
      strncpy(out, "hold off", out_len - 1);
    } else {
      strncat(out, ", hold off", out_len - strlen(out) - 1);
    }
    out[out_len - 1] = '\0';
  }
}

void keyConfigFormatEntrySummary(const KeyConfigEntry& entry, char* out,
                                 size_t out_len) {
  if (out == nullptr || out_len == 0) {
    return;
  }

  char summary[48];
  keyConfigFormatOverrideSummary(entry.behavior, summary, sizeof(summary));
  snprintf(out, out_len, "%s — %s", entry.name, summary);
}
