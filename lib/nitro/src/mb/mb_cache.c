// The parts of the program that the multiboot parent keeps in memory to send them to the children (libmb). The names
// are descriptive
#include "mb_private.h"

void MBCache_Clear(MBCache *cache) {
    sys_memset(cache, 0, sizeof(MBCache));
}

// Keeps a part of the program in the first free entry
void MBCache_Add(MBCache *cache, u32 offset, u32 size, void *buf, u32 state) {
    u32 irq = CPU_IRQDisable();
    MBCacheEntry *entry = &cache->entries[0];
    MBCacheEntry *end = &cache->entries[MB_CACHE_ENTRY_COUNT];

    while (TRUE) {
        if (entry >= end) {
            sys_exit();
        }
        if (entry->state == MB_CACHE_ENTRY_EMPTY) {
            entry->offset = offset;
            entry->size = size;
            entry->buf = buf;
            entry->state = state;
            break;
        }
        entry++;
    }
    CPU_SetIRQMask(irq);
}

// Copies a part of the program from the cache, if it is all there
BOOL MBCache_Read(MBCache *cache, u32 offset, void *dst, u32 size) {
    BOOL found = FALSE;
    u32 irq = CPU_IRQDisable();
    MBCacheEntry *entry;

    for (entry = &cache->entries[0]; entry < &cache->entries[MB_CACHE_ENTRY_COUNT]; entry++) {
        if (entry->state >= MB_CACHE_ENTRY_READY) {
            int relative = offset - entry->offset;

            if (relative >= 0 && relative + size <= entry->size) {
                sys_memcpy((u8 *)entry->buf + relative, dst, size);
                cache->missCounter = 0;
                found = TRUE;
                break;
            }
        }
    }
    CPU_SetIRQMask(irq);
    return found;
}
