#include "nitro/os.h"
#include "nnsys/fnd.h"

// NitroSystem's allocators (NNS_Fnd): a heap with its parameters behind a pair of alloc and free functions. swan's
// names: NNS_FndAllocFromAllocator AllocatorBase_Malloc, NNS_FndInitAllocatorForExpHeap CreateExpHeapAllocator,
// AllocatorAllocForExpHeap ExpHeapAllocator_Malloc, AllocatorAllocForSDKHeap SystemHeapAllocator_Malloc, and the
// function table sExpHeapAllocatorFuncs v_ExpHeapAllocator. The game only sets allocators up over expanded heaps; the
// frame, unit and NitroSDK heap versions lost their setup functions to dead-stripping, but their function tables, at
// the end of this file, kept their functions

static void *AllocatorAllocForExpHeap(NNSFndAllocator *allocator, u32 size);
static void AllocatorFreeForExpHeap(NNSFndAllocator *allocator, void *memBlock);
static void *AllocatorAllocForFrmHeap(NNSFndAllocator *allocator, u32 size);
static void AllocatorFreeForFrmHeap(NNSFndAllocator *allocator, void *memBlock);
static void *AllocatorAllocForUnitHeap(NNSFndAllocator *allocator, u32 size);
static void AllocatorFreeForUnitHeap(NNSFndAllocator *allocator, void *memBlock);
static void *AllocatorAllocForSDKHeap(NNSFndAllocator *allocator, u32 size);
static void AllocatorFreeForSDKHeap(NNSFndAllocator *allocator, void *memBlock);

// heapParam1 is the alignment
static void *AllocatorAllocForExpHeap(NNSFndAllocator *allocator, u32 size) {
    return NNS_FndAllocFromExpHeapEx(allocator->pHeap, size, (int)allocator->heapParam1);
}

static void AllocatorFreeForExpHeap(NNSFndAllocator *allocator, void *memBlock) {
    NNS_FndFreeToExpHeap(allocator->pHeap, memBlock);
}

// heapParam1 is the alignment
static void *AllocatorAllocForFrmHeap(NNSFndAllocator *allocator, u32 size) {
    return NNS_FndAllocFromFrmHeapEx(allocator->pHeap, size, (int)allocator->heapParam1);
}

// Blocks of a frame heap can't be freed one by one
static void AllocatorFreeForFrmHeap(NNSFndAllocator *allocator, void *memBlock) {
}

// Fails for anything bigger than the heap's block size
static void *AllocatorAllocForUnitHeap(NNSFndAllocator *allocator, u32 size) {
    if (size > NNS_FndGetMemBlockSizeForUnitHeap(allocator->pHeap)) {
        return NULL;
    }
    return NNS_FndAllocFromUnitHeap(allocator->pHeap);
}

static void AllocatorFreeForUnitHeap(NNSFndAllocator *allocator, void *memBlock) {
    NNS_FndFreeToUnitHeap(allocator->pHeap, memBlock);
}

// For a NitroSDK heap, pHeap holds the heap's handle and heapParam1 its arena
static void *AllocatorAllocForSDKHeap(NNSFndAllocator *allocator, u32 size) {
    return OS_AllocFromHeap((int)allocator->heapParam1, (int)allocator->pHeap, size);
}

static void AllocatorFreeForSDKHeap(NNSFndAllocator *allocator, void *memBlock) {
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
// expanded, NitroSDK. Their names are ours: NitroSystem's are each a static sAllocatorFunc in its setup function
const NNSFndAllocatorFunc NNSi_FndFrameHeapAllocatorFuncs = { AllocatorAllocForFrmHeap, AllocatorFreeForFrmHeap };
const NNSFndAllocatorFunc NNSi_FndUnitHeapAllocatorFuncs = { AllocatorAllocForUnitHeap, AllocatorFreeForUnitHeap };
const NNSFndAllocatorFunc NNSi_FndSystemHeapAllocatorFuncs = { AllocatorAllocForSDKHeap, AllocatorFreeForSDKHeap };
