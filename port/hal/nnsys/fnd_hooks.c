// NNS FND's frees, wrapped so that the trees the host's loaders built from a file (port/hal/nnsys/reloc.h) die with the
// memory the file was loaded into. port/cmake/nnsys.cmake compiles expheap.c, frameheap.c and unitheap.c with every
// function that gives memory back renamed port_sdk_<name>; each function of the original name here measures what the
// call frees, calls the SDK's, then releases the trees in that memory. The GFL heap (src/gfl/heapsys.c) frees and
// deletes through these, the sound heap's frame heap too, and so does everything else that frees NNS heap memory.
//
// The functions that free:
//   - NNS_FndFreeToExpHeap: the block's data
//   - NNS_FndResizeForMBlockExpHeap: when it shrinks a block, the cut-off end
//   - NNS_FndDestroyExpHeap, NNS_FndDestroyFrmHeap: the whole heap, its heads included
//   - NNS_FndFreeToFrmHeap: everything from the bottom, from the top or both
//   - NNS_FndFreeByStateToFrmHeap: what was allocated at either end since the recorded state
//   - NNS_FndUnitHeapFree: one unit
// Memory the game reuses without one of these (a static or stack buffer, or the OS arena) is seen when a file there is
// loaded again: the loaders check a registered tree against the file's bytes before returning it.
#include "hal/nnsys/reloc.h"
#include "nnsys/fnd.h"

void port_sdk_NNS_FndDestroyExpHeap(NNSFndHeapHandle heap);
u32 port_sdk_NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle heap, void *memBlock, u32 size);
void port_sdk_NNS_FndFreeToExpHeap(NNSFndHeapHandle heap, void *memBlock);
void port_sdk_NNS_FndDestroyFrmHeap(NNSFndHeapHandle heap);
void port_sdk_NNS_FndFreeToFrmHeap(NNSFndHeapHandle heap, int mode);
BOOL port_sdk_NNS_FndFreeByStateToFrmHeap(NNSFndHeapHandle heap, u32 tag);
void port_sdk_NNS_FndUnitHeapFree(NNSFndHeapHandle heap, void *memBlock);

// The frame and unit heaps' own heads follow the common one, as frameheap.c and unitheap.c find them
static NNSiFndFrameHeap *FrameHeapOf(NNSFndHeapHandle heap) {
    return (NNSiFndFrameHeap *)((u8 *)heap + sizeof(NNSiFndHeapHead));
}

static NNSiFndUnitHeap *UnitHeapOf(NNSFndHeapHandle heap) {
    return (NNSiFndUnitHeap *)((u8 *)heap + sizeof(NNSiFndHeapHead));
}

// [from, to), when it isn't empty
static void ReleaseBetween(const void *from, const void *to) {
    if ((const u8 *)from < (const u8 *)to) {
        port_reloc_release_range(from, (size_t)((const u8 *)to - (const u8 *)from));
    }
}

// Destroying a heap frees all of it: from its common head, where its memory starts, to its end
void NNS_FndDestroyExpHeap(NNSFndHeapHandle heap) {
    void *end = heap->heapEnd;

    port_sdk_NNS_FndDestroyExpHeap(heap);
    ReleaseBetween(heap, end);
}

u32 NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle heap, void *memBlock, u32 size) {
    u32 oldSize = NNS_FndGetSizeForMBlockExpHeap(memBlock);
    u32 newSize = port_sdk_NNS_FndResizeForMBlockExpHeap(heap, memBlock, size);

    // 0 when it couldn't grow; a block that grows frees nothing
    if (newSize != 0 && newSize < oldSize) {
        ReleaseBetween((u8 *)memBlock + newSize, (u8 *)memBlock + oldSize);
    }
    return newSize;
}

void NNS_FndFreeToExpHeap(NNSFndHeapHandle heap, void *memBlock) {
    u32 size = NNS_FndGetSizeForMBlockExpHeap(memBlock);

    port_sdk_NNS_FndFreeToExpHeap(heap, memBlock);
    ReleaseBetween(memBlock, (u8 *)memBlock + size);
}

void NNS_FndDestroyFrmHeap(NNSFndHeapHandle heap) {
    void *end = heap->heapEnd;

    port_sdk_NNS_FndDestroyFrmHeap(heap);
    ReleaseBetween(heap, end);
}

// What a frame heap frees is what it held before and no longer holds: the bottom shrinks to a lower low, the top to a
// higher high
static void ReleaseFrameHeapChange(void *lowBefore, void *highBefore, NNSiFndFrameHeap *frmHeap) {
    ReleaseBetween(frmHeap->low, lowBefore);
    ReleaseBetween(highBefore, frmHeap->high);
}

void NNS_FndFreeToFrmHeap(NNSFndHeapHandle heap, int mode) {
    NNSiFndFrameHeap *frmHeap = FrameHeapOf(heap);
    void *low = frmHeap->low;
    void *high = frmHeap->high;

    port_sdk_NNS_FndFreeToFrmHeap(heap, mode);
    ReleaseFrameHeapChange(low, high, frmHeap);
}

BOOL NNS_FndFreeByStateToFrmHeap(NNSFndHeapHandle heap, u32 tag) {
    NNSiFndFrameHeap *frmHeap = FrameHeapOf(heap);
    void *low = frmHeap->low;
    void *high = frmHeap->high;
    BOOL result = port_sdk_NNS_FndFreeByStateToFrmHeap(heap, tag);

    ReleaseFrameHeapChange(low, high, frmHeap);
    return result;
}

void NNS_FndUnitHeapFree(NNSFndHeapHandle heap, void *memBlock) {
    u32 unitSize = UnitHeapOf(heap)->unitSize;

    port_sdk_NNS_FndUnitHeapFree(heap, memBlock);
    ReleaseBetween(memBlock, (u8 *)memBlock + unitSize);
}
