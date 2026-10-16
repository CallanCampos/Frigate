// MSL C's math/s_frexp.c, Sun's fdlibm frexp: splits x into a fraction in [0.5, 1) with the sign of x and a power of
// two, stored in *eptr, so that x = fraction * 2**exponent. Zero, infinities and NaNs come back as they are with an
// exponent of 0; a subnormal x is first scaled by 2**54 (and the exponent corrected by -54).
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

static const double two54 = 1.80143985094819840000e+16;

double frexp(double x, int *eptr) {
    int hx, ix, lx;

    hx = __HI(x);
    ix = 0x7fffffff & hx;
    lx = __LO(x);
    *eptr = 0;
    if (ix >= 0x7ff00000 || ((ix | lx) == 0)) {
        return x;
    }
    if (ix < 0x00100000) {
        // subnormal
        x *= two54;
        hx = __HI(x);
        ix = hx & 0x7fffffff;
        *eptr = -54;
    }
    *eptr += (ix >> 20) - 1022;
    hx = (hx & 0x800fffff) | 0x3fe00000;
    __HI(x) = hx;
    return x;
}
