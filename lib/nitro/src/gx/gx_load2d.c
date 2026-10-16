// NitroSDK libgx's gx_load2d.c: the loads into the 2D engines' memory, which copy by DMA on GX's channel, by NDMA or
// with the CPU: the palettes, OAM, OBJ characters, BG characters and screens of both engines, and the extended
// palettes, which are loaded through a bank that is mapped to the CPU for the time (a Begin, the loads and an End). The
// file has no strings; its name is the SDK's, from the layout of libgx. It needs `-ipa file` (its extended palette
// state is reached from one base). The linker's veneers for the NDMA copy, the NDMA copy with a callback and the NDMA
// wait in TwlSDK's LTD module sit after their first callers; they are not part of the source.
#include "nitro/gx.h"

// NitroSDK's inline for the sub screen's OBJ VRAM, which gx.h doesn't have yet (ov194's pokemontrade_2d.c has its own)
static inline void *G2S_GetOBJCharPtr(void) {
    return (void *)HW_DB_OBJ_VRAM;
}

// The extended palette being loaded. The banks are what a Begin took from the engine (0 when it took none) and the End
// gives back; dest is the address of the first of them in the CPU's view, and offset where in the extended palette that
// bank starts, which is not 0 when only bank G holds the BGs' palette: it starts at its third slot. The first of the
// seven words is swan's g_GfxVRAMUploadBanksBGExtPltB
static u32 sObjExtPlttBanksB;
static u32 g_GfxVRAMUploadBanksBGExtPltB;
static u32 sObjExtPlttDestA;
static u32 sObjExtPlttBanksA;
static u32 sBGExtPlttOffsetA;
static u32 sBGExtPlttDestA;
static u32 sBGExtPlttBanksA;

void gfxUploadStdPaletteBGA(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_BG_PLTT + offset), size);
}

void gfxUploadStdPaletteBGB(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_DB_BG_PLTT + offset), size);
}

void gfxUploadStdPaletteObjA(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_OBJ_PLTT + offset), size);
}

void gfxUploadStdPaletteObjB(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_DB_OBJ_PLTT + offset), size);
}

void gfxUploadOAMA(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32(GXi_DmaId, src, (void *)(HW_OAM + offset), size);
}

void gfxUploadOAMB(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32(GXi_DmaId, src, (void *)(HW_DB_OAM + offset), size);
}

void gfxUploadObjCharA(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetOBJCharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadObjCharB(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetOBJCharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGScreen0A(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG0A();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen0B(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG0B();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen1A(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG1A();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen1B(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG1B();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen2A(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG2A();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen2B(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG2B();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen3A(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG3A();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGScreen3B(const void *src, u32 offset, u32 size) {
    u8 *screen = gfxGetScreenAddrBG3B();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void gfxUploadBGChar0A(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG0A();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar0B(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG0B();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar1A(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG1A();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar1B(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG1B();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar2A(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG2A();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar2B(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG2B();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar3A(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG3A();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxUploadBGChar3B(const void *src, u32 offset, u32 size) {
    u8 *chars = gfxGetCharAddrBG3B();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void gfxBeginBGExtPltAUpload(void) {
    GXVRamBGExtPltt banks = gfxAcquireBGExtPltBanksA();
    sBGExtPlttBanksA = banks;
    switch (banks) {
    case GX_VRAM_BGEXTPLTT_NONE:
        break;
    case GX_VRAM_BGEXTPLTT_0123_E:
        sBGExtPlttDestA = HW_LCDC_VRAM_E;
        sBGExtPlttOffsetA = 0;
        break;
    case GX_VRAM_BGEXTPLTT_23_G:
        sBGExtPlttDestA = HW_LCDC_VRAM_G;
        sBGExtPlttOffsetA = 0x4000;
        break;
    case GX_VRAM_BGEXTPLTT_01_F:
    case GX_VRAM_BGEXTPLTT_0123_FG:
        sBGExtPlttDestA = HW_LCDC_VRAM_F;
        sBGExtPlttOffsetA = 0;
        break;
    }
}

void gfxUploadExtPaletteBGA(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sBGExtPlttDestA + offset - sBGExtPlttOffsetA), size, NULL, NULL);
}

void gfxEndBGExtPltAUpload(void) {
    GXi_WaitDma(GXi_DmaId);
    gfxSetBGExtPltBanksA(sBGExtPlttBanksA);
    sBGExtPlttBanksA = 0;
    sBGExtPlttDestA = 0;
    sBGExtPlttOffsetA = 0;
}

void gfxBeginObjExtPltAUpload(void) {
    GXVRamOBJExtPltt banks = gfxAcquireObjExtPltBanksA();
    sObjExtPlttBanksA = banks;
    switch (banks) {
    case GX_VRAM_OBJEXTPLTT_NONE:
        break;
    case GX_VRAM_OBJEXTPLTT_0_F:
        sObjExtPlttDestA = HW_LCDC_VRAM_F;
        break;
    case GX_VRAM_OBJEXTPLTT_0_G:
        sObjExtPlttDestA = HW_LCDC_VRAM_G;
        break;
    }
}

void gfxUploadExtPaletteObjA(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sObjExtPlttDestA + offset), size, NULL, NULL);
}

void gfxEndObjExtPltAUpload(void) {
    GXi_WaitDma(GXi_DmaId);
    gfxSetObjExtPltBanksA(sObjExtPlttBanksA);
    sObjExtPlttBanksA = 0;
    sObjExtPlttDestA = 0;
}

void gfxBeginBGExtPltBUpload(void) {
    g_GfxVRAMUploadBanksBGExtPltB = gfxAcquireBGExtPltBanksB();
}

void gfxUploadExtPaletteBGB(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(HW_LCDC_VRAM_H + offset), size, NULL, NULL);
}

void gfxEndBGExtPltBUpload(void) {
    GXi_WaitDma(GXi_DmaId);
    gfxSetBGExtPltBanksB(g_GfxVRAMUploadBanksBGExtPltB);
    g_GfxVRAMUploadBanksBGExtPltB = 0;
}

void gfxBeginObjExtPltBUpload(void) {
    sObjExtPlttBanksB = gfxAcquireObjExtPltBanksB();
}

void gfxUploadExtPaletteObjB(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(HW_LCDC_VRAM_I + offset), size, NULL, NULL);
}

void gfxEndObjExtPltBUpload(void) {
    GXi_WaitDma(GXi_DmaId);
    gfxSetObjExtPltBanksB(sObjExtPlttBanksB);
    sObjExtPlttBanksB = 0;
}
