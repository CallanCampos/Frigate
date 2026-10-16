// MSL C's arith.c: the integer arithmetic of <stdlib.h>, which here is abs alone.
#include <stdlib.h>

int abs(int x) {
    return x >= 0 ? x : -x;
}
