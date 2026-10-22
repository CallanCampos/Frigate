// NitroSDK OS: the formatted text functions of the OS (os_printf.c), which call libstd's text formatter.
#include "nitro/os.h"
#include "nitro/std.h"

int OS_SPrintf(char *dest, const char *format, ...) {
    va_list args;
    int length;

    va_start(args, format);
    length = OS_VSPrintf(dest, format, args);
    va_end(args);
    return length;
}

// NitroSDK's OS_VSPrintf: no limit on the length
int OS_VSPrintf(char *dest, const char *format, va_list args) {
    return OS_VSNPrintf(dest, 0x7fffffff, format, args);
}

int OS_SNPrintf(char *dest, size_t size, const char *format, ...) {
    va_list args;
    int length;

    va_start(args, format);
    length = OS_VSNPrintf(dest, size, format, args);
    va_end(args);
    return length;
}

// NitroSDK's OS_VSNPrintf, which passes everything on to the formatter
int OS_VSNPrintf(char *dest, size_t size, const char *format, va_list args) {
    return STD_TVSNPrintf(dest, size, format, args);
}
