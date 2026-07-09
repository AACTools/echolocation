#pragma once

#include <stdbool.h>

bool bleStackEnsureInit(const char* device_name);
bool bleStackIsInitialized();
