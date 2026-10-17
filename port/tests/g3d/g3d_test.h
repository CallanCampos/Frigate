// Shared helpers of the geometry engine and 3D renderer tests (port/tests/g3d): exact checks that print each failure,
// and one function per geometry command that writes it to the engine as the game does, one parameter per write to the
// command's port. Values are written in hex or as sums of 20.12 constants, so each expected value can be worked out
// by hand; none comes from running the code under test.
#ifndef PORT_TESTS_G3D_G3D_TEST_H
#define PORT_TESTS_G3D_G3D_TEST_H

#include "gfx/ge/ge.h"
#include <stdint.h>
#include <stdio.h>

static int sChecks;
static int sFailures;
static const char *sTest = "";

#define CHECK(cond) Check(__LINE__, #cond, (cond) != 0)
#define CHECK_EQ(got, want) CheckEq(__LINE__, #got, (int64_t)(got), (int64_t)(want))
#define CHECK_HEX(got, want) CheckHex(__LINE__, #got, (uint32_t)(got), (uint32_t)(want))

static inline void Check(int line, const char *what, int ok) {
    sChecks++;
    if (!ok) {
        sFailures++;
        printf("%s (line %d): %s is false\n", sTest, line, what);
    }
}

static inline void CheckEq(int line, const char *what, int64_t got, int64_t want) {
    sChecks++;
    if (got != want) {
        sFailures++;
        printf("%s (line %d): %s is %lld, want %lld\n", sTest, line, what, (long long)got, (long long)want);
    }
}

static inline void CheckHex(int line, const char *what, uint32_t got, uint32_t want) {
    sChecks++;
    if (got != want) {
        sFailures++;
        printf("%s (line %d): %s is 0x%08x, want 0x%08x\n", sTest, line, what, got, want);
    }
}

// 1.0 in 20.12 and 4.12
#define ONE 0x1000

// The command port of a geometry command (GBATEK: 0x04000400 + 4 * command), and GXFIFO
#define PORT(cmd) (0x04000400u + 4u * (cmd))
#define FIFO 0x04000400u

static PortGe *sGe;

static inline void Cmd(uint32_t cmd, uint32_t param) {
    port_ge_write(sGe, PORT(cmd), param);
}

static inline void CmdN(uint32_t cmd, const uint32_t *params, int n) {
    for (int i = 0; i < n; i++) {
        Cmd(cmd, params[i]);
    }
}

static inline uint32_t Pack10(int32_t x, int32_t y, int32_t z) {
    return ((uint32_t)x & 0x3ff) | ((uint32_t)y & 0x3ff) << 10 | ((uint32_t)z & 0x3ff) << 20;
}

static inline uint32_t Pack16(int32_t lo, int32_t hi) {
    return ((uint32_t)lo & 0xffff) | (uint32_t)hi << 16;
}

static inline uint16_t Rgb(unsigned r, unsigned g, unsigned b) {
    return (uint16_t)(r | g << 5 | b << 10);
}

static inline void MtxMode(uint32_t mode) {
    Cmd(0x10, mode);
}
static inline void MtxPush(void) {
    Cmd(0x11, 0);
}
static inline void MtxPop(int32_t n) {
    Cmd(0x12, (uint32_t)n & 63);
}
static inline void MtxStore(uint32_t i) {
    Cmd(0x13, i);
}
static inline void MtxRestore(uint32_t i) {
    Cmd(0x14, i);
}
static inline void MtxIdentity(void) {
    Cmd(0x15, 0);
}
static inline void MtxLoad44(const int32_t m[16]) {
    CmdN(0x16, (const uint32_t *)m, 16);
}
static inline void MtxLoad43(const int32_t m[12]) {
    CmdN(0x17, (const uint32_t *)m, 12);
}
static inline void MtxMult44(const int32_t m[16]) {
    CmdN(0x18, (const uint32_t *)m, 16);
}
static inline void MtxMult33(const int32_t m[9]) {
    CmdN(0x1a, (const uint32_t *)m, 9);
}
static inline void MtxScale(int32_t x, int32_t y, int32_t z) {
    Cmd(0x1b, (uint32_t)x);
    Cmd(0x1b, (uint32_t)y);
    Cmd(0x1b, (uint32_t)z);
}
static inline void MtxTrans(int32_t x, int32_t y, int32_t z) {
    Cmd(0x1c, (uint32_t)x);
    Cmd(0x1c, (uint32_t)y);
    Cmd(0x1c, (uint32_t)z);
}
static inline void Color(uint16_t rgb) {
    Cmd(0x20, rgb);
}
static inline void Normal(int32_t x, int32_t y, int32_t z) {
    Cmd(0x21, Pack10(x, y, z));
}
static inline void TexCoord(int32_t s, int32_t t) {
    Cmd(0x22, Pack16(s, t));
}
static inline void Vtx16(int32_t x, int32_t y, int32_t z) {
    Cmd(0x23, Pack16(x, y));
    Cmd(0x23, (uint32_t)z & 0xffff);
}
static inline void Vtx10(int32_t x, int32_t y, int32_t z) {
    Cmd(0x24, Pack10(x, y, z));
}
static inline void VtxXY(int32_t x, int32_t y) {
    Cmd(0x25, Pack16(x, y));
}
static inline void VtxXZ(int32_t x, int32_t z) {
    Cmd(0x26, Pack16(x, z));
}
static inline void VtxYZ(int32_t y, int32_t z) {
    Cmd(0x27, Pack16(y, z));
}
static inline void VtxDiff(int32_t x, int32_t y, int32_t z) {
    Cmd(0x28, Pack10(x, y, z));
}
static inline void PolygonAttr(uint32_t attr) {
    Cmd(0x29, attr);
}
static inline void TexImageParam(uint32_t param) {
    Cmd(0x2a, param);
}
static inline void PlttBase(uint32_t base) {
    Cmd(0x2b, base);
}
static inline void DifAmb(uint16_t diffuse, uint16_t ambient, int setColor) {
    Cmd(0x30, diffuse | (setColor ? 0x8000u : 0) | (uint32_t)ambient << 16);
}
static inline void SpeEmi(uint16_t specular, uint16_t emission, int table) {
    Cmd(0x31, specular | (table ? 0x8000u : 0) | (uint32_t)emission << 16);
}
static inline void LightVector(uint32_t light, int32_t x, int32_t y, int32_t z) {
    Cmd(0x32, Pack10(x, y, z) | light << 30);
}
static inline void LightColor(uint32_t light, uint16_t rgb) {
    Cmd(0x33, rgb | light << 30);
}
static inline void Begin(uint32_t primitive) {
    Cmd(0x40, primitive);
}
static inline void End(void) {
    Cmd(0x41, 0);
}
static inline void SwapBuffers(uint32_t param) {
    Cmd(0x50, param);
}
static inline void Viewport(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    Cmd(0x60, x1 | y1 << 8 | x2 << 16 | y2 << 24);
}
static inline void BoxTest(int32_t x, int32_t y, int32_t z, int32_t w, int32_t h, int32_t d) {
    Cmd(0x70, Pack16(x, y));
    Cmd(0x70, Pack16(z, w));
    Cmd(0x70, Pack16(h, d));
}
static inline void PosTest(int32_t x, int32_t y, int32_t z) {
    Cmd(0x71, Pack16(x, y));
    Cmd(0x71, (uint32_t)z & 0xffff);
}
static inline void VecTest(int32_t x, int32_t y, int32_t z) {
    Cmd(0x72, Pack10(x, y, z));
}

// POLYGON_ATTR: alpha, polygon ID, both faces drawn, modulation, no lights
#define ATTR(alpha, id) (0xc0u | (uint32_t)(alpha) << 16 | (uint32_t)(id) << 24)
#define ATTR_FRONT_ONLY 0x80u
#define ATTR_BACK_ONLY 0x40u
#define ATTR_FAR_CLIP (1u << 12)
#define ATTR_1DOT (1u << 13)
#define ATTR_DEPTH_EQUAL (1u << 14)
#define ATTR_FOG (1u << 15)
#define ATTR_MODE(m) ((uint32_t)(m) << 4)

// Identity projection and position/vector matrices and the whole screen as the viewport: a vertex (x, y, z) then has
// w = 1.0, and lands on screen X (x + 1.0) * 256 / 2.0 and Y (1.0 - y) * 192 / 2.0
static inline void Basic(void) {
    MtxMode(0);
    MtxIdentity();
    MtxMode(3);
    MtxIdentity();
    MtxMode(2);
    MtxIdentity();
    Viewport(0, 0, 255, 191);
}

#endif // PORT_TESTS_G3D_G3D_TEST_H
