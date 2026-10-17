// A stand-in for the UCRT's stdio.h, for the Windows compile check in CMakeLists.txt: only what the spike uses
#ifndef WINDOWS_STUB_STDIO_H
#define WINDOWS_STUB_STDIO_H

#include <stddef.h>

int snprintf(char *buffer, size_t size, const char *format, ...);

#endif
