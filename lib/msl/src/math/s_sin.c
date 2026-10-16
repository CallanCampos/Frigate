// MSL C's math/s_sin.c, Sun's fdlibm sin: the sine of x. A x within pi / 4 goes straight to __kernel_sin (with no
// tail); an infinity or a NaN gives a NaN (x - x); any other x is reduced by __ieee754_rem_pio2 to y[0] + y[1] in
// [-pi / 4, pi / 4] and a quadrant n, and the sine is one of sin(y), cos(y), -sin(y) and -cos(y) by n % 4.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

double sin(double x) {
    double y[2], z = 0.0;
    int n, ix;

    ix = __HI(x) & 0x7fffffff;

    if (ix <= 0x3fe921fb) {
        return __kernel_sin(x, z, 0);
    } else if (ix >= 0x7ff00000) {
        return x - x;
    } else {
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
        case 0:
            return __kernel_sin(y[0], y[1], 1);
        case 1:
            return __kernel_cos(y[0], y[1]);
        case 2:
            return -__kernel_sin(y[0], y[1], 1);
        default:
            return -__kernel_cos(y[0], y[1]);
        }
    }
}
