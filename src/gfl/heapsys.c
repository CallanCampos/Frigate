#include "types.h"
#include "gfl/heapsys.h"
#include "nitro/os.h"
#include "nnsys/fnd.h"

// Heaps past the root heaps, for child heaps
#define CHILD_HEAP_COUNT 24
#define HEAP_INDEX_NONE 0xff

#ifdef PLATFORM_PC
// A host's structs are larger than the DS's (pointers are 8 bytes), so the platform layer sizes the heaps
u32 port_heap_size(u32 index, u32 size, BOOL root);
#define PORT_HEAP_SIZE(index, size, root) port_heap_size(index, size, root)
#else
#define PORT_HEAP_SIZE(index, size, root) size
#endif

typedef struct {
    NNSFndHeapHandle handle;
    // The heap and the memory a child heap is allocated from
    NNSFndHeapHandle parent;
    void *memory;
    u16 allocations;
    // Whether a heap without a parent was made from memory given to GFL_HeapAddRoot, which deleting it does not free
    u8 external;
} HeapEntry;

typedef struct {
    HeapEntry *heaps;
    // The index into heaps of each heap ID, or HEAP_INDEX_NONE
    u8 *indices;
    u16 maxHeapIds;
    u16 rootCount;
    u16 heapCount;
    u16 lastResult;
} HeapMgr;

static HeapMgr sHeapMgr;

NNSFndHeapHandle GFL_HeapGetBase(HeapID heapId) {
    u8 index = sHeapMgr.indices[heapId];

    if (index == HEAP_INDEX_NONE) {
        return NULL;
    }
    return sHeapMgr.heaps[index].handle;
}

NNSFndHeapHandle GFL_HeapGetParentBase(HeapID heapId) {
    u8 index = sHeapMgr.indices[heapId];

    if (index == HEAP_INDEX_NONE) {
        return NULL;
    }
    return sHeapMgr.heaps[index].parent;
}

void *GFL_HeapGetRawPtr(HeapID heapId) {
    u8 index = sHeapMgr.indices[heapId];

    if (index == HEAP_INDEX_NONE) {
        return NULL;
    }
    return sHeapMgr.heaps[index].memory;
}

u32 GFL_HeapGetAllocationCountCore(HeapID heapId) {
    u8 index = sHeapMgr.indices[heapId];

    if (index == HEAP_INDEX_NONE) {
        return 0;
    }
    return sHeapMgr.heaps[index].allocations;
}

BOOL GFL_HeapMgrInit(const HeapDef *defs, u16 rootCount, u16 maxHeapIds, u32 reserveSize) {
    u32 heapCount = rootCount + CHILD_HEAP_COUNT;
    u32 i;

    if (maxHeapIds < heapCount) {
        maxHeapIds = heapCount;
    }
    if (reserveSize != 0) {
        while (reserveSize % 4 != 0) {
            reserveSize++;
        }
        OS_AllocFromArenaLo(OS_ARENA_MAIN, reserveSize, 4);
    }
    sHeapMgr.heaps = OS_AllocFromArenaLo(OS_ARENA_MAIN, heapCount * sizeof(HeapEntry), 4);
    for (i = 0; i < heapCount; i++) {
        sHeapMgr.heaps[i].handle = NULL;
        sHeapMgr.heaps[i].parent = NULL;
        sHeapMgr.heaps[i].memory = NULL;
        sHeapMgr.heaps[i].allocations = 0;
        sHeapMgr.heaps[i].external = FALSE;
    }
    sHeapMgr.indices = OS_AllocFromArenaLo(OS_ARENA_MAIN, maxHeapIds, 4);
    for (i = 0; i < maxHeapIds; i++) {
        sHeapMgr.indices[i] = HEAP_INDEX_NONE;
    }
    sHeapMgr.maxHeapIds = maxHeapIds;
    sHeapMgr.rootCount = rootCount;
    sHeapMgr.heapCount = heapCount;
    sHeapMgr.lastResult = HEAP_RESULT_OK;
    for (i = 0; i < rootCount; i++) {
        void *memory = OS_AllocFromArenaLo(OS_ARENA_MAIN, PORT_HEAP_SIZE(i, defs[i].size, TRUE), 4);

        if (memory != NULL) {
            sHeapMgr.heaps[i].handle = NNS_FndCreateExpHeapEx(memory, PORT_HEAP_SIZE(i, defs[i].size, TRUE), 0);
            sHeapMgr.indices[i] = i;
        } else {
            sHeapMgr.lastResult = i;
            return FALSE;
        }
    }
    return TRUE;
}

BOOL GFL_HeapAddChild(HeapID parentHeapId, HeapID heapId, u32 size) {
    int alignment;
    NNSFndHeapHandle parent;
    void *memory;

    if (heapId & HEAPID_TAIL_BIT) {
        alignment = -4;
    } else {
        alignment = 4;
    }
    heapId &= HEAPID_TAIL_BIT - 1;
#ifdef PLATFORM_PC
    size = PORT_HEAP_SIZE(heapId, size, FALSE);
#endif
    if (sHeapMgr.indices[heapId] != HEAP_INDEX_NONE) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else if ((parent = GFL_HeapGetBase(parentHeapId & (HEAPID_TAIL_BIT - 1))) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_2;
    } else if ((memory = NNS_FndAllocFromExpHeapEx(parent, size, alignment)) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_3;
    } else if (GFL_HeapAdd(parent, heapId, memory, size)) {
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_HeapAddRoot(void *memory, u32 size, HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (sHeapMgr.indices[heapId] != HEAP_INDEX_NONE) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else if (GFL_HeapAdd(NULL, heapId, memory, size)) {
        sHeapMgr.heaps[sHeapMgr.indices[heapId]].external = TRUE;
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_HeapAdd(NNSFndHeapHandle parent, HeapID heapId, void *memory, u32 size) {
    int i;

    for (i = sHeapMgr.rootCount; i < sHeapMgr.heapCount; i++) {
        if (sHeapMgr.heaps[i].handle == NULL) {
            sHeapMgr.heaps[i].handle = NNS_FndCreateExpHeapEx(memory, size, 0);
            if (sHeapMgr.heaps[i].handle == NULL) {
                sHeapMgr.lastResult = HEAP_RESULT_5;
            } else {
                sHeapMgr.heaps[i].parent = parent;
                sHeapMgr.heaps[i].memory = memory;
                sHeapMgr.indices[heapId] = i;
                sHeapMgr.lastResult = HEAP_RESULT_OK;
                return TRUE;
            }
        }
    }
    sHeapMgr.lastResult = HEAP_RESULT_4;
    return FALSE;
}

BOOL GFL_HeapDeleteCore(HeapID heapId) {
    NNSFndHeapHandle heap;
    NNSFndHeapHandle parent;
    void *memory;
    u8 index;

    heapId &= HEAPID_TAIL_BIT - 1;
    heap = GFL_HeapGetBase(heapId);
    if (heap == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        parent = GFL_HeapGetParentBase(heapId);
        memory = GFL_HeapGetRawPtr(heapId);
        index = sHeapMgr.indices[heapId];
        NNS_FndDestroyExpHeap(heap);
        if (parent == NULL || memory == NULL) {
            if (!sHeapMgr.heaps[index].external) {
                sHeapMgr.lastResult = HEAP_RESULT_2;
                return FALSE;
            }
        } else {
            NNS_FndFreeToExpHeap(parent, memory);
        }
        sHeapMgr.heaps[index].handle = NULL;
        sHeapMgr.heaps[index].parent = NULL;
        sHeapMgr.heaps[index].memory = NULL;
        sHeapMgr.heaps[index].allocations = 0;
        sHeapMgr.heaps[index].external = FALSE;
        sHeapMgr.indices[heapId] = HEAP_INDEX_NONE;
        return TRUE;
    }
    return FALSE;
}

void *GFL_HeapAllocateCore(HeapID heapId, u32 size) {
    int alignment;
    NNSFndHeapHandle heap;
    u32 interrupts;
    HeapBlockHeader *block;

    if (heapId & HEAPID_TAIL_BIT) {
        alignment = -4;
    } else {
        alignment = 4;
    }
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else if ((heap = GFL_HeapGetBase(heapId)) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_2;
    } else {
        interrupts = OS_DisableInterrupts();
        size += sizeof(HeapBlockHeader);
        block = NNS_FndAllocFromExpHeapEx(heap, size, alignment);
        if (block == NULL) {
            sHeapMgr.lastResult = HEAP_RESULT_3;
        } else {
            block->magic = HEAP_BLOCK_MAGIC;
            block->heapId = heapId;
            sHeapMgr.heaps[sHeapMgr.indices[heapId]].allocations++;
            block++;
            sHeapMgr.lastResult = HEAP_RESULT_OK;
        }
        OS_RestoreInterrupts(interrupts);
        return block;
    }
    return NULL;
}

BOOL GFL_HeapFreeCore(void *ptr) {
    HeapBlockHeader *block = (HeapBlockHeader *)ptr - 1;
    HeapID heapId = ((HeapBlockHeader *)ptr - 1)->heapId;
    NNSFndHeapHandle heap;
    u32 interrupts;

    if (block->magic != HEAP_BLOCK_MAGIC) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_2;
    } else if ((heap = GFL_HeapGetBase(heapId)) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_3;
    } else if (GFL_HeapGetAllocationCountCore(heapId) == 0) {
        sHeapMgr.lastResult = HEAP_RESULT_4;
    } else {
        interrupts = OS_DisableInterrupts();
        sHeapMgr.heaps[sHeapMgr.indices[heapId]].allocations--;
        block->magic = HEAP_BLOCK_MAGIC - 1;
        NNS_FndFreeToExpHeap(heap, block);
        OS_RestoreInterrupts(interrupts);
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_HeapCreateAllocatorCore(NNSFndAllocator *allocator, HeapID heapId, int alignment) {
    if (alignment < 0) {
        alignment = -alignment;
    }
    if (heapId & HEAPID_TAIL_BIT) {
        alignment = -alignment;
    }
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        NNS_FndInitAllocatorForExpHeap(allocator, GFL_HeapGetBase(heapId), alignment);
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_HeapResizeCore(void *ptr, u32 size) {
    HeapBlockHeader *block = (HeapBlockHeader *)ptr - 1;
    NNSFndHeapHandle heap;
    u32 oldSize;
    u32 newSize;
    BOOL result;

    if (block->heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_2;
    } else if ((heap = GFL_HeapGetBase(block->heapId)) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_3;
    } else {
        oldSize = NNS_FndGetSizeForMBlockExpHeap(block);
        newSize = NNS_FndResizeForMBlockExpHeap(heap, block, size + sizeof(HeapBlockHeader));
        size += sizeof(HeapBlockHeader);
        result = TRUE;
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        if (oldSize < size) {
            if (newSize == 0) {
                sHeapMgr.lastResult = HEAP_RESULT_4;
                result = FALSE;
            }
        } else if (newSize == oldSize) {
            sHeapMgr.lastResult = HEAP_RESULT_5;
            result = FALSE;
        }
        return result;
    }
    return FALSE;
}

u32 GFL_HeapGetFreeSizeCore(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return NNS_FndGetTotalFreeSizeForExpHeap(GFL_HeapGetBase(heapId));
    }
    return 0;
}

u32 GFL_HeapGetHighestAllocatableSize(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return HeapBase_GetHighestAllocatableSize(GFL_HeapGetBase(heapId), 4);
    }
    return 0;
}

u32 GFL_HeapGetSize(HeapID heapId) {
    NNSiFndHeapHead *heap;

    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        heap = GFL_HeapGetBase(heapId);
        if (heap != NULL) {
            void *end = heap->heapEnd;

            sHeapMgr.lastResult = HEAP_RESULT_OK;
            return (u32)end - (u32)heap;
        }
    }
    return 0;
}

NNSFndHeapHandle GFL_HeapGetValidHeapBase(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return GFL_HeapGetBase(heapId);
    }
    return NULL;
}

u16 GFL_HeapGetAllocationCount(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else {
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return GFL_HeapGetAllocationCountCore(heapId);
    }
    return 0xffff;
}

HeapID GFL_HeapGetBlockHeapID(const void *ptr) {
    return ((const HeapBlockHeader *)ptr - 1)->heapId;
}

HeapDebugInfo *GFL_HeapGetBlockDebugInfoPtr(void *ptr) {
    HeapBlockHeader *block = (HeapBlockHeader *)((u8 *)ptr - sizeof(HeapBlockHeader));

    return &block->debug;
}

BOOL GFL_HeapStatusCheck(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heapId >= sHeapMgr.maxHeapIds) {
        sHeapMgr.lastResult = HEAP_RESULT_1;
    } else if (GFL_HeapGetBase(heapId) == NULL) {
        sHeapMgr.lastResult = HEAP_RESULT_2;
    } else {
        sHeapMgr.lastResult = HEAP_RESULT_OK;
        return TRUE;
    }
    return FALSE;
}

u16 GFL_HeapGetLastResult(void) {
    return sHeapMgr.lastResult;
}
