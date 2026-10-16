// TwlSDK's libstd, string functions (std/common/src/std_string.c): copy, search, length and compare, the last three
// reading a word at a time once the strings are aligned, and a scanf, which the file ends with (the scout merged the
// SDK's std_sscanf.c into it, since libstd's archive has no such file; the name is the SDK's). The scanf is one
// function of 0x5b0 bytes whose conversions are labelled blocks, reached by goto from the switch on the conversion
// character. The symbols keep their default names for now, and the comments give the SDK's.
#include "nitro/mi.h"
#include "nitro/std.h"

// Nonzero if a word has a zero byte, which the word loops stop on. The sum of 0x7f and the low seven bits of a byte has
// its high bit set unless they are all clear, and the byte's own high bit is ORed in, so all that is left clear, in
// the result and then in the complement it tests, is the high bit of a zero byte
#define STD_LOW7_MASK 0x7f7f7f7f
#define STD_HAS_ZERO_BYTE(word) (~((((word) & STD_LOW7_MASK) + STD_LOW7_MASK) | (word) | STD_LOW7_MASK))

// The flags of a conversion in STD_TVSScanf
#define STD_SCANF_PLUS 0x0002
#define STD_SCANF_NEGATIVE 0x0008
#define STD_SCANF_LONG 0x0020
#define STD_SCANF_SHORT 0x0040
#define STD_SCANF_LONG_LONG 0x0080
#define STD_SCANF_CHAR 0x0100
#define STD_SCANF_UNSIGNED 0x1000
#define STD_SCANF_SUPPRESS 0x2000

// Sets the bit of a character in a set of 256 bits
#define STD_SET_BIT(set, c) ((set)[(u32)(c) >> 5] |= 1 << ((u32)(c) & 31))

// Whether a byte begins a two-byte Shift-JIS character
#define STD_IS_LEAD_BYTE(c) ((u32)(((u8)(c) ^ 0x20) - 0xa1) < 0x3c)

static BOOL func_0207fa68(int c);
static void func_0207fa88(u32 *bitset, u32 from, u32 to);

// STD_CopyString
char *func_0207f7a4(char *dst, const char *src) {
    char *start = dst;

    while (*src != '\0') {
        *dst++ = *src++;
    }
    *dst = '\0';
    return start;
}

// STD_CopyLString
int func_0207f7cc(char *dst, const char *src, int size) {
    u32 offset = (u32)dst & 3;
    const char *s = src;
    int i = 0;

    if (!(offset ^ ((u32)src & 3))) {
        if (offset != 0) {
            while (i + offset < 4 && i < size - 1) {
                dst[i] = *s;
                if (*s == '\0') {
                    goto terminate;
                }
                s++;
                i++;
            }
        }
        while (i < size - 4) {
            u32 word = *(const u32 *)(src + i);

            if (STD_HAS_ZERO_BYTE(word)) {
                break;
            }
            *(u32 *)(dst + i) = word;
            i += 4;
        }
        s = src + i;
    }
    while (i < size - 1) {
        dst[i] = *s;
        if (*s == '\0') {
            break;
        }
        i++;
        s++;
    }
terminate:
    if (i >= size - 1 && size > 0) {
        dst[i] = '\0';
    }
    if (*s != '\0') {
        i += NNS_STD_StrLen(s);
    }
    return i;
}

// STD_SearchString
char *func_0207f874(const char *str, const char *sub) {
    int i = 0;

    while (str[i] != '\0') {
        int j = 0;
        const char *s = str + i;

        while (sub[j] != '\0' && s[j] == sub[j]) {
            j++;
        }
        if (sub[j] == '\0') {
            return (char *)str + i;
        }
        i++;
    }
    return NULL;
}

// STD_GetStringLength
int NNS_STD_StrLen(const char *str) {
    int len = 0;
    const u32 *p;

    if ((u32)str & 3) {
        do {
            if (str[len] == '\0') {
                return len;
            }
            len++;
        } while (((u32)(str + len)) & 3);
    }
    p = (const u32 *)(str + len);
    for (;;) {
        u32 word = *p;

        if (STD_HAS_ZERO_BYTE(word)) {
            break;
        }
        p++;
        len += 4;
    }
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// STD_GetStringNLength
int func_0207f900(const char *str, int maxlen) {
    int len = 0;
    const u32 *p;

    while (((u32)(str + len) & 3) && len < maxlen) {
        if (str[len] == '\0') {
            return len;
        }
        len++;
    }
    p = (const u32 *)(str + len);
    while (len < maxlen - 4) {
        u32 word = *p;

        if (STD_HAS_ZERO_BYTE(word)) {
            break;
        }
        p++;
        len += 4;
    }
    while (str[len] != '\0' && len < maxlen) {
        len++;
    }
    return len;
}

// STD_CompareString
int STD_CompareString(const char *a, const char *b) {
    u32 offset = (u32)a & 3;
    int i = 0;

    if (!(offset ^ ((u32)b & 3))) {
        if (offset != 0 && offset < 4) {
            do {
                u8 ca = a[i];
                u8 cb = b[i];

                if (ca != cb || ca == '\0') {
                    return ca - cb;
                }
                i++;
            } while (i + offset < 4);
        }
        for (;;) {
            u32 wa = *(const u32 *)(a + i);
            u32 wb = *(const u32 *)(b + i);

            if (wa != wb || STD_HAS_ZERO_BYTE(wa)) {
                break;
            }
            i += 4;
        }
    }
    for (;;) {
        u8 ca = a[i];
        u8 cb = b[i];

        if (ca != cb || ca == '\0') {
            return ca - cb;
        }
        i++;
    }
}

// STD_CompareNString
int func_0207f9c8(const char *a, const char *b, int len) {
    u32 offset;
    int i;

    if (len != 0) {
        offset = (u32)a & 3;
        i = 0;
        if (!(offset ^ ((u32)b & 3))) {
            if (offset != 0) {
                while (i + offset < 4 && i < len) {
                    u8 ca = a[i];
                    u8 cb = b[i];

                    if (ca != cb || ca == '\0') {
                        return ca - cb;
                    }
                    i++;
                }
            }
            while (i <= len - 4) {
                u32 wa = *(const u32 *)(a + i);
                u32 wb = *(const u32 *)(b + i);

                if (wa != wb || STD_HAS_ZERO_BYTE(wa)) {
                    break;
                }
                i += 4;
            }
        }
        while (i < len) {
            u8 ca = a[i];
            u8 cb = b[i];

            if (ca != cb || ca == '\0') {
                return ca - cb;
            }
            i++;
        }
    }
    return 0;
}

// STD_TVSScanf: reads the text as the format says and returns how many values it stored, or -1 if the text ended before
// the first conversion read anything. A conversion that fails ends the scan. The format character at the top of each
// pass and the field width of a conversion share one variable, as they share a register in the original
int func_0207fac0(const char *src, const char *fmt, va_list args) {
    // Declared in the order of the original's stack slots, the highest first, which ours differ from (see nonmatching)
    const char *textStart = src;
    BOOL matched = FALSE;
    u64 value;
    u32 flags;
    int digits;
    char *dstStart;
    BOOL include;
    BOOL range;
    char *setDst;
    int radix;
    int sc;
    int c;
    int count = 0;
    int width;
    char *dst;
    u8 prev;
    u32 set[8];
    char ch;
    char conv;

    while ((width = *fmt) != '\0') {
        if (func_0207fa68(width)) {
            while (func_0207fa68(*fmt)) {
                fmt++;
            }
            while (func_0207fa68(*src)) {
                src++;
            }
            continue;
        }
        if (width != '%') {
            if (width != *src) {
                break;
            }
            if (STD_IS_LEAD_BYTE(width)) {
                src++;
                // BUG: compares the second byte of the text with the first byte of the format character, and moves
                // on in the format by one byte only
#ifdef BUGFIX
                fmt++;
                if (*fmt != *src) {
                    break;
                }
#else
                if (width != *src) {
                    break;
                }
#endif
            }
            src++;
            fmt++;
            continue;
        }
        if (fmt[1] == '%') {
            if (width != *src) {
                break;
            }
            fmt += 2;
            continue;
        }

        value = 0;
        flags = 0;
        fmt++;
        width = 0;
        conv = *fmt;
        if (conv == '*') {
            flags |= STD_SCANF_SUPPRESS;
            fmt++;
            conv = *fmt;
        }
        while (conv >= '0' && conv <= '9') {
            width = width * 10 + conv - '0';
            fmt++;
            conv = *fmt;
        }
        switch (conv) {
        case 'h':
            fmt++;
            conv = *fmt;
            if (conv != 'h') {
                flags |= STD_SCANF_SHORT;
            } else {
                flags |= STD_SCANF_CHAR;
                fmt++;
                conv = *fmt;
            }
            break;
        case 'l':
            fmt++;
            conv = *fmt;
            if (conv != 'l') {
                flags |= STD_SCANF_LONG;
            } else {
                flags |= STD_SCANF_LONG_LONG;
                fmt++;
                conv = *fmt;
            }
            break;
        }

        switch (conv) {
        case 'd':
            radix = 10;
            break;
        case 'i':
            radix = -1;
            break;
        case 'o':
            radix = 8;
            break;
        case 'u':
            radix = 10;
            flags |= STD_SCANF_UNSIGNED;
            break;
        case 'p':
        case 'x':
        case 'X':
            radix = 16;
            flags |= STD_SCANF_UNSIGNED;
            break;
        case 'n':
            if (!(flags & STD_SCANF_SUPPRESS)) {
                count--;
            }
            fmt++;
            value = src - textStart;
            goto storeValue;
        case 'c':
        case 's':
            goto scanString;
        case '[':
            goto scanSet;
        default:
            goto finish;
        }

        fmt++;
        c = *src;
        while (func_0207fa68(c)) {
            src++;
            c = *src;
        }
        for (;;) {
            if (c == '+') {
                flags |= STD_SCANF_PLUS;
            } else if (c == '-') {
                flags |= STD_SCANF_NEGATIVE;
            } else {
                break;
            }
            src++;
            c = *src;
        }
        if (radix == -1) {
            if (c != '0') {
                radix = 10;
            } else if (src[1] == 'x' || src[1] == 'X') {
                radix = 16;
            } else {
                radix = 8;
            }
        }
        if (radix == 16 && c == '0' && (src[1] == 'x' || src[1] == 'X')) {
            src += 2;
            c = *src;
        }
        if (width == 0) {
            width = 0x7fffffff;
        }
        digits = 0;
        while (digits < width) {
            u32 digit = c - '0';

            if (digit >= 10) {
                if (digit - ('a' - '0') < 6) {
                    digit -= 'a' - '0' - 10;
                } else {
                    digit -= 'A' - '0';
                    if (digit < 6) {
                        digit += 10;
                    }
                    // BUG: any other character gets the digit c - 'A', which makes 'G' to 'P' digits of 6 to 15
#ifdef BUGFIX
                    else {
                        break;
                    }
#endif
                }
            }
            if (digit >= radix) {
                break;
            }
            src++;
            c = *src;
            value = value * radix + digit;
            digits++;
        }
        if (digits == 0) {
            break;
        }
        if (flags & STD_SCANF_NEGATIVE) {
            value = -value;
        }
        matched = TRUE;
    storeValue:
        if (!(flags & STD_SCANF_SUPPRESS)) {
            count++;
            if (flags & STD_SCANF_CHAR) {
                *va_arg(args, u8 *) = value;
            } else if (flags & STD_SCANF_SHORT) {
                *va_arg(args, u16 *) = value;
            } else if (flags & STD_SCANF_LONG_LONG) {
                *va_arg(args, u64 *) = value;
            } else {
                *va_arg(args, u32 *) = value;
            }
        }
        continue;
    scanString:
        dstStart = NULL;
        dst = NULL;
        fmt++;
        if (!(flags & STD_SCANF_SUPPRESS)) {
            dst = va_arg(args, char *);
            dstStart = dst;
        }
        if (conv == 's') {
            if (width == 0) {
                width = 0x7fffffff;
            }
            sc = *src;
            while (func_0207fa68(sc)) {
                src++;
                sc = *src;
            }
            while (sc != '\0' && !func_0207fa68(sc) && width > 0) {
                if (dst != NULL) {
                    *dst++ = sc;
                }
                src++;
                sc = *src;
                width--;
            }
            if (dst != dstStart) {
                *dst = '\0';
                count++;
                matched = TRUE;
            }
        } else {
            if (width == 0) {
                width = 1;
            }
            while (*src != '\0' && width > 0) {
                if (dst != NULL) {
                    *dst++ = *src;
                }
                width--;
                src++;
            }
            if (dst != dstStart) {
                count++;
                matched = TRUE;
                if (width > 0) {
                    *dst = '\0';
                }
            }
        }
        continue;
    scanSet:
        setDst = NULL;
        include = TRUE;
        fmt++;
        prev = 0;
        range = FALSE;
        MI_CpuClear32(set, sizeof(set));
        if (*fmt == '^') {
            fmt++;
            include = FALSE;
        }
        if (*fmt == ']') {
            STD_SET_BIT(set, (u8)*fmt);
            fmt++;
        }
        for (;;) {
            if (*fmt == '\0' || *fmt == ']') {
                if (range) {
                    STD_SET_BIT(set, prev);
                    STD_SET_BIT(set, '-');
                }
                if (*fmt == ']') {
                    fmt++;
                }
                break;
            }
            if (!range) {
                if (prev != 0 && *fmt == '-') {
                    range = TRUE;
                } else {
                    STD_SET_BIT(set, (u8)*fmt);
                    prev = *fmt;
                }
            } else {
                u32 cur = (u8)*fmt;

                if (prev > cur) {
                    STD_SET_BIT(set, prev);
                    STD_SET_BIT(set, '-');
                    STD_SET_BIT(set, cur);
                } else {
                    func_0207fa88(set, prev, cur + 1);
                }
                range = FALSE;
                prev = 0;
            }
            fmt++;
        }

        matched = TRUE;
        if (!(flags & STD_SCANF_SUPPRESS)) {
            count++;
            setDst = va_arg(args, char *);
        }
        if (width == 0) {
            width = 0x7fffffff;
        }
        ch = *src;
        while (ch != '\0' && width > 0) {
            if (include != ((set[(u32)(u8)ch >> 5] >> ((u32)(u8)ch & 31)) & 1)) {
                break;
            }
            if (setDst != NULL) {
                *setDst++ = ch;
            }
            src++;
            ch = *src;
            width--;
        }
        if (setDst != NULL) {
            *setDst = '\0';
        }
        continue;
    }
finish:
    if (*src == '\0' && !matched) {
        count = -1;
    }
    return count;
}

// STD_TSScanf
int func_0207fa4c(const char *src, const char *fmt, ...) {
    va_list args;
    int count;

    va_start(args, fmt);
    count = func_0207fac0(src, fmt, args);
    va_end(args);
    return count;
}

// STDi_IsSpace
static BOOL func_0207fa68(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// STDi_FillBitset: sets the bits from to to - 1
static void func_0207fa88(u32 *bitset, u32 from, u32 to) {
    if (from < to) {
        u32 mask;
        u32 lastMask = (1 << (to & 31)) - 1;

        do {
            u32 word = from >> 5;
            mask = ~0u << (from & 31);

            from = (word + 1) << 5;
            if (from > to) {
                mask &= lastMask;
            }
            bitset[word] |= mask;
        } while (from < to);
    }
}
