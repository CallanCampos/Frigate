// The CPU reference renderer of the DS's 3D rendering engine (ds3d.h). A frame is drawn in the hardware's order: the
// rear plane, the opaque polygons sorted by Y, the translucent ones sorted by Y or in the order they came, then edge
// marking, fog and anti-aliasing over the whole picture. Each polygon is scanned line by line between its left and
// right edges, which are walked the way the hardware walks them, with its integer screen coordinates (the DS has no
// sub-pixel precision). The buffers keep two layers: the pixel on top and, under an opaque edge pixel while
// anti-aliasing is on, the one beneath it, which the edge blends with by its coverage.
//
// Facts are GBATEK's unless a comment says they are melonDS's measured behaviour or the port's choice.
#include "gfx/ref/ds3d.h"
#include <stdlib.h>
#include <string.h>

#define WIDTH PORT_DS3D_WIDTH
#define HEIGHT PORT_DS3D_HEIGHT

// DISP3DCNT
#define CNT_TEXTURE 0x0001u
#define CNT_HIGHLIGHT 0x0002u // polygon mode 2 shades with highlights instead of toon colours
#define CNT_ALPHA_TEST 0x0004u
#define CNT_BLEND 0x0008u
#define CNT_ANTIALIAS 0x0010u
#define CNT_EDGE_MARK 0x0020u
#define CNT_FOG_ALPHA_ONLY 0x0040u
#define CNT_FOG 0x0080u
#define CNT_FOG_SHIFT(c) (((c) >> 8) & 15)
#define CNT_CLEAR_IMAGE 0x4000u

// POLYGON_ATTR
#define ATTR_MODE(a) (((a) >> 4) & 3)
#define ATTR_TRANSLUCENT_DEPTH (1u << 11) // translucent pixels write their depth
#define ATTR_DEPTH_EQUAL (1u << 14)
#define ATTR_FOG (1u << 15)
#define ATTR_ALPHA(a) (((a) >> 16) & 31)
#define ATTR_ID(a) (((a) >> 24) & 63)
enum {
    MODE_MODULATE,
    MODE_DECAL,
    MODE_TOON,
    MODE_SHADOW,
};

// TEXIMAGE_PARAM
#define TEX_REPEAT_S (1u << 16)
#define TEX_REPEAT_T (1u << 17)
#define TEX_FLIP_S (1u << 18)
#define TEX_FLIP_T (1u << 19)
#define TEX_SIZE_S(p) (8 << (((p) >> 20) & 7))
#define TEX_SIZE_T(p) (8 << (((p) >> 23) & 7))
#define TEX_FORMAT(p) (((p) >> 26) & 7)
#define TEX_COLOR0_TRANSPARENT (1u << 29)
enum {
    FORMAT_NONE,
    FORMAT_A3I5,
    FORMAT_4_COLOR,
    FORMAT_16_COLOR,
    FORMAT_256_COLOR,
    FORMAT_4x4,
    FORMAT_A5I3,
    FORMAT_DIRECT,
};

// A pixel's colour: red, green and blue in 6 bits at bits 0, 6 and 12, alpha in 5 bits at bit 18
#define PIXEL(r, g, b, a) ((uint32_t)(r) | (uint32_t)(g) << 6 | (uint32_t)(b) << 12 | (uint32_t)(a) << 18)
#define PIXEL_R(p) ((p) & 63)
#define PIXEL_G(p) (((p) >> 6) & 63)
#define PIXEL_B(p) (((p) >> 12) & 63)
#define PIXEL_A(p) (((p) >> 18) & 31)

// A pixel's attributes: the polygon ID of the opaque polygon there (or the clear ID), the ID of the last translucent
// polygon drawn over it and whether there is one, the fog flag, whether the opaque polygon faces away, and the edge
// flags of the opaque polygon (left, right, top and bottom edge)
#define PA_ID(a) ((a) & 63)
#define PA_TRANSLUCENT_ID_SHIFT 6
#define PA_TRANSLUCENT_ID(a) (((a) >> PA_TRANSLUCENT_ID_SHIFT) & 63)
#define PA_TRANSLUCENT (1u << 12)
#define PA_FOG (1u << 13)
#define PA_BACK (1u << 14)
#define PA_EDGE_SHIFT 16
#define PA_EDGES (15u << PA_EDGE_SHIFT)
#define EDGE_LEFT 1u
#define EDGE_RIGHT 2u
#define EDGE_TOP 4u
#define EDGE_BOTTOM 8u

// Anti-aliasing coverage, 0..31: 31 covers the pixel, c < 31 shows (c + 1) / 32 of it over the pixel beneath, 0 shows
// only the pixel beneath
#define FULL_COVERAGE 31
// What edge marking leaves as the coverage of a pixel it marks (melonDS's measurement, which it marks as unsure)
#define EDGE_MARK_COVERAGE 16

// The frame's buffers: the top layer's colour, depth (24 bits), attributes and anti-aliasing coverage, the shadow
// stencil, and the layer beneath (colour, depth, attributes), which holds what an opaque edge pixel covered while
// anti-aliasing is on
static uint32_t sColor[HEIGHT * WIDTH];
static uint32_t sDepth[HEIGHT * WIDTH];
static uint32_t sAttr[HEIGHT * WIDTH];
static uint8_t sStencil[HEIGHT * WIDTH];
static uint8_t sCoverage[HEIGHT * WIDTH];
static uint32_t sBelowColor[HEIGHT * WIDTH];
static uint32_t sBelowDepth[HEIGHT * WIDTH];
static uint32_t sBelowAttr[HEIGHT * WIDTH];

typedef struct Frame {
    const PortDs3dSource *src;
    uint32_t cnt;
    int wBuffer;   // depth is W (SWAP_BUFFERS bit 1), else Z
    int antialias; // DISP3DCNT bit 4
    int fillEdges; // anti-aliasing or edge marking: every edge's pixels belong to its polygon
    int lastWasShadowMask;
    uint32_t toon[32]; // the toon table as pixels
} Frame;

// A 5-bit channel widened to 6 bits: 0 stays 0, c becomes c * 2 + 1 (melonDS's measurement; GBATEK says only that
// the engine works in 6 bits). Vertex colours come from the geometry engine as c * 16 + 15, which is the same with
// three fractional bits
static uint32_t Widen(uint32_t c5) {
    return c5 != 0 ? c5 * 2 + 1 : 0;
}

static uint32_t Rgb15(uint32_t color, uint32_t alpha) {
    return PIXEL(Widen(color & 31), Widen((color >> 5) & 31), Widen((color >> 10) & 31), alpha);
}

static uint32_t Read16(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

// --- Textures (GBATEK "DS 3D Texture Formats")

static uint32_t TexRead8(const Frame *f, uint32_t address) {
    address &= 0x7ffff;
    const uint8_t *page = f->src->tex_pages[address >> 14];
    return page != NULL ? page[address & 0x3fff] : 0;
}

static uint32_t TexRead16(const Frame *f, uint32_t address) {
    address &= 0x7fffe;
    const uint8_t *page = f->src->tex_pages[address >> 14];
    return page != NULL ? Read16(page + (address & 0x3fff)) : 0;
}

// The palette space is 96 KiB; PLTT_BASE reaches 128 KiB, and past the sixth page nothing answers
static uint32_t PaletteRead(const Frame *f, uint32_t address) {
    address &= 0x1fffe;
    if ((address >> 14) >= 6) {
        return 0;
    }
    const uint8_t *page = f->src->texpltt_pages[address >> 14];
    return page != NULL ? Read16(page + (address & 0x3fff)) : 0;
}

// A texture coordinate in texels inside a texture of size texels: repeated (mirrored on odd repeats with flip) or
// clamped to the edge
static int32_t WrapTexel(int32_t c, int32_t size, int repeat, int flip) {
    if (repeat) {
        if (flip && (c & size)) {
            return (size - 1) - (c & (size - 1));
        }
        return c & (size - 1);
    }
    return c < 0 ? 0 : c >= size ? size - 1 : c;
}

// One channel of a 4x4 block's mixed colours: (c0 * w0 + c1 * w1) / 8 on 5-bit channels
static uint32_t Mix15(uint32_t c0, uint32_t c1, uint32_t w0, uint32_t w1) {
    uint32_t r = ((c0 & 31) * w0 + (c1 & 31) * w1) / 8;
    uint32_t g = (((c0 >> 5) & 31) * w0 + ((c1 >> 5) & 31) * w1) / 8;
    uint32_t b = (((c0 >> 10) & 31) * w0 + ((c1 >> 10) & 31) * w1) / 8;
    return r | g << 5 | b << 10;
}

// The texel at the 1.11.4 coordinates (s, t), as a pixel with its alpha (31 opaque, 0 transparent, or the A3I5 and
// A5I3 formats' alpha)
static uint32_t Texel(const Frame *f, const PortGePolygon *p, int32_t s, int32_t t) {
    uint32_t param = p->tex_param;
    int32_t width = TEX_SIZE_S(param);
    int32_t height = TEX_SIZE_T(param);
    uint32_t address = (param & 0xffff) << 3;
    uint32_t palette = p->pltt_base << 4;
    int transparent0 = (param & TEX_COLOR0_TRANSPARENT) != 0;
    uint32_t index;
    uint32_t value;

    s = WrapTexel(s >> 4, width, (param & TEX_REPEAT_S) != 0, (param & TEX_FLIP_S) != 0);
    t = WrapTexel(t >> 4, height, (param & TEX_REPEAT_T) != 0, (param & TEX_FLIP_T) != 0);
    uint32_t texel = (uint32_t)(t * width + s);
    switch (TEX_FORMAT(param)) {
    case FORMAT_A3I5: {
        // A 3-bit alpha widened to 5 bits as a * 4 + a / 2
        value = TexRead8(f, address + texel);
        uint32_t a = value >> 5;
        return Rgb15(PaletteRead(f, palette + (value & 31) * 2), a * 4 + a / 2);
    }
    case FORMAT_4_COLOR:
        // Its palette base is in units of 8 bytes, not 16
        index = (TexRead8(f, address + texel / 4) >> ((texel & 3) * 2)) & 3;
        if (index == 0 && transparent0) {
            return 0;
        }
        return Rgb15(PaletteRead(f, (p->pltt_base << 3) + index * 2), 31);
    case FORMAT_16_COLOR:
        index = (TexRead8(f, address + texel / 2) >> ((texel & 1) * 4)) & 15;
        if (index == 0 && transparent0) {
            return 0;
        }
        return Rgb15(PaletteRead(f, palette + index * 2), 31);
    case FORMAT_256_COLOR:
        index = TexRead8(f, address + texel);
        if (index == 0 && transparent0) {
            return 0;
        }
        return Rgb15(PaletteRead(f, palette + index * 2), 31);
    case FORMAT_4x4: {
        // Blocks of 4x4 texels, a word of 2-bit texels each, in slot 0 or 2; each block's palette index halfword is
        // in slot 1, at 0x20000 plus half the block's offset in its slot, plus 0x10000 for slot 2
        uint32_t block = address + (uint32_t)((t >> 2) * (width >> 2) + (s >> 2)) * 4;
        uint32_t word = TexRead16(f, block) | TexRead16(f, block + 2) << 16;
        uint32_t code = (word >> ((t & 3) * 8 + (s & 3) * 2)) & 3;
        uint32_t info = TexRead16(f, 0x20000 + ((block & 0x1ffff) >> 1) + ((block & 0x40000) ? 0x10000 : 0));
        uint32_t base = palette + (info & 0x3fff) * 4;
        uint32_t mode = info >> 14;
        if (code < 2 || (code == 2 && (mode & 1) == 0) || (code == 3 && mode == 2)) {
            // Colours 0 and 1 always, 2 in modes 0 and 2, 3 in mode 2 come from the palette
            return Rgb15(PaletteRead(f, base + code * 2), 31);
        }
        if (code == 3 && mode < 2) {
            return 0; // transparent in modes 0 and 1
        }
        uint32_t c0 = PaletteRead(f, base);
        uint32_t c1 = PaletteRead(f, base + 2);
        if (mode == 1) {
            return Rgb15(Mix15(c0, c1, 4, 4), 31); // (c0 + c1) / 2
        }
        return Rgb15(code == 2 ? Mix15(c0, c1, 5, 3) : Mix15(c0, c1, 3, 5), 31);
    }
    case FORMAT_A5I3:
        value = TexRead8(f, address + texel);
        return Rgb15(PaletteRead(f, palette + (value & 7) * 2), value >> 3);
    case FORMAT_DIRECT:
        value = TexRead16(f, address + texel * 2);
        return Rgb15(value, (value & 0x8000) ? 31 : 0);
    default:
        return PIXEL(63, 63, 63, 31);
    }
}

// --- Interpolation

// Perspective-correct interpolation as the hardware does it (melonDS's measurements; GBATEK says only that it is
// perspective-correct): over a length len from the end with W w0 to the end with W w1, the far end weighs
// x * w0 / (x * w0 + (len - x) * w1), computed with 9 fractional bits along edges and 8 across spans. Along edges the
// lowest bit of the W values is dropped, except that an odd w0 with an even w1 is taken as w0 - 1 above and w0 + 1
// below. When both W are equal and their bits 1-6 (edges) or 0-6 (spans) are clear the hardware interpolates
// linearly, exactly. Ws are the polygon's, normalised to 16 bits
typedef struct Persp {
    int32_t len;
    int64_t w0Num;
    int64_t w0Den;
    int64_t w1Den;
    int shift;
    int linear;
} Persp;

// Prepares the interpolation over len from the end with W w0 to the end with w1, along an edge (edge set) or across
// a span, by the rules above
static void PerspSetup(Persp *p, int32_t len, int32_t w0, int32_t w1, int edge) {
    p->len = len;
    p->linear = w0 == w1 && (w0 & (edge ? 0x7e : 0x7f)) == 0;
    if (edge) {
        if ((w0 & 1) && !(w1 & 1)) {
            p->w0Num = w0 - 1;
            p->w0Den = w0 + 1;
            p->w1Den = w1;
        } else {
            p->w0Num = w0 & ~1;
            p->w0Den = w0 & ~1;
            p->w1Den = w1 & ~1;
        }
        p->shift = 9;
    } else {
        p->w0Num = w0;
        p->w0Den = w0;
        p->w1Den = w1;
        p->shift = 8;
    }
}

// The far end's weight at x, in 1 << shift
static int32_t PerspFactor(const Persp *p, int32_t x) {
    int64_t num = ((int64_t)x * p->w0Num) << p->shift;
    int64_t den = (int64_t)x * p->w0Den + (int64_t)(p->len - x) * p->w1Den;
    return den != 0 ? (int32_t)(num / den) : 0;
}

// The interpolated value, worked out from the lower end so that both directions round alike
static int32_t PerspValue(int32_t a0, int32_t a1, int32_t factor, int shift) {
    if (a0 <= a1) {
        return a0 + (int32_t)(((int64_t)(a1 - a0) * factor) >> shift);
    }
    return a1 + (int32_t)(((int64_t)(a0 - a1) * ((1 << shift) - factor)) >> shift);
}

static int32_t LinearValue(int32_t a0, int32_t a1, int32_t x, int32_t len) {
    if (len <= 0 || a0 == a1) {
        return a0;
    }
    if (a0 < a1) {
        return a0 + (int32_t)((int64_t)(a1 - a0) * x / len);
    }
    return a1 + (int32_t)((int64_t)(a0 - a1) * (len - x) / len);
}

// LinearValue at x, x + 1, ... without a division at each step: a quotient and a remainder carried along
typedef struct Lerp {
    int32_t value;
    int32_t q;
    int32_t r;
    int32_t acc;
    int32_t len;
    int down;
} Lerp;

// Starts at x of len, from a0 at 0 to a1 at len
static void LerpStart(Lerp *l, int32_t a0, int32_t a1, int32_t len, int32_t x) {
    int64_t d = a0 <= a1 ? (int64_t)a1 - a0 : (int64_t)a0 - a1;
    int64_t n = d * (a0 <= a1 ? x : len - x);
    l->len = len;
    l->down = a0 > a1;
    l->value = (int32_t)((a0 <= a1 ? a0 : a1) + n / len);
    l->acc = (int32_t)(n % len);
    l->q = (int32_t)(d / len);
    l->r = (int32_t)(d % len);
}

// Moves to the next x
static void LerpStep(Lerp *l) {
    if (!l->down) {
        l->value += l->q;
        l->acc += l->r;
        if (l->acc >= l->len) {
            l->acc -= l->len;
            l->value++;
        }
    } else {
        l->value -= l->q;
        l->acc -= l->r;
        if (l->acc < 0) {
            l->acc += l->len;
            l->value--;
        }
    }
}

// --- Polygons

// What the rasterizer interpolates: red, green, blue (c * 16 + 15 for a 5-bit c above 0, else 0: the 6-bit colour with
// three fractional bits), s, t
#define VALUES 5

typedef struct Vertex {
    int32_t x;
    int32_t y;
    int32_t w; // W normalised to 16 bits for the polygon
    int32_t depth;
    int32_t a[VALUES];
} Vertex;

// An edge's values at one line
typedef struct Side {
    int32_t w;
    int32_t depth;
    int32_t a[VALUES];
} Side;

// Positions along an edge are in pixels with 18 fractional bits
#define POS_ONE (1 << 18)
#define POS_HALF (1 << 17)

// An edge from a vertex down to another, walked the way the hardware walks it (melonDS's measurements; GBATEK doesn't
// describe the rasterizer). Its slope is |dx| / dy with 18 fractional bits, worked out as |dx| times (1 << 18) / dy
// rounded down (the reciprocal of the height first), exactly 1.0 when |dx| == dy. An edge up to 45 degrees from the
// vertical is Y-major and has one pixel on each line; a flatter one is X-major and crosses a run of pixels on each
// line. The walker keeps a position that starts at an offset (EdgeStart) and grows by the slope at each line; its pixel
// is that many whole pixels from the top vertex, rightward or leftward, kept within the columns between the two
// vertices. Values along the edge are interpolated by line, from its top vertex to its bottom one, whatever its major
// axis; an edge at 45 degrees or flatter that runs outward (a left edge running left, a right edge running right)
// interpolates as if it began a line higher (melonDS's measurements)
typedef struct Edge {
    const Vertex *v0;
    const Vertex *v1;
    int32_t x0;
    int32_t y0;
    int32_t xMin; // the walker's pixel stays in [xMin, xMax]
    int32_t xMax;
    int32_t adx;
    int32_t dy;
    int32_t slope;  // 0 for a vertical edge (or one with no height)
    int32_t interp; // the lines its values are interpolated from above y0: 0, or 1 when it begins a line higher
    int negative;   // runs to the left
    int xMajor;
    Persp persp;
} Edge;

// right: the edge is the polygon's right side
static void EdgeSetup(Edge *e, const Vertex *v0, const Vertex *v1, int right) {
    int32_t dx = v1->x - v0->x;
    int32_t dy = v1->y - v0->y;
    int32_t adx = dx < 0 ? -dx : dx;
    e->v0 = v0;
    e->v1 = v1;
    e->x0 = v0->x;
    e->y0 = v0->y;
    e->xMin = dx < 0 ? v1->x : v0->x;
    e->xMax = dx > 0 ? v1->x - 1 : dx < 0 ? v0->x - 1 : v0->x;
    e->adx = adx;
    e->dy = dy;
    e->negative = dx < 0;
    if (dy <= 0) {
        e->slope = 0;
    } else if (adx == dy) {
        e->slope = POS_ONE;
    } else {
        e->slope = adx * (POS_ONE / dy);
    }
    e->xMajor = e->slope > POS_ONE;
    e->interp = e->slope >= POS_ONE && right != e->negative;
    PerspSetup(&e->persp, dy > 0 ? dy : 0, v0->w, v1->w, 1);
}

// Where the walker's position starts. On each line its pixel is, for a Y-major edge, the one the edge is in at the
// top of the line: the position starts at 0, or at one pixel when the edge runs left, so that a leftward edge on a
// pixel boundary is in the pixel left of it. For an X-major edge it is an end of the run of pixels whose middles lie
// between the edge at the top of the line and at the top of the next: the left end on the polygon's left side, the
// right end on its right side. Running right, the position starts at half a pixel on the left side and at the slope
// less half a pixel on the right; running left, at the slope plus half a pixel on the left and one and a half pixels
// on the right
static int64_t EdgePos(const Edge *e, int32_t y, int right) {
    int64_t start = 0;
    if (e->xMajor) {
        if (!e->negative) {
            start = right ? e->slope - POS_HALF : POS_HALF;
        } else {
            start = right ? POS_ONE + POS_HALF : e->slope + POS_HALF;
        }
    } else if (e->negative && e->slope != 0) {
        start = POS_ONE;
    }
    return start + (int64_t)(y - e->y0) * e->slope;
}

static int32_t EdgeX(const Edge *e, int64_t pos) {
    int32_t steps = (int32_t)(pos >> 18);
    int32_t x = e->negative ? e->x0 - steps : e->x0 + steps;
    return x < e->xMin ? e->xMin : x > e->xMax ? e->xMax : x;
}

// An edge on one line, as a side of the span: how many pixels its run has from the walker's pixel inward, and their
// anti-aliasing coverage, a constant (Y-major and vertical edges) or, for an X-major edge, the coverage of the run's
// leftmost pixel and its step per pixel to the right, both in 1/1024
typedef struct EdgeLine {
    int32_t run;
    uint32_t cov;
    uint32_t covStep;
    int xMajor;
} EdgeLine;

// The run and coverage of an edge on the line where the walker is at pos (melonDS's measurements). right is the side
// the edge was walked as; swapped is set when the span's two edges crossed, so that the edge is drawn on the other
// side.
//
// An X-major edge's run is the pixels its position passes over between this line and the next on its side's end:
// toward the next line on the left side running right and the right side running left, back toward the line before
// otherwise. Its coverage is how much of the line's height is inside the polygon at each pixel's middle column: the
// edge's height there above or below the vertex it is measured from (the top one running right, the bottom one running
// left, offset being the pixel's distance from it), in 1/1024 of a line: (offset * 1024 + 0x1ff) * dy / |dx| for the
// leftmost pixel and (dy << 10) / |dx| more for each pixel to its right, keeping the part within the line (a start of
// 0x3ff counts as 0). The left side shows that part divided by 32, at most 31, the right side 31 less it, at least 0.
//
// A Y-major edge's coverage is the part of its pixel on the inside at the line's middle row, in 1/32: the position plus
// half a step, from the pixel's left side when the edge runs right and its right side when it runs left, and the whole
// pixel when it is past the pixel by then; the left side running right and the right side running left take 31 less
// that. A vertical edge covers its pixels fully.
//
// When the edges have crossed, melonDS measured the hardware getting them wrong: each run is one pixel, a Y-major
// edge's coverage takes the other part and a vertical one's is 0
static void EdgeLineSetup(const Edge *e, int64_t pos, int right, int swapped, EdgeLine *l) {
    l->xMajor = e->xMajor;
    l->run = 1;
    l->covStep = 0;
    if (e->xMajor) {
        int32_t here = (int32_t)(pos >> 18);
        int32_t run =
            right != e->negative ? here - (int32_t)((pos - e->slope) >> 18) : (int32_t)((pos + e->slope) >> 18) - here;
        int32_t offset = e->negative ? e->adx - here : here;
        if (right) {
            offset -= run - 1;
        }
        uint32_t start = (uint32_t)(((int64_t)offset * 1024 + 0x1ff) * e->dy / e->adx) & 0x3ff;
        l->cov = start == 0x3ff ? 0 : start;
        l->covStep = (uint32_t)((e->dy << 10) / e->adx);
        l->run = swapped ? 1 : run;
    } else if (e->slope == 0) {
        l->cov = swapped ? 0 : FULL_COVERAGE;
    } else {
        int64_t middle = ((pos >> 9) + (e->slope >> 10)) >> 4;
        uint32_t part = (middle >> 5) == (pos >> 18) ? (uint32_t)(middle & 31) : FULL_COVERAGE;
        int other = swapped ? right != e->negative : right == e->negative;
        l->cov = other ? FULL_COVERAGE - part : part;
    }
}

// The coverage of the pixel k pixels right of the run's leftmost one
static uint32_t RunCoverage(const EdgeLine *l, int right, int32_t k) {
    if (!l->xMajor) {
        return l->cov;
    }
    uint32_t part = (l->cov + (uint32_t)k * l->covStep) >> 5;
    if (right) {
        return part >= FULL_COVERAGE ? 0 : FULL_COVERAGE - part;
    }
    return part > FULL_COVERAGE ? FULL_COVERAGE : part;
}

// The edge's values on line y. Z-buffer depth is interpolated linearly in screen space and everything else
// perspective-correctly, W-buffer depth included
static void EdgeValues(const Frame *f, const Edge *e, int32_t y, Side *s) {
    const Vertex *a = e->v0;
    const Vertex *b = e->v1;
    int32_t len = e->persp.len;
    int32_t pos = y - e->y0 + e->interp;
    pos = pos < 0 ? 0 : pos > len ? len : pos;
    if (len <= 0) {
        s->w = a->w;
        s->depth = a->depth;
        memcpy(s->a, a->a, sizeof(s->a));
        return;
    }
    if (e->persp.linear) {
        s->w = a->w;
        s->depth = LinearValue(a->depth, b->depth, pos, len);
        for (int i = 0; i < VALUES; i++) {
            s->a[i] = LinearValue(a->a[i], b->a[i], pos, len);
        }
        return;
    }
    int32_t factor = PerspFactor(&e->persp, pos);
    s->w = PerspValue(a->w, b->w, factor, 9);
    s->depth = f->wBuffer ? PerspValue(a->depth, b->depth, factor, 9) : LinearValue(a->depth, b->depth, pos, len);
    for (int i = 0; i < VALUES; i++) {
        s->a[i] = PerspValue(a->a[i], b->a[i], factor, 9);
    }
}

// What one polygon needs at each pixel
typedef struct Poly {
    const PortGePolygon *p;
    uint32_t attr;
    uint32_t id;
    uint32_t alpha; // 31 for a wireframe polygon
    int mode;
    int textured;
    int wire;
    int front; // faces the viewer (PORT_GE_POLYGON_FRONT)
} Poly;

// The depth test (GBATEK "POLYGON_ATTR" bit 14): less, or equal within a margin, 0x200 for Z and 0xff for W
// (melonDS's measurement; GBATEK: "equal" allows small differences). A polygon facing the viewer passes "less" at an
// equal depth too over an opaque pixel of a polygon facing away (melonDS's measurement), so that the front of
// double-sided or coplanar geometry wins over its back
static int DepthPasses(const Frame *f, const Poly *poly, uint32_t depth, uint32_t old, uint32_t oldAttr) {
    if (poly->attr & ATTR_DEPTH_EQUAL) {
        uint32_t margin = f->wBuffer ? 0xff : 0x200;
        return depth + margin >= old && depth <= old + margin;
    }
    if (poly->front && (oldAttr & (PA_BACK | PA_TRANSLUCENT)) == PA_BACK) {
        return depth <= old;
    }
    return depth < old;
}

// A translucent pixel blended into one layer: not over a translucent pixel of the same polygon ID, so a model's own
// surfaces don't blend twice; with blending on, (source * (alpha + 1) + destination * (31 - alpha)) / 32 and the
// larger alpha, except over alpha 0, where the source is written as it is. The fog flag is ANDed with the one beneath;
// the opaque polygon's ID, facing and edges stay
static void BlendTranslucent(const Frame *f, const Poly *poly, uint32_t *color, uint32_t *depthBuffer, uint32_t *attrs,
                             uint32_t r, uint32_t g, uint32_t b, uint32_t alpha, uint32_t depth) {
    uint32_t attr = *attrs;
    if ((attr & PA_TRANSLUCENT) && PA_TRANSLUCENT_ID(attr) == poly->id) {
        return;
    }
    uint32_t old = *color;
    uint32_t oldAlpha = PIXEL_A(old);
    if ((f->cnt & CNT_BLEND) && oldAlpha != 0) {
        r = (r * (alpha + 1) + PIXEL_R(old) * (31 - alpha)) >> 5;
        g = (g * (alpha + 1) + PIXEL_G(old) * (31 - alpha)) >> 5;
        b = (b * (alpha + 1) + PIXEL_B(old) * (31 - alpha)) >> 5;
        alpha = alpha > oldAlpha ? alpha : oldAlpha;
    }
    *color = PIXEL(r, g, b, alpha);
    if (poly->attr & ATTR_TRANSLUCENT_DEPTH) {
        *depthBuffer = depth;
    }
    uint32_t fog = (attr & PA_FOG) && (poly->attr & ATTR_FOG) ? PA_FOG : 0;
    *attrs = (attr & ~(PA_FOG | (63u << PA_TRANSLUCENT_ID_SHIFT))) | PA_TRANSLUCENT |
             poly->id << PA_TRANSLUCENT_ID_SHIFT | fog;
}

// One pixel of a polygon: the colour from the vertex colour, the texel and the polygon mode, the alpha test, then
// the depth test and the opaque or translucent write (GBATEK "DS 3D Texture Blending", "Alpha-Blending", "Shadow
// Polygons"). With anti-aliasing on, an opaque edge pixel pushes the pixel it covers into the layer beneath, and a
// pixel that fails the depth test against an edge pixel is tested against the one beneath it and can go there instead
// (melonDS's measurements)
static void DrawPixel(Frame *f, const Poly *poly, int index, uint32_t depth, const int32_t *a, uint32_t edges,
                      uint32_t coverage) {
    uint32_t attr = sAttr[index];

    if (poly->mode == MODE_SHADOW) {
        if (poly->id == 0) {
            // The shadow mask: marks where its depth test fails, draws nothing
            if (!DepthPasses(f, poly, depth, sDepth[index], attr)) {
                sStencil[index] = 1;
            }
            return;
        }
        // A shadow: only where the mask marked, and not on the polygon that casts it (same ID)
        if (!sStencil[index] || PA_ID(attr) == poly->id) {
            return;
        }
    }

    uint32_t vr = (uint32_t)a[0] >> 3;
    uint32_t vg = (uint32_t)a[1] >> 3;
    uint32_t vb = (uint32_t)a[2] >> 3;
    uint32_t hr = 0;
    uint32_t hg = 0;
    uint32_t hb = 0;
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t alpha;
    if (poly->mode == MODE_TOON) {
        // The toon table is indexed by the vertex colour's red (5 bits). Toon shading takes its colour; highlight
        // shading keeps red for all three channels and adds the table's colour after texturing
        uint32_t toon = f->toon[vr >> 1];
        if (f->cnt & CNT_HIGHLIGHT) {
            hr = PIXEL_R(toon);
            hg = PIXEL_G(toon);
            hb = PIXEL_B(toon);
            vg = vb = vr;
        } else {
            vr = PIXEL_R(toon);
            vg = PIXEL_G(toon);
            vb = PIXEL_B(toon);
        }
    }
    if (poly->textured) {
        uint32_t texel = Texel(f, poly->p, a[3], a[4]);
        uint32_t ta = PIXEL_A(texel);
        if (poly->mode == MODE_DECAL) {
            // The texel over the vertex colour by the texel's alpha; the polygon's alpha stays
            if (ta == 0) {
                r = vr;
                g = vg;
                b = vb;
            } else if (ta == 31) {
                r = PIXEL_R(texel);
                g = PIXEL_G(texel);
                b = PIXEL_B(texel);
            } else {
                r = (PIXEL_R(texel) * ta + vr * (31 - ta)) >> 5;
                g = (PIXEL_G(texel) * ta + vg * (31 - ta)) >> 5;
                b = (PIXEL_B(texel) * ta + vb * (31 - ta)) >> 5;
            }
            alpha = poly->alpha;
        } else {
            // Modulation: ((c1 + 1) * (c2 + 1) - 1) / 64 on 6-bit channels, / 32 on 5-bit alphas
            r = ((PIXEL_R(texel) + 1) * (vr + 1) - 1) >> 6;
            g = ((PIXEL_G(texel) + 1) * (vg + 1) - 1) >> 6;
            b = ((PIXEL_B(texel) + 1) * (vb + 1) - 1) >> 6;
            alpha = ((ta + 1) * (poly->alpha + 1) - 1) >> 5;
        }
    } else {
        r = vr;
        g = vg;
        b = vb;
        alpha = poly->alpha;
    }
    if (hr | hg | hb) {
        r = r + hr > 63 ? 63 : r + hr;
        g = g + hg > 63 ? 63 : g + hg;
        b = b + hb > 63 ? 63 : b + hb;
    }
    // A pixel with alpha 0 is never drawn; with the alpha test on, only alphas above the reference are
    if (alpha == 0 || ((f->cnt & CNT_ALPHA_TEST) && alpha <= f->src->regs.alpha_test_ref)) {
        return;
    }

    // Under an anti-aliased edge pixel, the pixel beneath it can take what fails against it
    int layered = f->antialias && (attr & PA_EDGES);
    if (alpha == 31) {
        uint32_t newAttr =
            poly->id | ((poly->attr & ATTR_FOG) ? PA_FOG : 0) | (poly->front ? 0 : PA_BACK) | edges << PA_EDGE_SHIFT;
        if (DepthPasses(f, poly, depth, sDepth[index], attr)) {
            if (f->antialias && edges) {
                sBelowColor[index] = sColor[index];
                sBelowDepth[index] = sDepth[index];
                sBelowAttr[index] = attr;
            }
            sCoverage[index] = (uint8_t)(edges ? coverage : FULL_COVERAGE);
            sColor[index] = PIXEL(r, g, b, 31);
            sDepth[index] = depth;
            sAttr[index] = newAttr;
        } else if (layered && DepthPasses(f, poly, depth, sBelowDepth[index], sBelowAttr[index])) {
            sBelowColor[index] = PIXEL(r, g, b, 31);
            sBelowDepth[index] = depth;
            sBelowAttr[index] = newAttr;
        }
        return;
    }

    // Translucent: over the top pixel, and over the one beneath an anti-aliased edge pixel too
    if (DepthPasses(f, poly, depth, sDepth[index], attr)) {
        BlendTranslucent(f, poly, &sColor[index], &sDepth[index], &sAttr[index], r, g, b, alpha, depth);
        if (layered && poly->mode != MODE_SHADOW) {
            BlendTranslucent(f, poly, &sBelowColor[index], &sBelowDepth[index], &sBelowAttr[index], r, g, b, alpha,
                             depth);
        }
    } else if (layered && poly->mode != MODE_SHADOW &&
               DepthPasses(f, poly, depth, sBelowDepth[index], sBelowAttr[index])) {
        BlendTranslucent(f, poly, &sBelowColor[index], &sBelowDepth[index], &sBelowAttr[index], r, g, b, alpha, depth);
    }
}

// One line of a polygon. The span runs from the left edge's pixel to the right edge's, both included, and values are
// interpolated across it; it is drawn in three parts: the left edge's run, the inside, the right edge's run, each
// beginning where the one before ended. An edge's run is drawn only when its fill rule says so; a wireframe polygon
// draws its edges' runs and, on its top and bottom lines, the inside
typedef struct Span {
    int32_t left;  // the left edge's pixel
    int32_t right; // the right edge's pixel
    const Edge *el;
    const Edge *er;
    EdgeLine ll;
    EdgeLine rl;
    int fillLeft;
    int fillRight;
} Span;

// Draws one line of a polygon, the span above, on line y: its colour, depth and texture coordinates interpolated
// across it, each pixel through the depth test, the polygon's mode and the translucency rules. lineEdges says
// whether y is the polygon's top or bottom line (EDGE_TOP, EDGE_BOTTOM), for the pixels' edge flags and wireframes
static void DrawSpan(Frame *f, const Poly *poly, int32_t y, const Span *span, uint32_t lineEdges) {
    int32_t left = span->left;
    int32_t end = span->right + 1;
    int32_t len = end - left;
    // The parts: the left run [left, leftEnd), the inside [leftEnd, insideEnd), the right run [insideEnd, end). Where
    // the runs overlap the left one has the pixels, and the right one's coverage starts at its first pixel drawn
    int32_t leftEnd = left + span->ll.run < end ? left + span->ll.run : end;
    int32_t insideEnd = end - span->rl.run > leftEnd ? end - span->rl.run : leftEnd;
    int32_t x0 = span->fillLeft ? left : leftEnd;
    int32_t x1 = span->fillRight ? end : insideEnd;
    Persp persp;
    Lerp lerp[VALUES];
    Lerp depthLerp;
    int depthLinear;
    Side sl;
    Side sr;

    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 > WIDTH ? WIDTH : x1;
    if (x0 >= x1) {
        return;
    }
    EdgeValues(f, span->el, y, &sl);
    EdgeValues(f, span->er, y, &sr);
    PerspSetup(&persp, len, sl.w, sr.w, 0);
    depthLinear = persp.linear || !f->wBuffer;
    if (persp.linear) {
        for (int i = 0; i < VALUES; i++) {
            LerpStart(&lerp[i], sl.a[i], sr.a[i], len, x0 - left);
        }
    }
    if (depthLinear) {
        LerpStart(&depthLerp, sl.depth, sr.depth, len, x0 - left);
    }
    for (int32_t x = x0; x < x1; x++) {
        int32_t values[VALUES];
        uint32_t depth = 0;
        uint32_t edges = lineEdges;
        uint32_t coverage = FULL_COVERAGE;

        if (persp.linear) {
            for (int i = 0; i < VALUES; i++) {
                values[i] = lerp[i].value;
                LerpStep(&lerp[i]);
            }
        } else {
            int32_t factor = PerspFactor(&persp, x - left);
            for (int i = 0; i < VALUES; i++) {
                values[i] = PerspValue(sl.a[i], sr.a[i], factor, 8);
            }
            if (!depthLinear) {
                depth = (uint32_t)PerspValue(sl.depth, sr.depth, factor, 8);
            }
        }
        if (depthLinear) {
            depth = (uint32_t)depthLerp.value;
            LerpStep(&depthLerp);
        }
        if (x < leftEnd) {
            edges |= EDGE_LEFT;
            coverage = RunCoverage(&span->ll, 0, x - left);
        } else if (x >= insideEnd) {
            edges |= EDGE_RIGHT;
            coverage = RunCoverage(&span->rl, 1, x - insideEnd);
        } else if (poly->wire && edges == 0) {
            continue;
        }
        DrawPixel(f, poly, y * WIDTH + x, depth, values, edges, coverage);
    }
}

// Scans one polygon (GBATEK "DS 3D Polygon Definitions by Vertices" for what the vertices are; the walking is
// melonDS's, see Edge). The two chains of edges go down from the top vertex, one each way round; which one is left
// comes from the polygon's turn on the screen. Lines from the top vertex's Y to the bottom one's, the bottom one
// excluded; a polygon flat on one line covers that line (melonDS)
static void DrawPolygon(Frame *f, const PortGePolygon *p) {
    const PortGeList *list = f->src->list;
    Vertex v[PORT_GE_POLYGON_MAX_VERTICES];
    int n = p->vertex_count;
    uint32_t maxW = 0;
    int top = 0;
    int32_t yTop = 0x7fffffff;
    int32_t yBottom = -1;
    Poly poly;

    if (n < 1 || n > PORT_GE_POLYGON_MAX_VERTICES) {
        return;
    }
    for (int i = 0; i < n; i++) {
        if (p->vertices[i] >= list->vertex_count) {
            return;
        }
        const PortGeVertex *gv = &list->vertices[p->vertices[i]];
        uint32_t w = gv->w > 0 ? (uint32_t)gv->w : 0;
        v[i].x = gv->x;
        v[i].y = gv->y;
        v[i].w = (int32_t)w;
        v[i].depth = gv->z;
        for (int c = 0; c < 3; c++) {
            v[i].a[c] = gv->color[c];
        }
        v[i].a[3] = gv->s;
        v[i].a[4] = gv->t;
        maxW = w > maxW ? w : maxW;
        if (v[i].y < yTop) {
            yTop = v[i].y;
            top = i;
        }
        yBottom = v[i].y > yBottom ? v[i].y : yBottom;
    }
    // The polygon's Ws are scaled together by a multiple of 4 bits so that the largest has 16 (melonDS). The W-buffer
    // depth is the W the scaled one stands for: the bits the scaling dropped are lost (melonDS)
    int wBits = 0;
    while (wBits < 32 && (maxW >> wBits) != 0) {
        wBits += 4;
    }
    for (int i = 0; i < n; i++) {
        uint32_t w = (uint32_t)v[i].w;
        uint32_t scaled = wBits < 16 ? w << (16 - wBits) : w >> (wBits - 16);
        v[i].w = (int32_t)scaled;
        if (f->wBuffer) {
            uint32_t kept = wBits < 16 ? w : scaled << (wBits - 16);
            v[i].depth = (int32_t)(kept > 0xffffff ? 0xffffff : kept);
        }
    }

    poly.p = p;
    poly.attr = p->attr;
    poly.id = ATTR_ID(p->attr);
    poly.mode = ATTR_MODE(p->attr);
    poly.alpha = ATTR_ALPHA(p->attr);
    poly.wire = poly.alpha == 0;
    if (poly.wire) {
        poly.alpha = 31; // a wireframe's edges are opaque
    }
    poly.textured = (f->cnt & CNT_TEXTURE) && TEX_FORMAT(p->tex_param) != FORMAT_NONE;
    poly.front = (p->flags & PORT_GE_POLYGON_FRONT) != 0;
    // The stencil is cleared when a shadow mask follows anything but a shadow mask (melonDS)
    int mask = poly.mode == MODE_SHADOW && poly.id == 0;
    if (mask && !f->lastWasShadowMask) {
        memset(sStencil, 0, sizeof(sStencil));
    }
    f->lastWasShadowMask = mask;
    // Every edge's run is drawn with anti-aliasing or edge marking on, for a wireframe polygon, and for a translucent
    // one while blending is on (melonDS)
    int fill = poly.wire || f->fillEdges || (ATTR_ALPHA(p->attr) < 31 && (f->cnt & CNT_BLEND));

    int64_t area = 0;
    for (int i = 0; i < n; i++) {
        const Vertex *a = &v[i];
        const Vertex *b = &v[(i + 1) % n];
        area += (int64_t)a->x * b->y - (int64_t)b->x * a->y;
    }
    // Clockwise on the screen (positive area with Y down): going forward from the top runs down the right side
    int stepLeft = area > 0 ? n - 1 : 1;
    int stepRight = n - stepLeft;
    int yEnd = yBottom > yTop ? yBottom : yTop + 1;
    int flat = yBottom == yTop;
    int l = top;
    int r = top;
    int lNext;
    int rNext;
    Edge el;
    Edge er;

    if (flat) {
        // One line: from the leftmost vertex to the rightmost, as two vertical edges
        for (int i = 0; i < n; i++) {
            l = v[i].x < v[l].x ? i : l;
            r = v[i].x > v[r].x ? i : r;
        }
        EdgeSetup(&el, &v[l], &v[l], 0);
        EdgeSetup(&er, &v[r], &v[r], 1);
        lNext = l;
        rNext = r;
    } else {
        for (int k = 0; k < n && v[(l + stepLeft) % n].y == yTop; k++) {
            l = (l + stepLeft) % n;
        }
        for (int k = 0; k < n && v[(r + stepRight) % n].y == yTop; k++) {
            r = (r + stepRight) % n;
        }
        lNext = (l + stepLeft) % n;
        rNext = (r + stepRight) % n;
        EdgeSetup(&el, &v[l], &v[lNext], 0);
        EdgeSetup(&er, &v[r], &v[rNext], 1);
    }

    for (int32_t y = yTop; y < yEnd && y < HEIGHT; y++) {
        if (!flat) {
            int moved = 0;
            for (int k = 0; k < n && v[lNext].y <= y; k++) {
                l = lNext;
                lNext = (l + stepLeft) % n;
                moved = 1;
            }
            if (moved) {
                EdgeSetup(&el, &v[l], &v[lNext], 0);
            }
            moved = 0;
            for (int k = 0; k < n && v[rNext].y <= y; k++) {
                r = rNext;
                rNext = (r + stepRight) % n;
                moved = 1;
            }
            if (moved) {
                EdgeSetup(&er, &v[r], &v[rNext], 1);
            }
        }
        int64_t posL = EdgePos(&el, y, 0);
        int64_t posR = EdgePos(&er, y, 1);
        int32_t xl = EdgeX(&el, posL);
        int32_t xr = EdgeX(&er, posR);
        // A vertical right edge is the column left of its X, unless the left edge is vertical too on the same X, or
        // it is at X 0 (melonDS)
        if (er.slope == 0 && (el.slope != 0 || xl != xr) && xr != 0) {
            xr--;
        }
        // On the bottom line, an X-major edge's run is drawn when the polygon has a flat bottom there (melonDS)
        int bottomRun = y == yBottom - 1 && el.v1->x != er.v1->x;
        Span span;
        if (xl <= xr) {
            span.left = xl;
            span.right = xr;
            span.el = &el;
            span.er = &er;
            EdgeLineSetup(&el, posL, 0, 0, &span.ll);
            EdgeLineSetup(&er, posR, 1, 0, &span.rl);
            // The fill rules (melonDS's measurements): a left edge's run is drawn unless it is X-major and runs to the
            // right, or when both edges have the same slope and the left run reaches the right end; a right edge's
            // only when it is X-major and runs to the right, or is vertical
            span.fillLeft = fill || el.negative || !el.xMajor || (bottomRun && el.xMajor) ||
                            (el.slope == er.slope && xl + span.ll.run == xr + 1);
            span.fillRight = fill || (!er.negative && er.xMajor) || er.slope == 0 || (bottomRun && er.xMajor);
        } else {
            // The edges have crossed (a thin or degenerate polygon): the span runs the other way, from the right
            // edge's pixel to the left one's, each edge drawn on the other side
            span.left = xr;
            span.right = xl;
            span.el = &er;
            span.er = &el;
            EdgeLineSetup(&er, posR, 1, 1, &span.ll);
            EdgeLineSetup(&el, posL, 0, 1, &span.rl);
            span.fillLeft = fill || er.negative || !er.xMajor || (bottomRun && er.xMajor);
            span.fillRight = fill || (!el.negative && el.xMajor) || (!(el.negative && el.xMajor) && er.slope == 0) ||
                             (bottomRun && el.xMajor);
        }
        uint32_t lineEdges = (y == yTop ? EDGE_TOP : 0) | (y == yEnd - 1 ? EDGE_BOTTOM : 0);
        DrawSpan(f, &poly, y, &span, lineEdges);
    }
}

// --- The frame

// A polygon draws with the translucent ones when its alpha is 1..30 or its texture format has alpha (A3I5, A5I3)
static int IsTranslucent(const PortGePolygon *p) {
    uint32_t alpha = ATTR_ALPHA(p->attr);
    uint32_t format = TEX_FORMAT(p->tex_param);
    return (alpha > 0 && alpha < 31) || format == FORMAT_A3I5 || format == FORMAT_A5I3;
}

static int CompareKeys(const void *a, const void *b) {
    uint32_t ka = *(const uint32_t *)a;
    uint32_t kb = *(const uint32_t *)b;
    return ka < kb ? -1 : ka > kb;
}

// The drawing order (GBATEK "SWAP_BUFFERS"): opaque polygons first, sorted by Y, then translucent ones, sorted by Y
// unless SWAP_BUFFERS bit 0 asked for their own order. Sorting by Y is by the bottom line, then the top one, stable
// (melonDS's measurement)
static size_t SortPolygons(const PortGeList *list, uint32_t *keys) {
    uint32_t count = list->polygon_count > PORT_GE_MAX_POLYGONS ? PORT_GE_MAX_POLYGONS : list->polygon_count;
    int manual = (list->swap_param & 1) != 0;
    for (uint32_t i = 0; i < count; i++) {
        const PortGePolygon *p = &list->polygons[i];
        uint32_t yTop = 255;
        uint32_t yBottom = 0;
        for (int k = 0; k < p->vertex_count && k < PORT_GE_POLYGON_MAX_VERTICES; k++) {
            uint32_t index = p->vertices[k];
            uint32_t y = index < list->vertex_count ? list->vertices[index].y : 0;
            yTop = y < yTop ? y : yTop;
            yBottom = y > yBottom ? y : yBottom;
        }
        int translucent = IsTranslucent(p);
        uint32_t key = translucent && manual ? 0 : (yBottom << 8 | yTop);
        // The class, then the key, then the index, which keeps the sort stable
        keys[i] = (uint32_t)translucent << 31 | key << 11 | i;
    }
    qsort(keys, count, sizeof(*keys), CompareKeys);
    return count;
}

// The clear depth's 15 bits as 24: d * 0x200 + ((d + 1) / 0x8000) * 0x1ff, so 0x7fff is the farthest, 0xffffff
static uint32_t ClearDepth(uint32_t d) {
    d &= 0x7fff;
    return d * 0x200 + ((d + 1) / 0x8000) * 0x1ff;
}

// The layer beneath starts as the rear plane too; only an edge pixel's is ever read, and drawing it set it
static void ClearBelow(void) {
    memcpy(sBelowColor, sColor, sizeof(sBelowColor));
    memcpy(sBelowDepth, sDepth, sizeof(sBelowDepth));
    memcpy(sBelowAttr, sAttr, sizeof(sBelowAttr));
}

// The rear plane (GBATEK "DS 3D Rear-Plane"): the clear colour, alpha, depth, polygon ID and fog flag everywhere, or
// with DISP3DCNT bit 14 the clear image: colours (bit 15 the alpha) from texture slot 2 and depths (bit 15 the fog
// flag) from slot 3, 256x256 each, scrolled by CLRIMAGE_OFFSET
static void Clear(const Frame *f) {
    const PortDs3dRegs *regs = &f->src->regs;
    uint32_t color = regs->clear_color;
    uint32_t id = (color >> 24) & 63;
    memset(sStencil, 0, sizeof(sStencil));
    memset(sCoverage, FULL_COVERAGE, sizeof(sCoverage));
    if (!(f->cnt & CNT_CLEAR_IMAGE)) {
        uint32_t pixel = Rgb15(color, (color >> 16) & 31);
        uint32_t depth = ClearDepth(regs->clear_depth);
        uint32_t attr = id | ((color & 0x8000) ? PA_FOG : 0);
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            sColor[i] = pixel;
            sDepth[i] = depth;
            sAttr[i] = attr;
        }
        ClearBelow();
        return;
    }
    uint32_t ox = regs->clear_offset & 0xff;
    uint32_t oy = regs->clear_offset >> 8;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            uint32_t offset = ((((uint32_t)y + oy) & 255) * 256 + (((uint32_t)x + ox) & 255)) * 2;
            uint32_t c = TexRead16(f, 0x40000 + offset);
            uint32_t d = TexRead16(f, 0x60000 + offset);
            int i = y * WIDTH + x;
            sColor[i] = Rgb15(c, (c & 0x8000) ? 31 : 0);
            sDepth[i] = ClearDepth(d);
            sAttr[i] = id | ((d & 0x8000) ? PA_FOG : 0);
        }
    }
    ClearBelow();
}

// The neighbour's polygon ID and depth for edge marking; off the picture they are the rear plane's (the port's
// choice: GBATEK doesn't say, and the clear colour register's are the natural ones)
static void Neighbour(const Frame *f, int x, int y, uint32_t *id, uint32_t *depth) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        *id = (f->src->regs.clear_color >> 24) & 63;
        *depth = ClearDepth(f->src->regs.clear_depth);
        return;
    }
    *id = PA_ID(sAttr[y * WIDTH + x]);
    *depth = sDepth[y * WIDTH + x];
}

static int DiffersFromNeighbour(const Frame *f, int x, int y, int checkDepth) {
    static const int kOffsets[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
    int i = y * WIDTH + x;
    uint32_t id = PA_ID(sAttr[i]);
    for (int k = 0; k < 4; k++) {
        uint32_t nid;
        uint32_t ndepth;
        Neighbour(f, x + kOffsets[k][0], y + kOffsets[k][1], &nid, &ndepth);
        if (nid != id && (!checkDepth || sDepth[i] < ndepth)) {
            return 1;
        }
    }
    return 0;
}

// Edge marking (GBATEK "DS 3D Toon, Edge, Fog, Alpha-Blending, Anti-Aliasing"): an edge pixel of an opaque polygon
// takes EDGE_COLOR[ID / 8] where a neighbour above, below, left or right has another polygon ID and is farther. The
// alpha stays, and the pixel's anti-aliasing coverage becomes EDGE_MARK_COVERAGE (melonDS)
static void EdgeMarking(const Frame *f) {
    static uint32_t marked[HEIGHT * WIDTH];
    uint32_t colors[8];
    int count = 0;
    for (int k = 0; k < 8; k++) {
        colors[k] = Rgb15(f->src->regs.edge_color[k], 0);
    }
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int i = y * WIDTH + x;
            if ((sAttr[i] & PA_EDGES) && DiffersFromNeighbour(f, x, y, 1)) {
                marked[count++] = (uint32_t)i;
            }
        }
    }
    for (int k = 0; k < count; k++) {
        uint32_t i = marked[k];
        sColor[i] = colors[PA_ID(sAttr[i]) >> 3] | (sColor[i] & PIXEL(0, 0, 0, 31));
        sCoverage[i] = EDGE_MARK_COVERAGE;
    }
}

// Fog (GBATEK, the same chapter): the density of a pixel with the fog flag comes from its depth. FOG_TABLE's entry n
// holds at depth FOG_OFFSET + (n + 1) * (0x400 >> FOG_SHIFT) (15-bit depths; here in 24 bits), the density between two
// entries is interpolated, entry 0 holds below and entry 31 above, and a density of 127 is taken as 128, all fog
// (melonDS's measurement). The colour and alpha, or with DISP3DCNT bit 6 the alpha only, become
// (fog * density + pixel * (128 - density)) / 128. Under an edge pixel with anti-aliasing on, the pixel beneath it is
// fogged too, by its own depth and flag (melonDS)
typedef struct FogParams {
    const PortDs3dRegs *regs;
    uint32_t shift;
    uint32_t step;
    uint32_t start;
    uint32_t color;
    int alphaOnly;
} FogParams;

// A pixel's colour c with the fog its depth gives, by the rules above
static uint32_t Fogged(const FogParams *p, uint32_t c, uint32_t depth) {
    const PortDs3dRegs *regs = p->regs;
    uint32_t density;
    if (depth < p->start) {
        density = regs->fog_table[0] & 127;
    } else {
        uint32_t entry = (depth - p->start) >> (19 - p->shift);
        uint32_t frac = (depth - p->start) & (p->step - 1);
        if (entry >= 31) {
            density = regs->fog_table[31] & 127;
        } else {
            density = ((regs->fog_table[entry] & 127) * (p->step - frac) + (regs->fog_table[entry + 1] & 127) * frac) >>
                      (19 - p->shift);
        }
    }
    if (density >= 127) {
        density = 128;
    }
    uint32_t fog = p->color;
    uint32_t alpha = (PIXEL_A(fog) * density + PIXEL_A(c) * (128 - density)) >> 7;
    if (p->alphaOnly) {
        return (c & ~PIXEL(0, 0, 0, 31)) | PIXEL(0, 0, 0, alpha);
    }
    uint32_t r = (PIXEL_R(fog) * density + PIXEL_R(c) * (128 - density)) >> 7;
    uint32_t g = (PIXEL_G(fog) * density + PIXEL_G(c) * (128 - density)) >> 7;
    uint32_t b = (PIXEL_B(fog) * density + PIXEL_B(c) * (128 - density)) >> 7;
    return PIXEL(r, g, b, alpha);
}

// Fogs the whole frame after it is drawn: every pixel with the fog flag, and with anti-aliasing on, the pixel beneath
// an edge pixel too
static void Fog(const Frame *f) {
    FogParams p;
    p.regs = &f->src->regs;
    p.shift = CNT_FOG_SHIFT(f->cnt);
    p.step = 0x80000u >> p.shift;
    p.start = ((uint32_t)(p.regs->fog_offset & 0x7fff) << 9) + p.step;
    p.color = Rgb15(p.regs->fog_color, (p.regs->fog_color >> 16) & 31);
    p.alphaOnly = (f->cnt & CNT_FOG_ALPHA_ONLY) != 0;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (sAttr[i] & PA_FOG) {
            sColor[i] = Fogged(&p, sColor[i], sDepth[i]);
        }
        if (f->antialias && (sAttr[i] & PA_EDGES) && (sBelowAttr[i] & PA_FOG)) {
            sBelowColor[i] = Fogged(&p, sBelowColor[i], sBelowDepth[i]);
        }
    }
}

// Anti-aliasing (GBATEK, the same chapter: opaque polygons' edges are blended with what is behind them by how much of
// the pixel they cover; the arithmetic is melonDS's measurement). Each edge pixel on top with a coverage c below 31
// shows (top * (c + 1) + beneath * (31 - c)) / 32 for red, green and blue, unless the pixel beneath has alpha 0, and
// the same for alpha always; coverage 0 shows the pixel beneath alone. An edge pixel's coverage comes from the walker
// (EdgeLineSetup), so polygons sharing an edge draw every pixel of it between them: whichever is on top shows the
// other beneath it
static void Antialias(void) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        uint32_t cov = sCoverage[i];
        if (cov >= FULL_COVERAGE || !(sAttr[i] & PA_EDGES)) {
            continue;
        }
        uint32_t below = sBelowColor[i];
        if (cov == 0) {
            sColor[i] = below;
            continue;
        }
        uint32_t c = sColor[i];
        uint32_t top = cov + 1;
        uint32_t rest = 32 - top;
        uint32_t r = PIXEL_R(c);
        uint32_t g = PIXEL_G(c);
        uint32_t b = PIXEL_B(c);
        if (PIXEL_A(below) != 0) {
            r = (r * top + PIXEL_R(below) * rest) >> 5;
            g = (g * top + PIXEL_G(below) * rest) >> 5;
            b = (b * top + PIXEL_B(below) * rest) >> 5;
        }
        uint32_t alpha = (PIXEL_A(c) * top + PIXEL_A(below) * rest) >> 5;
        sColor[i] = PIXEL(r, g, b, alpha);
    }
}

static uint32_t Widen8(uint32_t c6) {
    return (c6 << 2) | (c6 >> 4);
}

void port_ds3d_regs_from_io(PortDs3dRegs *regs, const uint8_t *io) {
    regs->disp3dcnt = (uint16_t)Read16(io + 0x60);
    for (int i = 0; i < 8; i++) {
        regs->edge_color[i] = (uint16_t)Read16(io + 0x330 + i * 2);
    }
    regs->alpha_test_ref = io[0x340] & 31;
    regs->clear_color = Read16(io + 0x350) | Read16(io + 0x352) << 16;
    regs->clear_depth = (uint16_t)Read16(io + 0x354);
    regs->clear_offset = (uint16_t)Read16(io + 0x356);
    regs->fog_color = Read16(io + 0x358) | Read16(io + 0x35a) << 16;
    regs->fog_offset = (uint16_t)Read16(io + 0x35c);
    for (int i = 0; i < 32; i++) {
        regs->fog_table[i] = io[0x360 + i];
        regs->toon_table[i] = (uint16_t)Read16(io + 0x380 + i * 2);
    }
}

void port_ds3d_render(const PortDs3dSource *src, uint32_t *out) {
    static uint32_t keys[PORT_GE_MAX_POLYGONS];
    Frame f;

    memset(&f, 0, sizeof(f));
    f.src = src;
    f.cnt = src->regs.disp3dcnt;
    f.wBuffer = src->list != NULL && (src->list->swap_param & 2) != 0;
    f.antialias = (f.cnt & CNT_ANTIALIAS) != 0;
    f.fillEdges = (f.cnt & (CNT_ANTIALIAS | CNT_EDGE_MARK)) != 0;
    for (int i = 0; i < 32; i++) {
        f.toon[i] = Rgb15(src->regs.toon_table[i], 0);
    }
    Clear(&f);
    if (src->list != NULL) {
        size_t count = SortPolygons(src->list, keys);
        for (size_t i = 0; i < count; i++) {
            DrawPolygon(&f, &src->list->polygons[keys[i] & 0x7ff]);
        }
    }
    if (f.cnt & CNT_EDGE_MARK) {
        EdgeMarking(&f);
    }
    if (f.cnt & CNT_FOG) {
        Fog(&f);
    }
    if (f.cnt & CNT_ANTIALIAS) {
        Antialias();
    }
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        uint32_t c = sColor[i];
        uint32_t a = PIXEL_A(c);
        out[i] = a == 0 ? 0
                        : ((a << 3) | (a >> 2)) << 24 | Widen8(PIXEL_R(c)) << 16 | Widen8(PIXEL_G(c)) << 8 |
                              Widen8(PIXEL_B(c));
    }
}

uint32_t port_ds3d_depth(int x, int y) {
    return sDepth[y * WIDTH + x];
}

int port_ds3d_polygon_id(int x, int y) {
    return (int)PA_ID(sAttr[y * WIDTH + x]);
}

int port_ds3d_fog_flag(int x, int y) {
    return (sAttr[y * WIDTH + x] & PA_FOG) != 0;
}
