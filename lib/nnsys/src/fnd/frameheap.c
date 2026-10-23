#include "heapcommoni.h"
#include "nnsys/fnd.h"

// NitroSystem's frame heaps (NNS_Fnd). Allocating moves one pointer up from the bottom or another down from the top;
// nothing is freed on its own, only everything from one end, or everything since a recorded state. swan names only
// InitFrameHeap, as InitHeapBaseFRMH. AllocFromHead and AllocFromTail have the names of expheap.c's statics, as they do
// in NitroSystem

#define NNSI_FRMHEAP_SIGNATURE 0x46524D48 // 'FRMH'

// The heap needs room for its heads
#define MIN_HEAP_SIZE (sizeof(NNSiFndHeapHead) + sizeof(NNSiFndFrmHeapHead))

static NNSiFndHeapHead *InitFrameHeap(void *start, void *end, u16 optFlag);
static void *AllocFromHead(NNSiFndFrmHeapHead *frmHeap, u32 size, int alignment);
static void *AllocFromTail(NNSiFndFrmHeapHead *frmHeap, u32 size, int alignment);
static void FreeHead(NNSiFndHeapHead *heap);
static void FreeTail(NNSiFndHeapHead *heap);

// The frame heap's own head, just after the common one
static inline NNSiFndFrmHeapHead *GetFrmHeapHeadPtrFromHeapHead(NNSiFndHeapHead *heap) {
    return (NNSiFndFrmHeapHead *)(NNSiGetUIntPtr(heap) + sizeof(NNSiFndHeapHead));
}

static inline NNSiFndHeapHead *GetHeapHeadPtrFromFrmHeapHead(NNSiFndFrmHeapHead *frmHeap) {
    return (NNSiFndHeapHead *)(NNSiGetUIntPtr(frmHeap) - sizeof(NNSiFndHeapHead));
}

static NNSiFndHeapHead *InitFrameHeap(void *start, void *end, u16 optFlag) {
    NNSiFndHeapHead *heap = start;
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);

    NNSi_FndInitHeapHead(heap, NNSI_FRMHEAP_SIGNATURE, (void *)(NNSiGetUIntPtr(frmHeap) + sizeof(NNSiFndFrmHeapHead)),
                         end, optFlag);
    frmHeap->headAllocator = heap->heapStart;
    frmHeap->tailAllocator = heap->heapEnd;
    frmHeap->pState = NULL;
    return heap;
}

static void *AllocFromHead(NNSiFndFrmHeapHead *frmHeap, u32 size, int alignment) {
    void *block = (void *)((NNSiGetUIntPtr(frmHeap->headAllocator) + (alignment - 1)) & ~(alignment - 1));
    void *newHead = (void *)(NNSiGetUIntPtr(block) + size);

    if (NNSiGetUIntPtr(newHead) > NNSiGetUIntPtr(frmHeap->tailAllocator)) {
        return NULL;
    }
    FillAllocMemory(GetHeapHeadPtrFromFrmHeapHead(frmHeap), frmHeap->headAllocator,
                    GetOffsetFromPtr(frmHeap->headAllocator, newHead));
    frmHeap->headAllocator = newHead;
    return block;
}

static void *AllocFromTail(NNSiFndFrmHeapHead *frmHeap, u32 size, int alignment) {
    void *block = (void *)((NNSiGetUIntPtr(frmHeap->tailAllocator) - size) & ~(alignment - 1));

    if (NNSiGetUIntPtr(block) < NNSiGetUIntPtr(frmHeap->headAllocator)) {
        return NULL;
    }
    FillAllocMemory(GetHeapHeadPtrFromFrmHeapHead(frmHeap), block, GetOffsetFromPtr(block, frmHeap->tailAllocator));
    frmHeap->tailAllocator = block;
    return block;
}

// Frees everything taken from the bottom, and forgets the recorded states, which lived there
static void FreeHead(NNSiFndHeapHead *heap) {
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);

    frmHeap->headAllocator = heap->heapStart;
    frmHeap->pState = NULL;
}

// Frees everything taken from the top, in the recorded states as well
static void FreeTail(NNSiFndHeapHead *heap) {
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);
    NNSiFndFrmHeapState *state;

    for (state = frmHeap->pState; state != NULL; state = state->pPrevState) {
        state->tailAllocator = heap->heapEnd;
    }
    frmHeap->tailAllocator = heap->heapEnd;
}

NNSFndHeapHandle NNS_FndCreateFrmHeapEx(void *startAddress, u32 size, u16 optFlag) {
    void *end = (void *)((NNSiGetUIntPtr(startAddress) + size) & ~3);

    startAddress = (void *)((NNSiGetUIntPtr(startAddress) + 3) & ~3);
    if (NNSiGetUIntPtr(startAddress) > NNSiGetUIntPtr(end) || GetOffsetFromPtr(startAddress, end) < MIN_HEAP_SIZE) {
        return NULL;
    }
    return InitFrameHeap(startAddress, end, optFlag);
}

void NNS_FndDestroyFrmHeap(NNSFndHeapHandle heap) {
    NNSi_FndFinalizeHeap(heap);
}

// A negative alignment takes the block from the top
void *NNS_FndAllocFromFrmHeapEx(NNSFndHeapHandle heap, u32 size, int alignment) {
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);

    if (size == 0) {
        size = 1;
    }
    size = (size + 3) & ~3;
    if (alignment >= 0) {
        return AllocFromHead(frmHeap, size, alignment);
    } else {
        return AllocFromTail(frmHeap, size, -alignment);
    }
}

// mode is a set of NNS_FND_FRMHEAP_FREE_* ends to free
void NNS_FndFreeToFrmHeap(NNSFndHeapHandle heap, int mode) {
    if (mode & NNS_FND_FRMHEAP_FREE_HEAD) {
        FreeHead(heap);
    }
    if (mode & NNS_FND_FRMHEAP_FREE_TAIL) {
        FreeTail(heap);
    }
}

// Saves both pointers under tag, in a record taken from the bottom of the heap. Returns FALSE when there's no
// room for it
BOOL NNS_FndRecordStateForFrmHeap(NNSFndHeapHandle heap, u32 tag) {
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);
    void *headBefore = frmHeap->headAllocator;
    NNSiFndFrmHeapState *record = AllocFromHead(frmHeap, sizeof(NNSiFndFrmHeapState), 4);

    if (record == NULL) {
        return FALSE;
    }
    record->tagName = tag;
    record->headAllocator = headBefore;
    record->tailAllocator = frmHeap->tailAllocator;
    record->pPrevState = frmHeap->pState;
    frmHeap->pState = record;
    return TRUE;
}

// Goes back to the state saved under tag, or to the latest one when tag is 0. Returns FALSE when there's no
// such state
BOOL NNS_FndFreeByStateToFrmHeap(NNSFndHeapHandle heap, u32 tag) {
    NNSiFndFrmHeapHead *frmHeap = GetFrmHeapHeadPtrFromHeapHead(heap);
    NNSiFndFrmHeapState *record = frmHeap->pState;

    if (tag != 0) {
        for (; record != NULL; record = record->pPrevState) {
            if (record->tagName == tag) {
                break;
            }
        }
    }
    if (record == NULL) {
        return FALSE;
    }
    frmHeap->headAllocator = record->headAllocator;
    frmHeap->tailAllocator = record->tailAllocator;
    frmHeap->pState = record->pPrevState;
    return TRUE;
}
