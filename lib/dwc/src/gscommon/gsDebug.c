#include "dwc/gsDebug.h"
#include "dwc/ext.h"

// GameSpy's debug output (gsDebug.c). The ROM names neither the file nor its functions: gsDebugVaList, gsDebugFormat,
// gsDebugBinary, HexEncode16 and gGSIDebugInstance are the later SDK's names, and gsiDebugPrintToConsole is ours

// The output: the game's callback, whether the lock was made, the lock, and the levels to show of each category and
// type (13 categories of 5 types)
typedef struct GSIDebugInstance {
    GSIDebugCallback callback;
    gsi_bool initialized;
    GSICriticalSection crit;
    gsi_u8 levels[13][5];
} GSIDebugInstance;

static void gsiDebugPrintToConsole(int category, int type, int level, const char *format, va_list args);
static void HexEncode16(const gsi_u8 *data, char *line, int size);

static GSIDebugInstance gGSIDebugInstance;

static void gsiDebugPrintToConsole(int category, int type, int level, const char *format, va_list args) {
    ScreenVPrintf(format, args);
}

void gsDebugVaList(int category, int type, int level, const char *format, va_list args) {
    if (!gGSIDebugInstance.initialized) {
        gGSIDebugInstance.initialized = gsi_true;
        gsiInitializeCriticalSection(&gGSIDebugInstance.crit);
    }
    gsiEnterCriticalSection(&gGSIDebugInstance.crit);
    if (gGSIDebugInstance.levels[category][type] & level) {
        if (gGSIDebugInstance.callback != NULL) {
            gGSIDebugInstance.callback(category, type, level, format, args);
        } else {
            gsiDebugPrintToConsole(category, type, level, format, args);
        }
    }
    gsiLeaveCriticalSection(&gGSIDebugInstance.crit);
}

void gsDebugFormat(int category, int type, int level, const char *format, ...) {
    va_list args;

    va_start(args, format);
    gsDebugVaList(category, type, level, format, args);
    va_end(args);
}

// Writes up to 16 bytes as a line: pairs of hexadecimal digits, then the bytes as characters from column 41
static void HexEncode16(const gsi_u8 *data, char *line, int size) {
    char *chars = line + 41;
    int odd = size & 1;

    memset(line, ' ', 0x40);
    while (size--) {
        gsi_u8 c = *data++;

        OS_SPrintf(line, "%02X", c);
        if (isgraph(c)) {
            *chars = c;
        } else {
            *chars = '.';
        }
        chars++;
        line += 2;
        if ((size & 1) == odd) {
            *line++ = ' ';
        }
    }
    *line = ' ';
    *chars = '\0';
}

void gsDebugBinary(int category, int type, int level, const gsi_u8 *data, int size) {
    int left = size;
    char line[0x50];

    while (left > 0) {
        int count = left < 16 ? left : 16;

        HexEncode16(data, line, count);
        gsDebugFormat(category, type, level, "  %s\r\n", line);
        data += count;
        left -= count;
    }
}
