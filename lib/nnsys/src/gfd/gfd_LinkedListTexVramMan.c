#include "gfdi_LinkedListVramMan_Common.h"
#include "nnsys/gfd.h"

// NitroSystem's gfd_LinkedListTexVramMan.c: the linked-list texture VRAM manager, which frees what it allocated. It
// keeps normal and 4x4-compressed textures in two free lists, with the 4x4 ones at the bottom of slots 0 and 2 and
// their palette indices at the bottom of slot 1

typedef struct {
    GfdVramFreeList normal;
    GfdVramFreeList comp4x4;
    GfdVramBlock *pool;
    u32 size;
    u32 size4x4;
    GfdVramBlock *work;
    u32 workSize;
} GfdLnkTexVramManager;

// How much of a slot is still unassigned, and how much has gone to normal and to 4x4-compressed textures
typedef struct {
    u32 unassigned;
    u32 normal;
    u32 comp4x4;
} GfdTexSlotSplit;

#define GFD_TEX_SLOT_COUNT 4
#define GFD_TEX_SLOT_SIZE 0x20000

static GfdLnkTexVramManager texMgr_;

u32 NNS_GfdGetLnkTexVramManagerWorkSize(u32 numMemBlk) {
    return numMemBlk * sizeof(GfdVramBlock);
}

void NNS_GfdInitLnkTexVramManager(u32 szByte, u32 szByteFor4x4, void *pManagementWork, u32 szByteManagementWork,
                                  BOOL useAsDefault) {
    texMgr_.size = szByte;
    texMgr_.size4x4 = szByteFor4x4;
    texMgr_.work = pManagementWork;
    texMgr_.workSize = szByteManagementWork;
    NNS_GfdResetLnkTexVramState();

    if (useAsDefault) {
        NNS_GfdDefaultFuncAllocTexVram = NNS_GfdAllocLnkTexVram;
        NNS_GfdDefaultFuncFreeTexVram = NNS_GfdFreeLnkTexVram;
    }
}

NNSGfdTexKey NNS_GfdAllocLnkTexVram(u32 szByte, BOOL is4x4comp, u32 opt) {
    u32 addr;
    BOOL ok;

    szByte = GfdTexAllocSize(szByte);
    if (szByte >= GFD_TEX_ALLOC_LIMIT) {
        return NNS_GFD_ALLOC_ERROR_TEXKEY;
    }

    if (is4x4comp) {
        ok = NNSi_GfdAllocLnkVram(&texMgr_.comp4x4, &texMgr_.pool, &addr, szByte);
    } else {
        ok = NNSi_GfdAllocLnkVram(&texMgr_.normal, &texMgr_.pool, &addr, szByte);
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
    const BOOL is4x4 = GfdTexKeyIs4x4(key);

    if (size != 0) {
        BOOL ok;

        if (is4x4) {
            ok = NNSi_GfdFreeLnkVram(&texMgr_.comp4x4, &texMgr_.pool, addr, size);
        } else {
            ok = NNSi_GfdFreeLnkVram(&texMgr_.normal, &texMgr_.pool, addr, size);
        }

        if (ok) {
            return 0;
        }
        return 1;
    }
    return 2;
}

static inline void AddRange(GfdVramFreeList *list, u32 start, u32 size) {
    if (size > 0) {
        NNSi_GfdAddNewFreeBlock(list, &texMgr_.pool, start, size);
    }
}

void NNS_GfdResetLnkTexVramState(void) {
    GfdTexSlotSplit slots[GFD_TEX_SLOT_COUNT] = {
        { GFD_TEX_SLOT_SIZE, 0, 0 },
        { GFD_TEX_SLOT_SIZE, 0, 0 },
        { GFD_TEX_SLOT_SIZE, 0, 0 },
        { GFD_TEX_SLOT_SIZE, 0, 0 },
    };
    u32 normalLeft;
    u32 left4x4;
    u32 indexSize;
    u32 i;
    GfdTexSlotSplit *slot;
    u32 j;

    left4x4 = texMgr_.size4x4;
    // The palette indices of 4x4-compressed textures take half their size
    indexSize = left4x4 / 2;
    normalLeft = texMgr_.size - (left4x4 + indexSize);

    // 4x4-compressed textures go in slots 0 and 2
    for (i = 0; i < GFD_TEX_SLOT_COUNT; i++) {
        if ((i == 0 || i == 2) && slots[i].unassigned > 0 && left4x4 > 0) {
            u32 part = slots[i].unassigned > left4x4 ? left4x4 : slots[i].unassigned;

            slots[i].comp4x4 += part;
            slots[i].unassigned -= part;
            left4x4 -= part;
        }
    }

    // Their palette indices go at the bottom of slot 1, and normal textures fill what is left
    slots[1].unassigned -= indexSize;
    for (j = 0, slot = slots; j < GFD_TEX_SLOT_COUNT; j++, slot++) {
        if (slot->unassigned > 0 && normalLeft > 0) {
            u32 part = slot->unassigned > normalLeft ? normalLeft : slot->unassigned;

            slot->normal += part;
            slot->unassigned -= part;
            normalLeft -= part;
        }
    }

    NNSi_GfdInitLnkVramMan(&texMgr_.normal);
    NNSi_GfdInitLnkVramMan(&texMgr_.comp4x4);
    texMgr_.pool = NNSi_GfdInitLnkVramBlockPool(texMgr_.work, texMgr_.workSize / sizeof(GfdVramBlock));

    AddRange(&texMgr_.comp4x4, 0, slots[0].comp4x4);
    AddRange(&texMgr_.normal, slots[0].comp4x4, slots[0].normal);
    AddRange(&texMgr_.comp4x4, GFD_TEX_SLOT_SIZE * 2, slots[2].comp4x4);
    AddRange(&texMgr_.normal, GFD_TEX_SLOT_SIZE * 2 + slots[2].comp4x4, slots[2].normal);
    AddRange(&texMgr_.normal, GFD_TEX_SLOT_SIZE * 3, slots[3].normal);
    AddRange(&texMgr_.normal, GFD_TEX_SLOT_SIZE + indexSize, slots[1].normal);

    NNSi_GfdMergeAllFreeBlocks(&texMgr_.normal, &texMgr_.pool);
    NNSi_GfdMergeAllFreeBlocks(&texMgr_.comp4x4, &texMgr_.pool);
}
