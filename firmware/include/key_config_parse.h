#pragma once

#include "key_config.h"

#include <stddef.h>

// Parses one line from keys.txt. Returns true if a valid override was found.
// out_name receives the key token from the line (e.g. "space").
bool keyConfigParseLine(const char* line, char* out_name, size_t out_name_len,
                        KeyBehavior* out_behavior);
