// The canonical per-game-frame digest that verification v2 holds Frigate to melonDS with (ADR 0031,
// port/docs/adr/0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md): one record per game frame,
// with a 64-bit hash for each group of the DS's state that the screens are made from (VRAM, palettes, OAM, the 2D and
// 3D registers, display capture, the 3D render list) and of the screens themselves. Each group is first put into
// canonical bytes, so that the port's record and the oracle's are equal exactly when the state is.
//
// Each program fills a PbwDigestFrame from its own model, and this file does the rest: the decodes and masks that make
// the two models' registers comparable (melonDS 1.1 keeps some registers decoded and drops the bits the hardware
// ignores; the port keeps the bytes the game wrote), the 3D list's vertices dereferenced and its depth made canonical,
// the VRAM that holds a capture of 3D zeroed, the encoding, the hash (XXH3-64 from port/third_party/xxhash) and the
// stream's format, "#pbwdigest 1" (port/tools/oracle/README.md, "Digests"). The groups that need probes of the game's
// state (the random state, NNS G3D's globals, the save, the player, the procs, the sound driver) come with their
// probes. The facts are melonDS 1.1's (the oracle's emulation core: GPU2D.cpp, GPU3D.cpp, GPU.cpp, NDS.cpp) and
// GBATEK's ("DS Video", "DS 3D Video", "DS Memory Control"), cited in digest.c.
//
// Plain C11, with neither melonDS nor the decomp's headers in it, as pbwinput.c and display3d.c are, so that both
// programs can compile it and its tests run without a ROM (port/tests/oracle/test_digest.c). The game functionality
// it touches is every screen's: what the game leaves in the display hardware at each VBlank.
#ifndef PORT_TOOLS_ORACLE_DIGEST_H
#define PORT_TOOLS_ORACLE_DIGEST_H

#include "display3d.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// The stream's version, in its first line: "#pbwdigest 1"
#define PBW_DIGEST_FORMAT 1

// The groups of a record, in the order of the stream's columns. Each one's canonical bytes are in README.md, "Digests"
typedef enum PbwDigestGroup {
    PBW_DIGEST_VRAM_A,        // vram.A to vram.I: each VRAM bank's bytes
    PBW_DIGEST_VRAM_B,        // vram.B
    PBW_DIGEST_VRAM_C,        // vram.C
    PBW_DIGEST_VRAM_D,        // vram.D
    PBW_DIGEST_VRAM_E,        // vram.E
    PBW_DIGEST_VRAM_F,        // vram.F
    PBW_DIGEST_VRAM_G,        // vram.G
    PBW_DIGEST_VRAM_H,        // vram.H
    PBW_DIGEST_VRAM_I,        // vram.I
    PBW_DIGEST_VRAM_MAP,      // vram.map: VRAMCNT_A to VRAMCNT_I
    PBW_DIGEST_PAL,           // pal: the standard palettes
    PBW_DIGEST_OAM,           // oam: both engines' OAM
    PBW_DIGEST_REGS2D_A,      // regs2d.A: engine A's 2D registers
    PBW_DIGEST_REGS2D_B,      // regs2d.B: engine B's
    PBW_DIGEST_SYS,           // sys: POWCNT1 and WRAMCNT
    PBW_DIGEST_CAPTURE,       // capture: the frame's display capture and the banks that hold a capture of 3D
    PBW_DIGEST_G3D_LIST,      // g3d.list: the 3D render list
    PBW_DIGEST_G3D_REGS,      // g3d.regs: the 3D rendering registers
    PBW_DIGEST_G3D_STATS,     // g3d.stats: the list's polygon count and polygon RAM's overflow flag
    PBW_DIGEST_SCREEN_TOP,    // screen.top: the top screen's pixels
    PBW_DIGEST_SCREEN_BOTTOM, // screen.bottom
    PBW_DIGEST_GROUPS
} PbwDigestGroup;

// A group's name in the stream's header ("vram.A" ... "screen.bottom"), or NULL for a number that isn't a group
const char *pbw_digest_group_name(int group);
// The group a name names, or -1
int pbw_digest_group_find(const char *name);

// What encoding a group gave
typedef enum PbwDigestStatus {
    PBW_DIGEST_OK,
    PBW_DIGEST_ABSENT,   // the program doesn't have the group's state: a NULL palette, OAM or screen ("-" in a stream)
    PBW_DIGEST_INVALID,  // the structure breaks its own rules: a vertex index past the list's vertices, a polygon of
                         // more than 10 vertices, a capture's region outside its bank
    PBW_DIGEST_NO_MEMORY // a PbwDigestBytes couldn't grow
} PbwDigestStatus;

// --- VRAM (vram.A to vram.I, vram.map)

// Nine banks, A to I, in pages of 16 KiB: A to D have 8 pages (128 KiB), E 4, F and G 1, H 2 and I 1, 41 in all
#define PBW_DIGEST_VRAM_BANKS 9
#define PBW_DIGEST_VRAM_PAGE_SIZE 0x4000u
#define PBW_DIGEST_VRAM_BANK_PAGES 8 // the most pages a bank has
// The pages of a bank, 0 for a number that isn't one
unsigned pbw_digest_vram_pages(int bank);

// The banks' bytes, each bank page by page wherever the program keeps it: the port's port_vram_bank_page(bank, page)
// (port/hal/gx/vram.h), melonDS's GPU::VRAM[bank] + page * 16 KiB. A NULL page reads as zeros. The VRAMCNT registers
// as each program keeps them: the port's port_vram_vramcnt, melonDS's GPU::VRAMCNT, which its writes have masked
typedef struct PbwDigestVram {
    const uint8_t *pages[PBW_DIGEST_VRAM_BANKS][PBW_DIGEST_VRAM_BANK_PAGES];
    uint8_t vramcnt[PBW_DIGEST_VRAM_BANKS];
} PbwDigestVram;

// --- 2D registers (regs2d.A, regs2d.B)

#define PBW_DIGEST_ENGINE_A 0 // its registers at 0x04000000
#define PBW_DIGEST_ENGINE_B 1 // at 0x04001000
// The bytes of an engine's register file that pbw_digest_regs2d_from_io reads: offsets 0x00 to 0x6f
#define PBW_DIGEST_REGS2D_IO_SIZE 0x70u

// One 2D engine's registers in the form melonDS 1.1 keeps them: GPU2D::Unit's fields, written as GPU2D.cpp's Write16
// and Write32 decode the game's stores, the signed ones as their bits. melonDS copies its unit's fields here; the port
// decodes the bytes the game wrote with pbw_digest_regs2d_from_io. The encoding then keeps what the hardware keeps:
// each field's mask, BGxX and BGxY's 28 bits sign-extended from bit 27, BLDY clamped to 16
typedef struct PbwDigestRegs2d {
    uint32_t dispcnt;      // DispCnt: DISPCNT (engine B keeps 0xc0b1fff7 of it)
    uint16_t bgcnt[4];     // BGCnt: BG0CNT to BG3CNT
    uint16_t bgHofs[4];    // BGXPos: BG0HOFS to BG3HOFS (9 bits act)
    uint16_t bgVofs[4];    // BGYPos: BG0VOFS to BG3VOFS (9 bits act)
    uint16_t bgPa[2];      // BGRotA: BG2PA, then BG3PA
    uint16_t bgPb[2];      // BGRotB: BG2PB, BG3PB
    uint16_t bgPc[2];      // BGRotC: BG2PC, BG3PC
    uint16_t bgPd[2];      // BGRotD: BG2PD, BG3PD
    uint32_t bgX[2];       // BGXRef: BG2X, BG3X (28 bits act; melonDS sets bits 28-31 when bit 27 is set)
    uint32_t bgY[2];       // BGYRef: BG2Y, BG3Y (the same)
    uint8_t win0[4];       // Win0Coords: WIN0H's X1 (its high byte) and X2 (its low byte), WIN0V's Y1 and Y2
    uint8_t win1[4];       // Win1Coords: WIN1H and WIN1V, split the same way
    uint8_t winCnt[4];     // WinCnt: WININ's window 0 and window 1 bytes, WINOUT's outside and OBJ window bytes
    uint8_t bgMosaic[2];   // BGMosaicSize: MOSAIC's BG width and height (bits 0-3 and 4-7)
    uint8_t objMosaic[2];  // OBJMosaicSize: MOSAIC's OBJ width and height (bits 8-11 and 12-15)
    uint16_t bldcnt;       // BlendCnt: BLDCNT
    uint16_t bldalpha;     // BlendAlpha: BLDALPHA
    uint8_t bldy;          // EVY: BLDY's 5 bits clamped to 16, the only form melonDS keeps
    uint16_t masterBright; // MasterBrightness: MASTER_BRIGHT
    uint32_t dispcapcnt;   // CaptureCnt: DISPCAPCNT, engine A's alone (engine B has none: 0)
} PbwDigestRegs2d;

// Fills regs from an engine's register file as the game wrote it, PBW_DIGEST_REGS2D_IO_SIZE bytes from 0x04000000
// (engine A) or 0x04001000 (engine B), little-endian as the DS's, decoded as melonDS's writes store each register
void pbw_digest_regs2d_from_io(PbwDigestRegs2d *regs, const uint8_t *io, int engine);

// An engine's canonical words: DISPCNT; BG0CNT to BG3CNT; BG0HOFS, BG0VOFS to BG3HOFS, BG3VOFS; BG2PA, BG2PB, BG2PC,
// BG2PD, BG2X, BG2Y, then BG3's six; WIN0's X1, X2, Y1, Y2, then WIN1's; WININ's two bytes and WINOUT's two; MOSAIC's
// BG width and height and OBJ width and height; BLDCNT; BLDALPHA; BLDY; MASTER_BRIGHT; DISPCAPCNT
#define PBW_DIGEST_REGS2D_WORDS 46
void pbw_digest_regs2d_words(const PbwDigestRegs2d *regs, int engine, uint32_t words[PBW_DIGEST_REGS2D_WORDS]);

// --- System registers (sys)

// POWCNT1 and WRAMCNT, as melonDS keeps them (NDS::PowerControl9, NDS::WRAMCnt) or from the port's register bytes
typedef struct PbwDigestSys {
    uint16_t powcnt1; // POWCNT1, 0x04000304, the ARM9's
    uint8_t wramcnt;  // WRAMCNT, 0x04000247
} PbwDigestSys;

// The bytes from 0x04000000 that pbw_digest_sys_from_io and pbw_digest_regs3d_from_io read: 0x000 to 0x3bf
#define PBW_DIGEST_IO_SIZE 0x3c0u
// Fills sys from the I/O registers as the game wrote them, from 0x04000000
void pbw_digest_sys_from_io(PbwDigestSys *sys, const uint8_t *io);

// --- The 3D render list (g3d.list, g3d.stats)

// A polygon has at most 10 vertices: a quad clipped by the six planes of the view volume
#define PBW_DIGEST_POLYGON_VERTICES 10

// One vertex of polygon RAM after clipping and the viewport, as both programs keep it: the port's PortGeVertex
// (port/gfx/ge/ge.h), melonDS's Vertex with its depth computed here (pbw_digest_zbuffer_depth)
typedef struct PbwDigestVertex {
    uint32_t x;        // screen X: PortGeVertex.x, Vertex::FinalPosition[0] (9 bits)
    uint32_t y;        // screen Y: .y, FinalPosition[1] (8 bits)
    uint32_t depth;    // the Z-buffer depth: .z; for melonDS pbw_digest_zbuffer_depth(Position[2], Position[3]), which
                       // stays the Z-buffer's when the list W-buffers. 24 bits; the encoding makes it 0 when W is 0
    uint32_t w;        // W: .w, Position[3] (24 bits)
    uint32_t color[3]; // red, green, blue as c * 16 + 15, 0 staying 0: .color, FinalColor
    uint32_t s, t;     // the texture coordinates: .s and .t, TexCoords (16 bits each)
} PbwDigestVertex;

// One polygon of polygon RAM: its attributes as latched and its vertices as indices into the list's vertices, which
// strips share between polygons in either program's way; the encoding copies each polygon's own
typedef struct PbwDigestPolygon {
    uint32_t attr;        // POLYGON_ATTR: PortGePolygon.attr, Polygon::Attr
    uint32_t texParam;    // TEXIMAGE_PARAM: .tex_param, TexParam
    uint32_t plttBase;    // PLTT_BASE: .pltt_base, TexPalette (13 bits)
    uint32_t vertexCount; // .vertex_count, NumVertices
    uint32_t front;       // nonzero when it faces the viewer: .flags & PORT_GE_POLYGON_FRONT, FacingView
    uint32_t vertices[PBW_DIGEST_POLYGON_VERTICES]; // .vertices; melonDS's Vertices[] pointers as indices
} PbwDigestPolygon;

// The render list the VBlank swapped in, which the next frame draws, in the order its polygons came: the port's
// port_ge_render_list (its renderer sorts them), melonDS's GPU3D::PolygonRAM bank that the swap closed (its
// RenderPolygonRAM holds them sorted). Polygons and vertices may be NULL when their counts are 0
typedef struct PbwDigestList {
    const PbwDigestPolygon *polygons;
    uint32_t polygonCount; // PortGeList.polygon_count, RenderNumPolygons
    const PbwDigestVertex *vertices;
    uint32_t vertexCount; // the entries of vertices, for the indices' bounds
    uint32_t swapParam;   // the parameter of the SWAP_BUFFERS that closed the list (2 bits): .swap_param,
                          // FlushAttributes
    uint32_t ramOverflow; // nonzero when polygon or vertex RAM overflowed (DISP3DCNT bit 13): port_ge_ram_overflow,
                          // GPU3D::DispCnt bit 13
} PbwDigestList;

// The Z-buffer depth melonDS 1.1 gives a vertex from its clip-space Z and its W in 24 bits (GPU3D.cpp,
// SubmitPolygon): ((Z * 0x4000 / W) + 0x3fff) * 0x200, cut to 32 bits, clamped to 0..0xffffff; 0x7ffe00 when W is 0
uint32_t pbw_digest_zbuffer_depth(int32_t clipZ, uint32_t w);

// --- 3D rendering registers (g3d.regs)

// The 3D rendering registers in the form melonDS 1.1 keeps them (GPU3D's DispCnt, AlphaRefVal, ClearAttr1, ClearAttr2
// as its two halves, FogColor, FogOffset, FogDensityTable, ToonTable, EdgeTable). The port fills it with
// pbw_digest_regs3d_from_io from its register bytes, which port_g3d_vblank latches at the VBlank
// (port/gfx/ge/ge_hal.c). The encoding keeps each register's bits that act
typedef struct PbwDigestRegs3d {
    uint32_t disp3dcnt;     // DISP3DCNT, 0x04000060 (bits 12 and 13 are status: the overflow is in g3d.stats)
    uint8_t alphaTestRef;   // ALPHA_TEST_REF, 0x04000340 (5 bits)
    uint32_t clearColor;    // CLEAR_COLOR, 0x04000350
    uint16_t clearDepth;    // CLEAR_DEPTH, 0x04000354 (15 bits)
    uint16_t clearOffset;   // CLRIMAGE_OFFSET, 0x04000356
    uint32_t fogColor;      // FOG_COLOR, 0x04000358
    uint16_t fogOffset;     // FOG_OFFSET, 0x0400035c (15 bits)
    uint8_t fogTable[32];   // FOG_TABLE, 0x04000360 (7 bits each)
    uint16_t toonTable[32]; // TOON_TABLE, 0x04000380 (15 bits each)
    uint16_t edgeColor[8];  // EDGE_COLOR, 0x04000330 (15 bits each)
} PbwDigestRegs3d;

// Fills regs from the I/O registers as the game wrote them, from 0x04000000
void pbw_digest_regs3d_from_io(PbwDigestRegs3d *regs, const uint8_t *io);

// The canonical words: DISP3DCNT, ALPHA_TEST_REF, CLEAR_COLOR, CLEAR_DEPTH, CLRIMAGE_OFFSET, FOG_COLOR, FOG_OFFSET,
// FOG_TABLE's 32 entries, TOON_TABLE's 32, EDGE_COLOR's 8
#define PBW_DIGEST_REGS3D_WORDS 79
void pbw_digest_regs3d_words(const PbwDigestRegs3d *regs, uint32_t words[PBW_DIGEST_REGS3D_WORDS]);

// --- A frame

// The screens' pixels: 256 by 192, 0xAARRGGBB, each channel the DS's 6 bits widened to 8 as (c << 2) | (c >> 4), as
// both programs' frames.tsv hashes them (port/app/run_dir.c, oracle.cpp's WriteFrame)
#define PBW_DIGEST_SCREEN_WIDTH 256
#define PBW_DIGEST_SCREEN_HEIGHT 192

// The standard palettes (0x05000000) and OAM (0x07000000): 2 KiB each, engine A's first, then engine B's
#define PBW_DIGEST_PALETTE_SIZE 0x800u
#define PBW_DIGEST_OAM_SIZE 0x800u

// What one program has at the VBlank that ends a game frame. Its pointers are the caller's, and are only read. A NULL
// palette, OAM or screen is a group the program doesn't have, "-" in its stream
typedef struct PbwDigestFrame {
    PbwDigestVram vram;        // vram.A to vram.I, vram.map
    const uint8_t *palette;    // pal
    const uint8_t *oam;        // oam
    PbwDigestRegs2d regs2d[2]; // regs2d.A, regs2d.B: by PBW_DIGEST_ENGINE_A and PBW_DIGEST_ENGINE_B
    PbwDigestSys sys;          // sys
    PbwCapture capture;        // capture: this frame's (display3d.h)
    // capture, and vram.A to vram.D: what banks A-D hold of 3D after this frame's capture (display3d.h). A region that
    // holds 3D is zeroed in its bank's bytes on both sides: it holds rasterized pixels, which the 3D groups stand for
    PbwBanks3d banks3d;
    PbwDigestList list;         // g3d.list, g3d.stats
    PbwDigestRegs3d regs3d;     // g3d.regs
    const uint32_t *screens[2]; // screen.top, then screen.bottom
} PbwDigestFrame;

// A growing buffer of canonical bytes, for the full dumps and the tests. Starts zeroed; pbw_digest_bytes_free frees it
typedef struct PbwDigestBytes {
    uint8_t *data;
    size_t size;
    size_t capacity;
} PbwDigestBytes;
void pbw_digest_bytes_free(PbwDigestBytes *bytes);

// Appends a group's canonical bytes to bytes. PBW_DIGEST_ABSENT and PBW_DIGEST_INVALID append nothing
PbwDigestStatus pbw_digest_group_bytes(const PbwDigestFrame *frame, int group, PbwDigestBytes *bytes);
// A group's hash, the hash of its canonical bytes
PbwDigestStatus pbw_digest_group_hash(const PbwDigestFrame *frame, int group, uint64_t *hash);
// The hash of canonical bytes: XXH3-64 with seed 0, so that xxHash's own tools (xxhsum -H3) give it too
uint64_t pbw_digest_hash(const void *bytes, size_t size);

// --- Records and streams

// One game frame's record
typedef struct PbwDigestRecord {
    uint32_t count;                     // GFL's frame count, the game frame: the key two streams are paired on
    uint32_t frame;                     // the program's video frame
    uint32_t vblanks;                   // how many VBlanks had this count
    uint8_t present[PBW_DIGEST_GROUPS]; // 1 for each group taken; one that wasn't is "-" in a stream
    uint64_t hash[PBW_DIGEST_GROUPS];
} PbwDigestRecord;

// Hashes every group of a frame into record's hashes and sets which are present; the caller sets count, frame and
// vblanks. Returns PBW_DIGEST_OK, or the status of the first group that couldn't be encoded, whose number goes in
// *group when group isn't NULL
PbwDigestStatus pbw_digest_record_frame(PbwDigestRecord *record, const PbwDigestFrame *frame, int *group);

// A stream (digest.tsv): "#pbwdigest 1", a header of tab-separated names (count, frame, vblanks, then the groups'),
// then one row per game frame: the three numbers in decimal, then each group's hash as 16 lowercase hex digits, or
// "-" for a group that wasn't taken. Each returns 0, or -1 when the file couldn't be written
int pbw_digest_write_header(FILE *file);
int pbw_digest_write_record(FILE *file, const PbwDigestRecord *record);

// Reads a stream. A header may name the groups in any order and leave some out: those are absent from its records
typedef struct PbwDigestReader {
    FILE *file;
    unsigned line;                          // the stream's last line read
    int groupColumns;                       // how many groups the header names
    int16_t columnGroup[PBW_DIGEST_GROUPS]; // the group of each, in the header's order
    char error[200];                        // why the last call failed
} PbwDigestReader;

// Reads the version line and the header. Returns 0, or -1 with reader->error saying why
int pbw_digest_reader_open(PbwDigestReader *reader, FILE *file);
// The next record: 1 with one, 0 at the end of the stream, -1 with reader->error saying why
int pbw_digest_reader_next(PbwDigestReader *reader, PbwDigestRecord *record);

// How many groups' hashes differ between two records, among the groups both have; differs, when it isn't NULL, gets 1
// for each such group and 0 for the others
int pbw_digest_record_differences(const PbwDigestRecord *a, const PbwDigestRecord *b,
                                  uint8_t differs[PBW_DIGEST_GROUPS]);

// Two streams' comparison, by count
typedef struct PbwDigestComparison {
    uint32_t compared;     // the counts both streams have, compared
    uint32_t onlyExpected; // the counts only the expected stream has, up to the first divergence
    uint32_t onlyActual;   // the counts only the actual stream has, the same
    int diverged;          // 1 when a compared count's groups differed: the first such count's records follow
    uint8_t differs[PBW_DIGEST_GROUPS]; // 1 for each group that differs there
    PbwDigestRecord expected;
    PbwDigestRecord actual;
    char error[240]; // why the comparison failed
} PbwDigestComparison;

// Compares two streams count by count, each stream with one row per count in rising order, and stops at the first
// count whose groups differ. A count that only one stream has is counted, not compared; neither is the video frame
// nor the VBlanks. Returns 0 when both streams could be read that far, diverged saying whether they differ, or -1
// with error saying which stream is malformed and where. Pairing a count's rows when a program writes several, and
// the sequences' lags, known differences and clock stretches, are the pairing's (ADR 0031), not this function's
int pbw_digest_compare_streams(FILE *expected, FILE *actual, PbwDigestComparison *comparison);

#ifdef __cplusplus
}
#endif

#endif // PORT_TOOLS_ORACLE_DIGEST_H
