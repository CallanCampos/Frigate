// MSL C's ansi_fp.c (named after the MSL source file, from swan's names for its functions): the conversion between a
// double and its decimal digits, which printf's floating-point formats and scanf's strtold use. A double becomes a
// decimal by scaling its mantissa to an integer and multiplying by the power of two that it stands for (as a decimal
// too); the way back sums the digits eight at a time, scales by a power of five and of two, then steps the double up
// or down until its own decimal is the closest to the digits.
#include "ansi_fp.h"
#include <float.h>
#include <math.h>

// BUG: __str2dec compares the first digit that it cuts off, a character, with 5 instead of '5', so every cut-off
// remainder is over half and rounds up
#ifdef BUGFIX
#define HALF_DIGIT '5'
#else
#define HALF_DIGIT 5
#endif

// The powers of ten that __dec2num scales its running value by, for up to eight digits at a time
static double pow_10[8] = { 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8 };

static int __must_round(decimal *d, int digits);
static void __dorounddecup(decimal *d, int digits);
static void __rounddec(decimal *d, int digits);

// Whether cutting d to its first digits rounds the last of them up: 1 when the digits cut off are over half and -1 when
// they are under, while exactly half rounds to the even digit (1 only when the last digit kept is odd)
static int __must_round(decimal *d, int digits) {
    unsigned char *p = d->sig.text + digits;
    unsigned char *end;

    if (*p > 5) {
        return 1;
    }
    if (*p < 5) {
        return -1;
    }
    end = d->sig.text + d->sig.length;
    for (p++; p < end; p++) {
        if (*p != 0) {
            return 1;
        }
    }
    if (d->sig.text[digits - 1] & 1) {
        return 1;
    }
    return -1;
}

// Adds one to the last of the first digits of d, carrying through the nines (the exponent grows when they are all
// nines)
static void __dorounddecup(decimal *d, int digits) {
    unsigned char *b = d->sig.text;
    unsigned char *p = b + digits - 1;

    for (;;) {
        if (*p < 9) {
            (*p)++;
            return;
        }
        if (p == b) {
            *p = 1;
            d->exp++;
            return;
        }
        *p-- = 0;
    }
}

// Cuts d to its first digits, rounding it to the nearest
static void __rounddec(decimal *d, int digits) {
    int round;

    if (digits <= 0) {
        return;
    }
    if (digits >= d->sig.length) {
        return;
    }
    round = __must_round(d, digits);
    d->sig.length = digits;
    if (round >= 0) {
        __dorounddecup(d, digits);
    }
}

// The decimal of an unsigned 64-bit integer: its digits from the least significant one, reversed
void __ull2dec(decimal *result, unsigned long long val) {
    unsigned char *i;
    unsigned char *j;

    result->sign = 0;
    result->sig.length = 0;
    for (; val != 0; val /= 10) {
        result->sig.text[result->sig.length++] = val % 10;
    }
    i = result->sig.text;
    j = i + result->sig.length - 1;
    for (; i < j; i++, j--) {
        unsigned char t = *i;
        *i = *j;
        *j = t;
    }
    result->exp = result->sig.length - 1;
}

// result = x * y, schoolbook: the products of one column of digits are summed at a time from the least significant
// column, and the digits that do not fit in SIGDIGLEN round the result
void __timesdec(decimal *result, const decimal *x, const decimal *y) {
    unsigned long accumulator = 0;
    unsigned char mantissa[2 * SIGDIGLEN];
    int i = x->sig.length + y->sig.length - 1;
    int n;
    unsigned char *p = mantissa + i + 1;
    unsigned char *end = p;
    int j;

    result->sign = 0;
    for (; i > 0; i--) {
        int k;
        int count;
        const unsigned char *xp;
        const unsigned char *yp;
        // The column's products pair x[k + m] with y[j - m], where k + j + 1 is i
        j = y->sig.length - 1;
        k = i - j - 1;
        if (k < 0) {
            k = 0;
            j = i - 1;
        }
        xp = x->sig.text + k;
        yp = y->sig.text + j;
        count = j + 1;
        if (count > x->sig.length - k) {
            count = x->sig.length - k;
        }
        for (; count > 0; count--) {
            accumulator += *xp * *yp;
            xp++;
            yp--;
        }
        *--p = accumulator % 10;
        accumulator /= 10;
    }
    result->exp = x->exp + y->exp;
    if (accumulator != 0) {
        *--p = accumulator;
        result->exp++;
    }
    for (n = 0; n < SIGDIGLEN && p < end; n++) {
        result->sig.text[n] = *p;
        p++;
    }
    result->sig.length = n;
    if (p < end) {
        if (*p < 5) {
            return;
        }
        if (*p == 5) {
            unsigned char *q;

            for (q = p + 1; q < end; q++) {
                if (*q != 0) {
                    goto round;
                }
            }
            if (!(p[-1] & 1)) {
                return;
            }
        }
    round:
        __dorounddecup(result, result->sig.length);
    }
}

// The decimal of the digit string s, of characters, with exp as the exponent of its first digit; the digits past
// SIGDIGLEN round it
void __str2dec(decimal *d, const char *s, short exp) {
    int i;

    d->exp = exp;
    d->sign = 0;
    for (i = 0; i < SIGDIGLEN && *s != '\0'; i++) {
        d->sig.text[i] = *s++ - '0';
    }
    d->sig.length = i;
    if (*s == '\0') {
        return;
    }
    if (*s < HALF_DIGIT) {
        return;
    }
    if (*s <= HALF_DIGIT) {
        const char *t;

        for (t = s + 1; *t != '\0'; t++) {
            if (*t != '0') {
                goto round;
            }
        }
        if (!(d->sig.text[i - 1] & 1)) {
            return;
        }
    }
round:
    __dorounddecup(d, d->sig.length);
}

// The decimal of two to the exp: from the digits of the powers that it knows (-64, -53, -32, -16 and -8 to 8), by
// squaring the power of half the exponent for the others
void __two_exp(decimal *result, int exp) {
    decimal y;
    decimal x;
    int neg;

    switch (exp) {
    case -64:
        __str2dec(result, "542101086242752217003726400434970855712890625", -20);
        return;
    case -53:
        __str2dec(result, "11102230246251565404236316680908203125", -16);
        return;
    case -32:
        __str2dec(result, "23283064365386962890625", -10);
        return;
    case -16:
        __str2dec(result, "152587890625", -5);
        return;
    case -8:
        __str2dec(result, "390625", -3);
        return;
    case -7:
        __str2dec(result, "78125", -3);
        return;
    case -6:
        __str2dec(result, "15625", -2);
        return;
    case -5:
        __str2dec(result, "3125", -2);
        return;
    case -4:
        __str2dec(result, "625", -2);
        return;
    case -3:
        __str2dec(result, "125", -1);
        return;
    case -2:
        __str2dec(result, "25", -1);
        return;
    case -1:
        __str2dec(result, "5", -1);
        return;
    case 0:
        __str2dec(result, "1", 0);
        return;
    case 1:
        __str2dec(result, "2", 0);
        return;
    case 2:
        __str2dec(result, "4", 0);
        return;
    case 3:
        __str2dec(result, "8", 0);
        return;
    case 4:
        __str2dec(result, "16", 1);
        return;
    case 5:
        __str2dec(result, "32", 1);
        return;
    case 6:
        __str2dec(result, "64", 1);
        return;
    case 7:
        __str2dec(result, "128", 2);
        return;
    case 8:
        __str2dec(result, "256", 2);
        return;
    }
    // The halving rounds toward zero, which the odd case below depends on: the sign bit is added before the shift
    neg = (exp & 0x80000000) >> 31;
    __two_exp(&y, (exp + neg) >> 1);
    __timesdec(result, &y, &y);
    if (exp & 1) {
        x = *result;
        if (exp > 0) {
            __str2dec(&y, "2", 0);
        } else {
            __str2dec(&y, "5", -1);
        }
        __timesdec(result, &x, &y);
    }
}

// Whether x and y are the same number
int __equals_dec(const decimal *x, const decimal *y) {
    int i;
    int n;

    if (x->sig.text[0] == 0) {
        return y->sig.text[0] == 0;
    }
    if (y->sig.text[0] == 0) {
        return x->sig.text[0] == 0;
    }
    if (x->exp == y->exp) {
        n = x->sig.length;
        if (n > y->sig.length) {
            n = y->sig.length;
        }
        for (i = 0; i < n; i++) {
            if (x->sig.text[i] != y->sig.text[i]) {
                return 0;
            }
        }
        if (n == x->sig.length) {
            x = y;
        }
        for (; i < x->sig.length; i++) {
            if (x->sig.text[i] != 0) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

// Whether x is less than y
int __less_dec(const decimal *x, const decimal *y) {
    int i;
    int n;

    if (x->sig.text[0] == 0) {
        return y->sig.text[0] != 0;
    }
    if (y->sig.text[0] == 0) {
        return 0;
    }
    if (x->exp == y->exp) {
        n = x->sig.length;
        if (n > y->sig.length) {
            n = y->sig.length;
        }
        for (i = 0; i < n; i++) {
            if (x->sig.text[i] < y->sig.text[i]) {
                return 1;
            }
            if (y->sig.text[i] < x->sig.text[i]) {
                return 0;
            }
        }
        if (n == x->sig.length) {
            for (; i < y->sig.length; i++) {
                if (y->sig.text[i] != 0) {
                    return 1;
                }
            }
        }
        return 0;
    }
    return x->exp < y->exp;
}

// Borrows for the digit i: one comes off the nearest non-zero digit to its left, the zeros in between become nines and
// i gets ten
static inline void __borrow(unsigned char *i) {
    unsigned char *k = i - 1;

    while (*k == 0) {
        k--;
    }
    while (k != i) {
        (*k)--;
        k++;
        *k += 10;
    }
}

// result = x - y, for x not less than y: the digits of y are subtracted from those of x column by column, with the
// borrow from the left, and the digits of y that do not fit in SIGDIGLEN round the difference
void __minus_dec(decimal *result, const decimal *x, const decimal *y) {
    unsigned char *end;
    int round;
    int shift;
    const unsigned char *ytext;
    unsigned char *rtext;
    unsigned char skip;
    int n;
    unsigned char *i;
    const unsigned char *j;
    unsigned char *k;
    const unsigned char *jend;
    int used;

    *result = *x;
    if (y->sig.text[0] == 0) {
        return;
    }
    n = result->sig.length;
    if (n < y->sig.length) {
        n = y->sig.length;
    }
    shift = result->exp - y->exp;
    n += shift;
    if (n > SIGDIGLEN) {
        n = SIGDIGLEN;
    }
    while (result->sig.length < n) {
        result->sig.text[result->sig.length++] = 0;
    }
    rtext = result->sig.text;
    i = rtext + n;
    if (y->sig.length + shift < n) {
        i = rtext + (y->sig.length + shift);
    }
    ytext = y->sig.text;
    j = ytext + (i - rtext - shift);
    jend = j;
    while (i > rtext && j > ytext) {
        i--;
        j--;
        if (*i < *j) {
            __borrow(i);
        }
        *i -= *j;
    }
    used = jend - ytext;
    if (used < y->sig.length) {
        // BUG: the digits of y that did not fit are meant to round the difference, which takes one off its last digit
        // when they are over half. This takes it off when they are under half (and not when they are over, or when
        // they are half and more), and it takes it off the digit where the loop above stopped, not the last one, in
        // every case but the exact half
        round = 0;
#ifdef BUGFIX
        i = rtext + used + shift - 1;
        if (*jend > 5) {
            round = 1;
        } else if (*jend == 5) {
            const unsigned char *yend = y->sig.text + y->sig.length;
            const unsigned char *q;

            round = *i & 1;
            for (q = jend + 1; q < yend; q++) {
                if (*q != 0) {
                    round = 1;
                }
            }
        }
#else
        if (*jend < 5) {
            round = 1;
        } else if (*jend == 5) {
            const unsigned char *yend = y->sig.text + y->sig.length;
            const unsigned char *q;

            for (q = jend + 1; q < yend; q++) {
                if (*q != 0) {
                    goto done;
                }
            }
            i = rtext + used + shift - 1;
            if (*i & 1) {
                round = 1;
            }
        }
#endif
        if (round) {
            if (*i < 1) {
                __borrow(i);
            }
            (*i)--;
        }
    }
#ifndef BUGFIX
done:
#endif
    for (k = rtext; *k == 0; k++) {
    }
    if (k > rtext) {
        skip = k - rtext;
        result->exp -= skip;
        end = rtext + result->sig.length;
        while (k < end) {
            *rtext = *k;
            rtext++;
            k++;
        }
        result->sig.length -= skip;
    }
    rtext = result->sig.text;
    k = rtext + result->sig.length;
    while (k > rtext) {
        if (*--k != 0) {
            break;
        }
    }
    result->sig.length = k - rtext + 1;
}

// The decimal of x: the mantissa as an integer, times the power of two that scales it
void __num2dec_internal(decimal *d, double x) {
    char sign = __signbitd(x) ? 1 : 0;
    decimal digits;
    decimal two;
    int n;
    long long bits;
    double f;
    int e;
    union {
        double d;
        long long ll;
    } u;

    if (x == 0.0) {
        d->sign = sign;
        d->exp = 0;
        d->sig.length = 1;
        d->sig.text[0] = 0;
        return;
    }
    if (__fpclassifyd(x) <= FP_INFINITE) {
        d->sign = sign;
        d->exp = 0;
        d->sig.length = 1;
        d->sig.text[0] = __fpclassifyd(x) == FP_NAN ? 'N' : 'I';
        return;
    }
    if (sign) {
        x = -x;
    }
    f = frexp(x, &e);
    u.d = f;
    bits = u.ll | 0x0010000000000000LL;
    n = 53 - __msl_generic_count_bits64((bits & -bits) - 1);
    __two_exp(&two, e - n);
    __ull2dec(&digits, (unsigned long long)ldexp(f, n));
    __timesdec(d, &digits, &two);
    d->sign = sign;
}

// Fills d with the decimal of x, rounded to the number of digits that form asks for, as characters
void __num2dec(const decform *form, double x, decimal *d) {
    short digits = form->digits;
    int i;

    __num2dec_internal(d, x);
    if (d->sig.text[0] > 9) {
        return;
    }
    if (digits > SIGDIGLEN) {
        digits = SIGDIGLEN;
    }
    __rounddec(d, digits);
    while (d->sig.length < digits) {
        d->sig.text[d->sig.length++] = 0;
    }
    d->exp -= d->sig.length - 1;
    for (i = 0; i < d->sig.length; i++) {
        d->sig.text[i] += '0';
    }
}

// The double that is closest to the decimal d: an estimate from the digits, then the neighbours of the estimate are
// compared by their own decimals
double __dec2num(const decimal *d) {
    decimal dec;
    decimal max;
    decimal tmp;
    decimal tmp2;
    decimal diff1;
    decimal diff2;
    double result;
    long long *bits;
    double val;
    double next;
    long long *nextbits;
    unsigned char *p;
    unsigned char *end;
    int exp10;
    int up;
    int down;

    if (d->sig.length == 0) {
        return copysign(0.0, d->sign == 0 ? 1.0 : -1.0);
    }
    switch (d->sig.text[0]) {
    case '0':
        return copysign(0.0, d->sign == 0 ? 1.0 : -1.0);
    case 'I':
        return copysign(INFINITY, d->sign == 0 ? 1.0 : -1.0);
    case 'N':
        bits = (long long *)&result;
        *bits = 0x7ff0000000000000LL;
        if (d->sign) {
            *bits |= 0x8000000000000000LL;
        }
        *bits |= 0x0008000000000000LL;
        return result;
    }
    dec = *d;
    end = dec.sig.text + dec.sig.length;
    for (p = dec.sig.text; p < end; p++) {
        *p -= '0';
    }
    dec.exp += dec.sig.length - 1;
    exp10 = dec.exp;
    __str2dec(&max, "179769313486231580793728714053034151", 308);
    if (__less_dec(&max, &dec)) {
        return copysign(INFINITY, d->sign == 0 ? 1.0 : -1.0);
    }
    p = dec.sig.text;
    val = *p++;
    while (p < end) {
        int i;
        unsigned long chunk;
        double prod;
        double sum;
        int n;

        n = (end - p) % 8;
        chunk = 0;
        if (n == 0) {
            n = 8;
        }
        for (i = 0; i < n; i++) {
            chunk = chunk * 10 + *p++;
        }
        prod = val * pow_10[n - 1];
        sum = prod + chunk;
        if (chunk != 0 && prod == sum) {
            break;
        }
        val = sum;
        exp10 -= n;
    }
    if (exp10 < 0) {
        val /= pow(5.0, -exp10);
    } else {
        val *= pow(5.0, exp10);
    }
    val = ldexp(val, exp10);
    if (__fpclassifyd(val) == FP_INFINITE) {
        val = DBL_MAX;
    }
    nextbits = (long long *)&next;
    up = 0;
    __num2dec_internal(&tmp, val);
    if (!__equals_dec(&tmp, &dec)) {
        if (__less_dec(&tmp, &dec)) {
            up = 1;
        }
        next = val;
        down = !up;
        for (;;) {
            if (!down) {
                *nextbits += 1;
                if (__fpclassifyd(next) == FP_INFINITE) {
                    goto done;
                }
            } else {
                *nextbits -= 1;
            }
            __num2dec_internal(&tmp2, next);
            if (up && !__less_dec(&tmp2, &dec)) {
                break;
            }
            if (!up && !__less_dec(&dec, &tmp2)) {
                double v;

                // diff1 is only the scratch of the swap here
                diff1 = tmp;
                tmp = tmp2;
                tmp2 = diff1;
                v = val;
                val = next;
                next = v;
                break;
            }
            tmp = tmp2;
            val = next;
        }
        __minus_dec(&diff1, &dec, &tmp);
        __minus_dec(&diff2, &tmp2, &dec);
        if (__equals_dec(&diff1, &diff2)) {
            if ((*(long long *)&val & 1) != 0) {
                val = next;
            }
        } else if (!__less_dec(&diff1, &diff2)) {
            val = next;
        }
    }
done:
    if (dec.sign) {
        val = -val;
    }
    return val;
}
