#include "nnsys/gfd.h"

// NitroSystem's gfd_FramePlttVramMan.c: the frame palette VRAM manager, which allocates from both ends of palette VRAM
// and frees only by going back to a state saved before. The game doesn't use its init, which the link left out, so
// nothing sets size here

// What is allocated: up to loAddr from the bottom, and down to hiAddr from the top
typedef struct {
    u32 loAddr;
    u32 hiAddr;
    u32 szTotal;
} NNSGfdFrmPlttVramManager;

static NNSGfdFrmPlttVramManager s_managerState_;

static inline BOOL AllocFromLo_(u32 size, BOOL is4pltt, u32 *outAddr) {
    const u32 lo = s_managerState_.loAddr;
    u32 pad;
    u32 total;

    // What aligning the low end skips
    if (is4pltt) {
        pad = (8 - (lo & 7)) & 7;
    } else {
        pad = (16 - (lo & 15)) & 15;
    }
    total = size + pad;

    if (s_managerState_.hiAddr - lo >= total) {
        const u32 newLo = lo + total;

        if (is4pltt && newLo > NNS_GFD_4PLTT_MAX_ADDR) {
            return FALSE;
        }
        *outAddr = lo + pad;
        s_managerState_.loAddr += total;
        return TRUE;
    }
    return FALSE;
}

static inline BOOL AllocFromHi_(u32 size, BOOL is4pltt, u32 *outAddr) {
    const u32 hi = s_managerState_.hiAddr;

    if (hi >= size) {
        const u32 newHi = hi - size;
        u32 total;

        // Adds what aligning the new high end skips
        if (is4pltt) {
            total = size + (newHi & 7);
        } else {
            total = size + (newHi & 15);
        }

        if (hi - s_managerState_.loAddr >= total) {
            if (is4pltt && hi > NNS_GFD_4PLTT_MAX_ADDR) {
                return FALSE;
            }
            s_managerState_.hiAddr -= total;
            *outAddr = s_managerState_.hiAddr;
            return TRUE;
        }
    }
    return FALSE;
}

NNSGfdPlttKey NNS_GfdAllocFrmPlttVram(u32 szByte, BOOL is4pltt, u32 bAllocFromLo) {
    u32 addr = 0;
    BOOL ok;

    szByte = NNSi_GfdGetPlttKeyRoundupSize(szByte);
    if (szByte >= NNS_GFD_PLTTSIZE_MAX) {
        return NNS_GFD_ALLOC_ERROR_PLTTKEY;
    }

    if (bAllocFromLo == TRUE) {
        ok = AllocFromLo_(szByte, is4pltt, &addr);
    } else {
        ok = AllocFromHi_(szByte, is4pltt, &addr);
    }

    if (ok) {
        return NNS_GfdMakePlttKey(addr, szByte);
    }
    return NNS_GFD_ALLOC_ERROR_PLTTKEY;
}

void NNS_GfdGetFrmPlttVramState(NNSGfdFrmPlttVramState *state) {
    state->address[0] = s_managerState_.loAddr;
    state->address[1] = s_managerState_.hiAddr;
}

void NNS_GfdSetFrmPlttVramState(const NNSGfdFrmPlttVramState *state) {
    s_managerState_.loAddr = state->address[0];
    s_managerState_.hiAddr = state->address[1];
}
