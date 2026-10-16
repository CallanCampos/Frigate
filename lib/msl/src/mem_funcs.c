// MSL C's mem_funcs.c: the fill that memset is built on, which stores single bytes up to a word boundary, then 32 bytes
// at a time as words, then the words and the bytes that are left.
#include "mem_funcs.h"

void __fill_mem(void *dst, int val, unsigned long n) {
    unsigned char *bp = dst;
    unsigned long *wp;
    unsigned long v = (unsigned char)val;
    unsigned long i;

    if (n >= 32) {
        i = -(unsigned long)dst & 3;
        if (i != 0) {
            n -= i;
            do {
                *bp++ = v;
            } while (--i != 0);
        }
        if (v != 0) {
            v |= (v << 24) | (v << 16) | (v << 8);
        }
        wp = (unsigned long *)bp;
        i = n >> 5;
        if (i != 0) {
            do {
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
                *wp++ = v;
            } while (--i != 0);
        }
        i = (n & 31) >> 2;
        if (i != 0) {
            do {
                *wp++ = v;
            } while (--i != 0);
        }
        bp = (unsigned char *)wp;
        n &= 3;
    }
    if (n != 0) {
        do {
            *bp++ = v;
        } while (--n != 0);
    }
}
