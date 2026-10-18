// The DS's geometry engine (ge.h). Commands pass the two stages the hardware has: the FIFO decoder turns port writes
// and packed GXFIFO words into entries of one command code and one parameter each, and the executor takes each
// command's parameters from consecutive entries and runs it. Matrices are 4x4, row by row, in 20.12 fixed point, with
// vectors as rows on the left (v' = v * M), as GBATEK writes them.
//
// Facts are GBATEK's unless a comment says they are melonDS's measured behaviour or the port's choice.
#include "gfx/ge/ge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GXFIFO 0x04000400u
#define GXFIFO_END 0x04000440u
#define PORTS_END 0x04000600u

enum {
    CMD_MTX_MODE = 0x10,
    CMD_MTX_PUSH = 0x11,
    CMD_MTX_POP = 0x12,
    CMD_MTX_STORE = 0x13,
    CMD_MTX_RESTORE = 0x14,
    CMD_MTX_IDENTITY = 0x15,
    CMD_MTX_LOAD_4x4 = 0x16,
    CMD_MTX_LOAD_4x3 = 0x17,
    CMD_MTX_MULT_4x4 = 0x18,
    CMD_MTX_MULT_4x3 = 0x19,
    CMD_MTX_MULT_3x3 = 0x1a,
    CMD_MTX_SCALE = 0x1b,
    CMD_MTX_TRANS = 0x1c,
    CMD_COLOR = 0x20,
    CMD_NORMAL = 0x21,
    CMD_TEXCOORD = 0x22,
    CMD_VTX_16 = 0x23,
    CMD_VTX_10 = 0x24,
    CMD_VTX_XY = 0x25,
    CMD_VTX_XZ = 0x26,
    CMD_VTX_YZ = 0x27,
    CMD_VTX_DIFF = 0x28,
    CMD_POLYGON_ATTR = 0x29,
    CMD_TEXIMAGE_PARAM = 0x2a,
    CMD_PLTT_BASE = 0x2b,
    CMD_DIF_AMB = 0x30,
    CMD_SPE_EMI = 0x31,
    CMD_LIGHT_VECTOR = 0x32,
    CMD_LIGHT_COLOR = 0x33,
    CMD_SHININESS = 0x34,
    CMD_BEGIN_VTXS = 0x40,
    CMD_END_VTXS = 0x41,
    CMD_SWAP_BUFFERS = 0x50,
    CMD_VIEWPORT = 0x60,
    CMD_BOX_TEST = 0x70,
    CMD_POS_TEST = 0x71,
    CMD_VEC_TEST = 0x72,
};

// MTX_MODE
enum {
    MODE_PROJECTION,
    MODE_POSITION,
    MODE_POSITION_VECTOR,
    MODE_TEXTURE,
};

// POLYGON_ATTR
#define ATTR_LIGHT(n) (1u << (n))
#define ATTR_BACK (1u << 6)
#define ATTR_FRONT (1u << 7)
#define ATTR_FAR_CLIP (1u << 12) // polygons crossing the far plane are clipped instead of hidden
#define ATTR_1DOT (1u << 13)     // 1-dot polygons beyond DISP_1DOT_DEPTH are drawn

// TEXIMAGE_PARAM bits 30-31: where texture coordinates come from
#define TEXGEN(param) ((param) >> 30)
enum {
    TEXGEN_NONE,
    TEXGEN_TEXCOORD,
    TEXGEN_NORMAL,
    TEXGEN_VERTEX,
};

// GXSTAT
#define GXSTAT_BOX_RESULT (1u << 1)
#define GXSTAT_POS_LEVEL_SHIFT 8
#define GXSTAT_PROJ_LEVEL_SHIFT 13
#define GXSTAT_STACK_ERROR (1u << 15)
#define GXSTAT_FIFO_UNDER_HALF (1u << 25)
#define GXSTAT_FIFO_EMPTY (1u << 26)
#define GXSTAT_FIFO_IRQ_SHIFT 30

// A vertex in clip space with what the rasterizer interpolates: the colour, each 5-bit channel c carried with 12
// fractional bits as (c << 12) + 0xfff through clipping (melonDS's measurement), and the texture coordinates
#define ATTR_COUNT 5
#define COLOR_FRACTION 0xfff
typedef struct ClipVertex {
    int32_t pos[4];           // x, y, z, w
    int32_t attr[ATTR_COUNT]; // red, green, blue, s, t
} ClipVertex;

// One FIFO entry: a command code and one of its parameters (or 0 for a command without any)
typedef struct Entry {
    uint32_t param;
    uint8_t cmd;
} Entry;

struct PortGe {
    // The FIFO decoder: the packed word's remaining command codes (the current one in the low byte), how many are
    // left (0: the next GXFIFO word is a new packed word) and how many parameters the current one still needs
    uint32_t packed;
    int packedLeft;
    int packedParams;
    // The entries that wait to run, [head, tail)
    Entry *queue;
    size_t head;
    size_t tail;
    size_t capacity;

    // Matrices and stacks. The projection and texture stacks have one entry and a 1-bit pointer, the position and
    // vector stacks 31 entries (32 kept, the last one reachable only with an error) and a 6-bit pointer
    int mtxMode;
    int32_t proj[16];
    int32_t pos[16];
    int32_t vec[16];
    int32_t tex[16];
    int32_t clip[16];
    int clipDirty;
    int32_t projStack[16];
    int32_t texStack[16];
    int32_t posStack[32][16];
    int32_t vecStack[32][16];
    int projSp;
    int texSp;
    int posSp;
    int stackError;

    // The current vertex (kept in 16 bits, as VTX_XY and the others reuse it), texture coordinates as TEXCOORD gave
    // them and after the texture matrix, the vertex colour (5 bits each)
    int16_t vertex[3];
    int16_t rawTexCoord[2];
    int16_t texCoord[2];
    uint8_t color[3];
    // Lighting: the directions toward the lights as LIGHT_VECTOR left them (11 bits, 1.0.9 and a sign bit) and each
    // light's specular divisor, the light colours and materials (5 bits)
    int32_t lightDir[4][3];
    int32_t specDivisor[4];
    uint8_t lightColor[4][3];
    uint8_t diffuse[3];
    uint8_t ambient[3];
    uint8_t specular[3];
    uint8_t emission[3];
    int useShininessTable;
    uint8_t shininess[128];

    // Polygons: POLYGON_ATTR as written and as BEGIN_VTXS latched it, the texture parameters, the viewport
    uint32_t attrPending;
    uint32_t attr;
    uint32_t texParam;
    uint32_t plttBase;
    uint32_t viewport;
    uint16_t oneDotDepth;
    // The primitive being assembled: its type, the vertices it holds, how many polygons a strip has made so far, and
    // for each held vertex its index in vertex RAM when the last polygon stored it unclipped (else -1), so the next
    // polygon of a strip shares it
    int primitive;
    ClipVertex prim[4];
    int32_t primRam[4];
    int primCount;
    int stripPolygons;

    // Tests and status
    int boxResult;
    int32_t posResult[4];
    uint16_t vecResult[3];
    uint32_t fifoIrq;
    int ramOverflow;

    // Polygon RAM: the list the rendering engine draws, the one swapped and waiting for the VBlank (or NULL), the one
    // being built, and a spare one (NULL while a swap waits)
    PortGeList *render;
    PortGeList *swapped;
    PortGeList *building;
    PortGeList *spare;
    PortGeList *lists[3];
};

static void OutOfMemory(void) {
    fprintf(stderr, "port: out of memory for the geometry engine\n");
    exit(1);
}

// v's low bits as a signed value
static int32_t SignExtend(uint32_t v, int bits) {
    uint32_t mask = (1u << bits) - 1;
    uint32_t sign = 1u << (bits - 1);
    return (int32_t)((v & mask) ^ sign) - (int32_t)sign;
}

static int16_t Low16(uint32_t v) {
    return (int16_t)SignExtend(v, 16);
}

// A sum of products, kept as the hardware's wide accumulator would and wrapping like it, shifted right (rounding
// down) and cut to 32 bits
static int32_t Narrow(uint64_t sum, int shift) {
    return (int32_t)(uint32_t)(uint64_t)((int64_t)sum >> shift);
}

static uint64_t Product(int32_t a, int32_t b) {
    return (uint64_t)((int64_t)a * b);
}

// a * b / c, rounded toward zero, for clipping; a * b can need 66 bits
static int64_t MulDiv(int64_t a, int64_t b, int64_t c) {
#if defined(__SIZEOF_INT128__)
    __extension__ typedef __int128 Wide;
    return (int64_t)((Wide)a * b / c);
#else
    long double q = (long double)a * (long double)b / (long double)c;
    return (int64_t)q;
#endif
}

// The orientation of three clip-space vertices, from x, y and w: the sign of det(v0, v1, v2), positive when they turn
// counter-clockwise on the screen with Y up
static int Orientation(const ClipVertex *v0, const ClipVertex *v1, const ClipVertex *v2) {
#if defined(__SIZEOF_INT128__)
    __extension__ typedef __int128 Wide;
    Wide a = (Wide)v1->pos[1] * v2->pos[3] - (Wide)v1->pos[3] * v2->pos[1];
    Wide b = (Wide)v1->pos[0] * v2->pos[3] - (Wide)v1->pos[3] * v2->pos[0];
    Wide c = (Wide)v1->pos[0] * v2->pos[1] - (Wide)v1->pos[1] * v2->pos[0];
    Wide det = v0->pos[0] * a - v0->pos[1] * b + v0->pos[3] * c;
#else
    long double a = (long double)v1->pos[1] * v2->pos[3] - (long double)v1->pos[3] * v2->pos[1];
    long double b = (long double)v1->pos[0] * v2->pos[3] - (long double)v1->pos[3] * v2->pos[0];
    long double c = (long double)v1->pos[0] * v2->pos[1] - (long double)v1->pos[1] * v2->pos[0];
    long double det = v0->pos[0] * a - v0->pos[1] * b + v0->pos[3] * c;
#endif
    return det > 0 ? 1 : det < 0 ? -1 : 0;
}

// --- Matrices

static void Identity(int32_t m[16]) {
    memset(m, 0, 16 * sizeof(*m));
    m[0] = m[5] = m[10] = m[15] = 0x1000;
}

// m = a * m, as the MTX_MULT commands do: each element a sum of four 20.12 products, shifted right by 12 (rounding
// down, also for negative sums) and cut to 32 bits
static void Multiply(int32_t m[16], const int32_t a[16]) {
    int32_t r[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            uint64_t sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += Product(a[i * 4 + k], m[k * 4 + j]);
            }
            r[i * 4 + j] = Narrow(sum, 12);
        }
    }
    memcpy(m, r, sizeof(r));
}

// The clip matrix is the position matrix times the projection matrix, made again when either has changed
static void UpdateClip(PortGe *ge) {
    if (ge->clipDirty) {
        memcpy(ge->clip, ge->proj, sizeof(ge->clip));
        Multiply(ge->clip, ge->pos);
        ge->clipDirty = 0;
    }
}

// MTX_LOAD_*: the mode's matrix becomes m; in mode 2 both the position and the vector matrix
static void Load(PortGe *ge, const int32_t m[16]) {
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        memcpy(ge->proj, m, sizeof(ge->proj));
        ge->clipDirty = 1;
        break;
    case MODE_POSITION:
        memcpy(ge->pos, m, sizeof(ge->pos));
        ge->clipDirty = 1;
        break;
    case MODE_POSITION_VECTOR:
        memcpy(ge->pos, m, sizeof(ge->pos));
        memcpy(ge->vec, m, sizeof(ge->vec));
        ge->clipDirty = 1;
        break;
    default:
        memcpy(ge->tex, m, sizeof(ge->tex));
        break;
    }
}

// MTX_MULT_*, MTX_SCALE and MTX_TRANS: the mode's matrix becomes a times it. In mode 2 the vector matrix is
// multiplied too, except by MTX_SCALE (vector = 0), so that scaling leaves the lighting alone
static void Mult(PortGe *ge, const int32_t a[16], int vector) {
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        Multiply(ge->proj, a);
        ge->clipDirty = 1;
        break;
    case MODE_POSITION:
        Multiply(ge->pos, a);
        ge->clipDirty = 1;
        break;
    case MODE_POSITION_VECTOR:
        Multiply(ge->pos, a);
        if (vector) {
            Multiply(ge->vec, a);
        }
        ge->clipDirty = 1;
        break;
    default:
        Multiply(ge->tex, a);
        break;
    }
}

// The 4x3 form of MTX_LOAD_4x3 and MTX_MULT_4x3: four rows of three, the fourth column (0, 0, 0, 1)
static void From4x3(int32_t m[16], const uint32_t *p) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            m[i * 4 + j] = (int32_t)p[i * 3 + j];
        }
        m[i * 4 + 3] = i == 3 ? 0x1000 : 0;
    }
}

// MTX_PUSH. The projection and texture stacks take one matrix: a second push sets the error flag and stores nothing
// (melonDS's measurement). The position and vector stack stores at its pointer and moves it up; pushing at 31 or
// above sets the error flag, and the store goes to entry (pointer & 31) all the same (melonDS)
static void PushMatrix(PortGe *ge) {
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        if (ge->projSp != 0) {
            ge->stackError = 1;
            break;
        }
        memcpy(ge->projStack, ge->proj, sizeof(ge->projStack));
        ge->projSp = 1;
        break;
    case MODE_TEXTURE:
        if (ge->texSp != 0) {
            ge->stackError = 1;
            break;
        }
        memcpy(ge->texStack, ge->tex, sizeof(ge->texStack));
        ge->texSp = 1;
        break;
    default:
        if (ge->posSp > 30) {
            ge->stackError = 1;
        }
        memcpy(ge->posStack[ge->posSp & 31], ge->pos, sizeof(ge->pos));
        memcpy(ge->vecStack[ge->posSp & 31], ge->vec, sizeof(ge->vec));
        ge->posSp = (ge->posSp + 1) & 63;
        break;
    }
}

// MTX_POP: the parameter's bits 0-5 are a signed count (-32..31) for the position and vector stack; the other stacks
// pop their one entry. A pop of an empty projection or texture stack sets the error flag and loads nothing (melonDS).
// The position and vector pointer wraps in 6 bits, and a result above 30 (beyond the top, or below 0) sets the flag;
// the load comes from entry (pointer & 31) all the same (melonDS)
static void PopMatrix(PortGe *ge, uint32_t param) {
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        if (ge->projSp == 0) {
            ge->stackError = 1;
            break;
        }
        ge->projSp = 0;
        memcpy(ge->proj, ge->projStack, sizeof(ge->proj));
        ge->clipDirty = 1;
        break;
    case MODE_TEXTURE:
        if (ge->texSp == 0) {
            ge->stackError = 1;
            break;
        }
        ge->texSp = 0;
        memcpy(ge->tex, ge->texStack, sizeof(ge->tex));
        break;
    default:
        ge->posSp = (ge->posSp - SignExtend(param, 6)) & 63;
        if (ge->posSp > 30) {
            ge->stackError = 1;
        }
        memcpy(ge->pos, ge->posStack[ge->posSp & 31], sizeof(ge->pos));
        memcpy(ge->vec, ge->vecStack[ge->posSp & 31], sizeof(ge->vec));
        ge->clipDirty = 1;
        break;
    }
}

// MTX_STORE and MTX_RESTORE: bits 0-4 give the position and vector stack's entry, where 31 sets the error flag (the
// access still goes to the 32nd entry the port keeps); the projection and texture stacks ignore it
static void StoreMatrix(PortGe *ge, uint32_t param) {
    uint32_t index = param & 31;
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        memcpy(ge->projStack, ge->proj, sizeof(ge->projStack));
        break;
    case MODE_TEXTURE:
        memcpy(ge->texStack, ge->tex, sizeof(ge->texStack));
        break;
    default:
        if (index == 31) {
            ge->stackError = 1;
        }
        memcpy(ge->posStack[index], ge->pos, sizeof(ge->pos));
        memcpy(ge->vecStack[index], ge->vec, sizeof(ge->vec));
        break;
    }
}

// MTX_RESTORE: the current matrix of the mode, or the position and vector pair, set back from the stack's entry (as
// StoreMatrix above says); the clip matrix is worked out again before its next use
static void RestoreMatrix(PortGe *ge, uint32_t param) {
    uint32_t index = param & 31;
    switch (ge->mtxMode) {
    case MODE_PROJECTION:
        memcpy(ge->proj, ge->projStack, sizeof(ge->proj));
        ge->clipDirty = 1;
        break;
    case MODE_TEXTURE:
        memcpy(ge->tex, ge->texStack, sizeof(ge->tex));
        break;
    default:
        if (index == 31) {
            ge->stackError = 1;
        }
        memcpy(ge->pos, ge->posStack[index], sizeof(ge->pos));
        memcpy(ge->vec, ge->vecStack[index], sizeof(ge->vec));
        ge->clipDirty = 1;
        break;
    }
}

// (x, y, z, 1) * the clip matrix, each component a 20.12 sum rounded down
static void Transform(PortGe *ge, int32_t x, int32_t y, int32_t z, int32_t out[4]) {
    UpdateClip(ge);
    for (int j = 0; j < 4; j++) {
        uint64_t sum = Product(x, ge->clip[j]) + Product(y, ge->clip[4 + j]) + Product(z, ge->clip[8 + j]) +
                       Product(0x1000, ge->clip[12 + j]);
        out[j] = Narrow(sum, 12);
    }
}

// A 1.0.9 vector times the 3x3 part of the vector matrix, shifted right by shift
static void VectorTimesMatrix(const PortGe *ge, const int32_t v[3], int shift, int32_t out[3]) {
    for (int j = 0; j < 3; j++) {
        uint64_t sum = Product(v[0], ge->vec[j]) + Product(v[1], ge->vec[4 + j]) + Product(v[2], ge->vec[8 + j]);
        out[j] = Narrow(sum, shift);
    }
}

// --- Vertex state

// TEXCOORD: in mode 1 the coordinates go through the texture matrix as (S, T, 1/16, 1/16), the last two being one
// unit of the 1.11.4 coordinates; in modes 2 and 3 they are added to what the normal or the vertex give later
static void TexCoord(PortGe *ge, uint32_t param) {
    int32_t s = Low16(param);
    int32_t t = Low16(param >> 16);
    ge->rawTexCoord[0] = (int16_t)s;
    ge->rawTexCoord[1] = (int16_t)t;
    if (TEXGEN(ge->texParam) == TEXGEN_TEXCOORD) {
        for (int j = 0; j < 2; j++) {
            uint64_t sum = Product(s, ge->tex[j]) + Product(t, ge->tex[4 + j]) + (uint64_t)(int64_t)ge->tex[8 + j] +
                           (uint64_t)(int64_t)ge->tex[12 + j];
            ge->texCoord[j] = (int16_t)Narrow(sum, 12);
        }
    } else {
        ge->texCoord[0] = (int16_t)s;
        ge->texCoord[1] = (int16_t)t;
    }
}

// Texture coordinate mode 2 at NORMAL and mode 3 at each vertex: TEXCOORD's coordinates plus the 1.0.9 normal times
// the texture matrix shifted right by 21, or the 4.12 vertex times it shifted right by 24
static void TexCoordFrom(PortGe *ge, int32_t x, int32_t y, int32_t z, int shift) {
    for (int j = 0; j < 2; j++) {
        uint64_t sum = Product(x, ge->tex[j]) + Product(y, ge->tex[4 + j]) + Product(z, ge->tex[8 + j]);
        ge->texCoord[j] = (int16_t)(ge->rawTexCoord[j] + Narrow(sum, shift));
    }
}

// Column j of the vector matrix's 3x3 part times a 1.0.9 vector: the sum of the three products
static int64_t VectorColumn(const PortGe *ge, const int32_t v[3], int j) {
    return (int64_t)v[0] * ge->vec[j] + (int64_t)v[1] * ge->vec[4 + j] + (int64_t)v[2] * ge->vec[8 + j];
}

// A product sum of VectorColumn as the lighting keeps it: cut to 23 bits (signed) and shifted right by 12, rounding
// down, which leaves 11 bits, 1.0.9 and a sign
static int32_t LightingUnits(int64_t sum) {
    return SignExtend((uint32_t)sum, 23) >> 12;
}

// LIGHT_VECTOR: the direction (1.0.9) through the vector matrix. The engine keeps the direction toward the light, each
// component's sum shifted right by 12 (rounding down), negated and cut to 11 bits (signed), and the light's specular
// divisor: 2^18 / (512 - z), z being the direction's own z as LightingUnits keeps it (0 when that is 512), which is
// 512 / (1 + z') for the z' toward the light and normalises the half vector (melonDS's measurements)
static void LightVector(PortGe *ge, uint32_t param) {
    int32_t v[3] = { SignExtend(param, 10), SignExtend(param >> 10, 10), SignExtend(param >> 20, 10) };
    uint32_t light = param >> 30;
    for (int j = 0; j < 3; j++) {
        int64_t sum = VectorColumn(ge, v, j);
        ge->lightDir[light][j] = SignExtend((uint32_t)-(sum >> 12), 11);
    }
    int32_t den = 512 - LightingUnits(VectorColumn(ge, v, 2));
    ge->specDivisor[light] = den != 0 ? (1 << 18) / den : 0;
}

// NORMAL: the vertex colour from the lights the polygon enables (GBATEK "Polygon Light Parameters" for the terms; the
// arithmetic is melonDS's measurement). The normal goes through the vector matrix as LightingUnits keeps it. For each
// light, the dot product of the direction toward the light and the normal is the sum of the three products each
// shifted right by 9 (rounding down). When it is positive:
//   the diffuse level is the dot cut to 11 bits (signed), and the light adds diffuse * light colour * level, cut to
//   20 bits, to each channel (no saturation: a dot past 1023 wraps);
//   the specular level comes from the half vector: s = the dot plus the normal's z, cut to 11 bits (signed), then
//   (s * s >> 10) cut to 10 bits, times the light's divisor, >> 8, less 512. That is 512 * cos 2a for the angle a
//   between the normal and the half vector, 2 cos^2 a - 1 where GBATEK writes cos^2 a. Below 0 it is 0, and cut to
//   14 bits (signed) it is 0..511.
// Otherwise both levels are 0. With the shininess table on, the specular level becomes the table's entry level / 4,
// times 2. The light then adds (specular * level + ambient * 512) * light colour to each channel. The sums start at
// the emission * 2^14 and wrap in 32 bits; each channel is its sum >> 14, at most 31
static void Normal(PortGe *ge, uint32_t param) {
    int32_t n[3] = { SignExtend(param, 10), SignExtend(param >> 10, 10), SignExtend(param >> 20, 10) };
    int32_t normal[3];
    uint32_t sum[3];

    if (TEXGEN(ge->texParam) == TEXGEN_NORMAL) {
        TexCoordFrom(ge, n[0], n[1], n[2], 21);
    }
    for (int j = 0; j < 3; j++) {
        normal[j] = LightingUnits(VectorColumn(ge, n, j));
    }
    for (int c = 0; c < 3; c++) {
        sum[c] = (uint32_t)ge->emission[c] << 14;
    }
    for (int l = 0; l < 4; l++) {
        if (!(ge->attr & ATTR_LIGHT(l))) {
            continue;
        }
        const int32_t *dir = ge->lightDir[l];
        int32_t dot = 0;
        for (int j = 0; j < 3; j++) {
            dot += (dir[j] * normal[j]) >> 9;
        }
        int32_t level = 0;
        if (dot > 0) {
            int32_t diffuse = SignExtend((uint32_t)dot, 11);
            for (int c = 0; c < 3; c++) {
                sum[c] += (uint32_t)(ge->diffuse[c] * ge->lightColor[l][c] * diffuse) & 0xfffff;
            }
            int32_t s = SignExtend((uint32_t)(dot + normal[2]), 11);
            int32_t square = ((s * s) >> 10) & 0x3ff;
            level = ((square * ge->specDivisor[l]) >> 8) - 512;
            level = level < 0 ? 0 : SignExtend((uint32_t)level, 14);
            level = level < 0 ? 0 : level > 511 ? 511 : level;
        }
        if (ge->useShininessTable) {
            level = ge->shininess[level >> 2] << 1;
        }
        for (int c = 0; c < 3; c++) {
            sum[c] += (uint32_t)((ge->specular[c] * level + (ge->ambient[c] << 9)) * ge->lightColor[l][c]);
        }
    }
    for (int c = 0; c < 3; c++) {
        uint32_t value = sum[c] >> 14;
        ge->color[c] = (uint8_t)(value > 31 ? 31 : value);
    }
}

static void SetColor(uint8_t color[3], uint32_t rgb) {
    color[0] = rgb & 31;
    color[1] = (rgb >> 5) & 31;
    color[2] = (rgb >> 10) & 31;
}

// --- Polygons

// Whether a vertex is inside the plane coordinate = side * w (axis 0-2 for x, y, z; side 1 or -1)
static int Inside(const ClipVertex *v, int axis, int side) {
    return (int64_t)side * v->pos[axis] <= (int64_t)v->pos[3];
}

// The point where the segment from an outside vertex to an inside one meets the plane. Every component and attribute
// moves from the outside vertex by (w_out - side * c_out) / ((w_out - side * c_out) - (w_in - side * c_in)) of the
// way, rounded toward zero, and the clipped coordinate is set on the plane exactly. GBATEK doesn't give the
// arithmetic; this is melonDS's
static ClipVertex Intersect(const ClipVertex *out, const ClipVertex *in, int axis, int side) {
    ClipVertex r;
    int64_t num = (int64_t)out->pos[3] - (int64_t)side * out->pos[axis];
    int64_t den = num - ((int64_t)in->pos[3] - (int64_t)side * in->pos[axis]);
    for (int i = 0; i < 4; i++) {
        r.pos[i] = (int32_t)(out->pos[i] + MulDiv((int64_t)in->pos[i] - out->pos[i], num, den));
    }
    r.pos[axis] = side * r.pos[3];
    for (int i = 0; i < ATTR_COUNT; i++) {
        r.attr[i] = (int32_t)(out->attr[i] + MulDiv((int64_t)in->attr[i] - out->attr[i], num, den));
    }
    return r;
}

// Clips a polygon against one plane: each vertex inside stays, and each one outside is replaced by the points where
// its edges to inside neighbours meet the plane, the previous neighbour's first (the order melonDS's measurements
// keep). Then each vertex's colour is cut back to its 5 bits, (c << 12) + 0xfff (melonDS), which changes only the new
// ones. A convex polygon gains at most one vertex a plane, so a quad ends with at most 10; a quad the game sends
// twisted (a bow tie) can cross a plane four times, and what would pass 10 vertices is dropped, as polygon RAM has no
// room for more
static int ClipPlane(const ClipVertex *in, int n, ClipVertex out[PORT_GE_POLYGON_MAX_VERTICES], int axis, int side) {
    int m = 0;
    for (int i = 0; i < n; i++) {
        const ClipVertex *v = &in[i];
        if (Inside(v, axis, side)) {
            if (m < PORT_GE_POLYGON_MAX_VERTICES) {
                out[m++] = *v;
            }
            continue;
        }
        const ClipVertex *prev = &in[(i + n - 1) % n];
        const ClipVertex *next = &in[(i + 1) % n];
        if (Inside(prev, axis, side) && m < PORT_GE_POLYGON_MAX_VERTICES) {
            out[m++] = Intersect(v, prev, axis, side);
        }
        if (Inside(next, axis, side) && m < PORT_GE_POLYGON_MAX_VERTICES) {
            out[m++] = Intersect(v, next, axis, side);
        }
    }
    for (int i = 0; i < m; i++) {
        for (int c = 0; c < 3; c++) {
            out[i].attr[c] = (out[i].attr[c] & ~COLOR_FRACTION) | COLOR_FRACTION;
        }
    }
    return m;
}

// Clips a polygon to the view volume, -w <= x, y, z <= w: far, near, top, bottom, right, left, Y before X as melonDS
// measured (GBATEK gives no order). Returns the vertex count (0 when nothing is left) and whether any vertex was
// outside
static int Clip(const ClipVertex *in, int n, ClipVertex out[PORT_GE_POLYGON_MAX_VERTICES], int *clipped) {
    static const int kPlanes[6][2] = { { 2, 1 }, { 2, -1 }, { 1, 1 }, { 1, -1 }, { 0, 1 }, { 0, -1 } };
    ClipVertex buffers[2][PORT_GE_POLYGON_MAX_VERTICES];
    int inside = 1;
    for (int i = 0; i < n && inside; i++) {
        for (int p = 0; p < 6; p++) {
            if (!Inside(&in[i], kPlanes[p][0], kPlanes[p][1])) {
                inside = 0;
                break;
            }
        }
    }
    *clipped = !inside;
    if (inside) {
        memcpy(out, in, n * sizeof(*in));
        return n;
    }
    const ClipVertex *src = in;
    int count = n;
    for (int p = 0; p < 6 && count > 0; p++) {
        ClipVertex *dst = p == 5 ? out : buffers[p & 1];
        count = ClipPlane(src, count, dst, kPlanes[p][0], kPlanes[p][1]);
        src = dst;
    }
    return count;
}

// The viewport transform (GBATEK "VIEWPORT"): X1, Y1, X2, Y2 in the parameter's bytes, Y1 and Y2 counted from the
// bottom of the screen. Screen X is (x + w) * (X2 - X1 + 1) / 2w + X1 and screen Y, from the top,
// (w - y) * (Y2 - Y1 + 1) / 2w + 191 - Y2; X keeps 9 bits and Y 8, as the hardware's registers do. The Z-buffer depth
// is ((z * 0x4000 / w) + 0x3fff) * 0x200, clamped to 24 bits; divisions round toward zero
static void ToScreen(const PortGe *ge, const ClipVertex *v, PortGeVertex *o) {
    int64_t x = v->pos[0];
    int64_t y = v->pos[1];
    int64_t z = v->pos[2];
    int64_t w = v->pos[3];
    int32_t x1 = ge->viewport & 0xff;
    int32_t y1 = (ge->viewport >> 8) & 0xff;
    int32_t x2 = (ge->viewport >> 16) & 0xff;
    int32_t y2 = ge->viewport >> 24;
    int64_t sx = x1;
    int64_t sy = 191 - y2;
    int64_t depth = 0;
    if (w != 0) {
        sx += (x + w) * (x2 - x1 + 1) / (2 * w);
        sy += (w - y) * (y2 - y1 + 1) / (2 * w);
        depth = (z * 0x4000 / w + 0x3fff) * 0x200;
    }
    o->x = (uint16_t)(sx & 0x1ff);
    o->y = (uint16_t)(sy & 0xff);
    o->z = (int32_t)(depth < 0 ? 0 : depth > 0xffffff ? 0xffffff : depth);
    o->w = v->pos[3];
    // The colour's 5 bits widened for the rasterizer: c * 16 + 15, 0 staying 0 (melonDS)
    for (int c = 0; c < 3; c++) {
        uint32_t c5 = (uint32_t)v->attr[c] >> 12;
        o->color[c] = (uint16_t)(c5 != 0 ? c5 * 16 + 15 : 0);
    }
    o->s = (int16_t)v->attr[3];
    o->t = (int16_t)v->attr[4];
}

// A polygon all of whose vertices land on one screen position is drawn as one dot; it is hidden when even its nearest
// vertex is beyond DISP_1DOT_DEPTH, unless POLYGON_ATTR bit 13 is set. The register holds W >> 9, so a W is beyond it
// when W >> 9 is greater, past (depth << 9) | 0x1ff (melonDS's limit). GBATEK doesn't define "1 dot"; the port takes it
// as this degenerate case
static int HiddenOneDot(const PortGe *ge, const PortGeVertex *v, int n) {
    if (ge->attr & ATTR_1DOT) {
        return 0;
    }
    int64_t nearest = v[0].w;
    for (int i = 1; i < n; i++) {
        if (v[i].x != v[0].x || v[i].y != v[0].y) {
            return 0;
        }
        if (v[i].w < nearest) {
            nearest = v[i].w;
        }
    }
    return nearest > (((int64_t)ge->oneDotDepth << 9) | 0x1ff);
}

// The next polygon of a strip can't share vertices with the last one
static void Unshare(PortGe *ge) {
    for (int i = 0; i < 4; i++) {
        ge->primRam[i] = -1;
    }
}

// A polygon from held vertices, in order: culling by its facing, the far plane, clipping, the viewport, the 1-dot
// rule and polygon RAM's limits, then stored. An unclipped polygon of a strip reuses the vertices it shares with the
// strip's last polygon when that one was stored unclipped too, as polygon RAM does; any other polygon stores all its
// vertices
static void Submit(PortGe *ge, const int *order, int n) {
    ClipVertex in[4];
    ClipVertex out[PORT_GE_POLYGON_MAX_VERTICES];
    PortGeVertex screen[PORT_GE_POLYGON_MAX_VERTICES];
    PortGeList *list = ge->building;
    int clipped;

    for (int i = 0; i < n; i++) {
        in[i] = ge->prim[order[i]];
    }
    // The facing comes from the first three vertices in clip space: front when they turn counter-clockwise as seen
    // (with Y up), back when clockwise. Vertices in line (a line or dot made of a polygon) are never culled, and count
    // as front for the rendering engine's depth test. GBATEK doesn't give the test; this is melonDS's, without its
    // scaling of large values (the port computes the sign exactly)
    int turn = Orientation(&in[0], &in[1], &in[2]);
    if ((turn > 0 && !(ge->attr & ATTR_FRONT)) || (turn < 0 && !(ge->attr & ATTR_BACK))) {
        Unshare(ge);
        return;
    }
    int front = turn >= 0;
    if (!(ge->attr & ATTR_FAR_CLIP)) {
        for (int i = 0; i < n; i++) {
            if (in[i].pos[2] > in[i].pos[3]) {
                Unshare(ge);
                return;
            }
        }
    }
    int m = Clip(in, n, out, &clipped);
    if (m == 0) {
        Unshare(ge);
        return;
    }
    for (int i = 0; i < m; i++) {
        ToScreen(ge, &out[i], &screen[i]);
    }
    if (HiddenOneDot(ge, screen, m)) {
        Unshare(ge);
        return;
    }
    uint32_t fresh = 0;
    for (int i = 0; i < m; i++) {
        if (clipped || ge->primRam[order[i]] < 0) {
            fresh++;
        }
    }
    if (list->polygon_count >= PORT_GE_MAX_POLYGONS || list->vertex_count + fresh > PORT_GE_MAX_VERTICES) {
        ge->ramOverflow = 1;
        Unshare(ge);
        return;
    }
    PortGePolygon *p = &list->polygons[list->polygon_count++];
    p->attr = ge->attr;
    p->tex_param = ge->texParam;
    p->pltt_base = ge->plttBase;
    p->vertex_count = (uint8_t)m;
    p->flags = front ? PORT_GE_POLYGON_FRONT : 0;
    for (int i = 0; i < m; i++) {
        int32_t index;
        if (!clipped && ge->primRam[order[i]] >= 0) {
            index = ge->primRam[order[i]];
        } else {
            index = (int32_t)list->vertex_count++;
            list->vertices[index] = screen[i];
        }
        p->vertices[i] = (uint16_t)index;
        if (!clipped) {
            ge->primRam[order[i]] = index;
        }
    }
    if (clipped) {
        Unshare(ge);
    }
}

// The primitives of BEGIN_VTXS (GBATEK): separate triangles and quads, triangle strips (each new vertex makes a
// triangle with the two before it, every second one in the order 1, 0, 2 so all turn the same way) and quad strips
// (each two new vertices make a quad with the two before them, in the order 0, 1, 3, 2)
static void AddVertex(PortGe *ge, const ClipVertex *v) {
    static const int kTriangle[3] = { 0, 1, 2 };
    static const int kTriangleOdd[3] = { 1, 0, 2 };
    static const int kQuad[4] = { 0, 1, 2, 3 };
    static const int kQuadStrip[4] = { 0, 1, 3, 2 };

    ge->prim[ge->primCount] = *v;
    ge->primRam[ge->primCount] = -1;
    ge->primCount++;
    switch (ge->primitive) {
    case 0:
        if (ge->primCount == 3) {
            Submit(ge, kTriangle, 3);
            ge->primCount = 0;
        }
        break;
    case 1:
        if (ge->primCount == 4) {
            Submit(ge, kQuad, 4);
            ge->primCount = 0;
        }
        break;
    case 2:
        if (ge->primCount == 3) {
            Submit(ge, (ge->stripPolygons & 1) ? kTriangleOdd : kTriangle, 3);
            ge->stripPolygons++;
            ge->prim[0] = ge->prim[1];
            ge->prim[1] = ge->prim[2];
            ge->primRam[0] = ge->primRam[1];
            ge->primRam[1] = ge->primRam[2];
            ge->primCount = 2;
        }
        break;
    default:
        if (ge->primCount == 4) {
            Submit(ge, kQuadStrip, 4);
            ge->prim[0] = ge->prim[2];
            ge->prim[1] = ge->prim[3];
            ge->primRam[0] = ge->primRam[2];
            ge->primRam[1] = ge->primRam[3];
            ge->primCount = 2;
        }
        break;
    }
}

// A vertex command has set the current vertex: texture coordinates from it in mode 3, then into clip space with the
// current colour and texture coordinates
static void Vertex(PortGe *ge) {
    ClipVertex v;
    if (TEXGEN(ge->texParam) == TEXGEN_VERTEX) {
        TexCoordFrom(ge, ge->vertex[0], ge->vertex[1], ge->vertex[2], 24);
    }
    Transform(ge, ge->vertex[0], ge->vertex[1], ge->vertex[2], v.pos);
    for (int c = 0; c < 3; c++) {
        v.attr[c] = (int32_t)((uint32_t)ge->color[c] << 12 | COLOR_FRACTION);
    }
    v.attr[3] = ge->texCoord[0];
    v.attr[4] = ge->texCoord[1];
    AddVertex(ge, &v);
}

// --- Tests

// BOX_TEST (GBATEK): whether any face of the box (x, y, z) to (x + w, y + h, z + d), 1.3.12, would be drawn: each
// face is clipped like a polygon, with the far-plane and 1-dot settings of the latched POLYGON_ATTR (which is why the
// SDK sends POLYGON_ATTR and BEGIN_VTXS first); no face is culled
static void BoxTest(PortGe *ge, const uint32_t *p) {
    static const int kFaces[6][4] = {
        { 0, 2, 6, 4 }, { 1, 3, 7, 5 }, { 0, 1, 5, 4 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 5, 7, 6 },
    };
    int32_t base[3] = { Low16(p[0]), Low16(p[0] >> 16), Low16(p[1]) };
    int32_t size[3] = { Low16(p[1] >> 16), Low16(p[2]), Low16(p[2] >> 16) };
    ClipVertex corner[8];

    memset(corner, 0, sizeof(corner));
    for (int i = 0; i < 8; i++) {
        int32_t c[3];
        for (int a = 0; a < 3; a++) {
            c[a] = base[a] + ((i >> a) & 1 ? size[a] : 0);
        }
        Transform(ge, c[0], c[1], c[2], corner[i].pos);
    }
    ge->boxResult = 0;
    for (int f = 0; f < 6 && !ge->boxResult; f++) {
        ClipVertex in[4];
        ClipVertex out[PORT_GE_POLYGON_MAX_VERTICES];
        PortGeVertex screen[PORT_GE_POLYGON_MAX_VERTICES];
        int clipped;
        int far = 0;
        for (int i = 0; i < 4; i++) {
            in[i] = corner[kFaces[f][i]];
            far |= in[i].pos[2] > in[i].pos[3];
        }
        if (far && !(ge->attr & ATTR_FAR_CLIP)) {
            continue;
        }
        int m = Clip(in, 4, out, &clipped);
        if (m == 0) {
            continue;
        }
        for (int i = 0; i < m; i++) {
            ToScreen(ge, &out[i], &screen[i]);
        }
        ge->boxResult = !HiddenOneDot(ge, screen, m);
    }
}

// VEC_TEST: a 1.0.9 vector times the vector matrix, as 1.3.12 halfwords; the hardware keeps 13 bits and copies bit
// 12 into bits 13-15 (melonDS's measurement; GBATEK: "1bit sign + 3bit integer + 12bit fraction", sign-expanded)
static void VecTest(PortGe *ge, uint32_t param) {
    int32_t v[3] = { SignExtend(param, 10), SignExtend(param >> 10, 10), SignExtend(param >> 20, 10) };
    int32_t r[3];
    VectorTimesMatrix(ge, v, 9, r);
    for (int j = 0; j < 3; j++) {
        uint16_t value = (uint16_t)(r[j] & 0x1fff);
        if (value & 0x1000) {
            value |= 0xe000;
        }
        ge->vecResult[j] = value;
    }
}

// --- The executor

// Parameters per command (GBATEK's table), or -1 for a code that is no command (NOP among them)
static int ParamCount(uint32_t cmd) {
    switch (cmd) {
    case CMD_MTX_PUSH:
    case CMD_MTX_IDENTITY:
    case CMD_END_VTXS:
        return 0;
    case CMD_MTX_MODE:
    case CMD_MTX_POP:
    case CMD_MTX_STORE:
    case CMD_MTX_RESTORE:
    case CMD_COLOR:
    case CMD_NORMAL:
    case CMD_TEXCOORD:
    case CMD_VTX_10:
    case CMD_VTX_XY:
    case CMD_VTX_XZ:
    case CMD_VTX_YZ:
    case CMD_VTX_DIFF:
    case CMD_POLYGON_ATTR:
    case CMD_TEXIMAGE_PARAM:
    case CMD_PLTT_BASE:
    case CMD_DIF_AMB:
    case CMD_SPE_EMI:
    case CMD_LIGHT_VECTOR:
    case CMD_LIGHT_COLOR:
    case CMD_BEGIN_VTXS:
    case CMD_SWAP_BUFFERS:
    case CMD_VIEWPORT:
    case CMD_VEC_TEST:
        return 1;
    case CMD_VTX_16:
    case CMD_POS_TEST:
        return 2;
    case CMD_MTX_SCALE:
    case CMD_MTX_TRANS:
    case CMD_BOX_TEST:
        return 3;
    case CMD_MTX_MULT_3x3:
        return 9;
    case CMD_MTX_LOAD_4x3:
    case CMD_MTX_MULT_4x3:
        return 12;
    case CMD_MTX_LOAD_4x4:
    case CMD_MTX_MULT_4x4:
        return 16;
    case CMD_SHININESS:
        return 32;
    default:
        return -1;
    }
}

static void ClearList(PortGeList *list) {
    list->polygon_count = 0;
    list->vertex_count = 0;
    list->swap_param = 0;
}

// Runs one geometry command with all its parameters (p, as many as ParamCount gives): the matrix commands, the
// vertex, colour, normal, texture and lighting state, polygon attributes, BEGIN_VTXS and END_VTXS, SWAP_BUFFERS, the
// viewport and the box, position and vector tests. Each case follows GBATEK's description of the command, or
// melonDS's where a comment says so; codes the hardware ignores do nothing
static void RunCommand(PortGe *ge, uint32_t cmd, const uint32_t *p) {
    int32_t m[16];

    switch (cmd) {
    case CMD_MTX_MODE:
        ge->mtxMode = p[0] & 3;
        break;
    case CMD_MTX_PUSH:
        PushMatrix(ge);
        break;
    case CMD_MTX_POP:
        PopMatrix(ge, p[0]);
        break;
    case CMD_MTX_STORE:
        StoreMatrix(ge, p[0]);
        break;
    case CMD_MTX_RESTORE:
        RestoreMatrix(ge, p[0]);
        break;
    case CMD_MTX_IDENTITY:
        Identity(m);
        Load(ge, m);
        break;
    case CMD_MTX_LOAD_4x4:
        for (int i = 0; i < 16; i++) {
            m[i] = (int32_t)p[i];
        }
        Load(ge, m);
        break;
    case CMD_MTX_LOAD_4x3:
        From4x3(m, p);
        Load(ge, m);
        break;
    case CMD_MTX_MULT_4x4:
        for (int i = 0; i < 16; i++) {
            m[i] = (int32_t)p[i];
        }
        Mult(ge, m, 1);
        break;
    case CMD_MTX_MULT_4x3:
        From4x3(m, p);
        Mult(ge, m, 1);
        break;
    case CMD_MTX_MULT_3x3:
        Identity(m);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                m[i * 4 + j] = (int32_t)p[i * 3 + j];
            }
        }
        Mult(ge, m, 1);
        break;
    case CMD_MTX_SCALE:
        Identity(m);
        m[0] = (int32_t)p[0];
        m[5] = (int32_t)p[1];
        m[10] = (int32_t)p[2];
        Mult(ge, m, 0);
        break;
    case CMD_MTX_TRANS:
        Identity(m);
        m[12] = (int32_t)p[0];
        m[13] = (int32_t)p[1];
        m[14] = (int32_t)p[2];
        Mult(ge, m, 1);
        break;
    case CMD_COLOR:
        SetColor(ge->color, p[0]);
        break;
    case CMD_NORMAL:
        Normal(ge, p[0]);
        break;
    case CMD_TEXCOORD:
        TexCoord(ge, p[0]);
        break;
    case CMD_VTX_16:
        ge->vertex[0] = Low16(p[0]);
        ge->vertex[1] = Low16(p[0] >> 16);
        ge->vertex[2] = Low16(p[1]);
        Vertex(ge);
        break;
    case CMD_VTX_10:
        // 1.3.6 each, which is 4.12 shifted left by 6
        for (int i = 0; i < 3; i++) {
            ge->vertex[i] = (int16_t)(SignExtend(p[0] >> (10 * i), 10) * 64);
        }
        Vertex(ge);
        break;
    case CMD_VTX_XY:
        ge->vertex[0] = Low16(p[0]);
        ge->vertex[1] = Low16(p[0] >> 16);
        Vertex(ge);
        break;
    case CMD_VTX_XZ:
        ge->vertex[0] = Low16(p[0]);
        ge->vertex[2] = Low16(p[0] >> 16);
        Vertex(ge);
        break;
    case CMD_VTX_YZ:
        ge->vertex[1] = Low16(p[0]);
        ge->vertex[2] = Low16(p[0] >> 16);
        Vertex(ge);
        break;
    case CMD_VTX_DIFF:
        // Three signed 10-bit differences, added to the 4.12 coordinates as they are (their 9 fractional bits are the
        // coordinates' lowest; GBATEK calls them 1.0.9 of 1/8 units)
        for (int i = 0; i < 3; i++) {
            ge->vertex[i] = (int16_t)(ge->vertex[i] + SignExtend(p[0] >> (10 * i), 10));
        }
        Vertex(ge);
        break;
    case CMD_POLYGON_ATTR:
        ge->attrPending = p[0];
        break;
    case CMD_TEXIMAGE_PARAM:
        ge->texParam = p[0];
        break;
    case CMD_PLTT_BASE:
        ge->plttBase = p[0] & 0x1fff;
        break;
    case CMD_DIF_AMB:
        SetColor(ge->diffuse, p[0]);
        SetColor(ge->ambient, p[0] >> 16);
        if (p[0] & 0x8000) {
            SetColor(ge->color, p[0]);
        }
        break;
    case CMD_SPE_EMI:
        SetColor(ge->specular, p[0]);
        SetColor(ge->emission, p[0] >> 16);
        ge->useShininessTable = (p[0] & 0x8000) != 0;
        break;
    case CMD_LIGHT_VECTOR:
        LightVector(ge, p[0]);
        break;
    case CMD_LIGHT_COLOR:
        SetColor(ge->lightColor[p[0] >> 30], p[0]);
        break;
    case CMD_SHININESS:
        for (int i = 0; i < 32; i++) {
            for (int b = 0; b < 4; b++) {
                ge->shininess[i * 4 + b] = (uint8_t)(p[i] >> (8 * b));
            }
        }
        break;
    case CMD_BEGIN_VTXS:
        ge->primitive = p[0] & 3;
        ge->attr = ge->attrPending;
        ge->primCount = 0;
        ge->stripPolygons = 0;
        Unshare(ge);
        break;
    case CMD_END_VTXS:
        // GBATEK: a dummy that does nothing
        break;
    case CMD_SWAP_BUFFERS:
        ge->building->swap_param = p[0] & 3;
        ge->swapped = ge->building;
        ge->building = ge->spare;
        ge->spare = NULL;
        ClearList(ge->building);
        break;
    case CMD_VIEWPORT:
        ge->viewport = p[0];
        break;
    case CMD_BOX_TEST:
        BoxTest(ge, p);
        break;
    case CMD_POS_TEST:
        // The coordinates become the current vertex too, as for VTX_16
        ge->vertex[0] = Low16(p[0]);
        ge->vertex[1] = Low16(p[0] >> 16);
        ge->vertex[2] = Low16(p[1]);
        Transform(ge, ge->vertex[0], ge->vertex[1], ge->vertex[2], ge->posResult);
        break;
    case CMD_VEC_TEST:
        VecTest(ge, p[0]);
        break;
    default:
        break;
    }
}

// Runs every command whose parameters have all come, in order, until one waits: a SWAP_BUFFERS while the last one
// still waits for the VBlank
static void Execute(PortGe *ge) {
    uint32_t params[32];
    while (ge->head < ge->tail) {
        uint32_t cmd = ge->queue[ge->head].cmd;
        int count = ParamCount(cmd);
        size_t need = count > 0 ? (size_t)count : 1;
        if (ge->tail - ge->head < need) {
            break;
        }
        if (cmd == CMD_SWAP_BUFFERS && ge->swapped != NULL) {
            break;
        }
        for (size_t i = 0; i < need; i++) {
            params[i] = ge->queue[ge->head + i].param;
        }
        ge->head += need;
        RunCommand(ge, cmd, params);
    }
    if (ge->head == ge->tail) {
        ge->head = ge->tail = 0;
    }
}

// --- The FIFO decoder

// Adds one entry, a command code with one parameter word (0 for a command that takes none), to the queue Execute
// runs from, growing the queue when it is full: the host has no FIFO limit to stall on
static void Enqueue(PortGe *ge, uint32_t cmd, uint32_t param) {
    if (ge->tail == ge->capacity) {
        if (ge->head > 0) {
            memmove(ge->queue, ge->queue + ge->head, (ge->tail - ge->head) * sizeof(*ge->queue));
            ge->tail -= ge->head;
            ge->head = 0;
        }
        if (ge->tail == ge->capacity) {
            size_t capacity = ge->capacity == 0 ? 4096 : ge->capacity * 2;
            Entry *queue = realloc(ge->queue, capacity * sizeof(*queue));
            if (queue == NULL) {
                OutOfMemory();
            }
            ge->queue = queue;
            ge->capacity = capacity;
        }
    }
    ge->queue[ge->tail].cmd = (uint8_t)cmd;
    ge->queue[ge->tail].param = param;
    ge->tail++;
}

// Steps through a packed word's commands: those without parameters are queued at once, and the first that has some
// waits for them. NOPs and undefined codes take no parameters and are dropped, so a word of four NOPs is a word
// without commands, and the word after the last parameter is the next packed word (GBATEK "Packed commands")
static void NextPacked(PortGe *ge) {
    while (ge->packedLeft > 0) {
        uint32_t cmd = ge->packed & 0xff;
        int count = ParamCount(cmd);
        if (count > 0) {
            ge->packedParams = count;
            return;
        }
        if (count == 0) {
            Enqueue(ge, cmd, 0);
        }
        ge->packed >>= 8;
        ge->packedLeft--;
    }
}

// One logged write: to GXFIFO (and its mirrors), a packed word of up to four command codes or the next parameter of
// the packed command that waits for one; to a command port (0x04000440 and up), one parameter of that port's command.
// Writes to undefined ports are dropped
static void Decode(PortGe *ge, uint32_t port, uint32_t value) {
    if (port >= GXFIFO && port < GXFIFO_END) {
        if (ge->packedLeft == 0) {
            ge->packed = value;
            ge->packedLeft = 4;
            NextPacked(ge);
            return;
        }
        Enqueue(ge, ge->packed & 0xff, value);
        if (--ge->packedParams == 0) {
            ge->packed >>= 8;
            ge->packedLeft--;
            NextPacked(ge);
        }
    } else if (port >= GXFIFO_END && port < PORTS_END) {
        uint32_t cmd = (port - GXFIFO) >> 2;
        if (ParamCount(cmd) >= 0) {
            Enqueue(ge, cmd, value);
        }
    }
}

// --- The interface

PortGe *port_ge_create(void) {
    PortGe *ge = calloc(1, sizeof(*ge));
    if (ge == NULL) {
        return NULL;
    }
    for (int i = 0; i < 3; i++) {
        ge->lists[i] = malloc(sizeof(PortGeList));
        if (ge->lists[i] == NULL) {
            port_ge_destroy(ge);
            return NULL;
        }
    }
    port_ge_reset(ge);
    return ge;
}

void port_ge_destroy(PortGe *ge) {
    if (ge == NULL) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        free(ge->lists[i]);
    }
    free(ge->queue);
    free(ge);
}

void port_ge_reset(PortGe *ge) {
    Entry *queue = ge->queue;
    size_t capacity = ge->capacity;
    PortGeList *lists[3] = { ge->lists[0], ge->lists[1], ge->lists[2] };

    memset(ge, 0, sizeof(*ge));
    ge->queue = queue;
    ge->capacity = capacity;
    for (int i = 0; i < 3; i++) {
        ge->lists[i] = lists[i];
        ClearList(lists[i]);
    }
    ge->render = lists[0];
    ge->building = lists[1];
    ge->spare = lists[2];
    ge->swapped = NULL;
    // DISP_1DOT_DEPTH starts at its farthest, hiding no dot (melonDS's reset; GBATEK gives no initial value)
    ge->oneDotDepth = 0x7fff;
    Unshare(ge);
}

void port_ge_run(PortGe *ge, const PortG3Write *writes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Decode(ge, writes[i].port, writes[i].value);
    }
    Execute(ge);
}

void port_ge_write(PortGe *ge, uint32_t port, uint32_t value) {
    Decode(ge, port, value);
    Execute(ge);
}

void port_ge_set_1dot_depth(PortGe *ge, uint16_t depth) {
    ge->oneDotDepth = depth & 0x7fff;
}

int port_ge_vblank(PortGe *ge) {
    if (ge->swapped == NULL) {
        return 0;
    }
    ge->spare = ge->render;
    ge->render = ge->swapped;
    ge->swapped = NULL;
    Execute(ge);
    return 1;
}

const PortGeList *port_ge_render_list(const PortGe *ge) {
    return ge->render;
}

const PortGeList *port_ge_building_list(const PortGe *ge) {
    return ge->building;
}

int port_ge_swap_pending(const PortGe *ge) {
    return ge->swapped != NULL;
}

size_t port_ge_stalled_entries(const PortGe *ge) {
    return ge->tail - ge->head;
}

uint32_t port_ge_gxstat(const PortGe *ge) {
    uint32_t value = GXSTAT_FIFO_UNDER_HALF | GXSTAT_FIFO_EMPTY;
    if (ge->boxResult) {
        value |= GXSTAT_BOX_RESULT;
    }
    value |= (uint32_t)(ge->posSp & 31) << GXSTAT_POS_LEVEL_SHIFT;
    value |= (uint32_t)ge->projSp << GXSTAT_PROJ_LEVEL_SHIFT;
    if (ge->stackError) {
        value |= GXSTAT_STACK_ERROR;
    }
    value |= ge->fifoIrq << GXSTAT_FIFO_IRQ_SHIFT;
    return value;
}

void port_ge_write_gxstat(PortGe *ge, uint32_t value) {
    if (value & GXSTAT_STACK_ERROR) {
        ge->stackError = 0;
    }
    ge->fifoIrq = value >> GXSTAT_FIFO_IRQ_SHIFT;
}

int port_ge_fifo_irq(const PortGe *ge) {
    return ge->fifoIrq == 1 || ge->fifoIrq == 2;
}

uint16_t port_ge_polygon_count(const PortGe *ge) {
    return (uint16_t)ge->building->polygon_count;
}

uint16_t port_ge_vertex_count(const PortGe *ge) {
    return (uint16_t)ge->building->vertex_count;
}

int port_ge_ram_overflow(const PortGe *ge) {
    return ge->ramOverflow;
}

void port_ge_ack_ram_overflow(PortGe *ge) {
    ge->ramOverflow = 0;
}

void port_ge_clip_matrix(PortGe *ge, uint32_t out[16]) {
    UpdateClip(ge);
    for (int i = 0; i < 16; i++) {
        out[i] = (uint32_t)ge->clip[i];
    }
}

void port_ge_vector_matrix(const PortGe *ge, uint32_t out[9]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out[i * 3 + j] = (uint32_t)ge->vec[i * 4 + j];
        }
    }
}

void port_ge_position_result(const PortGe *ge, uint32_t out[4]) {
    for (int i = 0; i < 4; i++) {
        out[i] = (uint32_t)ge->posResult[i];
    }
}

void port_ge_vector_result(const PortGe *ge, uint16_t out[3]) {
    for (int i = 0; i < 3; i++) {
        out[i] = ge->vecResult[i];
    }
}
