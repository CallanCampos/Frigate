#ifndef POKEBW2_DWC_GS_SCREEN_PRINT_H
#define POKEBW2_DWC_GS_SCREEN_PRINT_H

#include "types.h"
#include <stdarg.h>

// The screen that GameSpy's debug output prints to (gs_screen_print.c, a name of ours), in overlay 11, and the clock
// in seconds

// Prints a formatted text to the screen when printing is on, which it is not in the ROM
void ScreenVPrintf(const char *format, va_list args);
// The time in seconds, which it also writes to *seconds unless that is NULL
int gsiTimeInSec(int *seconds);

#endif // POKEBW2_DWC_GS_SCREEN_PRINT_H
