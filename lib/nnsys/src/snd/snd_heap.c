#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "snd_internal.h"
#include <stddef.h>

// NitroSystem's sound heap (NNS_SndHeap): a frame heap for sound data. Each saved state opens a level of the heap,
// which lists the blocks allocated since, so that loading a state can call their dispose callbacks before freeing
// them. swan names NNS_SndHeapCreate NNS_SndHeapCreate. The file name is a guess, and so are the statics' names

// Every block starts with its head, rounded up to the cache line
#define HEAP_ALIGN 32
#define BLOCK_HEAD_SIZE 32
#define ROUND_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

typedef struct SndHeapBlock {
    NNSFndLink link;
    u32 size;
    NNSSndHeapDisposeCallback callback;
    u32 data1;
    u32 data2;
} SndHeapBlock;

typedef struct SndHeapSection {
    NNSFndList blockList;
    NNSFndLink link;
} SndHeapSection;

static void InitHeapSection(SndHeapSection *section);
static BOOL InitHeap(NNSSndHeap *heap, NNSFndHeapHandle handle);
static BOOL NewSection(NNSSndHeap *heap);
static void WaitSoundDriver(void);

NNSSndHeapHandle NNS_SndHeapCreate(void *startAddress, u32 size) {
    NNSSndHeap *heap;
    NNSFndHeapHandle handle;
    void *endAddress = (u8 *)startAddress + size;

    startAddress = (void *)ROUND_UP((uptr)startAddress, 4);
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
    SndHeapSection *section;
    SndHeapBlock *block;
    BOOL doCallback = FALSE;

    while ((section = NNS_FndGetPrevListObject(&heap->levelList, NULL)) != NULL) {
        block = NULL;
        while ((block = NNS_FndGetPrevListObject(&section->blockList, block)) != NULL) {
            if (block->callback != NULL) {
                block->callback((u8 *)block + BLOCK_HEAD_SIZE, block->size, block->data1, block->data2);
                doCallback = TRUE;
            }
        }
        NNS_FndRemoveListObject(&heap->levelList, section);
    }
    NNS_FndFreeToFrmHeap(heap->handle, NNS_FND_FRMHEAP_RELEASE_BOTH);
    if (doCallback) {
        WaitSoundDriver();
    }
    NewSection(heap);
}

void *NNS_SndHeapAlloc(NNSSndHeapHandle heap, u32 size, NNSSndHeapDisposeCallback callback, u32 data1, u32 data2) {
    SndHeapSection *section;
    SndHeapBlock *block =
        NNS_FndAllocFromFrmHeapEx(heap->handle, ROUND_UP(size, HEAP_ALIGN) + BLOCK_HEAD_SIZE, HEAP_ALIGN);

    if (block == NULL) {
        return NULL;
    }
    section = NNS_FndGetPrevListObject(&heap->levelList, NULL);
    block->size = size;
    block->callback = callback;
    block->data1 = data1;
    block->data2 = data2;
    NNS_FndAppendListObject(&section->blockList, block);
    return (u8 *)block + BLOCK_HEAD_SIZE;
}

int NNS_SndHeapSaveState(NNSSndHeapHandle heap) {
    if (!NNS_FndRecordStateForFrmHeap(heap->handle, heap->levelList.numObjects)) {
        return -1;
    }
    if (!NewSection(heap)) {
        NNS_FndFreeByStateToFrmHeap(heap->handle, 0);
        return -1;
    }
    return heap->levelList.numObjects - 1;
}

void NNS_SndHeapLoadState(NNSSndHeapHandle heap, int level) {
    SndHeapSection *section;
    SndHeapBlock *block = NULL;
    BOOL doCallback = FALSE;

    if (level == 0) {
        NNS_SndHeapClear(heap);
        return;
    }
    while (level < heap->levelList.numObjects) {
        section = NNS_FndGetPrevListObject(&heap->levelList, NULL);
        while ((block = NNS_FndGetPrevListObject(&section->blockList, block)) != NULL) {
            if (block->callback != NULL) {
                block->callback((u8 *)block + BLOCK_HEAD_SIZE, block->size, block->data1, block->data2);
                doCallback = TRUE;
            }
        }
        NNS_FndRemoveListObject(&heap->levelList, section);
    }
    NNS_FndFreeByStateToFrmHeap(heap->handle, (u32)level);
    if (doCallback) {
        WaitSoundDriver();
    }
    NNS_FndRecordStateForFrmHeap(heap->handle, heap->levelList.numObjects);
    NewSection(heap);
}

int NNS_SndHeapGetCurrentLevel(NNSSndHeapHandle heap) {
    return heap->levelList.numObjects - 1;
}

static void InitHeapSection(SndHeapSection *section) {
    NNS_FndInitList(&section->blockList, offsetof(SndHeapBlock, link));
}

static BOOL InitHeap(NNSSndHeap *heap, NNSFndHeapHandle handle) {
    NNS_FndInitList(&heap->levelList, offsetof(SndHeapSection, link));
    heap->handle = handle;
    if (!NewSection(heap)) {
        return FALSE;
    }
    return TRUE;
}

static BOOL NewSection(NNSSndHeap *heap) {
    SndHeapSection *section = NNS_FndAllocFromFrmHeapEx(heap->handle, sizeof(SndHeapSection), 4);

    if (section == NULL) {
        return FALSE;
    }
    InitHeapSection(section);
    NNS_FndAppendListObject(&heap->levelList, section);
    return TRUE;
}

// Waits for the sound driver to have run every command sent, so that nothing plays from the memory freed
static void WaitSoundDriver(void) {
    u32 tag = SND_GetCurrentCommandTag();

    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
}
