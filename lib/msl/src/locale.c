// MSL C's locale.c: the C locale, the only one that the library has. A data-only object, built with -ipa file, which
// gives _current_locale and the table sections of their own (lib/msl/library.toml).
#include <locale.h>

// The weights of the characters from 0x20 to 0x7F
static unsigned short char_coll_tableC[96] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 17, 18, 19, 20, 21, 22, 23, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71,
    73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 24, 25, 26, 27, 28, 0,  44, 46, 48, 50, 52, 54, 56,
    58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 29, 30, 31, 32, 0,
};

static _Loc_ctype_cmpt _loc_ctyp_C = { __mbtowc_noconv, __wctomb_noconv };

static _Loc_coll_cmpt _loc_coll_C = { 0x20, 0x6E, 0, char_coll_tableC };

static _Loc_time_cmpt _loc_tim_C = {
    "AM|PM",
    "%a %b %e %T %Y",
    "%I:%M:%S %p",
    "%m/%d/%y",
    "%T",
    "Sun|Sunday|Mon|Monday|Tue|Tuesday|Wed|Wednesday|Thu|Thursday|Fri|Friday|Sat|Saturday",
    "Jan|January|Feb|February|Mar|March|Apr|April|May|May|Jun|June|Jul|July|Aug|August|Sep|September|Oct|October|Nov|"
    "November|Dec|December",
    "",
};

_Locale _current_locale = { &_loc_tim_C, &_loc_coll_C, &_loc_ctyp_C };
