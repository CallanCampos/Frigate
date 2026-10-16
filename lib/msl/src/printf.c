// MSL C's printf.c: the formatted output of <stdio.h>. The library keeps only the string versions, sprintf and
// vsnprintf: __pformatter walks the format string, parse_format reads one conversion into a print_format, and the
// *2str functions turn a number into text, back to front, at the end of a buffer. Output goes through a write
// function that the caller gives, so that the same code could serve a stream and a string (the stream versions are not
// in the ROM). The names are MSL's own, which swan's symbols have too, and the functions but vsnprintf and sprintf
// are static there as here.
#include "types.h"
#include "ansi_fp.h"
#include "secure_error.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// The smallest long long (the library's limits.h has it, which the ROM's code does not need otherwise)
#define LLONG_MIN (-0x7FFFFFFFFFFFFFFFLL - 1)

// The longest text that a number may take in __pformatter's buffer
#define MAX_FIELD_WIDTH 509

enum justification_options {
    left_justification,
    right_justification,
    zero_fill,
};

enum sign_options {
    only_minus,
    sign_always,
    space_holds_place,
};

// The length modifier of a conversion. wchar_argument is what %ls and %lc become
enum argument_options {
    normal_argument,
    char_argument,
    short_argument,
    long_argument,
    long_long_argument,
    wchar_argument,
    intmax_argument,
    size_t_argument,
    ptrdiff_argument,
    long_double_argument,
};

// One conversion of a format string, as parse_format reads it. A conversion_char of 0xFF stands for one that is not
// valid
typedef struct print_format {
    unsigned char justification_options;
    unsigned char sign_options;
    unsigned char precision_specified;
    unsigned char alternate_form;
    unsigned char argument_options;
    char conversion_char;
    int field_width;
    int precision;
} print_format;

typedef struct __OutStrCtrl {
    char *CharStr;
    size_t MaxCharCount;
    size_t CharsWritten;
} __OutStrCtrl;

// Reads the conversion that starts at the % of format_string into *format, taking the arguments of * widths and
// precisions from *arg, and returns where the text goes on after it
static const char *parse_format(const char *format_string, va_list *arg, print_format *format) {
    print_format f;
    const char *s = format_string;
    int c;
    int flag_found;

    f.justification_options = right_justification;
    f.sign_options = only_minus;
    f.precision_specified = 0;
    f.alternate_form = 0;
    f.argument_options = normal_argument;
    f.field_width = 0;
    f.precision = 0;

    c = *++s;
    if (c == '%') {
        f.conversion_char = c;
        *format = f;
        return s + 1;
    }

    // The flags
    for (;;) {
        flag_found = 1;
        switch (c) {
        case '-':
            f.justification_options = left_justification;
            break;
        case '+':
            f.sign_options = sign_always;
            break;
        case ' ':
            if (f.sign_options != sign_always) {
                f.sign_options = space_holds_place;
            }
            break;
        case '#':
            f.alternate_form = 1;
            break;
        case '0':
            if (f.justification_options != left_justification) {
                f.justification_options = zero_fill;
            }
            break;
        default:
            flag_found = 0;
            break;
        }
        if (!flag_found) {
            break;
        }
        c = *++s;
    }

    // The field width
    if (c == '*') {
        f.field_width = va_arg(*arg, int);
        c = *++s;
        if (f.field_width < 0) {
            f.justification_options = left_justification;
            f.field_width = -f.field_width;
        }
    } else {
        while (isdigit(c)) {
            f.field_width = f.field_width * 10 + (c - '0');
            c = *++s;
        }
    }

    if (f.field_width > MAX_FIELD_WIDTH) {
        f.conversion_char = 0xFF;
        *format = f;
        return s + 1;
    }

    // The precision
    if (c == '.') {
        c = *++s;
        f.precision_specified = 1;
        if (c == '*') {
            f.precision = va_arg(*arg, int);
            c = *++s;
            if (f.precision < 0) {
                f.precision_specified = 0;
            }
        } else {
            while (isdigit(c)) {
                f.precision = f.precision * 10 + (c - '0');
                c = *++s;
            }
        }
    }

    // The length modifier
    flag_found = 1;
    switch (c) {
    case 'h':
        f.argument_options = short_argument;
        if (s[1] == 'h') {
            f.argument_options = char_argument;
            c = *++s;
        }
        break;
    case 'l':
        f.argument_options = long_argument;
        if (s[1] == 'l') {
            f.argument_options = long_long_argument;
            c = *++s;
        }
        break;
    case 'L':
        f.argument_options = long_double_argument;
        break;
    case 'j':
        f.argument_options = intmax_argument;
        break;
    case 't':
        f.argument_options = ptrdiff_argument;
        break;
    case 'z':
        f.argument_options = size_t_argument;
        break;
    default:
        flag_found = 0;
        break;
    }
    if (flag_found) {
        c = *++s;
    }

    f.conversion_char = c;

    // The modifiers and the precision that the conversion allows
    switch (c) {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
        if (f.argument_options == long_double_argument) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 1;
        } else if (f.justification_options == zero_fill) {
            f.justification_options = right_justification;
        }
        break;
    case 'f':
    case 'F':
        if (f.argument_options == short_argument || f.argument_options == intmax_argument ||
            f.argument_options == size_t_argument || f.argument_options == ptrdiff_argument ||
            f.argument_options == long_long_argument) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 6;
        }
        break;
    case 'a':
    case 'A':
        if (!f.precision_specified) {
            f.precision = 13;
        }
        if (f.argument_options == short_argument || f.argument_options == intmax_argument ||
            f.argument_options == size_t_argument || f.argument_options == ptrdiff_argument ||
            f.argument_options == long_long_argument || f.argument_options == char_argument) {
            f.conversion_char = 0xFF;
        }
        break;
    case 'g':
    case 'G':
        if (f.precision == 0) {
            f.precision = 1;
        }
        // fall through
    case 'e':
    case 'E':
        if (f.argument_options == short_argument || f.argument_options == char_argument ||
            f.argument_options == intmax_argument || f.argument_options == size_t_argument ||
            f.argument_options == ptrdiff_argument || f.argument_options == long_long_argument) {
            f.conversion_char = 0xFF;
        } else if (!f.precision_specified) {
            f.precision = 6;
        }
        break;
    case 'p':
        f.conversion_char = 'x';
        f.alternate_form = 1;
        f.argument_options = long_argument;
        f.precision = 8;
        break;
    case 'c':
        if (f.argument_options == long_argument) {
            f.argument_options = wchar_argument;
        } else if (f.precision_specified || f.argument_options != normal_argument) {
            f.conversion_char = 0xFF;
        }
        break;
    case 's':
        if (f.argument_options == long_argument) {
            f.argument_options = wchar_argument;
        } else if (f.argument_options != normal_argument) {
            f.conversion_char = 0xFF;
        }
        break;
    case 'n':
        if (f.argument_options == long_double_argument) {
            f.conversion_char = 0xFF;
        }
        break;
    default:
        f.conversion_char = 0xFF;
        break;
    }

    *format = f;
    return s + 1;
}

// Writes the integer num in the base that format.conversion_char says, back to front, ending just before buff, and
// returns where it starts: with its sign, its 0 or 0x prefix and its zeros up to the precision (or the field width, for
// zero fill). Returns NULL if that would be more than MAX_FIELD_WIDTH characters. buff[-1] is the terminator
static char *long2str(long num, char *buff, print_format format) {
    unsigned long unsigned_num = num;
    unsigned long base;
    char *p = buff;
    int n;
    int minus = 0;
    int digits = 0;

    *--p = '\0';

    if (num == 0 && format.precision == 0 && !(format.alternate_form && format.conversion_char == 'o')) {
        return p;
    }

    switch (format.conversion_char) {
    case 'd':
    case 'i':
        base = 10;
        if (num < 0) {
            // The test is against the long long minimum, which no long is, so it never fails (as the original has it)
            if (num != LLONG_MIN) {
                unsigned_num = -num;
            }
            minus = 1;
        }
        break;
    case 'o':
        base = 8;
        format.sign_options = only_minus;
        break;
    case 'u':
        base = 10;
        format.sign_options = only_minus;
        break;
    case 'x':
    case 'X':
        base = 16;
        format.sign_options = only_minus;
        break;
    }

    do {
        n = unsigned_num % base;
        unsigned_num /= base;
        if (n < 10) {
            n += '0';
        } else if (format.conversion_char == 'x') {
            n += 'a' - 10;
        } else {
            n += 'A' - 10;
        }
        *--p = n;
        ++digits;
    } while (unsigned_num != 0);

    if (base == 8 && format.alternate_form && *p != '0') {
        *--p = '0';
        ++digits;
    }

    if (format.justification_options == zero_fill) {
        format.precision = format.field_width;
        if (minus || format.sign_options != only_minus) {
            --format.precision;
        }
        if (base == 16 && format.alternate_form) {
            format.precision -= 2;
        }
    }

    if (buff - p + format.precision > MAX_FIELD_WIDTH) {
        return NULL;
    }

    while (digits < format.precision) {
        *--p = '0';
        ++digits;
    }

    if (base == 16 && format.alternate_form) {
        *--p = format.conversion_char;
        *--p = '0';
    }

    if (minus) {
        *--p = '-';
    } else if (format.sign_options == sign_always) {
        *--p = '+';
    } else if (format.sign_options == space_holds_place) {
        *--p = ' ';
    }

    return p;
}

// long2str for a long long
static char *longlong2str(long long num, char *buff, print_format format) {
    unsigned long long unsigned_num = num;
    unsigned long long base;
    char *p = buff;
    int n;
    int minus = 0;
    int digits = 0;

    *--p = '\0';

    if (num == 0 && format.precision == 0 && !(format.alternate_form && format.conversion_char == 'o')) {
        return p;
    }

    switch (format.conversion_char) {
    case 'd':
    case 'i':
        base = 10;
        if (num < 0) {
            if (num != LLONG_MIN) {
                unsigned_num = -num;
            }
            minus = 1;
        }
        break;
    case 'o':
        base = 8;
        format.sign_options = only_minus;
        break;
    case 'u':
        base = 10;
        format.sign_options = only_minus;
        break;
    case 'x':
    case 'X':
        base = 16;
        format.sign_options = only_minus;
        break;
    }

    do {
        n = unsigned_num % base;
        unsigned_num /= base;
        if (n < 10) {
            n += '0';
        } else if (format.conversion_char == 'x') {
            n += 'a' - 10;
        } else {
            n += 'A' - 10;
        }
        *--p = n;
        ++digits;
    } while (unsigned_num != 0);

    if (base == 8 && format.alternate_form && *p != '0') {
        *--p = '0';
        ++digits;
    }

    if (format.justification_options == zero_fill) {
        format.precision = format.field_width;
        if (minus || format.sign_options != only_minus) {
            --format.precision;
        }
        if (base == 16 && format.alternate_form) {
            format.precision -= 2;
        }
    }

    if (buff - p + format.precision > MAX_FIELD_WIDTH) {
        return NULL;
    }

    while (digits < format.precision) {
        *--p = '0';
        ++digits;
    }

    if (base == 16 && format.alternate_form) {
        *--p = format.conversion_char;
        *--p = '0';
    }

    if (minus) {
        *--p = '-';
    } else if (format.sign_options == sign_always) {
        *--p = '+';
    } else if (format.sign_options == space_holds_place) {
        *--p = ' ';
    }

    return p;
}

// The bytes of num, which double2hex reverses in place so that the first one holds the sign and the top of the exponent
#define NUM_BYTE(i) (((unsigned char *)&num)[i])

// Writes num as a hexadecimal floating-point number (%a), back to front, ending just before buff: 0x1.8p+3. The
// exponent and the mantissa's digits are read from the double's bytes, which num is turned into the big-endian order of
// in place; value keeps the number itself for __num2dec and the test for zero. Returns NULL if the precision is more
// than MAX_FIELD_WIDTH. The text of an infinity or a NaN takes the case of the conversion, A or a
static char *double2hex(double num, char *buff, print_format format) {
    print_format exp_format;
    decimal d;
    decform form;
    double value = num;
    int exp;
    unsigned char *q;
    unsigned char digit;
    int count;
    unsigned char tmp;
    int bit;
    char *p;
    int i;
    unsigned char *bytes;

    if (format.precision > MAX_FIELD_WIDTH) {
        return NULL;
    }

    form.style = FLOATDECIMAL;
    form.digits = SIGDIGLEN;

    __num2dec(&form, value, &d);

    switch (d.sig.text[0]) {
    case '0':
        d.exp = 0;
        break;
    case 'I':
        if (d.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                strcpy(p, "-INF");
            } else {
                strcpy(p, "-inf");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                strcpy(p, "INF");
            } else {
                strcpy(p, "inf");
            }
        }
        return p;
    case 'N':
        if (d.sign) {
            p = buff - 5;
            if (format.conversion_char == 'A') {
                strcpy(p, "-NAN");
            } else {
                strcpy(p, "-nan");
            }
        } else {
            p = buff - 4;
            if (format.conversion_char == 'A') {
                strcpy(p, "NAN");
            } else {
                strcpy(p, "nan");
            }
        }
        return p;
    }

    bytes = (unsigned char *)&num;

    // The exponent is written as a decimal with its sign, as %+d does
    exp_format.justification_options = right_justification;
    exp_format.sign_options = sign_always;
    exp_format.precision_specified = 0;
    exp_format.alternate_form = 0;
    exp_format.argument_options = normal_argument;
    exp_format.field_width = 0;
    exp_format.precision = 1;
    exp_format.conversion_char = 'd';

    for (i = 0; i < 4; i++) {
        tmp = bytes[i];
        bytes[i] = bytes[7 - i];
        bytes[7 - i] = tmp;
    }

    // The exponent field: the 11 bits after the sign, less the bias of 1023 (a denormal has no bias to take off)
    exp = (((unsigned long)NUM_BYTE(0) << 25 | (unsigned long)NUM_BYTE(1) << 17) >> 21) & 0x7FF;
    p = long2str(exp != 0 ? exp - 1023 : 0, buff, exp_format);
    --p;
    if (format.conversion_char == 'a') {
        *p = 'p';
    } else {
        *p = 'P';
    }

    // The digits go from the last, whose last bit is bit 4 * precision + 11 of the 64 (counted from the top), to the
    // first, which starts at bit 12. Those past the 64 bits are zeros. A digit that starts in the byte before the one
    // its last bit is in takes its top bits from there
    bit = format.precision * 4 + 11;
    q = (unsigned char *)&num;
    for (count = format.precision; count >= 1; --count, bit -= 4) {
        if (bit < 64) {
            digit = q[bit >> 3] >> (7 - (bit & 7));
            if ((bit & ~7) != ((bit - 4) & ~7)) {
                digit |= (q[(bit >> 3) - 1] << 8) >> (7 - (bit & 7));
            }
            digit &= 0xF;
            if (digit < 10) {
                digit += '0';
            } else if (format.conversion_char == 'a') {
                digit += 'a' - 10;
            } else {
                digit += 'A' - 10;
            }
        } else {
            digit = '0';
        }
        *--p = digit;
    }

    if (format.precision != 0 || format.alternate_form) {
        *--p = '.';
    }

    if (0.0 != fabs(value)) {
        *--p = '1';
    } else {
        *--p = '0';
    }
    if (format.conversion_char == 'a') {
        *--p = 'x';
    } else {
        *--p = 'X';
    }
    *--p = '0';

    if (d.sign) {
        *--p = '-';
    } else if (format.sign_options == sign_always) {
        *--p = '+';
    } else if (format.sign_options == space_holds_place) {
        *--p = ' ';
    }

    return p;
}

// Rounds the digits of dec to new_length of them (to even, when what is cut off is exactly half), dropping the
// trailing zeros that leaves and carrying into a new first digit if it must. A new_length below 0, or no digit left
// after the carry is dropped through, makes the number 0
static void round_decimal(decimal *dec, int new_length) {
    unsigned char c;
    unsigned char *p;
    unsigned char *q;
    int carry;
    unsigned char *text;
    int length;

    if (new_length < 0) {
    zero:
        dec->sig.length = 1;
        dec->exp = 0;
        dec->sig.text[0] = '0';
        return;
    }

    length = dec->sig.length;
    if (new_length >= length) {
        return;
    }

    text = dec->sig.text;
    p = text + new_length + 1;
    c = *--p - '0';

    if (c == 5) {
        q = text + length;
        do {
            --q;
        } while (q > p && *q == '0');
        if (q == p) {
            // BUG: with new_length 0 there is no digit before p, and p[-1] is the number of digits (sig.length), which
            // is 1 for a lone 5, so it always rounds up: 0.5 prints as 1 for %.0f, where round-half-even gives 0
#ifdef BUGFIX
            carry = new_length != 0 ? p[-1] & 1 : 0;
#else
            carry = p[-1] & 1;
#endif
        } else {
            carry = 1;
        }
    } else {
        carry = c > 5;
    }

    while (new_length != 0) {
        c = *--p - '0' + carry;
        carry = c > 9;
        if (carry || c == 0) {
            --new_length;
        } else {
            *p = c + '0';
            break;
        }
    }

    if (carry) {
        dec->exp++;
        dec->sig.length = 1;
        dec->sig.text[0] = '1';
        return;
    }

    if (new_length == 0) {
        goto zero;
    }

    dec->sig.length = new_length;
}

// Writes num for the %e, %f and %g conversions, back to front, ending just before buff, and returns where it starts
static char *float2str(double num, char *buff, print_format format) {
    decform form;
    decimal d;
    char *p;
    unsigned char *q;
    int n;
    int digits;
    int int_digits;
    int frac_digits;
    int i;
    char sign_char;

    if (format.precision > MAX_FIELD_WIDTH) {
        return NULL;
    }

    form.style = FLOATDECIMAL;
    form.digits = SIGDIGLEN;

    __num2dec(&form, num, &d);

    // __num2dec gives the exponent of the last digit; the trailing zeros come off, which raises it
    q = d.sig.text + d.sig.length;
    while (d.sig.length > 1 && *--q == '0') {
        --d.sig.length;
        ++d.exp;
    }

    switch (d.sig.text[0]) {
    case '0':
        d.exp = 0;
        break;
    case 'I':
        if (num < 0.0) {
            p = buff - 5;
            if (isupper(format.conversion_char)) {
                strcpy(p, "-INF");
            } else {
                strcpy(p, "-inf");
            }
        } else {
            p = buff - 4;
            if (isupper(format.conversion_char)) {
                strcpy(p, "INF");
            } else {
                strcpy(p, "inf");
            }
        }
        return p;
    case 'N':
        if (d.sign) {
            p = buff - 5;
            if (isupper(format.conversion_char)) {
                strcpy(p, "-NAN");
            } else {
                strcpy(p, "-nan");
            }
        } else {
            p = buff - 4;
            if (isupper(format.conversion_char)) {
                strcpy(p, "NAN");
            } else {
                strcpy(p, "nan");
            }
        }
        return p;
    }

    // From here exp is the exponent of the first digit
    d.exp += d.sig.length - 1;
    p = buff;
    *--p = '\0';

    switch (format.conversion_char) {
    case 'g':
    case 'G':
        if (d.sig.length > format.precision) {
            round_decimal(&d, format.precision);
        }
        if (d.exp < -4 || d.exp >= format.precision) {
            if (format.alternate_form) {
                --format.precision;
            } else {
                format.precision = d.sig.length - 1;
            }
            if (format.conversion_char == 'g') {
                format.conversion_char = 'e';
            } else {
                format.conversion_char = 'E';
            }
            goto exponential;
        } else {
            if (format.alternate_form) {
                format.precision -= d.exp + 1;
            } else {
                format.precision = d.sig.length - (d.exp + 1);
                if (format.precision < 0) {
                    format.precision = 0;
                }
            }
            goto fixed;
        }
    case 'e':
    case 'E':
    exponential:
        if (d.sig.length > format.precision + 1) {
            round_decimal(&d, format.precision + 1);
        }

        n = d.exp;
        sign_char = '+';
        digits = 0;
        if (n < 0) {
            n = -n;
            sign_char = '-';
        }
        while (n != 0 || digits < 2) {
            *--p = n % 10 + '0';
            n /= 10;
            ++digits;
        }
        *--p = sign_char;
        *--p = format.conversion_char;

        if (buff - p + format.precision > MAX_FIELD_WIDTH) {
            return NULL;
        }

        if (d.sig.length < format.precision + 1) {
            n = format.precision + 2 - d.sig.length;
            while (--n != 0) {
                *--p = '0';
            }
        }

        q = d.sig.text + d.sig.length;
        n = d.sig.length;
        while (--n != 0) {
            *--p = *--q;
        }

        if (format.precision != 0 || format.alternate_form) {
            *--p = '.';
        }
        *--p = d.sig.text[0];

        if (d.sign) {
            *--p = '-';
        } else if (format.sign_options == sign_always) {
            *--p = '+';
        } else if (format.sign_options == space_holds_place) {
            *--p = ' ';
        }
        break;
    case 'f':
    case 'F':
    fixed:
        frac_digits = d.sig.length - d.exp - 1;
        if (frac_digits < 0) {
            frac_digits = 0;
        }
        if (frac_digits > format.precision) {
            round_decimal(&d, d.sig.length - (frac_digits - format.precision));
            frac_digits = d.sig.length - d.exp - 1;
            if (frac_digits < 0) {
                frac_digits = 0;
            }
        }
        int_digits = d.exp + 1;
        if (int_digits < 0) {
            int_digits = 0;
        }

        if (int_digits + frac_digits > MAX_FIELD_WIDTH) {
            return NULL;
        }

        q = d.sig.text + d.sig.length;

        for (i = 0; i < format.precision - frac_digits; ++i) {
            *--p = '0';
        }
        for (i = 0; i < frac_digits && i < d.sig.length; ++i) {
            *--p = *--q;
        }
        for (; i < frac_digits; ++i) {
            *--p = '0';
        }

        if (format.precision != 0 || format.alternate_form) {
            *--p = '.';
        }

        if (int_digits != 0) {
            i = 0;
            if (int_digits - d.sig.length > 0) {
                do {
                    *--p = '0';
                    ++i;
                } while (i < int_digits - d.sig.length);
            }
            for (; i < int_digits; ++i) {
                *--p = *--q;
            }
        } else {
            *--p = '0';
        }

        if (d.sign) {
            *--p = '-';
        } else if (format.sign_options == sign_always) {
            *--p = '+';
        } else if (format.sign_options == space_holds_place) {
            *--p = ' ';
        }
        break;
    }

    return p;
}

// Formats format_str with the arguments in arg, sending the text through WriteProc(WriteProcArg, text, length) (which
// returns 0 for failure) a piece at a time. Returns the number of characters, or -1. A secure call (is_secure) reports
// a null %s or %n argument as a constraint violation instead of printing it
static int __pformatter(void *(*WriteProc)(void *, const char *, size_t), void *WriteProcArg, const char *format_str,
                        va_list arg, int is_secure) {
    int num_chars;
    int chars_written;
    const char *format_ptr;
    long long_num;
    double long_double_num;
    char *buff_ptr;
    char *string_end;
    int total_chars;
    char buff[512];
    print_format format;
    char fill_char = ' ';
    long long long_long_num;
    const char *curr_format;

    format_ptr = format_str;
    chars_written = 0;

    while (*format_ptr != '\0') {
        curr_format = strchr(format_ptr, '%');
        if (curr_format == NULL) {
            num_chars = strlen(format_ptr);
            chars_written += num_chars;
            if (num_chars != 0 && !WriteProc(WriteProcArg, format_ptr, num_chars)) {
                return -1;
            }
            break;
        }

        num_chars = curr_format - format_ptr;
        chars_written += num_chars;
        if (num_chars != 0 && !WriteProc(WriteProcArg, format_ptr, num_chars)) {
            return -1;
        }

        format_ptr = parse_format(curr_format, &arg, &format);

        switch (format.conversion_char) {
        // The integers: the argument is taken at the size the length modifier says and cut down to a char or a short
        // for hh and h; the long longs go through longlong2str
        case 'd':
        case 'i':
            if (format.argument_options == long_argument) {
                long_num = va_arg(arg, long);
            } else if (format.argument_options == long_long_argument) {
                long_long_num = va_arg(arg, long long);
            } else if (format.argument_options == intmax_argument) {
                long_long_num = va_arg(arg, long long);
            } else if (format.argument_options == size_t_argument) {
                long_num = va_arg(arg, size_t);
            } else if (format.argument_options == ptrdiff_argument) {
                long_num = va_arg(arg, long);
            } else {
                long_num = va_arg(arg, int);
            }

            if (format.argument_options == short_argument) {
                long_num = (short)long_num;
            }
            if (format.argument_options == char_argument) {
                long_num = (signed char)long_num;
            }

            if (format.argument_options == long_long_argument || format.argument_options == intmax_argument) {
                if ((buff_ptr = longlong2str(long_long_num, buff + sizeof(buff), format)) == NULL) {
                    goto conversion_error;
                }
            } else {
                if ((buff_ptr = long2str(long_num, buff + sizeof(buff), format)) == NULL) {
                    goto conversion_error;
                }
            }
            num_chars = buff + sizeof(buff) - 1 - buff_ptr;
            break;
        case 'o':
        case 'u':
        case 'x':
        case 'X':
            if (format.argument_options == long_argument) {
                long_num = va_arg(arg, unsigned long);
            } else if (format.argument_options == long_long_argument) {
                long_long_num = va_arg(arg, unsigned long long);
            } else if (format.argument_options == intmax_argument) {
                long_long_num = va_arg(arg, unsigned long long);
            } else if (format.argument_options == size_t_argument) {
                long_num = va_arg(arg, size_t);
            } else if (format.argument_options == ptrdiff_argument) {
                long_num = va_arg(arg, unsigned long);
            } else {
                long_num = va_arg(arg, unsigned int);
            }

            if (format.argument_options == short_argument) {
                long_num = (unsigned short)long_num;
            }
            if (format.argument_options == char_argument) {
                long_num = (unsigned char)long_num;
            }

            if (format.argument_options == long_long_argument || format.argument_options == intmax_argument) {
                if ((buff_ptr = longlong2str(long_long_num, buff + sizeof(buff), format)) == NULL) {
                    goto conversion_error;
                }
            } else {
                if ((buff_ptr = long2str(long_num, buff + sizeof(buff), format)) == NULL) {
                    goto conversion_error;
                }
            }
            num_chars = buff + sizeof(buff) - 1 - buff_ptr;
            break;
        // The floating-point numbers, whose long double is a double here
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
            if (format.argument_options == long_double_argument) {
                long_double_num = va_arg(arg, long double);
            } else {
                long_double_num = va_arg(arg, double);
            }
            if ((buff_ptr = float2str(long_double_num, buff + sizeof(buff), format)) == NULL) {
                goto conversion_error;
            }
            num_chars = buff + sizeof(buff) - 1 - buff_ptr;
            break;
        case 'a':
        case 'A':
            if (format.argument_options == long_double_argument) {
                long_double_num = va_arg(arg, long double);
            } else {
                long_double_num = va_arg(arg, double);
            }
            if ((buff_ptr = double2hex(long_double_num, buff + sizeof(buff), format)) == NULL) {
                goto conversion_error;
            }
            num_chars = buff + sizeof(buff) - 1 - buff_ptr;
            break;
        // A string, of bytes or (for ls) of wide characters, which wcstombs turns into bytes in buff. A null pointer
        // prints as an empty string, or is reported if the call is secure
        case 's':
            if (format.argument_options == wchar_argument) {
                wchar_t *wcs_ptr = va_arg(arg, wchar_t *);

                if (is_secure && wcs_ptr == NULL) {
                    nothing(NULL, NULL, -1);
                    return -1;
                }
                if (wcs_ptr == NULL) {
                    wcs_ptr = L"";
                }
                buff_ptr = buff;
                num_chars = wcstombs(buff_ptr, wcs_ptr, sizeof(buff));
                if (num_chars < 0) {
                    goto conversion_error;
                }
            } else {
                buff_ptr = va_arg(arg, char *);
            }

            if (is_secure && buff_ptr == NULL) {
                nothing(NULL, NULL, -1);
                return -1;
            }
            if (buff_ptr == NULL) {
                buff_ptr = "";
            }

            if (format.alternate_form) {
                // A Pascal string: the length comes first
                num_chars = *buff_ptr++;
                if (format.precision_specified && num_chars > format.precision) {
                    num_chars = format.precision;
                }
            } else if (format.precision_specified) {
                num_chars = format.precision;
                string_end = memchr(buff_ptr, '\0', num_chars);
                if (string_end != NULL) {
                    num_chars = string_end - buff_ptr;
                }
            } else {
                num_chars = strlen(buff_ptr);
            }
            break;
        // Stores the number of characters so far, through the pointer (which buff_ptr holds), at the size of the length
        // modifier (a secure call reports it instead, and hh and the wide kind are ignored)
        case 'n':
            buff_ptr = va_arg(arg, char *);

            if (is_secure) {
                nothing(NULL, NULL, -1);
                return -1;
            }
            switch (format.argument_options) {
            case normal_argument:
                *(int *)buff_ptr = chars_written;
                break;
            case short_argument:
                *(short *)buff_ptr = chars_written;
                break;
            case long_argument:
                *(long *)buff_ptr = chars_written;
                break;
            case intmax_argument:
                *(long long *)buff_ptr = chars_written;
                break;
            case size_t_argument:
                *(size_t *)buff_ptr = chars_written;
                break;
            case ptrdiff_argument:
                *(long *)buff_ptr = chars_written;
                break;
            case long_long_argument:
                *(long long *)buff_ptr = chars_written;
                break;
            }
            continue;
        case 'c':
            buff_ptr = buff;
            *buff_ptr = va_arg(arg, int);
            num_chars = 1;
            break;
        case '%':
            buff_ptr = buff;
            *buff_ptr = '%';
            num_chars = 1;
            break;
        // An invalid conversion (0xFF is what parse_format marks it with), or one that gave no text, ends the output
        // with the rest of the format string from its % on
        case 0xFF:
        default:
        conversion_error:
            num_chars = strlen(curr_format);
            chars_written += num_chars;
            if (num_chars != 0 && !WriteProc(WriteProcArg, curr_format, num_chars)) {
                return -1;
            }
            return chars_written;
        }

        // The text is in buff_ptr, num_chars long; it goes out padded to the field width, on the left with spaces or,
        // for zero fill, with zeros after the sign and the 0x of a %a
        total_chars = num_chars;

        if (format.justification_options != left_justification) {
            fill_char = (format.justification_options == zero_fill) ? '0' : ' ';

            if ((*buff_ptr == '+' || *buff_ptr == '-' || *buff_ptr == ' ') && fill_char == '0') {
                if (!WriteProc(WriteProcArg, buff_ptr, 1)) {
                    return -1;
                }
                ++buff_ptr;
                --num_chars;
            }

            if (format.justification_options == zero_fill &&
                (format.conversion_char == 'a' || format.conversion_char == 'A')) {
                if (num_chars < 2) {
                    return -1;
                }
                if (!WriteProc(WriteProcArg, buff_ptr, 2)) {
                    return -1;
                }
                num_chars -= 2;
                buff_ptr += 2;
            }

            while (total_chars < format.field_width) {
                if (!WriteProc(WriteProcArg, &fill_char, 1)) {
                    return -1;
                }
                ++total_chars;
            }
        }

        if (num_chars != 0 && !WriteProc(WriteProcArg, buff_ptr, num_chars)) {
            return -1;
        }

        // Left justified: the padding follows the text
        if (format.justification_options == left_justification) {
            while (total_chars < format.field_width) {
                char blank = ' ';

                if (!WriteProc(WriteProcArg, &blank, 1)) {
                    return -1;
                }
                ++total_chars;
            }
        }

        chars_written += total_chars;
    }

    return chars_written;
}

// The write function of vsnprintf: copies what fits of the text into the buffer and counts it
static void *__StringWrite(void *osc, const char *Buffer, size_t NumChars) {
    __OutStrCtrl *Ctrl = (__OutStrCtrl *)osc;

    if (Ctrl->CharsWritten + NumChars > Ctrl->MaxCharCount) {
        NumChars = Ctrl->MaxCharCount - Ctrl->CharsWritten;
    }
    memcpy(Ctrl->CharStr + Ctrl->CharsWritten, Buffer, NumChars);
    Ctrl->CharsWritten += NumChars;
    return (void *)1;
}

int vsnprintf(char *s, size_t n, const char *format, va_list arg) {
    int end;
    __OutStrCtrl osc;

    osc.CharStr = s;
    osc.MaxCharCount = n;
    osc.CharsWritten = 0;
    end = __pformatter(__StringWrite, &osc, format, arg, 0);
    if (s != NULL) {
        if (end < n) {
            s[end] = '\0';
        } else if (n != 0) {
            s[n - 1] = '\0';
        }
    }
    return end;
}

int sprintf(char *s, const char *format, ...) {
    va_list args;

    va_start(args, format);
    return vsnprintf(s, (size_t)-1, format, args);
}
