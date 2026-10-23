// NitroSDK's heap allocator on a host: the decomp's os_alloc.c, compiled as it is from its own path, so that the cells,
// the lists and the allocation and freeing stay the decomp's own C (port/hal/os/replaced.txt keeps os_alloc.c out of
// the game's own sources). Three of its functions can't run on a 64-bit host as written, so they are renamed out of the
// way before the include and written again below, where they reach os_alloc.c's statics:
//   - OS_InitAlloc and OS_CreateHeap round addresses to 0x20 through u32 (OS_ROUND_UP_32, OS_ROUND_DOWN_32), which
//     truncates 64-bit pointers;
//   - OS_CreateExtraHeap puts its heap at the raw DS address 0x02000000, the 16 KiB of main memory before the program.
//     The host's is at that address's place in the main memory window (port/hal/hw), which nothing else uses.
// The rounding of sizes (OS_AllocFromHeap) stays the decomp's: a size fits in 32 bits.
#include "hal/hw/hw.h"
#include "hal/os/os_internal.h"
#include "nitro/os.h"
#include "twl/os.h"
#include <stdint.h>

#define OS_InitAlloc OSi_DsInitAlloc
#define OS_CreateHeap OSi_DsCreateHeap
#define OS_CreateExtraHeap OSi_DsCreateExtraHeap
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-to-int-cast"
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#include "lib/nitro/src/os/os_alloc.c"
#pragma clang diagnostic pop
#undef OS_InitAlloc
#undef OS_CreateHeap
#undef OS_CreateExtraHeap

// The DS's start of main memory, where OS_CreateExtraHeap's 16 KiB are, and their size
#define EXTRA_HEAP_ADDRESS 0x02000000
#define EXTRA_HEAP_SIZE 0x4000

// os_alloc.c's rounding, on the whole address
#define HOST_ROUND_UP_32(x) ((void *)(((uintptr_t)(x) + ALIGNMENT - 1) & ~(uintptr_t)(ALIGNMENT - 1)))
#define HOST_ROUND_DOWN_32(x) ((void *)((uintptr_t)(x) & ~(uintptr_t)(ALIGNMENT - 1)))

// NitroSDK's OS_InitAlloc: makes the arena's table of heaps, with room for the number of them, at the start of the
// memory that it is given, and returns where the memory that is left starts
void *OS_InitAlloc(OSArenaId arena, void *start, void *end, int maxHeaps) {
    OSHeapInfo *info = start;
    u32 heapsSize;
    u32 irq = OS_DisableInterrupts();
    int i;

    OSiHeapInfo[arena] = info;
    heapsSize = maxHeaps * sizeof(HeapDesc);
    info->heapArray = (HeapDesc *)((u8 *)info + sizeof(OSHeapInfo));
    info->numHeaps = maxHeaps;
    for (i = 0; i < info->numHeaps; i++) {
        HeapDesc *hp = &info->heapArray[i];

        hp->size = -1;
        hp->free = hp->allocated = NULL;
    }
    info->currentHeap = -1;
    info->arenaStart = HOST_ROUND_UP_32((u8 *)info->heapArray + heapsSize);
    end = HOST_ROUND_DOWN_32(end);
    info->arenaEnd = end;
    OS_RestoreInterrupts(irq);
    return info->arenaStart;
}

// NitroSDK's OS_CreateHeap: makes a heap of the memory from start to end (rounded in to 0x20), and returns its number,
// or -1 when the arena has no room for another heap
int OS_CreateHeap(OSArenaId arena, void *start, void *end) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    int heap;
    u32 irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    start = HOST_ROUND_UP_32(start);
    end = HOST_ROUND_DOWN_32(end);
    for (heap = 0; heap < info->numHeaps; heap++) {
        hp = &info->heapArray[heap];
        if (hp->size < 0) {
            hp->size = (s32)((u8 *)end - (u8 *)start);
            cell = start;
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = hp->size;
            hp->free = cell;
            hp->allocated = NULL;
            OS_RestoreInterrupts(irq);
            return heap;
        }
    }
    OS_RestoreInterrupts(irq);
    return -1;
}

// NitroSDK's OS_CreateExtraHeap: on a DS, makes a heap of the 16 KiB at the start of the main memory once, in the first
// heap of the arena that is free. Gives the heap's number, or -1
int OS_CreateExtraHeap(OSArenaId arena) {
    OSHeapInfo *info;
    HeapDesc *hp;
    Cell *cell;
    int heap;
    u32 irq = OS_DisableInterrupts();

    info = OSiHeapInfo[arena];
    if (!OS_IsRunOnTwl() && sExtraHeap.heap < 0) {
        for (heap = 0; heap < info->numHeaps; heap++) {
            hp = &info->heapArray[heap];
            if (hp->size < 0) {
                hp->size = EXTRA_HEAP_SIZE;
                cell = port_hw_pointer(EXTRA_HEAP_ADDRESS);
                cell->prev = NULL;
                cell->next = NULL;
                cell->size = EXTRA_HEAP_SIZE;
                hp->free = cell;
                hp->allocated = NULL;
                sExtraHeap.arena = arena;
                sExtraHeap.heap = heap;
                OS_RestoreInterrupts(irq);
                return heap;
            }
        }
    }
    OS_RestoreInterrupts(irq);
    return -1;
}

// Sets the allocator's statics back for a new run of port_os_run: no arena has heaps, and the extra heap is unmade
void port_osi_reset_alloc(void) {
    int i;

    for (i = 0; i < OS_ARENA_MAX; i++) {
        OSiHeapInfo[i] = NULL;
    }
    sExtraHeap.heap = -1;
    sExtraHeap.arena = -1;
}
