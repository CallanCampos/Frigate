#ifndef POKEBW2_WCHAR_H
#define POKEBW2_WCHAR_H

// The C library's wchar.h, from CodeWarrior's MSL, which the main module links

#include <stddef.h>
#include <stdio.h>

// MSL's wide character for the ARM platform is 16 bits
#ifndef __wchar_t_defined
#define __wchar_t_defined
typedef unsigned short wchar_t;
#endif

// The length of a wide string without its terminator, and a copy of it with the terminator (wstring.c)
size_t wcslen(const wchar_t *s);
wchar_t *wcscpy(wchar_t *dst, const wchar_t *src);

// Sets a stream's orientation to wide (mode above 0) or to bytes (below 0) if it has none (wchar_io.c). Returns the
// orientation the stream has (1 for wide, -1 for bytes), or the mode as it was given when the stream had none, and 0
// for a null or closed stream
int fwide(FILE *file, int mode);

#endif // POKEBW2_WCHAR_H
