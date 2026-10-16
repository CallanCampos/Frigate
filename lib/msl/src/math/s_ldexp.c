// MSL C's math/s_ldexp.c, ldexp: x * 2**n, which is fdlibm's scalbn behind a check for a zero, an infinity or a NaN
// (x comes back as it is). It moves the exponent of x by n, with the steps for a subnormal x (scaled by 2**54 first),
// for an overflow (huge, with the sign of x; also for a large n after the exponent underflows), for an underflow (tiny,
// with the sign) and for a subnormal result (the exponent moved to 54 above and the value scaled by 2**-54).
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

static const double two54 = 1.80143985094819840000e+16;
static const double twom54 = 5.55111512312578270212e-17;
static const double huge = 1.0e300;
static const double tiny = 1.0e-300;

double ldexp(double x, int n) {
    int k, hx, lx;

    if (__fpclassifyd(x) <= FP_INFINITE || x == 0.0) {
        return x;
    }

    hx = __HI(x);
    lx = __LO(x);
    k = (hx & 0x7ff00000) >> 20;
    if (k == 0) {
        // 0 or subnormal x
        if ((lx | (hx & 0x7fffffff)) == 0) {
            return x;
        }
        x *= two54;
        hx = __HI(x);
        k = ((hx & 0x7ff00000) >> 20) - 54;
        if (n < -50000) {
            return tiny * x;
        }
    }
    if (k == 0x7ff) {
        return x + x;
    }
    k = k + n;
    if (k > 0x7fe) {
        return huge * copysign(huge, x);
    }
    if (k > 0) {
        // normal result
        __HI(x) = (hx & 0x800fffff) | (k << 20);
        return x;
    }
    if (k <= -54) {
        if (n > 50000) {
            return huge * copysign(huge, x);
        } else {
            return tiny * copysign(tiny, x);
        }
    }
    // subnormal result
    k += 54;
    __HI(x) = (hx & 0x800fffff) | (k << 20);
    return twom54 * x;
}
