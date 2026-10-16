// MSL C's string.c: the string functions of <string.h>. strcpy and strcmp go a word at a time when both strings are
// aligned alike, and look for the terminator in a word by the bit trick that finds a zero byte: w - 0x01010101 has the
// top bit set in the first zero byte. strcpy masks it with ~w, which is then exact; strcmp's loop leaves that out, and
// a hit is checked bytewise. The library's other functions of the file are not in the ROM.
#include "types.h"
#include <string.h>

size_t strlen(const char *s) {
    size_t n = -1;

    do {
        n++;
    } while (*s++ != '\0');
    return n;
}

// Copies the bytes up to a word boundary, then a word at a time while the word has no zero byte, then the rest bytewise
char *strcpy(char *dst, const char *src) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    unsigned long c;

    if (((unsigned long)d & 3) == ((unsigned long)s & 3)) {
        int n = (unsigned long)s & 3;

        if (n != 0) {
            if ((*d = *s) == 0) {
                return dst;
            }
            n = 3 - n;
            while (n != 0) {
                c = *++s;
                if ((*++d = c) == 0) {
                    return dst;
                }
                n--;
            }
            d++;
            s++;
        }
        {
            unsigned long w = *(const unsigned long *)s;
            unsigned long t;

            t = w + 0xFEFEFEFF;
            t &= ~w;
            if (!(t & 0x80808080)) {
                d -= 4;
                do {
                    *(unsigned long *)(d += 4) = w;
                    w = *(const unsigned long *)(s += 4);
                    t = w + 0xFEFEFEFF;
                    t &= ~w;
                } while (!(t & 0x80808080));
                d += 4;
            }
        }
    }
    if ((*d = *s) != 0) {
        while ((*++d = *++s) != 0) {
        }
    }
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    const unsigned char *p = (const unsigned char *)src;
    unsigned char *q = (unsigned char *)dst;

    if (n != 0) {
        do {
            if ((*q++ = *p++) == '\0') {
                if (--n != 0) {
                    do {
                        *q++ = '\0';
                    } while (--n != 0);
                }
                break;
            }
        } while (--n != 0);
    }
    return dst;
}

char *strcat(char *dst, const char *src) {
    const char *p = src;
    char *q = dst;

    while (*q++ != '\0') {
    }
    q--;
    while ((*q++ = *p++) != '\0') {
    }
    return dst;
}

// The same as strcpy: bytes up to the boundary, then words while they are equal and have no zero byte, then the bytes
// of the word where it ends or differs
int strcmp(const char *a, const char *b) {
    const unsigned char *p1 = (const unsigned char *)a;
    const unsigned char *p2 = (const unsigned char *)b;
    unsigned long c1, c2;
    long diff;

    c1 = *p1;
    c2 = *p2;
    if ((diff = c1 - c2) != 0) {
        return diff;
    }
    if (((unsigned long)p2 & 3) == ((unsigned long)p1 & 3)) {
        int n = (unsigned long)p1 & 3;

        if (n != 0) {
            if (c1 == 0) {
                return 0;
            }
            n = 3 - n;
            while (n != 0) {
                c1 = *++p1;
                c2 = *++p2;
                if ((diff = c1 - c2) != 0) {
                    return diff;
                }
                if (c1 == 0) {
                    return 0;
                }
                n--;
            }
            p1++;
            p2++;
        }
        c1 = *(const unsigned long *)p1;
        c2 = *(const unsigned long *)p2;
        if (((c1 + 0xFEFEFEFF) & ~c1 & 0x80808080) == 0) {
            while (c1 == c2) {
                c1 = *(const unsigned long *)(p1 += 4);
                c2 = *(const unsigned long *)(p2 += 4);
                if ((c1 + 0xFEFEFEFF) & 0x80808080) {
                    goto zero_word;
                }
            }
            p1--;
            p2--;
            goto bytes;
        }
    zero_word:
        c1 = *p1;
        c2 = *p2;
        if ((diff = c1 - c2) != 0) {
            return diff;
        }
    }
bytes:
    if (c1 == 0) {
        return 0;
    }
    do {
        c1 = *++p1;
        c2 = *++p2;
        if ((diff = c1 - c2) != 0) {
            return diff;
        }
    } while (c1 != 0);
    return 0;
}

int strncmp(const char *a, const char *b, size_t n) {
    const unsigned char *p1 = (const unsigned char *)a;
    const unsigned char *p2 = (const unsigned char *)b;

    if (n != 0) {
        do {
            unsigned char c2 = *p2++;
            unsigned char c1 = *p1++;

            if (c1 != c2) {
                return c1 - c2;
            }
            if (c1 == '\0') {
                break;
            }
        } while (--n != 0);
    }
    return 0;
}

char *strchr(const char *s, int c) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char ch = c;
    unsigned char t;

    while ((t = *p++) != 0) {
        if (t == ch) {
            return (char *)(p - 1);
        }
    }
    if (ch != 0) {
        return NULL;
    }
    return (char *)(p - 1);
}

// The set becomes a bitmap of 256 bits, one bit for each byte value that it holds
size_t strcspn(const char *str, const char *set) {
    const unsigned char *p;
    unsigned char map[32] = { 0 };
    const unsigned char *q = (const unsigned char *)set;
    int c;

    c = *q++;
    while (c != 0) {
        map[(unsigned char)c >> 3] |= (unsigned char)(1 << (c & 7));
        c = *q++;
    }
    p = (const unsigned char *)str;
    c = *p++;
    while (c != 0) {
        if (map[(unsigned char)c >> 3] & (unsigned char)(1 << (c & 7))) {
            break;
        }
        c = *p++;
    }
    return (size_t)(p - (const unsigned char *)str - 1);
}

char *strstr(const char *str, const char *pat) {
    const unsigned char *s = (const unsigned char *)str;
    const unsigned char *p = (const unsigned char *)pat;
    unsigned int first, c1, c2;

    if (p == NULL || (first = *p) == 0) {
        return (char *)str;
    }
    while ((c1 = *s++) != 0) {
        if (c1 == first) {
            const unsigned char *a = s;
            const unsigned char *b = p + 1;

            do {
                c2 = *b++;
                c1 = *a++;
                if (c1 != c2) {
                    break;
                }
            } while (c1 != 0);
            if (c2 == 0) {
                return (char *)(s - 1);
            }
        }
    }
    return NULL;
}
