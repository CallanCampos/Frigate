// Which screens show the 3D engine's output in a frame, for the oracle's frames.tsv (README.md, "Run directories"),
// from the display registers GBATEK documents. The 3D picture reaches a screen two ways:
// - as engine A's BG0, in graphics display mode with BG0 shown as the 3D layer;
// - through the capture unit, which writes engine A's output (source A: BG+3D+OBJ, or the 3D picture alone), a VRAM
//   bank (source B) or both blended into one of the banks A-D. A bank that holds such a picture shows 3D wherever it is
//   displayed: by engine A in VRAM display mode (the title screen, src/ov162/title.c), or as a bitmap BG or bitmap OBJs
//   of either engine (dual-screen 3D, src/gfl/double3Ddisp.c).
// A bank holds 3D from a capture of 3D into it until a capture without 3D covers it, or until its captured region
// changes otherwise (the CPU or a DMA writing it), which a hash of the region detects. Plain C11 without melonDS, so
// that the tests can run it on made-up registers.
#ifndef PORT_TOOLS_ORACLE_DISPLAY3D_H
#define PORT_TOOLS_ORACLE_DISPLAY3D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The banks the capture unit can write: VRAM A-D, 128 KiB each
#define PBW_DISPLAY_BANKS 4
#define PBW_DISPLAY_BANK_SIZE 0x20000u

// The screens, as bits of pbw_display3d_screens's result
#define PBW_DISPLAY_TOP 1u
#define PBW_DISPLAY_BOTTOM 2u

// The registers that decide what a frame shows, as they were while it was drawn
typedef struct PbwDisplayRegs {
    uint32_t dispcnt[2];                // DISPCNT of engine A, then engine B
    uint16_t bgcnt[2][4];               // BG0CNT-BG3CNT of each engine
    uint16_t powcnt1;                   // the ARM9's POWCNT1
    uint8_t vramcnt[PBW_DISPLAY_BANKS]; // VRAMCNT_A-D
} PbwDisplayRegs;

// The capture unit in a frame
typedef struct PbwCapture {
    int ran;             // DISPCAPCNT's enable was latched at line 0, so the unit wrote during the frame
    uint32_t dispcapcnt; // DISPCAPCNT then
    uint32_t dispcntA;   // engine A's DISPCNT then: source A is its output, source B the bank it names
    int destInLcdc;      // the destination bank was mapped to the LCDC, which the unit writes only then
} PbwCapture;

// What the banks hold: whether a picture of 3D, and the region and hash of the capture that put it there
typedef struct PbwBanks3d {
    uint8_t holds3d[PBW_DISPLAY_BANKS];
    uint32_t offset[PBW_DISPLAY_BANKS];
    uint32_t size[PBW_DISPLAY_BANKS];
    uint64_t hash[PBW_DISPLAY_BANKS];
} PbwBanks3d;

// No bank holds 3D, as at power-on
void pbw_display3d_init(PbwBanks3d *banks);

// The screens (PBW_DISPLAY_TOP, PBW_DISPLAY_BOTTOM) that show 3D in a frame with these registers, given which banks
// held 3D when the frame started (a capture into a bank shown in the same frame writes each line after it is shown)
unsigned pbw_display3d_screens(const PbwDisplayRegs *regs, const uint8_t holds3d[PBW_DISPLAY_BANKS]);

// Records a frame's capture, once the frame is drawn: holds3dAtStart is what the banks held when the frame started
// (source B reads a bank as it was), memory the banks' bytes, for the hash
void pbw_display3d_capture(PbwBanks3d *banks, const PbwCapture *capture,
                           const uint8_t holds3dAtStart[PBW_DISPLAY_BANKS],
                           const uint8_t *const memory[PBW_DISPLAY_BANKS]);

// Before a frame: a bank whose captured region no longer has the bytes the capture wrote no longer holds 3D
void pbw_display3d_check(PbwBanks3d *banks, const uint8_t *const memory[PBW_DISPLAY_BANKS]);

#ifdef __cplusplus
}
#endif

#endif // PORT_TOOLS_ORACLE_DISPLAY3D_H
