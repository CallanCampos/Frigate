#ifndef POKEBW2_ERRNO_H
#define POKEBW2_ERRNO_H

// The C library's errno.h, from CodeWarrior's MSL, which the main module links

// The result of the last C library call that failed, one for the whole program (errno.c)
extern int errno;

// The errors that the library stores: a domain error (pow, for a negative base and a power that is not an integer),
// a result that doesn't fit its type (strtod and strtoul) and a stream that can't be positioned (file_pos.c)
#define EDOM 33
#define ERANGE 34
#define EFPOS 40

#endif // POKEBW2_ERRNO_H
