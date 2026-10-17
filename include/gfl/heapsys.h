#ifndef POKEBW2_GFL_HEAPSYS_H
#define POKEBW2_GFL_HEAPSYS_H

#include "types.h"
#include "gfl/heap.h"
#include "nnsys/fnd.h"

// The heap system under the heap API (heapsys.c and heap_dtcm.c, names the ROM does not embed). Heaps are NitroSystem
// expanded heaps by heap ID; a heap ID with HEAPID_TAIL_BIT allocates from the end. The functions record a
// HEAP_RESULT_* for GFL_HeapGetLastResult

enum {
    HEAP_RESULT_OK,
    HEAP_RESULT_1,
    HEAP_RESULT_2,
    HEAP_RESULT_3,
    HEAP_RESULT_4,
    HEAP_RESULT_5,
};

// What each allocation is preceded by
#define HEAP_BLOCK_MAGIC 0x194e

typedef struct {
    char file[0x12];
    u16 line;
} HeapDebugInfo;

typedef struct {
    HeapID heapId;
    u16 magic;
    HeapDebugInfo debug;
    u8 unk18[4];
#ifdef PLATFORM_PC
    // 32 bytes in all, so that the allocation after it keeps the 8-byte alignment a host's pointers need
    u8 hostPadding[4];
#endif
} HeapBlockHeader;

NNSFndHeapHandle GFL_HeapGetBase(HeapID heapId);
NNSFndHeapHandle GFL_HeapGetParentBase(HeapID heapId);
void *GFL_HeapGetRawPtr(HeapID heapId);
u32 GFL_HeapGetAllocationCountCore(HeapID heapId);
// Creates the root heaps, after reserving reserveSize bytes of the arena, with room for maxHeapIds heap IDs
BOOL GFL_HeapMgrInit(const HeapDef *defs, u16 rootCount, u16 maxHeapIds, u32 reserveSize);
BOOL GFL_HeapAddChild(HeapID parentHeapId, HeapID heapId, u32 size);
BOOL GFL_HeapAddRoot(void *memory, u32 size, HeapID heapId);
BOOL GFL_HeapAdd(NNSFndHeapHandle parent, HeapID heapId, void *memory, u32 size);
BOOL GFL_HeapDeleteCore(HeapID heapId);
void *GFL_HeapAllocateCore(HeapID heapId, u32 size);
BOOL GFL_HeapFreeCore(void *ptr);
BOOL GFL_HeapCreateAllocatorCore(NNSFndAllocator *allocator, HeapID heapId, int alignment);
BOOL GFL_HeapResizeCore(void *ptr, u32 size);
u32 GFL_HeapGetFreeSizeCore(HeapID heapId);
u32 GFL_HeapGetHighestAllocatableSize(HeapID heapId);
u32 GFL_HeapGetSize(HeapID heapId);
NNSFndHeapHandle GFL_HeapGetValidHeapBase(HeapID heapId);
u16 GFL_HeapGetAllocationCount(HeapID heapId);
HeapID GFL_HeapGetBlockHeapID(const void *ptr);
HeapDebugInfo *GFL_HeapGetBlockDebugInfoPtr(void *ptr);
BOOL GFL_HeapStatusCheck(HeapID heapId);
u16 GFL_HeapGetLastResult(void);

// A heap of DTCM, of at most 0x2f80 bytes
BOOL GFL_HeapDTCMInitCore(u32 size);
void *GFL_HeapDTCMAllocateCore(u32 size);
BOOL freeBlkFromDTCM(void *ptr);

#endif // POKEBW2_GFL_HEAPSYS_H
