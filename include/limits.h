#ifndef POKEBW2_LIMITS_H
#define POKEBW2_LIMITS_H

// The C library's limits.h, from CodeWarrior's MSL: the ranges of the integer types, of which strtoul.c uses the int
// and long ones

#define INT_MAX 0x7FFFFFFF
#define LONG_MAX 0x7FFFFFFF
#define LONG_MIN (-LONG_MAX - 1)
#define ULONG_MAX 0xFFFFFFFFUL
#define LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
#define LLONG_MIN (-LLONG_MAX - 1)
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL

#endif // POKEBW2_LIMITS_H
