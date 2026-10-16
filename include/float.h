#ifndef POKEBW2_FLOAT_H
#define POKEBW2_FLOAT_H

// The C library's float.h, from CodeWarrior's MSL, which the main module links

// The bit patterns of float.c: the infinity and the NaN of a float and the infinity of a double, which math.h's
// INFINITY, NAN and HUGE_VAL read as the float or the double that they stand for
extern unsigned long __float_huge[];  // swan's inf
extern unsigned long __float_nan[];   // swan's float_high
extern unsigned long __double_huge[]; // swan's HUGE_VAL

// The largest finite double, and the smallest one that is normal
#define DBL_MAX 1.7976931348623157e+308
#define DBL_MIN 2.2250738585072014e-308

#endif // POKEBW2_FLOAT_H
