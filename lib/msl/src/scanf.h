#ifndef POKEBW2_MSL_SCANF_H
#define POKEBW2_MSL_SCANF_H

// The read function of the scanning routines (scanf.c): strtod, strtol and the other number scans of the library read
// their characters through a function that the caller gives, so that the same code serves a string and a stream, and
// __StringRead is the one for a string. The names MSL_GET, MSL_UNGET and MSL_TEST are this decompilation's: the
// ROM only shows the numbers 0, 1 and 2. __StringRead and __InStringStruct are MSL's own (the library's private
// header)

// What the read function is asked to do: get the next character (it returns EOF at the end), give back the one it got
// last, or say whether the end of the text was reached
#define MSL_GET 0
#define MSL_UNGET 1
#define MSL_TEST 2

// The characters that the read function __StringRead gets from a string, and whether it came to the end of it
typedef struct {
    const char *NextChar;
    int NullCharDetected;
} __InStringStruct;

// Reads a string as the ReadProc(ReadProcArg, character, action) of the scanning routines, with an __InStringStruct
// as its ReadProcArg: MSL_GET returns the next character, or EOF at the terminator, which it does not pass;
// MSL_UNGET steps back over the character that MSL_GET returned (after the terminator, forgets it was reached) and
// returns the character it was given; MSL_TEST returns whether the terminator was reached
int __StringRead(void *isc, int ch, int action);

#endif // POKEBW2_MSL_SCANF_H
