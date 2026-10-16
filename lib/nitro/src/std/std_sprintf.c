// TwlSDK's libstd, formatted text (std/common/src/std_sprintf.c): the printf family. They all end in one formatter,
// which writes into a buffer of limited size and goes on counting when it is full. It knows the flags blank, plus,
// minus and zero, a width and a precision (each a number or * from the arguments), h, hh, l and ll, and the
// conversions c, s, d, i, u, o, x, X, p and %. The names are the SDK's.
#include "nitro/std.h"

// The flags of a conversion
#define STD_PRINTF_BLANK 0x0001
#define STD_PRINTF_PLUS 0x0002
#define STD_PRINTF_ALTERNATE 0x0004 // The 0x or 0 prefix. Only %p sets it: the flags do not include a #
#define STD_PRINTF_LEFT 0x0008
#define STD_PRINTF_ZERO 0x0010
#define STD_PRINTF_LONG 0x0020
#define STD_PRINTF_SHORT 0x0040
#define STD_PRINTF_LONG_LONG 0x0080
#define STD_PRINTF_CHAR 0x0100
#define STD_PRINTF_UNSIGNED 0x1000

// Where the text goes: how many bytes are left, where the next one is written and where the text begins. The
// writing goes on when no bytes are left, so that dst tells the length
typedef struct StringBuffer {
    u32 len;
    char *dst;
    char *start;
} StringBuffer;

static void string_put_char(StringBuffer *str, char c);
static void string_fill_char(StringBuffer *str, char c, int len);
static void string_put_string(StringBuffer *str, const char *src, int len);

static void string_put_char(StringBuffer *str, char c) {
    if (str->len != 0) {
        *str->dst = c;
        str->len--;
    }
    str->dst++;
}

static void string_fill_char(StringBuffer *str, char c, int len) {
    if (len > 0) {
        u32 i;
        u32 n = str->len;

        if (n > len) {
            n = len;
        }
        for (i = 0; i < n; i++) {
            str->dst[i] = c;
        }
        str->len -= n;
        str->dst += len;
    }
}

static void string_put_string(StringBuffer *str, const char *src, int len) {
    if (len > 0) {
        u32 i;
        u32 n = str->len;

        if (n > len) {
            n = len;
        }
        for (i = 0; i < n; i++) {
            str->dst[i] = src[i];
        }
        str->len -= n;
        str->dst += len;
    }
}

int STD_TSPrintf(char *dst, const char *fmt, ...) {
    va_list args;
    int length;

    va_start(args, fmt);
    length = STD_TVSNPrintf(dst, 0x7fffffff, fmt, args);
    va_end(args);
    return length;
}

int STD_TSNPrintf(char *dst, size_t len, const char *fmt, ...) {
    va_list args;
    int length;

    va_start(args, fmt);
    length = STD_TVSNPrintf(dst, len, fmt, args);
    va_end(args);
    return length;
}

int STD_TVSNPrintf(char *dst, size_t len, const char *fmt, va_list args) {
    char digits[24];
    int numDigits;
    char prefix[2];
    int prefixLen;
    StringBuffer str;
    const char *s = fmt;

    str.len = len;
    str.dst = str.start = dst;
    while (*s != '\0') {
        int flags = 0;
        int width;
        int prec;
        int radix;
        const char *percent;
        int hexBase;
        u64 value;
        int zeros;

        if (STD_IsSjisLeadByte(*s)) {
            string_put_char(&str, *s++);
            if (*s != '\0') {
                string_put_char(&str, *s++);
            }
            continue;
        }
        if (*s != '%') {
            string_put_char(&str, *s++);
            continue;
        }

        percent = s;
        width = 0;
        prec = -1;
        radix = 10;
        hexBase = 'a' - 10;
        for (;;) {
            switch (*++s) {
            case '+':
                // BUG: the plus flag only counts after a blank flag ("% +d"). In "%+d" it ends the flags, and the
                // conversion that follows is the +, which is none, so the text is printed as it is
#ifdef BUGFIX
                flags |= STD_PRINTF_PLUS;
                continue;
#else
                if (s[-1] == ' ') {
                    flags |= STD_PRINTF_PLUS;
                    continue;
                }
                break;
#endif
            case ' ':
                flags |= STD_PRINTF_BLANK;
                continue;
            case '-':
                flags |= STD_PRINTF_LEFT;
                continue;
            case '0':
                flags |= STD_PRINTF_ZERO;
                continue;
            }
            break;
        }

        if (*s == '*') {
            s++;
            width = va_arg(args, int);
            if (width < 0) {
                width = -width;
                flags |= STD_PRINTF_LEFT;
            }
        } else {
            while (*s >= '0' && *s <= '9') {
                width = width * 10 + *s++ - '0';
            }
        }

        if (*s == '.') {
            s++;
            prec = 0;
            if (*s == '*') {
                s++;
                prec = va_arg(args, int);
                if (prec < 0) {
                    prec = -1;
                }
            } else {
                while (*s >= '0' && *s <= '9') {
                    prec = prec * 10 + *s++ - '0';
                }
            }
        }

        switch (*s) {
        case 'h':
            s++;
            if (*s != 'h') {
                flags |= STD_PRINTF_SHORT;
            } else {
                s++;
                flags |= STD_PRINTF_CHAR;
            }
            break;
        case 'l':
            s++;
            if (*s != 'l') {
                flags |= STD_PRINTF_LONG;
            } else {
                s++;
                flags |= STD_PRINTF_LONG_LONG;
            }
            break;
        }

        // The numbers go on below the switch. A character, a string, a % and a conversion that does not exist are
        // written here, and their code goes on with the next character
        switch (*s) {
        case 'd':
        case 'i':
            break;
        case 'o':
            radix = 8;
            flags |= STD_PRINTF_UNSIGNED;
            break;
        case 'u':
            flags |= STD_PRINTF_UNSIGNED;
            break;
        case 'X':
            hexBase = 'A' - 10;
            goto hex;
        case 'p':
            flags |= STD_PRINTF_ALTERNATE;
            prec = 8;
            goto hex;
        case 'c':
            if (prec < 0) {
                int c = va_arg(args, int);

                if (flags & STD_PRINTF_LEFT) {
                    string_put_char(&str, c);
                    string_fill_char(&str, ' ', width - 1);
                } else {
                    string_fill_char(&str, (flags & STD_PRINTF_ZERO) ? '0' : ' ', width - 1);
                    string_put_char(&str, c);
                }
                s++;
                continue;
            }
            goto literal;
        case 's': {
            const char *string = va_arg(args, const char *);
            int n = 0;

            if (prec < 0) {
                while (string[n] != '\0') {
                    n++;
                }
            } else {
                while (n < prec && string[n] != '\0') {
                    n++;
                }
            }
            width -= n;
            if (flags & STD_PRINTF_LEFT) {
                string_put_string(&str, string, n);
                string_fill_char(&str, ' ', width);
            } else {
                string_fill_char(&str, (flags & STD_PRINTF_ZERO) ? '0' : ' ', width);
                string_put_string(&str, string, n);
            }
            s++;
            continue;
        }
        case '%':
            if (percent + 1 == s) {
                string_put_char(&str, *s++);
                continue;
            }
        default:
        literal:
            // What is not a conversion is written as it is, up to the character that stopped it, which is read again
            string_put_string(&str, percent, s - percent);
            continue;
        case 'x':
        hex:
            radix = 16;
            flags |= STD_PRINTF_UNSIGNED;
            break;
        }

        // The value, as a 64-bit number for all sizes, with its sign or prefix kept apart in prefix, which is
        // written backwards
        prefixLen = 0;
        if (flags & STD_PRINTF_LEFT) {
            flags &= ~STD_PRINTF_ZERO;
        }
        if (prec < 0) {
            prec = 1;
        } else {
            flags &= ~STD_PRINTF_ZERO;
        }

        if (flags & STD_PRINTF_UNSIGNED) {
            if (flags & STD_PRINTF_CHAR) {
                value = (u8)va_arg(args, u32);
            } else if (flags & STD_PRINTF_SHORT) {
                value = (u16)va_arg(args, u32);
            } else if (flags & STD_PRINTF_LONG_LONG) {
                value = va_arg(args, u64);
            } else {
                value = va_arg(args, u32);
            }
            flags &= ~(STD_PRINTF_BLANK | STD_PRINTF_PLUS);
            if (flags & STD_PRINTF_ALTERNATE) {
                if (radix == 16) {
                    if (value != 0) {
                        prefix[0] = hexBase + 0x21;
                        prefix[1] = '0';
                        prefixLen = 2;
                    }
                } else if (radix == 8) {
                    prefix[0] = '0';
                    prefixLen = 1;
                }
            }
        } else {
            if (flags & STD_PRINTF_CHAR) {
                value = (s8)va_arg(args, s32);
            } else if (flags & STD_PRINTF_SHORT) {
                value = (s16)va_arg(args, s32);
            } else if (flags & STD_PRINTF_LONG_LONG) {
                value = va_arg(args, s64);
            } else {
                value = va_arg(args, s32);
            }
            // The sign bit, the top of the high word
            if ((value >> 32) & 0x80000000) {
                value = ~value + 1;
                prefix[0] = '-';
                prefixLen = 1;
            } else if (value != 0 || prec != 0) {
                if (flags & STD_PRINTF_PLUS) {
                    prefix[0] = '+';
                    prefixLen = 1;
                } else if (flags & STD_PRINTF_BLANK) {
                    prefix[0] = ' ';
                    prefixLen = 1;
                }
            }
        }

        // The digits, last first. A value of 0 has none, and the precision pads it
        numDigits = 0;
        switch (radix) {
        case 8:
            while (value != 0) {
                int d = value & 7;

                value >>= 3;
                digits[numDigits++] = d + '0';
            }
            break;
        case 10:
            if ((value >> 32) == 0) {
                u32 v = value;

                while (v != 0) {
                    u32 q = v / 10;
                    int d = v - q * 10;

                    v = q;
                    digits[numDigits++] = d + '0';
                }
            } else {
                while (value != 0) {
                    u64 q = value / 10;
                    int d = value - q * 10;

                    value = q;
                    digits[numDigits++] = d + '0';
                }
            }
            break;
        case 16:
            while (value != 0) {
                int d = value & 15;

                value >>= 4;
                digits[numDigits++] = d < 10 ? d + '0' : d + hexBase;
            }
            break;
        }

        // The 0 prefix of an octal number is one of its digits
        if (prefixLen > 0 && prefix[0] == '0') {
            prefixLen = 0;
            digits[numDigits++] = '0';
        }
        zeros = prec - numDigits;
        if (flags & STD_PRINTF_ZERO) {
            int pad = width - numDigits - prefixLen;

            if (zeros < pad) {
                zeros = pad;
            }
        }
        if (zeros > 0) {
            width -= zeros;
        }
        width -= prefixLen + numDigits;

        if (!(flags & STD_PRINTF_LEFT)) {
            string_fill_char(&str, ' ', width);
        }
        while (prefixLen > 0) {
            string_put_char(&str, prefix[--prefixLen]);
        }
        string_fill_char(&str, '0', zeros);
        while (numDigits > 0) {
            string_put_char(&str, digits[--numDigits]);
        }
        if (flags & STD_PRINTF_LEFT) {
            string_fill_char(&str, ' ', width);
        }
        s++;
    }

    if (str.len != 0) {
        *str.dst = '\0';
    } else if (len != 0) {
        str.start[len - 1] = '\0';
    }
    return str.dst - str.start;
}
