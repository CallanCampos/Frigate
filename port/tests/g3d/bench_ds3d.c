// Timing of the geometry engine (port/gfx/ge) and the CPU 3D renderer (port/gfx/ref/ds3d.c). The task's target is a
// frame of 2,000 textured polygons rasterized in a few milliseconds. Two scenes of 2,000 textured triangles in
// perspective (W from 1.5 to 4.0, varying across each triangle), pseudo-random but the same every run:
//
// - "typical": sides of 6 to 24 pixels, about 220,000 pixels in all, some four times the screen; textures of five
//   formats (16 and 256 colours, direct, 4x4 compressed, A3I5), a tenth of the triangles translucent, anti-aliasing and
//   alpha blending on, as G3X_Init leaves them.
// - "heavy": the same with sides of 12 to 48 pixels (some 16 times the screen), and edge marking and fog on too.
//
// It prints the geometry engine's time to take the 2,000 triangles' commands (each as the game would write it, one
// port write per parameter) and the renderer's mean, median and best time per frame. With a limit in microseconds as
// its argument it is a gate: it fails when the median frame of "typical" isn't below the limit. Wall-clock limits
// depend on the machine, so ctest runs the gate only when PORT_GFX_TIMING_GATE is on (port/cmake/g3d.cmake).
#include "gfx/ge/ge.h"
#include "gfx/ref/ds3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRIANGLES 2000
#define FRAMES 60
#define PORT(cmd) (0x04000400u + 4u * (cmd))

static uint32_t sSeed;

static uint32_t Random(void) {
    sSeed = sSeed * 1103515245u + 12345u;
    return sSeed >> 8;
}

static int32_t Range(int32_t lo, int32_t hi) {
    return lo + (int32_t)(Random() % (uint32_t)(hi - lo + 1));
}

static double Now(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

static int CompareDoubles(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return x < y ? -1 : x > y;
}

static void Cmd(PortGe *ge, uint32_t cmd, uint32_t param) {
    port_ge_write(ge, PORT(cmd), param);
}

// The textures: 16 colours 64x64 at 0, 256 colours 64x64 at 0x1000, direct 32x32 at 0x2000, 4x4 64x64 at 0x3000
// (index data at 0x21800), A3I5 32x32 at 0x4000; random texels and palettes
static void MakeTextures(uint8_t *tex, uint8_t *pltt) {
    for (int i = 0; i < 0x5000; i++) {
        tex[i] = (uint8_t)Random();
    }
    for (int i = 0x2000; i < 0x2800; i += 2) {
        tex[i + 1] |= 0x80; // direct: opaque
    }
    for (int i = 0x21800; i < 0x21800 + 0x400; i += 2) {
        uint32_t info = Random() & 0xc0ff; // a mode and a palette offset within the first 1 KiB
        tex[i] = (uint8_t)info;
        tex[i + 1] = (uint8_t)(info >> 8);
    }
    for (int i = 0; i < 96 * 1024; i++) {
        pltt[i] = (uint8_t)Random();
    }
}

static const uint32_t kTextures[5] = {
    0u >> 3 | 3u << 20 | 3u << 23 | 3u << 26 | 1u << 16 | 1u << 17,                 // 16 colours, repeat
    0x1000u >> 3 | 3u << 20 | 3u << 23 | 4u << 26 | 1u << 16 | 1u << 17 | 1u << 18, // 256 colours, flip
    0x2000u >> 3 | 2u << 20 | 2u << 23 | 7u << 26,                                  // direct, clamp
    0x3000u >> 3 | 3u << 20 | 3u << 23 | 5u << 26 | 1u << 16 | 1u << 17,            // 4x4
    0x4000u >> 3 | 2u << 20 | 2u << 23 | 1u << 26 | 1u << 16 | 1u << 17,            // A3I5
};

// One frame's commands: a perspective projection (w = -z), then the triangles
static void Submit(PortGe *ge, int minSide, int maxSide, int fog) {
    sSeed = 12345;
    port_ge_reset(ge);
    Cmd(ge, 0x10, 0);
    static const int32_t kProj[16] = { 0x1000, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0x800, -0x1000, 0, 0, 0, 0 };
    for (int i = 0; i < 16; i++) {
        Cmd(ge, 0x16, (uint32_t)kProj[i]);
    }
    Cmd(ge, 0x10, 3);
    Cmd(ge, 0x15, 0);
    Cmd(ge, 0x10, 2);
    Cmd(ge, 0x15, 0);
    Cmd(ge, 0x60, 0xbfff0000);
    for (int t = 0; t < TRIANGLES; t++) {
        int translucent = Random() % 10 == 0;
        uint32_t attr = 0xc0u | (translucent ? 16u : 31u) << 16 | (uint32_t)(t % 64) << 24 | (fog ? 1u << 15 : 0);
        Cmd(ge, 0x29, attr);
        Cmd(ge, 0x2a, kTextures[Random() % 5]);
        Cmd(ge, 0x2b, Random() % 64);
        Cmd(ge, 0x40, 0);
        int32_t cx = Range(0, 255);
        int32_t cy = Range(0, 191);
        int32_t side = Range(minSide, maxSide);
        int32_t w = Range(0x1800, 0x4000);
        for (int v = 0; v < 3; v++) {
            int32_t x = cx + Range(-side / 2, side / 2);
            int32_t y = cy + Range(-side / 2, side / 2);
            int32_t vw = w + Range(-0x200, 0x200);
            // Screen X = (x' + w) * 256 / 2w with x' = x_object, w = -z_object
            int32_t ox = (int32_t)(((int64_t)(x * 32 - 0x1000) * vw) >> 12);
            int32_t oy = (int32_t)(((int64_t)(0x1000 - y * 128 / 3) * vw) >> 12);
            Cmd(ge, 0x20, Random() & 0x7fff);
            Cmd(ge, 0x22, (Random() & 0x3ff) | (Random() & 0x3ff) << 16);
            Cmd(ge, 0x23, ((uint32_t)ox & 0xffff) | (uint32_t)oy << 16);
            Cmd(ge, 0x23, (uint32_t)(-vw) & 0xffff);
        }
    }
    Cmd(ge, 0x50, 0);
    port_ge_vblank(ge);
}

static void Measure(const char *name, PortGe *ge, PortDs3dSource *src, uint32_t *out, int minSide, int maxSide,
                    uint16_t cnt, double *median) {
    double ge0 = Now();
    for (int i = 0; i < 10; i++) {
        Submit(ge, minSide, maxSide, (cnt & 0x80) != 0);
    }
    double geTime = (Now() - ge0) / 10;
    src->list = port_ge_render_list(ge);
    src->regs.disp3dcnt = cnt;
    double times[FRAMES];
    double sum = 0;
    for (int f = 0; f < FRAMES; f++) {
        double t0 = Now();
        port_ds3d_render(src, out);
        times[f] = Now() - t0;
        sum += times[f];
    }
    qsort(times, FRAMES, sizeof(times[0]), CompareDoubles);
    uint32_t hash = 2166136261u;
    long drawn = 0;
    for (int i = 0; i < PORT_DS3D_WIDTH * PORT_DS3D_HEIGHT; i++) {
        hash = (hash ^ out[i]) * 16777619u;
        drawn += out[i] != 0;
    }
    long area = 0;
    for (uint32_t p = 0; p < src->list->polygon_count; p++) {
        const PortGePolygon *poly = &src->list->polygons[p];
        const PortGeVertex *a = &src->list->vertices[poly->vertices[0]];
        for (int k = 1; k + 1 < poly->vertex_count; k++) {
            const PortGeVertex *b = &src->list->vertices[poly->vertices[k]];
            const PortGeVertex *c = &src->list->vertices[poly->vertices[k + 1]];
            long cross = (long)(b->x - a->x) * (c->y - a->y) - (long)(c->x - a->x) * (b->y - a->y);
            area += (cross < 0 ? -cross : cross) / 2;
        }
    }
    printf("%-8s %u polygons, %u vertices, ~%ld pixels of polygons (%.1f screens), %ld pixels drawn\n", name,
           src->list->polygon_count, src->list->vertex_count, area, area / 49152.0, drawn);
    printf("         geometry engine %.0f us/frame; renderer mean %.0f us, median %.0f us, best %.0f us "
           "(hash %08x)\n",
           geTime, sum / FRAMES, times[FRAMES / 2], times[0], hash);
    *median = times[FRAMES / 2];
}

int main(int argc, char **argv) {
    double limit = argc > 1 ? atof(argv[1]) : 0;
    uint8_t *tex = calloc(512 * 1024, 1);
    uint8_t *pltt = calloc(96 * 1024, 1);
    uint32_t *out = calloc(PORT_DS3D_WIDTH * PORT_DS3D_HEIGHT, sizeof(*out));
    PortGe *ge = port_ge_create();
    PortDs3dSource src;
    double typical;
    double heavy;

    if (tex == NULL || pltt == NULL || out == NULL || ge == NULL) {
        printf("out of memory\n");
        return 1;
    }
    sSeed = 1;
    MakeTextures(tex, pltt);
    memset(&src, 0, sizeof(src));
    for (int i = 0; i < 32; i++) {
        src.tex_pages[i] = tex + i * 0x4000;
    }
    for (int i = 0; i < 6; i++) {
        src.texpltt_pages[i] = pltt + i * 0x4000;
    }
    src.regs.clear_color = 31u << 16;
    src.regs.clear_depth = 0x7fff;
    src.regs.fog_color = 0x7fff | 31u << 16;
    src.regs.fog_offset = 0x6000;
    for (int i = 0; i < 32; i++) {
        src.regs.fog_table[i] = (uint8_t)(i * 4);
        src.regs.toon_table[i] = (uint16_t)(i * 0x421);
    }
    for (int i = 0; i < 8; i++) {
        src.regs.edge_color[i] = (uint16_t)(0x1f << (i % 3 * 5));
    }
    // Textures, alpha blending, anti-aliasing; the heavy scene adds edge marking and fog
    Measure("typical", ge, &src, out, 6, 24, 0x0019, &typical);
    Measure("heavy", ge, &src, out, 12, 48, 0x00b9 | 2u << 8, &heavy);
    port_ge_destroy(ge);
    free(tex);
    free(pltt);
    free(out);
    if (limit > 0 && typical >= limit) {
        printf("FAIL: the typical frame's median %.0f us is not below %.0f us\n", typical, limit);
        return 1;
    }
    return 0;
}
