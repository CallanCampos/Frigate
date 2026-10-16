// MSL C's math/k_sin.c, Sun's fdlibm __kernel_sin: the sine of x for |x| up to pi / 4, where y is the tail of x (the
// argument is x + y, as __ieee754_rem_pio2 splits it) and iy says whether there is one: with iy == 0 the tail is
// ignored. It is the polynomial of the series in x * x, times x * x * x, added to x. cos and sin call it.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

static const double half = 5.00000000000000000000e-01;
// The coefficients of the polynomial of sin
static const double S1 = -1.66666666666666324348e-01;
static const double S2 = 8.33333333332248946124e-03;
static const double S3 = -1.98412698298579493134e-04;
static const double S4 = 2.75573137070700676789e-06;
static const double S5 = -2.50507602534068634195e-08;
static const double S6 = 1.58969099521155010221e-10;

double __kernel_sin(double x, double y, int iy) {
    double z, r, v;
    int ix;

    ix = __HI(x) & 0x7fffffff;

    // |x| < 2**-27: the sine is x if x is not exact enough to matter
    if (ix < 0x3e400000) {
        if ((int)x == 0) {
            return x;
        }
    }

    z = x * x;
    v = z * x;
    r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
    if (iy == 0) {
        return x + v * (S1 + z * r);
    } else {
        return x - ((z * (half * y - v * r) - y) - v * S1);
    }
}
