// The DS's geometry engine on the host (PLAN.md §4.6). The game drives it as it drives the hardware, by writing
// geometry commands to the command ports (0x04000440-0x040005ff) and packed commands to GXFIFO (0x04000400); the HAL
// logs every such write (port/hal/hw: port_hw_g3_take) and the engine runs them in order. It keeps the matrix stacks,
// lighting and texture coordinate state, assembles, culls and clips polygons, applies the viewport and fills polygon
// RAM, the list the rendering engine draws (port/gfx/ref/ds3d.c), with the hardware's fixed-point arithmetic, so the
// readbacks (GXSTAT, RAM_COUNT, CLIPMTX_RESULT, VECMTX_RESULT, the tests' results) are the values the DS gives.
//
// The facts are GBATEK's ("DS 3D Geometry Commands", "Matrix Load/Multiply", "Matrix Stack", "Polygon Attributes",
// "Texture Coordinates", "Polygon Light Parameters", "Vertices", "Polygon Definitions by Vertices", "Test
// Functions", "Status"). Where GBATEK is silent or unsure, ge.c follows the behaviour melonDS measured on hardware,
// melonDS being the port's oracle (PLAN.md §6), and says so at each place; where neither says, ge.c says what it
// chose. No timing is modelled: every command completes as it arrives, so the engine is never busy and its FIFO
// always reads empty.
//
// Frame protocol: the game's commands run as they are logged (port_ge_run). SWAP_BUFFERS closes the list being built;
// the hardware swaps it in at the next VBlank, and the rendering engine draws it during the frame after that and every
// frame until the next swap. port_ge_vblank() does the swap: the swapped list becomes the render list. Commands after
// a SWAP_BUFFERS go on building the next list at once (the hardware would hold them until the VBlank, with the same
// result), except a second SWAP_BUFFERS before the VBlank, which waits in the FIFO with everything behind it, as on the
// DS. Plain C types only, so that the renderers and tests can include this header without the decomp's.
#ifndef PORT_GFX_GE_GE_H
#define PORT_GFX_GE_GE_H

#include "hal/hw/hw.h"
#include <stddef.h>
#include <stdint.h>

// Polygon RAM (GBATEK "DS 3D Overview"): 2048 polygons and 6144 vertices a frame; a polygon that doesn't fit is
// dropped and sets the overflow flag (DISP3DCNT bit 13)
#define PORT_GE_MAX_POLYGONS 2048
#define PORT_GE_MAX_VERTICES 6144
// A quad clipped by the six planes of the view volume has at most 10 vertices
#define PORT_GE_POLYGON_MAX_VERTICES 10

// One vertex of polygon RAM, after clipping and the viewport transform
typedef struct PortGeVertex {
    int32_t w;         // the clip-space W, 20.12 fixed point, in the 24 bits the hardware keeps (ge.c's ToScreen):
                       // positive inside the view volume; W-buffering's depth
    int32_t z;         // the Z-buffering depth, 0..0xffffff: ((Z * 0x4000 / W) + 0x3fff) * 0x200, clamped (GBATEK)
    uint16_t x;        // screen X, 9 bits (0..256 for a viewport on the screen; the right end is past the last pixel)
    uint16_t y;        // screen Y, 8 bits, 0 at the top (0..192)
    uint16_t color[3]; // red, green, blue: a 5-bit c as c * 16 + 15 (0 stays 0), the 6-bit 2c + 1 with 3 fraction bits
    int16_t s, t;      // the texture coordinates, 1.11.4 fixed point in texels
} PortGeVertex;

// PortGePolygon.flags
#define PORT_GE_POLYGON_FRONT 0x01 // the polygon faces the viewer (counter-clockwise as seen, as GL's front)

typedef struct PortGePolygon {
    uint32_t attr;                                   // POLYGON_ATTR as BEGIN_VTXS latched it
    uint32_t tex_param;                              // TEXIMAGE_PARAM when the polygon's last vertex came
    uint32_t pltt_base;                              // PLTT_BASE (13 bits) at the same time
    uint16_t vertices[PORT_GE_POLYGON_MAX_VERTICES]; // indices into the list's vertices, in the polygon's order
    uint8_t vertex_count;                            // 3..10
    uint8_t flags;
} PortGePolygon;

// One frame's polygon RAM. Strips share vertices between consecutive polygons, as polygon RAM does
typedef struct PortGeList {
    uint32_t polygon_count;
    uint32_t vertex_count;
    // The parameter of the SWAP_BUFFERS that closed the list: bit 0 sorts translucent polygons in the order they came
    // instead of by Y, bit 1 depth-buffers with W instead of Z (GBATEK "SWAP_BUFFERS"). The rendering engine applies
    // them to this list
    uint32_t swap_param;
    PortGePolygon polygons[PORT_GE_MAX_POLYGONS];
    PortGeVertex vertices[PORT_GE_MAX_VERTICES];
} PortGeList;

typedef struct PortGe PortGe;

// An engine in its power-on state, or NULL when memory runs out (about 1 MB: three lists and the command queue)
PortGe *port_ge_create(void);
void port_ge_destroy(PortGe *ge);
// Back to the power-on state: matrices zero and stacks empty, as GBATEK says the matrices start "undefined" and the
// SDK's G3X_Init loads them; empty lists, nothing pending; DISP_1DOT_DEPTH 0x7fff
void port_ge_reset(PortGe *ge);

// Runs logged writes (port/hal/hw/hw.h's PortG3Write): each to GXFIFO (0x04000400-0x0400043f) is a packed command
// word or one of its parameters, each to a command port (0x04000440-0x040005ff) one parameter of that command (a
// command without parameters runs at each write). Writes to undefined commands are ignored
void port_ge_run(PortGe *ge, const PortG3Write *writes, size_t count);
void port_ge_write(PortGe *ge, uint32_t port, uint32_t value);

// DISP_1DOT_DEPTH (0x04000610, 15 bits): polygons that come out as one dot are hidden where all their vertices have
// W >> 9 above it (GBATEK: the register holds W >> 9), unless their POLYGON_ATTR bit 13 is set. It is 0x7fff after a
// reset, which hides none. The HAL copies the register before each run
void port_ge_set_1dot_depth(PortGe *ge, uint16_t depth);

// The VBlank: a list swapped since the last VBlank becomes the render list, polygon RAM starts empty for the next
// one, and commands held behind a second SWAP_BUFFERS run. Returns 1 when a new list was swapped in
int port_ge_vblank(PortGe *ge);
// The list the rendering engine draws this frame (empty until the first swap)
const PortGeList *port_ge_render_list(const PortGe *ge);
// The list being built, for tests and debugging
const PortGeList *port_ge_building_list(const PortGe *ge);
// Whether a SWAP_BUFFERS waits for the VBlank, and how many FIFO entries (command and parameter) wait behind a second
int port_ge_swap_pending(const PortGe *ge);
size_t port_ge_stalled_entries(const PortGe *ge);

// GXSTAT (0x04000600) as read: bit 1 the box test's result, bits 8-12 the position and vector stack's level (the low
// 5 bits of its 6-bit pointer), bit 13 the projection stack's level, bit 15 the stack overflow/underflow error, bits
// 25-26 the FIFO under half full and empty (always: commands complete as they arrive), bits 30-31 the FIFO interrupt
// condition. The busy bits (0, 14, 27) and the FIFO count read 0
uint32_t port_ge_gxstat(const PortGe *ge);
// A write to GXSTAT: bit 15 set acknowledges the stack error, bits 30-31 set the FIFO interrupt condition
void port_ge_write_gxstat(PortGe *ge, uint32_t value);
// Whether the FIFO interrupt condition holds (GXSTAT bits 30-31: 1 under half full, 2 empty); with an empty FIFO
// either holds whenever it is set
int port_ge_fifo_irq(const PortGe *ge);

// RAM_COUNT (0x04000604) and VTXRAM_COUNT (0x04000606): the polygons and vertices in the list being built
uint16_t port_ge_polygon_count(const PortGe *ge);
uint16_t port_ge_vertex_count(const PortGe *ge);
// DISP3DCNT bit 13: polygon or vertex RAM overflowed since the last acknowledgement
int port_ge_ram_overflow(const PortGe *ge);
void port_ge_ack_ram_overflow(PortGe *ge);

// CLIPMTX_RESULT (0x04000640, 16 words): the current clip matrix, the position matrix times the projection matrix,
// row by row. VECMTX_RESULT (0x04000680, 9 words): the 3x3 part of the directional vector matrix, row by row. Both
// 20.12 fixed point
void port_ge_clip_matrix(PortGe *ge, uint32_t out[16]);
void port_ge_vector_matrix(const PortGe *ge, uint32_t out[9]);
// POS_RESULT (0x04000620, 4 words): POS_TEST's vertex times the clip matrix, x, y, z, w in 20.12. VEC_RESULT
// (0x04000630, 3 halfwords): VEC_TEST's vector times the vector matrix, 1.3.12 sign-extended from bit 12
void port_ge_position_result(const PortGe *ge, uint32_t out[4]);
void port_ge_vector_result(const PortGe *ge, uint16_t out[3]);

#endif // PORT_GFX_GE_GE_H
