// Timing of the geometry engine (port/gfx/ge) and the CPU 3D renderer (port/gfx/ref/ds3d.c). The task's target is a
// frame of 2,000 textured polygons rasterized in a few milliseconds. Two scenes of 2,000 textured triangles in
// perspective (W from 1.5 to 4.0, varying across each triangle), pseudo-random but the same every run:
//
// - "typical": sides of 6 to 24 pixels, about 220,000 pixels in all, some four times the screen; textures of five
//   formats (16 and 256 colours, direct, 4x4 compressed, A3I5), a tenth of the triangles translucent, anti-aliasing and
//   alpha blending on, as G3X_Init leaves them.
// - "heavy": the same with sides of 12 to 48 pixels (some 16 times the screen), and edge marking and fog on too.
//
// The scenes are g3d_scene.h's. It prints the geometry engine's time to take the 2,000 triangles' commands (each as
// the game would write it, one port write per parameter) and the renderer's mean, median and best time per frame, on
// one thread and then in as many bands as the program draws by default (port/gfx/workers.h: the host's performance
// cores, at most 8). With a limit in microseconds as its argument it is a gate: it fails when the median frame of
// "typical" on one thread isn't below the limit. Wall-clock limits depend on the machine, so ctest runs the gate only
// when PORT_GFX_TIMING_GATE is on (port/cmake/g3d.cmake).
#include "g3d_scene.h"
#include "gfx/ge/ge.h"
#include "gfx/ref/ds3d.h"
#include "gfx/workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAMES 60

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

static void Measure(const char *name, PortGe *ge, PortDs3dSource *src, uint32_t *out, int minSide, int maxSide,
                    uint16_t cnt, int parts, double *median) {
    double ge0 = Now();
    for (int i = 0; i < 10; i++) {
        SceneSubmit(ge, minSide, maxSide, (cnt & 0x80) != 0, 0);
    }
    double geTime = (Now() - ge0) / 10;
    src->list = port_ge_render_list(ge);
    src->regs.disp3dcnt = cnt;
    double times[FRAMES];
    double sum = 0;
    parts = port_workers_start(parts);
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
    printf("         geometry engine %.0f us/frame; renderer in %d band%s: mean %.0f us, median %.0f us, best %.0f us "
           "(hash %08x)\n",
           geTime, parts, parts != 1 ? "s" : "", sum / FRAMES, times[FRAMES / 2], times[0], hash);
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
    double banded;

    if (tex == NULL || pltt == NULL || out == NULL || ge == NULL) {
        printf("out of memory\n");
        return 1;
    }
    SceneSource(&src, tex, pltt);
    // Textures, alpha blending, anti-aliasing; the heavy scene adds edge marking and fog. One thread, then the
    // program's default bands
    Measure("typical", ge, &src, out, 6, 24, 0x0019, 1, &typical);
    Measure("heavy", ge, &src, out, 12, 48, 0x00b9 | 2u << 8, 1, &heavy);
    Measure("typical", ge, &src, out, 6, 24, 0x0019, 0, &banded);
    Measure("heavy", ge, &src, out, 12, 48, 0x00b9 | 2u << 8, 0, &banded);
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
