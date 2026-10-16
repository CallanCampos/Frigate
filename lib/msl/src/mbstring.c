// MSL C's mbstring.c: the conversions between multibyte and wide characters of <stdlib.h>. The library keeps four of
// the file's functions: the two conversions of the C locale, in which a character is one byte, wctomb, which goes
// through the current locale's conversion, and wcstombs, which uses it for a string. (The file's mbtowc and mbstowcs
// are not in the ROM.)
#include "types.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>

// Converts the first character of s: it returns how many bytes that takes, 0 for the terminator or -1 for no bytes
int __mbtowc_noconv(wchar_t *pwc, const char *s, size_t n) {
    if (s == NULL) {
        return 0;
    }
    if (n == 0) {
        return -1;
    }
    if (pwc != NULL) {
        *pwc = *s;
    }
    if (*s == '\0') {
        return 0;
    }
    return 1;
}

int __wctomb_noconv(char *s, wchar_t wc) {
    if (s == NULL) {
        return 0;
    }
    *s = wc;
    return 1;
}

int wctomb(char *s, wchar_t wc) {
    return _current_locale.ctype_cmpt_ptr->wctomb_func(s, wc);
}

// Stops at the terminator, which it stores when it has room, or before the character that would not fit in n bytes
size_t wcstombs(char *s, const wchar_t *pwcs, size_t n) {
    size_t written = 0;
    int len;
    char buf[4];

    if (s == NULL || pwcs == NULL) {
        return 0;
    }
    while (written <= n) {
        if (*pwcs == 0) {
            s[written] = '\0';
            break;
        }
        len = wctomb(buf, *pwcs++);
        if (written + len > n) {
            break;
        }
        strncpy(s + written, buf, len);
        written += len;
    }
    return written;
}
