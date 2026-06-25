#pragma once

#include <stdbool.h>

void speakerDetectBegin();
bool speakerDetectPoll();
bool speakerDetectIsModulePresent();
bool speakerDetectIsExternalConnected();
