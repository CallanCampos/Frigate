// The 3D renderer drawn in bands of lines on the renderers' workers (port/gfx/ref/ds3d.c, port/gfx/workers.c) against
// itself drawn on one thread: g3d_scene.h's frames of 2,000 random textured triangles, with every pass the bands share
// out (the rear plane and the clear image, anti-aliasing, alpha blending, edge marking, which looks across the bands,
// fog, Z and W depths, translucent polygons sorted or in their order), drawn in 2 to 8 bands, must give the picture,
// depths, polygon IDs and fog flags of one band, pixel for pixel. The one-band renderer is the reference because
// test_ds3d.c holds it to values worked out by hand, and runs its own scenes (shadows, wireframes, toon and highlight
// among them) in bands too. No ROM is needed.
#include "g3d_scene.h"
#include "g3d_test.h"
#include "gfx/workers.h"
#include <stdlib.h>

#define PIXELS (PORT_DS3D_WIDTH * PORT_DS3D_HEIGHT)

typedef struct Scene {
    const char *name;
    int minSide;
    int maxSide;
    uint16_t cnt; // DISP3DCNT
    uint32_t swapParam;
} Scene;

// DISP3DCNT: textures 0x1, blending 0x8, anti-aliasing 0x10, edge marking 0x20, fog 0x80 (shift in bits 8-11), the
// clear image 0x4000
static const Scene kScenes[] = {
    { "typical", 6, 24, 0x0019, 0 },
    { "heavy", 12, 48, 0x00b9 | 2u << 8, 0 },
    { "heavy, W and in order", 12, 48, 0x00b9 | 2u << 8, 3 },
    { "edges without anti-aliasing, clear image", 6, 24, 0x40a9 | 1u << 8, 0 },
};

static const int kParts[] = { 2, 3, 4, 5, 7, 8 };

typedef struct Picture {
    uint32_t out[PIXELS];
    uint32_t depth[PIXELS];
    uint8_t id[PIXELS];
    uint8_t fog[PIXELS];
} Picture;

static void Draw(PortDs3dSource *src, Picture *picture) {
    port_ds3d_render(src, picture->out);
    for (int y = 0; y < PORT_DS3D_HEIGHT; y++) {
        for (int x = 0; x < PORT_DS3D_WIDTH; x++) {
            int i = y * PORT_DS3D_WIDTH + x;
            picture->depth[i] = port_ds3d_depth(x, y);
            picture->id[i] = (uint8_t)port_ds3d_polygon_id(x, y);
            picture->fog[i] = (uint8_t)port_ds3d_fog_flag(x, y);
        }
    }
}

// The first pixel where the two differ, or -1
static int FirstDifference(const Picture *a, const Picture *b) {
    for (int i = 0; i < PIXELS; i++) {
        if (a->out[i] != b->out[i] || a->depth[i] != b->depth[i] || a->id[i] != b->id[i] || a->fog[i] != b->fog[i]) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    uint8_t *tex = calloc(512 * 1024, 1);
    uint8_t *pltt = calloc(96 * 1024, 1);
    Picture *one = calloc(1, sizeof(*one));
    Picture *banded = calloc(1, sizeof(*banded));
    PortDs3dSource src;

    sGe = port_ge_create();
    if (tex == NULL || pltt == NULL || one == NULL || banded == NULL || sGe == NULL) {
        printf("out of memory\n");
        return 1;
    }
    SceneSource(&src, tex, pltt);
    // The clear image's colours and depths in slots 2 and 3: random, so every pixel of the rear plane differs
    for (int i = 0x40000; i < 0x80000; i++) {
        tex[i] = (uint8_t)SceneRandom();
    }
    for (size_t s = 0; s < sizeof(kScenes) / sizeof(kScenes[0]); s++) {
        const Scene *scene = &kScenes[s];
        sTest = scene->name;
        SceneSubmit(sGe, scene->minSide, scene->maxSide, (scene->cnt & 0x80) != 0, scene->swapParam);
        src.list = port_ge_render_list(sGe);
        src.regs.disp3dcnt = scene->cnt;
        CHECK_EQ(port_workers_start(1), 1);
        Draw(&src, one);
        for (size_t p = 0; p < sizeof(kParts) / sizeof(kParts[0]); p++) {
            int parts = port_workers_start(kParts[p]);
            CHECK_EQ(parts, kParts[p]);
            // Twice, so that a band's workers see a job after one they ran
            for (int pass = 0; pass < 2; pass++) {
                memset(banded, 0, sizeof(*banded));
                Draw(&src, banded);
                int i = FirstDifference(one, banded);
                if (i >= 0) {
                    printf("%s, %d bands: pixel (%d, %d) is %08x depth %06x ID %d fog %d, one band's %08x %06x %d %d\n",
                           scene->name, parts, i % PORT_DS3D_WIDTH, i / PORT_DS3D_WIDTH, banded->out[i],
                           banded->depth[i], banded->id[i], banded->fog[i], one->out[i], one->depth[i], one->id[i],
                           one->fog[i]);
                }
                CHECK(i < 0);
            }
        }
    }
    port_ge_destroy(sGe);
    free(tex);
    free(pltt);
    free(one);
    free(banded);
    printf("%d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
