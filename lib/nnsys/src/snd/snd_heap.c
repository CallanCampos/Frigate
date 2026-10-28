#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "snd_internal.h"
#include <stddef.h>

// NitroSystem's sound heap (NNS_SndHeap): a frame heap for sound data. Each saved state opens a level of the heap,
// which lists the blocks allocated since, so that loading a state can call their dispose callbacks before freeing them.
// swan names NNS_SndHeapCreate NNS_FrmHeapCreate. The SDK's file is heap.c, and the names are NitroSystem's, as
// ntrtwl's reconstruction of NitroSystem has them, except BLOCK_HEAD_SIZE, which is ours

// Every block starts with its head, rounded up to the cache line
#define HEAP_ALIGN 32
#ifdef PLATFORM_PC
// A host's head, with its pointers and pointer-wide data, is larger than the DS's 32 bytes; the data starts after it
#define BLOCK_HEAD_SIZE ((u32)ROUNDUP(sizeof(NNSSndHeapBlock), HEAP_ALIGN))
#else
#define BLOCK_HEAD_SIZE 32
#endif
#define ROUNDUP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

typedef struct NNSSndHeapBlock {
    NNSFndLink link;
    u32 size;
    NNSSndHeapDisposeCallback callback;
    uptr data1;
    uptr data2;
} NNSSndHeapBlock;

#ifdef PLATFORM_PC
_Static_assert(BLOCK_HEAD_SIZE >= sizeof(NNSSndHeapBlock) && BLOCK_HEAD_SIZE % HEAP_ALIGN == 0,
               "a block's data starts after its head, at the heap's alignment");
#endif

typedef struct NNSSndHeapSection {
    NNSFndList blockList;
    NNSFndLink link;
} NNSSndHeapSection;

static void InitHeapSection(NNSSndHeapSection *section);
static BOOL InitHeap(NNSSndHeap *heap, NNSFndHeapHandle handle);
static BOOL NewSection(NNSSndHeap *heap);
static void EraseSync(void);

NNSSndHeapHandle NNS_SndHeapCreate(void *startAddress, u32 size) {
    NNSSndHeap *heap;
    NNSFndHeapHandle handle;
    void *endAddress = (u8 *)startAddress + size;

    startAddress = (void *)ROUNDUP((uptr)startAddress, 4);
    if (startAddress > endAddress) {
        return NNS_SND_HEAP_INVALID_HANDLE;
    }
    size = (u32)((u8 *)endAddress - (u8 *)startAddress);
    if (size < sizeof(NNSSndHeap)) {
        return NNS_SND_HEAP_INVALID_HANDLE;
    }
    size -= sizeof(NNSSndHeap);
    heap = startAddress;
    handle = NNS_FndCreateFrmHeapEx(heap + 1, size, 0);
    if (handle == NULL) {
        return NNS_SND_HEAP_INVALID_HANDLE;
    }
    if (!InitHeap(heap, handle)) {
        NNS_FndDestroyFrmHeap(handle);
        return NNS_SND_HEAP_INVALID_HANDLE;
    }
    return heap;
}

void NNS_SndHeapDestroy(NNSSndHeapHandle heap) {
    NNS_SndHeapClear(heap);
    NNS_FndDestroyFrmHeap(heap->handle);
}

void NNS_SndHeapClear(NNSSndHeapHandle heap) {
    NNSSndHeapSection *section;
    NNSSndHeapBlock *block;
    BOOL doCallback = FALSE;

    while ((section = NNS_FndGetPrevListObject(&heap->sectionList, NULL)) != NULL) {
        block = NULL;
        while ((block = NNS_FndGetPrevListObject(&section->blockList, block)) != NULL) {
            if (block->callback != NULL) {
                block->callback((u8 *)block + BLOCK_HEAD_SIZE, block->size, block->data1, block->data2);
                doCallback = TRUE;
            }
        }
        NNS_FndRemoveListObject(&heap->sectionList, section);
    }
#ifdef PLATFORM_PC
    // H8: disposal queues ARM7 invalidation, and its wait can advance audio still reading these owners.
    // Complete that wait before the host frame heap poisons or reuses their memory.
    if (doCallback) {
        EraseSync();
    }
    NNS_FndFreeToFrmHeap(heap->handle, NNS_FND_FRMHEAP_FREE_ALL);
#else
    NNS_FndFreeToFrmHeap(heap->handle, NNS_FND_FRMHEAP_FREE_ALL);
    if (doCallback) {
        EraseSync();
    }
#endif
    NewSection(heap);
}

void *NNS_SndHeapAlloc(NNSSndHeapHandle heap, u32 size, NNSSndHeapDisposeCallback callback, uptr data1, uptr data2) {
    NNSSndHeapSection *section;
    NNSSndHeapBlock *block =
        NNS_FndAllocFromFrmHeapEx(heap->handle, ROUNDUP(size, HEAP_ALIGN) + BLOCK_HEAD_SIZE, HEAP_ALIGN);

    if (block == NULL) {
        return NULL;
    }
    section = NNS_FndGetPrevListObject(&heap->sectionList, NULL);
    block->size = size;
    block->callback = callback;
    block->data1 = data1;
    block->data2 = data2;
    NNS_FndAppendListObject(&section->blockList, block);
    return (u8 *)block + BLOCK_HEAD_SIZE;
}

int NNS_SndHeapSaveState(NNSSndHeapHandle heap) {
    if (!NNS_FndRecordStateForFrmHeap(heap->handle, heap->sectionList.numObjects)) {
        return -1;
    }
    if (!NewSection(heap)) {
        NNS_FndFreeByStateToFrmHeap(heap->handle, 0);
        return -1;
    }
    return heap->sectionList.numObjects - 1;
}

void NNS_SndHeapLoadState(NNSSndHeapHandle heap, int level) {
    NNSSndHeapSection *section;
    NNSSndHeapBlock *block = NULL;
    BOOL doCallback = FALSE;

    if (level == 0) {
        NNS_SndHeapClear(heap);
        return;
    }
    while (level < heap->sectionList.numObjects) {
        section = NNS_FndGetPrevListObject(&heap->sectionList, NULL);
        while ((block = NNS_FndGetPrevListObject(&section->blockList, block)) != NULL) {
            if (block->callback != NULL) {
                block->callback((u8 *)block + BLOCK_HEAD_SIZE, block->size, block->data1, block->data2);
                doCallback = TRUE;
            }
        }
        NNS_FndRemoveListObject(&heap->sectionList, section);
    }
#ifdef PLATFORM_PC
    // H8: disposal queues ARM7 invalidation, and its wait can advance audio still reading these owners.
    // Complete that wait before the host frame heap poisons or reuses their memory.
    if (doCallback) {
        EraseSync();
    }
    NNS_FndFreeByStateToFrmHeap(heap->handle, (u32)level);
#else
    NNS_FndFreeByStateToFrmHeap(heap->handle, (u32)level);
    if (doCallback) {
        EraseSync();
    }
#endif
    NNS_FndRecordStateForFrmHeap(heap->handle, heap->sectionList.numObjects);
    NewSection(heap);
}

int NNS_SndHeapGetCurrentLevel(NNSSndHeapHandle heap) {
    return heap->sectionList.numObjects - 1;
}

static void InitHeapSection(NNSSndHeapSection *section) {
    NNS_FndInitList(&section->blockList, offsetof(NNSSndHeapBlock, link));
}

static BOOL InitHeap(NNSSndHeap *heap, NNSFndHeapHandle handle) {
    NNS_FndInitList(&heap->sectionList, offsetof(NNSSndHeapSection, link));
    heap->handle = handle;
    if (!NewSection(heap)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL NewSection(NNSSndHeap *heap) {
    NNSSndHeapSection *section = NNS_FndAllocFromFrmHeapEx(heap->handle, sizeof(NNSSndHeapSection), 4);

    if (section == NULL) {
        return FALSE;
    }
    InitHeapSection(section);
    NNS_FndAppendListObject(&heap->sectionList, section);
    return TRUE;
}

// Waits for the sound driver to have run every command sent, so that nothing plays from the memory freed
static void EraseSync(void) {
    u32 tag = SND_GetCurrentCommandTag();

    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
}
