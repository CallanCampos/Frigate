// Timing of port/gfx/ref, the CPU reference 2D renderer. PLAN.md §4.6 estimates that it "should cost well under a
// millisecond a frame"; on the M2 it was written on, "typical" takes about 0.7 ms for both engines and "max" about
// 1.75 ms, above that estimate. Three scenes of pseudo-random VRAM, palettes and OAM, rendered over and over:
//
// - "max", the heaviest frame the DS can show. Engine A: four text BGs (two 16-colour, two 256-colour with extended
//   palettes, 512x512 and scrolled), 128 OBJs of 64x32 (half 256 colours with extended palettes, some
//   semi-transparent; about 21 on each line, some 1,350 dots, under any per-line OBJ budget the DS could have), both
//   windows and alpha blending. Engine B: two text BGs, two rotated extended BGs (a 16-bit map with mosaic and a
//   direct-colour bitmap) and 128 affine 32x32 OBJs with rotations (about 21 a line, 1,550 cycles in the GBA's cost
//   model), and brightness.
// - "stress", the same with engine A's OBJs 32x32 and engine B's OBJs double size: some 43 affine 64-dot boxes on
//   every line, about three times what the GBA's cost model lets one line draw. It is reported, not checked.
// - "typical", a frame like a battle's. Engine A: the 3D layer (opaque in the middle, transparent at the sides), three
//   16-colour text BGs, 48 OBJs of 32x32, BG1 alpha-blended. Engine B: four 16-colour text BGs, 40 OBJs of 32x16 and
//   a window.
//
// It prints the mean, median and best time per frame (both engines, 192 lines each). With a limit in microseconds as
// its argument it is also a gate: it fails when the median of "typical" is not below half the limit or the best frame
// of "max" is not below the limit (the median and the best frame, since other work on the machine adds outliers).
// Wall-clock limits depend on the machine, so ctest runs the gate only when PORT_GFX_TIMING_GATE is on
// (port/cmake/gfx_tests.cmake). The clock is C11's timespec_get(), which needs no POSIX feature macro under -std=c11.
#include "gfx/ref/ds2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Memory {
    uint8_t io[0x80];
    uint8_t palette[1024];
    uint8_t oam[1024];
    uint8_t bg[512 * 1024];
    uint8_t obj[256 * 1024];
    uint8_t bgExt[4][8192];
    uint8_t objExt[8192];
    uint32_t line3d[256];
    PortDs2dSource src;
} Memory;

typedef struct Timing {
    double mean;
    double median;
    double best;
    uint32_t hash;
} Timing;

static uint32_t sSeed;

static uint32_t Random(void) {
    sSeed = sSeed * 1103515245u + 12345u;
    return sSeed >> 8;
}

static void Fill(uint8_t *mem, size_t size) {
    size_t i;

    for (i = 0; i < size; i++) {
        mem[i] = (uint8_t)Random();
    }
}

static void Put16(uint8_t *mem, uint32_t offset, uint16_t value) {
    mem[offset] = (uint8_t)value;
    mem[offset + 1] = (uint8_t)(value >> 8);
}

static void Put32(uint8_t *mem, uint32_t offset, uint32_t value) {
    Put16(mem, offset, (uint16_t)value);
    Put16(mem, offset + 2, (uint16_t)(value >> 16));
}

static void Setup(Memory *m, int engine, int stress) {
    int i;

    sSeed = 12345u + (uint32_t)engine;
    memset(m, 0, sizeof(*m));
    Fill(m->palette, sizeof(m->palette));
    Fill(m->bg, sizeof(m->bg));
    Fill(m->obj, sizeof(m->obj));
    Fill(&m->bgExt[0][0], sizeof(m->bgExt));
    Fill(m->objExt, sizeof(m->objExt));
    m->src.io = m->io;
    m->src.palette = m->palette;
    m->src.oam = m->oam;
    for (i = 0; i < (engine == 0 ? 32 : 8); i++) {
        m->src.bg_slots[i] = m->bg + i * 0x4000;
    }
    for (i = 0; i < (engine == 0 ? 16 : 8); i++) {
        m->src.obj_slots[i] = m->obj + i * 0x4000;
    }
    for (i = 0; i < 4; i++) {
        m->src.bg_ext_palette[i] = m->bgExt[i];
    }
    m->src.obj_ext_palette = m->objExt;

    // OBJs spread over the screen; the affine parameter groups' words are the fourth halfword of each entry
    for (i = 0; i < 128; i++) {
        uint16_t attr0 = (uint16_t)(Random() % 192);
        uint16_t attr1 = (uint16_t)(Random() % 256);
        uint16_t attr2 = (uint16_t)(Random() % 512 | (Random() % 4) << 10 | (Random() % 16) << 12);

        if (engine == 0) {
            attr0 |= (uint16_t)((i & 1) << 13);      // half of them 256 colours
            attr0 |= (uint16_t)((i % 8 == 0) << 10); // some semi-transparent
            if (stress) {
                attr1 |= 2 << 14; // 32x32
            } else {
                attr0 |= 1 << 14; // 64x32
                attr1 |= 3 << 14;
            }
        } else {
            attr0 |= stress ? 0x300 : 0x100; // affine, double size in the stress scene
            attr1 |= (uint16_t)(2 << 14 | (i % 32) << 9);
        }
        Put16(m->oam, (uint32_t)i * 8, attr0);
        Put16(m->oam, (uint32_t)i * 8 + 2, attr1);
        Put16(m->oam, (uint32_t)i * 8 + 4, attr2);
        Put16(m->oam, (uint32_t)i * 8 + 6, (uint16_t)(0xC0 + Random() % 0x80));
    }
}

static void SetupEngineA(Memory *m, int stress) {
    Setup(m, 0, stress);
    // Mode 0, BG0-3, OBJ, WIN0, WIN1, 1D OBJs with 128-byte boundary, extended palettes
    Put32(m->io, 0x00, 0x10000u | 0x1F00u | 0x6000u | 0x10u | 2u << 20 | 1u << 30 | 1u << 31);
    Put16(m->io, 0x08, 0 | 1 << 2 | 20 << 8 | 3 << 14);
    Put16(m->io, 0x0A, 1 | 2 << 2 | 24 << 8 | 3 << 14);
    Put16(m->io, 0x0C, 2 | 0x80 | 4 << 2 | 28 << 8 | 3 << 14);
    Put16(m->io, 0x0E, 3 | 0x80 | 6 << 2 | 30 << 8 | 1 << 14);
    Put16(m->io, 0x10, 13);
    Put16(m->io, 0x12, 77);
    Put16(m->io, 0x14, 301);
    Put16(m->io, 0x18, 5);
    Put16(m->io, 0x1E, 400);
    Put16(m->io, 0x40, 20 << 8 | 200);
    Put16(m->io, 0x44, 10 << 8 | 180);
    Put16(m->io, 0x42, 100 << 8 | 30);
    Put16(m->io, 0x46, 50 << 8 | 150);
    Put16(m->io, 0x48, 0x3F | 0x2F << 8);
    Put16(m->io, 0x4A, 0x1F);
    Put16(m->io, 0x50, 0x0F | 0x40 | 0x3F00);
    Put16(m->io, 0x52, 9 | 7 << 8);
}

static void SetupEngineB(Memory *m, int stress) {
    Setup(m, 1, stress);
    // Mode 5: BG0 and BG1 text, BG2 extended with a 16-bit map, BG3 a direct-colour bitmap, both rotated
    Put32(m->io, 0x00, 0x10005u | 0x1F00u | 0x10u | 1u << 30);
    Put16(m->io, 0x08, 0 | 1 << 2 | 20 << 8);
    Put16(m->io, 0x0A, 1 | 2 << 2 | 22 << 8);
    Put16(m->io, 0x0C, 2 | 0x40 | 3 << 2 | 24 << 8 | 1 << 13 | 2 << 14);
    Put16(m->io, 0x0E, 3 | 0x84 | 0 << 8 | 1 << 13 | 1 << 14);
    Put16(m->io, 0x20, 0xF0);
    Put16(m->io, 0x22, 0x40);
    Put16(m->io, 0x24, (uint16_t)-0x40);
    Put16(m->io, 0x26, 0xF0);
    Put16(m->io, 0x30, 0x110);
    Put16(m->io, 0x32, (uint16_t)-0x30);
    Put16(m->io, 0x34, 0x30);
    Put16(m->io, 0x36, 0x110);
    Put32(m->io, 0x28, 0x1234);
    Put32(m->io, 0x3C, 0x5678);
    Put16(m->io, 0x4C, 0x0011);
    Put16(m->io, 0x50, 0x01 | 0x80);
    Put16(m->io, 0x54, 6);
}

// A battle-like frame: see the top of the file
static void SetupTypical(Memory *a, Memory *b) {
    int i;

    Setup(a, 0, 0);
    for (i = 0; i < 256; i++) {
        a->line3d[i] = i >= 48 && i < 208 ? 0xF8000000u | (uint32_t)(i * 0x010203) : 0;
    }
    a->src.bg0_3d_line = a->line3d;
    Put32(a->io, 0x00, 0x10000u | 0x1F00u | 0x8u | 0x10u);
    Put16(a->io, 0x08, 1);
    Put16(a->io, 0x0A, 0 | 1 << 2 | 24 << 8);
    Put16(a->io, 0x0C, 2 | 2 << 2 | 26 << 8);
    Put16(a->io, 0x0E, 3 | 3 << 2 | 28 << 8);
    Put16(a->io, 0x14, 7);
    Put16(a->io, 0x50, 0x02 | 0x40 | 0x3D00);
    Put16(a->io, 0x52, 10 | 6 << 8);
    for (i = 0; i < 128; i++) {
        uint16_t attr0 = i < 48 ? (uint16_t)(Random() % 192) : 0x200;

        Put16(a->oam, (uint32_t)i * 8, attr0);
        Put16(a->oam, (uint32_t)i * 8 + 2, (uint16_t)(Random() % 256 | 2 << 14));
    }

    Setup(b, 1, 0);
    Put32(b->io, 0x00, 0x10000u | 0x1F00u | 0x2000u | 0x10u);
    Put16(b->io, 0x08, 0 | 1 << 2 | 24 << 8);
    Put16(b->io, 0x0A, 1 | 1 << 2 | 26 << 8);
    Put16(b->io, 0x0C, 2 | 2 << 2 | 28 << 8);
    Put16(b->io, 0x0E, 3 | 2 << 2 | 30 << 8);
    Put16(b->io, 0x40, 16 << 8 | 240);
    Put16(b->io, 0x44, 120 << 8 | 184);
    Put16(b->io, 0x48, 0x1F);
    Put16(b->io, 0x4A, 0x1E);
    for (i = 0; i < 128; i++) {
        uint16_t attr0 = i < 40 ? (uint16_t)(Random() % 192 | 1 << 14) : 0x200;

        Put16(b->oam, (uint32_t)i * 8, attr0);
        Put16(b->oam, (uint32_t)i * 8 + 2, (uint16_t)(Random() % 256 | 2 << 14));
    }
}

static double Now(void) {
    struct timespec ts;

    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

static int CompareDoubles(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;

    return (x > y) - (x < y);
}

#define FRAMES 300

static Timing Measure(Memory *a, Memory *b) {
    static uint32_t frame[2][192][256];
    static double times[FRAMES];
    Timing timing;
    int f;
    int line;
    int x;

    port_ds2d_reset(0);
    port_ds2d_reset(1);
    timing.mean = 0;
    for (f = 0; f < FRAMES + 10; f++) {
        double start = Now();

        port_ds2d_begin_frame(0);
        port_ds2d_begin_frame(1);
        for (line = 0; line < 192; line++) {
            port_ds2d_render_line(&a->src, 0, line, frame[0][line]);
            port_ds2d_render_line(&b->src, 1, line, frame[1][line]);
        }
        if (f >= 10) { // the first frames warm the caches
            times[f - 10] = Now() - start;
            timing.mean += times[f - 10] / FRAMES;
        }
    }
    qsort(times, FRAMES, sizeof(times[0]), CompareDoubles);
    timing.median = times[FRAMES / 2];
    timing.best = times[0];
    // A hash of the last frame keeps the work from being optimized away and shows that the output stays the same
    timing.hash = 2166136261u;
    for (line = 0; line < 192; line++) {
        for (x = 0; x < 256; x++) {
            timing.hash = (timing.hash ^ frame[0][line][x]) * 16777619u;
            timing.hash = (timing.hash ^ frame[1][line][x]) * 16777619u;
        }
    }
    return timing;
}

int main(int argc, char **argv) {
    Memory *a = malloc(sizeof(Memory));
    Memory *b = malloc(sizeof(Memory));
    double limit = argc > 1 ? atof(argv[1]) : 0.0;
    Timing max;
    Timing stress;
    Timing typical;

    if (a == NULL || b == NULL) {
        printf("out of memory\n");
        return 1;
    }
    SetupEngineA(a, 0);
    SetupEngineB(b, 0);
    max = Measure(a, b);
    SetupEngineA(a, 1);
    SetupEngineB(b, 1);
    stress = Measure(a, b);
    SetupTypical(a, b);
    typical = Measure(a, b);
    free(a);
    free(b);

    printf("ds2d max:    median %.0f us per frame (both engines, 192 lines), mean %.0f, best %.0f; hash %08x\n",
           max.median, max.mean, max.best, (unsigned)max.hash);
    printf("ds2d stress: median %.0f us per frame, mean %.0f, best %.0f; hash %08x\n", stress.median, stress.mean,
           stress.best, (unsigned)stress.hash);
    printf("ds2d typical: median %.0f us per frame, mean %.0f, best %.0f; hash %08x\n", typical.median, typical.mean,
           typical.best, (unsigned)typical.hash);
    if (limit > 0 && (typical.median >= limit / 2 || max.best >= limit)) {
        printf("FAIL: the median of typical is not below %.0f us, or the best frame of max not below %.0f us\n",
               limit / 2, limit);
        return 1;
    }
    return 0;
}
