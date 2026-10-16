// MSL C's math/s_copysign.c, Sun's fdlibm copysign: x with the sign of y.
// fdlibm's notice is in ../fdlibm.h.
#include "../fdlibm.h"
#include "math.h"

double copysign(double x, double y) {
    __HI(x) = (__HI(x) & 0x7fffffff) | (__HI(y) & 0x80000000);
    return x;
}
