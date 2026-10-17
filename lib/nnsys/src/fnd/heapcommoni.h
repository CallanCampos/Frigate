#ifndef POKEBW2_NNSYS_FND_HEAPCOMMONI_H
#define POKEBW2_NNSYS_FND_HEAPCOMMONI_H

#include "nitro/mi.h"
#include "nnsys/fnd.h"
#include <stddef.h>

// What FND's heaps share among themselves: address arithmetic and the heap options

// A stretch of memory, from start up to end
typedef struct {
    void *start;
    void *end;
} NNSiMemRegion;

// The address of p, for arithmetic
static inline uptr AddrOf(const void *p) {
    return (uptr)p;
}

// How many bytes lie from from up to to
static inline u32 BytesBetween(const void *from, const void *to) {
    return AddrOf(to) - AddrOf(from);
}

// The heap's options (NNS_FND_HEAP_CLEAR_ON_ALLOC) sit in the low byte of its attribute
#define HEAP_OPT_MASK 0xFF

static inline u16 GetOptForHeap(const NNSiFndHeapHead *heap) {
    return (u16)(heap->attribute & HEAP_OPT_MASK);
}

// Zeroes memory just handed out, when the heap asked for it
static inline void FillAllocMemory(NNSiFndHeapHead *heap, void *address, u32 size) {
    if (GetOptForHeap(heap) & NNS_FND_HEAP_CLEAR_ON_ALLOC) {
        MI_CpuClear32(address, size);
    }
}

void NNSi_FndInitHeapHead(NNSiFndHeapHead *heap, u32 signature, void *heapStart, void *heapEnd, u16 optFlag);
void NNSi_FndFinalizeHeap(NNSiFndHeapHead *heap);

#endif // POKEBW2_NNSYS_FND_HEAPCOMMONI_H
