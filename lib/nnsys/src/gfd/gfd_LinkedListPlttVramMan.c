#include "gfdi_LinkedListVramMan_Common.h"
#include "nnsys/gfd.h"

// NitroSystem's gfd_LinkedListPlttVramMan.c: the linked-list palette VRAM manager, which frees what it allocated

typedef struct {
    NNSiGfdLnkVramMan mgr;
    NNSiGfdLnkVramBlock *pBlockPoolList;
    u32 szByte;
    NNSiGfdLnkVramBlock *pWorkHead;
    u32 szByteManagementWork;
} NNS_GfdLnkPlttVramManager;

static NNS_GfdLnkPlttVramManager mgr_;

u32 NNS_GfdGetLnkPlttVramManagerWorkSize(u32 numMemBlk) {
    return numMemBlk * sizeof(NNSiGfdLnkVramBlock);
}

void NNS_GfdInitLnkPlttVramManager(u32 szByte, void *pManagementWork, u32 szByteManagementWork, BOOL useAsDefault) {
    mgr_.szByte = szByte;
    mgr_.pWorkHead = pManagementWork;
    mgr_.szByteManagementWork = szByteManagementWork;
    NNS_GfdResetLnkPlttVramState();

    if (useAsDefault) {
        NNS_GfdDefaultFuncAllocPlttVram = NNS_GfdAllocLnkPlttVram;
        NNS_GfdDefaultFuncFreePlttVram = NNS_GfdFreeLnkPlttVram;
    }
}

NNSGfdPlttKey NNS_GfdAllocLnkPlttVram(u32 szByte, BOOL is4pltt, u32 opt) {
    u32 addr;
    BOOL ok;

    szByte = NNSi_GfdGetPlttKeyRoundupSize(szByte);
    if (szByte >= NNS_GFD_PLTTSIZE_MAX) {
        return NNS_GFD_ALLOC_ERROR_PLTTKEY;
    }

    if (is4pltt) {
        ok = NNSi_GfdAllocLnkVramAligned(&mgr_.mgr, &mgr_.pBlockPoolList, &addr, szByte, 8);
        if (addr + szByte > NNS_GFD_4PLTT_MAX_ADDR) {
            NNSi_GfdFreeLnkVram(&mgr_.mgr, &mgr_.pBlockPoolList, addr, szByte);
            return NNS_GFD_ALLOC_ERROR_PLTTKEY;
        }
    } else {
        ok = NNSi_GfdAllocLnkVramAligned(&mgr_.mgr, &mgr_.pBlockPoolList, &addr, szByte, 16);
    }

    if (ok) {
        return NNS_GfdMakePlttKey(addr, szByte);
    }
    return NNS_GFD_ALLOC_ERROR_PLTTKEY;
}

// Returns 0 when freed and 1 when the manager had no block left to free it with
int NNS_GfdFreeLnkPlttVram(NNSGfdPlttKey key) {
    const u32 addr = NNS_GfdGetPlttKeyAddr(key);
    const u32 size = NNS_GfdGetPlttKeySize(key);
    const BOOL ok = NNSi_GfdFreeLnkVram(&mgr_.mgr, &mgr_.pBlockPoolList, addr, size);

    if (ok) {
        return 0;
    }
    return 1;
}

void NNS_GfdResetLnkPlttVramState(void) {
    mgr_.pBlockPoolList =
        NNSi_GfdInitLnkVramBlockPool(mgr_.pWorkHead, mgr_.szByteManagementWork / sizeof(NNSiGfdLnkVramBlock));
    NNSi_GfdInitLnkVramMan(&mgr_.mgr);
    NNSi_GfdAddNewFreeBlock(&mgr_.mgr, &mgr_.pBlockPoolList, 0, mgr_.szByte);
    NNSi_GfdMergeAllFreeBlocks(&mgr_.mgr, &mgr_.pBlockPoolList);
}
