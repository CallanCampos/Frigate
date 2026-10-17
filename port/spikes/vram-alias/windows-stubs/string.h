// A stand-in for the UCRT's string.h, for the Windows compile check in CMakeLists.txt: only what the spike uses
#ifndef WINDOWS_STUB_STRING_H
#define WINDOWS_STUB_STRING_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t size);
void *memset(void *dst, int value, size_t size);
int memcmp(const void *a, const void *b, size_t size);

#endif
