#include "gfdi_LinkedListVramMan_Common.h"
#include "nnsys/gfd.h"

// NitroSystem's gfd_LinkedListTexVramMan.c: the linked-list texture VRAM manager, which frees what it allocated. It
// keeps normal and 4x4-compressed textures in two free lists, with the 4x4 ones at the bottom of slots 0 and 2 and
// their palette indices at the bottom of slot 1

typedef struct {
    NNSiGfdLnkVramMan mgrNrm;
    NNSiGfdLnkVramMan mgr4x4;
    NNSiGfdLnkVramBlock *pBlockPoolList;
    u32 szByte;
    u32 szByteFor4x4;
    NNSiGfdLnkVramBlock *pWorkHead;
    u32 szByteManagementWork;
} NNS_GfdLnkTexVramManager;

// How much of a slot is still unassigned, and how much has gone to normal and to 4x4-compressed textures
typedef struct {
    u32 szFree;
    u32 szNrm;
    u32 sz4x4;
} SlotData;

#define GFD_TEX_SLOT_COUNT 4
#define GFD_SLOT_SIZE 0x20000

static NNS_GfdLnkTexVramManager mgr_;

u32 NNS_GfdGetLnkTexVramManagerWorkSize(u32 numMemBlk) {
    return numMemBlk * sizeof(NNSiGfdLnkVramBlock);
}

void NNS_GfdInitLnkTexVramManager(u32 szByte, u32 szByteFor4x4, void *pManagementWork, u32 szByteManagementWork,
                                  BOOL useAsDefault) {
    mgr_.szByte = szByte;
    mgr_.szByteFor4x4 = szByteFor4x4;
    mgr_.pWorkHead = pManagementWork;
    mgr_.szByteManagementWork = szByteManagementWork;
    NNS_GfdResetLnkTexVramState();

    if (useAsDefault) {
        NNS_GfdDefaultFuncAllocTexVram = NNS_GfdAllocLnkTexVram;
        NNS_GfdDefaultFuncFreeTexVram = NNS_GfdFreeLnkTexVram;
    }
}

NNSGfdTexKey NNS_GfdAllocLnkTexVram(u32 szByte, BOOL is4x4comp, u32 opt) {
    u32 addr;
    BOOL ok;

    szByte = NNSi_GfdGetTexKeyRoundupSize(szByte);
    if (szByte >= NNS_GFD_TEXSIZE_MAX) {
        return NNS_GFD_ALLOC_ERROR_TEXKEY;
    }

    if (is4x4comp) {
        ok = NNSi_GfdAllocLnkVram(&mgr_.mgr4x4, &mgr_.pBlockPoolList, &addr, szByte);
    } else {
        ok = NNSi_GfdAllocLnkVram(&mgr_.mgrNrm, &mgr_.pBlockPoolList, &addr, szByte);
    }

    if (ok) {
        return NNS_GfdMakeTexKey(addr, szByte, is4x4comp);
    }
    return NNS_GFD_ALLOC_ERROR_TEXKEY;
}

// Returns 0 when freed, 1 when the manager had no block left to free it with, and 2 for an empty key
int NNS_GfdFreeLnkTexVram(NNSGfdTexKey key) {
    const u32 addr = NNS_GfdGetTexKeyAddr(key);
    const u32 size = NNS_GfdGetTexKeySize(key);
    const BOOL is4x4 = NNS_GfdGetTexKey4x4Flag(key);

    if (size != 0) {
        BOOL ok;

        if (is4x4) {
            ok = NNSi_GfdFreeLnkVram(&mgr_.mgr4x4, &mgr_.pBlockPoolList, addr, size);
        } else {
            ok = NNSi_GfdFreeLnkVram(&mgr_.mgrNrm, &mgr_.pBlockPoolList, addr, size);
        }

        if (ok) {
            return 0;
        }
        return 1;
    }
    return 2;
}

static inline void InitSlotFreeBlock_(NNSiGfdLnkVramMan *list, u32 start, u32 size) {
    if (size > 0) {
        NNSi_GfdAddNewFreeBlock(list, &mgr_.pBlockPoolList, start, size);
    }
}

void NNS_GfdResetLnkTexVramState(void) {
    SlotData slots[GFD_TEX_SLOT_COUNT] = {
        { GFD_SLOT_SIZE, 0, 0 },
        { GFD_SLOT_SIZE, 0, 0 },
        { GFD_SLOT_SIZE, 0, 0 },
        { GFD_SLOT_SIZE, 0, 0 },
    };
    u32 normalLeft;
    u32 left4x4;
    u32 indexSize;
    u32 i;
    SlotData *slot;
    u32 j;

    left4x4 = mgr_.szByteFor4x4;
    // The palette indices of 4x4-compressed textures take half their size
    indexSize = left4x4 / 2;
    normalLeft = mgr_.szByte - (left4x4 + indexSize);

    // 4x4-compressed textures go in slots 0 and 2
    for (i = 0; i < GFD_TEX_SLOT_COUNT; i++) {
        if ((i == 0 || i == 2) && slots[i].szFree > 0 && left4x4 > 0) {
            u32 part = slots[i].szFree > left4x4 ? left4x4 : slots[i].szFree;

            slots[i].sz4x4 += part;
            slots[i].szFree -= part;
            left4x4 -= part;
        }
    }

    // Their palette indices go at the bottom of slot 1, and normal textures fill what is left
    slots[1].szFree -= indexSize;
    for (j = 0, slot = slots; j < GFD_TEX_SLOT_COUNT; j++, slot++) {
        if (slot->szFree > 0 && normalLeft > 0) {
            u32 part = slot->szFree > normalLeft ? normalLeft : slot->szFree;

            slot->szNrm += part;
            slot->szFree -= part;
            normalLeft -= part;
        }
    }

    NNSi_GfdInitLnkVramMan(&mgr_.mgrNrm);
    NNSi_GfdInitLnkVramMan(&mgr_.mgr4x4);
    mgr_.pBlockPoolList =
        NNSi_GfdInitLnkVramBlockPool(mgr_.pWorkHead, mgr_.szByteManagementWork / sizeof(NNSiGfdLnkVramBlock));

    InitSlotFreeBlock_(&mgr_.mgr4x4, 0, slots[0].sz4x4);
    InitSlotFreeBlock_(&mgr_.mgrNrm, slots[0].sz4x4, slots[0].szNrm);
    InitSlotFreeBlock_(&mgr_.mgr4x4, GFD_SLOT_SIZE * 2, slots[2].sz4x4);
    InitSlotFreeBlock_(&mgr_.mgrNrm, GFD_SLOT_SIZE * 2 + slots[2].sz4x4, slots[2].szNrm);
    InitSlotFreeBlock_(&mgr_.mgrNrm, GFD_SLOT_SIZE * 3, slots[3].szNrm);
    InitSlotFreeBlock_(&mgr_.mgrNrm, GFD_SLOT_SIZE + indexSize, slots[1].szNrm);

    NNSi_GfdMergeAllFreeBlocks(&mgr_.mgrNrm, &mgr_.pBlockPoolList);
    NNSi_GfdMergeAllFreeBlocks(&mgr_.mgr4x4, &mgr_.pBlockPoolList);
}
