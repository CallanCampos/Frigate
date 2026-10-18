// The geometry engine and the 3D renderer in the running program (ge_hal.h): one engine fed from the platform layer's
// log of geometry writes (port/hal/hw), the readback registers' values for hw.c's accessors, the swap at each
// VBlank, and the 3D frame rasterized before line 0 for engine A's BG0. Every 3D scene of the game (the title screen,
// the field, battles, Pokémon sprites) reaches the screens through here.
#include "gfx/ge/ge_hal.h"
#include "gfx/ge/ge.h"
#include "gfx/ref/ds3d.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_IO 0x04000000u
#define REG_DISP3DCNT 0x04000060u
#define REG_DISP_1DOT_DEPTH 0x04000610u
#define DISP3DCNT_UNDERFLOW 0x1000u
#define DISP3DCNT_RAM_OVERFLOW 0x2000u

static struct PortGe *sGe;
static uint32_t sFrame[PORT_DS3D_HEIGHT * PORT_DS3D_WIDTH];
// The rendering registers as the last VBlank latched them
static PortDs3dRegs sRegs;

// What the accessors hand out. GXSTAT's word is compared at the next access with what it was given, to see a write
static uint32_t sGxStat;
static uint32_t sGxStatGiven;
static uint16_t sRamCount;
static uint16_t sVtxRamCount;
static uint32_t sClipMtx[16];
static uint32_t sVecMtx[9];
static uint32_t sPosResult[4];
static uint16_t sVecResult[4];
static uint16_t sDisp3dcntGiven;
static int sDisp3dcntRouted;

struct PortGe *port_g3d_engine(void) {
    if (sGe == NULL) {
        sGe = port_ge_create();
        if (sGe == NULL) {
            fprintf(stderr, "port: out of memory for the geometry engine\n");
            exit(1);
        }
    }
    return sGe;
}

static uint16_t Read16(uint32_t address) {
    uint16_t value;
    memcpy(&value, port_hw_pointer(address), sizeof(value));
    return value;
}

static void Write16(uint32_t address, uint16_t value) {
    memcpy(port_hw_pointer(address), &value, sizeof(value));
}

void port_g3d_reset(void) {
    port_ge_reset(port_g3d_engine());
    memset(sFrame, 0, sizeof(sFrame));
    memset(&sRegs, 0, sizeof(sRegs));
    sGxStat = sGxStatGiven = port_ge_gxstat(sGe);
    sDisp3dcntRouted = 0;
    // DISP_1DOT_DEPTH's power-on value is the engine's, which hides no dot (ge.h); every sync copies the register
    Write16(REG_DISP_1DOT_DEPTH, 0x7fff);
}

static void ApplyGxStatWrite(void) {
    if (sGxStat != sGxStatGiven) {
        port_ge_write_gxstat(port_g3d_engine(), sGxStat);
        sGxStat = sGxStatGiven = port_ge_gxstat(sGe);
    }
}

void port_g3d_sync(void) {
    struct PortGe *ge = port_g3d_engine();
    size_t count;
    const PortG3Write *writes = port_hw_g3_take(&count);
    ApplyGxStatWrite();
    port_ge_set_1dot_depth(ge, Read16(REG_DISP_1DOT_DEPTH));
    port_ge_run(ge, writes, count);
}

// The rendering registers are latched at every VBlank, with the swap when there is one, and the frame after it is
// drawn from them (melonDS's measurement)
void port_g3d_vblank(void) {
    port_g3d_sync();
    port_ge_vblank(sGe);
    port_ds3d_regs_from_io(&sRegs, port_hw_pointer(REG_IO));
}

void port_g3d_render_frame(void) {
    PortDs3dSource src;
    memset(&src, 0, sizeof(src));
    src.list = port_ge_render_list(port_g3d_engine());
    src.regs = sRegs;
    for (int i = 0; i < PORT_VRAM_TEX_PAGES; i++) {
        src.tex_pages[i] = port_vram_tex_page(i);
    }
    for (int i = 0; i < PORT_VRAM_TEXPLTT_PAGES; i++) {
        src.texpltt_pages[i] = port_vram_texpltt_page(i);
    }
    port_ds3d_render(&src, sFrame);
}

const uint32_t *port_g3d_line(int line) {
    if (line < 0 || line >= PORT_DS3D_HEIGHT) {
        return NULL;
    }
    return &sFrame[line * PORT_DS3D_WIDTH];
}

const uint32_t *port_g3d_frame(void) {
    return sFrame;
}

int port_g3d_fifo_irq(void) {
    ApplyGxStatWrite();
    return port_ge_fifo_irq(port_g3d_engine());
}

void *port_g3d_gxstat(void) {
    port_g3d_sync();
    sGxStat = sGxStatGiven = port_ge_gxstat(sGe);
    return &sGxStat;
}

void *port_g3d_ram_count(void) {
    port_g3d_sync();
    sRamCount = port_ge_polygon_count(sGe);
    return &sRamCount;
}

void *port_g3d_vtxram_count(void) {
    port_g3d_sync();
    sVtxRamCount = port_ge_vertex_count(sGe);
    return &sVtxRamCount;
}

void *port_g3d_clipmtx_result(void) {
    port_g3d_sync();
    port_ge_clip_matrix(sGe, sClipMtx);
    return sClipMtx;
}

void *port_g3d_vecmtx_result(void) {
    port_g3d_sync();
    port_ge_vector_matrix(sGe, sVecMtx);
    return sVecMtx;
}

void *port_g3d_pos_result(void) {
    port_g3d_sync();
    port_ge_position_result(sGe, sPosResult);
    return sPosResult;
}

void *port_g3d_vec_result(void) {
    port_g3d_sync();
    port_ge_vector_result(sGe, sVecResult);
    sVecResult[3] = 0;
    return sVecResult;
}

void *port_g3d_disp3dcnt(void) {
    void *reg = port_hw_pointer(REG_DISP3DCNT);
    uint16_t value;
    port_g3d_sync();
    memcpy(&value, reg, sizeof(value));
    if (sDisp3dcntRouted && value != sDisp3dcntGiven && (value & DISP3DCNT_RAM_OVERFLOW)) {
        port_ge_ack_ram_overflow(sGe);
    }
    value &= (uint16_t)~(DISP3DCNT_UNDERFLOW | DISP3DCNT_RAM_OVERFLOW);
    if (port_ge_ram_overflow(sGe)) {
        value |= DISP3DCNT_RAM_OVERFLOW;
    }
    memcpy(reg, &value, sizeof(value));
    sDisp3dcntGiven = value;
    sDisp3dcntRouted = 1;
    return reg;
}
