#ifndef POKEBW2_DWC_SCREEN_H
#define POKEBW2_DWC_SCREEN_H

#include "types.h"
#include <stdarg.h>
#include <time.h>

// The screen that GameSpy's debug output prints to (screen.c, GameSpy's common/nitro/screen.c), in overlay 11, and the
// clock in seconds

// Prints a formatted text to the screen when printing is on, which it is not in the ROM
void VPrintf(const char *format, va_list args);
// The time in seconds, which it also writes to *seconds unless that is NULL
time_t gsiTimeInSec(time_t *seconds);

#endif // POKEBW2_DWC_SCREEN_H
