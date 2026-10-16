// MSL C's wstring.c: the string functions on wide characters of <wchar.h>. Only wcslen and wcscpy are in the ROM.
#include "types.h"
#include <wchar.h>

size_t wcslen(const wchar_t *s) {
    size_t n = -1;

    do {
        n++;
    } while (*s++ != 0);
    return n;
}

wchar_t *wcscpy(wchar_t *dst, const wchar_t *src) {
    wchar_t *d = dst;

    while ((*d++ = *src++) != 0) {
    }
    return dst;
}
