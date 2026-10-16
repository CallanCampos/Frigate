#ifndef MSL_FDLIBM_H
#define MSL_FDLIBM_H

// Private to MSL's math functions: the access to the halves of a double that the fdlibm sources use. The target is
// little-endian, so the high word is the second one.

#define __HI(x) *(1 + (int *)&x)
#define __LO(x) *(int *)&x

// The fdlibm functions that math.h's wrappers and the other fdlibm functions call
double __ieee754_pow(double x, double y);
int __ieee754_rem_pio2(double x, double *y);
double __kernel_cos(double x, double y);
double __kernel_sin(double x, double y, int iy);
int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2);

#endif // MSL_FDLIBM_H
