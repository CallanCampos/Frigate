#include "nnsys/gfd.h"

// NitroSystem's gfd_FramePlttVramMan.c: the frame palette VRAM manager, which allocates from both ends of palette VRAM
// and frees only by going back to a state saved before. The game doesn't use its init, which the link left out, so
// nothing sets size here

// What is allocated: up to lo from the bottom, and down to hi from the top
typedef struct {
    u32 lo;
    u32 hi;
    u32 size;
} GfdFrmPlttManager;

static GfdFrmPlttManager s_managerState_;

static inline BOOL AllocLow(u32 size, BOOL is4pltt, u32 *outAddr) {
    const u32 lo = s_managerState_.lo;
    u32 pad;
    u32 total;

    // What aligning the low end skips
    if (is4pltt) {
        pad = (8 - (lo & 7)) & 7;
    } else {
        pad = (16 - (lo & 15)) & 15;
    }
    total = size + pad;

    if (s_managerState_.hi - lo >= total) {
        const u32 newLo = lo + total;

        if (is4pltt && newLo > GFD_PLTT4_LIMIT) {
            return FALSE;
        }
        *outAddr = lo + pad;
        s_managerState_.lo += total;
        return TRUE;
    }
    return FALSE;
}

static inline BOOL AllocHigh(u32 size, BOOL is4pltt, u32 *outAddr) {
    const u32 hi = s_managerState_.hi;

    if (hi >= size) {
        const u32 newHi = hi - size;
        u32 total;

        // Adds what aligning the new high end skips
        if (is4pltt) {
            total = size + (newHi & 7);
        } else {
            total = size + (newHi & 15);
        }

        if (hi - s_managerState_.lo >= total) {
            if (is4pltt && hi > GFD_PLTT4_LIMIT) {
                return FALSE;
            }
            s_managerState_.hi -= total;
            *outAddr = s_managerState_.hi;
            return TRUE;
        }
    }
    return FALSE;
}

NNSGfdPlttKey NNS_GfdAllocFrmPlttVram(u32 szByte, BOOL is4pltt, u32 bAllocFromLo) {
    u32 addr = 0;
    BOOL ok;

    szByte = GfdPlttAllocSize(szByte);
    if (szByte >= GFD_PLTT_ALLOC_LIMIT) {
        return NNS_GFD_ALLOC_ERROR_PLTTKEY;
    }

    if (bAllocFromLo == TRUE) {
        ok = AllocLow(szByte, is4pltt, &addr);
    } else {
        ok = AllocHigh(szByte, is4pltt, &addr);
    }

    if (ok) {
        return GfdMakePlttKey(addr, szByte);
    }
    return NNS_GFD_ALLOC_ERROR_PLTTKEY;
}

void NNS_GfdGetFrmPlttVramState(NNSGfdFrmPlttVramState *state) {
    state->address[0] = s_managerState_.lo;
    state->address[1] = s_managerState_.hi;
}

void NNS_GfdSetFrmPlttVramState(const NNSGfdFrmPlttVramState *state) {
    s_managerState_.lo = state->address[0];
    s_managerState_.hi = state->address[1];
}
