#ifndef POKEBW2_LOCALE_H
#define POKEBW2_LOCALE_H

// The C library's locale.h, from CodeWarrior's MSL, which the main module links. It has only the C locale (locale.c),
// of which the library keeps three parts: how to tell the time, how to order characters and how to turn wide
// characters into bytes and back

#include <stddef.h>
#include <wchar.h>

// The strings of the time functions: AM and PM, and the formats of the date with the time, of the 12-hour time, of the
// date and of the time, in strftime's conversions; and the names of the days and months, each as short name|full name
typedef struct _Loc_time_cmpt {
    char *am_pm;
    char *date_time_format;
    char *time_12h_format;
    char *date_format;
    char *time_format;
    char *day_names;
    char *month_names;
    char *era;
} _Loc_time_cmpt;

// The collation of the characters from first_char on, by their weights
typedef struct _Loc_coll_cmpt {
    int first_char;
    int unk_4;
    int unk_8;
    const unsigned short *char_weights;
} _Loc_coll_cmpt;

// How the bytes of the locale's encoding become wide characters and back; the C locale's has none, so each byte is a
// character
typedef struct _Loc_ctype_cmpt {
    int (*mbtowc_func)(wchar_t *pwc, const char *s, size_t n);
    int (*wctomb_func)(char *s, wchar_t wc);
} _Loc_ctype_cmpt;

typedef struct _Locale {
    _Loc_time_cmpt *time_cmpt_ptr;
    _Loc_coll_cmpt *coll_cmpt_ptr;
    _Loc_ctype_cmpt *ctype_cmpt_ptr;
} _Locale;

extern _Locale _current_locale;

// The conversions of the C locale (mbstring.c), which its ctype component points to
int __mbtowc_noconv(wchar_t *pwc, const char *s, size_t n);
int __wctomb_noconv(char *s, wchar_t wc);

#endif // POKEBW2_LOCALE_H
