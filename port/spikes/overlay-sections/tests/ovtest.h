/* The interface of the synthetic overlays that gen_overlays.py generates for the overlay-sections spike's tests and
 * benchmarks (port/spikes/README.md, frozen). */
#ifndef OVTEST_H
#define OVTEST_H

/* The generated overlays' test interface (gen_overlays.py writes ov_table.c). */

typedef struct OvTestEntry {
    unsigned id;
    unsigned dsData; /* the overlay's .data and .bss sizes on the DS */
    unsigned dsBss;
    void (*dirty)(void);   /* writes every writable static of the overlay */
    int (*pristine)(void); /* whether every one holds its initial value */
    /* ld64's section$start and section$end bounds of the data and bss sections, NULL elsewhere. */
    unsigned char *dataStart;
    unsigned char *dataEnd;
    unsigned char *bssStart;
    unsigned char *bssEnd;
} OvTestEntry;

#if defined(__APPLE__)
#define OVTEST_BOUNDS(start, end) start, end
#else
#define OVTEST_BOUNDS(start, end) 0, 0
#endif

/* Always 0; indexing with it keeps the optimizer from splitting the overlays' arrays. */
extern volatile int gOvTestZero;

extern const OvTestEntry gOvTests[];
extern const int gOvTestCount;
extern const int gOvTestSections;

#endif /* OVTEST_H */
