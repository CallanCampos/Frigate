// MSL C's extras: the case-blind string comparisons that CodeWarrior's libraries add to <string.h>. stricmp compares
// through tolower; strnicmp is a call of file_io.c's __msl_strnicmp, which compares up to n characters the same way,
// and strcmpi a call of stricmp. The file's name is a guess
#include "file_io.h"
#include <ctype.h>
#include <string.h>

// Compares the strings up to the terminator without regard to case. Returns -1, 0 or 1
int stricmp(const char *a, const char *b) {
    unsigned char c1, c2;

    for (;;) {
        c1 = tolower(*a++);
        c2 = tolower(*b++);
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
        if (c1 == 0) {
            return 0;
        }
    }
}

int strnicmp(const char *a, const char *b, int n) {
    return __msl_strnicmp(a, b, n);
}

int strcmpi(const char *a, const char *b) {
    return stricmp(a, b);
}
