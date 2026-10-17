// Tests of port/gfx/ref/ds3d.c, the CPU 3D renderer, driven through the geometry engine as the game drives it. Each
// test builds a small scene and checks exact pixels worked out by hand in the comments, from GBATEK's rules and, where
// ds3d.c says it follows melonDS's measurements, from those (the fill rules, the colour widening, the depth margins);
// the anti-aliasing test checks the port's approximation, which ds3d.c describes. No ROM is needed.
//
// Screen positions: with identity matrices and the whole screen as the viewport, x = X * 32 - 0x1000 lands on screen
// X exactly, and y = 0x1000 - ceil(Y * 128 / 3) on screen Y (Y = (0x1000 - y) * 192 / 0x2000 rounded down). Colours:
// a 5-bit channel c widens to 6 bits as c ? 2c + 1 : 0 (W), and the output widens 6 bits to 8 as (c << 2) | (c >> 4)
// and a 5-bit alpha as (a << 3) | (a >> 2) (OUT).
#include "g3d_test.h"
#include "gfx/ref/ds3d.h"
#include <stdlib.h>
#include <string.h>

#define W(c) ((c) != 0 ? 2 * (c) + 1 : 0)
#define W8(c) ((((c) << 2) | ((c) >> 4)) & 0xffu)
#define A8(a) ((((a) << 3) | ((a) >> 2)) & 0xffu)
#define OUT(r, g, b, a) ((uint32_t)A8(a) << 24 | W8(r) << 16 | W8(g) << 8 | W8(b))

// DISP3DCNT
#define CNT_TEXTURE 0x0001u
#define CNT_HIGHLIGHT 0x0002u
#define CNT_ALPHA_TEST 0x0004u
#define CNT_BLEND 0x0008u
#define CNT_ANTIALIAS 0x0010u
#define CNT_EDGE_MARK 0x0020u
#define CNT_FOG_ALPHA_ONLY 0x0040u
#define CNT_FOG 0x0080u
#define CNT_CLEAR_IMAGE 0x4000u

// TEXIMAGE_PARAM
#define TEX(address, sizeS, sizeT, format)                                                                             \
    ((uint32_t)(address) >> 3 | (uint32_t)(sizeS) << 20 | (uint32_t)(sizeT) << 23 | (uint32_t)(format) << 26)
#define TEX_REPEAT_S (1u << 16)
#define TEX_REPEAT_T (1u << 17)
#define TEX_FLIP_S (1u << 18)
#define TEX_FLIP_T (1u << 19)
#define TEX_COLOR0_TRANSPARENT (1u << 29)

static uint8_t *sTex;  // the texture image space, 512 KiB
static uint8_t *sPltt; // the texture palette space, 96 KiB
static uint32_t *sOut;
static PortDs3dSource sSrc;

static void Start(const char *name) {
    sTest = name;
    port_ge_reset(sGe);
    Basic();
    memset(sTex, 0, 512 * 1024);
    memset(sPltt, 0, 96 * 1024);
    memset(&sSrc.regs, 0, sizeof(sSrc.regs));
    sSrc.regs.clear_depth = 0x7fff;
}

static void Render(uint32_t swapParam) {
    SwapBuffers(swapParam);
    port_ge_vblank(sGe);
    sSrc.list = port_ge_render_list(sGe);
    port_ds3d_render(&sSrc, sOut);
}

static uint32_t Px(int x, int y) {
    return sOut[y * PORT_DS3D_WIDTH + x];
}

#define CHECK_PX(x, y, want) CheckHex(__LINE__, "pixel (" #x ", " #y ")", Px(x, y), want)

static int Count(uint32_t color) {
    int n = 0;
    for (int i = 0; i < PORT_DS3D_WIDTH * PORT_DS3D_HEIGHT; i++) {
        n += sOut[i] == color;
    }
    return n;
}

static int32_t SX(int x) {
    return x * 32 - ONE;
}

static int32_t SY(int y) {
    return ONE - (y * 128 + 2) / 3;
}

static void Put16(uint8_t *mem, uint32_t address, uint16_t value) {
    mem[address] = (uint8_t)value;
    mem[address + 1] = (uint8_t)(value >> 8);
}

// A quad over screen [x0, x1) x [y0, y1), counter-clockwise as seen
static void Quad(int x0, int y0, int x1, int y1, int32_t z) {
    Begin(1);
    Vtx16(SX(x0), SY(y1), z);
    Vtx16(SX(x1), SY(y1), z);
    Vtx16(SX(x1), SY(y0), z);
    Vtx16(SX(x0), SY(y0), z);
}

// The same with texture coordinates (1.11.4) from (s0, t0) at the top left to (s1, t1) at the bottom right
static void TexQuad(int x0, int y0, int x1, int y1, int32_t s0, int32_t t0, int32_t s1, int32_t t1) {
    Begin(1);
    TexCoord(s0, t1);
    Vtx16(SX(x0), SY(y1), 0);
    TexCoord(s1, t1);
    Vtx16(SX(x1), SY(y1), 0);
    TexCoord(s1, t0);
    Vtx16(SX(x1), SY(y0), 0);
    TexCoord(s0, t0);
    Vtx16(SX(x0), SY(y0), 0);
}

// A projection with w = 4.0 for every vertex, x and y scaled to match: screen positions stay as above, and the depth is
// ((z * 0x4000 / 0x4000) + 0x3fff) * 0x200 = (z + 0x3fff) * 0x200, so each unit of z is 0x200 of depth
static void ProjectionW4(void) {
    int32_t m[16] = { 0x4000, 0, 0, 0, 0, 0x4000, 0, 0, 0, 0, ONE, 0, 0, 0, 0, 0x4000 };
    MtxMode(0);
    MtxLoad44(m);
    MtxMode(2);
}

// A projection with w = -z and z = 0: a vertex at z = -k has w = k, and lands on screen X (x + k) * 256 / 2k
static void ProjectionPerspective(void) {
    int32_t m[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, 0, -ONE, 0, 0, 0, 0 };
    MtxMode(0);
    MtxLoad44(m);
    MtxMode(2);
}

// The rear plane: clear colour, alpha, depth (15 bits widened to 24 as d * 0x200 + ((d + 1) / 0x8000) * 0x1ff),
// polygon ID and fog flag; and the clear image from texture slots 2 and 3, scrolled
static void TestClear(void) {
    Start("clear");
    sSrc.regs.clear_color = Rgb(31, 0, 15) | 31u << 16 | 5u << 24;
    Render(0);
    CHECK_PX(0, 0, OUT(63, 0, 31, 31));
    CHECK_PX(255, 191, OUT(63, 0, 31, 31));
    CHECK_HEX(port_ds3d_depth(0, 0), 0xffffff);
    CHECK_EQ(port_ds3d_polygon_id(100, 100), 5);
    CHECK_EQ(port_ds3d_fog_flag(100, 100), 0);
    Start("clear (transparent)");
    sSrc.regs.clear_color = Rgb(31, 31, 31) | 0x8000u;
    sSrc.regs.clear_depth = 0x4000;
    Render(0);
    CHECK_PX(10, 10, 0);
    CHECK_HEX(port_ds3d_depth(10, 10), 0x800000);
    CHECK_EQ(port_ds3d_fog_flag(10, 10), 1);
    Start("clear image");
    sSrc.regs.disp3dcnt = CNT_CLEAR_IMAGE;
    sSrc.regs.clear_color = 7u << 24;
    sSrc.regs.clear_offset = 3 << 8 | 2;
    // Screen (10, 20) reads image (12, 23); (254, 190) reads (0, 193)
    Put16(sTex, 0x40000 + (23 * 256 + 12) * 2, Rgb(0, 31, 0) | 0x8000);
    Put16(sTex, 0x60000 + (23 * 256 + 12) * 2, 0x1234 | 0x8000);
    Put16(sTex, 0x40000 + (193 * 256 + 0) * 2, Rgb(31, 31, 31) | 0x8000);
    Render(0);
    CHECK_PX(10, 20, OUT(0, 63, 0, 31));
    CHECK_HEX(port_ds3d_depth(10, 20), 0x1234 * 0x200);
    CHECK_EQ(port_ds3d_fog_flag(10, 20), 1);
    CHECK_EQ(port_ds3d_polygon_id(10, 20), 7);
    CHECK_PX(11, 20, 0); // colour 0, its alpha bit clear
    CHECK_HEX(port_ds3d_depth(11, 20), 0);
    CHECK_PX(254, 190, OUT(63, 63, 63, 31));
}

// A quad from X 10 to 30 and Y 20 to 40 covers [10, 30) x [20, 40): the right and bottom edges are outside
static void TestCoverage(void) {
    Start("coverage");
    PolygonAttr(ATTR(31, 1));
    Color(Rgb(31, 0, 0));
    Quad(10, 20, 30, 40, 0);
    Render(0);
    uint32_t red = OUT(63, 0, 0, 31);
    CHECK_PX(10, 20, red);
    CHECK_PX(29, 20, red);
    CHECK_PX(10, 39, red);
    CHECK_PX(29, 39, red);
    CHECK_PX(20, 30, red);
    CHECK_PX(9, 20, 0);
    CHECK_PX(30, 20, 0);
    CHECK_PX(10, 19, 0);
    CHECK_PX(10, 40, 0);
    CHECK_EQ(Count(red), 400);
    CHECK_HEX(port_ds3d_depth(20, 30), 0x7ffe00);
    CHECK_EQ(port_ds3d_polygon_id(20, 30), 1);
}

// Two opaque triangles sharing the quad's diagonal share its pixels out (ds3d.c's fill rules). The diagonal is 45
// degrees, Y-major, at X 10 + (y - 20) on line y: it is A's left edge (drawn: A covers [10 + y - 20, 30)) and B's right
// edge (not drawn: B covers [10, 10 + y - 20)). On line 20 B's span is empty, so it gets one pixel, X 10, where A,
// drawn first at the same depth, stays (the depth test is "less"). A: 20 + 19 + ... + 1 = 210 pixels, B: 0 + 1 + ... +
// 19 = 190
static void TestSharedEdge(void) {
    Start("shared edge");
    PolygonAttr(ATTR(31, 1));
    Begin(0);
    Color(Rgb(31, 0, 0));
    Vtx16(SX(10), SY(20), 0);
    Vtx16(SX(30), SY(40), 0);
    Vtx16(SX(30), SY(20), 0);
    Color(Rgb(0, 31, 0));
    Vtx16(SX(10), SY(20), 0);
    Vtx16(SX(10), SY(40), 0);
    Vtx16(SX(30), SY(40), 0);
    Render(0);
    uint32_t a = OUT(63, 0, 0, 31);
    uint32_t b = OUT(0, 63, 0, 31);
    CHECK_EQ(Count(a), 210);
    CHECK_EQ(Count(b), 190);
    CHECK_PX(10, 20, a);
    CHECK_PX(10, 21, b);
    CHECK_PX(11, 21, a);
    CHECK_PX(28, 39, b);
    CHECK_PX(29, 39, a);
    CHECK_PX(30, 30, 0);
    CHECK_PX(20, 40, 0);
}

// The depth test: less, or equal within 0x200 of Z (each unit of z is 0x200 here)
static void TestDepth(void) {
    Start("depth");
    ProjectionW4();
    PolygonAttr(ATTR(31, 1));
    Color(Rgb(31, 0, 0));
    Quad(10, 10, 20, 20, 0x100);
    Color(Rgb(0, 31, 0));
    Quad(10, 10, 20, 20, 0x200); // farther: hidden
    Color(Rgb(31, 0, 0));
    Quad(30, 10, 40, 20, 0); // depth 0x7ffe00
    Quad(50, 10, 60, 20, 0);
    Quad(70, 10, 80, 20, 0);
    Quad(90, 10, 100, 20, 0);
    Color(Rgb(0, 31, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_DEPTH_EQUAL);
    Begin(1);
    Quad(30, 10, 40, 20, 1);  // 0x800000: 0x200 off, equal
    Quad(50, 10, 60, 20, 2);  // 0x400 off: not equal
    Quad(70, 10, 80, 20, -1); // -0x200 off: equal
    PolygonAttr(ATTR(31, 1));
    Quad(90, 10, 100, 20, 0); // the same depth isn't less
    Color(Rgb(0, 0, 31));
    Quad(10, 10, 20, 20, 0x80); // nearer
    Render(0);
    CHECK_PX(15, 15, OUT(0, 0, 63, 31));
    CHECK_PX(35, 15, OUT(0, 63, 0, 31));
    CHECK_HEX(port_ds3d_depth(35, 15), 0x800000);
    CHECK_PX(55, 15, OUT(63, 0, 0, 31));
    CHECK_HEX(port_ds3d_depth(55, 15), 0x7ffe00);
    CHECK_PX(75, 15, OUT(0, 63, 0, 31));
    CHECK_PX(95, 15, OUT(63, 0, 0, 31));
}

// Translucent polygons: (source * (alpha + 1) + destination * (31 - alpha)) / 32 and the larger alpha; nothing over
// a translucent pixel of the same polygon ID; the alpha test passes alphas above the reference only
static void TestTranslucent(void) {
    Start("translucent");
    sSrc.regs.disp3dcnt = CNT_BLEND;
    sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(15, 1));
    Quad(10, 10, 20, 20, 0);
    Quad(10, 10, 20, 20, 0); // the same ID again: not drawn
    Quad(30, 10, 40, 20, 0);
    PolygonAttr(ATTR(15, 2));
    Quad(30, 10, 40, 20, 0); // another ID: blended again
    Render(0);
    // Red over blue: (63 * 16 + 0 * 16) / 32 = 31 red, (0 * 16 + 63 * 16) / 32 = 31 blue
    CHECK_PX(15, 15, OUT(31, 0, 31, 31));
    // Again over that: (63 * 16 + 31 * 16) / 32 = 47 red, 31 * 16 / 32 = 15 blue
    CHECK_PX(35, 15, OUT(47, 0, 15, 31));
    CHECK_HEX(port_ds3d_depth(15, 15), 0xffffff); // POLYGON_ATTR bit 11 clear: the depth stays
    Start("translucent (alpha test)");
    sSrc.regs.disp3dcnt = CNT_BLEND | CNT_ALPHA_TEST;
    sSrc.regs.alpha_test_ref = 15;
    sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(15, 3));
    Quad(50, 10, 60, 20, 0);
    PolygonAttr(ATTR(16, 4) | (1u << 11));
    Quad(70, 10, 80, 20, 0);
    Render(0);
    CHECK_PX(55, 15, OUT(0, 0, 63, 31));
    // (63 * 17) / 32 = 33 red, (63 * 15) / 32 = 29 blue; bit 11 writes the depth
    CHECK_PX(75, 15, OUT(33, 0, 29, 31));
    CHECK_HEX(port_ds3d_depth(75, 15), 0x7ffe00);
    Start("translucent (over alpha 0)");
    sSrc.regs.disp3dcnt = CNT_BLEND;
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(15, 1) | ATTR_FOG);
    Quad(10, 10, 20, 20, 0);
    Render(0);
    CHECK_PX(15, 15, OUT(63, 0, 0, 15));
    CHECK_EQ(port_ds3d_fog_flag(15, 15), 0); // ANDed with the rear plane's, clear
    Start("translucent (blending off)");
    sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(15, 1));
    Quad(10, 10, 20, 20, 0);
    Render(0);
    CHECK_PX(15, 15, OUT(63, 0, 0, 15));
}

static void Texel16(uint32_t address, int x, int y, int width, uint16_t value) {
    Put16(sTex, address + (uint32_t)(y * width + x) * 2, value);
}

// Every texture format on an 8x8 quad at the top left, texel (x, y) on pixel (x, y); white vertices, so modulation
// gives the texel exactly: ((t + 1) * 64 - 1) / 64 = t, ((a + 1) * 32 - 1) / 32 = a
static void TestTextures(void) {
    Start("texture (direct)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Texel16(0, x, y, 8, Rgb(x * 4, y * 4, 31) | 0x8000);
        }
    }
    Texel16(0, 7, 7, 8, 0); // alpha bit clear: transparent
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    TexImageParam(TEX(0, 0, 0, 7));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint32_t want = x == 7 && y == 7 ? 0 : OUT(W(x * 4), W(y * 4), 63, 31);
            CheckHex(__LINE__, "direct texel", Px(x, y), want);
        }
    }
    CHECK_PX(8, 0, 0);

    Start("texture (256 colours)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int i = 0; i < 64; i++) {
        Put16(sPltt, 16 + i * 2, Rgb(i & 31, (i >> 5) * 8 + 1, 3)); // PLTT_BASE 1: 16 bytes
        sTex[0x200 + i] = (uint8_t)i;
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(1);
    TexImageParam(TEX(0x200, 0, 0, 4) | TEX_COLOR0_TRANSPARENT);
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    TexImageParam(TEX(0x200, 0, 0, 4));
    TexQuad(16, 0, 24, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int i = y * 8 + x;
            uint32_t want = OUT(W(i & 31), W((i >> 5) * 8 + 1), W(3), 31);
            CheckHex(__LINE__, "256-colour texel", Px(x, y), i == 0 ? 0 : want);
            CheckHex(__LINE__, "256-colour texel, colour 0 drawn", Px(16 + x, y), want);
        }
    }

    Start("texture (16 colours)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int i = 0; i < 16; i++) {
        Put16(sPltt, 32 + i * 2, Rgb(i * 2, 31 - i * 2, i)); // PLTT_BASE 2: 32 bytes
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int i = (x + y) & 15;
            sTex[0x400 + (y * 8 + x) / 2] |= (uint8_t)(i << ((x & 1) * 4)); // even texels in the low nibble
        }
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(2);
    TexImageParam(TEX(0x400, 0, 0, 3));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int i = (x + y) & 15;
            CheckHex(__LINE__, "16-colour texel", Px(x, y), OUT(W(i * 2), W(31 - i * 2), W(i), 31));
        }
    }

    Start("texture (4 colours)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int i = 0; i < 4; i++) {
        Put16(sPltt, 24 + i * 2, Rgb(i * 10, 0, 31 - i * 10)); // PLTT_BASE 3 in 8-byte units: 24
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int i = (x + 2 * y) & 3;
            sTex[0x600 + (y * 8 + x) / 4] |= (uint8_t)(i << ((x & 3) * 2));
        }
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(3);
    TexImageParam(TEX(0x600, 0, 0, 2));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int i = (x + 2 * y) & 3;
            CheckHex(__LINE__, "4-colour texel", Px(x, y), OUT(W(i * 10), 0, W(31 - i * 10), 31));
        }
    }

    // A3I5: 3-bit alpha a widened as a * 4 + a / 2: 0, 4, 9, 13, 18, 22, 27, 31. Over a rear plane of alpha 0 the
    // translucent texels are written as they are
    Start("texture (A3I5)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE | CNT_BLEND;
    static const int kA3[8] = { 0, 4, 9, 13, 18, 22, 27, 31 };
    for (int i = 0; i < 8; i++) {
        Put16(sPltt, 64 + i * 2, Rgb(i, 31, 0)); // PLTT_BASE 4
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sTex[0x800 + y * 8 + x] = (uint8_t)(x | y << 5);
        }
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(4);
    TexImageParam(TEX(0x800, 0, 0, 1));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            CheckHex(__LINE__, "A3I5 texel", Px(x, y), y == 0 ? 0 : OUT(W(x), 63, 0, kA3[y]));
        }
    }

    Start("texture (A5I3)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE | CNT_BLEND;
    for (int i = 0; i < 8; i++) {
        Put16(sPltt, 80 + i * 2, Rgb(31, i * 4, 0)); // PLTT_BASE 5
    }
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sTex[0xa00 + y * 8 + x] = (uint8_t)(x | (y * 4 + 3) << 3);
        }
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(5);
    TexImageParam(TEX(0xa00, 0, 0, 6));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            CheckHex(__LINE__, "A5I3 texel", Px(x, y), OUT(63, W(x * 4), 0, y * 4 + 3));
        }
    }

    // 4x4 compressed: four blocks at 0x1000 in slot 0, every row of each the codes 0, 1, 2, 3; their palette index
    // halfwords at 0x20000 + 0x1000 / 2 + 2 * block. PLTT_BASE 6 (96 bytes), offsets in 4-byte units:
    //   block 0, mode 0, offset 0: entries 0, 1, 2, transparent
    //   block 1, mode 1, offset 1: entries 2, 3, (2 + 3) / 2, transparent
    //   block 2, mode 2, offset 2: entries 4, 5, 6, 7
    //   block 3, mode 3, offset 4: entries 8, 9, (5 * 8 + 3 * 9) / 8, (3 * 8 + 5 * 9) / 8
    Start("texture (4x4)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    const uint16_t kEntries[10] = {
        Rgb(31, 0, 0), Rgb(0, 31, 0), Rgb(0, 0, 31),   Rgb(31, 31, 0), Rgb(1, 2, 3),
        Rgb(4, 5, 6),  Rgb(7, 8, 9),  Rgb(10, 11, 12), Rgb(16, 0, 8),  Rgb(0, 16, 24),
    };
    static const uint16_t kInfo[4] = { 0x0000, 0x4001, 0x8002, 0xc004 };
    for (int i = 0; i < 10; i++) {
        Put16(sPltt, 96 + i * 2, kEntries[i]);
    }
    for (int b = 0; b < 4; b++) {
        Put16(sTex, 0x1000 + b * 4, 0xe4e4);
        Put16(sTex, 0x1000 + b * 4 + 2, 0xe4e4);
        Put16(sTex, 0x20000 + 0x800 + b * 2, kInfo[b]);
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    PlttBase(6);
    TexImageParam(TEX(0x1000, 0, 0, 5));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    // (2 + 3) / 2 = (15, 15, 15); (5 * 8 + 3 * 9) / 8 = (10, 6, 14); (3 * 8 + 5 * 9) / 8 = (6, 10, 18)
    const uint32_t want[4][4] = {
        { OUT(63, 0, 0, 31), OUT(0, 63, 0, 31), OUT(0, 0, 63, 31), 0 },
        { OUT(0, 0, 63, 31), OUT(63, 63, 0, 31), OUT(31, 31, 31, 31), 0 },
        { OUT(3, 5, 7, 31), OUT(9, 11, 13, 31), OUT(15, 17, 19, 31), OUT(21, 23, 25, 31) },
        { OUT(33, 0, 17, 31), OUT(0, 33, 49, 31), OUT(21, 13, 29, 31), OUT(13, 21, 37, 31) },
    };
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int block = (y >> 2) * 2 + (x >> 2);
            CheckHex(__LINE__, "4x4 texel", Px(x, y), want[block][x & 3]);
        }
    }

    // Repeat, flip and clamp: an 8x8 direct texture over 16x8 pixels (s from 0 to 16 texels), and over 8x16 for T
    Start("texture (wrap)");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            Texel16(0x2000, x, y, 8, Rgb(x * 4, y * 4, 0) | 0x8000);
        }
    }
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    TexImageParam(TEX(0x2000, 0, 0, 7) | TEX_REPEAT_S);
    TexQuad(0, 0, 16, 8, 0, 0, 256, 128);
    TexImageParam(TEX(0x2000, 0, 0, 7) | TEX_REPEAT_S | TEX_FLIP_S);
    TexQuad(0, 10, 16, 18, 0, 0, 256, 128);
    TexImageParam(TEX(0x2000, 0, 0, 7));
    TexQuad(0, 20, 16, 28, 0, 0, 256, 128);
    TexQuad(20, 20, 36, 28, -256, 0, 0, 128); // clamped from below
    TexImageParam(TEX(0x2000, 0, 0, 7) | TEX_REPEAT_S | TEX_FLIP_S);
    TexQuad(40, 0, 48, 8, -128, 0, 0, 128); // tile -1, odd: flipped
    TexImageParam(TEX(0x2000, 0, 0, 7) | TEX_REPEAT_T | TEX_FLIP_T);
    TexQuad(60, 0, 68, 16, 0, 0, 128, 256);
    Render(0);
    for (int x = 0; x < 16; x++) {
        int y = 3;
        CheckHex(__LINE__, "repeat S", Px(x, y), OUT(W((x & 7) * 4), W(y * 4), 0, 31));
        int f = x < 8 ? x : 15 - x;
        CheckHex(__LINE__, "flip S", Px(x, 10 + y), OUT(W(f * 4), W(y * 4), 0, 31));
        int c = x < 8 ? x : 7;
        CheckHex(__LINE__, "clamp S", Px(x, 20 + y), OUT(W(c * 4), W(y * 4), 0, 31));
        CheckHex(__LINE__, "clamp S below", Px(20 + x, 20 + y), OUT(0, W(y * 4), 0, 31));
    }
    for (int x = 0; x < 8; x++) {
        CheckHex(__LINE__, "flip S, tile -1", Px(40 + x, 3), OUT(W((7 - x) * 4), W(12), 0, 31));
    }
    for (int y = 0; y < 16; y++) {
        int f = y < 8 ? y : 15 - y;
        CheckHex(__LINE__, "flip T", Px(62, y), OUT(W(8), W(f * 4), 0, 31));
    }

    // With DISP3DCNT bit 0 clear, no texture: the vertex colour
    Start("texture (off)");
    Color(Rgb(5, 6, 7));
    PolygonAttr(ATTR(31, 1));
    TexImageParam(TEX(0x2000, 0, 0, 7));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    CHECK_PX(3, 3, OUT(11, 13, 15, 31));
}

// Polygon modes: toon (the table entry the vertex red indexes replaces the colour), highlight (red for all three,
// then the entry added), decal (the texel over the vertex colour by the texel's alpha)
static void TestModes(void) {
    Start("toon");
    sSrc.regs.toon_table[10] = Rgb(0, 31, 0); // red 10 widens to 21, and 21 / 2 = 10
    Color(Rgb(10, 0, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_MODE(2));
    Quad(0, 0, 8, 8, 0);
    Render(0);
    CHECK_PX(4, 4, OUT(0, 63, 0, 31));
    Start("highlight");
    sSrc.regs.disp3dcnt = CNT_HIGHLIGHT;
    sSrc.regs.toon_table[10] = Rgb(5, 5, 5); // 11 each
    Color(Rgb(10, 0, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_MODE(2));
    Quad(0, 0, 8, 8, 0);
    Render(0);
    CHECK_PX(4, 4, OUT(32, 32, 32, 31)); // 21 + 11
    Start("decal");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 4; x++) {
            Texel16(0, x, y, 8, Rgb(31, 0, 0) | 0x8000);
        }
    }
    for (int i = 0; i < 64; i++) {
        sTex[0x200 + i] = (uint8_t)(16 << 3); // A5I3: alpha 16, colour 0
    }
    Put16(sPltt, 0, Rgb(31, 0, 0));
    Color(Rgb(0, 0, 31));
    PolygonAttr(ATTR(31, 1) | ATTR_MODE(1));
    TexImageParam(TEX(0, 0, 0, 7));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    TexImageParam(TEX(0x200, 0, 0, 6));
    TexQuad(10, 0, 18, 8, 0, 0, 128, 128);
    Render(0);
    CHECK_PX(2, 2, OUT(63, 0, 0, 31)); // opaque texel
    CHECK_PX(6, 2, OUT(0, 0, 63, 31)); // transparent texel: the vertex colour, the polygon's alpha
    // (63 * 16 + 0 * 15) / 32 = 31 red, (0 * 16 + 63 * 15) / 32 = 29 blue
    CHECK_PX(12, 2, OUT(31, 0, 29, 31));
    Start("modulate");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int i = 0; i < 64; i++) {
        Texel16(0, i & 7, i >> 3, 8, Rgb(15, 31, 0) | 0x8000);
    }
    Color(Rgb(16, 16, 16)); // 33
    PolygonAttr(ATTR(31, 1));
    TexImageParam(TEX(0, 0, 0, 7));
    TexQuad(0, 0, 8, 8, 0, 0, 128, 128);
    Render(0);
    // ((31 + 1) * (33 + 1) - 1) / 64 = 16; ((63 + 1) * 34 - 1) / 64 = 33; ((0 + 1) * 34 - 1) / 64 = 0
    CHECK_PX(3, 3, OUT(16, 33, 0, 31));
}

// Fog: FOG_OFFSET 0x4000 (0x800000 in 24 bits), shift 0 (steps of 0x400 in 15 bits, 0x80000 in 24): entry n holds at
// 0x800000 + (n + 1) * 0x80000. A quad at z = 0x501 has depth (0x501 + 0x3fff) * 0x200 = 0x8a0000, a quarter of the
// way from entry 0 (0x880000, density 32) to entry 1 (96): 32 * 3/4 + 96 / 4 = 48. Fog white, alpha 31:
// red (63 * 48 + 63 * 80) / 128 = 63, green and blue 63 * 48 / 128 = 23, alpha 31
static void TestFog(void) {
    Start("fog");
    sSrc.regs.disp3dcnt = CNT_FOG;
    sSrc.regs.fog_color = Rgb(31, 31, 31) | 31u << 16;
    sSrc.regs.fog_offset = 0x4000;
    // The rear plane is fogged too: depth 0xffffff is between entries 14 and 15, both 127, which means all fog: the
    // fog's colour and alpha
    sSrc.regs.clear_color = 0x8000;
    for (int i = 0; i < 32; i++) {
        sSrc.regs.fog_table[i] = 100;
    }
    sSrc.regs.fog_table[0] = 32;
    sSrc.regs.fog_table[1] = 96;
    sSrc.regs.fog_table[14] = 127;
    sSrc.regs.fog_table[15] = 127;
    ProjectionW4();
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_FOG);
    Quad(10, 10, 20, 20, 0x501);
    Quad(50, 10, 60, 20, 0x100); // depth 0x81fe00, before entry 0: density 32, green 63 * 32 / 128 = 15
    PolygonAttr(ATTR(31, 1));
    Quad(30, 10, 40, 20, 0x501); // no fog flag
    Render(0);
    CHECK_PX(15, 15, OUT(63, 23, 23, 31));
    CHECK_PX(55, 15, OUT(63, 15, 15, 31));
    CHECK_PX(35, 15, OUT(63, 0, 0, 31));
    CHECK_PX(200, 100, OUT(63, 63, 63, 31));
    // Alpha only, fog alpha 0: alpha (0 * 48 + 31 * 80) / 128 = 19, the colour stays
    Start("fog (alpha only)");
    sSrc.regs.disp3dcnt = CNT_FOG | CNT_FOG_ALPHA_ONLY;
    sSrc.regs.fog_color = Rgb(31, 31, 31);
    sSrc.regs.fog_offset = 0x4000;
    sSrc.regs.fog_table[0] = 32;
    sSrc.regs.fog_table[1] = 96;
    ProjectionW4();
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_FOG);
    Quad(10, 10, 20, 20, 0x501);
    Render(0);
    CHECK_PX(15, 15, OUT(63, 0, 0, 19));
    // Shift 2: steps of 0x20000, entry n at 0x820000 + n * 0x20000; 0x8a0000 is entry 4 exactly, density 100:
    // green 63 * 100 / 128 = 49
    Start("fog (shift)");
    sSrc.regs.disp3dcnt = CNT_FOG | 2u << 8;
    sSrc.regs.fog_color = Rgb(31, 31, 31) | 31u << 16;
    sSrc.regs.fog_offset = 0x4000;
    for (int i = 0; i < 32; i++) {
        sSrc.regs.fog_table[i] = (uint8_t)(i == 4 ? 100 : 0);
    }
    ProjectionW4();
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(31, 1) | ATTR_FOG);
    Quad(10, 10, 20, 20, 0x501);
    Render(0);
    CHECK_PX(15, 15, OUT(63, 49, 49, 31));
}

// Edge marking: an opaque edge pixel takes EDGE_COLOR[ID / 8] where a neighbour has another ID and is farther. Quads
// 1 and 2 (ID 8) touch at X 30, quad 3 (ID 16) is nearer and touches quad 2 at X 50
static void TestEdgeMarking(void) {
    Start("edge marking");
    sSrc.regs.disp3dcnt = CNT_EDGE_MARK;
    sSrc.regs.clear_color = 31u << 16;
    sSrc.regs.edge_color[1] = Rgb(0, 31, 0);
    sSrc.regs.edge_color[2] = Rgb(0, 0, 31);
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(31, 8));
    Quad(10, 20, 30, 40, 0);
    Quad(30, 20, 50, 40, 0);
    PolygonAttr(ATTR(31, 16));
    Quad(50, 20, 70, 40, -0x400);
    Render(0);
    uint32_t red = OUT(63, 0, 0, 31);
    uint32_t green = OUT(0, 63, 0, 31);
    uint32_t blue = OUT(0, 0, 63, 31);
    CHECK_PX(10, 30, green); // the rear plane on the left
    CHECK_PX(20, 20, green); // top
    CHECK_PX(20, 39, green); // bottom
    CHECK_PX(20, 30, red);   // inside: no edge
    CHECK_PX(29, 30, red);   // quad 2, same ID, on the right
    CHECK_PX(30, 30, red);
    CHECK_PX(49, 30, red);  // quad 3 on the right is nearer
    CHECK_PX(50, 30, blue); // quad 2 on the left is farther
    CHECK_PX(69, 30, blue);
    CHECK_PX(60, 30, red);
}

// Shadows: the mask (mode 3, ID 0) marks where its depth test fails, behind the floor; the shadow (mode 3, ID 2,
// alpha 16, black) draws only there, and not on its own ID: (0 * 17 + 63 * 15) / 32 = 29
static void TestShadow(void) {
    Start("shadow");
    sSrc.regs.disp3dcnt = CNT_BLEND;
    sSrc.regs.clear_color = 31u << 16;
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    Quad(0, 0, 64, 64, 0);
    Color(0);
    PolygonAttr(ATTR(15, 0) | ATTR_MODE(3));
    Quad(16, 16, 48, 48, 0x400);
    PolygonAttr(ATTR(16, 2) | ATTR_MODE(3));
    Quad(8, 8, 56, 56, -0x400);
    Render(0);
    CHECK_PX(20, 20, OUT(29, 29, 29, 31));
    CHECK_PX(47, 47, OUT(29, 29, 29, 31));
    CHECK_PX(10, 10, OUT(63, 63, 63, 31));
    CHECK_PX(50, 50, OUT(63, 63, 63, 31));
    Start("shadow (same ID)");
    sSrc.regs.disp3dcnt = CNT_BLEND;
    sSrc.regs.clear_color = 31u << 16;
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    Quad(0, 0, 64, 64, 0);
    Color(0);
    PolygonAttr(ATTR(15, 0) | ATTR_MODE(3));
    Quad(16, 16, 48, 48, 0x400);
    PolygonAttr(ATTR(16, 1) | ATTR_MODE(3));
    Quad(8, 8, 56, 56, -0x400);
    Render(0);
    CHECK_PX(20, 20, OUT(63, 63, 63, 31));
}

// Wireframe (alpha 0): only the edges, opaque; a polygon that comes out as one dot draws that dot
static void TestWireframeAndDot(void) {
    Start("wireframe");
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(0, 1));
    Quad(10, 10, 20, 20, 0);
    Render(0);
    uint32_t red = OUT(63, 0, 0, 31);
    CHECK_PX(10, 10, red);
    CHECK_PX(19, 10, red);
    CHECK_PX(10, 19, red);
    CHECK_PX(19, 19, red);
    CHECK_PX(15, 10, red);
    CHECK_PX(10, 15, red);
    CHECK_PX(19, 15, red);
    CHECK_PX(15, 19, red);
    CHECK_PX(15, 15, 0);
    CHECK_EQ(Count(red), 36);
    Start("dot");
    port_ge_set_1dot_depth(sGe, 0x7fff);
    Color(Rgb(31, 0, 0));
    PolygonAttr(ATTR(31, 1));
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(2, -1, 0);
    Vtx16(1, -2, 0);
    Render(0);
    CHECK_PX(128, 96, red);
    CHECK_EQ(Count(red), 1);
}

// The edge walker (ds3d.c's Edge, melonDS's measurements), anti-aliasing off. Triangle A (10, 10), (50, 20), (10, 20)
// has an X-major right edge running right, slope 40 * (2^18 / 10) = 1048560 (4.0 less a little); on each line its
// run is the pixels whose middles lie between the edge at the top of the line and at the top of the next, 10.5 to
// 13.5 on line 10 and 14.5 to 17.5 on line 11, and a right edge running right is filled: A covers [10, 14) on line 10,
// [10, 18) on line 11, 4 * (y - 9) pixels on line y, 220 in all. Triangle B (10, 10), (60, 10), (50, 20) shares the
// edge as its left edge, which isn't filled: B starts where A ends, at 14 and 18. B's right edge runs left at 45
// degrees and isn't filled either: on line y its walker is at 60 - 1 - (y - 10), the pixel left of the edge's top end,
// and B stops before it, at 59 - (y - 10) on line y (45 pixels on line 10, 5 fewer each line)
static void TestEdgeWalker(void) {
    Start("edge walker");
    PolygonAttr(ATTR(31, 1));
    Begin(0);
    Color(Rgb(31, 0, 0));
    Vtx16(SX(10), SY(10), 0);
    Vtx16(SX(50), SY(20), 0);
    Vtx16(SX(10), SY(20), 0);
    Color(Rgb(0, 31, 0));
    Vtx16(SX(10), SY(10), 0);
    Vtx16(SX(60), SY(10), 0);
    Vtx16(SX(50), SY(20), 0);
    Render(0);
    uint32_t a = OUT(63, 0, 0, 31);
    uint32_t b = OUT(0, 63, 0, 31);
    CHECK_EQ(Count(a), 220);
    CHECK_EQ(Count(b), 225);
    CHECK_PX(13, 10, a);
    CHECK_PX(14, 10, b);
    CHECK_PX(17, 11, a);
    CHECK_PX(18, 11, b);
    CHECK_PX(58, 10, b);
    CHECK_PX(59, 10, 0);
    CHECK_PX(57, 11, b);
    CHECK_PX(58, 11, 0);
    CHECK_PX(49, 19, a);
    CHECK_PX(50, 19, 0);
}

// Anti-aliasing (coverage from the walker and the layer beneath, melonDS's measurements). Triangle (10, 10), (10, 40),
// (40, 10): its right edge runs left at 45 degrees; on line 20 the walker is at 40 - 1 - 10 = 29, the pixel left of the
// edge's top end (X 30), and at the line's middle the edge is at 29.5, half the pixel: the middle position's part
// within the pixel is 16/32 from its right side, and a right edge running left keeps 31 - 16 = 15. With anti-aliasing
// off that edge isn't filled and pixel 29 stays the rear plane; on, pixel 29 is drawn, the blue rear plane goes
// beneath it, and it shows (63 * 16 + 0 * 16) / 32 = 31 red and (0 * 16 + 63 * 16) / 32 = 31 blue. The vertical left
// edge covers its pixels fully
static void TestAntialias(void) {
    for (int on = 0; on < 2; on++) {
        Start(on ? "anti-aliasing" : "anti-aliasing (off)");
        sSrc.regs.disp3dcnt = on ? CNT_ANTIALIAS : 0;
        sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
        Color(Rgb(31, 0, 0));
        PolygonAttr(ATTR(31, 1));
        Begin(0);
        Vtx16(SX(10), SY(10), 0);
        Vtx16(SX(10), SY(40), 0);
        Vtx16(SX(40), SY(10), 0);
        Render(0);
        CHECK_PX(10, 20, OUT(63, 0, 0, 31));
        CHECK_PX(28, 20, OUT(63, 0, 0, 31));
        CHECK_PX(29, 20, on ? OUT(31, 0, 31, 31) : OUT(0, 0, 63, 31));
        CHECK_PX(30, 20, OUT(0, 0, 63, 31));
    }
}

// Two triangles sharing an edge of slope 0.5, AA on, a blue rear plane: neither the pixels of the edge nor any beside
// it (two either side, on lines 26-34, where both triangles are wider than that) show the rear plane, whichever
// triangle is drawn first and whether their polygon IDs are the same or not. Running right, from (100, 20) to (110,
// 40): the slope is 10 * (2^18 / 20) = 131070; on line 25 the walker is at 655350 (2.5 pixels), pixel 102, for both
// triangles. At the line's middle, ((655350 >> 9) + (131070 >> 10)) >> 4 = 87 is 23/32 into the pixel: the triangle on
// the left (red) covers 23, the one on the right (green) 31 - 23 = 8. The one drawn first is on top and the other goes
// beneath: red first shows (63 * 24 + 0 * 8) / 32 = 47 red and 63 * 8 / 32 = 15 green; green first, 63 * 23 / 32 = 45
// red and 63 * 9 / 32 = 17 green. Running left, from (110, 20) to (100, 40): the walker starts a pixel in, 917494 on
// line 25 (3.5 pixels), pixel 107, and the middle's 119 is 23/32 from the pixel's right side: the left triangle covers
// 31 - 23 = 8 and the right one 23
static void TestAntialiasSharedEdge(void) {
    for (int k = 0; k < 8; k++) {
        int left = k & 1; // the edge runs left
        int same = (k >> 1) & 1;
        int greenFirst = k >> 2;
        Start("anti-aliasing (shared edge)");
        sSrc.regs.disp3dcnt = CNT_ANTIALIAS;
        sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
        int tx = left ? 110 : 100;
        int bx = left ? 100 : 110;
        for (int t = 0; t < 2; t++) {
            int green = t != greenFirst;
            PolygonAttr(ATTR(31, green && !same ? 2 : 1));
            Color(green ? Rgb(0, 31, 0) : Rgb(31, 0, 0));
            Begin(0);
            Vtx16(SX(tx), SY(20), 0);
            if (green) {
                Vtx16(SX(tx + 30), SY(20), 0);
                Vtx16(SX(bx), SY(40), 0);
            } else {
                Vtx16(SX(bx), SY(40), 0);
                Vtx16(SX(left ? 70 : 80), SY(40), 0);
            }
        }
        Render(0);
        int clear = 0;
        for (int y = 26; y <= 34; y++) {
            int edge = tx + (bx - tx) * (2 * (y - 20) + 1) / 40;
            for (int x = edge - 2; x <= edge + 2; x++) {
                clear += (Px(x, y) & 0xff) != 0;
            }
        }
        CHECK_EQ(clear, 0);
        if (!left) {
            CHECK_PX(102, 25, greenFirst ? OUT(45, 17, 0, 31) : OUT(47, 15, 0, 31));
            CHECK_PX(101, 25, OUT(63, 0, 0, 31));
            CHECK_PX(103, 25, OUT(0, 63, 0, 31));
        } else {
            CHECK_PX(107, 25, greenFirst ? OUT(15, 47, 0, 31) : OUT(17, 45, 0, 31));
            CHECK_PX(106, 25, OUT(63, 0, 0, 31));
            CHECK_PX(108, 25, OUT(0, 63, 0, 31));
        }
    }
}

// Anti-aliasing with edge marking: the triangle (100, 20), (200, 40), (136, 60) has a Y-major left edge running right
// (slope 0.9), whose pixel on many lines has coverage 0. It is drawn all the same, with its edge flag, so edge marking
// marks it on every line, and a marked pixel's coverage becomes 16: it shows (63 * 17 + 0 * 15) / 32 = 33 of the red
// edge colour over the black rear plane beneath it
static void TestAntialiasEdgeMarking(void) {
    Start("anti-aliasing and edge marking");
    sSrc.regs.disp3dcnt = CNT_ANTIALIAS | CNT_EDGE_MARK;
    sSrc.regs.clear_color = 31u << 16;
    sSrc.regs.edge_color[0] = Rgb(31, 0, 0);
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    Begin(0);
    Vtx16(SX(100), SY(20), 0);
    Vtx16(SX(200), SY(40), 0);
    Vtx16(SX(136), SY(60), 0);
    Render(0);
    int unmarked = 0;
    for (int y = 21; y < 59; y++) {
        int x = 0;
        while (x < PORT_DS3D_WIDTH && port_ds3d_polygon_id(x, y) != 1) {
            x++;
        }
        unmarked += Px(x, y) != OUT(33, 0, 0, 31);
    }
    CHECK_EQ(unmarked, 0);
}

// A polygon facing the viewer passes "less" at an equal depth over an opaque polygon facing away, and only then: two
// quads on the same pixels at the same depth, the first red and the second green, in the four combinations of facing
static void BackQuad(int x0, int y0, int x1, int y1, int32_t z) {
    Begin(1);
    Vtx16(SX(x0), SY(y0), z);
    Vtx16(SX(x1), SY(y0), z);
    Vtx16(SX(x1), SY(y1), z);
    Vtx16(SX(x0), SY(y1), z);
}

static void TestFrontFacing(void) {
    Start("front facing");
    uint32_t red = OUT(63, 0, 0, 31);
    uint32_t green = OUT(0, 63, 0, 31);
    PolygonAttr(ATTR(31, 1));
    for (int k = 0; k < 4; k++) {
        int x = 10 + k * 20;
        for (int i = 0; i < 2; i++) {
            int front = i == 0 ? k & 1 : (k >> 1) & 1;
            Color(i == 0 ? Rgb(31, 0, 0) : Rgb(0, 31, 0));
            if (front) {
                Quad(x, 10, x + 10, 20, 0);
            } else {
                BackQuad(x, 10, x + 10, 20, 0);
            }
        }
    }
    Render(0);
    CHECK_PX(15, 15, red);   // back, then back: the first stays
    CHECK_PX(35, 15, red);   // front, then back: the first stays
    CHECK_PX(55, 15, green); // back, then front: the front wins
    CHECK_PX(75, 15, red);   // front, then front: the first stays
}

// W-buffering (SWAP_BUFFERS bit 1): the depth is W. Two quads on the same pixels with z = 0 in clip space but W 2.0
// and then 1.0: with Z both have depth 0x7ffe00 and the second fails "less"; with W the second is nearer
static void TestWBuffer(void) {
    for (int w = 0; w < 2; w++) {
        Start(w ? "w-buffer" : "w-buffer (z)");
        ProjectionPerspective();
        PolygonAttr(ATTR(31, 1));
        for (int k = 2; k >= 1; k--) {
            Color(k == 2 ? Rgb(31, 0, 0) : Rgb(0, 31, 0));
            Begin(1);
            Vtx16(SX(10) * k, SY(20) * k, -ONE * k);
            Vtx16(SX(20) * k, SY(20) * k, -ONE * k);
            Vtx16(SX(20) * k, SY(10) * k, -ONE * k);
            Vtx16(SX(10) * k, SY(10) * k, -ONE * k);
        }
        Render(w ? 2 : 0);
        CHECK_PX(15, 15, w ? OUT(0, 63, 0, 31) : OUT(63, 0, 0, 31));
        CHECK_HEX(port_ds3d_depth(15, 15), w ? ONE : 0x7ffe00);
    }
    // The depth is the W the polygon's 16-bit scaled W stands for: Ws of 0x1000f (red, first) and 0x10001 (green)
    // need 20 bits, are scaled by 4 bits and both stand for 0x10000, so the second isn't less. The projection
    // diag(w, w, w, w) keeps the quad on the same pixels
    Start("w-buffer (scaled)");
    PolygonAttr(ATTR(31, 1));
    for (int k = 0; k < 2; k++) {
        int32_t w = k == 0 ? 0x1000f : 0x10001;
        int32_t m[16] = { w, 0, 0, 0, 0, w, 0, 0, 0, 0, w, 0, 0, 0, 0, w };
        MtxMode(0);
        MtxLoad44(m);
        MtxMode(2);
        Color(k == 0 ? Rgb(31, 0, 0) : Rgb(0, 31, 0));
        Quad(10, 10, 20, 20, 0);
    }
    Render(2);
    CHECK_PX(15, 15, OUT(63, 0, 0, 31));
    CHECK_HEX(port_ds3d_depth(15, 15), 0x10000);
}

// Perspective-correct texture coordinates: a span from X 0 (W 1.0, s = 0) to X 64 (W 3.0, s = 1024, 64 texels). At X
// 32 the far end weighs (32 * 0x1000 << 8) / (32 * 0x1000 + 32 * 0x3000) = 64 / 256: s = 1024 * 64 / 256 = 256,
// texel 16 (linear interpolation would give 32); at X 63, 244 / 256: s = 976, texel 61
static void TestPerspective(void) {
    Start("perspective");
    sSrc.regs.disp3dcnt = CNT_TEXTURE;
    for (int x = 0; x < 128; x++) {
        for (int y = 0; y < 8; y++) {
            Texel16(0, x, y, 128, Rgb(x & 31, x >> 5, 0) | 0x8000);
        }
    }
    ProjectionPerspective();
    Color(0x7fff);
    PolygonAttr(ATTR(31, 1));
    TexImageParam(TEX(0, 4, 0, 7));
    Begin(1);
    TexCoord(0, 0);
    Vtx16(-ONE, 0, -ONE); // screen (0, 96)
    TexCoord(1024, 0);
    Vtx16(-0x1800, 0, -3 * ONE); // (64, 96)
    TexCoord(1024, 0);
    Vtx16(-0x1800, 3 * ONE, -3 * ONE); // (64, 0)
    TexCoord(0, 0);
    Vtx16(-ONE, ONE, -ONE); // (0, 0)
    Render(0);
    CHECK_PX(0, 10, OUT(0, 0, 0, 31));
    CHECK_PX(32, 10, OUT(W(16), 0, 0, 31));
    CHECK_PX(63, 10, OUT(W(29), W(1), 0, 31));
    CHECK_PX(64, 10, 0);
}

// Sorting: translucent polygons by Y (bottom line, then top) unless SWAP_BUFFERS bit 0 keeps their order; opaque ones
// always by Y. P1 (red, lines 50-99) comes before P2 (green, lines 10-59); by Y, P2 is drawn first
static void TestSorting(void) {
    for (int manual = 0; manual < 2; manual++) {
        Start(manual ? "sorting (manual)" : "sorting (auto)");
        sSrc.regs.disp3dcnt = CNT_BLEND;
        sSrc.regs.clear_color = Rgb(0, 0, 31) | 31u << 16;
        Color(Rgb(31, 0, 0));
        PolygonAttr(ATTR(15, 1));
        Quad(0, 50, 40, 100, 0);
        Color(Rgb(0, 31, 0));
        PolygonAttr(ATTR(15, 2));
        Quad(0, 10, 40, 60, 0);
        // Opaque at equal depth: whichever is drawn first stays
        Color(Rgb(31, 0, 0));
        PolygonAttr(ATTR(31, 3));
        Quad(100, 50, 140, 100, 0);
        Color(Rgb(0, 31, 0));
        Quad(100, 10, 140, 60, 0);
        Render(manual ? 1 : 0);
        // Auto: green over blue (0, 31, 31), then red: (31, 15, 15). Manual: red first (31, 0, 31), then green
        // (15, 31, 15)
        CHECK_PX(20, 55, manual ? OUT(15, 31, 15, 31) : OUT(31, 15, 15, 31));
        CHECK_PX(120, 55, OUT(0, 63, 0, 31));
    }
}

int main(void) {
    sGe = port_ge_create();
    sTex = calloc(512 * 1024, 1);
    sPltt = calloc(96 * 1024, 1);
    sOut = calloc(PORT_DS3D_WIDTH * PORT_DS3D_HEIGHT, sizeof(*sOut));
    if (sGe == NULL || sTex == NULL || sPltt == NULL || sOut == NULL) {
        printf("out of memory\n");
        return 1;
    }
    for (int i = 0; i < 32; i++) {
        sSrc.tex_pages[i] = sTex + i * 0x4000;
    }
    for (int i = 0; i < 6; i++) {
        sSrc.texpltt_pages[i] = sPltt + i * 0x4000;
    }
    TestClear();
    TestCoverage();
    TestSharedEdge();
    TestDepth();
    TestTranslucent();
    TestTextures();
    TestModes();
    TestFog();
    TestEdgeMarking();
    TestShadow();
    TestWireframeAndDot();
    TestEdgeWalker();
    TestAntialias();
    TestAntialiasSharedEdge();
    TestAntialiasEdgeMarking();
    TestFrontFacing();
    TestWBuffer();
    TestPerspective();
    TestSorting();
    port_ge_destroy(sGe);
    free(sTex);
    free(sPltt);
    free(sOut);
    printf("%d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
