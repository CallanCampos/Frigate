// NitroSDK libgx's gx_load2d.c: the loads into the 2D engines' memory, which copy by DMA on GX's channel, by NDMA or
// with the CPU: the palettes, OAM, OBJ characters, BG characters and screens of both engines, and the extended
// palettes, which are loaded through a bank that is mapped to the CPU for the time (a Begin, the loads and an End). The
// file has no strings; its name is the SDK's, from the layout of libgx. It needs `-ipa file` (its extended palette
// state is reached from one base). The linker's veneers for the NDMA copy, the NDMA copy with a callback and the NDMA
// wait in TwlSDK's LTD module sit after their first callers; they are not part of the source.
#include "nitro/gx.h"

// The extended palette being loaded. The banks are what a Begin took from the engine (0 when it took none) and the End
// gives back; dest is the address of the first of them in the CPU's view, and offset where in the extended palette that
// bank starts, which is not 0 when only bank G holds the BGs' palette: it starts at its third slot. The first of the
// seven words is swan's sSubBGExtPltt. MWCC lays the words out in a fixed permutation of their order
// here, which is why sSubOBJExtPltt, the last in .bss, is declared first
static u32 sSubOBJExtPltt;
static u32 sSubBGExtPltt;
static u32 sOBJExtPlttLCDCBlk;
static u32 sOBJExtPltt;
static u32 sBGExtPlttLCDCOffset;
static u32 sBGExtPlttLCDCBlk;
static u32 sBGExtPltt;

void GX_LoadBGPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_BG_PLTT + offset), size);
}

void GXS_LoadBGPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_DB_BG_PLTT + offset), size);
}

void GX_LoadOBJPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_OBJ_PLTT + offset), size);
}

void GXS_LoadOBJPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy16(GXi_DmaId, src, (void *)(HW_DB_OBJ_PLTT + offset), size);
}

void GX_LoadOAM(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32(GXi_DmaId, src, (void *)(HW_OAM + offset), size);
}

void GXS_LoadOAM(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32(GXi_DmaId, src, (void *)(HW_DB_OAM + offset), size);
}

void GX_LoadOBJ(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetOBJCharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GXS_LoadOBJ(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetOBJCharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GX_LoadBG0Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2_GetBG0ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GXS_LoadBG0Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2S_GetBG0ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GX_LoadBG1Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2_GetBG1ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GXS_LoadBG1Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2S_GetBG1ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GX_LoadBG2Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2_GetBG2ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GXS_LoadBG2Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2S_GetBG2ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GX_LoadBG3Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2_GetBG3ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GXS_LoadBG3Scr(const void *src, u32 offset, u32 size) {
    u8 *screen = G2S_GetBG3ScrPtr();
    GXi_DmaCopy16(GXi_DmaId, src, screen + offset, size);
}

void GX_LoadBG0Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetBG0CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GXS_LoadBG0Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetBG0CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GX_LoadBG1Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetBG1CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GXS_LoadBG1Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetBG1CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GX_LoadBG2Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetBG2CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GXS_LoadBG2Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetBG2CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GX_LoadBG3Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2_GetBG3CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GXS_LoadBG3Char(const void *src, u32 offset, u32 size) {
    u8 *chars = G2S_GetBG3CharPtr();
    GXi_DmaCopy32(GXi_DmaId, src, chars + offset, size);
}

void GX_BeginLoadBGExtPltt(void) {
    GXVRamBGExtPltt banks = GX_ResetBankForBGExtPltt();
    sBGExtPltt = banks;
    switch (banks) {
    case GX_VRAM_BGEXTPLTT_NONE:
        break;
    case GX_VRAM_BGEXTPLTT_0123_E:
        sBGExtPlttLCDCBlk = HW_LCDC_VRAM_E;
        sBGExtPlttLCDCOffset = 0;
        break;
    case GX_VRAM_BGEXTPLTT_23_G:
        sBGExtPlttLCDCBlk = HW_LCDC_VRAM_G;
        sBGExtPlttLCDCOffset = 0x4000;
        break;
    case GX_VRAM_BGEXTPLTT_01_F:
    case GX_VRAM_BGEXTPLTT_0123_FG:
        sBGExtPlttLCDCBlk = HW_LCDC_VRAM_F;
        sBGExtPlttLCDCOffset = 0;
        break;
    }
}

void GX_LoadBGExtPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sBGExtPlttLCDCBlk + offset - sBGExtPlttLCDCOffset), size, NULL, NULL);
}

void GX_EndLoadBGExtPltt(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForBGExtPltt(sBGExtPltt);
    sBGExtPltt = 0;
    sBGExtPlttLCDCBlk = 0;
    sBGExtPlttLCDCOffset = 0;
}

void GX_BeginLoadOBJExtPltt(void) {
    GXVRamOBJExtPltt banks = GX_ResetBankForOBJExtPltt();
    sOBJExtPltt = banks;
    switch (banks) {
    case GX_VRAM_OBJEXTPLTT_NONE:
        break;
    case GX_VRAM_OBJEXTPLTT_0_F:
        sOBJExtPlttLCDCBlk = HW_LCDC_VRAM_F;
        break;
    case GX_VRAM_OBJEXTPLTT_0_G:
        sOBJExtPlttLCDCBlk = HW_LCDC_VRAM_G;
        break;
    }
}

void GX_LoadOBJExtPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sOBJExtPlttLCDCBlk + offset), size, NULL, NULL);
}

void GX_EndLoadOBJExtPltt(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForOBJExtPltt(sOBJExtPltt);
    sOBJExtPltt = 0;
    sOBJExtPlttLCDCBlk = 0;
}

void GXS_BeginLoadBGExtPltt(void) {
    sSubBGExtPltt = GX_ResetBankForSubBGExtPltt();
}

void GXS_LoadBGExtPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(HW_LCDC_VRAM_H + offset), size, NULL, NULL);
}

void GXS_EndLoadBGExtPltt(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForSubBGExtPltt(sSubBGExtPltt);
    sSubBGExtPltt = 0;
}

void GXS_BeginLoadOBJExtPltt(void) {
    sSubOBJExtPltt = GX_ResetBankForSubOBJExtPltt();
}

void GXS_LoadOBJExtPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(HW_LCDC_VRAM_I + offset), size, NULL, NULL);
}

void GXS_EndLoadOBJExtPltt(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForSubOBJExtPltt(sSubOBJExtPltt);
    sSubOBJExtPltt = 0;
}
