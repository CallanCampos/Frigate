// MSL C's strtold.c: the scan of a floating-point number of <stdlib.h>. __strtold reads the text of a number through a
// read function and builds the double; strtod gives it a string to read, and atod (swan's name; it is atof) is strtod
// without the end pointer. The scan is a state machine on the characters: a decimal number goes from white space and
// sign through its digits to its exponent, "0x" starts a hexadecimal one with a state machine of its own, and
// "INFINITY" and "NAN(...)" have their states too.
#include "types.h"
#include "ansi_fp.h"
#include "strtold.h"
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The most significant digits that a decimal number keeps: the others only move its exponent
#define MAX_SIG_DIG 20

// The exponent of ten of the largest double
#define MAX_EXP 308

// The most hexadecimal digits that a hexadecimal number keeps, which are 56 bits
#define MAX_HEX_DIG 14

// The states of the scan. They are bits so that the loop and the end can test several at once.
enum {
    SCAN_START = 0x0001,      // white space, then the sign
    SCAN_CHECK_ZERO = 0x0002, // the first character of the number
    SCAN_LEADING_ZEROS = 0x0004,
    SCAN_INT_DIGITS = 0x0008,
    SCAN_FRACTION_START = 0x0010, // after a point that had no digits before it: a digit has to follow
    SCAN_FRACTION_DIGITS = 0x0020,
    SCAN_SIG_END = 0x0040, // the significand is done: an 'E' starts the exponent
    SCAN_EXP_SIGN = 0x0080,
    SCAN_EXP_START = 0x0100, // the first digit of the exponent
    SCAN_EXP_ZEROS = 0x0200,
    SCAN_EXP_DIGITS = 0x0400,
    SCAN_FINISHED = 0x0800,
    SCAN_FAILED = 0x1000,
    SCAN_NAN = 0x2000,
    SCAN_INFINITY = 0x4000,
    SCAN_HEX = 0x8000 // a hexadecimal number: hex_state tells where in it
};

// The states that leave a number: the scan ended at the end of its digits or of its exponent
#define SCAN_NUMBER_STATES                                                                                             \
    (SCAN_LEADING_ZEROS | SCAN_INT_DIGITS | SCAN_FRACTION_DIGITS | SCAN_EXP_ZEROS | SCAN_EXP_DIGITS | SCAN_FINISHED)

// The states of a hexadecimal number
enum {
    HEX_START = 0x001,
    HEX_LEADING_ZEROS = 0x002,
    HEX_INT_DIGITS = 0x004,
    HEX_FRACTION_DIGITS = 0x008,
    HEX_EXP_CHAR = 0x010, // the 'P' of the exponent
    HEX_EXP_SIGN = 0x020,
    HEX_EXP_START = 0x040,
    HEX_EXP_ZEROS = 0x080,
    HEX_EXP_DIGITS = 0x100
};

// The states that leave a hexadecimal number
#define HEX_NUMBER_STATES (HEX_LEADING_ZEROS | HEX_INT_DIGITS | HEX_FRACTION_DIGITS | HEX_EXP_ZEROS | HEX_EXP_DIGITS)

// Reads at most max_width characters of the number at the start of the text, after its white space. *chars_scanned is
// how many of them were the number (0 when there was none) and the character after it is given back. *overflow is set
// when the value is out of the range of a double: the result is then infinity, or 0 for a negative exponent, and a
// value under DBL_MIN is flagged too, but kept.
double __strtold(int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg, int *chars_scanned,
                 int *overflow) {
    int scan_state = SCAN_START;
    int hex_state = 0;
    int count = 0;
    int spaces = 0;
    int c;
    decimal d = { 0 };
    int negative = 0;
    int exp_negative = 0;
    int exp = 0;
    int dec_exp = 0;
    int sign_detected = 0;
    unsigned char *cp;
    int hex_exp = 0;
    int hex_exp_negative = 0;
    unsigned int int_digits = 0;
    int nibble;
    unsigned char mant[8]; // the digits of a hexadecimal number, two to a byte from the top
    int failed;
    int len;
    unsigned char *p;
    double result;

    *overflow = 0;
    count++;
    c = (*ReadProc)(ReadProcArg, 0, MSL_GET);

    while (count <= max_width && c != EOF && !(scan_state & (SCAN_FINISHED | SCAN_FAILED))) {
        switch (scan_state) {
        case SCAN_START:
            if (isspace(c)) {
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                spaces++;
                break;
            }
            switch (toupper(c)) {
            case '-':
                negative = 1;
            case '+':
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                sign_detected = 1;
                break;
            case 'I':
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                scan_state = SCAN_INFINITY;
                break;
            case 'N':
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                scan_state = SCAN_NAN;
                break;
            default:
                scan_state = SCAN_CHECK_ZERO;
                break;
            }
            break;

        case SCAN_INFINITY: {
            int i = 1;
            char inf_string[] = "INFINITY";

            while (i < 8 && inf_string[i] == toupper(c)) {
                i++;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            }
            if (i == 3 || i == 8) {
                if (negative) {
                    result = -INFINITY;
                } else {
                    result = INFINITY;
                }
                *chars_scanned = spaces + i + sign_detected;
                return result;
            }
            scan_state = SCAN_FAILED;
            break;
        }

        case SCAN_NAN: {
            int i = 1;
            int j = 0;
            char nan_string[] = "NAN(";
            char nan_arg[32] = { 0 };

            while (i < 4 && nan_string[i] == toupper(c)) {
                i++;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            }
            if (i == 3 || i == 4) {
                if (i == 4) {
                    // BUG: j also counts the ')' before the terminator is written, so a payload of 31 characters or
                    // more writes it past nan_arg
#ifdef BUGFIX
                    while (j < (int)sizeof(nan_arg) - 2 && (isdigit(c) || isalpha(c) || c == '.')) {
#else
                    while (j < 32 && (isdigit(c) || isalpha(c) || c == '.')) {
#endif
                        nan_arg[j] = c;
                        j++;
                        count++;
                        c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                    }
                    if (c != ')') {
                        scan_state = SCAN_FAILED;
                        break;
                    }
                    j++;
                }
                nan_arg[j] = 0;
                if (negative) {
                    result = -nan(nan_arg);
                } else {
                    result = nan(nan_arg);
                }
                *chars_scanned = spaces + i + j + sign_detected;
                return result;
            }
            scan_state = SCAN_FAILED;
            break;
        }

        case SCAN_CHECK_ZERO:
            if (c == '.') {
                scan_state = SCAN_FRACTION_START;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            if (!isdigit(c)) {
                scan_state = SCAN_FAILED;
                break;
            }
            if (c == '0') {
                count++;
                if (toupper(c = (*ReadProc)(ReadProcArg, 0, MSL_GET)) == 'X') {
                    scan_state = SCAN_HEX;
                    hex_state = HEX_START;
                } else {
                    scan_state = SCAN_LEADING_ZEROS;
                }
            } else {
                scan_state = SCAN_INT_DIGITS;
            }
            break;

        case SCAN_LEADING_ZEROS:
            if (c != '0') {
                scan_state = SCAN_INT_DIGITS;
                break;
            }
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_INT_DIGITS:
            if (!isdigit(c)) {
                if (c != '.') {
                    scan_state = SCAN_SIG_END;
                    break;
                }
                scan_state = SCAN_FRACTION_DIGITS;
            } else if (d.sig.length < MAX_SIG_DIG) {
                d.sig.text[d.sig.length++] = c;
            } else {
                dec_exp++;
            }
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_FRACTION_START:
            if (!isdigit(c)) {
                scan_state = SCAN_FAILED;
            } else {
                scan_state = SCAN_FRACTION_DIGITS;
            }
            break;

        case SCAN_FRACTION_DIGITS:
            if (!isdigit(c)) {
                scan_state = SCAN_SIG_END;
                break;
            }
            if (d.sig.length < MAX_SIG_DIG) {
                if (c != '0' || d.sig.length != 0) {
                    d.sig.text[d.sig.length++] = c;
                }
                dec_exp--;
            }
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_SIG_END:
            if (toupper(c) != 'E') {
                scan_state = SCAN_FINISHED;
                break;
            }
            scan_state = SCAN_EXP_SIGN;
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_EXP_SIGN:
            if (c == '+') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            } else if (c == '-') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                exp_negative = 1;
            }
            scan_state = SCAN_EXP_START;
            break;

        case SCAN_EXP_START:
            if (!isdigit(c)) {
                scan_state = SCAN_FAILED;
                break;
            }
            if (c != '0') {
                scan_state = SCAN_EXP_DIGITS;
                break;
            }
            scan_state = SCAN_EXP_ZEROS;
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_EXP_ZEROS:
            if (c != '0') {
                scan_state = SCAN_EXP_DIGITS;
                break;
            }
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_EXP_DIGITS:
            if (!isdigit(c)) {
                scan_state = SCAN_FINISHED;
                break;
            }
            exp = exp * 10 + (c - '0');
            if (exp > MAX_EXP) {
                *overflow = 1;
            }
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;

        case SCAN_HEX:
            switch (hex_state) {
            case HEX_START:
                cp = mant;
                int_digits = 0;
                memset(cp, 0, 8);
                nibble = 0;
                hex_state = HEX_LEADING_ZEROS;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_LEADING_ZEROS:
                if (c != '0') {
                    hex_state = HEX_INT_DIGITS;
                    break;
                }
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_INT_DIGITS:
                if (!isxdigit(c)) {
                    if (c != '.') {
                        hex_state = HEX_EXP_CHAR;
                        break;
                    }
                    hex_state = HEX_FRACTION_DIGITS;
                } else if (int_digits < MAX_HEX_DIG) {
                    unsigned char byte;
                    unsigned char digit;

                    int_digits++;
                    byte = cp[nibble / 2];
                    c = toupper(c);
                    digit = c >= 'A' ? c - 'A' + 10 : c - '0';
                    if (nibble % 2 == 0) {
                        byte |= digit << 4;
                    } else {
                        byte |= digit;
                    }
                    cp[nibble / 2] = byte;
                    nibble++;
                }
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_FRACTION_DIGITS:
                if (!isxdigit(c)) {
                    hex_state = HEX_EXP_CHAR;
                    break;
                }
                // BUG: the digits after the point are kept while fewer than 14 digits came before it, not while fewer
                // than 14 are kept, so a long fraction runs over mant
#ifdef BUGFIX
                if (nibble < MAX_HEX_DIG) {
#else
                if (int_digits < MAX_HEX_DIG) {
#endif
                    unsigned char byte;
                    unsigned char digit;

                    byte = cp[nibble / 2];
                    c = toupper(c);
                    digit = c >= 'A' ? c - 'A' + 10 : c - '0';
                    if (nibble % 2 == 0) {
                        byte |= digit << 4;
                    } else {
                        byte |= digit;
                    }
                    cp[nibble / 2] = byte;
                    nibble++;
                }
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_EXP_CHAR:
                if (toupper(c) != 'P') {
                    scan_state = SCAN_FINISHED;
                    break;
                }
                hex_state = HEX_EXP_SIGN;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_EXP_SIGN:
                if (c == '-') {
                    hex_exp_negative = 1;
                } else if (c != '+') {
                    (*ReadProc)(ReadProcArg, c, MSL_UNGET);
                    count--;
                }
                hex_state = HEX_EXP_START;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_EXP_START:
                if (!isdigit(c)) {
                    scan_state = SCAN_FAILED;
                    break;
                }
                if (c != '0') {
                    hex_state = HEX_EXP_DIGITS;
                    break;
                }
                hex_state = HEX_EXP_ZEROS;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_EXP_ZEROS:
                if (c != '0') {
                    hex_state = HEX_EXP_DIGITS;
                    break;
                }
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;

            case HEX_EXP_DIGITS:
                if (!isdigit(c)) {
                    scan_state = SCAN_FINISHED;
                    break;
                }
                hex_exp = hex_exp * 10 + (c - '0');
                // BUG: this tests exp, the exponent of a decimal number, which is 0 here, so a long exponent never
                // sets *overflow on its own
#ifdef BUGFIX
                if (hex_exp > 0x7FFF) {
#else
                if (exp > 0x7FFF) {
#endif
                    *overflow = 1;
                }
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            break;
        }
    }

    if (scan_state != SCAN_HEX) {
        failed = !(scan_state & SCAN_NUMBER_STATES);
    } else {
        failed = count - 1 <= 2 || !(hex_state & HEX_NUMBER_STATES);
    }
    if (failed) {
        *chars_scanned = 0;
    } else {
        *chars_scanned = count - 1 + spaces;
    }
    (*ReadProc)(ReadProcArg, c, MSL_UNGET);

    if (hex_state == 0) {
        if (exp_negative) {
            exp = -exp;
        }
        len = d.sig.length;
        p = d.sig.text + len;
        while (len-- != 0 && *--p == '0') {
            dec_exp++;
        }
        d.sig.length = len + 1;
        if (d.sig.length == 0) {
            d.sig.text[d.sig.length++] = '0';
        }
        if (exp < -MAX_EXP || exp > MAX_EXP) {
            if (!*overflow && d.sig.text[0] == '0' && d.sig.text[1] == 0) {
                return 0.0;
            }
            *overflow = 1;
        }
        exp += dec_exp;
        if (*overflow) {
            if (exp_negative) {
                return 0.0;
            }
            if (negative) {
                return -HUGE_VAL;
            }
            return HUGE_VAL;
        }
        d.exp = exp;
        result = __dec2num(&d);
        if (result != 0.0 && result < DBL_MIN) {
            *overflow = 1;
        } else if (result > DBL_MAX) {
            *overflow = 1;
            result = HUGE_VAL;
        }
        if (negative && (scan_state & SCAN_NUMBER_STATES)) {
            result = -result;
        }
        return result;
    } else {
        double hex_value;
        unsigned char *dbl = (unsigned char *)&hex_value;
        int shift;
        int carry;
        unsigned i;
        unsigned j;
        unsigned k;
        unsigned bin_exp;
        int n;

        if (hex_exp_negative) {
            hex_exp = -hex_exp;
        }
        // The exponent is of the first bit of the first digit, so it moves by 4 for each digit before the point. The
        // mantissa is shifted left until its leading 1 is out, which is the one the double does not store.
        hex_exp += int_digits * 4;
        for (i = 0; i < 4 && !(mant[0] & (0x80 >> i)); i++) {
            hex_exp--;
        }
        hex_exp--;
        shift = i + 1;
        if (shift != 0) {
            carry = 0;
            for (cp = &mant[7]; cp >= mant; cp--) {
                int byte = *cp;

                *cp = carry | (byte << shift);
                carry = (byte >> (8 - shift)) & 0xFF;
            }
        }
        // The 52 bits that are left go after the 12 of the sign and the exponent, from the top byte down
        memset(dbl, 0, 8);
        for (j = 0, k = 12; j < 52; j += 8, k += 8) {
            int byte = mant[j / 8];

            if (j + 8 > 52) {
                byte &= (0xFF << (52 - j)) & 0xFF;
            }
            dbl[k / 8] |= byte >> (k % 8);
            dbl[(k + 8) / 8] |= byte << (8 - k % 8);
        }
        bin_exp = exp + 0x3FF + hex_exp;
        if (bin_exp & ~0x7FF) {
            *overflow = 1;
            return 0.0;
        }
        dbl[0] |= (bin_exp >> 4) & 0x7F;
        dbl[1] |= bin_exp << 4;
        if (negative) {
            dbl[0] |= 0x80;
        }
        // The bytes were built from the top of the double down, and the double is stored little-endian
        for (n = 0; n < 4; n++) {
            unsigned char t = dbl[n];

            dbl[n] = dbl[7 - n];
            dbl[7 - n] = t;
        }
        return *(double *)dbl;
    }
}

double strtod(const char *str, char **end) {
    double value;
    int count;
    int overflow;
    __InStringStruct isc;

    isc.NextChar = str;
    isc.NullCharDetected = 0;
    value = __strtold(0x7FFFFFFF, __StringRead, &isc, &count, &overflow);
    if (end != NULL) {
        *end = (char *)str + count;
    }
    // The range check of the value is gone: only the call of fabs is left
    fabs(value);
    if (overflow) {
        errno = ERANGE;
    }
    return value;
}

double atof(const char *str) {
    return strtod(str, NULL);
}
