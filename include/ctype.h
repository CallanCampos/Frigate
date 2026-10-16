#ifndef POKEBW2_CTYPE_H
#define POKEBW2_CTYPE_H

// The C library's ctype.h, from CodeWarrior's MSL, which the main module links. It has only the C locale: three tables
// for the characters below 0x80 (ctype.c), and the functions that read them

// The bits of __ctype_map, one for each class that the functions below test (the names are MSL's)
#define __ctype_alpha 0x0001
#define __ctype_blank 0x0002
#define __ctype_cntrl 0x0004
#define __ctype_digit 0x0008
#define __ctype_graph 0x0010
#define __ctype_lower 0x0020
#define __ctype_print 0x0040
#define __ctype_punct 0x0080
#define __ctype_space 0x0100
#define __ctype_upper 0x0200
#define __ctype_xdigit 0x0400

// The lower and the upper case of each character, and its classes
extern const unsigned char __lower_map[0x80];
extern const unsigned char __upper_map[0x80];
extern const unsigned short __ctype_map[0x80];

// The class functions test the table for the characters below 0x80 and give 0 for the others. They are written as
// MSL has them (a range test with `c < 0 ||`), which orders the compiler's predicated instructions as the ROM has them
static inline int isalpha(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_alpha;
}

static inline int isblank(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_blank;
}

static inline int iscntrl(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_cntrl;
}

static inline int isdigit(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_digit;
}

static inline int isgraph(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_graph;
}

static inline int islower(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_lower;
}

static inline int isprint(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_print;
}

static inline int ispunct(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_punct;
}

static inline int isspace(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_space;
}

static inline int isupper(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_upper;
}

static inline int isxdigit(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & __ctype_xdigit;
}

static inline int isalnum(int c) {
    return (c < 0 || c >= 0x80) ? 0 : __ctype_map[c] & (__ctype_alpha | __ctype_digit);
}

static inline int tolower(int c) {
    return (c < 0 || c >= 0x80) ? c : __lower_map[c];
}

static inline int toupper(int c) {
    return (c < 0 || c >= 0x80) ? c : __upper_map[c];
}

#endif // POKEBW2_CTYPE_H
