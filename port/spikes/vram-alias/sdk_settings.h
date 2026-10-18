// Frozen phase-0 spike (port/spikes/README.md), outside Frigate's build: the live code is port/tests/gx/sdk_settings.h;
// change that, not this copy.
// Every VRAM setting of the SDK, for test_vram.c: for each case of a GX_SetBankFor* switch in
// lib/nitro/src/gx/gx_vramcnt.c (85 of them) and for GX_SetBankForLCDC of each bank, the VRAMCNT bytes the SDK writes
// and where each bank page must then be. The writes were taken from gx_vramcnt.c by a script. The places were written
// out by hand from GBATEK's address table ("DS Memory Control - VRAM"), as DS addresses in the CPU views and as
// GBATEK's slot numbers elsewhere, plus the mirrors melonDS maps (GPU.cpp MapVRAM_FG, MapVRAM_H and MapVRAM_I).
// Nothing here comes from vram_layout.c, so a wrong offset or a missing place there fails a row
#ifndef SDK_SETTINGS_H
#define SDK_SETTINGS_H

#include "vram.h"

// A VRAMCNT write, bank 'A' to 'I' (0 ends the list)
typedef struct SdkWrite {
    char bank;
    uint8_t value;
} SdkWrite;

// A run of a bank's pages from its first, at a byte offset in a view: all of them, or only pages of them
typedef struct SdkPlace {
    char bank;
    uint8_t view;
    uint32_t offset;
    uint8_t pages;
    uint8_t mirror;
} SdkPlace;

typedef struct SdkSetting {
    const char *name;
    SdkWrite writes[4];
    SdkPlace places[8];
} SdkSetting;

#define W(bank, mst, ofs) { bank, (uint8_t)(0x80 | (ofs) << 3 | (mst)) }
// The whole bank, a mirror of the whole bank, or only its first two pages (E's lower 32 KiB as extended palettes)
#define P(bank, where) { bank, where, 0, 0 }
#define M(bank, where) { bank, where, 0, 1 }
#define P2(bank, where) { bank, where, 2, 0 }
// GBATEK's addresses in the CPU views, and its slots in the others: texture slots of 128 KiB, texture palette slots of
// 16 KiB and extended palette slots of 8 KiB
#define LCDC(addr) VRAM_VIEW_LCDC, (addr) - 0x06800000u
#define BG_A(addr) VRAM_VIEW_BG_A, (addr) - 0x06000000u
#define OBJ_A(addr) VRAM_VIEW_OBJ_A, (addr) - 0x06400000u
#define BG_B(addr) VRAM_VIEW_BG_B, (addr) - 0x06200000u
#define OBJ_B(addr) VRAM_VIEW_OBJ_B, (addr) - 0x06600000u
#define TEX(slot) VRAM_VIEW_TEX, (slot) * 0x20000u
#define TEXPLTT(slot) VRAM_VIEW_TEXPLTT, (slot) * 0x4000u
#define BGEXT_A(slot) VRAM_VIEW_BGEXT_A, (slot) * 0x2000u
#define OBJEXT_A VRAM_VIEW_OBJEXT_A, 0
#define BGEXT_B(slot) VRAM_VIEW_BGEXT_B, (slot) * 0x2000u
#define OBJEXT_B VRAM_VIEW_OBJEXT_B, 0

// GX_VRAM_ARM7_* give C and D to the ARM7, which the host doesn't have: they are nowhere
static const SdkSetting SDK_SETTINGS[] = {
    { "GX_SetBankForLCDC(GX_VRAM_A)", { W('A', 0, 0) }, { P('A', LCDC(0x06800000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_B)", { W('B', 0, 0) }, { P('B', LCDC(0x06820000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_C)", { W('C', 0, 0) }, { P('C', LCDC(0x06840000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_D)", { W('D', 0, 0) }, { P('D', LCDC(0x06860000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_E)", { W('E', 0, 0) }, { P('E', LCDC(0x06880000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_F)", { W('F', 0, 0) }, { P('F', LCDC(0x06890000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_G)", { W('G', 0, 0) }, { P('G', LCDC(0x06894000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_H)", { W('H', 0, 0) }, { P('H', LCDC(0x06898000)) } },
    { "GX_SetBankForLCDC(GX_VRAM_I)", { W('I', 0, 0) }, { P('I', LCDC(0x068A0000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_128_D)", { W('D', 1, 0) }, { P('D', BG_A(0x06000000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_CD)",
      { W('D', 1, 1), W('C', 1, 0) },
      { P('C', BG_A(0x06000000)), P('D', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_128_C)", { W('C', 1, 0) }, { P('C', BG_A(0x06000000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_384_BCD)",
      { W('D', 1, 2), W('C', 1, 1), W('B', 1, 0) },
      { P('B', BG_A(0x06000000)), P('C', BG_A(0x06020000)), P('D', BG_A(0x06040000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_BC)",
      { W('C', 1, 1), W('B', 1, 0) },
      { P('B', BG_A(0x06000000)), P('C', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_128_B)", { W('B', 1, 0) }, { P('B', BG_A(0x06000000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_512_ABCD)",
      { W('D', 1, 3), W('C', 1, 2), W('B', 1, 1), W('A', 1, 0) },
      { P('A', BG_A(0x06000000)), P('B', BG_A(0x06020000)), P('C', BG_A(0x06040000)), P('D', BG_A(0x06060000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_384_ABC)",
      { W('C', 1, 2), W('B', 1, 1), W('A', 1, 0) },
      { P('A', BG_A(0x06000000)), P('B', BG_A(0x06020000)), P('C', BG_A(0x06040000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_AB)",
      { W('B', 1, 1), W('A', 1, 0) },
      { P('A', BG_A(0x06000000)), P('B', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_128_A)", { W('A', 1, 0) }, { P('A', BG_A(0x06000000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_384_ABD)",
      { W('A', 1, 0), W('B', 1, 1), W('D', 1, 2) },
      { P('A', BG_A(0x06000000)), P('B', BG_A(0x06020000)), P('D', BG_A(0x06040000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_384_ACD)",
      { W('D', 1, 2), W('A', 1, 0), W('C', 1, 1) },
      { P('A', BG_A(0x06000000)), P('C', BG_A(0x06020000)), P('D', BG_A(0x06040000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_AC)",
      { W('A', 1, 0), W('C', 1, 1) },
      { P('A', BG_A(0x06000000)), P('C', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_AD)",
      { W('A', 1, 0), W('D', 1, 1) },
      { P('A', BG_A(0x06000000)), P('D', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_256_BD)",
      { W('B', 1, 0), W('D', 1, 1) },
      { P('B', BG_A(0x06000000)), P('D', BG_A(0x06020000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_96_EFG)",
      { W('G', 1, 3), W('F', 1, 2), W('E', 1, 0) },
      { P('E', BG_A(0x06000000)), P('F', BG_A(0x06010000)), P('G', BG_A(0x06014000)), M('F', BG_A(0x06018000)),
        M('G', BG_A(0x0601C000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_80_EF)",
      { W('F', 1, 2), W('E', 1, 0) },
      { P('E', BG_A(0x06000000)), P('F', BG_A(0x06010000)), M('F', BG_A(0x06018000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_64_E)", { W('E', 1, 0) }, { P('E', BG_A(0x06000000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_80_EG)",
      { W('G', 1, 2), W('E', 1, 0) },
      { P('E', BG_A(0x06000000)), P('G', BG_A(0x06010000)), M('G', BG_A(0x06018000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_32_FG)",
      { W('G', 1, 1), W('F', 1, 0) },
      { P('F', BG_A(0x06000000)), P('G', BG_A(0x06004000)), M('F', BG_A(0x06008000)), M('G', BG_A(0x0600C000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_16_F)", { W('F', 1, 0) }, { P('F', BG_A(0x06000000)), M('F', BG_A(0x06008000)) } },
    { "GX_SetBankForBG(GX_VRAM_BG_16_G)", { W('G', 1, 0) }, { P('G', BG_A(0x06000000)), M('G', BG_A(0x06008000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_256_AB)",
      { W('B', 2, 1), W('A', 2, 0) },
      { P('A', OBJ_A(0x06400000)), P('B', OBJ_A(0x06420000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_128_A)", { W('A', 2, 0) }, { P('A', OBJ_A(0x06400000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_128_B)", { W('B', 2, 0) }, { P('B', OBJ_A(0x06400000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_96_EFG)",
      { W('G', 2, 3), W('F', 2, 2), W('E', 2, 0) },
      { P('E', OBJ_A(0x06400000)), P('F', OBJ_A(0x06410000)), P('G', OBJ_A(0x06414000)), M('F', OBJ_A(0x06418000)),
        M('G', OBJ_A(0x0641C000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_80_EF)",
      { W('F', 2, 2), W('E', 2, 0) },
      { P('E', OBJ_A(0x06400000)), P('F', OBJ_A(0x06410000)), M('F', OBJ_A(0x06418000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_64_E)", { W('E', 2, 0) }, { P('E', OBJ_A(0x06400000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_80_EG)",
      { W('G', 2, 2), W('E', 2, 0) },
      { P('E', OBJ_A(0x06400000)), P('G', OBJ_A(0x06410000)), M('G', OBJ_A(0x06418000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_32_FG)",
      { W('G', 2, 1), W('F', 2, 0) },
      { P('F', OBJ_A(0x06400000)), P('G', OBJ_A(0x06404000)), M('F', OBJ_A(0x06408000)), M('G', OBJ_A(0x0640C000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_16_F)",
      { W('F', 2, 0) },
      { P('F', OBJ_A(0x06400000)), M('F', OBJ_A(0x06408000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_16_G)",
      { W('G', 2, 0) },
      { P('G', OBJ_A(0x06400000)), M('G', OBJ_A(0x06408000)) } },
    { "GX_SetBankForOBJ(GX_VRAM_OBJ_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_0123_E)", { W('E', 4, 0) }, { P2('E', BGEXT_A(0)) } },
    { "GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_23_G)", { W('G', 4, 1) }, { P('G', BGEXT_A(2)) } },
    { "GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_0123_FG)",
      { W('G', 4, 1), W('F', 4, 0) },
      { P('F', BGEXT_A(0)), P('G', BGEXT_A(2)) } },
    { "GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_01_F)", { W('F', 4, 0) }, { P('F', BGEXT_A(0)) } },
    { "GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForOBJExtPltt(GX_VRAM_OBJEXTPLTT_0_F)", { W('F', 5, 0) }, { P('F', OBJEXT_A) } },
    { "GX_SetBankForOBJExtPltt(GX_VRAM_OBJEXTPLTT_0_G)", { W('G', 5, 0) }, { P('G', OBJEXT_A) } },
    { "GX_SetBankForOBJExtPltt(GX_VRAM_OBJEXTPLTT_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_AC)", { W('A', 3, 0), W('C', 3, 1) }, { P('A', TEX(0)), P('C', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_AD)", { W('A', 3, 0), W('D', 3, 1) }, { P('A', TEX(0)), P('D', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_BD)", { W('B', 3, 0), W('D', 3, 1) }, { P('B', TEX(0)), P('D', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_012_ABD)",
      { W('A', 3, 0), W('B', 3, 1), W('D', 3, 2) },
      { P('A', TEX(0)), P('B', TEX(1)), P('D', TEX(2)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_012_ACD)",
      { W('A', 3, 0), W('C', 3, 1), W('D', 3, 2) },
      { P('A', TEX(0)), P('C', TEX(1)), P('D', TEX(2)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_0_D)", { W('D', 3, 0) }, { P('D', TEX(0)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_CD)", { W('D', 3, 1), W('C', 3, 0) }, { P('C', TEX(0)), P('D', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_0_C)", { W('C', 3, 0) }, { P('C', TEX(0)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_012_BCD)",
      { W('D', 3, 2), W('C', 3, 1), W('B', 3, 0) },
      { P('B', TEX(0)), P('C', TEX(1)), P('D', TEX(2)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_BC)", { W('C', 3, 1), W('B', 3, 0) }, { P('B', TEX(0)), P('C', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_0_B)", { W('B', 3, 0) }, { P('B', TEX(0)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_0123_ABCD)",
      { W('D', 3, 3), W('C', 3, 2), W('B', 3, 1), W('A', 3, 0) },
      { P('A', TEX(0)), P('B', TEX(1)), P('C', TEX(2)), P('D', TEX(3)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_012_ABC)",
      { W('C', 3, 2), W('B', 3, 1), W('A', 3, 0) },
      { P('A', TEX(0)), P('B', TEX(1)), P('C', TEX(2)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_01_AB)", { W('B', 3, 1), W('A', 3, 0) }, { P('A', TEX(0)), P('B', TEX(1)) } },
    { "GX_SetBankForTex(GX_VRAM_TEX_0_A)", { W('A', 3, 0) }, { P('A', TEX(0)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_01_FG)",
      { W('G', 3, 1), W('F', 3, 0) },
      { P('F', TEXPLTT(0)), P('G', TEXPLTT(1)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_0_F)", { W('F', 3, 0) }, { P('F', TEXPLTT(0)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_0_G)", { W('G', 3, 0) }, { P('G', TEXPLTT(0)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_012345_EFG)",
      { W('G', 3, 3), W('F', 3, 2), W('E', 3, 0) },
      { P('E', TEXPLTT(0)), P('F', TEXPLTT(4)), P('G', TEXPLTT(5)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_01234_EF)",
      { W('F', 3, 2), W('E', 3, 0) },
      { P('E', TEXPLTT(0)), P('F', TEXPLTT(4)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_0123_E)", { W('E', 3, 0) }, { P('E', TEXPLTT(0)) } },
    { "GX_SetBankForTexPltt(GX_VRAM_TEXPLTT_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_256_AB)",
      { W('A', 3, 2), W('B', 3, 3) },
      { P('A', TEX(2)), P('B', TEX(3)) } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_128_B)", { W('B', 3, 3) }, { P('B', TEX(3)) } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_256_CD)",
      { W('C', 3, 2), W('D', 3, 3) },
      { P('C', TEX(2)), P('D', TEX(3)) } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_128_D)", { W('D', 3, 3) }, { P('D', TEX(3)) } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_128_A)", { W('A', 3, 3) }, { P('A', TEX(3)) } },
    { "GX_SetBankForClearImage(GX_VRAM_CLEARIMAGE_128_C)", { W('C', 3, 3) }, { P('C', TEX(3)) } },
    { "GX_SetBankForARM7(GX_VRAM_ARM7_256_CD)", { W('D', 2, 1), W('C', 2, 0) }, { { 0 } } },
    { "GX_SetBankForARM7(GX_VRAM_ARM7_128_C)", { W('C', 2, 0) }, { { 0 } } },
    { "GX_SetBankForARM7(GX_VRAM_ARM7_128_D)", { W('D', 2, 0) }, { { 0 } } },
    { "GX_SetBankForARM7(GX_VRAM_ARM7_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForSubBG(GX_VRAM_SUB_BG_128_C)", { W('C', 4, 0) }, { P('C', BG_B(0x06200000)) } },
    { "GX_SetBankForSubBG(GX_VRAM_SUB_BG_48_HI)",
      { W('I', 1, 0), W('H', 1, 0) },
      { P('H', BG_B(0x06200000)), P('I', BG_B(0x06208000)), M('I', BG_B(0x0620C000)), M('H', BG_B(0x06210000)),
        M('I', BG_B(0x06218000)), M('I', BG_B(0x0621C000)) } },
    { "GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H)",
      { W('H', 1, 0) },
      { P('H', BG_B(0x06200000)), M('H', BG_B(0x06210000)) } },
    { "GX_SetBankForSubBG(GX_VRAM_SUB_BG_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_128_D)", { W('D', 4, 0) }, { P('D', OBJ_B(0x06600000)) } },
    { "GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I)",
      { W('I', 2, 0) },
      { P('I', OBJ_B(0x06600000)), M('I', OBJ_B(0x06604000)), M('I', OBJ_B(0x06608000)), M('I', OBJ_B(0x0660C000)),
        M('I', OBJ_B(0x06610000)), M('I', OBJ_B(0x06614000)), M('I', OBJ_B(0x06618000)), M('I', OBJ_B(0x0661C000)) } },
    { "GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForSubBGExtPltt(GX_VRAM_SUB_BGEXTPLTT_0123_H)", { W('H', 2, 0) }, { P('H', BGEXT_B(0)) } },
    { "GX_SetBankForSubBGExtPltt(GX_VRAM_SUB_BGEXTPLTT_NONE)", { { 0 } }, { { 0 } } },
    { "GX_SetBankForSubOBJExtPltt(GX_VRAM_SUB_OBJEXTPLTT_0_I)", { W('I', 3, 0) }, { P('I', OBJEXT_B) } },
    { "GX_SetBankForSubOBJExtPltt(GX_VRAM_SUB_OBJEXTPLTT_NONE)", { { 0 } }, { { 0 } } },
};

#undef W
#undef P
#undef M
#undef P2
#undef LCDC
#undef BG_A
#undef OBJ_A
#undef BG_B
#undef OBJ_B
#undef TEX
#undef TEXPLTT
#undef BGEXT_A
#undef OBJEXT_A
#undef BGEXT_B
#undef OBJEXT_B

#endif // SDK_SETTINGS_H
