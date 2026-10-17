/*
 * What goes where: each kind of object in kinds.c, compiled as overlay 900 (split layout) and 901 (combined
 * layout), must land in the overlay's data section, its bss section, or outside both, as kinds.c says; and a
 * reset must bring back every value, including those of objects the optimizer shrank or split.
 */
#include <stdio.h>
#include <stdlib.h>

#include "kinds.h"
#include "ovsec.h"

static int sFailures;

static const char *const sWhereNames[] = { "data", "bss", "outside" };

static int Inside(const OvSecRange *range, const KindsEntry *e)
{
    const unsigned char *p = e->address;
    return range->start != NULL && p >= range->start && p + e->size <= range->start + range->size;
}

static void CheckOverlay(unsigned id, const char *layout, int (*list)(KindsEntry *), void (*dirty)(void),
                         int (*pristine)(void))
{
    const OvSecOverlay *ov = OvSec_Get(id);
    KindsEntry entries[KINDS_MAX];
    int count, i;

    printf("overlay %u, %s layout: data %zu bytes, bss %zu bytes\n", id, layout, ov ? ov->data.size : 0,
           ov ? ov->bss.size : 0);
    if (ov == NULL) {
        printf("FAIL: overlay %u has no sections\n", id);
        sFailures++;
        return;
    }
    count = list(entries);
    for (i = 0; i < count; i++) {
        enum KindsWhere expected = entries[i].where;
        enum KindsWhere actual = Inside(&ov->data, &entries[i]) ? KINDS_DATA
            : Inside(&ov->bss, &entries[i])                     ? KINDS_BSS
                                                                : KINDS_OUTSIDE;
        /* The combined layout folds zero-initialized objects into the data section. */
        if (ov->bss.start == NULL && expected == KINDS_BSS) {
            expected = KINDS_DATA;
        }
        printf("  %-42s %-8s %s\n", entries[i].name, sWhereNames[actual], actual == expected ? "" : "FAIL");
        if (actual != expected) {
            printf("FAIL: %s should be %s\n", entries[i].name, sWhereNames[expected]);
            sFailures++;
        }
    }
    if (!pristine()) {
        printf("FAIL: overlay %u is not pristine at startup\n", id);
        sFailures++;
    }
    dirty();
    if (pristine()) {
        printf("FAIL: overlay %u is pristine after Dirty\n", id);
        sFailures++;
    }
    OvSec_Reset(id);
    if (!pristine()) {
        printf("FAIL: overlay %u is not pristine after OvSec_Reset\n", id);
        sFailures++;
    }
}

int main(void)
{
    unsigned char *snapshot = malloc(OvSec_Scan() + 1);

    OvSec_Snapshot(snapshot);
    CheckOverlay(900, "split", Kinds900_List, Kinds900_Dirty, Kinds900_Pristine);
    CheckOverlay(901, "combined", Kinds901_List, Kinds901_Dirty, Kinds901_Pristine);
    printf("%s\n", sFailures != 0 ? "FAILED" : "ok");
    free(snapshot);
    return sFailures != 0;
}
