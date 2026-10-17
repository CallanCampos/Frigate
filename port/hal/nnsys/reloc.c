// The relocation registry (port/hal/nnsys/reloc.h). The trees are kept in one array sorted by key and kind: a lookup
// is a binary search, and a release, which every heap free makes, searches the keys that can lie in an overlapping
// range. A tree's key lies in its range, so a range that overlaps [begin, end) has its key less than the widest
// registered range away from it.
//
// PORT_RELOC_LOG=1 in the environment prints each tree registered and released, with its kind, key and range.
#include "hal/nnsys/reloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct RelocEntry {
    uintptr_t key;
    PortRelocKind kind;
    uintptr_t begin;
    uintptr_t end;
    void *tree;
    PortRelocDestructor destroy;
} RelocEntry;

static RelocEntry *sEntries;
static size_t sCount;
static size_t sCapacity;
// The widest range registered since the registry was last empty
static uintptr_t sMaxSpan;
// Whether PORT_RELOC_LOG asks for a line per tree: -1 until read
static int sLog = -1;

static const char *const sKindNames[PORT_RELOC_KIND_COUNT] = {
    "character", "palette", "palette-cmp", "cell-bank", "multicell-bank", "anim-bank", "cell-exattr", "font",
};

static void Log(char sign, const RelocEntry *entry) {
    if (sLog < 0) {
        const char *value = getenv("PORT_RELOC_LOG");

        sLog = value != NULL && value[0] != '\0' && value[0] != '0';
    }
    if (sLog) {
        fprintf(stderr, "port: reloc: %c %s %p [%p, %p) %zu trees\n", sign,
                entry->kind < PORT_RELOC_KIND_COUNT ? sKindNames[entry->kind] : "?", (void *)entry->key,
                (void *)entry->begin, (void *)entry->end, sCount);
    }
}

static int Compare(uintptr_t key, PortRelocKind kind, const RelocEntry *entry) {
    if (key != entry->key) {
        return key < entry->key ? -1 : 1;
    }
    if (kind != entry->kind) {
        return kind < entry->kind ? -1 : 1;
    }
    return 0;
}

// The index of the first entry not before key and kind
static size_t LowerBound(uintptr_t key, PortRelocKind kind) {
    size_t low = 0;
    size_t high = sCount;

    while (low < high) {
        size_t middle = low + (high - low) / 2;

        if (Compare(key, kind, &sEntries[middle]) > 0) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

static RelocEntry *Find(const void *key, PortRelocKind kind) {
    size_t index = LowerBound((uintptr_t)key, kind);

    if (index < sCount && Compare((uintptr_t)key, kind, &sEntries[index]) == 0) {
        return &sEntries[index];
    }
    return NULL;
}

static void Remove(size_t index) {
    memmove(&sEntries[index], &sEntries[index + 1], (sCount - index - 1) * sizeof(RelocEntry));
    sCount--;
    if (sCount == 0) {
        sMaxSpan = 0;
    }
}

static void SetRange(RelocEntry *entry, const void *begin, size_t size) {
    entry->begin = (uintptr_t)begin;
    entry->end = (uintptr_t)begin + size;
    // The key always lies in the range, which the release's search relies on
    if (entry->key < entry->begin) {
        entry->begin = entry->key;
    }
    if (entry->key >= entry->end) {
        entry->end = entry->key + 1;
    }
    if (entry->end - entry->begin > sMaxSpan) {
        sMaxSpan = entry->end - entry->begin;
    }
}

void port_reloc_register(const void *key, PortRelocKind kind, const void *begin, size_t size, void *tree,
                         PortRelocDestructor destroy) {
    RelocEntry *entry = Find(key, kind);
    size_t index;

    if (entry != NULL) {
        if (entry->tree != tree) {
            void *old = entry->tree;
            PortRelocDestructor oldDestroy = entry->destroy;

            entry->tree = tree;
            entry->destroy = destroy;
            if (oldDestroy != NULL) {
                oldDestroy(old);
            }
        }
        SetRange(entry, begin, size);
        Log('=', entry);
        return;
    }

    if (sCount == sCapacity) {
        size_t capacity = sCapacity != 0 ? sCapacity * 2 : 64;
        RelocEntry *entries = realloc(sEntries, capacity * sizeof(RelocEntry));

        if (entries == NULL) {
            fprintf(stderr, "port: reloc: out of host memory for %zu trees\n", capacity);
            abort();
        }
        sEntries = entries;
        sCapacity = capacity;
    }
    index = LowerBound((uintptr_t)key, kind);
    memmove(&sEntries[index + 1], &sEntries[index], (sCount - index) * sizeof(RelocEntry));
    sCount++;
    entry = &sEntries[index];
    entry->key = (uintptr_t)key;
    entry->kind = kind;
    entry->tree = tree;
    entry->destroy = destroy;
    SetRange(entry, begin, size);
    Log('+', entry);
}

void *port_reloc_find(const void *key, PortRelocKind kind) {
    RelocEntry *entry = Find(key, kind);

    return entry != NULL ? entry->tree : NULL;
}

int port_reloc_range(const void *key, PortRelocKind kind, const void **begin, size_t *size) {
    RelocEntry *entry = Find(key, kind);

    if (entry == NULL) {
        return 0;
    }
    *begin = (const void *)entry->begin;
    *size = entry->end - entry->begin;
    return 1;
}

size_t port_reloc_release_range(const void *begin, size_t size) {
    uintptr_t low = (uintptr_t)begin;
    uintptr_t high = low + size;
    uintptr_t first;
    uintptr_t last;
    size_t index;
    size_t released = 0;

    if (sCount == 0 || size == 0) {
        return 0;
    }
    if (high < low) {
        high = UINTPTR_MAX;
    }
    // The keys of the ranges that can overlap: within the widest range of either end
    first = low > sMaxSpan ? low - sMaxSpan : 0;
    last = high < UINTPTR_MAX - sMaxSpan ? high + sMaxSpan : UINTPTR_MAX;
    index = LowerBound(first, 0);
    while (index < sCount && sEntries[index].key < last) {
        RelocEntry entry = sEntries[index];

        if (entry.begin < high && entry.end > low) {
            // Out of the array before the destructor runs, so that the registry is whole whatever it does
            Remove(index);
            Log('-', &entry);
            if (entry.destroy != NULL) {
                entry.destroy(entry.tree);
            }
            released++;
        } else {
            index++;
        }
    }
    return released;
}

void port_reloc_release(const void *key, PortRelocKind kind) {
    RelocEntry *found = Find(key, kind);
    RelocEntry entry;

    if (found == NULL) {
        return;
    }
    entry = *found;
    Remove((size_t)(found - sEntries));
    Log('-', &entry);
    if (entry.destroy != NULL) {
        entry.destroy(entry.tree);
    }
}

size_t port_reloc_count(void) {
    return sCount;
}

size_t port_reloc_count_kind(PortRelocKind kind) {
    size_t count = 0;

    for (size_t i = 0; i < sCount; i++) {
        if (sEntries[i].kind == kind) {
            count++;
        }
    }
    return count;
}

void port_reloc_release_all(void) {
    while (sCount != 0) {
        RelocEntry entry = sEntries[sCount - 1];

        Remove(sCount - 1);
        if (entry.destroy != NULL) {
            entry.destroy(entry.tree);
        }
    }
    free(sEntries);
    sEntries = NULL;
    sCapacity = 0;
}
