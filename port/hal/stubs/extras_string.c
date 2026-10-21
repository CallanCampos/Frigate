// MSL's C-locale case-blind string extensions, used by GameSpy's server browser (lib/dwc/src/serverbrowsing).
// The semantics are lib/msl/src/extras_string.c and file_io.c: fold ASCII only, leave high bytes unchanged, return
// exactly -1/0/1, and for strnicmp compare nothing when its signed bound is nonpositive. Host locales do not apply.
#include <stddef.h>

static unsigned char Lower(unsigned char c) {
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

static int Compare(const char *a, const char *b, size_t count) {
    for (size_t i = 0; i < count; i++) {
        unsigned char left = Lower((unsigned char)a[i]);
        unsigned char right = Lower((unsigned char)b[i]);
        if (left != right) {
            return left < right ? -1 : 1;
        }
        if (left == 0) {
            return 0;
        }
    }
    return 0;
}

int stricmp(const char *a, const char *b) {
    return Compare(a, b, (size_t)-1);
}

int strcmpi(const char *a, const char *b) {
    return stricmp(a, b);
}

int strnicmp(const char *a, const char *b, int n) {
    return Compare(a, b, n > 0 ? (size_t)n : 0);
}
