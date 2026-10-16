// The parts of the program that the multiboot parent keeps in memory to send them to the children (libmb). The names
// are descriptive
#include "mb_private.h"

void MBCache_Clear(MBCache *cache) {
    MI_CpuFill8(cache, 0, sizeof(MBCache));
}

// Keeps a part of the program in the first free entry
void MBCache_Add(MBCache *cache, u32 offset, u32 size, void *buf, u32 state) {
    u32 irq = OS_DisableInterrupts();
    MBCacheEntry *entry = &cache->entries[0];
    MBCacheEntry *end = &cache->entries[MB_CACHE_ENTRY_COUNT];

    while (TRUE) {
        if (entry >= end) {
            OS_Terminate();
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
    OS_RestoreInterrupts(irq);
}

// Copies a part of the program from the cache, if it is all there
BOOL MBCache_Read(MBCache *cache, u32 offset, void *dst, u32 size) {
    BOOL found = FALSE;
    u32 irq = OS_DisableInterrupts();
    MBCacheEntry *entry;

    for (entry = &cache->entries[0]; entry < &cache->entries[MB_CACHE_ENTRY_COUNT]; entry++) {
        if (entry->state >= MB_CACHE_ENTRY_READY) {
            int relative = offset - entry->offset;

            if (relative >= 0 && relative + size <= entry->size) {
                MI_CpuCopy8((u8 *)entry->buf + relative, dst, size);
                cache->missCounter = 0;
                found = TRUE;
                break;
            }
        }
    }
    OS_RestoreInterrupts(irq);
    return found;
}
