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
//
// The heaps' alignment is wrapped too. The DS needs 4 bytes for everything the game keeps in a heap, a host 8 for its
// pointers, so expanded and frame heaps start on 8 bytes, every block is aligned to at least 8 (towards the same end
// for a negative alignment), and an expanded heap's blocks are a multiple of 8 long, which keeps the block heads FND
// writes between them aligned as well. GFL's own head before each allocation is 32 bytes on a host for the same reason
// (include/gfl/heapsys.h).
//
// In the sanitized build (PORT_SANITIZE, port/docs/sanitizers.md), the wrappers also tell AddressSanitizer what is
// allocated in the expanded and frame heaps, which hold nearly all of the game's memory: the GFL heaps and everything
// in them, and the sound heap. A heap's memory is poisoned when it is created, a block is made addressable when it is
// allocated, for the bytes the DS would give it (the size asked for, rounded up to 4), and poisoned again when it is
// freed, and a destroyed heap's memory goes back to its owner. So an access past a block's end, into FND's block heads
// or free memory, or to a block after it was freed, is an ASan report naming the game's code. FND itself
// (lib/nnsys/src/fnd's heaps) is compiled without ASan, as a C library's malloc is, so that it can write its own heads
// and free lists (port/cmake/nnsys.cmake defines PORT_POISON_HEAPS here then). Unit heaps, which the game doesn't
// create, are left alone. Where the game reads past a block as the DS does, and uses what it read (the copyright
// screen's characters, src/gfl/arc_util.c), port_heap_ds_read_begin and port_heap_ds_read_end, at the end of this file
// (declared in port/include/port_host.h), let that one read reach what follows the block, then poison again what was
// poisoned there. In every build, port_heap_ds_read_begin first checks that the read stays in the game's memory: in
// one of the expanded and frame heaps alive (which the wrappers of their creation and destruction keep a list of), or
// else in the main arena they are carved from (port/hal/os/os_arena.c); a read that reaches outside both stops the
// program with a message, rather than read, and unpoison, memory that isn't the game's.
#include "hal/nnsys/reloc.h"
#include "hal/os/port_os.h"
#include "nnsys/fnd.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef PORT_POISON_HEAPS
#include <sanitizer/asan_interface.h>
#define POISON(start, size) ASAN_POISON_MEMORY_REGION(start, size)
#define UNPOISON(start, size) ASAN_UNPOISON_MEMORY_REGION(start, size)
#else
#define POISON(start, size) ((void)(start), (void)(size))
#define UNPOISON(start, size) ((void)(start), (void)(size))
#endif

#define HOST_HEAP_ALIGN 8

NNSFndHeapHandle port_sdk_NNS_FndCreateExpHeapEx(void *startAddress, u32 size, u16 optFlag);
void *port_sdk_NNS_FndAllocFromExpHeapEx(NNSFndHeapHandle heap, u32 size, int alignment);
NNSFndHeapHandle port_sdk_NNS_FndCreateFrmHeapEx(void *startAddress, u32 size, u16 optFlag);
void *port_sdk_NNS_FndAllocFromFrmHeapEx(NNSFndHeapHandle heap, u32 size, int alignment);
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

// [from, to) given back to its heap: its trees are released, and ASan sees it freed
static void FreeBetween(const void *from, const void *to) {
    ReleaseBetween(from, to);
    if ((const u8 *)from < (const u8 *)to) {
        POISON(from, (size_t)((const u8 *)to - (const u8 *)from));
    }
}

// What the DS gives a block of size bytes: FND rounds every size up to 4, and 0 to 4
static u32 DsBlockSize(u32 size) {
    return ((size == 0 ? 1 : size) + 3) & ~3u;
}

// A new heap's memory, from the end of its heads, is free until something is allocated from it
static NNSFndHeapHandle PoisonNewHeap(NNSFndHeapHandle heap) {
    if (heap != NULL) {
        POISON(heap->heapStart, (size_t)((u8 *)heap->heapEnd - (u8 *)heap->heapStart));
    }
    return heap;
}

// A block allocated from a heap: the DS's size of it is addressable, and the host's rounding after it is not
static void *UnpoisonBlock(void *block, u32 size) {
    if (block != NULL) {
        UNPOISON(block, DsBlockSize(size));
    }
    return block;
}

// A destroyed heap's memory, heads included, is its owner's again: a block of another heap or the arena it came from
static void UnpoisonDestroyedHeap(void *heap, void *end) {
    UNPOISON(heap, (size_t)((u8 *)end - (u8 *)heap));
}

// The expanded and frame heaps alive, each from its common head, where its memory starts, to its end
typedef struct LiveHeap {
    NNSFndHeapHandle heap;
    const u8 *end;
} LiveHeap;

static LiveHeap *sLiveHeaps;
static size_t sLiveHeapCount;
static size_t sLiveHeapCapacity;

static NNSFndHeapHandle AddLiveHeap(NNSFndHeapHandle heap) {
    if (heap == NULL) {
        return NULL;
    }
    if (sLiveHeapCount == sLiveHeapCapacity) {
        size_t capacity = sLiveHeapCapacity == 0 ? 64 : sLiveHeapCapacity * 2;
        LiveHeap *heaps = realloc(sLiveHeaps, capacity * sizeof(*heaps));

        if (heaps == NULL) {
            fprintf(stderr, "port: fnd_hooks: out of memory for the list of the game's heaps\n");
            abort();
        }
        sLiveHeaps = heaps;
        sLiveHeapCapacity = capacity;
    }
    sLiveHeaps[sLiveHeapCount].heap = heap;
    sLiveHeaps[sLiveHeapCount].end = heap->heapEnd;
    sLiveHeapCount++;
    return heap;
}

static void RemoveLiveHeap(NNSFndHeapHandle heap) {
    for (size_t i = 0; i < sLiveHeapCount; i++) {
        if (sLiveHeaps[i].heap == heap) {
            sLiveHeaps[i] = sLiveHeaps[--sLiveHeapCount];
            return;
        }
    }
}

// Whether [start, end) lies in the memory of one heap alive, its heads included, or else in the main arena: the game's
// memory, which a read the DS makes past a block may reach
static int InGameMemory(const u8 *start, const u8 *end) {
    for (size_t i = 0; i < sLiveHeapCount; i++) {
        if (start >= (const u8 *)sLiveHeaps[i].heap && end <= sLiveHeaps[i].end) {
            return 1;
        }
    }
    return port_os_main_arena_holds(start, (size_t)(end - start));
}

static int HostAlignment(int alignment) {
    if (alignment < 0) {
        return alignment > -HOST_HEAP_ALIGN ? -HOST_HEAP_ALIGN : alignment;
    }
    return alignment < HOST_HEAP_ALIGN ? HOST_HEAP_ALIGN : alignment;
}

// A block's size, which FND itself rounds to 4; 0 is 1, as for FND
static u32 HostBlockSize(u32 size) {
    return ((size == 0 ? 1 : size) + HOST_HEAP_ALIGN - 1) & ~(u32)(HOST_HEAP_ALIGN - 1);
}

// The start of a heap's memory moved up to the host's alignment, and its size less what that skipped; a size too small
// to reach it is 0, which FND refuses
static void *HostHeapStart(void *startAddress, u32 *size) {
    uptr skipped = (HOST_HEAP_ALIGN - (uptr)startAddress % HOST_HEAP_ALIGN) % HOST_HEAP_ALIGN;

    *size = *size > skipped ? *size - (u32)skipped : 0;
    return (u8 *)startAddress + skipped;
}

NNSFndHeapHandle NNS_FndCreateExpHeapEx(void *startAddress, u32 size, u16 optFlag) {
    startAddress = HostHeapStart(startAddress, &size);
    return PoisonNewHeap(AddLiveHeap(port_sdk_NNS_FndCreateExpHeapEx(startAddress, size, optFlag)));
}

void *NNS_FndAllocFromExpHeapEx(NNSFndHeapHandle heap, u32 size, int alignment) {
    return UnpoisonBlock(port_sdk_NNS_FndAllocFromExpHeapEx(heap, HostBlockSize(size), HostAlignment(alignment)), size);
}

NNSFndHeapHandle NNS_FndCreateFrmHeapEx(void *startAddress, u32 size, u16 optFlag) {
    startAddress = HostHeapStart(startAddress, &size);
    return PoisonNewHeap(AddLiveHeap(port_sdk_NNS_FndCreateFrmHeapEx(startAddress, size, optFlag)));
}

void *NNS_FndAllocFromFrmHeapEx(NNSFndHeapHandle heap, u32 size, int alignment) {
    return UnpoisonBlock(port_sdk_NNS_FndAllocFromFrmHeapEx(heap, size, HostAlignment(alignment)), size);
}

// Destroying a heap frees all of it: from its common head, where its memory starts, to its end. Its trees are
// released, and the memory is its owner's again
void NNS_FndDestroyExpHeap(NNSFndHeapHandle heap) {
    void *end = heap->heapEnd;

    RemoveLiveHeap(heap);
    port_sdk_NNS_FndDestroyExpHeap(heap);
    ReleaseBetween(heap, end);
    UnpoisonDestroyedHeap(heap, end);
}

u32 NNS_FndResizeForMBlockExpHeap(NNSFndHeapHandle heap, void *memBlock, u32 size) {
    u32 oldSize = NNS_FndGetSizeForMBlockExpHeap(memBlock);
    u32 newSize = port_sdk_NNS_FndResizeForMBlockExpHeap(heap, memBlock, HostBlockSize(size));

    // 0 when it couldn't grow; a block that grows frees nothing. Both sizes are the host's, a multiple of 8, and FND
    // keeps the old one when it refuses a shrink too small to leave a free block, so they say nothing of the DS's: a
    // resized block always has the DS's size of the new one addressable, and nothing after it
    if (newSize != 0 && newSize < oldSize) {
        FreeBetween((u8 *)memBlock + newSize, (u8 *)memBlock + oldSize);
    }
    if (newSize != 0) {
        POISON(memBlock, newSize > oldSize ? newSize : oldSize);
        UnpoisonBlock(memBlock, size);
    }
    return newSize;
}

void NNS_FndFreeToExpHeap(NNSFndHeapHandle heap, void *memBlock) {
    u32 size = NNS_FndGetSizeForMBlockExpHeap(memBlock);

    port_sdk_NNS_FndFreeToExpHeap(heap, memBlock);
    FreeBetween(memBlock, (u8 *)memBlock + size);
}

void NNS_FndDestroyFrmHeap(NNSFndHeapHandle heap) {
    void *end = heap->heapEnd;

    RemoveLiveHeap(heap);
    port_sdk_NNS_FndDestroyFrmHeap(heap);
    ReleaseBetween(heap, end);
    UnpoisonDestroyedHeap(heap, end);
}

// What a frame heap frees is what it held before and no longer holds: the bottom shrinks to a lower low, the top to a
// higher high
static void ReleaseFrameHeapChange(void *lowBefore, void *highBefore, NNSiFndFrameHeap *frmHeap) {
    FreeBetween(frmHeap->low, lowBefore);
    FreeBetween(highBefore, frmHeap->high);
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

#ifdef PORT_POISON_HEAPS
// ASan's shadow granule: one shadow byte says how many of these 8 bytes are addressable, from the first
#define ASAN_GRANULE 8

// The read the DS makes past a block that is under way: the span it may reach, widened to whole granules, and which of
// the span's bytes were poisoned when it began
static struct {
    u8 *start;
    size_t size;
    u8 *poisoned;
} sDsRead;
#endif

// In the sanitized build the span is widened to whole granules, which is what it unpoisons, so that is what must lie in
// the game's memory
void port_heap_ds_read_begin(const void *start, unsigned int size) {
#ifdef PORT_POISON_HEAPS
    uptr from = (uptr)start & ~(uptr)(ASAN_GRANULE - 1);
    uptr to = ((uptr)start + size + ASAN_GRANULE - 1) & ~(uptr)(ASAN_GRANULE - 1);
    size_t i;
#else
    uptr from = (uptr)start;
    uptr to = (uptr)start + size;
#endif

    if (size != 0 && !InGameMemory((const u8 *)from, (const u8 *)to)) {
        fprintf(stderr,
                "port: fnd_hooks: a read past a heap block, 0x%x bytes from %p, reaches outside the game's heaps and "
                "the main arena\n",
                size, start);
        abort();
    }
#ifdef PORT_POISON_HEAPS
    if (sDsRead.poisoned != NULL) {
        fprintf(stderr, "port: fnd_hooks: a read past a heap block began while another was under way\n");
        abort();
    }
    if (size == 0) {
        return;
    }
    sDsRead.start = (u8 *)from;
    sDsRead.size = (size_t)(to - from);
    sDsRead.poisoned = malloc(sDsRead.size);
    if (sDsRead.poisoned == NULL) {
        fprintf(stderr, "port: fnd_hooks: out of memory for a read past a heap block\n");
        abort();
    }
    for (i = 0; i < sDsRead.size; i++) {
        sDsRead.poisoned[i] = (u8)__asan_address_is_poisoned(sDsRead.start + i);
    }
    UNPOISON(sDsRead.start, sDsRead.size);
#endif
}

// A granule's poisoned bytes are always its last ones (its shadow byte counts the addressable ones from the first), so
// each run of bytes that were poisoned starts within a granule and ends on a granule's end: poisoning the runs again,
// in address order, gives the shadow back as it was before the read
void port_heap_ds_read_end(void) {
#ifdef PORT_POISON_HEAPS
    size_t i = 0;

    if (sDsRead.poisoned == NULL) {
        return;
    }
    while (i < sDsRead.size) {
        size_t first = i;

        if (!sDsRead.poisoned[i]) {
            i++;
            continue;
        }
        while (i < sDsRead.size && sDsRead.poisoned[i]) {
            i++;
        }
        POISON(sDsRead.start + first, i - first);
    }
    free(sDsRead.poisoned);
    sDsRead.poisoned = NULL;
    sDsRead.size = 0;
#endif
}
