#include "types.h"
#include "gfl/sound.h"
#include "nnsys/snd.h"

// The BGM stack: each level holds a BGM's handle and sequence, and the sound heap's levels before and after its files
// were loaded, so that pushing a BGM frees the waves of the one below and popping it loads them again. The file's
// name is descriptive, a guess: the ROM has no string for it. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except BGMStack_ResetEntry, BGMStack_InitEntry and
// BGMStack_FreeEntry. The data's names, types, constants and fields are ours

#define BGM_STACK_SIZE 6
#define BGM_STACK_NO_SEQ 0xffff

typedef struct {
    BOOL active;
    u16 seq;
    NNSSndHandle handle;
    // The sound heap's level when the entry was made, before its BGM, after its sequence and bank, and after its
    // waves
    s32 baseLevel;
    s32 bgmLevel;
    s32 seqLevel;
    s32 waveLevel;
} BGMStackEntry;

typedef struct {
    u16 player;
    s32 depth;
    NNSSndHeapHandle *heap;
} BGMStack;

// What GFL_SndBGMStack_Preload preloads: the count of sequences, and the level to go back to
typedef struct {
    u32 count;
    s32 level;
} BGMStackPreload;

static void BGMStack_ResetEntry(s32 depth);
static void BGMStack_InitEntry(s32 level);
static void BGMStack_FreeEntry(void);

static BGMStack sBGMStack;
static BGMStackEntry sBGMStackEntries[BGM_STACK_SIZE];

void GFL_SndBGMStack_SetHeap(NNSSndHeapHandle *heap) {
    sBGMStack.heap = heap;
}

static void BGMStack_ResetEntry(s32 depth) {
    sBGMStackEntries[depth].active = FALSE;
    sBGMStackEntries[depth].seq = BGM_STACK_NO_SEQ;
    NNS_SndHandleReleaseSeq(&sBGMStackEntries[depth].handle);
    sBGMStackEntries[depth].baseLevel = -1;
    sBGMStackEntries[depth].bgmLevel = -1;
    sBGMStackEntries[depth].seqLevel = -1;
    sBGMStackEntries[depth].waveLevel = -1;
}

static void BGMStack_InitEntry(s32 level) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    entry->baseLevel = level;
    entry->bgmLevel = level;
    entry->seqLevel = level;
    entry->waveLevel = level;
    sBGMStackEntries[sBGMStack.depth].active = TRUE;
}

static void BGMStack_FreeEntry(void) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    NNS_SndHandleReleaseSeq(&entry->handle);
    NNS_SndHeapLoadState(*sBGMStack.heap, entry->baseLevel);
    BGMStack_ResetEntry(sBGMStack.depth);
}

void GFL_SndBGMStack_Init(u16 player) {
    s32 i;

    sBGMStack.player = player;
    sBGMStack.depth = 0;
    for (i = 0; i < BGM_STACK_SIZE; i++) {
        BGMStack_ResetEntry(i);
        NNS_SndHandleInit(&sBGMStackEntries[i].handle);
    }
    NNS_SndPlayerSetPlayableSeqCount(sBGMStack.player, BGM_STACK_SIZE);
    BGMStack_InitEntry(NNS_SndHeapSaveState(*sBGMStack.heap));
}

u32 GFL_SndGetLastResID(void) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    if (sBGMStackEntries[sBGMStack.depth].active == FALSE) {
        return 0;
    }
    if (entry->seq == BGM_STACK_NO_SEQ) {
        return 0;
    }
    return entry->seq;
}

s32 GFL_SndBGMStack_GetDepth(void) {
    return sBGMStack.depth;
}

NNSSndHandle *GFL_SndBGMStack_GetHandle(void) {
    return &sBGMStackEntries[sBGMStack.depth].handle;
}

BOOL GFL_SndBGMStack_LoadSeq(u32 seq) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];
    BOOL result;

    if (NNS_SndArcLoadSeqEx(seq, NNS_SND_ARC_LOAD_SEQ | NNS_SND_ARC_LOAD_BANK, *sBGMStack.heap) == FALSE) {
        return FALSE;
    }
    entry->seqLevel = NNS_SndHeapSaveState(*sBGMStack.heap);
    result = NNS_SndArcLoadSeqEx(seq, NNS_SND_ARC_LOAD_WAVE, *sBGMStack.heap);
    if (result == FALSE) {
        return FALSE;
    }
    entry->waveLevel = NNS_SndHeapSaveState(*sBGMStack.heap);
    entry->seq = seq;
    return result;
}

void GFL_SndBGMStack_SaveSeqLevel(void) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    entry->seqLevel = NNS_SndHeapSaveState(*sBGMStack.heap);
}

BOOL GFL_SndBGMStack_SetLoadedSeq(u32 seq) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    entry->waveLevel = NNS_SndHeapSaveState(*sBGMStack.heap);
    entry->seq = seq;
    return TRUE;
}

void GFL_SndBGMStack_FreeSeq(void) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];

    if (sBGMStackEntries[sBGMStack.depth].active) {
        entry->seq = BGM_STACK_NO_SEQ;
        NNS_SndHeapLoadState(*sBGMStack.heap, entry->bgmLevel);
        entry->seqLevel = entry->bgmLevel;
        entry->waveLevel = entry->bgmLevel;
    }
}

BOOL GFL_SndBGMStack_Push(void) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];
    s32 level;

    if (sBGMStack.depth >= BGM_STACK_SIZE - 1) {
        return FALSE;
    }
    if (entry->seqLevel != -1) {
        NNS_SndHeapLoadState(*sBGMStack.heap, entry->seqLevel);
        entry->waveLevel = -1;
        level = entry->seqLevel;
    } else {
        level = entry->bgmLevel;
    }
    sBGMStack.depth++;
    BGMStack_InitEntry(level);
    return TRUE;
}

BOOL GFL_SndBGMStack_Pop(void) {
    BGMStackEntry *entry;

    if (sBGMStack.depth <= 0) {
        return FALSE;
    }
    BGMStack_FreeEntry();
    sBGMStack.depth--;
    entry = &sBGMStackEntries[sBGMStack.depth];
    if (entry->seq != BGM_STACK_NO_SEQ) {
        NNS_SndArcLoadSeqEx(entry->seq, NNS_SND_ARC_LOAD_BANK | NNS_SND_ARC_LOAD_WAVE, *sBGMStack.heap);
        entry->waveLevel = NNS_SndHeapSaveState(*sBGMStack.heap);
    }
    return TRUE;
}

u32 GFL_SndBGMStack_Preload(const u32 *seqs, u32 count) {
    BGMStackEntry *entry = &sBGMStackEntries[sBGMStack.depth];
    BGMStackPreload *preload;
    s32 level = entry->baseLevel;
    u32 i;

    BGMStack_FreeEntry();
    preload = NNS_SndHeapAlloc(*sBGMStack.heap, sizeof(BGMStackPreload), NULL, 0, 0);
    for (i = 0; i < count; i++) {
        NNS_SndArcLoadSeq(seqs[i], *sBGMStack.heap);
    }
    preload->count = count;
    preload->level = level;
    BGMStack_InitEntry(NNS_SndHeapSaveState(*sBGMStack.heap));
    return (u32)preload;
}

void GFL_SndBGMStack_Unload(u32 handle) {
    s32 level = ((BGMStackPreload *)handle)->level;

    BGMStack_FreeEntry();
    NNS_SndHeapLoadState(*sBGMStack.heap, level);
    BGMStack_InitEntry(level);
}
