#ifndef POKEBW2_DWC_GSDEBUG_H
#define POKEBW2_DWC_GSDEBUG_H

#include "types.h"
#include <stdarg.h>

// GameSpy's debug output (gsDebug.c), in overlay 11. A message has a category, a type and a level (bits); it is shown
// when the table of levels has its level for its category and type, through the callback that the game set or else
// on the screen (gs_screen_print.c)

typedef void (*GSIDebugCallback)(int category, int type, int level, const char *format, va_list args);

void gsDebugVaList(int category, int type, int level, const char *format, va_list args);
void gsDebugFormat(int category, int type, int level, const char *format, ...);
// Shows size bytes, 16 to a line, as hexadecimal digits and as characters
void gsDebugBinary(int category, int type, int level, const u8 *data, int size);

#endif // POKEBW2_DWC_GSDEBUG_H
