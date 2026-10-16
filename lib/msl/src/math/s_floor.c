// MSL C's math/s_floor.c, Sun's fdlibm floor: the largest integer not above x, as a double. It works on the two words
// of x by the exponent j0: for |x| < 1 the result is 0 or -1 by the sign; for |x| < 2**20 the fraction bits of the high
// word are cleared (one is added first for a negative x that has any), and the low word is cleared; up to 2**52 it is
// the low word's fraction bits that go, with a carry into the high word for a negative x; from there on x is already
// an integer, or an infinity or a NaN (x + x). The addition of huge is only there to raise the inexact flag.
#include "../fdlibm.h"
#include "math.h"

static const double huge = 1.0e300;

double floor(double x) {
    int i0, i1, j0;
    unsigned i, j;

    i0 = __HI(x);
    i1 = __LO(x);
    j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
    if (j0 < 20) {
        if (j0 < 0) {
            // |x| < 1
            if (huge + x > 0.0) {
                if (i0 >= 0) {
                    i0 = i1 = 0;
                } else if (((i0 & 0x7fffffff) | i1) != 0) {
                    i0 = 0xbff00000;
                    i1 = 0;
                }
            }
        } else {
            i = 0x000fffff >> j0;
            if (((i0 & i) | i1) == 0) {
                // x is an integer
                return x;
            }
            if (huge + x > 0.0) {
                if (i0 < 0) {
                    i0 += 0x00100000 >> j0;
                }
                i0 &= ~i;
                i1 = 0;
            }
        }
    } else if (j0 > 51) {
        if (j0 == 0x400) {
            // infinity or NaN
            return x + x;
        } else {
            // x is an integer
            return x;
        }
    } else {
        i = 0xffffffffU >> (j0 - 20);
        if ((i1 & i) == 0) {
            // x is an integer
            return x;
        }
        if (huge + x > 0.0) {
            if (i0 < 0) {
                if (j0 == 20) {
                    i0 += 1;
                } else {
                    j = i1 + (1 << (52 - j0));
                    if (j < i1) {
                        // carry
                        i0 += 1;
                    }
                    i1 = j;
                }
            }
            i1 &= ~i;
        }
    }
    __HI(x) = i0;
    __LO(x) = i1;
    return x;
}
