// MSL C's float.c: the bit patterns of the infinity and the NaN of a float and of the infinity of a double, which
// <math.h> turns into INFINITY, NAN and HUGE_VAL. A data-only object. The target is little-endian, so a double's
// high word comes second.
#include <float.h>

unsigned long __float_nan[] = { 0x7FFFFFFF };
unsigned long __float_huge[] = { 0x7F800000 };
unsigned long __double_huge[] = { 0, 0x7FF00000 };
