// The CPU reference renderer of the DS's 2D engines (ds2d.h). Each line is drawn in the hardware's order: the OBJ
// line, the window mask, each BG line, then the layers stacked back to front so every dot knows its top two visible
// layers, the colour effect between them, the display mode and master brightness.
//
// Facts are GBATEK's unless a comment says they are melonDS's measured behaviour (where GBATEK is silent or unsure).
#include "gfx/ref/ds2d.h"
#include <string.h>

// I/O registers, as offsets from the engine's base
#define REG_DISPCNT 0x00
#define REG_BGCNT(n) (0x08 + 2 * (n))
#define REG_BGHOFS(n) (0x10 + 4 * (n))
#define REG_BGVOFS(n) (0x12 + 4 * (n))
#define REG_BGPA(i) (0x20 + 0x10 * (i)) // i = 0 for BG2, 1 for BG3; PB, PC, PD follow
#define REG_BGX(i) (0x28 + 0x10 * (i))
#define REG_BGY(i) (0x2C + 0x10 * (i))
#define REG_WINH(w) (0x40 + 2 * (w))
#define REG_WINV(w) (0x44 + 2 * (w))
#define REG_WININ 0x48
#define REG_WINOUT 0x4A
#define REG_MOSAIC 0x4C
#define REG_BLDCNT 0x50
#define REG_BLDALPHA 0x52
#define REG_BLDY 0x54
#define REG_DISPCAPCNT 0x64
#define REG_MASTER_BRIGHT 0x6C

#define DISPCNT_BG0_3D (1u << 3)
#define DISPCNT_OBJ_1D (1u << 4)
#define DISPCNT_BMP_OBJ_256 (1u << 5)
#define DISPCNT_BMP_OBJ_1D (1u << 6)
#define DISPCNT_FORCED_BLANK (1u << 7)
#define DISPCNT_BG(n) (1u << (8 + (n)))
#define DISPCNT_OBJ (1u << 12)
#define DISPCNT_WIN(w) (1u << (13 + (w)))
#define DISPCNT_OBJWIN (1u << 15)
#define DISPCNT_BMP_OBJ_1D_256 (1u << 22)
#define DISPCNT_HBLANK_FREE (1u << 23)
#define DISPCNT_BG_EXT_PALETTE (1u << 30)
#define DISPCNT_OBJ_EXT_PALETTE (1u << 31)
// Engine B has no 3D BG0, no display modes 2 and 3, no VRAM display block, no 256-byte bitmap OBJ boundary and no
// character and screen bases in DISPCNT. melonDS keeps exactly these bits clear on engine B
#define DISPCNT_ENGINE_B_BITS 0xC0B1FFF7u

#define BGCNT_MOSAIC (1u << 6)
#define BGCNT_256_COLORS (1u << 7)
#define BGCNT_BIT13 (1u << 13) // BG0/BG1: extended palette slot 2/3; BG2/BG3: wrap around instead of transparent

#define DISPCAPCNT_ENABLE (1u << 31)

// A layer's dot: the colour with 6 bits per channel, red in bits 16-21, green in 8-13 and blue in 0-5, and in the
// bits around them its flags, its alpha and, once stacked, its layer
#define PX_COLOR 0x003F3F3Fu
#define PX_3D (1u << 6)
#define PX_BITMAP (1u << 7)
#define PX_SEMI (1u << 14)
#define PX_ALPHA_SHIFT 22 // bits 22-26: a bitmap OBJ's EVA (2..16), or the 3D layer's alpha (0..31)
#define PX_LAYER_SHIFT 28 // bits 28-30: the layer number (0-3 BG, 4 OBJ, 5 backdrop)
#define PX_OPAQUE (1u << 31)
#define PX_ALPHA(px) (((px) >> PX_ALPHA_SHIFT) & 0x1F)
#define PX_LAYER(px) (((px) >> PX_LAYER_SHIFT) & 7)

// The OBJ line's attributes per dot: the BG-relative priority and whether a mosaic OBJ wrote it
#define OBJ_PRIORITY 3u
#define OBJ_MOSAIC 4u

// Layer numbers in BLDCNT and the window controls
#define LAYER_OBJ 4
#define LAYER_BACKDROP 5
#define WINCTL_EFFECTS 0x20u

#define WHITE6 0x003F3F3Fu

enum BgKind { BG_NONE, BG_TEXT, BG_3D, BG_AFFINE, BG_EXTENDED, BG_LARGE };

// Where one window is inside on the current line: up to two ranges of dots, [start, end)
typedef struct WindowSpan {
    int count;
    unsigned start[2];
    unsigned end[2];
} WindowSpan;

// The engine state the hardware keeps from one line to the next
typedef struct LineState {
    int nextLine;           // the line whose state comes next; 263 once a frame has no lines left
    int32_t ref[2][2];      // BG2 and BG3's internal reference points (X, Y), 20.8 fixed point in 28 bits
    int32_t drawRef[2][2];  // the points the current line is drawn from, held through a vertical mosaic block
    uint32_t seenRef[2][2]; // the register values last seen, to notice writes
    unsigned bgMosaicY;     // the BG vertical mosaic counter
    unsigned bgMosaicMax;   // its size, taken from MOSAIC when a block starts
    unsigned bgMosaicLine;  // the line the current BG block started on
    int bgMosaicLatch;      // a BG block starts on this line
    unsigned objMosaicY;    // the OBJ vertical mosaic counter
    unsigned objMosaicLine; // the line the current OBJ block started on
    uint8_t winActive[2];   // per window: bit 0 inside vertically, bit 1 inside horizontally
    WindowSpan winSpan[2];  // the line's horizontal ranges with both bits set
} LineState;

// One engine: its state between lines and its line buffers
typedef struct Engine {
    int framePending;    // the next line starts a frame
    LineState state;     // as the next line finds it
    LineState lineStart; // as the last line rendered found it, so that the line can be drawn again
    uint8_t reload[2];   // writes the platform layer reported (bit 0 X, bit 1 Y)
    int paletteValid;    // palette18 was converted from paletteRaw
    uint8_t paletteRaw[1024];
    uint32_t palette18[512]; // the BG then OBJ palette as 18-bit opaque dots, converted when the palette changes
    uint32_t bg[4][256];
    uint32_t obj[256];
    uint8_t objAttr[256];
    uint8_t objWindow[256];
    uint8_t winMask[256];
    unsigned objPriorities; // the BG-relative priorities the OBJ line has dots of, a bit each
    uint32_t top[256];      // the front visible layer's dot
    uint32_t below[256];    // the one behind it
    uint32_t composite[256];
    int compositeValid;
} Engine;

// The registers one line uses, decoded
typedef struct Regs {
    uint32_t dispcnt;
    uint16_t bgcnt[4];
    uint16_t hofs[4];
    uint16_t vofs[4];
    int32_t pa[2], pb[2], pc[2], pd[2];
    uint32_t refX[2], refY[2];
    uint16_t winH[2], winV[2];
    uint16_t winIn, winOut;
    uint16_t mosaic;
    uint16_t bldcnt, bldalpha, bldy;
    uint32_t dispcapcnt;
    uint16_t masterBright;
} Regs;

// What drawing one line needs
typedef struct Line {
    const PortDs2dSource *src;
    const Regs *regs;
    const uint32_t *palette; // the engine's converted palette: BG colours 0-255, OBJ colours 256-511
    int engine;
    unsigned line;
    uint32_t bgMask;     // the BG view's size minus 1
    uint32_t objMask;    // the OBJ view's size minus 1
    uint32_t charBase;   // engine A's DISPCNT character base
    uint32_t screenBase; // engine A's DISPCNT screen base
} Line;

// Both engines start as port_ds2d_reset() leaves them
static Engine sEngines[2] = { { .framePending = 1, .state.nextLine = 263 },
                              { .framePending = 1, .state.nextLine = 263 } };
static int sObjBudget;
static int sObjBudgetHBlankFree;

static const uint8_t kObjWidth[4][4] = {
    { 8, 16, 32, 64 },  // square
    { 16, 32, 32, 64 }, // horizontal
    { 8, 8, 16, 32 },   // vertical
    { 8, 8, 8, 8 },     // prohibited: drawn as 8x8, as melonDS does
};
static const uint8_t kObjHeight[4][4] = {
    { 8, 16, 32, 64 },
    { 8, 8, 16, 32 },
    { 16, 32, 32, 64 },
    { 8, 8, 8, 8 },
};

// Sizes of the extended affine bitmaps (BGxCNT bits 14-15)
static const uint16_t kBitmapWidth[4] = { 128, 256, 512, 512 };
static const uint16_t kBitmapHeight[4] = { 128, 256, 256, 512 };

static uint16_t Read16(const uint8_t *p, uint32_t offset) {
    return (uint16_t)(p[offset] | p[offset + 1] << 8);
}

static uint32_t Read32(const uint8_t *p, uint32_t offset) {
    return Read16(p, offset) | (uint32_t)Read16(p, offset + 2) << 16;
}

// VRAM reads through a view of 16 KiB slots; the address is already inside the view. 16- and 32-bit reads are
// aligned, so they never cross a slot
static uint8_t Vram8(const uint8_t *const *slots, uint32_t address) {
    const uint8_t *slot = slots[address >> 14];
    return slot != NULL ? slot[address & 0x3FFF] : 0;
}

static uint16_t Vram16(const uint8_t *const *slots, uint32_t address) {
    const uint8_t *slot = slots[address >> 14];
    return slot != NULL ? Read16(slot, address & 0x3FFF) : 0;
}

static uint32_t Vram32(const uint8_t *const *slots, uint32_t address) {
    const uint8_t *slot = slots[address >> 14];
    return slot != NULL ? Read32(slot, address & 0x3FFF) : 0;
}

// A 15-bit colour as the 2D engine's 18-bit colour. GBATEK says the LCDs have 6 bits per channel but not how 15-bit
// colours widen; melonDS measured c * 2 for red and blue and c * 2 plus bit 15 for green
static uint32_t Color18(uint16_t color) {
    uint32_t r = (color & 0x1Fu) << 1;
    uint32_t g = ((color >> 5) & 0x1Fu) << 1 | (uint32_t)(color >> 15);
    uint32_t b = ((color >> 10) & 0x1Fu) << 1;
    return r << 16 | g << 8 | b;
}

// An 8 KiB extended palette slot; an unmapped slot reads 0, which is black
static uint32_t ExtPaletteColor(const uint8_t *slot, uint32_t index) {
    return Color18(slot != NULL ? Read16(slot, index * 2) : 0) | PX_OPAQUE;
}

// Converts the palette entries that changed since the last line
static void UpdatePalette(Engine *engine, const uint8_t *palette) {
    unsigned i;

    if (engine->paletteValid && memcmp(engine->paletteRaw, palette, sizeof(engine->paletteRaw)) == 0) {
        return;
    }
    for (i = 0; i < 512; i++) {
        uint16_t color = Read16(palette, i * 2);

        if (!engine->paletteValid || Read16(engine->paletteRaw, i * 2) != color) {
            engine->palette18[i] = Color18(color) | PX_OPAQUE;
        }
    }
    memcpy(engine->paletteRaw, palette, sizeof(engine->paletteRaw));
    engine->paletteValid = 1;
}

// A reference point register (28 bits, signed) as a number
static int32_t SignExtend28(uint32_t value) {
    return (int32_t)(value << 4) >> 4;
}

static unsigned Min16(unsigned value) {
    return value > 16 ? 16 : value;
}

// Colour effects, per 6-bit channel, all three at once: each channel gets a 16-bit lane of a 64-bit word, where the
// products (at most 63 * 32 + 16) stay inside their lane
#define LANES(v) ((uint64_t)(v) * 0x000100010001ull) // the same value in each lane
#define LANE_MASK(m) LANES(m)

static uint64_t Spread(uint32_t color) {
    return (color & 0x3Fu) | (uint64_t)(color & 0x3F00u) << 8 | (uint64_t)(color & 0x3F0000u) << 16;
}

static uint32_t Gather(uint64_t lanes) {
    return (uint32_t)(lanes & 0x3F) | (uint32_t)(lanes >> 8 & 0x3F00) | (uint32_t)(lanes >> 16 & 0x3F0000);
}

// GBATEK gives alpha blending as EVA * c1 + EVB * c2 saturated at the top, brightness up as c + (63 - c) * EVY and
// down as c - c * EVY, all in 16ths and without rounding, and for the 3D layer guesses EVA = A/2, EVB = 16 - A/2. The
// rounding and the 3D weights are melonDS's measurements: alpha (c1 * EVA + c2 * EVB + 8) / 16 saturated at 63,
// brightness up c + ((63 - c) * EVY + 8) / 16 and down c - (c * EVY + 7) / 16, the 3D blend
// (c1 * (A + 1) + c2 * (31 - A) + 16) / 32, and master brightness up c + (63 - c) * F / 16 and down
// c - (c * F + 15) / 16
static uint32_t BlendAlpha(uint32_t a, uint32_t b, unsigned eva, unsigned evb) {
    uint64_t sum = (Spread(a) * eva + Spread(b) * evb + LANES(8)) >> 4 & LANE_MASK(0x7F);

    // Lanes reach 126 at most: those at 64 or above become 63
    return Gather(sum | (sum >> 6 & LANE_MASK(1)) * 0x3F);
}

// The 3D layer over a 2nd target, with its own alpha; alpha 31 leaves the 3D colour as it is
static uint32_t Blend3d(uint32_t a, uint32_t b, unsigned alpha) {
    unsigned eva = alpha + 1;

    if (eva == 32) {
        return a & PX_COLOR;
    }
    return Gather((Spread(a) * eva + Spread(b) * (32 - eva) + LANES(16)) >> 5);
}

// Brightness up: c + ((63 - c) * EVY + bias) / 16
static uint32_t Brighten(uint32_t a, unsigned evy, unsigned bias) {
    uint64_t c = Spread(a);

    return Gather(c + (((LANE_MASK(0x3F) - c) * evy + LANES(bias)) >> 4 & LANE_MASK(0x3F)));
}

// Brightness down: c - (c * EVY + bias) / 16
static uint32_t Darken(uint32_t a, unsigned evy, unsigned bias) {
    uint64_t c = Spread(a);

    return Gather(c - ((c * evy + LANES(bias)) >> 4 & LANE_MASK(0x3F)));
}

static void ReadRegs(const uint8_t *io, int engine, Regs *regs) {
    int n;
    int i;

    regs->dispcnt = Read32(io, REG_DISPCNT);
    if (engine != 0) {
        regs->dispcnt &= DISPCNT_ENGINE_B_BITS;
    }
    for (n = 0; n < 4; n++) {
        regs->bgcnt[n] = Read16(io, REG_BGCNT(n));
        regs->hofs[n] = Read16(io, REG_BGHOFS(n)) & 0x1FF;
        regs->vofs[n] = Read16(io, REG_BGVOFS(n)) & 0x1FF;
    }
    for (i = 0; i < 2; i++) {
        regs->pa[i] = (int16_t)Read16(io, REG_BGPA(i));
        regs->pb[i] = (int16_t)Read16(io, REG_BGPA(i) + 2);
        regs->pc[i] = (int16_t)Read16(io, REG_BGPA(i) + 4);
        regs->pd[i] = (int16_t)Read16(io, REG_BGPA(i) + 6);
        regs->refX[i] = Read32(io, REG_BGX(i));
        regs->refY[i] = Read32(io, REG_BGY(i));
        regs->winH[i] = Read16(io, REG_WINH(i));
        regs->winV[i] = Read16(io, REG_WINV(i));
    }
    regs->winIn = Read16(io, REG_WININ);
    regs->winOut = Read16(io, REG_WINOUT);
    regs->mosaic = Read16(io, REG_MOSAIC);
    regs->bldcnt = Read16(io, REG_BLDCNT);
    regs->bldalpha = Read16(io, REG_BLDALPHA);
    regs->bldy = Read16(io, REG_BLDY);
    regs->dispcapcnt = engine == 0 ? Read32(io, REG_DISPCAPCNT) : 0;
    regs->masterBright = Read16(io, REG_MASTER_BRIGHT);
}

// The vertical half of the windows, at the start of every line, VBlank lines included. GBATEK ("DS Window Glitches"):
// the DS counts lines 0..262 but compares only their lower 8 bits with Y1 and Y2, so the state is a flag that line Y2
// clears and line Y1 sets, kept across frames; Y2 wins when both match (melonDS)
static void UpdateWindowsVertical(Engine *engine, const Regs *regs, unsigned line) {
    int w;

    for (w = 0; w < 2; w++) {
        unsigned y1 = regs->winV[w] >> 8;
        unsigned y2 = regs->winV[w] & 0xFF;

        if ((line & 0xFF) == y2) {
            engine->state.winActive[w] &= (uint8_t)~1u;
        } else if ((line & 0xFF) == y1) {
            engine->state.winActive[w] |= 1;
        }
    }
}

static void AddSpan(WindowSpan *span, unsigned start, unsigned end) {
    if (start < end) {
        span->start[span->count] = start;
        span->end[span->count] = end;
        span->count++;
    }
}

// The horizontal half, swept across the line while the window is on in DISPCNT: dot X2 clears the flag and dot X1
// sets it (X2 wins when both match), and the flag carries into the next line. So X1 is inclusive and X2 exclusive,
// X2 = 0 reaches the right edge, X1 = X2 shows nothing, and X1 > X2 wraps from X1 to the next line's X2 (GBATEK "DS
// Window Glitches"; melonDS). The sweep comes down to at most two ranges per line
static void SweepWindowsHorizontal(Engine *engine, const Regs *regs) {
    int w;

    for (w = 0; w < 2; w++) {
        unsigned x1 = regs->winH[w] >> 8;
        unsigned x2 = regs->winH[w] & 0xFF;
        int carried = (engine->state.winActive[w] & 2) != 0;
        WindowSpan horizontal = { 0, { 0, 0 }, { 0, 0 } };

        engine->state.winSpan[w].count = 0;
        if (!(regs->dispcnt & DISPCNT_WIN(w))) {
            continue;
        }
        if (x1 == x2) {
            AddSpan(&horizontal, 0, carried ? x2 : 0);
            engine->state.winActive[w] &= (uint8_t)~2u;
        } else if (x1 < x2) {
            AddSpan(&horizontal, carried ? 0 : x1, x2);
            engine->state.winActive[w] &= (uint8_t)~2u;
        } else {
            AddSpan(&horizontal, 0, carried ? x2 : 0);
            AddSpan(&horizontal, x1, 256);
            engine->state.winActive[w] |= 2;
        }
        if (engine->state.winActive[w] & 1) {
            engine->state.winSpan[w] = horizontal;
        }
    }
}

// The first line of a frame: finish the last frame's window comparisons through the VBlank lines, then latch the
// reference points and restart the mosaic counters
static void StartFrame(Engine *engine, const Regs *regs) {
    int i;

    for (; engine->state.nextLine < 263; engine->state.nextLine++) {
        UpdateWindowsVertical(engine, regs, (unsigned)engine->state.nextLine);
    }
    for (i = 0; i < 2; i++) {
        engine->state.ref[i][0] = SignExtend28(regs->refX[i]);
        engine->state.ref[i][1] = SignExtend28(regs->refY[i]);
        engine->state.seenRef[i][0] = regs->refX[i];
        engine->state.seenRef[i][1] = regs->refY[i];
        engine->reload[i] = 0;
    }
    engine->state.bgMosaicY = 0;
    engine->state.bgMosaicMax = (regs->mosaic >> 4) & 0xF;
    engine->state.bgMosaicLatch = 1;
    engine->state.objMosaicY = 0;
    engine->state.objMosaicLine = 0;
    engine->state.nextLine = 0;
    engine->framePending = 0;
}

// The state a line starts with
static void BeginLine(Engine *engine, const Regs *regs, unsigned line) {
    int i;

    UpdateWindowsVertical(engine, regs, line);
    SweepWindowsHorizontal(engine, regs);

    // A write to a reference point register outside VBlank goes straight to the internal register (GBATEK)
    for (i = 0; i < 2; i++) {
        if (regs->refX[i] != engine->state.seenRef[i][0] || (engine->reload[i] & 1)) {
            engine->state.ref[i][0] = SignExtend28(regs->refX[i]);
            engine->state.seenRef[i][0] = regs->refX[i];
        }
        if (regs->refY[i] != engine->state.seenRef[i][1] || (engine->reload[i] & 2)) {
            engine->state.ref[i][1] = SignExtend28(regs->refY[i]);
            engine->state.seenRef[i][1] = regs->refY[i];
        }
        engine->reload[i] = 0;
    }

    // Vertical mosaic: a block keeps the line it started on for text BGs and the reference point it started with for
    // affine ones (melonDS)
    if (engine->state.bgMosaicLatch) {
        engine->state.bgMosaicLine = line;
    }
    for (i = 0; i < 2; i++) {
        if (!(regs->bgcnt[2 + i] & BGCNT_MOSAIC) || engine->state.bgMosaicLatch) {
            engine->state.drawRef[i][0] = engine->state.ref[i][0];
            engine->state.drawRef[i][1] = engine->state.ref[i][1];
        }
    }

    // The OBJ counter compares with MOSAIC directly and runs only while OBJs are on (melonDS)
    if (line > 0 && (regs->dispcnt & DISPCNT_OBJ)) {
        if (engine->state.objMosaicY == ((regs->mosaic >> 12) & 0xFu)) {
            engine->state.objMosaicY = 0;
            engine->state.objMosaicLine = line;
        } else {
            engine->state.objMosaicY = (engine->state.objMosaicY + 1) & 0xF;
        }
    }
}

// The state a line leaves for the next
static void EndLine(Engine *engine, const Regs *regs, unsigned line) {
    int i;

    // The internal reference points advance by PB and PD after each line (GBATEK), while the BG is on (melonDS)
    for (i = 0; i < 2; i++) {
        if (regs->dispcnt & DISPCNT_BG(2 + i)) {
            engine->state.ref[i][0] = SignExtend28((uint32_t)engine->state.ref[i][0] + (uint32_t)regs->pb[i]);
            engine->state.ref[i][1] = SignExtend28((uint32_t)engine->state.ref[i][1] + (uint32_t)regs->pd[i]);
        }
    }

    // The BG counter takes its size from MOSAIC only when a block starts (melonDS)
    if (engine->state.bgMosaicY == engine->state.bgMosaicMax) {
        engine->state.bgMosaicMax = (regs->mosaic >> 4) & 0xF;
        engine->state.bgMosaicY = 0;
        engine->state.bgMosaicLatch = 1;
    } else {
        engine->state.bgMosaicY = (engine->state.bgMosaicY + 1) & 0xF;
        engine->state.bgMosaicLatch = 0;
    }
    engine->state.nextLine = (int)line + 1;
}

// What BG n is in this BG mode (GBATEK's table). Engine B's BG0 is always text; modes 6 (engine A only) and 7 have
// no BG1 to BG3 otherwise
static enum BgKind BgKindOf(const Line *ln, int n) {
    unsigned mode = ln->regs->dispcnt & 7;

    if (n == 0) {
        if (ln->regs->dispcnt & DISPCNT_BG0_3D) {
            return BG_3D;
        }
        return mode == 6 && ln->engine == 0 ? BG_NONE : BG_TEXT;
    }
    if (n == 1) {
        return mode <= 5 ? BG_TEXT : BG_NONE;
    }
    if (n == 2) {
        switch (mode) {
        case 0:
        case 1:
        case 3:
            return BG_TEXT;
        case 2:
        case 4:
            return BG_AFFINE;
        case 5:
            return BG_EXTENDED;
        case 6:
            return ln->engine == 0 ? BG_LARGE : BG_NONE;
        default:
            return BG_NONE;
        }
    }
    switch (mode) {
    case 0:
        return BG_TEXT;
    case 1:
    case 2:
        return BG_AFFINE;
    case 3:
    case 4:
    case 5:
        return BG_EXTENDED;
    default:
        return BG_NONE;
    }
}

// The extended palette slot a 256-colour tile BG reads: BG0 and BG1 use slot 0 and 1, or 2 and 3 with BGxCNT bit 13
static const uint8_t *BgExtPalette(const Line *ln, int n) {
    int slot = n;

    if (n < 2 && (ln->regs->bgcnt[n] & BGCNT_BIT13)) {
        slot = n + 2;
    }
    return ln->src->bg_ext_palette[slot];
}

// A tile row's 8 dots in the other order: 4-bit dots in a word, 8-bit dots in a double word
static uint32_t ReverseNibbles(uint32_t x) {
    x = x >> 16 | x << 16;
    x = (x >> 8 & 0x00FF00FFu) | (x & 0x00FF00FFu) << 8;
    return (x >> 4 & 0x0F0F0F0Fu) | (x & 0x0F0F0F0Fu) << 4;
}

static uint64_t ReverseBytes(uint64_t x) {
    x = x >> 32 | x << 32;
    x = (x >> 16 & 0x0000FFFF0000FFFFull) | (x & 0x0000FFFF0000FFFFull) << 16;
    return (x >> 8 & 0x00FF00FF00FF00FFull) | (x & 0x00FF00FF00FF00FFull) << 8;
}

// A text BG: 32x32-tile screen blocks of 16-bit entries (tile, flips, palette), 16 or 256 colours. The 33 tiles
// the line touches are decoded whole, then the line is copied out from the fine scroll offset
static void DrawTextBg(const Engine *engine, const Line *ln, int n, uint32_t *restrict dst) {
    const Regs *regs = ln->regs;
    const uint8_t *const *vram = ln->src->bg_slots;
    const uint32_t *palette = ln->palette;
    uint32_t mask = ln->bgMask;
    unsigned cnt = regs->bgcnt[n];
    unsigned size = cnt >> 14;
    unsigned widthMask = (size & 1) ? 511 : 255;
    unsigned heightMask = (size & 2) ? 511 : 255;
    unsigned line = (cnt & BGCNT_MOSAIC) ? engine->state.bgMosaicLine : ln->line;
    unsigned y = (line + regs->vofs[n]) & heightMask;
    unsigned hofs = regs->hofs[n];
    uint32_t screen = ln->screenBase + ((cnt >> 8) & 0x1F) * 0x800u + ((y >> 3) & 31) * 64;
    uint32_t chr = ln->charBase + ((cnt >> 2) & 0xF) * 0x4000u;
    int colors256 = (cnt & BGCNT_256_COLORS) != 0;
    int extended = colors256 && (regs->dispcnt & DISPCNT_BG_EXT_PALETTE);
    const uint8_t *extPalette = BgExtPalette(ln, n);
    uint32_t tiles[33 * 8];
    unsigned t;
    unsigned dot;

    // A screen 512 dots tall has its lower blocks after the upper one (256x512) or two (512x512)
    if (y >= 256) {
        screen += size == 3 ? 0x1000 : 0x800;
    }
    for (t = 0; t < 33; t++) {
        unsigned x = (hofs + t * 8) & widthMask;
        unsigned entry = Vram16(vram, (screen + (x >= 256 ? 0x800 : 0) + ((x >> 3) & 31) * 2) & mask);
        unsigned tile = entry & 0x3FF;
        unsigned row = (entry & 0x800) ? 7 - (y & 7) : (y & 7);
        uint32_t *out = tiles + t * 8;

        if (colors256) {
            uint32_t address = (chr + tile * 64 + row * 8) & mask;
            uint64_t data = Vram32(vram, address) | (uint64_t)Vram32(vram, address + 4) << 32;
            unsigned paletteBase = (entry >> 12) * 256;

            data = (entry & 0x400) ? ReverseBytes(data) : data;
            for (dot = 0; dot < 8; dot++) {
                unsigned index = (unsigned)(data >> (dot * 8)) & 0xFF;
                uint32_t color = extended ? ExtPaletteColor(extPalette, paletteBase + index) : palette[index];

                out[dot] = index != 0 ? color : 0;
            }
        } else {
            uint32_t data = Vram32(vram, (chr + tile * 32 + row * 4) & mask);
            const uint32_t *colors = palette + (entry >> 12) * 16;

            data = (entry & 0x400) ? ReverseNibbles(data) : data;
            for (dot = 0; dot < 8; dot++) {
                unsigned index = (data >> (dot * 4)) & 0xF;
                uint32_t color = colors[index];

                out[dot] = index != 0 ? color : 0;
            }
        }
    }
    memcpy(dst, tiles + (hofs & 7), 256 * sizeof(*dst));
}

// The 3D layer: 256 dots followed by 256 transparent ones, scrolled by BG0HOFS only (GBATEK "DS 3D Final 2D Output")
static void Draw3dBg(const Line *ln, uint32_t *dst) {
    const uint32_t *pixels = ln->src->bg0_3d_line;
    unsigned sx;

    for (sx = 0; sx < 256; sx++) {
        unsigned x = (sx + ln->regs->hofs[0]) & 511;
        uint32_t p;

        if (pixels == NULL || x >= 256 || (p = pixels[x]) >> 24 == 0) {
            dst[sx] = 0;
            continue;
        }
        dst[sx] = ((p >> 2) & PX_COLOR) | (p >> 27) << PX_ALPHA_SHIFT | PX_3D | PX_OPAQUE;
    }
}

// The texture coordinates an affine BG reads along one line: (X + x * PA, Y + x * PC), whole dots, inside the
// width x height area or wrapped around it
typedef struct AffineWalk {
    uint32_t x, y;   // 20.8 fixed point, as unsigned so that the sums wrap
    uint32_t dx, dy; // PA and PC
    unsigned width, height;
    int wrap;
} AffineWalk;

// The next dot's coordinates; returns 0 when it is outside a BG that doesn't wrap
static int AffineNext(AffineWalk *walk, unsigned *x, unsigned *y) {
    int32_t tx = (int32_t)walk->x >> 8;
    int32_t ty = (int32_t)walk->y >> 8;

    walk->x += walk->dx;
    walk->y += walk->dy;
    if (walk->wrap) {
        *x = (unsigned)tx & (walk->width - 1);
        *y = (unsigned)ty & (walk->height - 1);
        return 1;
    }
    if ((uint32_t)tx >= walk->width || (uint32_t)ty >= walk->height) {
        return 0;
    }
    *x = (unsigned)tx;
    *y = (unsigned)ty;
    return 1;
}

// Affine, extended and large BGs: each dot reads (X + x * PA, Y + x * PC) from the line's reference point
static void DrawAffineBg(const Engine *engine, const Line *ln, int n, enum BgKind kind, uint32_t *dst) {
    const Regs *regs = ln->regs;
    const uint8_t *const *vram = ln->src->bg_slots;
    uint32_t mask = ln->bgMask;
    unsigned cnt = regs->bgcnt[n];
    unsigned size = cnt >> 14;
    int i = n - 2;
    uint32_t screen = ln->screenBase + ((cnt >> 8) & 0x1F) * 0x800u;
    uint32_t chr = ln->charBase + ((cnt >> 2) & 0xF) * 0x4000u;
    AffineWalk walk;
    unsigned sx;
    unsigned x;
    unsigned y;

    walk.x = (uint32_t)engine->state.drawRef[i][0];
    walk.y = (uint32_t)engine->state.drawRef[i][1];
    walk.dx = (uint32_t)regs->pa[i];
    walk.dy = (uint32_t)regs->pc[i];
    walk.wrap = (cnt & BGCNT_BIT13) != 0;

    if (kind == BG_AFFINE) {
        // 256 colours from the standard palette, 8-bit map entries; extended palettes don't apply
        walk.width = walk.height = 128u << size;
        for (sx = 0; sx < 256; sx++) {
            unsigned tile;
            unsigned index;
            uint32_t color;

            if (!AffineNext(&walk, &x, &y)) {
                dst[sx] = 0;
                continue;
            }
            tile = Vram8(vram, (screen + (y >> 3) * (walk.width >> 3) + (x >> 3)) & mask);
            index = Vram8(vram, (chr + tile * 64 + (y & 7) * 8 + (x & 7)) & mask);
            color = ln->palette[index];
            dst[sx] = index != 0 ? color : 0;
        }
    } else if (kind == BG_EXTENDED && !(cnt & BGCNT_256_COLORS)) {
        // 16-bit map entries as in text BGs, 256-colour tiles; BG2 and BG3 use extended palette slots 2 and 3
        int extended = (regs->dispcnt & DISPCNT_BG_EXT_PALETTE) != 0;
        const uint8_t *extPalette = ln->src->bg_ext_palette[n];

        walk.width = walk.height = 128u << size;
        for (sx = 0; sx < 256; sx++) {
            unsigned entry;
            unsigned col;
            unsigned row;
            unsigned index;
            uint32_t color;

            if (!AffineNext(&walk, &x, &y)) {
                dst[sx] = 0;
                continue;
            }
            entry = Vram16(vram, (screen + ((y >> 3) * (walk.width >> 3) + (x >> 3)) * 2) & mask);
            col = (entry & 0x400) ? 7 - (x & 7) : (x & 7);
            row = (entry & 0x800) ? 7 - (y & 7) : (y & 7);
            index = Vram8(vram, (chr + (entry & 0x3FF) * 64 + row * 8 + col) & mask);
            color = extended ? ExtPaletteColor(extPalette, (entry >> 12) * 256 + index) : ln->palette[index];
            dst[sx] = index != 0 ? color : 0;
        }
    } else if (kind == BG_EXTENDED && (cnt & 0x4)) {
        // Direct colour bitmap at BGxCNT's screen base in 16 KiB steps, without DISPCNT's; bit 15 is the alpha flag
        uint32_t base = ((cnt >> 8) & 0x1F) * 0x4000u;

        walk.width = kBitmapWidth[size];
        walk.height = kBitmapHeight[size];
        for (sx = 0; sx < 256; sx++) {
            uint16_t color;

            if (!AffineNext(&walk, &x, &y)) {
                dst[sx] = 0;
                continue;
            }
            color = Vram16(vram, (base + (y * walk.width + x) * 2) & mask);
            dst[sx] = (Color18(color) | PX_OPAQUE) & -(uint32_t)(color >> 15);
        }
    } else {
        // 256-colour bitmaps: the extended one at BGxCNT's screen base in 16 KiB steps, without DISPCNT's, and the
        // large one, which fills all 512 KiB of engine A's BG VRAM; large sizes 2 and 3 are prohibited and show nothing
        uint32_t base = 0;

        if (kind == BG_EXTENDED) {
            base = ((cnt >> 8) & 0x1F) * 0x4000u;
            walk.width = kBitmapWidth[size];
            walk.height = kBitmapHeight[size];
        } else if (size <= 1) {
            walk.width = size == 0 ? 512 : 1024;
            walk.height = size == 0 ? 1024 : 512;
        } else {
            memset(dst, 0, 256 * sizeof(*dst));
            return;
        }
        for (sx = 0; sx < 256; sx++) {
            unsigned index;
            uint32_t color;

            if (!AffineNext(&walk, &x, &y)) {
                dst[sx] = 0;
                continue;
            }
            index = Vram8(vram, (base + y * walk.width + x) & mask);
            color = ln->palette[index];
            dst[sx] = index != 0 ? color : 0;
        }
    }
}

// Horizontal mosaic: each block of MOSAIC's H size dots, counted from the left edge, shows its first dot
static void ApplyBgMosaic(const Line *ln, uint32_t *dst) {
    unsigned size = (ln->regs->mosaic & 0xF) + 1;
    unsigned start;
    unsigned x;

    if (size == 1) {
        return;
    }
    for (start = 0; start < 256; start += size) {
        for (x = start + 1; x < start + size && x < 256; x++) {
            dst[x] = dst[start];
        }
    }
}

// One OBJ's texture
typedef struct ObjTexture {
    int bitmap;
    int colors256;
    uint32_t base;        // the first tile or dot
    uint32_t rowStride;   // bytes from one tile row (or bitmap row) to the next
    uint32_t tileBytes;   // 32 or 64
    uint32_t paletteBase; // the OBJ's colour 0: in the OBJ palette (16 colours) or the extended one (256 colours)
    const uint8_t *extPalette;
    int useExt;
} ObjTexture;

// An affine OBJ's colours along the line (0 where transparent): dot i reads texel ((tx + i * PA) >> 8,
// (ty + i * PC) >> 8), nothing outside the texture. One loop per texture format, with everything it reads in locals
static void ObjAffineSpan(const Line *ln, const ObjTexture *tex, unsigned width, unsigned height, int32_t tx,
                          int32_t ty, int32_t pa, int32_t pc, int count, uint32_t *restrict span) {
    const uint8_t *const *vram = ln->src->obj_slots;
    const uint32_t *palette = ln->palette + 256;
    const uint8_t *extPalette = tex->extPalette;
    uint32_t mask = ln->objMask;
    uint32_t base = tex->base;
    uint32_t rowStride = tex->rowStride;
    uint32_t paletteBase = tex->paletteBase;
    int useExt = tex->useExt;
    int i;

    for (i = 0; i < count; i++) {
        span[i] = 0;
    }
    if (tex->bitmap) {
        for (i = 0; i < count; i++, tx += pa, ty += pc) {
            unsigned x = (unsigned)(tx >> 8);
            unsigned y = (unsigned)(ty >> 8);

            if (x < width && y < height) {
                uint16_t dot = Vram16(vram, (base + y * rowStride + x * 2) & mask);

                span[i] = (Color18(dot) | PX_OPAQUE) & -(uint32_t)(dot >> 15);
            }
        }
    } else if (tex->colors256) {
        for (i = 0; i < count; i++, tx += pa, ty += pc) {
            unsigned x = (unsigned)(tx >> 8);
            unsigned y = (unsigned)(ty >> 8);

            if (x < width && y < height) {
                unsigned index =
                    Vram8(vram, (base + (y >> 3) * rowStride + (x >> 3) * 64 + (y & 7) * 8 + (x & 7)) & mask);
                uint32_t color = useExt ? ExtPaletteColor(extPalette, paletteBase + index) : palette[index];

                span[i] = index != 0 ? color : 0;
            }
        }
    } else {
        const uint32_t *colors = palette + paletteBase;

        for (i = 0; i < count; i++, tx += pa, ty += pc) {
            unsigned x = (unsigned)(tx >> 8);
            unsigned y = (unsigned)(ty >> 8);

            if (x < width && y < height) {
                unsigned byte =
                    Vram8(vram, (base + (y >> 3) * rowStride + (x >> 3) * 32 + (y & 7) * 4 + (x & 7) / 2) & mask);
                unsigned index = (byte >> ((x & 1) * 4)) & 0xF;
                uint32_t color = colors[index];

                span[i] = index != 0 ? color : 0;
            }
        }
    }
}

// Texture row y of a normal OBJ, decoded once for the line: row[x] is texel (x, y)
static void ObjRow(const Line *ln, const ObjTexture *tex, unsigned width, unsigned y, uint32_t *row) {
    const uint8_t *const *vram = ln->src->obj_slots;
    uint32_t address = tex->base + (y >> 3) * tex->rowStride + (y & 7) * (tex->tileBytes / 8);
    unsigned x;
    unsigned dot;

    if (tex->bitmap) {
        address = tex->base + y * tex->rowStride;
        for (x = 0; x < width; x++) {
            uint16_t color = Vram16(vram, (address + x * 2) & ln->objMask);

            row[x] = (Color18(color) | PX_OPAQUE) & -(uint32_t)(color >> 15);
        }
        return;
    }
    for (x = 0; x < width; x += 8, address += tex->tileBytes) {
        if (tex->colors256) {
            uint32_t lo = Vram32(vram, address & ln->objMask);
            uint32_t hi = Vram32(vram, (address + 4) & ln->objMask);

            for (dot = 0; dot < 8; dot++) {
                unsigned index = (dot < 4 ? lo >> (dot * 8) : hi >> ((dot - 4) * 8)) & 0xFF;
                uint32_t color =
                    tex->useExt ? ExtPaletteColor(tex->extPalette, tex->paletteBase + index) : ln->palette[256 + index];

                row[x + dot] = index != 0 ? color : 0;
            }
        } else {
            uint32_t data = Vram32(vram, address & ln->objMask);
            const uint32_t *colors = ln->palette + 256 + tex->paletteBase;

            for (dot = 0; dot < 8; dot++) {
                unsigned index = (data >> (dot * 4)) & 0xF;
                uint32_t color = colors[index];

                row[x + dot] = index != 0 ? color : 0;
            }
        }
    }
}

// One OBJ's dots into the OBJ line, from its first visible dot. An opaque dot replaces a transparent one or one of a
// lower BG-relative priority, so the line keeps the best priority and, between equals, the lowest OAM number (GBATEK:
// the DS orders OBJs by a 9-bit value of both). A transparent dot over a transparent one still leaves its priority
// and mosaic flag, which OBJ mosaic reads (melonDS). Returns whether a dot was opaque
static uint32_t PutObjSpan(uint32_t *restrict obj, uint8_t *restrict objAttr, const uint32_t *restrict colors,
                           int count, uint32_t flags, unsigned attr) {
    uint32_t any = 0;
    int i;

    for (i = 0; i < count; i++) {
        uint32_t color = colors[i];
        uint32_t old = obj[i];
        unsigned oldAttr = objAttr[i];
        uint32_t opaque = color >> 31;
        uint32_t transparentBefore = (old >> 31) ^ 1u;
        uint32_t replace = opaque & (transparentBefore | ((attr & OBJ_PRIORITY) < (oldAttr & OBJ_PRIORITY)));

        obj[i] = replace ? color | flags : old;
        objAttr[i] = (uint8_t)((replace | transparentBefore) ? attr : oldAttr);
        any |= opaque;
    }
    return any;
}

// An OBJ window OBJ's opaque dots into the OBJ window
static void PutObjWindowSpan(uint8_t *restrict window, const uint32_t *restrict colors, int count) {
    int i;

    for (i = 0; i < count; i++) {
        window[i] |= (uint8_t)(colors[i] >> 31);
    }
}

// OBJ horizontal mosaic, applied to the finished OBJ line from the left edge: a dot takes the latched dot unless the
// block starts, either dot isn't from a mosaic OBJ, or the dot has a better priority (melonDS's measured rules)
static void ApplyObjMosaic(Engine *engine, const Regs *regs) {
    unsigned size = (regs->mosaic >> 8) & 0xF;
    unsigned counter = 0;
    uint32_t latched = 0;
    uint8_t latchedAttr = 0;
    unsigned x;

    if (size == 0) {
        return;
    }
    for (x = 0; x < 256; x++) {
        uint8_t attr = engine->objAttr[x];

        if (counter == 0 || !(attr & OBJ_MOSAIC) || !(latchedAttr & OBJ_MOSAIC) ||
            (attr & OBJ_PRIORITY) < (latchedAttr & OBJ_PRIORITY)) {
            latched = engine->obj[x];
            latchedAttr = attr;
        }
        engine->obj[x] = latched;
        engine->objAttr[x] = latchedAttr;
        counter = counter == size ? 0 : counter + 1;
    }
}

// The OBJ line and the OBJ window, from OAM in order
static void DrawObjs(Engine *engine, const Line *ln) {
    const Regs *regs = ln->regs;
    const uint8_t *oam = ln->src->oam;
    uint32_t dispcnt = regs->dispcnt;
    int budget = (dispcnt & DISPCNT_HBLANK_FREE) ? sObjBudgetHBlankFree : sObjBudget;
    int spent = 0;
    unsigned i;

    memset(engine->obj, 0, sizeof(engine->obj));
    memset(engine->objAttr, 0, sizeof(engine->objAttr));
    memset(engine->objWindow, 0, sizeof(engine->objWindow));
    engine->objPriorities = 0;
    if (!(dispcnt & DISPCNT_OBJ)) {
        return;
    }

    for (i = 0; i < 128; i++) {
        unsigned attr0 = Read16(oam, i * 8);
        unsigned attr1 = Read16(oam, i * 8 + 2);
        unsigned attr2 = Read16(oam, i * 8 + 4);
        int affine = (attr0 & 0x100) != 0;
        unsigned mode = (attr0 >> 10) & 3;
        unsigned width = kObjWidth[attr0 >> 14][attr1 >> 14];
        unsigned height = kObjHeight[attr0 >> 14][attr1 >> 14];
        unsigned boundWidth = width;
        unsigned boundHeight = height;
        int window = mode == 2;
        int mosaic = (attr0 & 0x1000) && !window;
        unsigned y = attr0 & 0xFF;
        unsigned row;
        int x;
        int xBegin;
        int xEnd;
        uint32_t flags = PX_OPAQUE;
        unsigned attr = ((attr2 >> 10) & 3) | (mosaic ? OBJ_MOSAIC : 0);
        ObjTexture tex;
        uint32_t span[128]; // the OBJ's colours along the line: up to 128 dots, or a 64-dot row and its mirror
        const uint32_t *colors;
        int count;
        int i;

        if (!affine && (attr0 & 0x200)) {
            continue; // OBJ disabled
        }
        if (affine && (attr0 & 0x200)) {
            boundWidth *= 2;
            boundHeight *= 2;
        }
        if (((ln->line - y) & 0xFF) >= boundHeight) {
            continue;
        }
        if (budget > 0) {
            spent += affine ? 10 + 2 * (int)boundWidth : (int)width;
            if (spent > budget) {
                break;
            }
        }

        // Vertical mosaic draws the row of the line the block started on; an OBJ starting inside a block draws its
        // top row until the next block (melonDS)
        row = ((mosaic ? engine->state.objMosaicLine : ln->line) - y) & 0xFF;
        if (row >= boundHeight) {
            row = 0;
        }
        x = (int)(attr1 & 0x1FF);
        if (x >= 256) {
            x -= 512; // X is 9 bits: 256..511 is left of the screen
        }
        if (x <= -(int)boundWidth) {
            continue;
        }
        xBegin = x < 0 ? 0 : x;
        xEnd = x + (int)boundWidth > 256 ? 256 : x + (int)boundWidth;
        count = xEnd - xBegin;

        memset(&tex, 0, sizeof(tex));
        if (mode == 3) {
            // Bitmap OBJ: 15-bit dots with an alpha bit; attribute 2's palette field is the OBJ's alpha, 0 invisible,
            // blending as EVA = alpha + 1 and EVB = 16 - EVA (melonDS)
            unsigned alpha = attr2 >> 12;
            unsigned tile = attr2 & 0x3FF;

            if (alpha == 0) {
                continue;
            }
            tex.bitmap = 1;
            if (dispcnt & DISPCNT_BMP_OBJ_1D) {
                tex.base = tile * ((dispcnt & DISPCNT_BMP_OBJ_1D_256) ? 256u : 128u);
                tex.rowStride = width * 2;
            } else if (dispcnt & DISPCNT_BMP_OBJ_256) {
                tex.base = (tile & 0x1F) * 0x10 + (tile & ~0x1Fu) * 0x80;
                tex.rowStride = 512;
            } else {
                tex.base = (tile & 0xF) * 0x10 + (tile & ~0xFu) * 0x80;
                tex.rowStride = 256;
            }
            flags |= PX_BITMAP | (alpha + 1) << PX_ALPHA_SHIFT;
        } else {
            unsigned tile = attr2 & 0x3FF;

            tex.colors256 = (attr0 & 0x2000) != 0;
            tex.tileBytes = tex.colors256 ? 64 : 32;
            if (dispcnt & DISPCNT_OBJ_1D) {
                tex.base = tile * (32u << ((dispcnt >> 20) & 3));
                tex.rowStride = (width / 8) * tex.tileBytes;
            } else {
                // 2D mapping: a matrix 32 tiles of 32 bytes wide, the same as the GBA
                tex.base = tile * 32;
                tex.rowStride = 32 * 32;
            }
            if (tex.colors256) {
                // With extended OBJ palettes on, attribute 2's palette field picks one of 16 palettes of 256 colours
                tex.useExt = (dispcnt & DISPCNT_OBJ_EXT_PALETTE) != 0;
                tex.extPalette = ln->src->obj_ext_palette;
                tex.paletteBase = (attr2 >> 12) * 256;
            } else {
                tex.paletteBase = (attr2 >> 12) * 16;
            }
            if (mode == 1) {
                flags |= PX_SEMI;
            }
        }

        if (affine) {
            unsigned group = (attr1 >> 9) & 0x1F;
            int32_t pa = (int16_t)Read16(oam, group * 32 + 6);
            int32_t pb = (int16_t)Read16(oam, group * 32 + 14);
            int32_t pc = (int16_t)Read16(oam, group * 32 + 22);
            int32_t pd = (int16_t)Read16(oam, group * 32 + 30);
            int32_t cx = xBegin - x - (int)boundWidth / 2;
            int32_t cy = (int32_t)row - (int)boundHeight / 2;
            // Texture coordinates in 8 fractional bits, from the OBJ's centre
            int32_t tx = pa * cx + pb * cy + (int32_t)(width << 7);
            int32_t ty = pc * cx + pd * cy + (int32_t)(height << 7);

            ObjAffineSpan(ln, &tex, width, height, tx, ty, pa, pc, count, span);
            colors = span;
        } else {
            unsigned col = (unsigned)(xBegin - x);

            ObjRow(ln, &tex, width, (attr1 & 0x2000) ? height - 1 - row : row, span);
            colors = span + col;
            if (attr1 & 0x1000) {
                for (i = 0; i < count; i++) {
                    span[64 + i] = span[width - 1 - col - (unsigned)i];
                }
                colors = span + 64;
            }
        }
        if (window) {
            PutObjWindowSpan(engine->objWindow + xBegin, colors, count);
        } else if (PutObjSpan(engine->obj + xBegin, engine->objAttr + xBegin, colors, count, flags, attr)) {
            engine->objPriorities |= 1u << (attr & OBJ_PRIORITY);
        }
    }
    ApplyObjMosaic(engine, regs);
}

static void FillMask(Engine *engine, const WindowSpan *span, unsigned mask) {
    int k;

    for (k = 0; k < span->count; k++) {
        memset(engine->winMask + span->start[k], (int)mask, span->end[k] - span->start[k]);
    }
}

// Which layers and effects each dot shows: WIN0, then WIN1, then the OBJ window, then outside; everything when no
// window is on (GBATEK)
static void BuildWindowMask(Engine *engine, const Regs *regs) {
    uint32_t dispcnt = regs->dispcnt;
    unsigned x;

    if (!(dispcnt & (DISPCNT_WIN(0) | DISPCNT_WIN(1) | DISPCNT_OBJWIN))) {
        memset(engine->winMask, 0x3F, sizeof(engine->winMask));
        return;
    }
    memset(engine->winMask, regs->winOut & 0x3F, sizeof(engine->winMask));
    if (dispcnt & DISPCNT_OBJWIN) {
        uint8_t objMask = (uint8_t)((regs->winOut >> 8) & 0x3F);

        for (x = 0; x < 256; x++) {
            if (engine->objWindow[x]) {
                engine->winMask[x] = objMask;
            }
        }
    }
    FillMask(engine, &engine->state.winSpan[1], (regs->winIn >> 8) & 0x3F);
    FillMask(engine, &engine->state.winSpan[0], regs->winIn & 0x3F);
}

// One layer over the stack: where it has a dot the window shows, the dot, tagged with its layer, goes on top and the
// old top behind it. Written without branches, so that it vectorizes
static void StackLayer(uint32_t *restrict top, uint32_t *restrict below, const uint32_t *restrict layer,
                       const uint8_t *restrict winMask, unsigned number) {
    uint32_t tag = (uint32_t)number << PX_LAYER_SHIFT;
    unsigned x;

    for (x = 0; x < 256; x++) {
        uint32_t dot = layer[x];
        uint32_t take = 0u - ((dot >> 31) & (winMask[x] >> number) & 1u);
        uint32_t front = top[x];

        below[x] = (front & take) | (below[x] & ~take);
        top[x] = ((dot | tag) & take) | (front & ~take);
    }
}

// The OBJ dots of one BG-relative priority over the stack
static void StackObjs(uint32_t *restrict top, uint32_t *restrict below, const uint32_t *restrict obj,
                      const uint8_t *restrict objAttr, const uint8_t *restrict winMask, unsigned priority) {
    uint32_t tag = (uint32_t)LAYER_OBJ << PX_LAYER_SHIFT;
    unsigned x;

    for (x = 0; x < 256; x++) {
        uint32_t dot = obj[x];
        uint32_t take = 0u - ((dot >> 31) & (winMask[x] >> LAYER_OBJ) & ((objAttr[x] & OBJ_PRIORITY) == priority) & 1u);
        uint32_t front = top[x];

        below[x] = (front & take) | (below[x] & ~take);
        top[x] = ((dot | tag) & take) | (front & ~take);
    }
}

// BG, OBJ and 3D after the colour effects, before master brightness: display capture's source A
static void Compose(Engine *engine, const Line *ln) {
    const Regs *regs = ln->regs;
    uint32_t dispcnt = regs->dispcnt;
    uint32_t *out = engine->composite;
    uint32_t backdrop = ln->palette[0] | (uint32_t)LAYER_BACKDROP << PX_LAYER_SHIFT;
    unsigned bldcnt = regs->bldcnt;
    unsigned effect = (bldcnt >> 6) & 3;
    unsigned eva = Min16(regs->bldalpha & 0x1F);
    unsigned evb = Min16((regs->bldalpha >> 8) & 0x1F);
    unsigned evy = Min16(regs->bldy & 0x1F);
    int drawn[4] = { 0, 0, 0, 0 };
    int n;
    int priority;
    unsigned x;

    // Forced blank shows white in place of the BG/OBJ picture (melonDS)
    if (dispcnt & DISPCNT_FORCED_BLANK) {
        for (x = 0; x < 256; x++) {
            out[x] = WHITE6;
        }
        return;
    }

    DrawObjs(engine, ln);
    BuildWindowMask(engine, regs);

    for (n = 0; n < 4; n++) {
        enum BgKind kind;

        if (!(dispcnt & DISPCNT_BG(n)) || (kind = BgKindOf(ln, n)) == BG_NONE) {
            continue;
        }
        switch (kind) {
        case BG_TEXT:
            DrawTextBg(engine, ln, n, engine->bg[n]);
            break;
        case BG_3D:
            Draw3dBg(ln, engine->bg[n]);
            break;
        default:
            DrawAffineBg(engine, ln, n, kind, engine->bg[n]);
            break;
        }
        if (kind != BG_3D && (regs->bgcnt[n] & BGCNT_MOSAIC)) {
            ApplyBgMosaic(ln, engine->bg[n]);
        }
        drawn[n] = 1;
    }

    // Back to front: priority 3 to 0; within one, BGs from BG3 to BG0 and the OBJs last, so an OBJ is in front of a
    // BG of its own priority and the lower BG number in front of a higher one
    for (x = 0; x < 256; x++) {
        engine->top[x] = backdrop;
        engine->below[x] = backdrop;
    }
    for (priority = 3; priority >= 0; priority--) {
        for (n = 3; n >= 0; n--) {
            if (drawn[n] && (int)(regs->bgcnt[n] & 3) == priority) {
                StackLayer(engine->top, engine->below, engine->bg[n], engine->winMask, (unsigned)n);
            }
        }
        if (engine->objPriorities & (1u << priority)) {
            StackObjs(engine->top, engine->below, engine->obj, engine->objAttr, engine->winMask, (unsigned)priority);
        }
    }

    // Colour effects. A semi-transparent or bitmap OBJ, and the 3D layer, blend with a 2nd target below them whatever
    // BLDCNT's mode, 1st targets and the window say (GBATEK for the OBJs and, unsure, for 3D; melonDS measured both
    // ignoring the window). Otherwise the top layer, as a 1st target where the window allows effects, gets BLDCNT's
    // effect
    for (x = 0; x < 256; x++) {
        uint32_t top = engine->top[x];
        uint32_t result = top & PX_COLOR;
        int secondTarget = (bldcnt & (0x100u << PX_LAYER(engine->below[x]))) != 0;

        if ((top & (PX_SEMI | PX_BITMAP)) && secondTarget) {
            if (top & PX_BITMAP) {
                result = BlendAlpha(top, engine->below[x], PX_ALPHA(top), 16 - PX_ALPHA(top));
            } else {
                result = BlendAlpha(top, engine->below[x], eva, evb);
            }
        } else if ((top & PX_3D) && secondTarget) {
            result = Blend3d(top, engine->below[x], PX_ALPHA(top));
        } else if (effect != 0 && (bldcnt & (1u << PX_LAYER(top))) && (engine->winMask[x] & WINCTL_EFFECTS)) {
            if (effect == 1) {
                if (secondTarget) {
                    result = BlendAlpha(top, engine->below[x], eva, evb);
                }
            } else if (effect == 2) {
                result = Brighten(top, evy, 8);
            } else {
                result = Darken(top, evy, 7);
            }
        }
        out[x] = result;
    }
}

// Display mode 2: 256x192 15-bit dots from VRAM bank A to D in LCDC, without the 2D engine; bit 15 is ignored
static void DrawVramDisplay(const Line *ln, uint32_t *dst) {
    uint32_t base = ((ln->regs->dispcnt >> 18) & 3) * 0x20000u + ln->line * 512;
    unsigned x;

    for (x = 0; x < 256; x++) {
        dst[x] = Color18(Vram16(ln->src->lcdc_slots, base + x * 2) & 0x7FFF);
    }
}

// Master brightness (GBATEK "DS Video Stuff"): up c + (63 - c) * F / 16, down c - c * F / 16, F above 16 as 16, mode 3
// none. melonDS measured up rounding down and down rounding up (see BlendAlpha)
static void ApplyMasterBrightness(uint16_t value, uint32_t *line) {
    unsigned mode = value >> 14;
    unsigned factor = Min16(value & 0x1F);
    unsigned x;

    if (mode == 1) {
        for (x = 0; x < 256; x++) {
            line[x] = Brighten(line[x], factor, 0);
        }
    } else if (mode == 2) {
        for (x = 0; x < 256; x++) {
            line[x] = Darken(line[x], factor, 15);
        }
    }
}

void port_ds2d_render_line(const PortDs2dSource *src, int engineNum, int lineNum, uint32_t out[256]) {
    Engine *engine;
    Regs regs;
    Line ln;
    uint32_t line6[256];
    unsigned mode;
    int compose;
    unsigned x;

    if (lineNum < 0 || lineNum >= 192) {
        return;
    }
    engineNum &= 1;
    engine = &sEngines[engineNum];
    ReadRegs(src->io, engineNum, &regs);

    if (!engine->framePending && lineNum == engine->state.nextLine - 1) {
        // The line last rendered, again: it starts from the state it started with the first time
        engine->state = engine->lineStart;
    } else if (engine->framePending || lineNum < engine->state.nextLine) {
        StartFrame(engine, &regs);
    }
    while (engine->state.nextLine < lineNum) {
        BeginLine(engine, &regs, (unsigned)engine->state.nextLine);
        EndLine(engine, &regs, (unsigned)engine->state.nextLine);
    }
    engine->lineStart = engine->state;
    BeginLine(engine, &regs, (unsigned)lineNum);

    ln.src = src;
    ln.regs = &regs;
    ln.palette = engine->palette18;
    ln.engine = engineNum;
    ln.line = (unsigned)lineNum;
    ln.bgMask = engineNum == 0 ? 0x7FFFF : 0x1FFFF;
    ln.objMask = engineNum == 0 ? 0x3FFFF : 0x1FFFF;
    ln.charBase = ((regs.dispcnt >> 24) & 7) * 0x10000u;
    ln.screenBase = ((regs.dispcnt >> 27) & 7) * 0x10000u;

    // Display mode: engine A has four, engine B only bit 16
    mode = (regs.dispcnt >> 16) & (engineNum == 0 ? 3 : 1);
    compose = mode == 1 || (regs.dispcapcnt & DISPCAPCNT_ENABLE);
    if (compose) {
        UpdatePalette(engine, src->palette);
        Compose(engine, &ln);
    }
    engine->compositeValid = compose;

    switch (mode) {
    case 0:
        // Display off: white, and master brightness doesn't apply
        for (x = 0; x < 256; x++) {
            out[x] = 0xFFFFFFFFu;
        }
        EndLine(engine, &regs, (unsigned)lineNum);
        return;
    case 1:
        memcpy(line6, engine->composite, sizeof(line6));
        break;
    case 2:
        DrawVramDisplay(&ln, line6);
        break;
    default:
        // Main memory display (the DISP_MMEM_FIFO stream) isn't implemented: black
        memset(line6, 0, sizeof(line6));
        break;
    }
    ApplyMasterBrightness(regs.masterBright, line6);

    // 6 bits to 8 per channel, (c << 2) | (c >> 4), all three channels at once
    for (x = 0; x < 256; x++) {
        out[x] = 0xFF000000u | line6[x] << 2 | ((line6[x] >> 4) & 0x030303u);
    }
    EndLine(engine, &regs, (unsigned)lineNum);
}

void port_ds2d_begin_frame(int engineNum) {
    sEngines[engineNum & 1].framePending = 1;
}

void port_ds2d_reset(int engineNum) {
    Engine *engine = &sEngines[engineNum & 1];

    memset(engine, 0, sizeof(*engine));
    engine->framePending = 1;
    engine->state.nextLine = 263;
}

void port_ds2d_reload_reference(int engineNum, int bg, int axis) {
    if (bg == 2 || bg == 3) {
        sEngines[engineNum & 1].reload[bg - 2] |= (uint8_t)(axis ? 2 : 1);
    }
}

void port_ds2d_set_obj_cycle_budget(int cycles, int cyclesHBlankFree) {
    sObjBudget = cycles;
    sObjBudgetHBlankFree = cyclesHBlankFree;
}

const uint32_t *port_ds2d_capture_source_a(void) {
    return sEngines[0].compositeValid ? sEngines[0].composite : NULL;
}
