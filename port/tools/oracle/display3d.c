// Which screens show the 3D engine's output in a frame, for the oracle's frames.tsv, so that compare.py compares a
// screen with 3D within a tolerance and the others exactly; the rules are in display3d.h, the registers' bits in
// GBATEK.
#include "display3d.h"

// DISPCNT: the BG mode (bits 0-2), BG0 as the 3D layer (bit 3, engine A), forced blank (bit 7), the layers shown (bits
// 8-12) and the display mode (bits 16-17: off, graphics, VRAM, main memory; engine B has the first two), with the
// bank VRAM display shows (bits 18-19)
#define DISPCNT_BG_MODE_MASK 7u
#define DISPCNT_BG0_3D (1u << 3)
#define DISPCNT_FORCED_BLANK (1u << 7)
#define DISPCNT_BG0 (1u << 8)
#define DISPCNT_BG2 (1u << 10)
#define DISPCNT_BG3 (1u << 11)
#define DISPCNT_OBJ (1u << 12)
#define DISPCNT_DISPLAY_SHIFT 16
#define DISPCNT_VRAM_BANK_SHIFT 18
#define DISPLAY_OFF 0u
#define DISPLAY_GRAPHICS 1u
#define DISPLAY_VRAM 2u

// BGxCNT of an extended BG (BG2 or BG3 in modes 3-5): bit 7 makes it a bitmap
#define BGCNT_EXTENDED_BITMAP (1u << 7)

// POWCNT1: engine B's power (bit 9) and the display swap (bit 15: engine A on the top screen)
#define POWCNT1_ENGINE_B (1u << 9)
#define POWCNT1_ENGINE_A_TOP (1u << 15)

// VRAMCNT: the master select (bits 0-2; banks A and B have bits 0-1) and the enable (bit 7). Every bank's 0 is the
// LCDC and 1 engine A's BG; A and B's 2 is engine A's OBJ, C's 4 engine B's BG and D's 4 engine B's OBJ
#define VRAMCNT_ENABLE (1u << 7)
#define MST_LCDC 0u
#define MST_MAIN_BG 1u
#define MST_MAIN_OBJ 2u
#define MST_SUB 4u
#define BANK_A 0
#define BANK_B 1
#define BANK_C 2
#define BANK_D 3

// DISPCAPCNT: the destination bank (bits 16-17), its offset (bits 18-19, 32 KiB steps) and the size (bits 20-21),
// source A (bit 24: 0 engine A's output, 1 the 3D picture), source B (bit 25: 0 VRAM, 1 the main memory FIFO) and what
// is captured (bits 29-30: A, B, or the two blended)
#define CAPTURE_DEST_SHIFT 16
#define CAPTURE_OFFSET_SHIFT 18
#define CAPTURE_OFFSET_STEP 0x8000u
#define CAPTURE_SIZE_SHIFT 20
#define CAPTURE_SOURCE_A_3D (1u << 24)
#define CAPTURE_SOURCE_B_FIFO (1u << 25)
#define CAPTURE_SOURCE_SHIFT 29
#define CAPTURE_SOURCE_A 0u
#define CAPTURE_SOURCE_B 1u

// The capture sizes' bytes: 128x128, 256x64, 256x128 and 256x192 pixels of 2 bytes
static const uint32_t sCaptureBytes[4] = { 128 * 128 * 2, 256 * 64 * 2, 256 * 128 * 2, 256 * 192 * 2 };

void pbw_display3d_init(PbwBanks3d *banks) {
    int bank;

    for (bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        banks->holds3d[bank] = 0;
        banks->offset[bank] = 0;
        banks->size[bank] = 0;
        banks->hash[bank] = 0;
    }
}

static int Bg0Is3d(uint32_t dispcnt) {
    return (dispcnt & DISPCNT_BG0) != 0 && (dispcnt & DISPCNT_BG0_3D) != 0;
}

static unsigned MasterSelect(const PbwDisplayRegs *regs, int bank) {
    return regs->vramcnt[bank] & (bank <= BANK_B ? 3u : 7u);
}

static int BankEnabled(const PbwDisplayRegs *regs, int bank) {
    return (regs->vramcnt[bank] & VRAMCNT_ENABLE) != 0;
}

// Whether the engine's BGs read the bank
static int BankIsBg(const PbwDisplayRegs *regs, int engine, int bank) {
    if (!BankEnabled(regs, bank)) {
        return 0;
    }
    return engine == 0 ? MasterSelect(regs, bank) == MST_MAIN_BG
                       : bank == BANK_C && MasterSelect(regs, bank) == MST_SUB;
}

// Whether the engine's OBJs read the bank
static int BankIsObj(const PbwDisplayRegs *regs, int engine, int bank) {
    if (!BankEnabled(regs, bank)) {
        return 0;
    }
    return engine == 0 ? bank <= BANK_B && MasterSelect(regs, bank) == MST_MAIN_OBJ
                       : bank == BANK_D && MasterSelect(regs, bank) == MST_SUB;
}

// Whether a bitmap BG is shown: an extended BG set to a bitmap (BG3 in modes 3-5, BG2 in mode 5) or engine A's large
// bitmap (BG2 in mode 6)
static int BitmapBgShown(const PbwDisplayRegs *regs, int engine) {
    uint32_t dispcnt = regs->dispcnt[engine];
    unsigned mode = dispcnt & DISPCNT_BG_MODE_MASK;

    if (mode >= 3 && mode <= 5 && (dispcnt & DISPCNT_BG3) && (regs->bgcnt[engine][3] & BGCNT_EXTENDED_BITMAP)) {
        return 1;
    }
    if (mode == 5 && (dispcnt & DISPCNT_BG2) && (regs->bgcnt[engine][2] & BGCNT_EXTENDED_BITMAP)) {
        return 1;
    }
    return engine == 0 && mode == 6 && (dispcnt & DISPCNT_BG2);
}

// Whether an engine's picture has 3D in it. A bitmap BG or bitmap OBJs in a bank that holds 3D count without looking
// at their addresses or at OAM: a bank captured into is seldom also used for other pictures while it holds one
static int EngineShows3d(const PbwDisplayRegs *regs, int engine, const uint8_t holds3d[PBW_DISPLAY_BANKS]) {
    uint32_t dispcnt = regs->dispcnt[engine];
    unsigned display = (dispcnt >> DISPCNT_DISPLAY_SHIFT) & (engine == 0 ? 3u : 1u);
    int bank;

    if (engine == 1 && !(regs->powcnt1 & POWCNT1_ENGINE_B)) {
        return 0;
    }
    if (engine == 0 && display == DISPLAY_VRAM) {
        // The bank shows only while the LCDC has it
        bank = (int)((dispcnt >> DISPCNT_VRAM_BANK_SHIFT) & 3);
        return holds3d[bank] && BankEnabled(regs, bank) && MasterSelect(regs, bank) == MST_LCDC;
    }
    if (display != DISPLAY_GRAPHICS || (dispcnt & DISPCNT_FORCED_BLANK)) {
        return 0;
    }
    if (engine == 0 && Bg0Is3d(dispcnt)) {
        return 1;
    }
    for (bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        if (!holds3d[bank]) {
            continue;
        }
        if (BankIsBg(regs, engine, bank) && BitmapBgShown(regs, engine)) {
            return 1;
        }
        if (BankIsObj(regs, engine, bank) && (dispcnt & DISPCNT_OBJ)) {
            return 1;
        }
    }
    return 0;
}

unsigned pbw_display3d_screens(const PbwDisplayRegs *regs, const uint8_t holds3d[PBW_DISPLAY_BANKS]) {
    unsigned aScreen = (regs->powcnt1 & POWCNT1_ENGINE_A_TOP) ? PBW_DISPLAY_TOP : PBW_DISPLAY_BOTTOM;
    unsigned bScreen = aScreen == PBW_DISPLAY_TOP ? PBW_DISPLAY_BOTTOM : PBW_DISPLAY_TOP;
    unsigned screens = 0;

    if (EngineShows3d(regs, 0, holds3d)) {
        screens |= aScreen;
    }
    if (EngineShows3d(regs, 1, holds3d)) {
        screens |= bScreen;
    }
    return screens;
}

// FNV-1a over a region of a bank, which wraps around at the bank's end as the capture unit's writes do
static uint64_t HashRegion(const uint8_t *memory, uint32_t offset, uint32_t size) {
    uint64_t hash = 0xcbf29ce484222325u;
    uint32_t i;

    for (i = 0; i < size; i++) {
        hash ^= memory[(offset + i) & (PBW_DISPLAY_BANK_SIZE - 1)];
        hash *= 0x100000001b3u;
    }
    return hash;
}

void pbw_display3d_capture(PbwBanks3d *banks, const PbwCapture *capture,
                           const uint8_t holds3dAtStart[PBW_DISPLAY_BANKS],
                           const uint8_t *const memory[PBW_DISPLAY_BANKS]) {
    uint32_t control = capture->dispcapcnt;
    int dest = (int)((control >> CAPTURE_DEST_SHIFT) & 3);
    unsigned source = (control >> CAPTURE_SOURCE_SHIFT) & 3;
    uint32_t offset = ((control >> CAPTURE_OFFSET_SHIFT) & 3) * CAPTURE_OFFSET_STEP;
    uint32_t size = sCaptureBytes[(control >> CAPTURE_SIZE_SHIFT) & 3];
    int aHas3d;
    int bHas3d;
    int has3d;

    if (!capture->ran || !capture->destInLcdc) {
        return;
    }
    aHas3d = (control & CAPTURE_SOURCE_A_3D) != 0 || Bg0Is3d(capture->dispcntA);
    bHas3d = (control & CAPTURE_SOURCE_B_FIFO) == 0 &&
             holds3dAtStart[(capture->dispcntA >> DISPCNT_VRAM_BANK_SHIFT) & 3] != 0;
    has3d = source == CAPTURE_SOURCE_A ? aHas3d : source == CAPTURE_SOURCE_B ? bHas3d : aHas3d || bHas3d;
    if (has3d) {
        banks->holds3d[dest] = 1;
        banks->offset[dest] = offset;
        banks->size[dest] = size;
    } else if (banks->holds3d[dest]) {
        // A picture without 3D ends the bank's 3D only when it covers the region the 3D went in; else the rest of that
        // stays, and its hash is taken again over what the capture changed
        uint32_t into = (banks->offset[dest] - offset) & (PBW_DISPLAY_BANK_SIZE - 1);
        if (into + banks->size[dest] <= size) {
            banks->holds3d[dest] = 0;
            return;
        }
    } else {
        return;
    }
    banks->hash[dest] = HashRegion(memory[dest], banks->offset[dest], banks->size[dest]);
}

void pbw_display3d_check(PbwBanks3d *banks, const uint8_t *const memory[PBW_DISPLAY_BANKS]) {
    int bank;

    for (bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        if (banks->holds3d[bank] &&
            HashRegion(memory[bank], banks->offset[bank], banks->size[bank]) != banks->hash[bank]) {
            banks->holds3d[bank] = 0;
        }
    }
}
