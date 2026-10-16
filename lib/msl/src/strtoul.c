// MSL C's strtoul.c: the integer conversions of <stdlib.h>. __strtoul and __strtoull scan the number as a state
// machine on its characters (white space and sign, the base prefix, the digits) through a read function, which
// strtoul and strtol (and strtoll, with __strtoull) give a string to read and turn the scan into their result. atol
// and atoll are strtol and strtoll in base 10. The ROM's order differs from the library's other builds, which put
// __strtoull next to __strtoul.
#include "types.h"
#include "scanf.h"
#include "strtoul.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// The states of the scan. They are bits so that the loop and the end can test several at once.
enum {
    SCAN_START = 0x01,      // white space, then the sign
    SCAN_CHECK_ZERO = 0x02, // the first character after the sign: a 0 may start a prefix
    SCAN_ZERO = 0x04,       // after that 0: an x makes the base 16
    SCAN_FIRST_DIGIT = 0x08,
    SCAN_DIGITS = 0x10,
    SCAN_FINISHED = 0x20, // a character that is not a digit ended the number
    SCAN_FAILED = 0x40    // the text is not a number
};

// The states that leave a number: the scan ended at its digits, or at the 0 that was all of it
#define SCAN_NUMBER_STATES (SCAN_ZERO | SCAN_DIGITS | SCAN_FINISHED)

unsigned long __strtoul(int base, int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg,
                        int *chars_scanned, int *negative, int *overflow) {
    int scan_state = SCAN_START;
    int count = 0;
    int spaces = 0;
    // Not read yet when the base or the width is invalid: the loop test then looks at it, but the failed state ends the
    // loop whatever it holds
    int c;
    unsigned long value = 0;
    unsigned long max_by_base = 0; // the largest value that another digit can still be added to

    *overflow = 0;
    *negative = 0;

    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        scan_state = SCAN_FAILED;
    } else {
        count++;
        c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
    }
    if (base != 0) {
        max_by_base = ULONG_MAX / base;
    }

    while (count <= max_width && c != EOF && !(scan_state & (SCAN_FINISHED | SCAN_FAILED))) {
        switch (scan_state) {
        case SCAN_START:
            if (isspace(c)) {
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                spaces++;
                break;
            }
            if (c == '+') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            } else if (c == '-') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                *negative = 1;
            }
            scan_state = SCAN_CHECK_ZERO;
            break;

        case SCAN_CHECK_ZERO:
            if ((base == 0 || base == 16) && c == '0') {
                scan_state = SCAN_ZERO;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            scan_state = SCAN_FIRST_DIGIT;
            break;

        case SCAN_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                scan_state = SCAN_FIRST_DIGIT;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            if (base == 0) {
                base = 8;
            }
            scan_state = SCAN_DIGITS;
            break;

        case SCAN_FIRST_DIGIT:
        case SCAN_DIGITS:
            if (base == 0) {
                base = 10;
            }
            if (max_by_base == 0) {
                max_by_base = ULONG_MAX / base;
            }
            if (isdigit(c)) {
                c -= '0';
                if (c >= base) {
                    if (scan_state == SCAN_DIGITS) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILED;
                    }
                    c += '0';
                    break;
                }
            } else {
                if (!isalpha(c) || toupper(c) - 'A' + 10 >= base) {
                    if (scan_state == SCAN_DIGITS) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILED;
                    }
                    break;
                }
                c = toupper(c) - 'A' + 10;
            }
            if (value > max_by_base) {
                *overflow = 1;
            }
            value *= base;
            if (c > ULONG_MAX - value) {
                *overflow = 1;
            }
            value += c;
            scan_state = SCAN_DIGITS;
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;
        }
    }

    if (!(scan_state & SCAN_NUMBER_STATES)) {
        value = 0;
        *chars_scanned = 0;
    } else {
        *chars_scanned = count - 1 + spaces;
    }
    (*ReadProc)(ReadProcArg, c, MSL_UNGET);
    return value;
}

unsigned long strtoul(const char *str, char **end, int base) {
    unsigned long value;
    int count, negative, overflow;
    __InStringStruct isc;

    isc.NextChar = str;
    isc.NullCharDetected = 0;
    value = __strtoul(base, INT_MAX, __StringRead, &isc, &count, &negative, &overflow);
    if (end) {
        *end = (char *)str + count;
    }
    if (overflow) {
        value = ULONG_MAX;
        errno = ERANGE;
    } else {
        value = negative ? -value : value;
    }
    return value;
}

long strtol(const char *str, char **end, int base) {
    unsigned long value;
    int count, negative, overflow;
    __InStringStruct isc;

    isc.NextChar = str;
    isc.NullCharDetected = 0;
    value = __strtoul(base, INT_MAX, __StringRead, &isc, &count, &negative, &overflow);
    if (end) {
        *end = (char *)str + count;
    }
    if (overflow || (!negative && value > LONG_MAX) || (negative && value > (unsigned long)LONG_MIN)) {
        value = negative ? LONG_MIN : LONG_MAX;
        errno = ERANGE;
    } else {
        value = negative ? -value : value;
    }
    return value;
}

unsigned long long __strtoull(int base, int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg,
                              int *chars_scanned, int *negative, int *overflow) {
    int scan_state = SCAN_START;
    int count = 0;
    int spaces = 0;
    // Not read yet when the base or the width is invalid: the loop test then looks at it, but the failed state ends the
    // loop whatever it holds
    int c;
    unsigned long long value = 0;
    unsigned long long max_by_base = 0; // the largest value that another digit can still be added to

    *overflow = 0;
    *negative = 0;

    if (base < 0 || base == 1 || base > 36 || max_width < 1) {
        scan_state = SCAN_FAILED;
    } else {
        count++;
        c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
    }
    if (base != 0) {
        max_by_base = ULLONG_MAX / base;
    }

    while (count <= max_width && c != EOF && !(scan_state & (SCAN_FINISHED | SCAN_FAILED))) {
        switch (scan_state) {
        case SCAN_START:
            if (isspace(c)) {
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                spaces++;
                break;
            }
            if (c == '+') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            } else if (c == '-') {
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                *negative = 1;
            }
            scan_state = SCAN_CHECK_ZERO;
            break;

        case SCAN_CHECK_ZERO:
            if ((base == 0 || base == 16) && c == '0') {
                scan_state = SCAN_ZERO;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            scan_state = SCAN_FIRST_DIGIT;
            break;

        case SCAN_ZERO:
            if (c == 'X' || c == 'x') {
                base = 16;
                scan_state = SCAN_FIRST_DIGIT;
                count++;
                c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
                break;
            }
            if (base == 0) {
                base = 8;
            }
            scan_state = SCAN_DIGITS;
            break;

        case SCAN_FIRST_DIGIT:
        case SCAN_DIGITS:
            if (base == 0) {
                base = 10;
            }
            if (max_by_base == 0) {
                max_by_base = ULLONG_MAX / base;
            }
            if (isdigit(c)) {
                c -= '0';
                if (c >= base) {
                    if (scan_state == SCAN_DIGITS) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILED;
                    }
                    c += '0';
                    break;
                }
            } else {
                if (!isalpha(c) || toupper(c) - 'A' + 10 >= base) {
                    if (scan_state == SCAN_DIGITS) {
                        scan_state = SCAN_FINISHED;
                    } else {
                        scan_state = SCAN_FAILED;
                    }
                    break;
                }
                c = toupper(c) - 'A' + 10;
            }
            if (value > max_by_base) {
                *overflow = 1;
            }
            value *= base;
            if (c > ULLONG_MAX - value) {
                *overflow = 1;
            }
            value += c;
            scan_state = SCAN_DIGITS;
            count++;
            c = (*ReadProc)(ReadProcArg, 0, MSL_GET);
            break;
        }
    }

    if (!(scan_state & SCAN_NUMBER_STATES)) {
        value = 0;
        *chars_scanned = 0;
    } else {
        *chars_scanned = count - 1 + spaces;
    }
    (*ReadProc)(ReadProcArg, c, MSL_UNGET);
    return value;
}

long long strtoll(const char *str, char **end, int base) {
    unsigned long long value;
    int count, negative, overflow;
    __InStringStruct isc;

    isc.NextChar = str;
    isc.NullCharDetected = 0;
    value = __strtoull(base, INT_MAX, __StringRead, &isc, &count, &negative, &overflow);
    if (end) {
        *end = (char *)str + count;
    }
    if (overflow || (!negative && value > LLONG_MAX) || (negative && value > (unsigned long long)LLONG_MIN)) {
        value = negative ? LLONG_MIN : LLONG_MAX;
        errno = ERANGE;
    } else {
        value = negative ? -value : value;
    }
    return value;
}

long atol(const char *str) {
    return strtol(str, NULL, 10);
}

long long atoll(const char *str) {
    return strtoll(str, NULL, 10);
}
