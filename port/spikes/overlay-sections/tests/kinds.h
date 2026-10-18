/* The kinds of object with static storage that kinds.c defines and test_kinds.c checks: where each must land
 * (the overlay's data or bss section, or neither) once its file has an overlay's forced include. Part of the frozen
 * overlay-sections spike (port/spikes/README.md). */
#ifndef KINDS_H
#define KINDS_H

#include <stddef.h>

/* Where an object with static storage should land once its file has an overlay's forced include. */
enum KindsWhere {
    KINDS_DATA,    /* the overlay's data section: restored from the snapshot */
    KINDS_BSS,     /* the overlay's bss section: zeroed (the data section in the combined layout) */
    KINDS_OUTSIDE, /* neither: constants, literals, and compound literals (the pragma's one gap) */
};

typedef struct KindsEntry {
    const char *name;
    const void *address;
    size_t size;
    enum KindsWhere where;
} KindsEntry;

#define KINDS_MAX 32

/* kinds.c is compiled once per layout, as overlay 900 (split) and 901 (combined). */
int Kinds900_List(KindsEntry *out);
void Kinds900_Dirty(void);
int Kinds900_Pristine(void);
int Kinds901_List(KindsEntry *out);
void Kinds901_Dirty(void);
int Kinds901_Pristine(void);

#endif /* KINDS_H */
