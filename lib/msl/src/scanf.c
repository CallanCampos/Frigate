// MSL's scanf.c: the read function that its scanning routines (strtod, strtol and the others) call to get characters
// out of a string. The scanf entry points themselves are not in the ROM, as nothing in the game calls them.
#include "scanf.h"
#include <stdio.h>

int __StringRead(void *isc, int ch, int action) {
    __InStringStruct *in = isc;

    switch (action) {
    case MSL_GET: {
        // The terminator is not consumed, so that reading again says the end again
        char c = *in->NextChar;
        if (c == '\0') {
            in->NullCharDetected = 1;
            return EOF;
        }
        in->NextChar++;
        return c;
    }
    case MSL_UNGET:
        if (!in->NullCharDetected) {
            in->NextChar--;
        } else {
            in->NullCharDetected = 0;
        }
        return ch;
    case MSL_TEST:
        return in->NullCharDetected;
    }
    return 0;
}
