// The geometry engine and the 3D renderer in the running program (ge_hal.h): one engine fed from the platform layer's
// log of geometry writes (port/hal/hw), the readback registers' values for hw.c's accessors, the swap at each
// VBlank, and the 3D frame rasterized before line 0 for engine A's BG0, unless it would be the last one again. Every 3D
// scene of the game (the title screen, the field, battles, Pokémon sprites) reaches the screens through here. With
// --frame-times, the geometry engine's runs are timed here (port/hal/os/frame_time.h).
#include "gfx/ge/ge_hal.h"
#include "gfx/ge/ge.h"
#include "gfx/ref/ds3d.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include "hal/os/frame_time.h"
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

// What the frame in sFrame was drawn from. The renderer keeps nothing from one frame to the next, so a frame drawn from
// the same list, registers and texture memory is the same picture, with the same buffers behind it
// (port/gfx/ref/ds3d.c's depth and polygon IDs), and isn't drawn again: the hardware draws the last list every frame
// until the next swap, and a still 3D scene (a menu over the title's Kyurem, the bag's) would cost a whole frame's
// drawing each time. The list is named by its pointer and sListChanges, which counts the swaps and resets, the only
// times polygon RAM's lists change (port/gfx/ge/ge.h); the registers and the slots' pages by value; the texture image
// and palette memory by content, against copies made once a list is drawn twice in a row, so that a scene whose list
// changes every frame copies nothing (a still list whose textures change is copied at each change)
static uint32_t sListChanges;
static struct {
    int valid;
    int copied;
    const PortGeList *list;
    uint32_t listChanges;
    PortDs3dRegs regs;
    const uint8_t *texPages[PORT_VRAM_TEX_PAGES];
    const uint8_t *plttPages[PORT_VRAM_TEXPLTT_PAGES];
} sDrawn;
static uint8_t sTexCopy[PORT_VRAM_TEX_PAGES][PORT_VRAM_SLOT_SIZE];
static uint8_t sPlttCopy[PORT_VRAM_TEXPLTT_PAGES][PORT_VRAM_SLOT_SIZE];
static uint32_t sDraws;

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
    sListChanges++;
    sDrawn.valid = 0;
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
    // Timed only when there are commands: the readbacks sync before each read
    uint64_t begin = count != 0 ? port_frame_time_begin() : 0;
    port_ge_run(ge, writes, count);
    port_frame_time_end(PORT_FRAME_TIME_GEOMETRY, begin);
}

// The rendering registers are latched at every VBlank, with the swap when there is one, and the frame after it is
// drawn from them (melonDS's measurement)
void port_g3d_vblank(void) {
    port_g3d_sync();
    if (port_ge_vblank(sGe)) {
        sListChanges++;
    }
    port_ds3d_regs_from_io(&sRegs, port_hw_pointer(REG_IO));
}

// Whether src names what the last frame was drawn from, all but the memory's content
static int SameSource(const PortDs3dSource *src) {
    return sDrawn.valid && src->list == sDrawn.list && sListChanges == sDrawn.listChanges &&
           memcmp(&sRegs, &sDrawn.regs, sizeof(sRegs)) == 0 &&
           memcmp(src->tex_pages, sDrawn.texPages, sizeof(sDrawn.texPages)) == 0 &&
           memcmp(src->texpltt_pages, sDrawn.plttPages, sizeof(sDrawn.plttPages)) == 0;
}

// Whether the slots' memory holds what the copies do; a slot without a page reads 0 and has no copy
static int SameMemory(const PortDs3dSource *src) {
    for (int i = 0; i < PORT_VRAM_TEX_PAGES; i++) {
        if (src->tex_pages[i] != NULL && memcmp(src->tex_pages[i], sTexCopy[i], PORT_VRAM_SLOT_SIZE) != 0) {
            return 0;
        }
    }
    for (int i = 0; i < PORT_VRAM_TEXPLTT_PAGES; i++) {
        if (src->texpltt_pages[i] != NULL && memcmp(src->texpltt_pages[i], sPlttCopy[i], PORT_VRAM_SLOT_SIZE) != 0) {
            return 0;
        }
    }
    return 1;
}

static void CopyMemory(const PortDs3dSource *src) {
    for (int i = 0; i < PORT_VRAM_TEX_PAGES; i++) {
        if (src->tex_pages[i] != NULL) {
            memcpy(sTexCopy[i], src->tex_pages[i], PORT_VRAM_SLOT_SIZE);
        }
    }
    for (int i = 0; i < PORT_VRAM_TEXPLTT_PAGES; i++) {
        if (src->texpltt_pages[i] != NULL) {
            memcpy(sPlttCopy[i], src->texpltt_pages[i], PORT_VRAM_SLOT_SIZE);
        }
    }
}

void port_g3d_render_frame(void) {
    PortDs3dSource src;
    int same;

    memset(&src, 0, sizeof(src));
    src.list = port_ge_render_list(port_g3d_engine());
    src.regs = sRegs;
    for (int i = 0; i < PORT_VRAM_TEX_PAGES; i++) {
        src.tex_pages[i] = port_vram_tex_page(i);
    }
    for (int i = 0; i < PORT_VRAM_TEXPLTT_PAGES; i++) {
        src.texpltt_pages[i] = port_vram_texpltt_page(i);
    }
    same = SameSource(&src);
    if (same && sDrawn.copied && SameMemory(&src)) {
        return;
    }
    port_ds3d_render(&src, sFrame);
    sDraws++;
    // The memory is copied whenever this frame is drawn from the same list, registers and pages as the last one, and
    // checked at the next
    sDrawn.copied = same;
    if (same) {
        CopyMemory(&src);
    }
    sDrawn.valid = 1;
    sDrawn.list = src.list;
    sDrawn.listChanges = sListChanges;
    memcpy(&sDrawn.regs, &sRegs, sizeof(sRegs));
    memcpy(sDrawn.texPages, src.tex_pages, sizeof(sDrawn.texPages));
    memcpy(sDrawn.plttPages, src.texpltt_pages, sizeof(sDrawn.plttPages));
}

uint32_t port_g3d_draws(void) {
    return sDraws;
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
