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

// Whether a byte begins a two-byte Shift-JIS character
#define STD_IS_LEAD_BYTE(c) ((u32)(((u8)(c) ^ 0x20) - 0xa1) < 0x3c)

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

int STD_TSNPrintf(char *dst, u32 len, const char *fmt, ...) {
    va_list args;
    int length;

    va_start(args, fmt);
    length = STD_TVSNPrintf(dst, len, fmt, args);
    va_end(args);
    return length;
}

int STD_TVSNPrintf(char *dst, u32 len, const char *fmt, va_list args) {
    char prefix[2];
    char digits[24];
    StringBuffer str;

    str.len = len;
    str.dst = dst;
    str.start = dst;
    while (*fmt != '\0') {
        int flags = 0;
        int width;
        int prec;
        int radix;
        int hexBase;
        const char *percent;
        u64 value;
        int prefixLen;
        int numDigits;
        int zeros;

        if (STD_IS_LEAD_BYTE(*fmt)) {
            string_put_char(&str, *fmt++);
            if (*fmt != '\0') {
                string_put_char(&str, *fmt++);
            }
            continue;
        }
        if (*fmt != '%') {
            string_put_char(&str, *fmt++);
            continue;
        }

        percent = fmt;
        width = 0;
        prec = -1;
        radix = 10;
        hexBase = 'a' - 10;
        for (;;) {
            switch (*++fmt) {
            case '+':
                // BUG: the plus flag only counts after a blank flag ("% +d"). In "%+d" it ends the flags, and the
                // conversion that follows is the +, which is none, so the text is printed as it is
#ifdef BUGFIX
                flags |= STD_PRINTF_PLUS;
                continue;
#else
                if (fmt[-1] == ' ') {
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

        if (*fmt == '*') {
            fmt++;
            width = va_arg(args, int);
            if (width < 0) {
                width = -width;
                flags |= STD_PRINTF_LEFT;
            }
        } else {
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + *fmt++ - '0';
            }
        }

        if (*fmt == '.') {
            fmt++;
            prec = 0;
            if (*fmt == '*') {
                fmt++;
                prec = va_arg(args, int);
                if (prec < 0) {
                    prec = -1;
                }
            } else {
                while (*fmt >= '0' && *fmt <= '9') {
                    prec = prec * 10 + *fmt++ - '0';
                }
            }
        }

        switch (*fmt) {
        case 'h':
            fmt++;
            if (*fmt != 'h') {
                flags |= STD_PRINTF_SHORT;
            } else {
                fmt++;
                flags |= STD_PRINTF_CHAR;
            }
            break;
        case 'l':
            fmt++;
            if (*fmt != 'l') {
                flags |= STD_PRINTF_LONG;
            } else {
                fmt++;
                flags |= STD_PRINTF_LONG_LONG;
            }
            break;
        }

        // The numbers go on below the switch. A character, a string, a % and a conversion that does not exist are
        // written here, and their code goes on with the next character
        switch (*fmt) {
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
                fmt++;
                continue;
            }
            goto literal;
        case 's': {
            const char *s = va_arg(args, const char *);
            int n = 0;

            if (prec < 0) {
                while (s[n] != '\0') {
                    n++;
                }
            } else {
                while (n < prec && s[n] != '\0') {
                    n++;
                }
            }
            width -= n;
            if (flags & STD_PRINTF_LEFT) {
                string_put_string(&str, s, n);
                string_fill_char(&str, ' ', width);
            } else {
                string_fill_char(&str, (flags & STD_PRINTF_ZERO) ? '0' : ' ', width);
                string_put_string(&str, s, n);
            }
            fmt++;
            continue;
        }
        case '%':
            if (percent + 1 == fmt) {
                string_put_char(&str, *fmt++);
                continue;
            }
        default:
        literal:
            // What is not a conversion is written as it is, up to the character that stopped it, which is read again
            string_put_string(&str, percent, fmt - percent);
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
            if (value & 0x8000000000000000ULL) {
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
                digits[numDigits++] = (value & 7) + '0';
                value >>= 3;
            }
            break;
        case 10:
            if ((value >> 32) == 0) {
                u32 v = value;

                while (v != 0) {
                    u32 q = v / 10;

                    digits[numDigits++] = v - q * 10 + '0';
                    v = q;
                }
            } else {
                while (value != 0) {
                    u64 q = value / 10;

                    digits[numDigits++] = value - q * 10 + '0';
                    value = q;
                }
            }
            break;
        case 16:
            while (value != 0) {
                int d = value & 15;

                digits[numDigits++] = d < 10 ? d + '0' : d + hexBase;
                value >>= 4;
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
        fmt++;
    }

    if (str.len != 0) {
        *str.dst = '\0';
    } else if (len != 0) {
        str.start[len - 1] = '\0';
    }
    return str.dst - str.start;
}
