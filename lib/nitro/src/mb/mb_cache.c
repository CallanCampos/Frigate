// The parts of the program that the multiboot parent keeps in memory to send them to the children (libmb). The names
// are descriptive
#include "mb_private.h"

void MBi_InitCache(MBiCacheList *cache) {
    MI_CpuFill8(cache, 0, sizeof(MBiCacheList));
}

// Keeps a part of the program in the first free entry
void MBi_AttachCacheBuffer(MBiCacheList *cache, u32 offset, u32 size, void *buf, u32 state) {
    OSIntrMode irq = OS_DisableInterrupts();
    MBiCacheInfo *entry = &cache->list[0];
    MBiCacheInfo *end = &cache->list[MB_CACHE_INFO_MAX];

    while (TRUE) {
        if (entry >= end) {
            OS_Terminate();
        }
        if (entry->state == MB_CACHE_STATE_EMPTY) {
            entry->src = offset;
            entry->len = size;
            entry->ptr = buf;
            entry->state = state;
            break;
        }
        entry++;
    }
    OS_RestoreInterrupts(irq);
}

// Copies a part of the program from the cache, if it is all there
BOOL MBi_ReadFromCache(MBiCacheList *cache, u32 offset, void *dst, u32 size) {
    BOOL found = FALSE;
    OSIntrMode irq = OS_DisableInterrupts();
    MBiCacheInfo *entry;

    for (entry = &cache->list[0]; entry < &cache->list[MB_CACHE_INFO_MAX]; entry++) {
        if (entry->state >= MB_CACHE_STATE_READY) {
            int relative = offset - entry->src;

            if (relative >= 0 && relative + size <= entry->len) {
                MI_CpuCopy8((u8 *)entry->ptr + relative, dst, size);
                cache->lifetime = 0;
                found = TRUE;
                break;
            }
        }
    }
    OS_RestoreInterrupts(irq);
    return found;
}
