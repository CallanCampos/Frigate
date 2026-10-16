// MSL C's math/s_fabs.c, Sun's fdlibm fabs: the absolute value of x, by clearing the sign bit. The ROM names this
// function floatAbs (swan's name); it is fabs, which e_pow, e_rem_pio2 and the printf and strtod code call.
#include "../fdlibm.h"
#include "math.h"

double fabs(double x) {
    __HI(x) &= 0x7fffffff;
    return x;
}
