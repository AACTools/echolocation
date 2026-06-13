#pragma once

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <cstdarg>
#include <cstdio>
#endif

namespace echo {

#ifndef NATIVE_TEST
inline void latencyLog(const char* tag, const char* fmt, ...) {
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  std::vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  Serial.printf("[%lu][%s] %s\n", millis(), tag, buffer);
}
#else
inline void latencyLog(const char*, const char*, ...) {}
#endif

}  // namespace echo
