// The canonical per-game-frame digest (digest.h): the decodes of the port's register bytes into the form melonDS 1.1
// keeps, each group's canonical bytes, the hash, and the stream's writer, reader and comparison. The facts are
// melonDS 1.1's, the release port/tools/oracle/melonds.cmake pins (src/GPU2D.cpp's Unit::Write8, Write16 and Write32;
// GPU3D.cpp's Write16, Write32 and SubmitPolygon; GPU.cpp's MapVRAM_*; NDS.cpp's ARM9IOWrite16, ARM9IOWrite32 and
// MapSharedWRAM), and GBATEK's for the bits each register keeps. Every screen of the game is made from the state
// encoded here.
#include "digest.h"

// xxHash's single header, from the release port/tools/oracle/xxhash.cmake pins, which the build fetches and puts on a
// system include path (port/cmake/oracle.cmake). Its functions are inline and private to this file, so that they can't
// meet the copy of xxHash that melonDS's core links into the oracle
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

// XXH3's output is fixed from xxHash 0.8.0, the release that declared it stable; the tests check known answers
#if XXH_VERSION_NUMBER < 800
#error "the digest's hash is XXH3-64 as xxHash 0.8.0 and later give it (port/tools/oracle/xxhash.cmake)"
#endif

static const char *const sGroupNames[PBW_DIGEST_GROUPS] = {
    "vram.A", "vram.B",  "vram.C",   "vram.D",   "vram.E",    "vram.F",     "vram.G",
    "vram.H", "vram.I",  "vram.map", "pal",      "oam",       "regs2d.A",   "regs2d.B",
    "sys",    "capture", "g3d.list", "g3d.regs", "g3d.stats", "screen.top", "screen.bottom",
};

const char *pbw_digest_group_name(int group) {
    return group >= 0 && group < PBW_DIGEST_GROUPS ? sGroupNames[group] : NULL;
}

int pbw_digest_group_find(const char *name) {
    int group;

    for (group = 0; group < PBW_DIGEST_GROUPS; group++) {
        if (strcmp(name, sGroupNames[group]) == 0) {
            return group;
        }
    }
    return -1;
}

// --- Registers

// The pages of banks A to I (GBATEK "DS Memory Control - VRAM"): 128, 128, 128, 128, 64, 16, 16, 32 and 16 KiB
static const uint8_t sBankPages[PBW_DIGEST_VRAM_BANKS] = { 8, 8, 8, 8, 4, 1, 1, 2, 1 };

// The bits of VRAMCNT_A to VRAMCNT_I that melonDS keeps (GPU.cpp's MapVRAM_AB, _CD, _E, _FG, _H and _I): the enable
// (bit 7), the offset (bits 3-4) of the banks that have one, and the master select's 2 or 3 bits
static const uint8_t sVramcntKept[PBW_DIGEST_VRAM_BANKS] = { 0x9b, 0x9b, 0x9f, 0x9f, 0x87, 0x9f, 0x9f, 0x83, 0x83 };

// DISPCNT of engine B: melonDS keeps these bits (GPU2D.cpp: "if (Num) DispCnt &= 0xC0B1FFF7"), engine B having no 3D
// BG0, no VRAM or main memory display, no 256-byte bitmap OBJ boundary and no character or screen base (GBATEK)
#define DISPCNT_B_KEPT 0xc0b1fff7u
// The bits that act (GBATEK "DS Video BG Control", "DS Video Special Effects", "DS Video Display Capture"; melonDS
// masks DISPCAPCNT, BLDCNT and BLDALPHA alike, and clamps BLDY)
#define SCROLL_KEPT 0x1ffu         // BGxHOFS, BGxVOFS: 9 bits
#define REFERENCE_KEPT 0x0fffffffu // BGxX, BGxY: 28 bits, signed, 8 of them fraction
#define REFERENCE_SIGN 0x08000000u
#define WINDOW_CONTROL_KEPT 0x3fu // each byte of WININ and WINOUT: BG0-3, OBJ and the colour effect
#define MOSAIC_KEPT 0xfu          // each of MOSAIC's four sizes
#define BLDCNT_KEPT 0x3fffu
#define BLDALPHA_KEPT 0x1f1fu
#define BLDY_KEPT 0x1fu
#define BLDY_MAX 16u
#define MASTER_BRIGHT_KEPT 0xc01fu // the factor (bits 0-4) and the mode (bits 14-15)
#define DISPCAPCNT_KEPT 0xef3f1f1fu
// The 3D rendering registers' bits (GBATEK "DS 3D Rear-Plane", "DS 3D Fog", "DS 3D Toon, Edge, Fog, Alpha-Blending";
// melonDS keeps DISP3DCNT's 0x4fff with its status bits 12 and 13, ALPHA_TEST_REF's 5 bits, FOG_OFFSET's 15 and the
// fog table's 7, and the others as written)
#define DISP3DCNT_KEPT 0x4fffu
#define ALPHA_TEST_REF_KEPT 0x1fu
#define CLEAR_COLOR_KEPT 0x3f1fffffu
#define DEPTH_KEPT 0x7fffu // CLEAR_DEPTH, FOG_OFFSET
#define FOG_COLOR_KEPT 0x001f7fffu
#define FOG_DENSITY_KEPT 0x7fu
#define COLOR_KEPT 0x7fffu // TOON_TABLE and EDGE_COLOR entries
// POWCNT1 (the ARM9's: both LCDs, engine A, the 3D renderer and geometry engine, engine B, the display swap) and
// WRAMCNT (2 bits), as melonDS keeps them (NDS.cpp: "PowerControl9 = val & 0x820F"; MapSharedWRAM switches on
// "WRAMCnt & 0x3")
#define POWCNT1_KEPT 0x820fu
#define WRAMCNT_KEPT 0x3u

static uint32_t Read16(const uint8_t *io, uint32_t offset) {
    return (uint32_t)io[offset] | (uint32_t)io[offset + 1] << 8;
}

static uint32_t Read32(const uint8_t *io, uint32_t offset) {
    return Read16(io, offset) | Read16(io, offset + 2) << 16;
}

// BG2X, BG2Y, BG3X or BG3Y as melonDS's writes store it: the value written, with bits 28-31 set when bit 27 is
// (Write32's "if (val & 0x08000000) val |= 0xF0000000", and Write16's on the upper half); set bits 28-31 stay
static uint32_t MelonReference(uint32_t written) {
    return (written & REFERENCE_SIGN) != 0 ? written | ~REFERENCE_KEPT : written;
}

// The reference point's 28 bits sign-extended from bit 27, as the hardware uses them
static uint32_t SignExtend28(uint32_t value) {
    return ((value & REFERENCE_KEPT) ^ REFERENCE_SIGN) - REFERENCE_SIGN;
}

// BLDY's factor: 5 bits, where 17 to 31 act as 16 (melonDS: "if (EVY > 16) EVY = 16")
static uint32_t Bldy(uint32_t written) {
    written &= BLDY_KEPT;
    return written > BLDY_MAX ? BLDY_MAX : written;
}

void pbw_digest_regs2d_from_io(PbwDigestRegs2d *regs, const uint8_t *io, int engine) {
    uint32_t mosaic = Read16(io, 0x4c);
    int i;

    memset(regs, 0, sizeof(*regs));
    regs->dispcnt = Read32(io, 0x00);
    if (engine == PBW_DIGEST_ENGINE_B) {
        regs->dispcnt &= DISPCNT_B_KEPT;
    }
    for (i = 0; i < 4; i++) {
        regs->bgcnt[i] = (uint16_t)Read16(io, 0x08 + 2 * i);
        regs->bgHofs[i] = (uint16_t)Read16(io, 0x10 + 4 * i);
        regs->bgVofs[i] = (uint16_t)Read16(io, 0x12 + 4 * i);
    }
    // BG2's affine parameters at 0x20, BG3's at 0x30: PA, PB, PC, PD, then X and Y
    for (i = 0; i < 2; i++) {
        uint32_t base = 0x20 + 0x10 * i;
        regs->bgPa[i] = (uint16_t)Read16(io, base);
        regs->bgPb[i] = (uint16_t)Read16(io, base + 2);
        regs->bgPc[i] = (uint16_t)Read16(io, base + 4);
        regs->bgPd[i] = (uint16_t)Read16(io, base + 6);
        regs->bgX[i] = MelonReference(Read32(io, base + 8));
        regs->bgY[i] = MelonReference(Read32(io, base + 12));
    }
    // WINxH and WINxV hold X2 or Y2 in their low byte and X1 or Y1 in their high one; melonDS splits them into X1, X2,
    // Y1, Y2 (Write16: "Win0Coords[1] = val & 0xFF; Win0Coords[0] = val >> 8")
    regs->win0[0] = io[0x41];
    regs->win0[1] = io[0x40];
    regs->win0[2] = io[0x45];
    regs->win0[3] = io[0x44];
    regs->win1[0] = io[0x43];
    regs->win1[1] = io[0x42];
    regs->win1[2] = io[0x47];
    regs->win1[3] = io[0x46];
    // melonDS keeps WININ's and WINOUT's bytes whole
    for (i = 0; i < 4; i++) {
        regs->winCnt[i] = io[0x48 + i];
    }
    regs->bgMosaic[0] = (uint8_t)(mosaic & MOSAIC_KEPT);
    regs->bgMosaic[1] = (uint8_t)(mosaic >> 4 & MOSAIC_KEPT);
    regs->objMosaic[0] = (uint8_t)(mosaic >> 8 & MOSAIC_KEPT);
    regs->objMosaic[1] = (uint8_t)(mosaic >> 12 & MOSAIC_KEPT);
    regs->bldcnt = (uint16_t)(Read16(io, 0x50) & BLDCNT_KEPT);
    regs->bldalpha = (uint16_t)(Read16(io, 0x52) & BLDALPHA_KEPT);
    regs->bldy = (uint8_t)Bldy(io[0x54]);
    // melonDS keeps MASTER_BRIGHT as written
    regs->masterBright = (uint16_t)Read16(io, 0x6c);
    // Engine B has no capture unit: melonDS sends 16- and 32-bit stores at 0x04000064 and 0x04000066 to engine A and
    // none at 0x04001064 anywhere (NDS.cpp's ARM9IOWrite16 and ARM9IOWrite32), so engine B's field stays 0 from reset
    regs->dispcapcnt = engine == PBW_DIGEST_ENGINE_A ? Read32(io, 0x64) & DISPCAPCNT_KEPT : 0;
}

void pbw_digest_regs2d_words(const PbwDigestRegs2d *regs, int engine, uint32_t words[PBW_DIGEST_REGS2D_WORDS]) {
    const uint8_t *const windows[2] = { regs->win0, regs->win1 };
    int n = 0;
    int i;
    int j;

    words[n++] = engine == PBW_DIGEST_ENGINE_B ? regs->dispcnt & DISPCNT_B_KEPT : regs->dispcnt;
    for (i = 0; i < 4; i++) {
        words[n++] = regs->bgcnt[i];
    }
    for (i = 0; i < 4; i++) {
        words[n++] = regs->bgHofs[i] & SCROLL_KEPT;
        words[n++] = regs->bgVofs[i] & SCROLL_KEPT;
    }
    for (i = 0; i < 2; i++) {
        words[n++] = regs->bgPa[i];
        words[n++] = regs->bgPb[i];
        words[n++] = regs->bgPc[i];
        words[n++] = regs->bgPd[i];
        words[n++] = SignExtend28(regs->bgX[i]);
        words[n++] = SignExtend28(regs->bgY[i]);
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 4; j++) {
            words[n++] = windows[i][j];
        }
    }
    for (i = 0; i < 4; i++) {
        words[n++] = regs->winCnt[i] & WINDOW_CONTROL_KEPT;
    }
    words[n++] = regs->bgMosaic[0] & MOSAIC_KEPT;
    words[n++] = regs->bgMosaic[1] & MOSAIC_KEPT;
    words[n++] = regs->objMosaic[0] & MOSAIC_KEPT;
    words[n++] = regs->objMosaic[1] & MOSAIC_KEPT;
    words[n++] = regs->bldcnt & BLDCNT_KEPT;
    words[n++] = regs->bldalpha & BLDALPHA_KEPT;
    words[n++] = Bldy(regs->bldy);
    words[n++] = regs->masterBright & MASTER_BRIGHT_KEPT;
    words[n++] = engine == PBW_DIGEST_ENGINE_A ? regs->dispcapcnt & DISPCAPCNT_KEPT : 0;
}

void pbw_digest_sys_from_io(PbwDigestSys *sys, const uint8_t *io) {
    sys->powcnt1 = (uint16_t)(Read16(io, 0x304) & POWCNT1_KEPT);
    // melonDS keeps WRAMCNT's byte whole
    sys->wramcnt = io[0x247];
}

void pbw_digest_regs3d_from_io(PbwDigestRegs3d *regs, const uint8_t *io) {
    int i;

    memset(regs, 0, sizeof(*regs));
    regs->disp3dcnt = Read16(io, 0x60);
    regs->alphaTestRef = (uint8_t)(io[0x340] & ALPHA_TEST_REF_KEPT);
    regs->clearColor = Read32(io, 0x350);
    regs->clearDepth = (uint16_t)Read16(io, 0x354);
    regs->clearOffset = (uint16_t)Read16(io, 0x356);
    regs->fogColor = Read32(io, 0x358);
    regs->fogOffset = (uint16_t)(Read16(io, 0x35c) & DEPTH_KEPT);
    for (i = 0; i < 32; i++) {
        regs->fogTable[i] = (uint8_t)(io[0x360 + i] & FOG_DENSITY_KEPT);
        regs->toonTable[i] = (uint16_t)Read16(io, 0x380 + 2 * i);
    }
    for (i = 0; i < 8; i++) {
        regs->edgeColor[i] = (uint16_t)Read16(io, 0x330 + 2 * i);
    }
}

void pbw_digest_regs3d_words(const PbwDigestRegs3d *regs, uint32_t words[PBW_DIGEST_REGS3D_WORDS]) {
    int n = 0;
    int i;

    words[n++] = regs->disp3dcnt & DISP3DCNT_KEPT;
    words[n++] = regs->alphaTestRef & ALPHA_TEST_REF_KEPT;
    words[n++] = regs->clearColor & CLEAR_COLOR_KEPT;
    words[n++] = regs->clearDepth & DEPTH_KEPT;
    words[n++] = regs->clearOffset;
    words[n++] = regs->fogColor & FOG_COLOR_KEPT;
    words[n++] = regs->fogOffset & DEPTH_KEPT;
    for (i = 0; i < 32; i++) {
        words[n++] = regs->fogTable[i] & FOG_DENSITY_KEPT;
    }
    for (i = 0; i < 32; i++) {
        words[n++] = regs->toonTable[i] & COLOR_KEPT;
    }
    for (i = 0; i < 8; i++) {
        words[n++] = regs->edgeColor[i] & COLOR_KEPT;
    }
}

uint32_t pbw_digest_zbuffer_depth(int32_t clipZ, uint32_t w) {
    int64_t depth;
    uint32_t cut;

    w &= 0xffffffu;
    if (w == 0) {
        return 0x7ffe00u;
    }
    depth = ((int64_t)clipZ * 0x4000 / (int64_t)w + 0x3fff) * 0x200;
    // melonDS stores the product in a 32-bit signed integer, then clamps it: a negative one is 0
    cut = (uint32_t)(uint64_t)depth;
    if ((cut & 0x80000000u) != 0) {
        return 0;
    }
    return cut > 0xffffffu ? 0xffffffu : cut;
}

unsigned pbw_digest_vram_pages(int bank) {
    return bank >= 0 && bank < PBW_DIGEST_VRAM_BANKS ? sBankPages[bank] : 0;
}

// --- Encoding

void pbw_digest_bytes_free(PbwDigestBytes *bytes) {
    free(bytes->data);
    bytes->data = NULL;
    bytes->size = 0;
    bytes->capacity = 0;
}

// Room for more bytes after the buffer's size, or 0 when it can't grow
static int Reserve(PbwDigestBytes *bytes, size_t more) {
    size_t capacity = bytes->capacity != 0 ? bytes->capacity : 4096;
    uint8_t *data;

    if (bytes->capacity - bytes->size >= more) {
        return 1;
    }
    while (capacity - bytes->size < more) {
        if (capacity > SIZE_MAX / 2) {
            return 0;
        }
        capacity *= 2;
    }
    data = realloc(bytes->data, capacity);
    if (data == NULL) {
        return 0;
    }
    bytes->data = data;
    bytes->capacity = capacity;
    return 1;
}

// Where a group's canonical bytes go: into a hash, into a byte buffer, or both. Small pieces are staged, so that the
// hash takes them a kilobyte at a time
typedef struct Encoder {
    XXH3_state_t *state;
    PbwDigestBytes *bytes;
    int noMemory;
    size_t staged;
    uint8_t stage[1024];
} Encoder;

static void Emit(Encoder *encoder, const void *data, size_t size) {
    if (size == 0) {
        return;
    }
    if (encoder->state != NULL) {
        (void)XXH3_64bits_update(encoder->state, data, size);
    }
    if (encoder->bytes != NULL && !encoder->noMemory) {
        if (!Reserve(encoder->bytes, size)) {
            encoder->noMemory = 1;
            return;
        }
        memcpy(encoder->bytes->data + encoder->bytes->size, data, size);
        encoder->bytes->size += size;
    }
}

static void Flush(Encoder *encoder) {
    Emit(encoder, encoder->stage, encoder->staged);
    encoder->staged = 0;
}

static void PutBytes(Encoder *encoder, const void *data, size_t size) {
    if (encoder->staged + size > sizeof(encoder->stage)) {
        Flush(encoder);
        if (size > sizeof(encoder->stage)) {
            Emit(encoder, data, size);
            return;
        }
    }
    memcpy(encoder->stage + encoder->staged, data, size);
    encoder->staged += size;
}

static void PutZeros(Encoder *encoder, size_t size) {
    static const uint8_t zeros[1024];

    while (size > 0) {
        size_t piece = size < sizeof(zeros) ? size : sizeof(zeros);
        PutBytes(encoder, zeros, piece);
        size -= piece;
    }
}

// A 32-bit word, little-endian on every host
static void PutWord(Encoder *encoder, uint32_t word) {
    const uint8_t bytes[4] = { (uint8_t)word, (uint8_t)(word >> 8), (uint8_t)(word >> 16), (uint8_t)(word >> 24) };

    PutBytes(encoder, bytes, sizeof(bytes));
}

static void PutWords(Encoder *encoder, const uint32_t *words, size_t count) {
    size_t i;

    for (i = 0; i < count; i++) {
        PutWord(encoder, words[i]);
    }
}

// A bank's region that holds a capture of 3D (banks A-D only), checked: 0 with an empty region for a bank that holds
// none, -1 for a region outside its bank
static int HeldRegion(const PbwDigestFrame *frame, int bank, uint32_t *offset, uint32_t *size) {
    *offset = 0;
    *size = 0;
    if (bank >= PBW_DISPLAY_BANKS || !frame->banks3d.holds3d[bank]) {
        return 0;
    }
    if (frame->banks3d.offset[bank] >= PBW_DISPLAY_BANK_SIZE || frame->banks3d.size[bank] > PBW_DISPLAY_BANK_SIZE) {
        return -1;
    }
    *offset = frame->banks3d.offset[bank];
    *size = frame->banks3d.size[bank];
    return 0;
}

// Bytes from..to of a bank, from its page that starts at start (NULL reads as zeros)
static void PutSpan(Encoder *encoder, const uint8_t *page, uint32_t start, uint32_t from, uint32_t to) {
    if (to <= from) {
        return;
    }
    if (page != NULL) {
        PutBytes(encoder, page + (from - start), to - from);
    } else {
        PutZeros(encoder, to - from);
    }
}

// The part of [low, high) in the page [start, end), as a cut, when there is one
static void AddCut(uint32_t cuts[2][2], int *count, uint32_t low, uint32_t high, uint32_t start, uint32_t end) {
    uint32_t from = low > start ? low : start;
    uint32_t to = high < end ? high : end;

    if (from < to) {
        cuts[*count][0] = from;
        cuts[*count][1] = to;
        (*count)++;
    }
}

// One page of a bank, the bytes of the region a capture of 3D holds zeroed. The region wraps around the bank's end, as
// the capture unit's writes do (display3d.c), so it is [0, wrapped) and [offset, its end) in the bank's order
static void PutPage(Encoder *encoder, const uint8_t *page, uint32_t start, uint32_t heldOffset, uint32_t heldSize) {
    uint32_t end = start + PBW_DIGEST_VRAM_PAGE_SIZE;
    uint32_t heldEnd = heldOffset + heldSize;
    uint32_t wrapped = 0;
    uint32_t cuts[2][2];
    uint32_t at = start;
    int count = 0;
    int i;

    if (heldEnd > PBW_DISPLAY_BANK_SIZE) {
        wrapped = heldEnd - PBW_DISPLAY_BANK_SIZE;
        heldEnd = PBW_DISPLAY_BANK_SIZE;
    }
    AddCut(cuts, &count, 0, wrapped, start, end);
    AddCut(cuts, &count, heldOffset, heldEnd, start, end);
    for (i = 0; i < count; i++) {
        PutSpan(encoder, page, start, at, cuts[i][0]);
        PutZeros(encoder, cuts[i][1] - cuts[i][0]);
        at = cuts[i][1];
    }
    PutSpan(encoder, page, start, at, end);
}

// vram.A to vram.I: the bank's bytes, page by page
static PbwDigestStatus EncodeBank(const PbwDigestFrame *frame, int bank, Encoder *encoder) {
    uint32_t heldOffset;
    uint32_t heldSize;
    unsigned page;

    if (HeldRegion(frame, bank, &heldOffset, &heldSize) != 0) {
        return PBW_DIGEST_INVALID;
    }
    for (page = 0; page < sBankPages[bank]; page++) {
        PutPage(encoder, frame->vram.pages[bank][page], page * PBW_DIGEST_VRAM_PAGE_SIZE, heldOffset, heldSize);
    }
    return PBW_DIGEST_OK;
}

// capture: whether the unit ran, and with which DISPCAPCNT, engine A's DISPCNT and destination in the LCDC (all 0
// when it didn't run), then for banks A-D whether each holds 3D, and the region's offset and size (0 when not)
static PbwDigestStatus EncodeCapture(const PbwDigestFrame *frame, Encoder *encoder) {
    const PbwCapture *capture = &frame->capture;
    int bank;

    for (bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        uint32_t offset;
        uint32_t size;
        if (HeldRegion(frame, bank, &offset, &size) != 0) {
            return PBW_DIGEST_INVALID;
        }
    }
    if (capture->ran) {
        PutWord(encoder, 1);
        PutWord(encoder, capture->dispcapcnt & DISPCAPCNT_KEPT);
        PutWord(encoder, capture->dispcntA);
        PutWord(encoder, capture->destInLcdc ? 1 : 0);
    } else {
        PutZeros(encoder, 4 * 4);
    }
    for (bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        uint32_t offset;
        uint32_t size;
        (void)HeldRegion(frame, bank, &offset, &size);
        PutWord(encoder, frame->banks3d.holds3d[bank] ? 1 : 0);
        PutWord(encoder, offset);
        PutWord(encoder, size);
    }
    return PBW_DIGEST_OK;
}

// g3d.list: the polygon count and the SWAP_BUFFERS bits; then each polygon's POLYGON_ATTR, TEXIMAGE_PARAM, PLTT_BASE,
// vertex count and facing, each followed by its vertices, dereferenced: screen X and Y, the depth (0 when W is 0),
// W, red, green, blue, S and T. Checked first, so that a list that breaks its rules encodes nothing
static PbwDigestStatus EncodeList(const PbwDigestList *list, Encoder *encoder) {
    uint32_t i;
    uint32_t j;

    if (list->polygonCount != 0 && list->polygons == NULL) {
        return PBW_DIGEST_INVALID;
    }
    for (i = 0; i < list->polygonCount; i++) {
        const PbwDigestPolygon *polygon = &list->polygons[i];
        if (polygon->vertexCount > PBW_DIGEST_POLYGON_VERTICES) {
            return PBW_DIGEST_INVALID;
        }
        for (j = 0; j < polygon->vertexCount; j++) {
            if (list->vertices == NULL || polygon->vertices[j] >= list->vertexCount) {
                return PBW_DIGEST_INVALID;
            }
        }
    }
    PutWord(encoder, list->polygonCount);
    PutWord(encoder, list->swapParam & 3);
    for (i = 0; i < list->polygonCount; i++) {
        const PbwDigestPolygon *polygon = &list->polygons[i];
        PutWord(encoder, polygon->attr);
        PutWord(encoder, polygon->texParam);
        PutWord(encoder, polygon->plttBase & 0x1fff);
        PutWord(encoder, polygon->vertexCount);
        PutWord(encoder, polygon->front ? 1 : 0);
        for (j = 0; j < polygon->vertexCount; j++) {
            const PbwDigestVertex *vertex = &list->vertices[polygon->vertices[j]];
            uint32_t w = vertex->w & 0xffffff;
            PutWord(encoder, vertex->x & 0x1ff);
            PutWord(encoder, vertex->y & 0xff);
            // W = 0: melonDS gives the vertex the depth 0x7ffe00 and marks the polygon degenerate, the port gives it 0;
            // the hardware draws no such polygon, so the depth is 0 on both sides, and X, Y and W are still compared
            PutWord(encoder, w == 0 ? 0 : vertex->depth & 0xffffff);
            PutWord(encoder, w);
            PutWord(encoder, vertex->color[0]);
            PutWord(encoder, vertex->color[1]);
            PutWord(encoder, vertex->color[2]);
            PutWord(encoder, vertex->s & 0xffff);
            PutWord(encoder, vertex->t & 0xffff);
        }
    }
    return PBW_DIGEST_OK;
}

// screen.top, screen.bottom: each pixel's red, green and blue as the DS's 6 bits, a byte each, row by row
static void EncodeScreen(const uint32_t *pixels, Encoder *encoder) {
    uint8_t row[PBW_DIGEST_SCREEN_WIDTH * 3];
    int x;
    int y;

    for (y = 0; y < PBW_DIGEST_SCREEN_HEIGHT; y++) {
        for (x = 0; x < PBW_DIGEST_SCREEN_WIDTH; x++) {
            uint32_t pixel = pixels[y * PBW_DIGEST_SCREEN_WIDTH + x];
            row[x * 3] = (uint8_t)(pixel >> 18 & 0x3f);
            row[x * 3 + 1] = (uint8_t)(pixel >> 10 & 0x3f);
            row[x * 3 + 2] = (uint8_t)(pixel >> 2 & 0x3f);
        }
        PutBytes(encoder, row, sizeof(row));
    }
}

static PbwDigestStatus Encode(const PbwDigestFrame *frame, int group, Encoder *encoder) {
    uint32_t words[PBW_DIGEST_REGS3D_WORDS];
    int i;

    switch (group) {
    case PBW_DIGEST_VRAM_A:
    case PBW_DIGEST_VRAM_B:
    case PBW_DIGEST_VRAM_C:
    case PBW_DIGEST_VRAM_D:
    case PBW_DIGEST_VRAM_E:
    case PBW_DIGEST_VRAM_F:
    case PBW_DIGEST_VRAM_G:
    case PBW_DIGEST_VRAM_H:
    case PBW_DIGEST_VRAM_I:
        return EncodeBank(frame, group - PBW_DIGEST_VRAM_A, encoder);
    case PBW_DIGEST_VRAM_MAP:
        for (i = 0; i < PBW_DIGEST_VRAM_BANKS; i++) {
            PutWord(encoder, frame->vram.vramcnt[i] & sVramcntKept[i]);
        }
        return PBW_DIGEST_OK;
    case PBW_DIGEST_PAL:
        if (frame->palette == NULL) {
            return PBW_DIGEST_ABSENT;
        }
        PutBytes(encoder, frame->palette, PBW_DIGEST_PALETTE_SIZE);
        return PBW_DIGEST_OK;
    case PBW_DIGEST_OAM:
        if (frame->oam == NULL) {
            return PBW_DIGEST_ABSENT;
        }
        PutBytes(encoder, frame->oam, PBW_DIGEST_OAM_SIZE);
        return PBW_DIGEST_OK;
    case PBW_DIGEST_REGS2D_A:
    case PBW_DIGEST_REGS2D_B: {
        int engine = group == PBW_DIGEST_REGS2D_A ? PBW_DIGEST_ENGINE_A : PBW_DIGEST_ENGINE_B;
        pbw_digest_regs2d_words(&frame->regs2d[engine], engine, words);
        PutWords(encoder, words, PBW_DIGEST_REGS2D_WORDS);
        return PBW_DIGEST_OK;
    }
    case PBW_DIGEST_SYS:
        PutWord(encoder, frame->sys.powcnt1 & POWCNT1_KEPT);
        PutWord(encoder, frame->sys.wramcnt & WRAMCNT_KEPT);
        return PBW_DIGEST_OK;
    case PBW_DIGEST_CAPTURE:
        return EncodeCapture(frame, encoder);
    case PBW_DIGEST_G3D_LIST:
        return EncodeList(&frame->list, encoder);
    case PBW_DIGEST_G3D_REGS:
        pbw_digest_regs3d_words(&frame->regs3d, words);
        PutWords(encoder, words, PBW_DIGEST_REGS3D_WORDS);
        return PBW_DIGEST_OK;
    case PBW_DIGEST_G3D_STATS:
        PutWord(encoder, frame->list.polygonCount);
        PutWord(encoder, frame->list.ramOverflow ? 1 : 0);
        return PBW_DIGEST_OK;
    case PBW_DIGEST_SCREEN_TOP:
    case PBW_DIGEST_SCREEN_BOTTOM: {
        const uint32_t *pixels = frame->screens[group - PBW_DIGEST_SCREEN_TOP];
        if (pixels == NULL) {
            return PBW_DIGEST_ABSENT;
        }
        EncodeScreen(pixels, encoder);
        return PBW_DIGEST_OK;
    }
    default:
        return PBW_DIGEST_INVALID;
    }
}

PbwDigestStatus pbw_digest_group_bytes(const PbwDigestFrame *frame, int group, PbwDigestBytes *bytes) {
    Encoder encoder;
    size_t before = bytes->size;
    PbwDigestStatus status;

    memset(&encoder, 0, sizeof(encoder));
    encoder.bytes = bytes;
    status = Encode(frame, group, &encoder);
    Flush(&encoder);
    if (status == PBW_DIGEST_OK && encoder.noMemory) {
        status = PBW_DIGEST_NO_MEMORY;
    }
    if (status != PBW_DIGEST_OK) {
        bytes->size = before;
    }
    return status;
}

PbwDigestStatus pbw_digest_group_hash(const PbwDigestFrame *frame, int group, uint64_t *hash) {
    XXH3_state_t state;
    Encoder encoder;
    PbwDigestStatus status;

    XXH3_INITSTATE(&state);
    (void)XXH3_64bits_reset(&state);
    memset(&encoder, 0, sizeof(encoder));
    encoder.state = &state;
    status = Encode(frame, group, &encoder);
    Flush(&encoder);
    *hash = status == PBW_DIGEST_OK ? XXH3_64bits_digest(&state) : 0;
    return status;
}

uint64_t pbw_digest_hash(const void *bytes, size_t size) {
    return XXH3_64bits(bytes, size);
}

PbwDigestStatus pbw_digest_record_frame(PbwDigestRecord *record, const PbwDigestFrame *frame, int *group) {
    int g;

    // Every group's hash and presence is set, an absent group's hash to 0
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        PbwDigestStatus status = pbw_digest_group_hash(frame, g, &record->hash[g]);
        record->present[g] = status == PBW_DIGEST_OK;
        if (status != PBW_DIGEST_OK && status != PBW_DIGEST_ABSENT) {
            // A record of part of a frame, the rest absent or left from the record before, would compare as if it were
            // a whole one: it holds none of it instead
            memset(record->present, 0, sizeof(record->present));
            memset(record->hash, 0, sizeof(record->hash));
            if (group != NULL) {
                *group = g;
            }
            return status;
        }
    }
    return PBW_DIGEST_OK;
}

// --- Streams

int pbw_digest_write_header(FILE *file) {
    int g;

    fprintf(file, "#pbwdigest %d\ncount\tframe\tvblanks", PBW_DIGEST_FORMAT);
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        fprintf(file, "\t%s", sGroupNames[g]);
    }
    fputc('\n', file);
    return ferror(file) ? -1 : 0;
}

int pbw_digest_write_record(FILE *file, const PbwDigestRecord *record) {
    int g;

    fprintf(file, "%" PRIu32 "\t%" PRIu32 "\t%" PRIu32, record->count, record->frame, record->vblanks);
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        if (record->present[g]) {
            fprintf(file, "\t%016" PRIx64, record->hash[g]);
        } else {
            fputs("\t-", file);
        }
    }
    fputc('\n', file);
    return ferror(file) ? -1 : 0;
}

// --- The stream's writer

int pbw_digest_writer_open(PbwDigestWriter *writer, FILE *file) {
    memset(writer, 0, sizeof(*writer));
    writer->file = file;
    if (pbw_digest_write_header(file) != 0 || fflush(file) != 0) {
        writer->failed = 1;
        snprintf(writer->error, sizeof(writer->error), "can't write the stream's header");
        return -1;
    }
    return 0;
}

static int WriterFail(PbwDigestWriter *writer) {
    writer->failed = 1;
    return -1;
}

int pbw_digest_writer_vblank(PbwDigestWriter *writer, uint32_t count, uint32_t frame, const PbwDigestFrame *state) {
    PbwDigestRecord record;
    PbwDigestStatus status;
    int group = -1;

    if (writer->failed) {
        return -1;
    }
    if (!writer->started) {
        // The start: a count of 1 at the VBlank after one whose count was 0. Anything before is nothing to record
        int start = writer->seen && writer->previous == 0 && count == 1;
        writer->seen = 1;
        writer->previous = count;
        if (!start) {
            return 0;
        }
    } else if (count != writer->record.count && count != writer->record.count + 1) {
        snprintf(writer->error, sizeof(writer->error),
                 "frame %" PRIu32 ": GFL's frame count is %" PRIu32 " after %" PRIu32
                 ": after the stream's start it only stays or rises by one",
                 frame, count, writer->record.count);
        return WriterFail(writer);
    }
    memset(&record, 0, sizeof(record));
    status = pbw_digest_record_frame(&record, state, &group);
    if (status != PBW_DIGEST_OK) {
        snprintf(writer->error, sizeof(writer->error), "frame %" PRIu32 ", count %" PRIu32 ": %s can't be encoded (%s)",
                 frame, count, pbw_digest_group_name(group) != NULL ? pbw_digest_group_name(group) : "a group",
                 status == PBW_DIGEST_NO_MEMORY ? "out of memory" : "the structure breaks its own rules");
        return WriterFail(writer);
    }
    record.count = count;
    record.frame = frame;
    record.vblanks = 1;
    if (writer->started && count == writer->record.count) {
        // A later VBlank of the pending count: its state is the record's now
        record.vblanks = writer->record.vblanks + 1;
    } else if (writer->started) {
        // The next count: the pending record's count had its last VBlank at the one before
        if (pbw_digest_write_record(writer->file, &writer->record) != 0 || fflush(writer->file) != 0) {
            snprintf(writer->error, sizeof(writer->error), "frame %" PRIu32 ": can't write count %" PRIu32 "'s record",
                     frame, writer->record.count);
            return WriterFail(writer);
        }
    }
    writer->started = 1;
    writer->record = record;
    return 0;
}

int pbw_digest_writer_close(PbwDigestWriter *writer) {
    int failed = writer->failed;

    // The pending record isn't written: its count may have had VBlanks still to come
    if (writer->file != NULL && (fflush(writer->file) != 0 || ferror(writer->file))) {
        if (!failed) {
            snprintf(writer->error, sizeof(writer->error), "can't write the stream");
        }
        failed = 1;
    }
    writer->file = NULL;
    writer->failed = failed;
    return failed ? -1 : 0;
}

// The longest line a reader takes, and the most fields a line has
#define LINE_SIZE 4096
#define FIELDS (3 + PBW_DIGEST_GROUPS)

// The stream's next line without its end ("\n" or "\r\n"): 1 with one, 0 at the end, -1 with the reader's error set
static int ReadLine(PbwDigestReader *reader, char *line) {
    size_t length;

    if (fgets(line, LINE_SIZE, reader->file) == NULL) {
        if (ferror(reader->file)) {
            snprintf(reader->error, sizeof(reader->error), "line %u: can't read the stream", reader->line + 1);
            return -1;
        }
        return 0;
    }
    reader->line++;
    length = strlen(line);
    if (length > 0 && line[length - 1] == '\n') {
        line[--length] = '\0';
    } else if (!feof(reader->file)) {
        snprintf(reader->error, sizeof(reader->error), "line %u: longer than %d characters", reader->line,
                 LINE_SIZE - 2);
        return -1;
    }
    if (length > 0 && line[length - 1] == '\r') {
        line[--length] = '\0';
    }
    return 1;
}

// Splits a line at its tabs, in place: the number of fields, or -1 for more than FIELDS
static int Split(char *line, char *fields[FIELDS]) {
    int count = 0;
    char *at = line;

    for (;;) {
        char *tab = strchr(at, '\t');
        if (count == FIELDS) {
            return -1;
        }
        fields[count++] = at;
        if (tab == NULL) {
            return count;
        }
        *tab = '\0';
        at = tab + 1;
    }
}

// A decimal count, frame or number of VBlanks: digits only, up to 0xffffffff
static int ParseNumber(const char *text, uint32_t *value) {
    uint64_t number = 0;

    if (*text == '\0') {
        return 0;
    }
    for (; *text != '\0'; text++) {
        if (*text < '0' || *text > '9') {
            return 0;
        }
        number = number * 10 + (uint64_t)(*text - '0');
        if (number > 0xffffffffu) {
            return 0;
        }
    }
    *value = (uint32_t)number;
    return 1;
}

// A group's hash: 16 hex digits
static int ParseHash(const char *text, uint64_t *value) {
    uint64_t hash = 0;
    int i;

    for (i = 0; i < 16; i++) {
        char c = text[i];
        unsigned digit;
        if (c >= '0' && c <= '9') {
            digit = (unsigned)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (unsigned)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = (unsigned)(c - 'A' + 10);
        } else {
            return 0;
        }
        hash = hash << 4 | digit;
    }
    if (text[16] != '\0') {
        return 0;
    }
    *value = hash;
    return 1;
}

int pbw_digest_reader_open(PbwDigestReader *reader, FILE *file) {
    static const char *const keys[3] = { "count", "frame", "vblanks" };
    char line[LINE_SIZE];
    char version[16];
    char *fields[FIELDS];
    uint8_t named[PBW_DIGEST_GROUPS] = { 0 };
    int count;
    int got;
    int i;

    memset(reader, 0, sizeof(*reader));
    reader->file = file;
    got = ReadLine(reader, line);
    if (got <= 0) {
        if (got == 0) {
            snprintf(reader->error, sizeof(reader->error), "empty: no \"#pbwdigest %d\" line", PBW_DIGEST_FORMAT);
        }
        return -1;
    }
    if (strncmp(line, "#pbwdigest ", 11) != 0) {
        snprintf(reader->error, sizeof(reader->error), "line 1: not a digest stream: no \"#pbwdigest %d\" line",
                 PBW_DIGEST_FORMAT);
        return -1;
    }
    // The version as pbw_digest_write_header writes it, digits alone
    snprintf(version, sizeof(version), "%d", PBW_DIGEST_FORMAT);
    if (strcmp(line + 11, version) != 0) {
        snprintf(reader->error, sizeof(reader->error), "line 1: version \"%.40s\", where this reader knows %d",
                 line + 11, PBW_DIGEST_FORMAT);
        return -1;
    }
    got = ReadLine(reader, line);
    if (got <= 0) {
        if (got == 0) {
            snprintf(reader->error, sizeof(reader->error), "line 2: no header");
        }
        return -1;
    }
    count = Split(line, fields);
    if (count < 3) {
        snprintf(reader->error, sizeof(reader->error), "line 2: the header has %s fields",
                 count < 0 ? "too many" : "too few");
        return -1;
    }
    for (i = 0; i < 3; i++) {
        if (strcmp(fields[i], keys[i]) != 0) {
            snprintf(reader->error, sizeof(reader->error), "line 2: column %d is \"%.40s\", not \"%s\"", i + 1,
                     fields[i], keys[i]);
            return -1;
        }
    }
    for (i = 3; i < count; i++) {
        int group = pbw_digest_group_find(fields[i]);
        if (group < 0) {
            snprintf(reader->error, sizeof(reader->error), "line 2: \"%.40s\" is no group this reader knows",
                     fields[i]);
            return -1;
        }
        if (named[group]) {
            snprintf(reader->error, sizeof(reader->error), "line 2: group %s named twice", sGroupNames[group]);
            return -1;
        }
        named[group] = 1;
        reader->columnGroup[reader->groupColumns++] = (int16_t)group;
    }
    return 0;
}

int pbw_digest_reader_next(PbwDigestReader *reader, PbwDigestRecord *record) {
    char line[LINE_SIZE];
    char *fields[FIELDS];
    int count;
    int got;
    int i;

    got = ReadLine(reader, line);
    if (got <= 0) {
        return got;
    }
    count = Split(line, fields);
    if (count != 3 + reader->groupColumns) {
        snprintf(reader->error, sizeof(reader->error), "line %u: %d fields where the header has %d", reader->line,
                 count, 3 + reader->groupColumns);
        return -1;
    }
    memset(record, 0, sizeof(*record));
    if (!ParseNumber(fields[0], &record->count) || !ParseNumber(fields[1], &record->frame) ||
        !ParseNumber(fields[2], &record->vblanks)) {
        snprintf(reader->error, sizeof(reader->error), "line %u: count, frame and vblanks must be decimal numbers",
                 reader->line);
        return -1;
    }
    for (i = 0; i < reader->groupColumns; i++) {
        int group = reader->columnGroup[i];
        const char *text = fields[3 + i];
        if (strcmp(text, "-") == 0) {
            continue;
        }
        if (!ParseHash(text, &record->hash[group])) {
            snprintf(reader->error, sizeof(reader->error), "line %u: %s is \"%.40s\", not 16 hex digits or \"-\"",
                     reader->line, sGroupNames[group], text);
            return -1;
        }
        record->present[group] = 1;
    }
    return 1;
}

int pbw_digest_record_differences(const PbwDigestRecord *a, const PbwDigestRecord *b,
                                  uint8_t differs[PBW_DIGEST_GROUPS]) {
    int count = 0;
    int g;

    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        int differ = a->present[g] && b->present[g] && a->hash[g] != b->hash[g];
        if (differs != NULL) {
            differs[g] = (uint8_t)differ;
        }
        count += differ;
    }
    return count;
}

int pbw_digest_record_missing(const PbwDigestRecord *expected, const PbwDigestRecord *actual,
                              uint8_t missing[PBW_DIGEST_GROUPS]) {
    int count = 0;
    int g;

    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        int lacks = expected->present[g] && !actual->present[g];
        if (missing != NULL) {
            missing[g] = (uint8_t)lacks;
        }
        count += lacks;
    }
    return count;
}

// One stream of a comparison: its reader, its current record, and the last count, which must go up
typedef struct Side {
    const char *name;
    PbwDigestReader reader;
    PbwDigestRecord record;
    int have;
    int started;
    uint32_t last;
} Side;

static int Advance(Side *side, PbwDigestComparison *comparison) {
    int got = pbw_digest_reader_next(&side->reader, &side->record);

    if (got < 0) {
        snprintf(comparison->error, sizeof(comparison->error), "%s stream: %s", side->name, side->reader.error);
        return -1;
    }
    side->have = got == 1;
    if (side->have) {
        if (side->started && side->record.count <= side->last) {
            snprintf(comparison->error, sizeof(comparison->error),
                     "%s stream: line %u: count %" PRIu32 " after count %" PRIu32
                     ": a stream has one row per count, in rising order",
                     side->name, side->reader.line, side->record.count, side->last);
            return -1;
        }
        side->started = 1;
        side->last = side->record.count;
    }
    return 0;
}

// Compares one count's two records: 1 when they diverge (a group's hashes differ, or the actual record lacks a group
// the expected one has), the groups both have counted as compared either way
static int CompareRecords(const PbwDigestRecord *expected, const PbwDigestRecord *actual,
                          PbwDigestComparison *comparison) {
    int differ = pbw_digest_record_differences(expected, actual, comparison->differs);
    int lack = pbw_digest_record_missing(expected, actual, comparison->missing);
    int g;

    comparison->compared++;
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        comparison->groupCompared[g] += expected->present[g] && actual->present[g];
    }
    return differ != 0 || lack != 0;
}

int pbw_digest_compare_streams(FILE *expected, FILE *actual, PbwDigestComparison *comparison) {
    Side sides[2];
    Side *e = &sides[0];
    Side *a = &sides[1];
    int i;

    memset(comparison, 0, sizeof(*comparison));
    memset(sides, 0, sizeof(sides));
    e->name = "expected";
    a->name = "actual";
    for (i = 0; i < 2; i++) {
        if (pbw_digest_reader_open(&sides[i].reader, i == 0 ? expected : actual) != 0) {
            snprintf(comparison->error, sizeof(comparison->error), "%s stream: %s", sides[i].name,
                     sides[i].reader.error);
            return -1;
        }
        if (Advance(&sides[i], comparison) != 0) {
            return -1;
        }
    }
    while (e->have && a->have) {
        if (e->record.count < a->record.count) {
            comparison->onlyExpected++;
            if (Advance(e, comparison) != 0) {
                return -1;
            }
        } else if (a->record.count < e->record.count) {
            comparison->onlyActual++;
            if (Advance(a, comparison) != 0) {
                return -1;
            }
        } else {
            if (CompareRecords(&e->record, &a->record, comparison)) {
                comparison->diverged = 1;
                comparison->expected = e->record;
                comparison->actual = a->record;
                return 0;
            }
            if (Advance(e, comparison) != 0 || Advance(a, comparison) != 0) {
                return -1;
            }
        }
    }
    while (e->have) {
        comparison->onlyExpected++;
        if (Advance(e, comparison) != 0) {
            return -1;
        }
    }
    while (a->have) {
        comparison->onlyActual++;
        if (Advance(a, comparison) != 0) {
            return -1;
        }
    }
    return 0;
}
