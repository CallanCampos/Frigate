// The CPU reference renderer of the DS's two 2D engines (PLAN.md §4.6). It draws one line of one engine from the
// engine's registers, palettes, OAM and VRAM as the game left them, so the caller can change any of them between lines
// (HBlank effects) and the result stays exact. It is the oracle every later renderer is compared with.
//
// The facts are GBATEK's ("DS Video", "DS Video Stuff", "DS 3D Final 2D Output" and the GBA "LCD" chapters the DS
// keeps). Where GBATEK is silent or unsure, the renderer follows the behaviour melonDS measured on hardware, since
// melonDS is the port's oracle (PLAN.md §6, T4); ds2d.c says so at each place. Nothing here includes the decomp's
// headers.
//
// Frame protocol, per engine:
//
//   port_ds2d_begin_frame(engine);            once per frame, before its first line
//   for (line = 0; line < 192; line++) {
//       (run the game's HBlank work for the line, then fill the source)
//       port_ds2d_render_line(&src, engine, line, out);
//   }
//
// The renderer keeps the state the hardware keeps between lines inside, per engine: the affine BGs' internal
// reference points, the mosaic counters and the windows' on/off state. port_ds2d_begin_frame() makes the next line
// start a frame: it latches BG2X/BG2Y/BG3X/BG3Y into the internal reference points (GBATEK: copied "during each
// vblank"), restarts the mosaic counters and runs the window comparisons of the VBlank lines 192..262 that the renderer
// doesn't see. A line number lower than the last one rendered starts a frame by itself, so a caller that always renders
// 0..191 in order may skip the call. Lines must otherwise come in increasing order; a skipped line advances the state
// as if it had been drawn. The last line rendered may be rendered again (for a capture or a debugger): it is drawn from
// the state it started with, as if the source as it is now had been there the first time, and the frame goes on from
// there. All of this runs on one thread.
#ifndef PORT_GFX_REF_DS2D_H
#define PORT_GFX_REF_DS2D_H

#include <stdint.h>

// What one engine line is drawn from. Every pointer is the caller's; the renderer only reads through them
typedef struct PortDs2dSource {
    // The engine's I/O registers from 0x04000000 (engine A) or 0x04001000 (engine B), 0x80 bytes, plus for engine A
    // the master brightness and DISPCAPCNT at the same offsets as on the DS
    const uint8_t *io;
    // The engine's 1 KiB of palette RAM: BG palette (512 bytes) then OBJ palette (512 bytes)
    const uint8_t *palette;
    // The engine's 1 KiB of OAM
    const uint8_t *oam;
    // The engine's BG VRAM view in 16 KiB slots (engine A 512 KiB, engine B 128 KiB); NULL reads 0
    const uint8_t *bg_slots[32];
    // The OBJ view (engine A 256 KiB, engine B 128 KiB)
    const uint8_t *obj_slots[16];
    // 8 KiB extended BG palette slots, NULL when unmapped
    const uint8_t *bg_ext_palette[4];
    // 8 KiB extended OBJ palette, NULL when unmapped
    const uint8_t *obj_ext_palette;
    // Engine A's VRAM display mode: the LCDC view in 16 KiB slots
    const uint8_t *lcdc_slots[41];
    // Engine A: the 3D layer's pixels for this line (0xAARRGGBB, alpha 0 = transparent), or NULL
    const uint32_t *bg0_3d_line;
} PortDs2dSource;

// Draws line 0..191 of engine 0 (A) or 1 (B) into out as 0xFFRRGGBB. The DS's LCD gets 6 bits per channel; each is
// widened to 8 bits as (c6 << 2) | (c6 >> 4), so 0 and 63 become 0x00 and 0xFF. 15-bit colours enter the 2D engine as
// c6 = c5 * 2 for red and blue and c5 * 2 + bit 15 for green (ds2d.c), so the 15-bit white 0x7FFF shows as 0xFFFBFBFB.
//
// The 3D layer's pixels (bg0_3d_line) are 6-bit colours widened the same way: the renderer reads c8 >> 2 back. Its
// alpha is the 3D engine's 5-bit alpha a5, read back as a8 >> 3, so the 3D renderer should store (a5 << 3) | (a5 >> 2)
// (or any widening whose top 5 bits are a5); a8 = 0 is transparent.
//
// Not drawn: display capture (port_ds2d_capture_source_a() is the hook) and the main memory display FIFO (display mode
// 3 shows black). The OBJ unit draws a line during the line before it on the DS, so OAM and OBJ registers written in
// the HBlank before line N reach line N here but not on hardware; the caller decides which state the source shows.
void port_ds2d_render_line(const PortDs2dSource *src, int engine, int line, uint32_t out[256]);

// Makes the engine's next line start a new frame (see above)
void port_ds2d_begin_frame(int engine);

// Puts the engine in its power-on state: no window active, mosaic counters and reference points zero, and the next
// line starting a frame. Tests call it to begin from a known state
void port_ds2d_reset(int engine);

// The game wrote BGxX (axis 0) or BGxY (axis 1) of BG2 or BG3 (bg 2 or 3) during the frame: on the DS that copies the
// value into the internal reference point at once, so the next line starts from it instead of advancing by PB or PD.
// The renderer notices a write by itself when the register's value changed since the line before; a write of the same
// value again, which also restarts the line from it, is only seen through this call
void port_ds2d_reload_reference(int engine, int bg, int axis);

// The per-line OBJ budget. GBATEK gives the GBA's cost model (an OBJ n dots wide costs n cycles, an affine OBJ
// 10 + 2n with n its double-size width, counted in OAM order for every OBJ on the line, off-screen ones too) and the
// GBA's budget, but no figures for the DS, and melonDS draws every OBJ. So the default is 0, no limit; a positive
// budget applies the GBA model with that many cycles, or cycles_hblank_free when DISPCNT bit 23 (OBJ processing during
// H-Blank, the GBA's "H-Blank Interval Free") is set. An OBJ whose cost doesn't fit is dropped whole (the hardware
// would cut it where the budget runs out), and so is every later one
void port_ds2d_set_obj_cycle_budget(int cycles, int cycles_hblank_free);

// Display capture's source A ("Graphics", BG+3D+OBJ after the colour effects, before master brightness) for the line
// last rendered on engine A, or NULL when that line didn't compose it: it is composed in display mode 1, or in any
// mode while DISPCAPCNT bit 31 is set. Each pixel has 6-bit channels, red in bits 16-21, green in bits 8-13 and blue
// in bits 0-5. Display capture itself (DISPCAPCNT, the VRAM writes) is port/gfx/capture.c, which reads it here
const uint32_t *port_ds2d_capture_source_a(void);

#endif
