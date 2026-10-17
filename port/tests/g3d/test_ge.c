// Unit tests of port/gfx/ge, the geometry engine. Each test writes a command stream to the engine as the game does and
// checks readbacks and polygon RAM against values worked out by hand in the comments, from GBATEK's arithmetic (20.12
// matrices, sums of products shifted right by 12, rounding down) and, where ge.c says it follows melonDS's
// measurements, from those. No ROM is needed. It prints each failure and returns nonzero if there was one.
#include "g3d_test.h"
#include <stdlib.h>
#include <string.h>

static void Start(const char *name) {
    sTest = name;
    port_ge_reset(sGe);
}

static void CheckClip(int line, const int32_t want[16]) {
    uint32_t got[16];
    port_ge_clip_matrix(sGe, got);
    for (int i = 0; i < 16; i++) {
        CheckHex(line, "CLIPMTX_RESULT", got[i], (uint32_t)want[i]);
    }
}

static void CheckVec(int line, const int32_t want[9]) {
    uint32_t got[9];
    port_ge_vector_matrix(sGe, got);
    for (int i = 0; i < 9; i++) {
        CheckHex(line, "VECMTX_RESULT", got[i], (uint32_t)want[i]);
    }
}

static uint32_t PosLevel(void) {
    return (port_ge_gxstat(sGe) >> 8) & 31;
}

static int StackError(void) {
    return (port_ge_gxstat(sGe) >> 15) & 1;
}

static const PortGeList *Building(void) {
    return port_ge_building_list(sGe);
}

static const PortGeVertex *PolyVertex(const PortGeList *list, uint32_t polygon, int i) {
    return &list->vertices[list->polygons[polygon].vertices[i]];
}

static void CheckVertex(int line, const PortGeVertex *v, int x, int y, int32_t z, int r, int g, int b) {
    CheckEq(line, "screen x", v->x, x);
    CheckEq(line, "screen y", v->y, y);
    CheckHex(line, "depth", (uint32_t)v->z, (uint32_t)z);
    CheckEq(line, "red", v->color[0], r);
    CheckEq(line, "green", v->color[1], g);
    CheckEq(line, "blue", v->color[2], b);
}

// A small triangle in the middle of the screen, front facing
static void Triangle(void) {
    Vtx16(0, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0);
}

static const int32_t kIdentity[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE };

// GXFIFO's packed words (GBATEK "Packed commands"): up to four command codes, lowest byte first, then the parameters
// of each in turn; commands without parameters run at once, and a word of NOPs takes none
static void TestFifo(void) {
    Start("fifo");
    MtxMode(0);
    MtxIdentity();
    // MTX_MODE (1 parameter), MTX_PUSH and MTX_IDENTITY (none), MTX_SCALE (3)
    port_ge_write(sGe, FIFO, 0x1b151110);
    port_ge_write(sGe, FIFO, 1);
    port_ge_write(sGe, FIFO, 0x2000);
    port_ge_write(sGe, FIFO, 0x3000);
    port_ge_write(sGe, FIFO, 0x4000);
    CHECK_EQ(PosLevel(), 1);
    // The position matrix is diag(2, 3, 4, 1) and the projection the identity, so the clip matrix is the same
    int32_t scaled[16] = { 0x2000, 0, 0, 0, 0, 0x3000, 0, 0, 0, 0, 0x4000, 0, 0, 0, 0, ONE };
    CheckClip(__LINE__, scaled);
    // A word of four NOPs; the next word is a new packed word with MTX_IDENTITY alone
    port_ge_write(sGe, FIFO, 0);
    port_ge_write(sGe, FIFO, 0x15);
    CheckClip(__LINE__, kIdentity);
    // MTX_TRANS, an undefined code (0x1d), MTX_MODE: the trans's three parameters, then the mode's (projection)
    port_ge_write(sGe, FIFO, 0x00101d1c);
    port_ge_write(sGe, FIFO, ONE);
    port_ge_write(sGe, FIFO, 2 * ONE);
    port_ge_write(sGe, FIFO, 3 * ONE);
    port_ge_write(sGe, FIFO, 0);
    // In projection mode now: scaling by 2 makes clip = pos * diag(2, 2, 2, 1), pos's last row (1, 2, 3, 1)
    MtxScale(0x2000, 0x2000, 0x2000);
    int32_t want[16] = { 0x2000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, 0x2000, 0, 0x2000, 0x4000, 0x6000, ONE };
    CheckClip(__LINE__, want);
    // A log, as port_hw_g3_take gives it, runs the same way: port writes and FIFO words interleaved
    const PortG3Write log[] = {
        { PORT(0x10), 1 }, { FIFO, 0x15 }, { PORT(0x1b), 0x3000 }, { PORT(0x1b), ONE }, { PORT(0x1b), ONE },
    };
    port_ge_run(sGe, log, sizeof(log) / sizeof(log[0]));
    // pos = diag(3, 1, 1, 1), proj = diag(2, 2, 2, 1)
    int32_t want2[16] = { 0x6000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, ONE };
    CheckClip(__LINE__, want2);
    // A command split over two log calls waits for its last parameter
    const PortG3Write first[] = { { PORT(0x1c), ONE }, { PORT(0x1c), 0 } };
    const PortG3Write second[] = { { PORT(0x1c), 0 } };
    port_ge_run(sGe, first, 2);
    CheckClip(__LINE__, want2);
    port_ge_run(sGe, second, 1);
    // pos's last row becomes (3, 0, 0, 1): row 0 of pos times 1.0 added; clip row 3 = (3 * 2, 0, 0, 1)
    int32_t want3[16] = { 0x6000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, 0x2000, 0, 0x6000, 0, 0, ONE };
    CheckClip(__LINE__, want3);
}

// MTX_MULT_4x4: each element is the sum of four 20.12 products, shifted right by 12 with rounding down (also for
// negative sums), kept in 64 bits and cut to 32
static void TestMultiply(void) {
    Start("multiply");
    MtxMode(1);
    MtxIdentity();
    MtxMode(0);
    int32_t m[16] = { -3, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE };
    int32_t a[16] = { 0x800, 1, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE };
    MtxLoad44(m);
    MtxMult44(a);
    // Row 0: (0x800 * -3 + 1 * 0) >> 12 = -6144 >> 12 = floor(-1.5) = -2; (0x800 * 0 + 1 * 0x1000) >> 12 = 1. The
    // other rows are the identity's times m. With pos the identity, the clip matrix is the projection matrix
    int32_t want[16] = { -2, 1, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE };
    CheckClip(__LINE__, want);
    // (2^31 - 1)^2 = 2^62 - 2^32 + 1; >> 12 = 2^50 - 2^20, whose low 32 bits are 0xfff00000
    int32_t big[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, 0x7fffffff };
    MtxLoad44(big);
    MtxMult44(big);
    int32_t want2[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, (int32_t)0xfff00000 };
    CheckClip(__LINE__, want2);
}

// MTX_LOAD_4x3, MTX_SCALE, MTX_MULT_3x3, MTX_TRANS in mode 2 (position and vector together; scaling leaves the vector
// matrix alone), mode 1 (position only) and mode 3 (texture)
static void TestMatrixForms(void) {
    Start("matrix forms");
    MtxMode(0);
    MtxIdentity();
    MtxMode(2);
    int32_t m43[12] = { ONE, 0, 0, 0, ONE, 0, 0, 0, ONE, 0x100, 0x200, 0x300 };
    MtxLoad43(m43);
    int32_t loaded[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 0x100, 0x200, 0x300, ONE };
    int32_t identity3[9] = { ONE, 0, 0, 0, ONE, 0, 0, 0, ONE };
    CheckClip(__LINE__, loaded);
    CheckVec(__LINE__, identity3);
    // diag(2, 3, 0.5, 1) * pos: rows 0-2 scaled, row 3 kept
    MtxScale(0x2000, 0x3000, 0x800);
    int32_t scaled[16] = { 0x2000, 0, 0, 0, 0, 0x3000, 0, 0, 0, 0, 0x800, 0, 0x100, 0x200, 0x300, ONE };
    CheckClip(__LINE__, scaled);
    CheckVec(__LINE__, identity3);
    // R * pos with R's rows (0, 1, 0), (-1, 0, 0), (0, 0, 1): row 0 = pos row 1, row 1 = -pos row 0
    int32_t r[9] = { 0, ONE, 0, -ONE, 0, 0, 0, 0, ONE };
    MtxMult33(r);
    int32_t rotated[16] = { 0, 0x3000, 0, 0, -0x2000, 0, 0, 0, 0, 0, 0x800, 0, 0x100, 0x200, 0x300, ONE };
    CheckClip(__LINE__, rotated);
    CheckVec(__LINE__, r);
    // Translation by (1, 0, 0): row 3 += 1.0 * row 0 = (0, 0x3000, 0, 0)
    MtxTrans(ONE, 0, 0);
    int32_t moved[16] = { 0, 0x3000, 0, 0, -0x2000, 0, 0, 0, 0, 0, 0x800, 0, 0x100, 0x3200, 0x300, ONE };
    CheckClip(__LINE__, moved);
    CheckVec(__LINE__, r);
    // Mode 1: the position matrix only; R * pos: row 0 = (-0x2000, 0, 0, 0), row 1 = -(0, 0x3000, 0, 0)
    MtxMode(1);
    MtxMult33(r);
    int32_t again[16] = { -0x2000, 0, 0, 0, 0, -0x3000, 0, 0, 0, 0, 0x800, 0, 0x100, 0x3200, 0x300, ONE };
    CheckClip(__LINE__, again);
    CheckVec(__LINE__, r);
    // Mode 3: the texture matrix, which the clip matrix doesn't involve
    MtxMode(3);
    MtxScale(0x5000, 0x5000, 0x5000);
    CheckClip(__LINE__, again);
}

// The matrix stacks (GBATEK "Matrix Stack"; ge.c for the overflow cases melonDS measured)
static void TestStacks(void) {
    Start("stacks");
    MtxMode(0);
    MtxIdentity();
    MtxMode(2);
    int32_t p1[12] = { ONE, 0, 0, 0, ONE, 0, 0, 0, ONE, ONE, 0, 0 };
    int32_t p2[12] = { ONE, 0, 0, 0, ONE, 0, 0, 0, ONE, 2 * ONE, 0, 0 };
    int32_t p3[12] = { ONE, 0, 0, 0, ONE, 0, 0, 0, ONE, 3 * ONE, 0, 0 };
    int32_t c1[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, ONE, 0, 0, ONE };
    int32_t c2[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 2 * ONE, 0, 0, ONE };
    int32_t c3[16] = { ONE, 0, 0, 0, 0, ONE, 0, 0, 0, 0, ONE, 0, 3 * ONE, 0, 0, ONE };
    MtxLoad43(p1);
    MtxPush();
    MtxIdentity();
    MtxPush();
    MtxLoad43(p3);
    MtxPush();
    CHECK_EQ(PosLevel(), 3);
    CHECK_EQ(StackError(), 0);
    MtxPop(1); // pointer 2: entry 2 is p3
    CheckClip(__LINE__, c3);
    MtxPop(2); // pointer 0: entry 0 is p1
    CheckClip(__LINE__, c1);
    CHECK_EQ(PosLevel(), 0);
    CHECK_EQ(StackError(), 0);
    // Below the bottom: the 6-bit pointer wraps to 63, past 30, which is an error; GXSTAT shows its low 5 bits
    MtxPop(1);
    CHECK_EQ(PosLevel(), 31);
    CHECK_EQ(StackError(), 1);
    // Writing bit 15 acknowledges the error
    port_ge_write_gxstat(sGe, 0x8000);
    CHECK_EQ(StackError(), 0);
    MtxPop(-1); // 63 + 1 wraps to 0
    CHECK_EQ(PosLevel(), 0);
    CHECK_EQ(StackError(), 0);
    // MTX_STORE and MTX_RESTORE by index; index 31 is an error
    MtxLoad43(p2);
    MtxStore(5);
    MtxIdentity();
    MtxRestore(5);
    CheckClip(__LINE__, c2);
    CHECK_EQ(StackError(), 0);
    MtxStore(31);
    CHECK_EQ(StackError(), 1);
    port_ge_write_gxstat(sGe, 0x8000);
    // 31 pushes fit (pointers 0..30), the 32nd overflows
    for (int i = 0; i < 31; i++) {
        MtxPush();
    }
    CHECK_EQ(PosLevel(), 31);
    CHECK_EQ(StackError(), 0);
    MtxPush();
    CHECK_EQ(StackError(), 1);
    CHECK_EQ(PosLevel(), 0); // 32: its low 5 bits
    port_ge_write_gxstat(sGe, 0x8000);
    MtxPop(31);
    MtxPop(1);
    CHECK_EQ(PosLevel(), 0);
    CHECK_EQ(StackError(), 0);
    // The projection stack: one entry, its level in GXSTAT bit 13; a second push is an error and stores nothing
    MtxMode(1);
    MtxIdentity();
    MtxMode(0);
    MtxScale(0x2000, 0x2000, 0x2000);
    CHECK_EQ((port_ge_gxstat(sGe) >> 13) & 1, 0);
    MtxPush();
    CHECK_EQ((port_ge_gxstat(sGe) >> 13) & 1, 1);
    MtxIdentity();
    MtxPush();
    CHECK_EQ(StackError(), 1);
    port_ge_write_gxstat(sGe, 0x8000);
    MtxPop(1);
    CHECK_EQ((port_ge_gxstat(sGe) >> 13) & 1, 0);
    int32_t doubled[16] = { 0x2000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, 0x2000, 0, 0, 0, 0, ONE };
    CheckClip(__LINE__, doubled);
    CHECK_EQ(StackError(), 0);
    MtxPop(1);
    CHECK_EQ(StackError(), 1);
    // The texture stack has one entry too
    port_ge_write_gxstat(sGe, 0x8000);
    MtxMode(3);
    MtxPush();
    CHECK_EQ(StackError(), 0);
    MtxPush();
    CHECK_EQ(StackError(), 1);
}

// The vertex commands (GBATEK "Vertices"): VTX_16 (4.12), VTX_10 (1.3.6), VTX_XY/XZ/YZ (one coordinate kept),
// VTX_DIFF (10-bit differences added to the 4.12 coordinates as they are). With identity matrices and the whole
// screen, w = 1.0, screen X = (x + 0x1000) * 256 / 0x2000 = (x + 0x1000) / 32, screen Y = (0x1000 - y) * 192 / 0x2000,
// depth = ((z * 0x4000 / 0x1000) + 0x3fff) * 0x200. A 5-bit colour c reaches polygon RAM as c * 16 + 15
static void TestVertices(void) {
    Start("vertices");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Color(0x7fff); // 31: 511
    TexCoord(0x10, -0x20);
    Vtx16(0, 0, 0);       // (128, 96), depth 0x3fff * 0x200 = 0x7ffe00
    Vtx10(0x20, 0, 0x10); // x = 0x20 << 6 = 0x800: X = 0x1800 / 32 = 192; z = 0x400: (0x1000 + 0x3fff) * 0x200
    Color(Rgb(1, 2, 31)); // 31, 47, 511
    VtxXY(0, 0x800);      // z stays 0x400; Y = 0x800 * 192 / 0x2000 = 48
    const PortGeList *list = Building();
    CHECK_EQ(list->polygon_count, 1);
    CHECK_EQ(list->vertex_count, 3);
    CheckVertex(__LINE__, PolyVertex(list, 0, 0), 128, 96, 0x7ffe00, 511, 511, 511);
    CheckVertex(__LINE__, PolyVertex(list, 0, 1), 192, 96, 0x9ffe00, 511, 511, 511);
    CheckVertex(__LINE__, PolyVertex(list, 0, 2), 128, 48, 0x9ffe00, 31, 47, 511);
    CHECK_EQ(PolyVertex(list, 0, 0)->w, ONE);
    CHECK_EQ(PolyVertex(list, 0, 0)->s, 0x10);
    CHECK_EQ(PolyVertex(list, 0, 0)->t, -0x20);
    CHECK_EQ(list->polygons[0].flags, PORT_GE_POLYGON_FRONT);
    // (0x400, 0x800, 0): X = 0x1400 / 32 = 160, Y 48
    VtxXZ(0x400, 0);
    // (0x400, -0x400, -0x800): Y = 0x1400 * 192 / 0x2000 = 120; depth (-0x2000 + 0x3fff) * 0x200 = 0x3ffe00
    VtxYZ(-0x400, -0x800);
    // + (0x100, -0x200, 0): (0x500, -0x600, -0x800): X = 0x1500 / 32 = 168, Y = 0x1600 * 192 / 0x2000 = 132
    VtxDiff(0x100, -0x200, 0);
    CHECK_EQ(list->polygon_count, 2);
    CheckVertex(__LINE__, PolyVertex(list, 1, 0), 160, 48, 0x7ffe00, 31, 47, 511);
    CheckVertex(__LINE__, PolyVertex(list, 1, 1), 160, 120, 0x3ffe00, 31, 47, 511);
    CheckVertex(__LINE__, PolyVertex(list, 1, 2), 168, 132, 0x3ffe00, 31, 47, 511);
    CHECK_EQ(port_ge_polygon_count(sGe), 2);
    CHECK_EQ(port_ge_vertex_count(sGe), 6);
    // TEXIMAGE_PARAM and PLTT_BASE as they were at the polygon's last vertex, POLYGON_ATTR as BEGIN_VTXS latched it
    PolygonAttr(ATTR(7, 9));
    TexImageParam(0x12345678 & 0x3fffffff);
    PlttBase(0x1abc);
    Triangle();
    CHECK_HEX(list->polygons[2].attr, ATTR(31, 0));
    CHECK_HEX(list->polygons[2].tex_param, 0x12345678 & 0x3fffffff);
    CHECK_HEX(list->polygons[2].pltt_base, 0x1abc);
}

// Culling by facing: counter-clockwise as seen is the front (POLYGON_ATTR bit 7 draws it, bit 6 the back)
static void TestCulling(void) {
    Start("culling");
    Basic();
    PolygonAttr(0x001f0000 | ATTR_FRONT_ONLY);
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(0x800, 0, 0);
    Vtx16(0, 0x800, 0); // counter-clockwise: drawn
    Vtx16(0, 0, 0);
    Vtx16(0, 0x800, 0);
    Vtx16(0x800, 0, 0); // clockwise: culled
    CHECK_EQ(Building()->polygon_count, 1);
    CHECK_EQ(Building()->polygons[0].flags, PORT_GE_POLYGON_FRONT);
    PolygonAttr(0x001f0000 | ATTR_BACK_ONLY);
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(0x800, 0, 0);
    Vtx16(0, 0x800, 0); // culled
    Vtx16(0, 0, 0);
    Vtx16(0, 0x800, 0);
    Vtx16(0x800, 0, 0); // drawn, as a back face
    CHECK_EQ(Building()->polygon_count, 2);
    CHECK_EQ(Building()->polygons[1].flags, 0);
    PolygonAttr(0x001f0000);
    Begin(0);
    Triangle();
    CHECK_EQ(Building()->polygon_count, 2);
    // Vertices in line (a line made of a polygon) turn neither way: never culled, even with neither face drawn, and
    // front for the depth test
    for (uint32_t faces = 0; faces < 4; faces++) {
        PolygonAttr(0x001f0000 | faces << 6);
        Begin(0);
        Vtx16(0, 0, 0);
        Vtx16(0x400, 0, 0);
        Vtx16(0x800, 0, 0);
        CHECK_EQ(Building()->polygon_count, 3 + faces);
        CHECK_EQ(Building()->polygons[2 + faces].flags, PORT_GE_POLYGON_FRONT);
    }
}

// Clipping to the view volume: an outside vertex gives way to the points where its edges to inside neighbours meet
// the plane, the previous neighbour's first, each interpolated from the outside vertex by
// (w_out - x_out) / ((w_out - x_out) - (w_in - x_in)); colours go through as (c << 12) + 0xfff and are cut back to
// their 5 bits after each plane
static void TestClipping(void) {
    Start("clipping");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Color(Rgb(31, 0, 0));
    Vtx16(0, 0, 0); // A, inside
    Color(Rgb(0, 31, 0));
    Vtx16(0x2000, 0, 0); // B, beyond x = w
    Color(Rgb(0, 0, 31));
    Vtx16(0, 0x800, 0); // C, inside
    // B to A: (0x1000 - 0x2000) / ((0x1000 - 0x2000) - (0x1000 - 0)) = 1/2: x = 0x1000 (X 256); red
    // 0xfff + (0x1ffff - 0xfff) / 2 = 0x107ff and green 0x1ffff - 0xf800 = 0x107ff, cut back to 0x10fff: 16 each, which
    // the rasterizer gets as 16 * 16 + 15 = 271
    // B to C: 1/2 again: y = 0x400 (Y = 0xc00 * 192 / 0x2000 = 72), green and blue 271
    const PortGeList *list = Building();
    CHECK_EQ(list->polygon_count, 1);
    CHECK_EQ(list->polygons[0].vertex_count, 4);
    CHECK_EQ(list->vertex_count, 4);
    CheckVertex(__LINE__, PolyVertex(list, 0, 0), 128, 96, 0x7ffe00, 511, 0, 0);
    CheckVertex(__LINE__, PolyVertex(list, 0, 1), 256, 96, 0x7ffe00, 271, 271, 0);
    CheckVertex(__LINE__, PolyVertex(list, 0, 2), 256, 72, 0x7ffe00, 0, 271, 271);
    CheckVertex(__LINE__, PolyVertex(list, 0, 3), 128, 48, 0x7ffe00, 0, 0, 511);
    // Wholly beyond the near plane (z < -w): nothing
    Vtx16(0, 0, -0x2000);
    Vtx16(0x400, 0, -0x2000);
    Vtx16(0, 0x400, -0x2000);
    CHECK_EQ(list->polygon_count, 1);
    // Crossing the far plane: hidden without POLYGON_ATTR bit 12
    Vtx16(0, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0x1800);
    CHECK_EQ(list->polygon_count, 1);
    // ...and clipped with it: the far vertex becomes two on z = w, depth (0x4000 + 0x3fff) * 0x200 = 0xfffe00
    PolygonAttr(ATTR(31, 0) | ATTR_FAR_CLIP);
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0x1800);
    CHECK_EQ(list->polygon_count, 2);
    CHECK_EQ(list->polygons[1].vertex_count, 4);
    CHECK_HEX(PolyVertex(list, 1, 2)->z, 0xfffe00);
    CHECK_HEX(PolyVertex(list, 1, 3)->z, 0xfffe00);
    CHECK_HEX(PolyVertex(list, 1, 0)->z, 0x7ffe00);
    // A corner: Y is clipped before X (melonDS's order). A (0, 0), B (0x2000, 0x1800) beyond x = w and y = w, C
    // (0, 0x800). Against y = w, B gives way to the point toward A, 1/3 of the way: x = 0x2000 - 0x2000 / 3 = 0x1556
    // (rounded toward zero), and toward C, 1/2: (0x1000, 0x1000). Against x = w, (0x1556, 0x1000) gives way to the
    // point toward A, (0x1000, 0x1000 - 0x1000 * 0x556 / 0x1556) = (0x1000, 0xc00) on screen (256, 24), and toward
    // (0x1000, 0x1000), which is that point itself: five vertices, two of them (256, 0). X first would give four
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(0x2000, 0x1800, 0);
    Vtx16(0, 0x800, 0);
    CHECK_EQ(list->polygon_count, 3);
    CHECK_EQ(list->polygons[2].vertex_count, 5);
    static const int kCorner[5][2] = { { 128, 96 }, { 256, 24 }, { 256, 0 }, { 256, 0 }, { 128, 48 } };
    for (int i = 0; i < 5; i++) {
        CheckEq(__LINE__, "corner x", PolyVertex(list, 2, i)->x, kCorner[i][0]);
        CheckEq(__LINE__, "corner y", PolyVertex(list, 2, i)->y, kCorner[i][1]);
    }
}

// Strips share vertices in vertex RAM; a triangle strip's every second triangle is taken in the order 1, 0, 2 and a
// quad strip's quads in the order 0, 1, 3, 2, so all face the same way
static void TestStrips(void) {
    Start("strips");
    Basic();
    PolygonAttr(0x001f0000 | ATTR_FRONT_ONLY);
    Begin(2);
    Vtx16(0, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0);
    Vtx16(0x400, 0x400, 0);
    Vtx16(0, 0x800, 0);
    const PortGeList *list = Building();
    CHECK_EQ(list->polygon_count, 3);
    CHECK_EQ(list->vertex_count, 5);
    const uint16_t tri[3][3] = { { 0, 1, 2 }, { 2, 1, 3 }, { 2, 3, 4 } };
    for (int p = 0; p < 3; p++) {
        for (int i = 0; i < 3; i++) {
            CHECK_EQ(list->polygons[p].vertices[i], tri[p][i]);
        }
    }
    // Quads (v0, v1, v3, v2) and (v2, v3, v5, v4): the first stores its four in that order (5, 6, 7, 8 for v0, v1,
    // v3, v2), the second shares v2 and v3 and stores v5 and v4
    Begin(3);
    Vtx16(0, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0);
    Vtx16(0x400, 0x400, 0);
    Vtx16(0, 0x800, 0);
    Vtx16(0x400, 0x800, 0);
    CHECK_EQ(list->polygon_count, 5);
    CHECK_EQ(list->vertex_count, 11);
    const uint16_t quad[2][4] = { { 5, 6, 7, 8 }, { 8, 7, 9, 10 } };
    for (int p = 0; p < 2; p++) {
        for (int i = 0; i < 4; i++) {
            CHECK_EQ(list->polygons[3 + p].vertices[i], quad[p][i]);
        }
    }
    // Separate triangles share nothing
    Begin(0);
    Triangle();
    Triangle();
    CHECK_EQ(list->polygon_count, 7);
    CHECK_EQ(list->vertex_count, 17);
    CHECK_EQ(port_ge_polygon_count(sGe), 7);
    CHECK_EQ(port_ge_vertex_count(sGe), 17);
    // A clipped polygon shares nothing, and the next can't share with it: the first triangle loses a corner to the
    // left plane (4 vertices), the second stores its 3, the third shares two and stores 1
    Start("strips (clipped)");
    Basic();
    PolygonAttr(0x001f0000 | ATTR_FRONT_ONLY);
    Begin(2);
    Vtx16(-0x1400, 0, 0);
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0);
    Vtx16(0x400, 0x400, 0);
    Vtx16(0, 0x800, 0);
    list = Building();
    CHECK_EQ(list->polygon_count, 3);
    CHECK_EQ(list->polygons[0].vertex_count, 4);
    CHECK_EQ(list->vertex_count, 8);
}

// Polygon RAM holds 2048 polygons and 6144 vertices; what doesn't fit is dropped and sets the overflow flag
static void TestRamLimits(void) {
    Start("ram limits");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    for (int i = 0; i < 2049; i++) {
        Vtx16(0, 0, 0);
        Vtx16(0x40, 0, 0);
        Vtx16(0, 0x40, 0);
    }
    CHECK_EQ(Building()->polygon_count, 2048);
    CHECK_EQ(Building()->vertex_count, 6144);
    CHECK_EQ(port_ge_ram_overflow(sGe), 1);
    port_ge_ack_ram_overflow(sGe);
    CHECK_EQ(port_ge_ram_overflow(sGe), 0);
    Start("ram limits (vertices)");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(1);
    for (int i = 0; i < 1537; i++) {
        Vtx16(0, 0, 0);
        Vtx16(0x40, 0, 0);
        Vtx16(0x40, 0x40, 0);
        Vtx16(0, 0x40, 0);
    }
    CHECK_EQ(Building()->polygon_count, 1536);
    CHECK_EQ(Building()->vertex_count, 6144);
    CHECK_EQ(port_ge_ram_overflow(sGe), 1);
    // A strip's polygon needs room only for the vertices it doesn't share: 1535 quads use 6140 vertices, a strip's
    // first triangle 3 more and its second the last one
    Start("ram limits (strips)");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(1);
    for (int i = 0; i < 1535; i++) {
        Vtx16(0, 0, 0);
        Vtx16(0x40, 0, 0);
        Vtx16(0x40, 0x40, 0);
        Vtx16(0, 0x40, 0);
    }
    Begin(2);
    Vtx16(0, 0, 0);
    Vtx16(0x40, 0, 0);
    Vtx16(0, 0x40, 0);
    Vtx16(0x40, 0x40, 0);
    CHECK_EQ(Building()->polygon_count, 1537);
    CHECK_EQ(Building()->vertex_count, 6144);
    CHECK_EQ(port_ge_ram_overflow(sGe), 0);
}

// SWAP_BUFFERS closes the list; the VBlank swaps it in. A second SWAP_BUFFERS before the VBlank waits, with
// everything after it
static void TestSwap(void) {
    Start("swap");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Triangle();
    SwapBuffers(2);
    CHECK_EQ(port_ge_swap_pending(sGe), 1);
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 0);
    CHECK_EQ(port_ge_polygon_count(sGe), 0);
    CHECK_EQ(port_ge_vblank(sGe), 1);
    CHECK_EQ(port_ge_swap_pending(sGe), 0);
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 1);
    CHECK_EQ(port_ge_render_list(sGe)->swap_param, 2);
    Triangle();
    Triangle();
    SwapBuffers(1);
    Triangle();
    SwapBuffers(0);
    Triangle();
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 1);
    CHECK_EQ(port_ge_polygon_count(sGe), 1); // the triangle between the swaps
    CHECK(port_ge_stalled_entries(sGe) > 0);
    CHECK_EQ(port_ge_vblank(sGe), 1);
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 2);
    CHECK_EQ(port_ge_render_list(sGe)->swap_param, 1);
    CHECK_EQ(port_ge_swap_pending(sGe), 1); // the second swap ran at the VBlank
    CHECK_EQ(port_ge_stalled_entries(sGe), 0);
    CHECK_EQ(port_ge_polygon_count(sGe), 1); // the last triangle
    CHECK_EQ(port_ge_vblank(sGe), 1);
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 1);
    CHECK_EQ(port_ge_render_list(sGe)->swap_param, 0);
    CHECK_EQ(port_ge_vblank(sGe), 0);
    CHECK_EQ(port_ge_render_list(sGe)->polygon_count, 1);
}

// Lighting (GBATEK "Polygon Light Parameters", with ge.c's arithmetic, melonDS's measurement), identity matrices. A
// light vector l keeps -l (its components shifted right by 12 after the identity's 0x1000) and the specular divisor
// 2^18 / (512 - l.z); the normal n stays n. dot = sum of (-l_i * n_i) >> 9; diffuse = dot; s = dot + n.z, specular
// = (((s * s >> 10) & 0x3ff) * divisor >> 8) - 512 within 0..511. Each channel: (emission << 14 + diffuse colour *
// light * dot (cut to 20 bits) + (specular colour * level + ambient * 512) * light) >> 14, at most 31; polygon RAM
// has c * 16 + 15
static int32_t LitRed(int32_t lx, int32_t ly, int32_t lz, int32_t nx, int32_t ny, int32_t nz, uint16_t diffuse,
                      uint16_t specular) {
    PolygonAttr(ATTR(31, 0) | 1); // light 0
    Begin(0);
    LightVector(0, lx, ly, lz);
    LightColor(0, 0x7fff);
    DifAmb(diffuse, 0, 0);
    SpeEmi(specular, 0, 0);
    Normal(nx, ny, nz);
    Triangle();
    const PortGeList *list = Building();
    return PolyVertex(list, list->polygon_count - 1, 0)->color[0];
}

static void TestLighting(void) {
    Start("lighting");
    Basic();
    PolygonAttr(ATTR(31, 0) | 1); // light 0
    Begin(0);
    // Light (0, 0, -512) and normal (0, 0, 511): -l = (0, 0, 512), divisor 2^18 / 1024 = 256; dot = 512 * 511 >> 9 =
    // 511; s = 511 + 511 = 1022, 1022 * 1022 >> 10 = 1020, specular level (1020 * 256 >> 8) - 512 = 508
    LightVector(0, 0, 0, -512);
    LightColor(0, 0x7fff);
    DifAmb(Rgb(16, 8, 31), Rgb(4, 0, 0), 0);
    SpeEmi(0, Rgb(1, 2, 3), 0);
    Normal(0, 0, 511);
    // red   = (1 << 14) + 16 * 31 * 511 + 4 * 512 * 31 = 333328, >> 14 = 20: 335
    // green = (2 << 14) + 8 * 31 * 511 = 159496, >> 14 = 9: 159
    // blue  = (3 << 14) + 31 * 31 * 511 = 540223, >> 14 = 32, saturated to 31: 511
    Triangle();
    const PortGeList *list = Building();
    CHECK_EQ(PolyVertex(list, 0, 0)->color[0], 335);
    CHECK_EQ(PolyVertex(list, 0, 0)->color[1], 159);
    CHECK_EQ(PolyVertex(list, 0, 0)->color[2], 511);
    // Specular red: + 31 * 508 * 31 = 488188, red = 821516 >> 14 = 50, saturated to 31
    SpeEmi(Rgb(31, 0, 0), Rgb(1, 2, 3), 0);
    Normal(0, 0, 511);
    Triangle();
    CHECK_EQ(PolyVertex(list, 1, 0)->color[0], 511);
    CHECK_EQ(PolyVertex(list, 1, 0)->color[1], 159);
    // The shininess table: level 508 reads entry 508 / 4 = 127 (here 64), times 2: 128; red = 333328 + 31 * 128 * 31 =
    // 456336, >> 14 = 27: 447
    uint32_t table[32];
    memset(table, 0, sizeof(table));
    table[127 / 4] = 64u << (8 * (127 % 4));
    CmdN(0x34, table, 32);
    SpeEmi(Rgb(31, 0, 0), Rgb(1, 2, 3), 1);
    Normal(0, 0, 511);
    Triangle();
    CHECK_EQ(PolyVertex(list, 2, 0)->color[0], 447);
    // A light the polygon doesn't enable adds nothing: the emission alone, 1, 2, 3: 31, 47, 63
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Normal(0, 0, 511);
    Triangle();
    CHECK_EQ(PolyVertex(list, 3, 0)->color[0], 31);
    CHECK_EQ(PolyVertex(list, 3, 0)->color[1], 47);
    CHECK_EQ(PolyVertex(list, 3, 0)->color[2], 63);
    // DIF_AMB bit 15 sets the vertex colour to the diffuse colour: 16, 8, 31: 271, 143, 511
    DifAmb(Rgb(16, 8, 31), 0, 1);
    Triangle();
    CHECK_EQ(PolyVertex(list, 4, 0)->color[0], 271);
    CHECK_EQ(PolyVertex(list, 4, 0)->color[1], 143);
    CHECK_EQ(PolyVertex(list, 4, 0)->color[2], 511);
    // The light vector goes through the vector matrix when LIGHT_VECTOR comes: with a 180-degree turn about y
    // (x and z negated), (0, 0, 511) becomes (0, 0, -511), so -l = (0, 0, 511); dot = 511 * 511 >> 9 = 510, red =
    // (1 << 14) + 16 * 31 * 510 + 4 * 512 * 31 = 332832, >> 14 = 20
    PolygonAttr(ATTR(31, 0) | 1);
    Begin(0);
    MtxMode(2);
    int32_t turn[9] = { -ONE, 0, 0, 0, ONE, 0, 0, 0, -ONE };
    MtxMult33(turn);
    LightVector(0, 0, 0, 511);
    MtxIdentity();
    DifAmb(Rgb(16, 8, 31), Rgb(4, 0, 0), 0);
    SpeEmi(0, Rgb(1, 2, 3), 0);
    Normal(0, 0, 511);
    Triangle();
    CHECK_EQ(PolyVertex(list, 5, 0)->color[0], 335);

    // Oblique lights, specular 31 and white light only. Light (0, -362, -362), normal (0, 0, 511): -l = (0, 362, 362),
    // divisor 2^18 / (512 + 362) = 299; dot = 362 * 511 >> 9 = 361; s = 361 + 511 = 872, 872 * 872 >> 10 = 742, level
    // (742 * 299 >> 8) - 512 = 354; red = 31 * 354 * 31 >> 14 = 20
    Start("lighting (oblique)");
    Basic();
    CHECK_EQ(LitRed(0, -362, -362, 0, 0, 511, 0, 31), 20 * 16 + 15);
    // Light (-296, -296, -296), normal (0, 0, 511): divisor 2^18 / 808 = 324; dot = 296 * 511 >> 9 = 295; s = 806,
    // 806 * 806 >> 10 = 634, level (634 * 324 >> 8) - 512 = 290; red = 31 * 290 * 31 >> 14 = 17
    CHECK_EQ(LitRed(-296, -296, -296, 0, 0, 511, 0, 31), 17 * 16 + 15);
    // Light (0, -362, -362), normal (0, 181, 474): dot = (362 * 181 >> 9) + (362 * 474 >> 9) = 127 + 335 = 462;
    // s = 462 + 474 = 936, 936 * 936 >> 10 = 855, level (855 * 299 >> 8) - 512 = 486; red = 31 * 486 * 31 >> 14 = 28.
    // Diffuse 31 alone instead: 31 * 31 * 462 >> 14 = 27
    CHECK_EQ(LitRed(0, -362, -362, 0, 181, 474, 0, 31), 28 * 16 + 15);
    CHECK_EQ(LitRed(0, -362, -362, 0, 181, 474, 31, 0), 27 * 16 + 15);
    // A light behind the surface (dot <= 0) gives nothing
    CHECK_EQ(LitRed(0, 0, 511, 0, 0, 511, 31, 31), 0);
    // A dot past 1023 wraps in 11 bits and the diffuse product is cut to 20 bits: light (-512, -512, -512) and normal
    // (511, 511, 2) give dot = 511 + 511 + 2 = 1024, which is -1024 in 11 bits; 31 * 31 * -1024 cut to 20 bits is
    // 0x100000 - 984064 = 64512, >> 14 = 3
    CHECK_EQ(LitRed(-512, -512, -512, 511, 511, 2, 31, 0), 3 * 16 + 15);
}

// Texture coordinates (GBATEK "Texture Coordinates"): mode 1 transforms (S, T, 1/16, 1/16) by the texture matrix;
// mode 3 adds the vertex times the matrix >> 24, mode 2 the normal times the matrix >> 21
static void TestTexCoords(void) {
    Start("texcoords");
    Basic();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    MtxMode(3);
    int32_t t1[16] = { 0x2000, 0, 0, 0, 0, 0x3000, 0, 0, ONE, 0, 0, 0, 0x8000, -ONE, 0, 0 };
    MtxLoad44(t1);
    TexImageParam(1u << 30);
    // s = (0x10 * 0x2000 + 0x20 * 0 + 0x1000 + 0x8000) >> 12 = 0x29, t = (0x20 * 0x3000 - 0x1000) >> 12 = 0x5f
    TexCoord(0x10, 0x20);
    Triangle();
    const PortGeList *list = Building();
    CHECK_EQ(PolyVertex(list, 0, 0)->s, 0x29);
    CHECK_EQ(PolyVertex(list, 0, 0)->t, 0x5f);
    // Mode 3: (5, 7) plus (x * 16.0) >> 24 for s: x = 0x1000 gives 16, x = 0x800 gives 8
    int32_t t3[16] = { 0x10000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    MtxLoad44(t3);
    TexImageParam(3u << 30);
    TexCoord(5, 7);
    Vtx16(ONE, 0, 0);
    Vtx16(0x800, 0, 0);
    Vtx16(0, 0x800, 0);
    CHECK_EQ(PolyVertex(list, 1, 0)->s, 21);
    CHECK_EQ(PolyVertex(list, 1, 1)->s, 13);
    CHECK_EQ(PolyVertex(list, 1, 2)->s, 5);
    CHECK_EQ(PolyVertex(list, 1, 0)->t, 7);
    // Mode 2: (5, 7) plus (nx * 512.0) >> 21 for s: nx = 0x100 gives 256
    int32_t t2[16] = { 0x200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    MtxLoad44(t2);
    TexImageParam(2u << 30);
    TexCoord(5, 7);
    Normal(0x100, 0, 0);
    Triangle();
    CHECK_EQ(PolyVertex(list, 2, 0)->s, 261);
    CHECK_EQ(PolyVertex(list, 2, 0)->t, 7);
}

// POS_TEST, VEC_TEST and BOX_TEST (GBATEK "Test Functions")
static void TestTests(void) {
    Start("tests");
    Basic();
    MtxMode(1);
    MtxTrans(ONE, 2 * ONE, 3 * ONE);
    // (0x800, 0, 0, 1) * clip: (0x800 + 0x1000, 0x2000, 0x3000, 0x1000)
    PosTest(0x800, 0, 0);
    uint32_t pos[4];
    port_ge_position_result(sGe, pos);
    CHECK_HEX(pos[0], 0x1800);
    CHECK_HEX(pos[1], 0x2000);
    CHECK_HEX(pos[2], 0x3000);
    CHECK_HEX(pos[3], ONE);
    // Rounding down: x scaled by 0.5, (1 * 0x800) >> 12 = 0 and (-1 * 0x800) >> 12 = -1
    MtxIdentity();
    MtxScale(0x800, ONE, ONE);
    PosTest(1, 0, 0);
    port_ge_position_result(sGe, pos);
    CHECK_HEX(pos[0], 0);
    PosTest(-1, 0, 0);
    port_ge_position_result(sGe, pos);
    CHECK_HEX(pos[0], 0xffffffff);
    // VEC_TEST: (n * 0x1000) >> 9 = n << 3, kept in 13 bits and sign-extended from bit 12
    MtxMode(2);
    MtxIdentity();
    uint16_t vec[3];
    VecTest(0x100, -0x80, 0x1ff);
    port_ge_vector_result(sGe, vec);
    CHECK_HEX(vec[0], 0x0800);
    CHECK_HEX(vec[1], 0xfc00);
    CHECK_HEX(vec[2], 0x0ff8);
    // With the vector matrix doubled, 0x100 gives 0x1000 and 0x1ff gives 0x1ff0: bit 12 set, so both read negative
    int32_t twice[9] = { 0x2000, 0, 0, 0, 0x2000, 0, 0, 0, 0x2000 };
    MtxMult33(twice);
    VecTest(0x100, 0, 0x1ff);
    port_ge_vector_result(sGe, vec);
    CHECK_HEX(vec[0], 0xf000);
    CHECK_HEX(vec[1], 0);
    CHECK_HEX(vec[2], 0xfff0);
    // BOX_TEST, after POLYGON_ATTR with far-plane clipping and 1-dot display and BEGIN_VTXS, as the game sends it
    MtxIdentity();
    PolygonAttr(0xc0 | ATTR_FAR_CLIP | ATTR_1DOT);
    Begin(0);
    End();
    BoxTest(-0x400, -0x400, -0x400, 0x800, 0x800, 0x800);
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 1);
    BoxTest(0x2000, 0, 0, 0x800, 0x800, 0x800); // beyond x = w
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 0);
    BoxTest(-0x400, -0x400, 0x1800, 0x800, 0x800, 0x800); // beyond the far plane
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 0);
    BoxTest(-0x400, -0x400, 0x800, 0x800, 0x800, 0x1000); // crosses the far plane, its near face inside
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 1);
    // Without far-plane clipping the faces that cross it don't count, but the near face still does
    PolygonAttr(0xc0 | ATTR_1DOT);
    Begin(0);
    End();
    BoxTest(-0x400, -0x400, 0x800, 0x800, 0x800, 0x1000);
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 1);
    // A box from behind the near plane to beyond the far one: its near and far faces are outside and its four sides
    // cross the far plane, so it counts only with far-plane clipping
    BoxTest(-0x400, -0x400, -0x2000, 0x800, 0x800, 0x4000);
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 0);
    PolygonAttr(0xc0 | ATTR_1DOT | ATTR_FAR_CLIP);
    Begin(0);
    End();
    BoxTest(-0x400, -0x400, -0x2000, 0x800, 0x800, 0x4000);
    CHECK_EQ((port_ge_gxstat(sGe) >> 1) & 1, 1);
}

// VIEWPORT (64, 32, 191, 159): width and height 128; screen X = (x + w) * 128 / 2w + 64, Y = (w - y) * 128 / 2w +
// (191 - 159)
static void TestViewport(void) {
    Start("viewport");
    Basic();
    Viewport(64, 32, 191, 159);
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Vtx16(-ONE, -ONE, 0x800); // (64, 160); depth (0x2000 + 0x3fff) * 0x200 = 0xbffe00
    Vtx16(ONE, -ONE, -ONE);   // (192, 160); depth (-0x4000 + 0x3fff) * 0x200 < 0: 0
    Vtx16(ONE, ONE, ONE);     // (192, 32); depth (0x4000 + 0x3fff) * 0x200 = 0xfffe00
    const PortGeList *list = Building();
    CHECK_EQ(list->polygon_count, 1);
    CheckVertex(__LINE__, PolyVertex(list, 0, 0), 64, 160, 0xbffe00, 0, 0, 0);
    CheckVertex(__LINE__, PolyVertex(list, 0, 1), 192, 160, 0, 0, 0, 0);
    CheckVertex(__LINE__, PolyVertex(list, 0, 2), 192, 32, 0xfffe00, 0, 0, 0);
    Vtx16(0, 0, 0); // (128, 96)
    Vtx16(0x400, 0, 0);
    Vtx16(0, 0x400, 0);
    CheckVertex(__LINE__, PolyVertex(list, 1, 0), 128, 96, 0x7ffe00, 0, 0, 0);
}

// The dot of TestOneDot at W w: the projection diag(w, w, w, w) scales every coordinate alike, so the vertices land
// on (128, 96) still. Returns whether polygon RAM took it
static int DotAtW(int32_t w) {
    int32_t m[16] = { w, 0, 0, 0, 0, w, 0, 0, 0, 0, w, 0, 0, 0, 0, w };
    uint32_t before = Building()->polygon_count;
    MtxMode(0);
    MtxLoad44(m);
    MtxMode(2);
    MtxIdentity();
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(2, -1, 0);
    Vtx16(1, -2, 0);
    return Building()->polygon_count > before;
}

// A polygon whose vertices all land on one screen position is a dot; where all its vertices have W >> 9 above
// DISP_1DOT_DEPTH it is hidden unless POLYGON_ATTR bit 13 is set. Vertices (0, 0), (2, -1), (1, -2) in 1/4096 all land
// on (128, 96)
static void TestOneDot(void) {
    // The register starts at 0x7fff, which hides nothing
    Start("1-dot (power-on)");
    Basic();
    CHECK_EQ(DotAtW(ONE), 1);
    CHECK_EQ(DotAtW(0x7fffff), 1);
    // Depth 8: W 0x1000 to 0x11ff (W >> 9 == 8) are drawn, 0x1200 is beyond
    Start("1-dot (boundary)");
    Basic();
    port_ge_set_1dot_depth(sGe, 8);
    CHECK_EQ(DotAtW(0x1100), 1);
    CHECK_EQ(DotAtW(0x11ff), 1);
    CHECK_EQ(DotAtW(0x1200), 0);
    Start("1-dot");
    Basic();
    port_ge_set_1dot_depth(sGe, 4); // up to W 0x9ff: the dot's W, 1.0, is beyond
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(2, -1, 0);
    Vtx16(1, -2, 0);
    CHECK_EQ(Building()->polygon_count, 0);
    PolygonAttr(ATTR(31, 0) | ATTR_1DOT);
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(2, -1, 0);
    Vtx16(1, -2, 0);
    CHECK_EQ(Building()->polygon_count, 1);
    port_ge_set_1dot_depth(sGe, 0x7fff);
    PolygonAttr(ATTR(31, 0));
    Begin(0);
    Vtx16(0, 0, 0);
    Vtx16(2, -1, 0);
    Vtx16(1, -2, 0);
    CHECK_EQ(Building()->polygon_count, 2);
}

// GXSTAT's fixed bits and the FIFO interrupt condition
static void TestStatus(void) {
    Start("status");
    // Under half full and empty, nothing busy
    CHECK_HEX(port_ge_gxstat(sGe), 0x06000000);
    CHECK_EQ(port_ge_fifo_irq(sGe), 0);
    port_ge_write_gxstat(sGe, 2u << 30); // when empty
    CHECK_HEX(port_ge_gxstat(sGe), 0x86000000);
    CHECK_EQ(port_ge_fifo_irq(sGe), 1);
    port_ge_write_gxstat(sGe, 1u << 30); // when under half full
    CHECK_EQ(port_ge_fifo_irq(sGe), 1);
    port_ge_write_gxstat(sGe, 0);
    CHECK_EQ(port_ge_fifo_irq(sGe), 0);
}

int main(void) {
    sGe = port_ge_create();
    if (sGe == NULL) {
        printf("out of memory\n");
        return 1;
    }
    TestFifo();
    TestMultiply();
    TestMatrixForms();
    TestStacks();
    TestVertices();
    TestCulling();
    TestClipping();
    TestStrips();
    TestRamLimits();
    TestSwap();
    TestLighting();
    TestTexCoords();
    TestTests();
    TestViewport();
    TestOneDot();
    TestStatus();
    port_ge_destroy(sGe);
    printf("%d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
