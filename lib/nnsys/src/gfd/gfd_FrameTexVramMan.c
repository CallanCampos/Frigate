#include "nnsys/gfd.h"

// NitroSystem's gfd_FrameTexVramMan.c: the frame texture VRAM manager, which allocates normal textures down from the
// top of each region and 4x4-compressed ones up from the bottom of slots 0 and 2, with their palette indices in the
// halves of slot 1, and frees only by going back to a state saved before

// One region of texture VRAM, a slot or half of slot 1, and how much of it is allocated: up to head from the bottom and
// down to tail from the top, as offsets from the region's VRAM address, baseAddress
typedef struct {
    u32 head;
    u32 tail;
    BOOL bActive;
    BOOL bHalfSize;
    u16 index;
    u16 pad16_;
    u32 baseAddress;
} NNSGfdFrmTexRegionState;

typedef struct {
    u16 numSlot;
    u16 pad;
} NNSGfdFrmTexVramMnager;

#define NNS_GFD_NUM_TEX_VRAM_REGION 5

static NNSGfdFrmTexRegionState vramRegions_[NNS_GFD_NUM_TEX_VRAM_REGION];

// The regions 4x4-compressed textures go in, and the regions normal textures go in in the order they are tried
static NNSGfdFrmTexRegionState *tex4x4SearchArray_[2] = { &vramRegions_[0], &vramRegions_[3] };
static NNSGfdFrmTexRegionState *texNrmSearchArray_[NNS_GFD_NUM_TEX_VRAM_REGION] = {
    &vramRegions_[4], &vramRegions_[3], &vramRegions_[0], &vramRegions_[2], &vramRegions_[1],
};

static NNSGfdFrmTexRegionState vramRegions_[NNS_GFD_NUM_TEX_VRAM_REGION] = {
    { 0xffffffff, 0xffffffff, FALSE, FALSE, 0, 0xffff, 0x00000 },
    { 0xffffffff, 0xffffffff, FALSE, TRUE, 1, 0xffff, 0x20000 },
    { 0xffffffff, 0xffffffff, FALSE, TRUE, 2, 0xffff, 0x30000 },
    { 0xffffffff, 0xffffffff, FALSE, FALSE, 3, 0xffff, 0x40000 },
    { 0xffffffff, 0xffffffff, FALSE, FALSE, 4, 0xffff, 0x60000 },
};

static NNSGfdFrmTexVramMnager frmExVramMan_;

void NNSi_GfdSetTexNrmSearchArray(int first, int second, int third, int fourth, int fifth) {
    texNrmSearchArray_[0] = &vramRegions_[first];
    texNrmSearchArray_[1] = &vramRegions_[second];
    texNrmSearchArray_[2] = &vramRegions_[third];
    texNrmSearchArray_[3] = &vramRegions_[fourth];
    texNrmSearchArray_[4] = &vramRegions_[fifth];
}

void NNS_GfdInitFrmTexVramManager(u16 numSlot, BOOL useAsDefault) {
    // With slot 2 in use, slot 1 is left for the palette indices of 4x4-compressed textures as long as possible
    if (numSlot <= 2) {
        NNSi_GfdSetTexNrmSearchArray(4, 3, 2, 0, 1);
    } else {
        NNSi_GfdSetTexNrmSearchArray(4, 3, 0, 2, 1);
    }

    frmExVramMan_.numSlot = numSlot;
    NNS_GfdResetFrmTexVramState();

    if (useAsDefault) {
        NNS_GfdDefaultFuncAllocTexVram = NNS_GfdAllocFrmTexVram;
        NNS_GfdDefaultFuncFreeTexVram = NNS_GfdFreeFrmTexVram;
    }
}

void NNS_GfdResetFrmTexVramState(void) {
    int i;
    const u16 numSlot = frmExVramMan_.numSlot;
    // Slot 1 is two regions
    int numRegion = numSlot > 1 ? numSlot + 1 : numSlot;

    for (i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++) {
        if (i < numRegion) {
            vramRegions_[i].bActive = TRUE;
        } else {
            vramRegions_[i].bActive = FALSE;
        }

        if (vramRegions_[i].bHalfSize) {
            vramRegions_[i].head = 0;
            vramRegions_[i].tail = 0x10000;
        } else {
            vramRegions_[i].head = 0;
            vramRegions_[i].tail = 0x20000;
        }
    }
}

// The region that holds the palette indices of the 4x4-compressed textures in a region
static inline NNSGfdFrmTexRegionState *Get4x4IdxRegion_(NNSGfdFrmTexRegionState *region) {
    switch (region->index) {
    case 0:
        return &vramRegions_[1];
    case 3:
        return &vramRegions_[2];
    default:
        return NULL;
    }
}

static inline BOOL HasRoom(const NNSGfdFrmTexRegionState *region, u32 size) {
    return region->bActive && region->tail - region->head >= size;
}

static inline BOOL DoAlloc4x4_(u32 size, u32 *outAddr) {
    int i;

    for (i = 0; i < 2; i++) {
        NNSGfdFrmTexRegionState *region = tex4x4SearchArray_[i];

        if (HasRoom(region, size)) {
            NNSGfdFrmTexRegionState *indexRegion = Get4x4IdxRegion_(region);

            if (HasRoom(indexRegion, size / 2)) {
                u32 offset = region->head;

                region->head += size;
                indexRegion->head += size / 2;
                *outAddr = offset + region->baseAddress;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static inline BOOL DoAllocNrm_(u32 size, u32 *outAddr) {
    int i;

    for (i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++) {
        NNSGfdFrmTexRegionState *region = texNrmSearchArray_[i];

        if (HasRoom(region, size)) {
            region->tail -= size;
            *outAddr = region->tail + region->baseAddress;
            return TRUE;
        }
    }
    return FALSE;
}

NNSGfdTexKey NNS_GfdAllocFrmTexVram(u32 szByte, BOOL is4x4comp, u32 opt) {
    u32 addr;
    BOOL ok;

    szByte = NNSi_GfdGetTexKeyRoundupSize(szByte);
    if (szByte >= NNS_GFD_TEXSIZE_MAX) {
        return NNS_GFD_ALLOC_ERROR_TEXKEY;
    }

    if (is4x4comp) {
        ok = DoAlloc4x4_(szByte, &addr);
    } else {
        ok = DoAllocNrm_(szByte, &addr);
    }

    if (ok) {
        return NNS_GfdMakeTexKey(addr, szByte, is4x4comp);
    }
    return NNS_GFD_ALLOC_ERROR_TEXKEY;
}

int NNS_GfdFreeFrmTexVram(NNSGfdTexKey key) {
    return 0;
}

void NNS_GfdGetFrmTexVramState(NNSGfdFrmTexVramState *state) {
    int i;

    for (i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++) {
        state->address[i * 2] = vramRegions_[i].head;
        state->address[i * 2 + 1] = vramRegions_[i].tail;
    }
}

void NNS_GfdSetFrmTexVramState(const NNSGfdFrmTexVramState *state) {
    int i;

    for (i = 0; i < NNS_GFD_NUM_TEX_VRAM_REGION; i++) {
        vramRegions_[i].head = state->address[i * 2];
        vramRegions_[i].tail = state->address[i * 2 + 1];
    }
}
