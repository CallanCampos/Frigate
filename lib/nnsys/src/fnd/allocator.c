#include "nitro/os.h"
#include "nnsys/fnd.h"

// NitroSystem's allocators (NNS_Fnd): a heap with its parameters behind a pair of alloc and free functions. swan's
// names: NNS_FndAllocFromAllocator AllocatorBase_Malloc, NNS_FndInitAllocatorForExpHeap CreateExpHeapAllocator,
// AllocatorAllocForExpHeap ExpHeapAllocator_Malloc. The game
// only sets allocators up over expanded heaps; the frame, unit and NitroSDK heap versions lost their setup functions
// to dead-stripping, but their function tables, at the end of this file, kept their functions

static void *AllocatorAllocForExpHeap(NNSFndAllocator *allocator, u32 size);
static void AllocatorFreeForExpHeap(NNSFndAllocator *allocator, void *memBlock);
static void *FrameHeapAllocatorAlloc(NNSFndAllocator *allocator, u32 size);
static void FrameHeapAllocatorFree(NNSFndAllocator *allocator, void *memBlock);
static void *UnitHeapAllocatorAlloc(NNSFndAllocator *allocator, u32 size);
static void UnitHeapAllocatorFree(NNSFndAllocator *allocator, void *memBlock);
static void *SystemHeapAllocator_Malloc(NNSFndAllocator *allocator, u32 size);
static void SystemHeapAllocator_Free(NNSFndAllocator *allocator, void *memBlock);

// heapParam1 is the alignment
static void *AllocatorAllocForExpHeap(NNSFndAllocator *allocator, u32 size) {
    return NNS_FndAllocFromExpHeapEx(allocator->pHeap, size, (int)allocator->heapParam1);
}

static void AllocatorFreeForExpHeap(NNSFndAllocator *allocator, void *memBlock) {
    NNS_FndFreeToExpHeap(allocator->pHeap, memBlock);
}

// heapParam1 is the alignment
static void *FrameHeapAllocatorAlloc(NNSFndAllocator *allocator, u32 size) {
    return NNS_FndAllocFromFrmHeapEx(allocator->pHeap, size, (int)allocator->heapParam1);
}

// Blocks of a frame heap can't be freed one by one
static void FrameHeapAllocatorFree(NNSFndAllocator *allocator, void *memBlock) {
}

// Fails for anything bigger than the heap's block size
static void *UnitHeapAllocatorAlloc(NNSFndAllocator *allocator, u32 size) {
    if (size > NNS_FndGetUnitHeapUnitSize(allocator->pHeap)) {
        return NULL;
    }
    return NNS_FndUnitHeapAlloc(allocator->pHeap);
}

static void UnitHeapAllocatorFree(NNSFndAllocator *allocator, void *memBlock) {
    NNS_FndUnitHeapFree(allocator->pHeap, memBlock);
}

// For a NitroSDK heap, pHeap holds the heap's handle and heapParam1 its arena
static void *SystemHeapAllocator_Malloc(NNSFndAllocator *allocator, u32 size) {
    return OS_AllocFromHeap((int)allocator->heapParam1, (int)allocator->pHeap, size);
}

static void SystemHeapAllocator_Free(NNSFndAllocator *allocator, void *memBlock) {
    OS_FreeToHeap((int)allocator->heapParam1, (int)allocator->pHeap, memBlock);
}

void *NNS_FndAllocFromAllocator(NNSFndAllocator *allocator, u32 size) {
    return allocator->pFunc->pfAlloc(allocator, size);
}

void NNS_FndFreeToAllocator(NNSFndAllocator *allocator, void *memBlock) {
    allocator->pFunc->pfFree(allocator, memBlock);
}

void NNS_FndInitAllocatorForExpHeap(NNSFndAllocator *allocator, NNSFndHeapHandle heap, int alignment) {
    static const NNSFndAllocatorFunc sExpHeapAllocatorFuncs = { AllocatorAllocForExpHeap, AllocatorFreeForExpHeap };

    allocator->pFunc = &sExpHeapAllocatorFuncs;
    allocator->pHeap = heap;
    allocator->heapParam1 = alignment;
    allocator->heapParam2 = 0;
}

// The tables of the dead-stripped setup functions for frame, unit and NitroSDK heaps. MWCC drops a static nothing
// reads, so they are globals, and they come in this order so that .rodata holds them as the ROM does: unit, frame,
// expanded, NitroSDK
const NNSFndAllocatorFunc NNSi_FndFrameHeapAllocatorFuncs = { FrameHeapAllocatorAlloc, FrameHeapAllocatorFree };
const NNSFndAllocatorFunc NNSi_FndUnitHeapAllocatorFuncs = { UnitHeapAllocatorAlloc, UnitHeapAllocatorFree };
const NNSFndAllocatorFunc NNSi_FndSystemHeapAllocatorFuncs = { SystemHeapAllocator_Malloc, SystemHeapAllocator_Free };
