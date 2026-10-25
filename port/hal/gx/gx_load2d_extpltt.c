// NitroSDK libgx's loads of the main engine's extended palettes (gx_load2d.c) on the host: GX_BeginLoadBGExtPltt,
// GX_LoadBGExtPltt, GX_EndLoadBGExtPltt and the same three for the OBJ palette. The decomp's versions keep the bank's
// LCDC address in a static u32, which a host address doesn't fit in, so port/cmake/gx.cmake renames them in
// gx_load2d.c (where they stay, unused) and these take their names. The rest of gx_load2d.c compiles as it is: the sub
// engine's loads add their offset to HW_LCDC_VRAM_H and _I directly, and copy by DMA as the file's other loads do.
//
// The functions are the decomp's with the addresses kept as DS addresses, and with the copy that the SDK starts on GX's
// DMA channel made at once by port_gx_load_copy (port/hal/gx/gx_load3d.c says why), so the End has no DMA to wait for.
// The sub engine's banks, sSubBGExtPltt (swan's g_GfxVRAMUploadBanksBGExtPltB) and sSubOBJExtPltt, which only its loads
// use, stay the decomp's statics in gx_load2d.c
#include "hal/gx/gx_internal.h"
#include "nitro/gx.h"

// The banks' addresses in LCDC mode (HW_LCDC_VRAM_E-G)
#define LCDC_VRAM_E 0x06880000u
#define LCDC_VRAM_F 0x06890000u
#define LCDC_VRAM_G 0x06894000u

// The extended palette being loaded. The banks are what a Begin took from the engine (0 when it took none) and the End
// gives back; the LCDCBlk words are the DS address of the first of them in the CPU's view, and sBGExtPlttLCDCOffset
// where in the extended palette that bank starts, which is not 0 when only bank G holds the BGs' palette: it starts at
// its third slot. They have the names of gx_load2d.c's, NitroSDK's, which stay there, unused
static u32 sOBJExtPlttLCDCBlk;
static u32 sOBJExtPltt;
static u32 sBGExtPlttLCDCOffset;
static u32 sBGExtPlttLCDCBlk;
static u32 sBGExtPltt;

void GX_BeginLoadBGExtPltt(void) {
    GXVRamBGExtPltt banks = GX_ResetBankForBGExtPltt();
    sBGExtPltt = banks;
    switch (banks) {
    case GX_VRAM_BGEXTPLTT_NONE:
        break;
    case GX_VRAM_BGEXTPLTT_0123_E:
        sBGExtPlttLCDCBlk = LCDC_VRAM_E;
        sBGExtPlttLCDCOffset = 0;
        break;
    case GX_VRAM_BGEXTPLTT_23_G:
        sBGExtPlttLCDCBlk = LCDC_VRAM_G;
        sBGExtPlttLCDCOffset = 0x4000;
        break;
    case GX_VRAM_BGEXTPLTT_01_F:
    case GX_VRAM_BGEXTPLTT_0123_FG:
        sBGExtPlttLCDCBlk = LCDC_VRAM_F;
        sBGExtPlttLCDCOffset = 0;
        break;
    }
}

void GX_LoadBGExtPltt(const void *src, u32 offset, u32 size) {
    port_gx_load_copy(sBGExtPlttLCDCBlk + offset - sBGExtPlttLCDCOffset, src, size);
}

void GX_EndLoadBGExtPltt(void) {
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
        sOBJExtPlttLCDCBlk = LCDC_VRAM_F;
        break;
    case GX_VRAM_OBJEXTPLTT_0_G:
        sOBJExtPlttLCDCBlk = LCDC_VRAM_G;
        break;
    }
}

void GX_LoadOBJExtPltt(const void *src, u32 offset, u32 size) {
    port_gx_load_copy(sOBJExtPlttLCDCBlk + offset, src, size);
}

void GX_EndLoadOBJExtPltt(void) {
    GX_SetBankForOBJExtPltt(sOBJExtPltt);
    sOBJExtPltt = 0;
    sOBJExtPlttLCDCBlk = 0;
}
