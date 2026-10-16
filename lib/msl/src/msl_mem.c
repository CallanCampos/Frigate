// MSL C's mem.c: the memory functions of <string.h>, which copy and compare a byte at a time: memcpy, memmove, memset,
// memchr and memcmp. (The name is MSL's mem.c with the library's prefix, as pokediamond names its object.)
#include "types.h"
#include "mem_funcs.h"
#include <string.h>

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (n != 0) {
        do {
            *d++ = *s++;
        } while (--n != 0);
    }
    return dst;
}

// Copies forward when src is not below dst, and backward from the ends otherwise, so that the ranges can overlap
void *memmove(void *dst, const void *src, size_t n) {
    const unsigned char *s;
    unsigned char *d;

    if (src >= dst) {
        s = src;
        d = dst;
        if (n != 0) {
            do {
                *d++ = *s++;
            } while (--n != 0);
        }
    } else {
        s = (const unsigned char *)src + n;
        d = (unsigned char *)dst + n;
        if (n != 0) {
            do {
                *--d = *--s;
            } while (--n != 0);
        }
    }
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    __fill_mem(dst, c, n);
    return dst;
}

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = s;
    unsigned char ch = c;

    if (n != 0) {
        do {
            if (*p++ == ch) {
                return (void *)(p - 1);
            }
        } while (--n != 0);
    }
    return NULL;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *p = a;
    const unsigned char *q = b;

    if (n != 0) {
        do {
            if (*p++ != *q++) {
                return p[-1] < q[-1] ? -1 : 1;
            }
        } while (--n != 0);
    }
    return 0;
}
