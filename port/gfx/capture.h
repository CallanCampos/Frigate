// Display capture (GBATEK "DS Video Display Capture"): engine A's DISPCAPCNT (0x04000064) writes, as each line is
// drawn, the picture engine A composes, the 3D layer, a VRAM bank or a blend of two of them into VRAM bank A to D while
// the bank is in LCDC. The game uses it for motion blur and to show 3D on both screens; the title screen draws its
// scene into VRAM D this way and shows VRAM D. Where GBATEK is silent, this follows melonDS, the port's oracle
// (PLAN.md §6, T4): the rounding of the blend, the alpha of each source, and the read offset that VRAM display mode
// ignores. Plain C types only.
#ifndef PORT_GFX_CAPTURE_H
#define PORT_GFX_CAPTURE_H

#include <stdint.h>

struct PortDs2dSource;

// The halfwords of a 128 KiB bank (A to D) and its 16 KiB pages
#define PORT_CAPTURE_BANK_HALFWORDS 0x10000u
#define PORT_CAPTURE_BANK_PAGES 8

// Writes line 0-191 of a capture into the destination bank, from DISPCAPCNT and DISPCNT as engine A's line saw them.
// source_a is the composed picture as port_ds2d_capture_source_a() gives it (6-bit channels, red in bits 16-21), or
// NULL when the line didn't compose one; line_3d the 3D layer's pixels (0xAARRGGBB, ds2d.h) or NULL, which is
// transparent. dst and src_b are the eight 16 KiB pages of the destination bank and of source B's bank, all NULL for a
// bank that isn't in LCDC: then nothing is written, or source B reads as missing. Returns the number of halfwords
// written from the halfword DISPCAPCNT's write offset and the line give (0 below the capture's height), for the
// caller's note of the write; they may wrap past the bank's end to its start
unsigned port_capture_line(uint32_t dispcapcnt, uint32_t dispcnt, int line, const uint32_t *source_a,
                           const uint32_t *line_3d, uint8_t *const dst[PORT_CAPTURE_BANK_PAGES],
                           const uint8_t *const src_b[PORT_CAPTURE_BANK_PAGES]);

// The capture of a frame, for the screens' line drawing (port/gfx/screens.c), right after engine A's line is drawn
// from engine_a: line 0 latches DISPCAPCNT's enable bit, each line of the capture's height is written into the bank,
// and the end of line 191 clears the enable bit, as the hardware does at the VBlank
void port_capture_draw_line(const struct PortDs2dSource *engine_a, int line);

// Forgets a capture in progress (a new run)
void port_capture_reset(void);

// What the capture unit had at line 96 of the frame being drawn: the line where pbw-oracle looks at melonDS's unit
// (port/tools/oracle/oracle.cpp, PROBE_LINE), between the latch at line 0 and the end at the VBlank. The digest stream
// (port/app/digest_stream.c) takes it at the VBlank for its capture group and for the tracking of the 3D that
// captures leave in VRAM (port/tools/oracle/display3d.h), as the oracle does. It is taken whether or not the unit
// captures in the frame, and is all zeros from line 0 until line 96 is drawn
typedef struct PortCaptureRecord {
    int ran;             // DISPCAPCNT's enable was latched at line 0, so the unit writes in this frame
    uint32_t dispcapcnt; // DISPCAPCNT as the I/O registers held it at line 96
    uint32_t dispcnt_a;  // engine A's DISPCNT, the same
    int dest_in_lcdc;    // the destination bank's pages were in the LCDC, which the unit writes only then
} PortCaptureRecord;

// The frame's record so far
const PortCaptureRecord *port_capture_record(void);

#endif // PORT_GFX_CAPTURE_H
