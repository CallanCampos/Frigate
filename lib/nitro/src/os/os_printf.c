// NitroSDK OS: the formatted text functions of the OS (os_printf.c), which call libstd's text formatter.
#include "nitro/os.h"

// NitroSDK's OS_SPrintf
int func_0207a290(char *dest, const char *format, ...) {
    va_list args;
    int length;

    va_start(args, format);
    length = func_0207a2ac(dest, format, args);
    va_end(args);
    return length;
}

// NitroSDK's OS_VSPrintf: no limit on the length
int func_0207a2ac(char *dest, const char *format, va_list args) {
    return func_0207a2dc(dest, 0x7fffffff, format, args);
}

// NitroSDK's OS_SNPrintf
int func_0207a2c0(char *dest, u32 size, const char *format, ...) {
    va_list args;
    int length;

    va_start(args, format);
    length = func_0207a2dc(dest, size, format, args);
    va_end(args);
    return length;
}

// NitroSDK's OS_VSNPrintf, which passes everything on to the formatter
int func_0207a2dc(char *dest, u32 size, const char *format, va_list args) {
    return OS_VSNPrintf(dest, size, format, args);
}
