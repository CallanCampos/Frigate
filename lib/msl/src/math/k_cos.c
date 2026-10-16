// MSL C's math/k_cos.c, Sun's fdlibm __kernel_cos: the cosine of x for |x| up to pi / 4, where y is the tail of x
// (the argument is x + y, as __ieee754_rem_pio2 splits it). It is 1 - x * x / 2 plus the polynomial of the series in
// x * x. For |x| from 0.3 on, a part of x * x / 2 (qx, about |x| / 4 with its low word cleared) is taken off 1 on its
// own and off x * x / 2 too, so that the bits of the small terms are not lost in the subtraction from 1.
#include "../fdlibm.h"
#include "math.h"

static const double one = 1.0;
// The coefficients of the polynomial of cos
static const double C1 = 4.16666666666666019037e-02;
static const double C2 = -1.38888888888741095749e-03;
static const double C3 = 2.48015872894767294178e-05;
static const double C4 = -2.75573143513906633035e-07;
static const double C5 = 2.08757232129817482790e-09;
static const double C6 = -1.13596475577881948265e-11;

double __kernel_cos(double x, double y) {
    double a, hz, z, r, qx;
    int ix;

    ix = __HI(x) & 0x7fffffff;

    // |x| < 2**-27: the cosine is 1 if x is not exact enough to matter
    if (ix < 0x3e400000) {
        if ((int)x == 0) {
            return one;
        }
    }

    z = x * x;
    r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    if (ix < 0x3fd33333) {
        // |x| < 0.3
        return one - (0.5 * z - (z * r - x * y));
    } else {
        if (ix > 0x3fe90000) {
            // |x| > 0.78125
            qx = 0.28125;
        } else {
            // |x| / 4 with its low word cleared
            __HI(qx) = ix - 0x00200000;
            __LO(qx) = 0;
        }
        hz = 0.5 * z - qx;
        a = one - qx;
        return a - (hz - (z * r - x * y));
    }
}
