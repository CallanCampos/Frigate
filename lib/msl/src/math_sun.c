// MSL C's math_sun.c (name guessed from MSL's layout; the object is the 44 bytes of scalbn alone). MSL's scalbn is
// not fdlibm's bit-twiddling one: it splits x with frexp and joins it again with ldexp, the exponent moved by n.
#include "math.h"

// x * 2^n
double scalbn(double x, int n) {
    int e;

    x = frexp(x, &e);
    e += n;
    return ldexp(x, e);
}
