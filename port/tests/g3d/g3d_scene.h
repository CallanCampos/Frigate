// The scenes of the 3D renderer's timing (bench_ds3d.c) and of its bands' test (test_ds3d_bands.c): 2,000 textured
// triangles in perspective (W from 1.5 to 4.0, varying across each triangle), pseudo-random but the same every run,
// with textures of five formats (16 and 256 colours, direct, 4x4 compressed, A3I5) and a tenth of the triangles
// translucent; and the rendering registers both use. The sides' range and fog are the caller's. No ROM is needed.
#ifndef PORT_TESTS_G3D_G3D_SCENE_H
#define PORT_TESTS_G3D_G3D_SCENE_H

#include "gfx/ge/ge.h"
#include "gfx/ref/ds3d.h"
#include <stdint.h>
#include <string.h>

#define SCENE_TRIANGLES 2000
#define SCENE_PORT(cmd) (0x04000400u + 4u * (cmd))

static uint32_t sSceneSeed;

static uint32_t SceneRandom(void) {
    sSceneSeed = sSceneSeed * 1103515245u + 12345u;
    return sSceneSeed >> 8;
}

static int32_t SceneRange(int32_t lo, int32_t hi) {
    return lo + (int32_t)(SceneRandom() % (uint32_t)(hi - lo + 1));
}

static void SceneCmd(PortGe *ge, uint32_t cmd, uint32_t param) {
    port_ge_write(ge, SCENE_PORT(cmd), param);
}

// The textures: 16 colours 64x64 at 0, 256 colours 64x64 at 0x1000, direct 32x32 at 0x2000, 4x4 64x64 at 0x3000
// (index data at 0x21800), A3I5 32x32 at 0x4000; random texels and palettes
static void SceneTextures(uint8_t *tex, uint8_t *pltt) {
    for (int i = 0; i < 0x5000; i++) {
        tex[i] = (uint8_t)SceneRandom();
    }
    for (int i = 0x2000; i < 0x2800; i += 2) {
        tex[i + 1] |= 0x80; // direct: opaque
    }
    for (int i = 0x21800; i < 0x21800 + 0x400; i += 2) {
        uint32_t info = SceneRandom() & 0xc0ff; // a mode and a palette offset within the first 1 KiB
        tex[i] = (uint8_t)info;
        tex[i + 1] = (uint8_t)(info >> 8);
    }
    for (int i = 0; i < 96 * 1024; i++) {
        pltt[i] = (uint8_t)SceneRandom();
    }
}

static const uint32_t kSceneTextures[5] = {
    0u >> 3 | 3u << 20 | 3u << 23 | 3u << 26 | 1u << 16 | 1u << 17,                 // 16 colours, repeat
    0x1000u >> 3 | 3u << 20 | 3u << 23 | 4u << 26 | 1u << 16 | 1u << 17 | 1u << 18, // 256 colours, flip
    0x2000u >> 3 | 2u << 20 | 2u << 23 | 7u << 26,                                  // direct, clamp
    0x3000u >> 3 | 3u << 20 | 3u << 23 | 5u << 26 | 1u << 16 | 1u << 17,            // 4x4
    0x4000u >> 3 | 2u << 20 | 2u << 23 | 1u << 26 | 1u << 16 | 1u << 17,            // A3I5
};

// One frame's commands: a perspective projection (w = -z), then the triangles, closed by SWAP_BUFFERS with swapParam
// (bit 0 translucent polygons in their order, bit 1 W-buffering), and swapped in
static void SceneSubmit(PortGe *ge, int minSide, int maxSide, int fog, uint32_t swapParam) {
    sSceneSeed = 12345;
    port_ge_reset(ge);
    SceneCmd(ge, 0x10, 0);
    static const int32_t kProj[16] = { 0x1000, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0x800, -0x1000, 0, 0, 0, 0 };
    for (int i = 0; i < 16; i++) {
        SceneCmd(ge, 0x16, (uint32_t)kProj[i]);
    }
    SceneCmd(ge, 0x10, 3);
    SceneCmd(ge, 0x15, 0);
    SceneCmd(ge, 0x10, 2);
    SceneCmd(ge, 0x15, 0);
    SceneCmd(ge, 0x60, 0xbfff0000);
    for (int t = 0; t < SCENE_TRIANGLES; t++) {
        int translucent = SceneRandom() % 10 == 0;
        uint32_t attr = 0xc0u | (translucent ? 16u : 31u) << 16 | (uint32_t)(t % 64) << 24 | (fog ? 1u << 15 : 0);
        SceneCmd(ge, 0x29, attr);
        SceneCmd(ge, 0x2a, kSceneTextures[SceneRandom() % 5]);
        SceneCmd(ge, 0x2b, SceneRandom() % 64);
        SceneCmd(ge, 0x40, 0);
        int32_t cx = SceneRange(0, 255);
        int32_t cy = SceneRange(0, 191);
        int32_t side = SceneRange(minSide, maxSide);
        int32_t w = SceneRange(0x1800, 0x4000);
        for (int v = 0; v < 3; v++) {
            int32_t x = cx + SceneRange(-side / 2, side / 2);
            int32_t y = cy + SceneRange(-side / 2, side / 2);
            int32_t vw = w + SceneRange(-0x200, 0x200);
            // Screen X = (x' + w) * 256 / 2w with x' = x_object, w = -z_object
            int32_t ox = (int32_t)(((int64_t)(x * 32 - 0x1000) * vw) >> 12);
            int32_t oy = (int32_t)(((int64_t)(0x1000 - y * 128 / 3) * vw) >> 12);
            SceneCmd(ge, 0x20, SceneRandom() & 0x7fff);
            SceneCmd(ge, 0x22, (SceneRandom() & 0x3ff) | (SceneRandom() & 0x3ff) << 16);
            SceneCmd(ge, 0x23, ((uint32_t)ox & 0xffff) | (uint32_t)oy << 16);
            SceneCmd(ge, 0x23, (uint32_t)(-vw) & 0xffff);
        }
    }
    SceneCmd(ge, 0x50, swapParam);
    port_ge_vblank(ge);
}

// The texture and palette spaces as src's pages, filled by MakeTextures, and the rendering registers: an opaque black
// rear plane at the farthest depth, white fog from 0x6000, a toon table of greys and edge colours of each channel
static void SceneSource(PortDs3dSource *src, uint8_t *tex, uint8_t *pltt) {
    sSceneSeed = 1;
    SceneTextures(tex, pltt);
    memset(src, 0, sizeof(*src));
    for (int i = 0; i < 32; i++) {
        src->tex_pages[i] = tex + i * 0x4000;
    }
    for (int i = 0; i < 6; i++) {
        src->texpltt_pages[i] = pltt + i * 0x4000;
    }
    src->regs.clear_color = 31u << 16;
    src->regs.clear_depth = 0x7fff;
    src->regs.fog_color = 0x7fff | 31u << 16;
    src->regs.fog_offset = 0x6000;
    for (int i = 0; i < 32; i++) {
        src->regs.fog_table[i] = (uint8_t)(i * 4);
        src->regs.toon_table[i] = (uint16_t)(i * 0x421);
    }
    for (int i = 0; i < 8; i++) {
        src->regs.edge_color[i] = (uint16_t)(0x1f << (i % 3 * 5));
    }
}

#endif // PORT_TESTS_G3D_G3D_SCENE_H
