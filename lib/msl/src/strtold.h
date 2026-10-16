#ifndef POKEBW2_MSL_STRTOLD_H
#define POKEBW2_MSL_STRTOLD_H

#include "scanf.h"

// The scan of a floating-point number (strtold.c), which strtod calls: __strtold reads the characters through a
// function that the caller gives (scanf.h), so that the same code serves a string and a stream. The name __strtold is
// MSL's own (the library's private header)

// Reads max_width characters at most, through ReadProc(ReadProcArg, character, action). chars_scanned gets how many it
// took for the number; overflow, whether the number is out of the range of a double
double __strtold(int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg, int *chars_scanned,
                 int *overflow);

#endif // POKEBW2_MSL_STRTOLD_H
