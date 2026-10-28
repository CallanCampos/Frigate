// NitroSystem's actual sound-heap Clear/LoadState functions (lib/nnsys/src/snd/snd_heap.c) and linked lists,
// with a small frame-heap fixture and a queued ARM7 disposal command. The command flush models audio catching up
// before invalidation executes: it must read the still-owned wave, then complete the command before memory reuse.
// Callback-free heaps must not introduce a sound wait. No audio samples, DSP timing or hardware FIFO are replaced.
#include "nitro/snd.h"
#include "nnsys/fnd.h"
#include "nnsys/snd.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static _Alignas(32) u8 sArea[4096];
static NNSiFndHeapHead sFrameHeap;
static u8 *sFrameStart;
static size_t sHead;
static size_t sSavedHead;
static u8 *sWave;
static BOOL sWaveOwned;
static BOOL sPending;
static int sWaits;
static int sFrees;
static int sFailures;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "sound heap disposal: %s:%d: %s\n", __FILE__, __LINE__, #condition);                       \
            sFailures++;                                                                                               \
        }                                                                                                              \
    } while (0)

NNSFndHeapHandle NNS_FndCreateFrmHeapEx(void *startAddress, u32 size, u16 optFlag) {
    (void)size;
    (void)optFlag;
    sFrameStart = startAddress;
    sHead = 0;
    return &sFrameHeap;
}

void NNS_FndDestroyFrmHeap(NNSFndHeapHandle heap) {
    CHECK(heap == &sFrameHeap);
}

void *NNS_FndAllocFromFrmHeapEx(NNSFndHeapHandle heap, u32 size, int alignment) {
    uintptr_t address = (uintptr_t)(sFrameStart + sHead);
    uintptr_t aligned = (address + (u32)alignment - 1) & ~((uintptr_t)(u32)alignment - 1);

    CHECK(heap == &sFrameHeap);
    sHead = aligned - (uintptr_t)sFrameStart + size;
    CHECK(sFrameStart + sHead <= sArea + sizeof(sArea));
    return (void *)aligned;
}

static void ReleaseWave(void) {
    // These are the fixture's ownership transitions, independent of the SDK's disposal ordering.
    CHECK(!sPending);
    sWaveOwned = FALSE;
    sFrees++;
}

void NNS_FndFreeToFrmHeap(NNSFndHeapHandle heap, int mode) {
    CHECK(heap == &sFrameHeap);
    CHECK(mode == NNS_FND_FRMHEAP_FREE_ALL);
    ReleaseWave();
    sHead = 0;
}

BOOL NNS_FndRecordStateForFrmHeap(NNSFndHeapHandle heap, u32 tag) {
    CHECK(heap == &sFrameHeap);
    CHECK(tag == 1);
    sSavedHead = sHead;
    return TRUE;
}

BOOL NNS_FndFreeByStateToFrmHeap(NNSFndHeapHandle heap, u32 tag) {
    CHECK(heap == &sFrameHeap);
    CHECK(tag == 1);
    ReleaseWave();
    sHead = sSavedHead;
    return TRUE;
}

static void DisposeWave(void *mem, u32 size, uptr data1, uptr data2) {
    CHECK(mem == sWave);
    CHECK(size == 64);
    CHECK(data1 == 0x1234 && data2 == 0x5678);
    CHECK(sWaveOwned);
    sPending = TRUE;
}

u32 SND_GetCurrentCommandTag(void) {
    CHECK(sPending);
    return 7;
}

BOOL SND_FlushCommand(u32 flags) {
    CHECK(flags == SND_COMMAND_BLOCK);
    CHECK(sPending);
    // SoundService advances elapsed samples before it executes queued invalidations. Its source is still live.
    CHECK(sWaveOwned);
    CHECK(sWave[0] == 0x5a && sWave[63] == 0x5a);
    return TRUE;
}

void SND_WaitForCommandProc(u32 tag) {
    CHECK(tag == 7);
    CHECK(sPending && sWaveOwned);
    sPending = FALSE;
    sWaits++;
}

int main(int argc, char **argv) {
    NNSSndHeapHandle heap;
    BOOL load;
    BOOL callback;

    if (argc != 2) {
        return 2;
    }
    load = strncmp(argv[1], "load-", 5) == 0;
    callback = strcmp(argv[1], "clear-callback") == 0 || strcmp(argv[1], "load-callback") == 0;
    heap = NNS_SndHeapCreate(sArea, sizeof(sArea));
    CHECK(heap != NULL);
    if (load) {
        CHECK(NNS_SndHeapSaveState(heap) == 1);
    }
    sWave = NNS_SndHeapAlloc(heap, 64, callback ? DisposeWave : NULL, 0x1234, 0x5678);
    CHECK(sWave != NULL);
    memset(sWave, 0x5a, 64);
    sWaveOwned = TRUE;
    if (load) {
        NNS_SndHeapLoadState(heap, 1);
    } else {
        NNS_SndHeapClear(heap);
    }
    CHECK(sFrees == 1);
    CHECK(!sWaveOwned && !sPending);
    CHECK(sWaits == (callback ? 1 : 0));
    CHECK(NNS_SndHeapGetCurrentLevel(heap) == (load ? 1 : 0));
    return sFailures != 0;
}
