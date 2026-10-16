#ifndef POKEBW2_MATH_H
#define POKEBW2_MATH_H

#include <float.h>

// The C library's math functions, from CodeWarrior's MSL, which the main module links

// The classes that __fpclassifyd returns
#define FP_NAN 1
#define FP_INFINITE 2
#define FP_ZERO 3
#define FP_NORMAL 4
#define FP_SUBNORMAL 5

// The bit patterns of float.c (float.h), read as the float or double that they stand for
#define INFINITY (*(float *)__float_huge)
#define NAN (*(float *)__float_nan)
#define HUGE_VAL (*(double *)__double_huge)

double copysign(double x, double y);
double cos(double x);
double fabs(double x);
double floor(double x);
double frexp(double x, int *eptr);
double ldexp(double x, int n);
double nan(const char *tagp);
double pow(double x, double y);
double scalbn(double x, int n);
double sin(double x);
double sqrt(double x);

// MSL's own classification helpers, which printf and scanf call
int __msl_generic_count_bits64(unsigned long long x);
int __signbitd(double x);
int __fpclassifyd(double x);

#endif // POKEBW2_MATH_H
