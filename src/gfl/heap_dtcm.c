#include "types.h"
#include "gfl/heapsys.h"
#include "nitro/os.h"

// A heap of DTCM, an OS heap in the DTCM arena. The name is a guess, from GFL_HeapDTCMInit

#define DTCM_HEAP_MAX_SIZE 0x2f80

typedef struct {
    u32 unk0;
    int handle;
    u32 unk8;
} DTCMHeap;

static DTCMHeap sDTCMHeap;

BOOL GFL_HeapDTCMInitCore(u32 size) {
    void *start;
    void *lo;
    BOOL result;

    if (size > DTCM_HEAP_MAX_SIZE) {
        return FALSE;
    }
    start = OS_GetArenaLo(OS_ARENA_DTCM);
    OS_SetArenaHi(OS_ARENA_DTCM, (void *)(HW_DTCM + 0x3000));
    result = TRUE;
    lo = OS_InitAlloc(OS_ARENA_DTCM, start, (u8 *)start + size, 1);
    OS_SetArenaLo(OS_ARENA_DTCM, lo);
    sDTCMHeap.handle = OS_CreateHeap(OS_ARENA_DTCM, lo, (u8 *)start + size);
    if (sDTCMHeap.handle == OS_HEAP_INVALID) {
        result = FALSE;
    }
    return result;
}

void *GFL_HeapDTCMAllocateCore(u32 size) {
    return OS_AllocFromHeap(OS_ARENA_DTCM, sDTCMHeap.handle, size);
}

BOOL freeBlkFromDTCM(void *ptr) {
    OS_FreeToHeap(OS_ARENA_DTCM, sDTCMHeap.handle, ptr);
    return TRUE;
}
