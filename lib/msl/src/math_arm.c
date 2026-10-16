// MSL C's math_arm.c (the name is a guess from the ARM platform group of MSL_C_NITRO_Ai_LE.a, whose math object here is
// as long as nan alone): nan, which returns the library's NaN constant widened to a double.
#include "math.h"

double nan(const char *tagp) {
    return NAN;
}
