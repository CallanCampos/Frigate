// Tests of port/gfx/ge/ge_hal.c, the geometry engine and 3D renderer in the program, against fakes of the three
// platform-layer services it uses: the geometry write log (port_hw_g3_take), the I/O registers' memory
// (port_hw_pointer) and the texture slots (port_vram_tex_page, port_vram_texpltt_page). Each test logs writes as the
// game's register stores would, then reads the registers through the accessors port/hal/hw/hw.c forwards to.
#include "gfx/ge/ge.h"
#include "gfx/ge/ge_hal.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sChecks;
static int sFailures;

#define CHECK_HEX(got, want) CheckHex(__LINE__, #got, (uint32_t)(got), (uint32_t)(want))

static void CheckHex(int line, const char *what, uint32_t got, uint32_t want) {
    sChecks++;
    if (got != want) {
        sFailures++;
        printf("line %d: %s is 0x%08x, want 0x%08x\n", line, what, got, want);
    }
}

// --- The fakes

static PortG3Write sLog[4096];
static size_t sLogCount;
static PortG3Write sTaken[4096];
static uint8_t sIo[0x1000];
static uint8_t sTex[PORT_VRAM_TEX_PAGES][PORT_VRAM_SLOT_SIZE];
static uint8_t sPltt[PORT_VRAM_TEXPLTT_PAGES][PORT_VRAM_SLOT_SIZE];

const PortG3Write *port_hw_g3_take(size_t *count) {
    memcpy(sTaken, sLog, sLogCount * sizeof(*sLog));
    *count = sLogCount;
    sLogCount = 0;
    return sTaken;
}

void *port_hw_pointer(uint32_t address) {
    if (address < 0x04000000 || address >= 0x04000000 + sizeof(sIo)) {
        printf("unexpected address 0x%08x\n", address);
        exit(1);
    }
    return &sIo[address - 0x04000000];
}

const uint8_t *port_vram_tex_page(int page) {
    return sTex[page];
}

const uint8_t *port_vram_texpltt_page(int page) {
    return sPltt[page];
}

// --- Helpers

static void Log(uint32_t cmd, uint32_t value) {
    sLog[sLogCount].port = 0x04000400 + 4 * cmd;
    sLog[sLogCount].value = value;
    sLogCount++;
}

static void Io16(uint32_t offset, uint16_t value) {
    memcpy(&sIo[offset], &value, sizeof(value));
}

static void Io32(uint32_t offset, uint32_t value) {
    memcpy(&sIo[offset], &value, sizeof(value));
}

static uint32_t Read32(void *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static uint16_t Read16(void *p) {
    uint16_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void Write32(void *p, uint32_t value) {
    memcpy(p, &value, sizeof(value));
}

static void Write16(void *p, uint16_t value) {
    memcpy(p, &value, sizeof(value));
}

// Identity matrices, the whole screen, a polygon attribute
static void LogBasic(uint32_t attr) {
    Log(0x10, 0);
    Log(0x15, 0);
    Log(0x10, 2);
    Log(0x15, 0);
    Log(0x60, 0xbfff0000);
    Log(0x29, attr);
    Log(0x40, 1);
}

// A quad over the screen pixels [x0, x1) x [y0, y1) (port/tests/g3d/test_ds3d.c has the arithmetic)
static void LogQuad(int x0, int y0, int x1, int y1) {
    int32_t xs[2] = { x0 * 32 - 0x1000, x1 * 32 - 0x1000 };
    int32_t ys[2] = { 0x1000 - (y0 * 128 + 2) / 3, 0x1000 - (y1 * 128 + 2) / 3 };
    static const int kCorners[4][2] = { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } };
    for (int i = 0; i < 4; i++) {
        Log(0x23, ((uint32_t)xs[kCorners[i][0]] & 0xffff) | (uint32_t)ys[kCorners[i][1]] << 16);
        Log(0x23, 0);
    }
}

// The readbacks run what was logged first, so they see every command before them
static void TestReadbacks(void) {
    port_g3d_reset();
    LogBasic(0x001f00c0);
    Log(0x11, 0);      // MTX_PUSH in mode 2
    Log(0x1b, 0x2000); // MTX_SCALE by 2
    Log(0x1b, 0x2000);
    Log(0x1b, 0x2000);
    CHECK_HEX((Read32(port_g3d_gxstat()) >> 8) & 31, 1);
    uint32_t *clip = port_g3d_clipmtx_result();
    CHECK_HEX(clip[0], 0x2000);
    CHECK_HEX(clip[5], 0x2000);
    CHECK_HEX(clip[15], 0x1000);
    uint32_t *vec = port_g3d_vecmtx_result();
    CHECK_HEX(vec[0], 0x1000); // MTX_SCALE leaves the vector matrix
    CHECK_HEX(vec[8], 0x1000);
    Log(0x15, 0);
    LogQuad(10, 10, 20, 20);
    CHECK_HEX(Read16(port_g3d_ram_count()), 1);
    CHECK_HEX(Read16(port_g3d_vtxram_count()), 4);
    Log(0x71, 0x0800); // POS_TEST (0.5, 0, 0)
    Log(0x71, 0);
    uint32_t *pos = port_g3d_pos_result();
    CHECK_HEX(pos[0], 0x800);
    CHECK_HEX(pos[3], 0x1000);
    Log(0x72, 0x100); // VEC_TEST (0.5, 0, 0)
    uint16_t *v = port_g3d_vec_result();
    CHECK_HEX(v[0], 0x800);
    CHECK_HEX(v[1], 0);
}

// GXSTAT is read and written through the word the accessor gives: a store the game makes to it is applied at the next
// access (or sync)
static void TestGxstatWrites(void) {
    port_g3d_reset();
    void *gxstat = port_g3d_gxstat();
    CHECK_HEX(Read32(gxstat), 0x06000000);
    CHECK_HEX(port_g3d_fifo_irq(), 0);
    // G3X_SetFifoIntrCond(GX_FIFOINTR_COND_EMPTY): a read, then a store of the new word
    gxstat = port_g3d_gxstat();
    Write32(port_g3d_gxstat(), (Read32(gxstat) & ~0xc0000000u) | 2u << 30);
    CHECK_HEX(port_g3d_fifo_irq(), 1);
    CHECK_HEX(Read32(port_g3d_gxstat()), 0x86000000);
    // A stack error, acknowledged by a store with bit 15 set (here the value read differs, so the store is seen)
    Log(0x10, 1);
    Log(0x12, 1); // MTX_POP of an empty stack
    gxstat = port_g3d_gxstat();
    CHECK_HEX(Read32(gxstat) & 0x8000, 0x8000);
    Write32(gxstat, 0x8000 | 2u << 30);
    CHECK_HEX(Read32(port_g3d_gxstat()) & 0x8000, 0);
}

// DISP_1DOT_DEPTH is copied from the register before each sync: a dot beyond it is hidden. The reset writes its
// power-on value, 0x7fff, which hides none
static void TestOneDotDepth(void) {
    Io16(0x610, 0);
    port_g3d_reset();
    CHECK_HEX(Read16(&sIo[0x610]), 0x7fff);
    LogBasic(0x001f00c0);
    Log(0x40, 0);
    Log(0x23, 0);
    Log(0x23, 0);
    Log(0x23, 2 | (uint32_t)(-1 & 0xffff) << 16);
    Log(0x23, 0);
    Log(0x23, 1 | (uint32_t)(-2 & 0xffff) << 16);
    Log(0x23, 0);
    CHECK_HEX(Read16(port_g3d_ram_count()), 1);
    port_g3d_reset();
    Io16(0x610, 4); // up to W 0x9ff
    LogBasic(0x001f00c0);
    Log(0x40, 0);
    Log(0x23, 0);
    Log(0x23, 0);
    Log(0x23, 2 | (uint32_t)(-1 & 0xffff) << 16);
    Log(0x23, 0);
    Log(0x23, 1 | (uint32_t)(-2 & 0xffff) << 16);
    Log(0x23, 0);
    CHECK_HEX(Read16(port_g3d_ram_count()), 0);
    Io16(0x610, 0x7fff);
    Log(0x23, 0);
    Log(0x23, 0);
    Log(0x23, 2 | (uint32_t)(-1 & 0xffff) << 16);
    Log(0x23, 0);
    Log(0x23, 1 | (uint32_t)(-2 & 0xffff) << 16);
    Log(0x23, 0);
    CHECK_HEX(Read16(port_g3d_ram_count()), 1);
    Io16(0x610, 0);
}

// The frame: commands, SWAP_BUFFERS, the VBlank, then the picture drawn from the registers the VBlank latched and the
// texture slots. Until the next swap the same list is drawn every frame
static void TestFrame(void) {
    port_g3d_reset();
    Io32(0x350, 0x1fu << 10 | 31u << 16); // clear: blue, opaque
    Io16(0x354, 0x7fff);
    Io16(0x60, 1); // textures on
    // Nothing latched yet: the registers are all 0, a transparent rear plane
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[12], 0);
    port_g3d_vblank();
    // A 4x4 direct texture at 0 in texture slot 0, every texel red; the quad maps it 1:1 onto (10, 10)
    for (int i = 0; i < 16; i++) {
        sTex[0][i * 2] = 0x1f;
        sTex[0][i * 2 + 1] = 0x80;
    }
    LogBasic(0x001f00c0);
    Log(0x20, 0x7fff);        // white vertices
    Log(0x2a, 7u << 26 | 0u); // TEXIMAGE_PARAM: direct, 8x8 at 0
    LogQuad(10, 10, 14, 14);
    Log(0x50, 0);
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[12], 0xff0000ff); // not swapped in yet: the rear plane
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[12], 0xffff0000);
    CHECK_HEX(port_g3d_line(12)[9], 0xff0000ff);
    CHECK_HEX(port_g3d_frame()[12 * 256 + 12], 0xffff0000);
    CHECK_HEX(port_g3d_line(192) == NULL, 1);
    // A register written after the VBlank waits for the next one: the clear colour turns green only then
    Io32(0x350, 0x1fu << 5 | 31u << 16);
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[9], 0xff0000ff);
    // The next frame draws the same list
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[12], 0xffff0000);
    CHECK_HEX(port_g3d_line(12)[9], 0xff00ff00);
    Io32(0x350, 0x1fu << 10 | 31u << 16);
    port_g3d_vblank();
    // A new frame's commands go to the next list; the RAM counts are that list's
    LogQuad(20, 20, 24, 24);
    LogQuad(30, 30, 34, 34);
    CHECK_HEX(Read16(port_g3d_ram_count()), 2);
    Log(0x50, 0);
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_line(12)[12], 0xff0000ff);
    CHECK_HEX(port_g3d_line(22)[22], 0xffff0000);
    CHECK_HEX(Read16(port_g3d_ram_count()), 0);
    memset(sTex[0], 0, 32);
    Io16(0x60, 0);
    Io32(0x350, 0);
}

// A frame drawn from what the last one was drawn from keeps the last picture: the same list (no swap or reset since),
// latched registers, slots' pages and texture memory. The memory is copied whenever a frame is drawn from the same
// list, registers and pages as the one before, and compared at the next, so a still scene is drawn twice, then kept
// until something it is drawn from changes; a change of the memory alone is drawn once
static void TestKeep(void) {
    uint32_t draws;

    port_g3d_reset();
    Io32(0x350, 0x1fu << 10 | 31u << 16); // clear: blue, opaque
    Io16(0x354, 0x7fff);
    Io16(0x60, 1); // textures on
    for (int i = 0; i < 16; i++) {
        sTex[0][i * 2] = 0x1f; // red
        sTex[0][i * 2 + 1] = 0x80;
    }
    LogBasic(0x001f00c0);
    Log(0x20, 0x7fff);
    Log(0x2a, 7u << 26 | 0u);
    LogQuad(10, 10, 14, 14);
    Log(0x50, 0);
    port_g3d_vblank();
    draws = port_g3d_draws();
    port_g3d_render_frame();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 2);
    for (int i = 0; i < 3; i++) {
        port_g3d_render_frame();
    }
    CHECK_HEX(port_g3d_draws() - draws, 2);
    CHECK_HEX(port_g3d_line(12)[12], 0xffff0000);
    // A texel changed (the quad has no texture coordinates, so every pixel shows texel 0): drawn again, with it
    sTex[0][0] = 0x00;
    sTex[0][1] = 0xfc; // blue
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 3);
    CHECK_HEX(port_g3d_line(12)[12], 0xff0000ff);
    port_g3d_render_frame();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 3);
    // A palette slot's memory counts too, though this texture reads none
    sPltt[5][0x3fff] = 1;
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 4);
    sPltt[5][0x3fff] = 0;
    port_g3d_render_frame();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 5);
    // A register changes the picture only once a VBlank latches it
    Io32(0x350, 0x1fu << 5 | 31u << 16); // green
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 5);
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 6);
    CHECK_HEX(port_g3d_line(12)[9], 0xff00ff00);
    port_g3d_render_frame();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 7);
    // A VBlank without a swap or a change keeps the picture; a swap of the same commands is another list, drawn
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 7);
    LogBasic(0x001f00c0);
    Log(0x20, 0x7fff);
    Log(0x2a, 7u << 26 | 0u);
    LogQuad(10, 10, 14, 14);
    Log(0x50, 0);
    port_g3d_vblank();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 8);
    // A reset empties the lists and the picture: drawn again, the rear plane alone
    port_g3d_reset();
    port_g3d_render_frame();
    CHECK_HEX(port_g3d_draws() - draws, 9);
    CHECK_HEX(port_g3d_line(12)[12], 0);
    memset(sTex[0], 0, 32);
    Io16(0x60, 0);
    Io32(0x350, 0);
}

// DISP3DCNT through its accessor, if the generator ever routes it: bit 13 shows polygon RAM's overflow, and a store
// with bit 13 set acknowledges it
static void TestDisp3dcnt(void) {
    port_g3d_reset();
    Io16(0x60, 0x3000 | 0x0008); // as G3X_Init leaves it: the acknowledge bits set, blending on
    CHECK_HEX(Read16(port_g3d_disp3dcnt()), 0x0008);
    LogBasic(0x001f00c0);
    for (int i = 0; i < 2049; i++) {
        LogQuad(0, 0, 2, 2);
        if (sLogCount > 4000) {
            port_g3d_sync();
        }
    }
    void *reg = port_g3d_disp3dcnt();
    CHECK_HEX(Read16(reg), 0x2008);
    Write16(reg, (uint16_t)(Read16(reg) & ~0x3000u)); // a store that keeps the flags clear: no acknowledgement
    CHECK_HEX(Read16(port_g3d_disp3dcnt()), 0x2008);
    reg = port_g3d_disp3dcnt();
    Write16(reg, 0x2000 | 0x0001); // the overflow bit and a change: acknowledged
    CHECK_HEX(Read16(port_g3d_disp3dcnt()), 0x0001);
    Io16(0x60, 0);
}

int main(void) {
    TestReadbacks();
    TestGxstatWrites();
    TestOneDotDepth();
    TestFrame();
    TestKeep();
    TestDisp3dcnt();
    printf("%d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
