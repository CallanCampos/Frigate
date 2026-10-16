// MSL C's math/w_pow.c, Sun's fdlibm wrapper for pow: the C library's pow, a tail call of __ieee754_pow. (The Sun
// wrapper's checks for a domain or range error are not in this build; __ieee754_pow sets errno itself.)
#include "../fdlibm.h"
#include "math.h"

double pow(double x, double y) {
    return __ieee754_pow(x, y);
}
