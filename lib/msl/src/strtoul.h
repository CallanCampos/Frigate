#ifndef POKEBW2_MSL_STRTOUL_H
#define POKEBW2_MSL_STRTOUL_H

// The scan of an integer (strtoul.c), which strtoul, strtol and strtoll call: __strtoul and __strtoull read the
// characters through a function that the caller gives, as __strtold does, so that the same code serves a string and
// a stream. The names are MSL's own (the library's private header)

// Reads the integer at the start of the text in the given base (0 reads the base off the prefix: 0x is 16, 0 is 8,
// else 10), after its white space and sign, taking max_width characters at most through
// ReadProc(ReadProcArg, character, action). chars_scanned gets how many characters the number took, with the white
// space and the sign (0 when there was no number), *negative whether a minus sign came first, and *overflow whether the
// number is out of the range of the return type. The character after the number is given back.
unsigned long __strtoul(int base, int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg,
                        int *chars_scanned, int *negative, int *overflow);
unsigned long long __strtoull(int base, int max_width, int (*ReadProc)(void *, int, int), void *ReadProcArg,
                              int *chars_scanned, int *negative, int *overflow);

#endif // POKEBW2_MSL_STRTOUL_H
