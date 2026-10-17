// Tests of the oracle's per-screen 3D decision (port/tools/oracle/display3d.c) on made-up frames: the 3D layer as
// engine A's BG0 on either screen, the title screen's capture into VRAM D shown in VRAM display mode (with the
// registers the oracle read from Black 2's title), dual-screen 3D through bitmap BGs and OBJs of engine B, and the ends
// of a bank's 3D: a capture without 3D over it, or the CPU writing it.
#include "display3d.h"
#include <stdio.h>
#include <string.h>

static int sFailed;
static int sChecks;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        sChecks++;                                                                                                     \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            sFailed++;                                                                                                 \
        }                                                                                                              \
    } while (0)

// DISPCNT and the other registers' bits the tests use (GBATEK)
#define GRAPHICS (1u << 16)
#define VRAM_DISPLAY(bank) ((2u << 16) | ((uint32_t)(bank) << 18))
#define BG0_3D ((1u << 8) | (1u << 3))
#define BG2 (1u << 10)
#define BG3 (1u << 11)
#define OBJ (1u << 12)
#define FORCED_BLANK (1u << 7)
#define BG_MODE(mode) ((uint32_t)(mode))
#define EXTENDED_DC_BITMAP 0x0084u // BGxCNT: bit 7, a bitmap, and bit 2, direct colour
#define POWCNT1_A_BOTTOM 0x020fu   // both engines and the 3D on, engine A on the bottom screen
#define POWCNT1_A_TOP 0x820fu
#define VRAM(mst, offset) ((uint8_t)(0x80u | (mst) | ((offset) << 3)))
#define LCDC VRAM(0, 0)
#define CAPTURE(control) ((control) | 0x80000000u) // DISPCAPCNT as the probe sees it, its enable latched
// DISPCAPCNT: engine A's output, 256x192, at the start of a bank
#define WHOLE_SCREEN_INTO(bank) (0x00300000u | ((uint32_t)(bank) << 16))
#define BANK_A 0
#define BANK_B 1
#define BANK_C 2
#define BANK_D 3

static uint8_t sMemory[PBW_DISPLAY_BANKS][PBW_DISPLAY_BANK_SIZE];
static const uint8_t *const sBanks[PBW_DISPLAY_BANKS] = { sMemory[0], sMemory[1], sMemory[2], sMemory[3] };

// The capture unit writing a picture into a bank, as the oracle would find it after the frame: the bytes change
static void Capture(PbwBanks3d *banks, uint32_t control, uint32_t dispcntA, uint8_t fill) {
    PbwCapture capture = { 1, CAPTURE(control), dispcntA, 1 };
    uint8_t atStart[PBW_DISPLAY_BANKS];
    int dest = (int)((control >> 16) & 3);

    memcpy(atStart, banks->holds3d, sizeof(atStart));
    memset(sMemory[dest], fill, 256 * 192 * 2);
    pbw_display3d_capture(banks, &capture, atStart, sBanks);
}

static void TestBg0(void) {
    PbwDisplayRegs regs = { 0 };
    uint8_t none[PBW_DISPLAY_BANKS] = { 0 };

    regs.dispcnt[0] = GRAPHICS | BG0_3D;
    regs.dispcnt[1] = GRAPHICS | BG3;
    regs.powcnt1 = POWCNT1_A_TOP;
    CHECK(pbw_display3d_screens(&regs, none) == PBW_DISPLAY_TOP);
    // POWCNT1's display swap puts engine A on the bottom screen
    regs.powcnt1 = POWCNT1_A_BOTTOM;
    CHECK(pbw_display3d_screens(&regs, none) == PBW_DISPLAY_BOTTOM);
    // BG0 as a 2D layer, BG0 hidden, a forced blank, the display off: no 3D
    regs.dispcnt[0] = GRAPHICS | (1u << 8);
    CHECK(pbw_display3d_screens(&regs, none) == 0);
    regs.dispcnt[0] = GRAPHICS | (1u << 3);
    CHECK(pbw_display3d_screens(&regs, none) == 0);
    regs.dispcnt[0] = GRAPHICS | BG0_3D | FORCED_BLANK;
    CHECK(pbw_display3d_screens(&regs, none) == 0);
    regs.dispcnt[0] = BG0_3D;
    CHECK(pbw_display3d_screens(&regs, none) == 0);
    // In VRAM display mode the screen shows the bank, not BG0
    regs.dispcnt[0] = VRAM_DISPLAY(BANK_D) | BG0_3D;
    regs.vramcnt[BANK_D] = LCDC;
    CHECK(pbw_display3d_screens(&regs, none) == 0);
}

// Black 2's title screen (src/ov162/title.c) as the oracle read it at frame 4000: engine A on the bottom screen in
// VRAM display mode, showing bank D, with BG0 as the 3D layer, and each frame capturing engine A's output blended with
// bank D into bank D
static void TestTitleCapture(void) {
    const uint32_t dispcntA = 0x001e0918;
    const uint32_t dispcapcnt = 0x4033040c;
    PbwDisplayRegs regs = { 0 };
    PbwBanks3d banks;

    pbw_display3d_init(&banks);
    regs.dispcnt[0] = dispcntA;
    regs.dispcnt[1] = GRAPHICS | BG0_3D | BG3; // engine B has no 3D layer, whatever its BG0 bit 3 says
    regs.powcnt1 = POWCNT1_A_BOTTOM;
    regs.vramcnt[BANK_D] = LCDC;

    // The first frame shows bank D as it was, without 3D, and captures into it
    pbw_display3d_check(&banks, sBanks);
    CHECK(pbw_display3d_screens(&regs, banks.holds3d) == 0);
    Capture(&banks, dispcapcnt, dispcntA, 0x11);
    CHECK(banks.holds3d[BANK_D]);
    // From the next on, the bottom screen shows the 3D
    pbw_display3d_check(&banks, sBanks);
    CHECK(banks.holds3d[BANK_D]);
    CHECK(pbw_display3d_screens(&regs, banks.holds3d) == PBW_DISPLAY_BOTTOM);
    // With the 3D layer off, the blend with bank D carries the old 3D on
    Capture(&banks, dispcapcnt, dispcntA & ~0x8u, 0x22);
    pbw_display3d_check(&banks, sBanks);
    CHECK(pbw_display3d_screens(&regs, banks.holds3d) == PBW_DISPLAY_BOTTOM);
    // A bank the LCDC doesn't have isn't shown
    regs.vramcnt[BANK_D] = VRAM(1, 0);
    CHECK(pbw_display3d_screens(&regs, banks.holds3d) == 0);
    regs.vramcnt[BANK_D] = LCDC;
    // A capture of engine A alone, without the 3D layer, over the whole picture: the 3D is gone
    Capture(&banks, 0x00330000, dispcntA & ~0x8u, 0x33);
    CHECK(!banks.holds3d[BANK_D]);
    CHECK(pbw_display3d_screens(&regs, banks.holds3d) == 0);
    // The 3D picture alone (source A's bit 24) has 3D whatever BG0 is
    Capture(&banks, 0x01330000, 0, 0x44);
    CHECK(banks.holds3d[BANK_D]);
    // Source B from the main memory FIFO has none
    Capture(&banks, 0x22330000, 0, 0x55);
    CHECK(!banks.holds3d[BANK_D]);
}

// Dual-screen 3D (src/gfl/double3Ddisp.c): engine A draws 3D on one screen and captures it, and engine B shows the last
// capture on the other, as a direct colour bitmap BG2 from bank C or as bitmap OBJs from bank D
static void TestDualScreen(void) {
    PbwDisplayRegs top = { 0 };
    PbwDisplayRegs bottom = { 0 };
    PbwBanks3d banks;
    int frame;

    // GFL_G3DDual3DSetupTopScreen: engine A on top, capturing into D; engine B shows C as BG2
    top.dispcnt[0] = GRAPHICS | BG0_3D;
    top.dispcnt[1] = GRAPHICS | BG_MODE(5) | BG2;
    top.bgcnt[1][2] = EXTENDED_DC_BITMAP | 0x4000;
    top.powcnt1 = POWCNT1_A_TOP;
    top.vramcnt[BANK_C] = VRAM(4, 0);
    top.vramcnt[BANK_D] = LCDC;
    // GFL_G3DDual3DSetupBottomScreen: engine A on the bottom, capturing into C; engine B shows D as OBJs
    bottom.dispcnt[0] = GRAPHICS | BG0_3D;
    bottom.dispcnt[1] = GRAPHICS | BG_MODE(5) | OBJ;
    bottom.powcnt1 = POWCNT1_A_BOTTOM;
    bottom.vramcnt[BANK_C] = LCDC;
    bottom.vramcnt[BANK_D] = VRAM(4, 0);

    pbw_display3d_init(&banks);
    CHECK(pbw_display3d_screens(&top, banks.holds3d) == PBW_DISPLAY_TOP);
    Capture(&banks, WHOLE_SCREEN_INTO(BANK_D), top.dispcnt[0], 0x10);
    for (frame = 0; frame < 4; frame++) {
        const PbwDisplayRegs *regs = frame % 2 == 0 ? &bottom : &top;
        int dest = frame % 2 == 0 ? BANK_C : BANK_D;

        pbw_display3d_check(&banks, sBanks);
        CHECK(pbw_display3d_screens(regs, banks.holds3d) == (PBW_DISPLAY_TOP | PBW_DISPLAY_BOTTOM));
        Capture(&banks, WHOLE_SCREEN_INTO(dest), regs->dispcnt[0], (uint8_t)(0x20 + frame));
    }

    // Engine B off: its screen shows nothing
    top.powcnt1 &= ~0x0200u;
    CHECK(pbw_display3d_screens(&top, banks.holds3d) == PBW_DISPLAY_TOP);
    top.powcnt1 |= 0x0200u;
    // BG2 as an affine BG of tiles, or hidden, doesn't show the bitmap
    top.bgcnt[1][2] = 0;
    CHECK(pbw_display3d_screens(&top, banks.holds3d) == PBW_DISPLAY_TOP);
    top.bgcnt[1][2] = EXTENDED_DC_BITMAP;
    top.dispcnt[1] = GRAPHICS | BG_MODE(5);
    CHECK(pbw_display3d_screens(&top, banks.holds3d) == PBW_DISPLAY_TOP);
    // Without OBJs shown, bank D's OBJs aren't either
    bottom.dispcnt[1] = GRAPHICS | BG_MODE(5);
    CHECK(pbw_display3d_screens(&bottom, banks.holds3d) == PBW_DISPLAY_BOTTOM);
}

// Engine A's own bitmaps: BG3 in mode 3, the large bitmap of mode 6, and bitmap OBJs from banks A and B
static void TestEngineABitmaps(void) {
    PbwDisplayRegs regs = { 0 };
    uint8_t holds[PBW_DISPLAY_BANKS] = { 0 };

    regs.powcnt1 = POWCNT1_A_TOP;
    regs.vramcnt[BANK_A] = VRAM(1, 0);
    holds[BANK_A] = 1;
    regs.dispcnt[0] = GRAPHICS | BG_MODE(3) | BG3;
    regs.bgcnt[0][3] = EXTENDED_DC_BITMAP;
    CHECK(pbw_display3d_screens(&regs, holds) == PBW_DISPLAY_TOP);
    regs.bgcnt[0][3] = 0;
    CHECK(pbw_display3d_screens(&regs, holds) == 0);
    regs.dispcnt[0] = GRAPHICS | BG_MODE(6) | BG2;
    CHECK(pbw_display3d_screens(&regs, holds) == PBW_DISPLAY_TOP);
    // Bank A as a texture slot is no BG
    regs.vramcnt[BANK_A] = VRAM(3, 0);
    CHECK(pbw_display3d_screens(&regs, holds) == 0);
    // Bank B as engine A's OBJ VRAM, with OBJs shown
    holds[BANK_A] = 0;
    holds[BANK_B] = 1;
    regs.vramcnt[BANK_B] = VRAM(2, 0);
    regs.dispcnt[0] = GRAPHICS | OBJ;
    CHECK(pbw_display3d_screens(&regs, holds) == PBW_DISPLAY_TOP);
    // A disabled bank shows nothing
    regs.vramcnt[BANK_B] = 2;
    CHECK(pbw_display3d_screens(&regs, holds) == 0);
}

static void TestEnds(void) {
    PbwBanks3d banks;
    PbwCapture skipped = { 0, CAPTURE(0x01330000), 0, 1 };
    PbwCapture notLcdc = { 1, CAPTURE(0x01330000), 0, 0 };
    uint8_t none[PBW_DISPLAY_BANKS] = { 0 };

    // A capture that didn't run, or whose bank the LCDC didn't have, writes nothing
    pbw_display3d_init(&banks);
    pbw_display3d_capture(&banks, &skipped, none, sBanks);
    pbw_display3d_capture(&banks, &notLcdc, none, sBanks);
    CHECK(!banks.holds3d[BANK_D]);

    // The CPU writing the captured region ends the 3D; writing past it doesn't
    Capture(&banks, 0x01330000, 0, 0x60);
    sMemory[BANK_D][256 * 192 * 2] ^= 0xff;
    pbw_display3d_check(&banks, sBanks);
    CHECK(banks.holds3d[BANK_D]);
    sMemory[BANK_D][1000] ^= 0xff;
    pbw_display3d_check(&banks, sBanks);
    CHECK(!banks.holds3d[BANK_D]);

    // A capture without 3D over part of the 3D keeps it, and the check accepts what it wrote
    Capture(&banks, 0x01330000, 0, 0x70);
    memset(sMemory[BANK_D], 0x71, 256 * 64 * 2);
    {
        PbwCapture part = { 1, CAPTURE(0x00130000), 0, 1 };
        pbw_display3d_capture(&banks, &part, banks.holds3d, sBanks);
    }
    pbw_display3d_check(&banks, sBanks);
    CHECK(banks.holds3d[BANK_D]);
    // A whole picture without 3D that starts after the 3D's start doesn't cover it; one at the same place does
    {
        PbwCapture later = { 1, CAPTURE(0x00370000), 0, 1 };
        PbwCapture same = { 1, CAPTURE(0x00330000), 0, 1 };
        pbw_display3d_capture(&banks, &later, banks.holds3d, sBanks);
        CHECK(banks.holds3d[BANK_D]);
        pbw_display3d_capture(&banks, &same, banks.holds3d, sBanks);
        CHECK(!banks.holds3d[BANK_D]);
    }
    // 3D captured at offset 0x18000 wraps around the bank's end, and a write at its start ends it
    Capture(&banks, 0x013f0000, 0, 0x80);
    pbw_display3d_check(&banks, sBanks);
    CHECK(banks.holds3d[BANK_D] && banks.offset[BANK_D] == 0x18000);
    sMemory[BANK_D][0x100] ^= 0xff;
    pbw_display3d_check(&banks, sBanks);
    CHECK(!banks.holds3d[BANK_D]);
}

int main(void) {
    TestBg0();
    TestTitleCapture();
    TestDualScreen();
    TestEngineABitmaps();
    TestEnds();
    if (sFailed != 0) {
        fprintf(stderr, "%d of %d checks failed\n", sFailed, sChecks);
        return 1;
    }
    printf("%d checks passed\n", sChecks);
    return 0;
}
