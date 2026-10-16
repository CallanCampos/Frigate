// MSL C's math/s_cos.c, Sun's fdlibm cos: the cosine of x. A x within pi / 4 goes straight to __kernel_cos; an
// infinity or a NaN gives a NaN (x - x); any other x is reduced by __ieee754_rem_pio2 to y[0] + y[1] in [-pi / 4,
// pi / 4] and a quadrant n, and the cosine is one of cos(y), -sin(y), -cos(y) and sin(y) by n % 4.
#include "../fdlibm.h"
#include "math.h"

double cos(double x) {
    double y[2], z = 0.0;
    int n, ix;

    ix = __HI(x) & 0x7fffffff;

    if (ix <= 0x3fe921fb) {
        return __kernel_cos(x, z);
    } else if (ix >= 0x7ff00000) {
        return x - x;
    } else {
        n = __ieee754_rem_pio2(x, y);
        switch (n & 3) {
        case 0:
            return __kernel_cos(y[0], y[1]);
        case 1:
            return -__kernel_sin(y[0], y[1], 1);
        case 2:
            return -__kernel_cos(y[0], y[1]);
        default:
            return __kernel_sin(y[0], y[1], 1);
        }
    }
}
