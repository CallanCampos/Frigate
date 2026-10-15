#include "system/screentex.h"
#include "types.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"

// A texture of the screen, captured to one of VRAM banks A to D. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#define SCREENTEX_WIDTH 256
#define SCREENTEX_HEIGHT 192
#define SCREENTEX_SIZE (SCREENTEX_WIDTH * SCREENTEX_HEIGHT * sizeof(u16))
// The size of a VRAM bank of A to D in the texture image slots
#define TEX_BANK_SIZE 0x20000

// A capture bank: its VRAM bank, the capture's destination and where it is in LCDC mode
typedef struct {
    u32 vramBank;
    u32 captureDest;
    void *lcdcAddr;
} ScreenCaptureBank;

static const ScreenCaptureBank SCREEN_CAPTURE_BANK_INFO[4] = {
    { GX_VRAM_A, GX_CAPTURE_DEST_VRAM_A_0x00000, (void *)HW_LCDC_VRAM_A },
    { GX_VRAM_B, GX_CAPTURE_DEST_VRAM_B_0x00000, (void *)HW_LCDC_VRAM_B },
    { GX_VRAM_C, GX_CAPTURE_DEST_VRAM_C_0x00000, (void *)HW_LCDC_VRAM_C },
    { GX_VRAM_D, GX_CAPTURE_DEST_VRAM_D_0x00000, (void *)HW_LCDC_VRAM_D },
};

// A texture file (BTX0) with one 256x192 direct color texture named "direct", whose image is not in the file
static const u8 CAPTURE_TEX_RESHEADER[0x8c] = {
    0x42, 0x54, 0x58, 0x30, 0xff, 0xfe, 0x01, 0x00, 0x8c, 0x00, 0x02, 0x00, 0x10, 0x00, 0x01, 0x00, 0x14, 0x00,
    0x00, 0x00, 0x54, 0x45, 0x58, 0x30, 0x78, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x3c, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x78, 0x00, 0x02, 0x00, 0x78, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x68, 0x00, 0x00, 0x00, 0x78, 0x00, 0x02, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x08, 0x00, 0x10, 0x00, 0x7f, 0x01,
    0x00, 0x00, 0x2e, 0x00, 0x01, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xd0, 0x1e, 0x00, 0x01, 0x08, 0x80,
    0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00,
};

void *GFL_G3DScreenTexCreate(HeapID heapId, u32 bank) {
    void *resource;
    NNSG3dResFileHeader *file;
    NNSG3dResTex *tex;
    u32 size;
    u32 texBanks;
    u32 offset;

    resource = GFL_HeapAllocate(heapId, GFL_G3DResGetAllocSize(), FALSE, "screentex.c", 59);
    offset = 0;
    file = GFL_HeapAllocate(heapId, sizeof(CAPTURE_TEX_RESHEADER), FALSE, "screentex.c", 60);
    sys_memcpy(CAPTURE_TEX_RESHEADER, file, sizeof(CAPTURE_TEX_RESHEADER));
    GFL_G3DResBindData(resource, 2, file);
    tex = NNS_G3DResGetTexBlock(file);
    size = NNS_G3DResTexGetTexDataSize(tex);
    // The texture image slots are the texture banks in order, so the bank's image starts after the banks before it
    texBanks = gfxGetTextureBanks();
    switch (bank) {
    case SCREENTEX_BANK_D:
        // BUG: `|=` for `&`, which is always true, so every bank before the capture bank is counted, texture bank or
        // not. This is right only while the banks before it all hold textures
#ifdef BUGFIX
        if (texBanks & GX_VRAM_C) {
#else
        if (texBanks |= GX_VRAM_C) {
#endif
            offset += TEX_BANK_SIZE;
        }
    case SCREENTEX_BANK_C:
#ifdef BUGFIX
        if (texBanks & GX_VRAM_B) {
#else
        if (texBanks |= GX_VRAM_B) {
#endif
            offset += TEX_BANK_SIZE;
        }
    case SCREENTEX_BANK_B:
#ifdef BUGFIX
        if (texBanks & GX_VRAM_A) {
#else
        if (texBanks |= GX_VRAM_A) {
#endif
            offset += TEX_BANK_SIZE;
        }
    case SCREENTEX_BANK_A:
        break;
    }
    NNS_G3DResTexSetGfdKeys(tex, NNS_GfdMakeTexKey(offset, size, FALSE), 0);
    return resource;
}

void GFL_G3DScreenTexCapture(u32 bank, u32 destBank) {
    u32 texBanks = gfxGetTextureBanks();

    gfxSetLCDCBanks(SCREEN_CAPTURE_BANK_INFO[bank].vramBank);
    GX_SetCapture(GX_CAPTURE_SIZE_256x192, GX_CAPTURE_MODE_A, GX_CAPTURE_SRCA_2D3D, GX_CAPTURE_SRCB_VRAM_0x00000,
                  SCREEN_CAPTURE_BANK_INFO[bank].captureDest, 16, 0);
    OS_WaitVBlankIntr();
    OS_WaitVBlankIntr();
    gfxDisableLCDCBanks();
    gfxSetTextureBanks(texBanks);
    if (bank != destBank) {
        texBanks = gfxAcquireTextureBanks();
        sys_memcpy32_fast(SCREEN_CAPTURE_BANK_INFO[bank].lcdcAddr, SCREEN_CAPTURE_BANK_INFO[destBank].lcdcAddr,
                          SCREENTEX_SIZE);
        gfxDisableLCDCBanks();
        gfxSetTextureBanks(texBanks);
    }
}

void GFL_G3DAnmMdlDrawHeadless(G3DActor *actor) {
    VecFx32 camPos = { 0, 0, FX32_CONST(528) };
    VecFx32 camTarget = { 0, 0, 0 };
    VecFx32 camUp = { 0, FX32_ONE, 0 };
    SRTMatrix status = {
        { 0, 0, 0 },
        { FX32_ONE, FX32_ONE, FX32_ONE },
        { FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE },
    };

    NNS_G3dGlbPerspective(FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, FX32_ONE,
                          FX32_CONST(1024));
    NNS_G3dGlbLookAt(&camPos, &camUp, &camTarget);
    MTX_RotX33_(&status.rotation, FX_SinIdx(DEG_TO_IDX(90)), FX_CosIdx(DEG_TO_IDX(90)));
    GFL_G3DSysDrawObj(actor, &status);
}
