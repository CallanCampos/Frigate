// MSL C's math_api.c: the helpers behind <math.h>'s classification of a double, which the number-to-string conversions
// of the printf and scanf code call.
#include "fdlibm.h"
#include "math.h"

// The number of set bits of a 64-bit value, by the tree of partial sums
int __msl_generic_count_bits64(unsigned long long x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x += x >> 8;
    x += x >> 16;
    x += x >> 32;
    return (int)(x & 0xFF);
}

// Whether the sign bit is set, which is also true of -0.0 and of a NaN with the sign set
int __signbitd(double x) {
    return (__HI(x) & 0x80000000) ? 1 : 0;
}

// One of the FP_* classes of x, from its exponent and mantissa
int __fpclassifyd(double x) {
    switch (__HI(x) & 0x7ff00000) {
    case 0x7ff00000:
        return ((__HI(x) & 0x000fffff) || __LO(x)) ? FP_NAN : FP_INFINITE;
    case 0:
        return ((__HI(x) & 0x000fffff) || __LO(x)) ? FP_SUBNORMAL : FP_ZERO;
    }
    return FP_NORMAL;
}
