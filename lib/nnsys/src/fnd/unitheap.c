#include "heapcommoni.h"
#include "nnsys/fnd.h"

// NitroSystem's unit heaps (NNS_Fnd): blocks of one size, the free ones chained through their first word. swan calls
// none of them by name. The game never makes one, so only what the unit heap allocator in allocator.c reaches is
// left; creating a unit heap was dead-stripped

static NNSiFndUntHeapMBlockHead *PopMBlock(NNSiFndUntMBlockList *list);

// The unit heap's own head, just after the common one
static inline NNSiFndUntHeapHead *GetUnitHeapHeadPtrFromHeapHead(NNSiFndHeapHead *heap) {
    return (NNSiFndUntHeapHead *)(NNSiGetUIntPtr(heap) + sizeof(NNSiFndHeapHead));
}

// Unchains the first free block, or returns NULL when there is none
static NNSiFndUntHeapMBlockHead *PopMBlock(NNSiFndUntMBlockList *list) {
    NNSiFndUntHeapMBlockHead *first = list->head;

    if (first != NULL) {
        list->head = first->pMBlkHdNext;
    }
    return first;
}

void *NNS_FndAllocFromUnitHeap(NNSFndHeapHandle heap) {
    NNSiFndUntHeapHead *unitHeap = GetUnitHeapHeadPtrFromHeapHead(heap);
    NNSiFndUntHeapMBlockHead *block = PopMBlock(&unitHeap->mbFreeList);

    if (block != NULL) {
        FillAllocMemory(heap, block, unitHeap->mBlkSize);
    }
    return block;
}

void NNS_FndFreeToUnitHeap(NNSFndHeapHandle heap, void *memBlock) {
    NNSiFndUntHeapHead *unitHeap = GetUnitHeapHeadPtrFromHeapHead(heap);
    NNSiFndUntHeapMBlockHead *block = memBlock;

    block->pMBlkHdNext = unitHeap->mbFreeList.head;
    unitHeap->mbFreeList.head = block;
}
