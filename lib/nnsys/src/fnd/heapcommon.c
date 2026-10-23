#include "heapcommoni.h"
#include "nnsys/fnd.h"

// The part every NitroSystem heap shares. Each heap is linked into the child list of the heap whose memory it was
// made in, or into a root list when no heap holds it. swan calls NNSi_FndInitHeapHead SetHeapBaseInfo

static NNSiFndHeapHead *FindContainHeap(NNSFndList *list, const void *memBlock);
static NNSFndList *FindListContainHeap(NNSiFndHeapHead *heap);

static BOOL sRootListInitialized;
static NNSFndList sRootList;

// Searches list, and the children of the heap that holds memBlock, for the deepest heap holding memBlock
static NNSiFndHeapHead *FindContainHeap(NNSFndList *list, const void *memBlock) {
    NNSiFndHeapHead *candidate;

    for (candidate = NNS_FndGetNextListObject(list, NULL); candidate != NULL;
         candidate = NNS_FndGetNextListObject(list, candidate)) {
        if (NNSiGetUIntPtr(candidate->heapStart) <= NNSiGetUIntPtr(memBlock) &&
            NNSiGetUIntPtr(memBlock) < NNSiGetUIntPtr(candidate->heapEnd)) {
            NNSiFndHeapHead *inner = FindContainHeap(&candidate->childList, memBlock);

            if (inner != NULL) {
                return inner;
            }
            return candidate;
        }
    }
    return NULL;
}

// Where heap is to be linked
static NNSFndList *FindListContainHeap(NNSiFndHeapHead *heap) {
    NNSFndList *owner = &sRootList;
    NNSiFndHeapHead *container = FindContainHeap(&sRootList, heap);

    if (container != NULL) {
        owner = &container->childList;
    }
    return owner;
}

void NNSi_FndInitHeapHead(NNSiFndHeapHead *heap, u32 signature, void *heapStart, void *heapEnd, u16 optFlag) {
    heap->signature = signature;
    heap->heapStart = heapStart;
    heap->heapEnd = heapEnd;
    // The code clears the whole attribute, then clears and sets the option byte in it
    heap->attribute = 0;
    heap->attribute &= ~HEAP_OPT_MASK;
    heap->attribute |= optFlag & HEAP_OPT_MASK;
    NNS_FndInitList(&heap->childList, offsetof(NNSiFndHeapHead, link));
    if (!sRootListInitialized) {
        NNS_FndInitList(&sRootList, offsetof(NNSiFndHeapHead, link));
        sRootListInitialized = TRUE;
    }
    NNS_FndAppendListObject(FindListContainHeap(heap), heap);
}

void NNSi_FndFinalizeHeap(NNSiFndHeapHead *heap) {
    NNS_FndRemoveListObject(FindListContainHeap(heap), heap);
}
