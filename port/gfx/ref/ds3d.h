// The CPU reference renderer of the DS's 3D rendering engine (PLAN.md §4.6). It draws one frame's polygon list, as the
// geometry engine (port/gfx/ge) left it at SWAP_BUFFERS, into the 256x192 picture the 2D engine A shows as BG0, with
// the rendering engine's rules: the rear plane (clear colour, depth and polygon ID, or the clear image), opaque
// polygons and then translucent ones in the hardware's order, edges and fill rules, Z or W depth buffering with the
// less and equal tests, textures of every format from the texture and palette slots, the polygon modes (modulate,
// decal, toon, highlight, shadow with its stencil), alpha blending and the alpha test, polygon IDs, wireframe, then
// edge marking, fog and anti-aliasing. It is the oracle every later 3D renderer is compared with.
//
// The facts are GBATEK's ("DS 3D Rendering", "DS 3D Polygon Attributes", "DS 3D Texture Formats", "DS 3D Texture
// Attributes", "DS 3D Texture Blending", "DS 3D Toon, Edge, Fog, Alpha-Blending, Anti-Aliasing", "DS 3D Rear-Plane").
// Where GBATEK is silent or unsure, the renderer follows melonDS's measurements of the hardware (the edge walker, the
// fill rules, anti-aliasing coverage and its second layer, among others), and ds3d.c says so; where neither says,
// ds3d.c says what it chose. The whole frame is drawn at once from the registers in src, which the caller latches at
// the VBlank as the hardware does (port/gfx/ge/ge_hal.c).
//
// Nothing here includes the decomp's headers.
#ifndef PORT_GFX_REF_DS3D_H
#define PORT_GFX_REF_DS3D_H

#include "gfx/ge/ge.h"
#include <stdint.h>

#define PORT_DS3D_WIDTH 256
#define PORT_DS3D_HEIGHT 192

// The rendering engine's registers
typedef struct PortDs3dRegs {
    uint16_t disp3dcnt;      // 0x04000060 (bits 12 and 13, the acknowledge flags, are ignored)
    uint16_t edge_color[8];  // 0x04000330, EDGE_COLOR: the edge marking colour of polygon IDs 8n..8n+7
    uint8_t alpha_test_ref;  // 0x04000340, ALPHA_TEST_REF (5 bits)
    uint32_t clear_color;    // 0x04000350, CLEAR_COLOR: colour, fog flag (bit 15), alpha (16-20), polygon ID (24-29)
    uint16_t clear_depth;    // 0x04000354, CLEAR_DEPTH (15 bits)
    uint16_t clear_offset;   // 0x04000356, CLRIMAGE_OFFSET: the clear image's X (bits 0-7) and Y (8-15) scroll
    uint32_t fog_color;      // 0x04000358, FOG_COLOR: colour and alpha (16-20)
    uint16_t fog_offset;     // 0x0400035c, FOG_OFFSET (15 bits)
    uint8_t fog_table[32];   // 0x04000360, FOG_TABLE: densities 0..127
    uint16_t toon_table[32]; // 0x04000380, TOON_TABLE
} PortDs3dRegs;

// Fills regs from the I/O registers' memory as the DS lays it out, from 0x04000000 (at least 0x3c0 bytes)
void port_ds3d_regs_from_io(PortDs3dRegs *regs, const uint8_t *io);

// What one frame is drawn from. Every pointer is the caller's; the renderer only reads through them
typedef struct PortDs3dSource {
    // The polygon list, or NULL for none (only the rear plane)
    const PortGeList *list;
    PortDs3dRegs regs;
    // The texture image space in 32 pages of 16 KiB (port/hal/gx/vram.h's port_vram_tex_page; the clear image's
    // colour is in pages 16-23 and its depth in pages 24-31), and the texture palette space in 6 pages; NULL reads 0
    const uint8_t *tex_pages[32];
    const uint8_t *texpltt_pages[6];
} PortDs3dSource;

// Draws the frame into out, 192 lines of 256 pixels, each 0xAARRGGBB as port/gfx/ref/ds2d.h's bg0_3d_line takes it:
// the rendering engine's 6-bit channels c6 widened to (c6 << 2) | (c6 >> 4), its 5-bit alpha a5 to
// (a5 << 3) | (a5 >> 2), and 0 for a pixel with alpha 0. Display capture's 3D source reads the same pixels: c8 >> 2
// and a8 >> 3 give the engine's own values back
void port_ds3d_render(const PortDs3dSource *src, uint32_t *out);

// After a render, for tests and debugging: a pixel's depth (24 bits, Z or W as the list asked), the polygon ID of its
// opaque polygon (or the clear ID) and whether its fog flag is set
uint32_t port_ds3d_depth(int x, int y);
int port_ds3d_polygon_id(int x, int y);
int port_ds3d_fog_flag(int x, int y);

#endif // PORT_GFX_REF_DS3D_H
