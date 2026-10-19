// The geometry engine (ge.h) and the 3D renderer (port/gfx/ref/ds3d.h) in the running program: one engine fed from
// the HAL's log of geometry writes (port/hal/hw: port_hw_g3_take), the memory the readback registers' accessors
// return, the VBlank's swap, and the 3D picture engine A shows as BG0.
//
// How the platform layer uses it:
//   - port/hal/hw/hw.c's accessors for GXSTAT, RAM_COUNT, VTXRAM_COUNT, CLIPMTX_RESULT and VECMTX_RESULT return
//     port_g3d_gxstat() and the others below. Each runs the commands logged so far first, so a readback after a
//     command sees its effect, as on the DS once the engine isn't busy (it never is here).
//   - The frame dispatcher calls port_g3d_vblank() at each VBlank (a frame hook): the list a SWAP_BUFFERS closed
//     becomes the one drawn, and the rendering registers are latched.
//   - The screens call port_g3d_render_frame() before line 0 of each frame and pass port_g3d_line(line) as engine A's
//     bg0_3d_line. The hardware draws the 3D picture during the frame from the list swapped and the registers latched
//     at the VBlank before it, and keeps drawing that list until the next swap; here a frame whose inputs are all the
//     last one's keeps the last picture, which is the same.
//   - port_g3d_reset() runs once the I/O registers' memory is set up: it writes DISP_1DOT_DEPTH's power-on value there.
// Plain C types only.
#ifndef PORT_GFX_GE_GE_HAL_H
#define PORT_GFX_GE_GE_HAL_H

#include <stdint.h>

struct PortGe;

// The program's engine (made at the first use), and a power-on reset of it, of the picture (transparent) and of the
// latched rendering registers (all 0), which also writes DISP_1DOT_DEPTH's power-on value, 0x7fff, to 0x04000610
struct PortGe *port_g3d_engine(void);
void port_g3d_reset(void);

// Runs the geometry writes logged since the last call, after copying DISP_1DOT_DEPTH (0x04000610) into the engine
void port_g3d_sync(void);
// The VBlank: runs what is logged, swaps in a list SWAP_BUFFERS closed, and latches the rendering registers
// (0x04000060, 0x04000330-0x040003bf) for the next frame
void port_g3d_vblank(void);

// Draws the 3D picture from the swapped-in list, the rendering registers the last VBlank latched and the texture and
// palette slots as they are now. When the list, the registers, the slots' pages and their memory are all what the last
// picture was drawn from, the picture and the renderer's buffers are already what drawing would make, and it isn't
// drawn again; port_g3d_draws counts the pictures drawn, for tests
void port_g3d_render_frame(void);
uint32_t port_g3d_draws(void);
// Line 0..191 of the picture (256 pixels, 0xAARRGGBB with alpha 0 transparent), or NULL for another line; and the
// whole picture, line after line
const uint32_t *port_g3d_line(int line);
const uint32_t *port_g3d_frame(void);

// Whether the geometry FIFO's interrupt condition (GXSTAT bits 30-31) holds; the FIFO always reads empty, so it holds
// whenever a condition is set. The GXFIFO interrupt (IE bit 21) is level-triggered on the DS
int port_g3d_fifo_irq(void);

// The memory one access to a readback register uses, for port/hal/hw's accessors:
//   GXSTAT (0x04000600, 32 bits): read and written. A write is applied at the next access to any of these or at the
//   next sync, when the word differs from what was read (bit 15 set acknowledges the stack error, bits 30-31 set the
//   FIFO interrupt condition); a write of exactly the value read is not seen, which only matters to a write that
//   acknowledges an error already set, and no code in the decomp reads that flag.
//   RAM_COUNT (0x04000604) and VTXRAM_COUNT (0x04000606), 16 bits; CLIPMTX_RESULT (0x04000640, 16 words);
//   VECMTX_RESULT (0x04000680, 9 words); POS_RESULT (0x04000620, 4 words); VEC_RESULT (0x04000630, 3 halfwords).
//   DISP3DCNT (0x04000060, 16 bits), if it is ever routed: the register's own memory, with bit 13 set while polygon
//   RAM has overflowed; a write with bit 13 set acknowledges it (with the same blind spot as GXSTAT)
void *port_g3d_gxstat(void);
void *port_g3d_ram_count(void);
void *port_g3d_vtxram_count(void);
void *port_g3d_clipmtx_result(void);
void *port_g3d_vecmtx_result(void);
void *port_g3d_pos_result(void);
void *port_g3d_vec_result(void);
void *port_g3d_disp3dcnt(void);

#endif // PORT_GFX_GE_GE_HAL_H
