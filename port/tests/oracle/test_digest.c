// Tests of the canonical per-game-frame digest (port/tools/oracle/digest.c) on made-up states in both programs'
// shapes: melonDS 1.1's, its registers as GPU2D.cpp, GPU3D.cpp, GPU.cpp and NDS.cpp store the game's writes (worked
// out by hand from those files), and the port's, the bytes the game wrote and the render list as port/gfx/ge/ge.c
// builds it. Both shapes must give the same canonical words; the masks must hold; BG2's and BG3's X and Y must be
// sign-extended from bit 27, BLDY clamped and WINxH split; the depth must be canonical where W is 0; strips' shared
// vertices must be dereferenced; and the VRAM that holds a capture of 3D must be zeroed. Each of those checks also
// checks that its input gives other words without the rule, so that a digest without it fails. Every field of every
// group counts: the render list's words one by one and the bits each keeps, the list's size and overflow, the
// capture's words, both engines' palettes and OAM. Then the hash against xxHash's own known answers, and the stream's
// writer, reader and comparison, which stops where a group differs or where the actual stream lacks a group the
// expected one holds. The game functionality it covers is every screen's, the state the display hardware holds at a
// VBlank, with no ROM and no game code.
#include "digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sFailed;
static int sChecks;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        sChecks++;                                                                                                     \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            sFailed++;                                                                                                 \
        }                                                                                                              \
    } while (0)

// Where words differ, the first word that does, printed; 1 when all are equal
static int SameWords(const char *what, const uint32_t *a, const uint32_t *b, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (a[i] != b[i]) {
            fprintf(stderr, "%s: word %d is 0x%08x, not 0x%08x\n", what, i, (unsigned)a[i], (unsigned)b[i]);
            return 0;
        }
    }
    return 1;
}

// Whether the port's decode of its register bytes is melonDS's unit field for field
static int SameRegs2d(const PbwDigestRegs2d *a, const PbwDigestRegs2d *b) {
    return a->dispcnt == b->dispcnt && memcmp(a->bgcnt, b->bgcnt, sizeof(a->bgcnt)) == 0 &&
           memcmp(a->bgHofs, b->bgHofs, sizeof(a->bgHofs)) == 0 &&
           memcmp(a->bgVofs, b->bgVofs, sizeof(a->bgVofs)) == 0 && memcmp(a->bgPa, b->bgPa, sizeof(a->bgPa)) == 0 &&
           memcmp(a->bgPb, b->bgPb, sizeof(a->bgPb)) == 0 && memcmp(a->bgPc, b->bgPc, sizeof(a->bgPc)) == 0 &&
           memcmp(a->bgPd, b->bgPd, sizeof(a->bgPd)) == 0 && memcmp(a->bgX, b->bgX, sizeof(a->bgX)) == 0 &&
           memcmp(a->bgY, b->bgY, sizeof(a->bgY)) == 0 && memcmp(a->win0, b->win0, sizeof(a->win0)) == 0 &&
           memcmp(a->win1, b->win1, sizeof(a->win1)) == 0 && memcmp(a->winCnt, b->winCnt, sizeof(a->winCnt)) == 0 &&
           memcmp(a->bgMosaic, b->bgMosaic, sizeof(a->bgMosaic)) == 0 &&
           memcmp(a->objMosaic, b->objMosaic, sizeof(a->objMosaic)) == 0 && a->bldcnt == b->bldcnt &&
           a->bldalpha == b->bldalpha && a->bldy == b->bldy && a->masterBright == b->masterBright &&
           a->dispcapcnt == b->dispcapcnt;
}

// The I/O registers as the port keeps them, the bytes the game wrote, from 0x04000000: engine A's 2D registers, the
// 3D rendering registers, POWCNT1 and WRAMCNT below 0x400, engine B's 2D registers at 0x1000
#define IO_BASE 0x04000000u
static uint8_t sIo[0x1070];

static void Store8(uint32_t address, uint32_t value) {
    sIo[address - IO_BASE] = (uint8_t)value;
}

static void Store16(uint32_t address, uint32_t value) {
    Store8(address, value);
    Store8(address + 1, value >> 8);
}

static void Store32(uint32_t address, uint32_t value) {
    Store16(address, value);
    Store16(address + 2, value >> 16);
}

// Indices of digest.h's canonical 2D words
#define W_DISPCNT 0
#define W_BG0HOFS 5
#define W_BG0VOFS 6
#define W_BG2X 17
#define W_BG2Y 18
#define W_BG3X 23
#define W_BG3Y 24
#define W_WIN0 25
#define W_WININ 33
#define W_BLDCNT 41
#define W_BLDY 43
#define W_MASTER_BRIGHT 44
#define W_DISPCAPCNT 45

// --- The hash

// xxHash's own known answers for XXH3-64 with seed 0 (its command-line tool's sanity check), over the bytes its
// generator makes: byte i is the top byte of 2654435761 times 11400714785074694797 to the ith power, modulo 2^64. One
// length for each of XXH3's ways through an input: empty, 1-3, 4-8, 9-16, 17-128, 129-240 and the long one
static void TestHash(void) {
    static const struct {
        size_t length;
        uint64_t hash;
    } known[] = {
        { 0, 0x2D06800538D394C2ull },  { 1, 0xC44BDFF4074EECDBull },   { 6, 0x27B56A84CD2D7325ull },
        { 12, 0xA713DAF0DFBB77E7ull }, { 24, 0xA3FE70BF9D3510EBull },  { 48, 0x397DA259ECBA1F11ull },
        { 80, 0xBCDEFBBB2C47C90Aull }, { 195, 0xCD94217EE362EC3Aull }, { 403, 0xCDEB804D65C6DEA4ull },
    };
    static uint8_t bytes[403];
    uint64_t generator = 2654435761u;
    size_t i;

    for (i = 0; i < sizeof(bytes); i++) {
        bytes[i] = (uint8_t)(generator >> 56);
        generator *= 11400714785074694797ull;
    }
    for (i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
        CHECK(pbw_digest_hash(bytes, known[i].length) == known[i].hash);
    }
}

// --- 2D registers

// melonDS's engine after the game wrote all ones to every register (Write32 and Write16, the engine on): what
// GPU2D.cpp keeps of each. DISPCNT whole on engine A and 0xc0b1fff7 of it on B; BGxCNT, the scrolls, the affine
// parameters, the windows' bytes, WININ and WINOUT and MASTER_BRIGHT as written; BGxX and BGxY with bits 28-31 set,
// bit 27 being set; MOSAIC split into nibbles; BLDCNT 0x3fff; BLDALPHA 0x1f1f; EVY clamped to 16; DISPCAPCNT
// 0xef3f1f1f on engine A, and 0 on B: NDS.cpp's ARM9IOWrite16 and ARM9IOWrite32 send the stores at 0x04000064 and
// 0x04000066 to unit A and none at 0x04001064 to any unit, so unit B's CaptureCnt keeps its reset value
static void MelonAllOnes(PbwDigestRegs2d *regs, int engine) {
    int i;

    memset(regs, 0, sizeof(*regs));
    regs->dispcnt = engine == PBW_DIGEST_ENGINE_A ? 0xffffffffu : 0xc0b1fff7u;
    for (i = 0; i < 4; i++) {
        regs->bgcnt[i] = 0xffff;
        regs->bgHofs[i] = 0xffff;
        regs->bgVofs[i] = 0xffff;
        regs->win0[i] = 0xff;
        regs->win1[i] = 0xff;
        regs->winCnt[i] = 0xff;
    }
    for (i = 0; i < 2; i++) {
        regs->bgPa[i] = 0xffff;
        regs->bgPb[i] = 0xffff;
        regs->bgPc[i] = 0xffff;
        regs->bgPd[i] = 0xffff;
        regs->bgX[i] = 0xffffffffu;
        regs->bgY[i] = 0xffffffffu;
        regs->bgMosaic[i] = 0xf;
        regs->objMosaic[i] = 0xf;
    }
    regs->bldcnt = 0x3fff;
    regs->bldalpha = 0x1f1f;
    regs->bldy = 16;
    regs->masterBright = 0xffff;
    regs->dispcapcnt = engine == PBW_DIGEST_ENGINE_A ? 0xef3f1f1fu : 0;
}

// The masks: every register written with all ones, on both engines
static void TestRegs2dMasks(void) {
    int engine;

    memset(sIo, 0xff, sizeof(sIo));
    for (engine = PBW_DIGEST_ENGINE_A; engine <= PBW_DIGEST_ENGINE_B; engine++) {
        PbwDigestRegs2d port;
        PbwDigestRegs2d melon;
        uint32_t portWords[PBW_DIGEST_REGS2D_WORDS];
        uint32_t melonWords[PBW_DIGEST_REGS2D_WORDS];
        uint32_t expected[PBW_DIGEST_REGS2D_WORDS];
        int n = 0;
        int i;

        // By hand from GBATEK's bits: DISPCNT (engine B's 0xc0b1fff7), BGxCNT, 9-bit scrolls, the affine parameters,
        // the 28-bit X and Y sign-extended, the windows' bytes, WININ's and WINOUT's 6 bits, MOSAIC's nibbles, BLDCNT,
        // BLDALPHA, BLDY at 16, MASTER_BRIGHT, DISPCAPCNT (engine A's alone)
        expected[n++] = engine == PBW_DIGEST_ENGINE_A ? 0xffffffffu : 0xc0b1fff7u;
        for (i = 0; i < 4; i++) {
            expected[n++] = 0xffff;
        }
        for (i = 0; i < 8; i++) {
            expected[n++] = 0x1ff;
        }
        for (i = 0; i < 2; i++) {
            expected[n++] = 0xffff;
            expected[n++] = 0xffff;
            expected[n++] = 0xffff;
            expected[n++] = 0xffff;
            expected[n++] = 0xffffffffu;
            expected[n++] = 0xffffffffu;
        }
        for (i = 0; i < 8; i++) {
            expected[n++] = 0xff;
        }
        for (i = 0; i < 4; i++) {
            expected[n++] = 0x3f;
        }
        for (i = 0; i < 4; i++) {
            expected[n++] = 0xf;
        }
        expected[n++] = 0x3fff;
        expected[n++] = 0x1f1f;
        expected[n++] = 16;
        expected[n++] = 0xc01f;
        expected[n++] = engine == PBW_DIGEST_ENGINE_A ? 0xef3f1f1fu : 0;
        CHECK(n == PBW_DIGEST_REGS2D_WORDS);

        pbw_digest_regs2d_from_io(&port, sIo + 0x1000 * engine, engine);
        MelonAllOnes(&melon, engine);
        pbw_digest_regs2d_words(&port, engine, portWords);
        pbw_digest_regs2d_words(&melon, engine, melonWords);
        CHECK(SameWords("regs2d, port, all ones", portWords, expected, PBW_DIGEST_REGS2D_WORDS));
        CHECK(SameWords("regs2d, melonDS, all ones", melonWords, expected, PBW_DIGEST_REGS2D_WORDS));
        // Without the masks the words would differ: melonDS keeps the scrolls, WININ, WINOUT and MASTER_BRIGHT whole,
        // and the port keeps every byte, engine B's DISPCAPCNT among them
        CHECK(melon.bgVofs[0] != expected[W_BG0VOFS] && melon.winCnt[0] != expected[W_WININ]);
        CHECK(melon.masterBright != expected[W_MASTER_BRIGHT]);
        CHECK(sIo[0x1000 * engine + 0x51] != (expected[W_BLDCNT] >> 8));
        if (engine == PBW_DIGEST_ENGINE_B) {
            CHECK(sIo[0x1000 + 0x03] != (expected[W_DISPCNT] >> 24));
            CHECK(sIo[0x1000 + 0x64] != 0 && expected[W_DISPCAPCNT] == 0);
        }
        // The decode is melonDS's unit field for field, engine B's DISPCAPCNT left at 0 as unit B's is
        CHECK(SameRegs2d(&port, &melon));
        // The encoding masks whatever it is given, a structure of all ones too: it doesn't count on the decode
        memset(&melon, 0xff, sizeof(melon));
        pbw_digest_regs2d_words(&melon, engine, melonWords);
        CHECK(SameWords("regs2d, all ones in every field", melonWords, expected, PBW_DIGEST_REGS2D_WORDS));
    }
}

// Every register a value of its own, so that a field read from the wrong register fails: engine A's stores as the game
// makes them (Store16 and Store32), with bits the hardware ignores set in a few, and melonDS's unit after the same
// stores, worked out from GPU2D.cpp's Write16 and Write32
static void TestRegs2dFields(void) {
    // DISPCNT and BG0CNT-BG3CNT; the scrolls, 9 bits; BG2's PA-PD, X and Y (sign-extended), then BG3's; WIN0's and
    // WIN1's X1, X2, Y1, Y2; WININ's and WINOUT's bytes, 6 bits; MOSAIC; BLDCNT, BLDALPHA, BLDY, MASTER_BRIGHT and
    // DISPCAPCNT
    static const uint32_t expected[PBW_DIGEST_REGS2D_WORDS] = {
        0x40013b1du, 0x1a01, 0x2b02, 0x3c03, 0x4d04, 0x123,       0x045,  0x067,       0x089,       0x1ab,
        0x0cd,       0x0ef,  0x101,  0x0100, 0xff80, 0x0040,      0x0200, 0x00012345u, 0xffff0000u, 0x0111,
        0x0222,      0x0333, 0x0444, 0x0100, 0x0200, 0x08,        0xf8,   0x10,        0xb0,        0x10,
        0x20,        0x30,   0x40,   0x3f,   0x01,   0x3e,        0x1f,   0x1,         0x2,         0x3,
        0x4,         0x3f41, 0x0a06, 3,      0x8010, 0x80310010u,
    };
    static const uint16_t bgcnt[4] = { 0x1a01, 0x2b02, 0x3c03, 0x4d04 };
    static const uint16_t hofs[4] = { 0x0123, 0x0067, 0x01ab, 0x00ef };
    static const uint16_t vofs[4] = { 0xfe45, 0x0089, 0x00cd, 0x0101 };
    PbwDigestRegs2d port;
    PbwDigestRegs2d melon;
    uint32_t portWords[PBW_DIGEST_REGS2D_WORDS];
    uint32_t melonWords[PBW_DIGEST_REGS2D_WORDS];
    int i;

    memset(sIo, 0, sizeof(sIo));
    Store32(0x04000000, 0x40013b1d);
    for (i = 0; i < 4; i++) {
        Store16(0x04000008 + 2 * i, bgcnt[i]);
        Store16(0x04000010 + 4 * i, hofs[i]);
        Store16(0x04000012 + 4 * i, vofs[i]);
    }
    Store16(0x04000020, 0x0100);
    Store16(0x04000022, 0xff80);
    Store16(0x04000024, 0x0040);
    Store16(0x04000026, 0x0200);
    Store32(0x04000028, 0x00012345);
    Store32(0x0400002c, 0x0fff0000);
    Store16(0x04000030, 0x0111);
    Store16(0x04000032, 0x0222);
    Store16(0x04000034, 0x0333);
    Store16(0x04000036, 0x0444);
    Store32(0x04000038, 0x00000100);
    Store32(0x0400003c, 0x00000200);
    Store16(0x04000040, 0x08f8);
    Store16(0x04000042, 0x1020);
    Store16(0x04000044, 0x10b0);
    Store16(0x04000046, 0x3040);
    Store16(0x04000048, 0xc13f);
    Store16(0x0400004a, 0x1f3e);
    Store16(0x0400004c, 0x4321);
    Store16(0x04000050, 0xbf41);
    Store16(0x04000052, 0x0a06);
    Store16(0x04000054, 0x0003);
    Store32(0x04000064, 0x80310010);
    Store16(0x0400006c, 0x8010);
    pbw_digest_regs2d_from_io(&port, sIo, PBW_DIGEST_ENGINE_A);

    memset(&melon, 0, sizeof(melon));
    melon.dispcnt = 0x40013b1d;
    for (i = 0; i < 4; i++) {
        melon.bgcnt[i] = bgcnt[i];
        melon.bgHofs[i] = hofs[i];
        melon.bgVofs[i] = vofs[i];
    }
    melon.bgPa[0] = 0x0100;
    melon.bgPb[0] = 0xff80;
    melon.bgPc[0] = 0x0040;
    melon.bgPd[0] = 0x0200;
    melon.bgX[0] = 0x00012345;
    melon.bgY[0] = 0xffff0000u; // bit 27 set: "val |= 0xF0000000"
    melon.bgPa[1] = 0x0111;
    melon.bgPb[1] = 0x0222;
    melon.bgPc[1] = 0x0333;
    melon.bgPd[1] = 0x0444;
    melon.bgX[1] = 0x00000100;
    melon.bgY[1] = 0x00000200;
    memcpy(melon.win0, (const uint8_t[4]){ 0x08, 0xf8, 0x10, 0xb0 }, 4);
    memcpy(melon.win1, (const uint8_t[4]){ 0x10, 0x20, 0x30, 0x40 }, 4);
    memcpy(melon.winCnt, (const uint8_t[4]){ 0x3f, 0xc1, 0x3e, 0x1f }, 4);
    melon.bgMosaic[0] = 0x1;
    melon.bgMosaic[1] = 0x2;
    melon.objMosaic[0] = 0x3;
    melon.objMosaic[1] = 0x4;
    melon.bldcnt = 0x3f41; // Write16: "BlendCnt = val & 0x3FFF"
    melon.bldalpha = 0x0a06;
    melon.bldy = 3;
    melon.masterBright = 0x8010;
    melon.dispcapcnt = 0x80310010;

    CHECK(SameRegs2d(&port, &melon));
    pbw_digest_regs2d_words(&port, PBW_DIGEST_ENGINE_A, portWords);
    pbw_digest_regs2d_words(&melon, PBW_DIGEST_ENGINE_A, melonWords);
    CHECK(SameWords("regs2d fields, port", portWords, expected, PBW_DIGEST_REGS2D_WORDS));
    CHECK(SameWords("regs2d fields, melonDS", melonWords, expected, PBW_DIGEST_REGS2D_WORDS));
    // A horizontal scroll is never a vertical one
    CHECK(expected[W_BG0HOFS] != expected[W_BG0VOFS]);
}

// BG2X, BG2Y, BG3X and BG3Y: 28 bits, sign-extended from bit 27, whatever bits 28-31 hold. melonDS sets bits 28-31
// when bit 27 is set (Write32's "if (val & 0x08000000) val |= 0xF0000000", and Write16's "if (val & 0x0800) val |=
// 0xF000" on the upper half), and keeps them as written when it isn't
static void TestRegs2dSignExtension(void) {
    PbwDigestRegs2d port;
    PbwDigestRegs2d melon;
    uint32_t portWords[PBW_DIGEST_REGS2D_WORDS];
    uint32_t melonWords[PBW_DIGEST_REGS2D_WORDS];

    memset(sIo, 0, sizeof(sIo));
    Store32(0x04000028, 0x08000000); // BG2X: the most negative, -0x8000000
    Store16(0x0400002c, 0xffff);     // BG2Y as two halves: -1
    Store16(0x0400002e, 0x0fff);
    Store32(0x04000038, 0xf0000123); // BG3X: positive, with bits 28-31 set by the game
    Store32(0x0400003c, 0x07ffffff); // BG3Y: the most positive
    pbw_digest_regs2d_from_io(&port, sIo, PBW_DIGEST_ENGINE_A);
    memset(&melon, 0, sizeof(melon));
    melon.bgX[0] = 0xf8000000u;
    melon.bgY[0] = 0xffffffffu;
    melon.bgX[1] = 0xf0000123u;
    melon.bgY[1] = 0x07ffffffu;
    CHECK(SameRegs2d(&port, &melon));
    pbw_digest_regs2d_words(&port, PBW_DIGEST_ENGINE_A, portWords);
    pbw_digest_regs2d_words(&melon, PBW_DIGEST_ENGINE_A, melonWords);
    CHECK(portWords[W_BG2X] == 0xf8000000u && melonWords[W_BG2X] == 0xf8000000u);
    CHECK(portWords[W_BG2Y] == 0xffffffffu && melonWords[W_BG2Y] == 0xffffffffu);
    CHECK(portWords[W_BG3X] == 0x00000123u && melonWords[W_BG3X] == 0x00000123u);
    CHECK(portWords[W_BG3Y] == 0x07ffffffu && melonWords[W_BG3Y] == 0x07ffffffu);
    CHECK(SameWords("sign extension", portWords, melonWords, PBW_DIGEST_REGS2D_WORDS));
    // The 28 bits alone, unextended, aren't the value, and neither is melonDS's word as it keeps it
    CHECK((0x08000000u & 0x0fffffffu) != portWords[W_BG2X] && melon.bgX[1] != portWords[W_BG3X]);
}

// BLDY: 5 bits, 17 to 31 acting as 16 (melonDS: "EVY = val & 0x1F; if (EVY > 16) EVY = 16"). The cases 0x11 and 0x1f
// are the negative controls: a decode or an encoding without the clamp gives their own value, not 16
static void TestRegs2dBldy(void) {
    static const struct {
        uint8_t written;
        uint8_t melon;
        uint32_t word;
    } cases[] = {
        { 0x00, 0, 0 }, { 0x0f, 15, 15 }, { 0x10, 16, 16 }, { 0x11, 16, 16 }, { 0x1f, 16, 16 }, { 0xe5, 5, 5 },
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        PbwDigestRegs2d port;
        PbwDigestRegs2d melon;
        uint32_t portWords[PBW_DIGEST_REGS2D_WORDS];
        uint32_t melonWords[PBW_DIGEST_REGS2D_WORDS];

        memset(sIo, 0, sizeof(sIo));
        Store16(0x04000054, cases[i].written);
        pbw_digest_regs2d_from_io(&port, sIo, PBW_DIGEST_ENGINE_A);
        memset(&melon, 0, sizeof(melon));
        melon.bldy = cases[i].melon;
        CHECK(SameRegs2d(&port, &melon));
        pbw_digest_regs2d_words(&port, PBW_DIGEST_ENGINE_A, portWords);
        pbw_digest_regs2d_words(&melon, PBW_DIGEST_ENGINE_A, melonWords);
        CHECK(portWords[W_BLDY] == cases[i].word && melonWords[W_BLDY] == cases[i].word);
    }
    // The encoding clamps whatever it is given, not only what the decode gave
    {
        PbwDigestRegs2d regs;
        uint32_t words[PBW_DIGEST_REGS2D_WORDS];
        memset(&regs, 0, sizeof(regs));
        regs.bldy = 0x1d;
        pbw_digest_regs2d_words(&regs, PBW_DIGEST_ENGINE_A, words);
        CHECK(words[W_BLDY] == 16);
    }
}

// WINxH and WINxV: X2 or Y2 in the low byte, X1 or Y1 in the high one; melonDS keeps X1, X2, Y1, Y2 ("Win0Coords[1] =
// val & 0xFF; Win0Coords[0] = val >> 8"), and so do the canonical words
static void TestRegs2dWindows(void) {
    static const uint32_t expected[8] = { 0x10, 0xf0, 0x20, 0xa0, 0x30, 0x40, 0x50, 0x60 };
    PbwDigestRegs2d port;
    PbwDigestRegs2d melon;
    uint32_t portWords[PBW_DIGEST_REGS2D_WORDS];
    uint32_t melonWords[PBW_DIGEST_REGS2D_WORDS];

    memset(sIo, 0, sizeof(sIo));
    Store16(0x04000040, 0x10f0); // WIN0H: X1 0x10, X2 0xf0
    Store16(0x04000042, 0x3040); // WIN1H
    Store16(0x04000044, 0x20a0); // WIN0V: Y1 0x20, Y2 0xa0
    Store16(0x04000046, 0x5060); // WIN1V
    pbw_digest_regs2d_from_io(&port, sIo, PBW_DIGEST_ENGINE_A);
    memset(&melon, 0, sizeof(melon));
    memcpy(melon.win0, (const uint8_t[4]){ 0x10, 0xf0, 0x20, 0xa0 }, 4);
    memcpy(melon.win1, (const uint8_t[4]){ 0x30, 0x40, 0x50, 0x60 }, 4);
    CHECK(SameRegs2d(&port, &melon));
    pbw_digest_regs2d_words(&port, PBW_DIGEST_ENGINE_A, portWords);
    pbw_digest_regs2d_words(&melon, PBW_DIGEST_ENGINE_A, melonWords);
    CHECK(SameWords("windows, port", portWords + W_WIN0, expected, 8));
    CHECK(SameWords("windows, melonDS", melonWords + W_WIN0, expected, 8));
    // The register's bytes in memory order put X2 first: unsplit, or split low byte first, they aren't these words
    CHECK(sIo[0x40] != expected[0]);
}

// --- System registers, VRAMCNT and the 3D registers

static void TestSysAndMap(void) {
    static const uint8_t melonVramcnt[PBW_DIGEST_VRAM_BANKS] = { 0x9b, 0x9b, 0x9f, 0x9f, 0x87, 0x9f, 0x9f, 0x83, 0x83 };
    static const uint8_t expectedSys[8] = { 0x0f, 0x82, 0, 0, 0x03, 0, 0, 0 };
    uint8_t expectedMap[PBW_DIGEST_VRAM_BANKS * 4] = { 0 };
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestBytes bytes = { 0 };
    int bank;

    // All ones written: POWCNT1 (melonDS keeps "val & 0x820F"), WRAMCNT (kept whole), each VRAMCNT (melonDS keeps
    // 0x9b of A and B, 0x9f of C, D, F and G, 0x87 of E and 0x83 of H and I)
    memset(sIo, 0xff, sizeof(sIo));
    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    pbw_digest_sys_from_io(&port.sys, sIo);
    melon.sys.powcnt1 = 0x820f;
    melon.sys.wramcnt = 0xff;
    // The decode stores both as melonDS does: POWCNT1's kept bits, WRAMCNT's whole byte
    CHECK(port.sys.powcnt1 == melon.sys.powcnt1 && port.sys.wramcnt == melon.sys.wramcnt);
    for (bank = 0; bank < PBW_DIGEST_VRAM_BANKS; bank++) {
        port.vram.vramcnt[bank] = 0xff;
        melon.vram.vramcnt[bank] = melonVramcnt[bank];
        // vram.map's words: each bank's VRAMCNT as melonDS keeps it, little-endian
        expectedMap[bank * 4] = melonVramcnt[bank];
    }
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_SYS, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 8 && memcmp(bytes.data, expectedSys, 8) == 0);
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&melon, PBW_DIGEST_SYS, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 8 && memcmp(bytes.data, expectedSys, 8) == 0);
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_MAP, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == sizeof(expectedMap) && memcmp(bytes.data, expectedMap, sizeof(expectedMap)) == 0);
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&melon, PBW_DIGEST_VRAM_MAP, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == sizeof(expectedMap) && memcmp(bytes.data, expectedMap, sizeof(expectedMap)) == 0);
    // WRAMCNT's byte as melonDS keeps it isn't its 2 bits
    CHECK(melon.sys.wramcnt != expectedSys[4]);
    pbw_digest_bytes_free(&bytes);
}

// Whether the port's decode of its 3D register bytes is melonDS's GPU3D field for field, but for DISP3DCNT, whose
// status bits melonDS sets itself (digest.h)
static int SameRegs3dButDisp3dcnt(const PbwDigestRegs3d *a, const PbwDigestRegs3d *b) {
    return a->alphaTestRef == b->alphaTestRef && a->clearColor == b->clearColor && a->clearDepth == b->clearDepth &&
           a->clearOffset == b->clearOffset && a->fogColor == b->fogColor && a->fogOffset == b->fogOffset &&
           memcmp(a->fogTable, b->fogTable, sizeof(a->fogTable)) == 0 &&
           memcmp(a->toonTable, b->toonTable, sizeof(a->toonTable)) == 0 &&
           memcmp(a->edgeColor, b->edgeColor, sizeof(a->edgeColor)) == 0;
}

static void TestRegs3d(void) {
    PbwDigestRegs3d port;
    PbwDigestRegs3d melon;
    uint32_t portWords[PBW_DIGEST_REGS3D_WORDS];
    uint32_t melonWords[PBW_DIGEST_REGS3D_WORDS];
    uint32_t expected[PBW_DIGEST_REGS3D_WORDS];
    int n = 0;
    int i;

    // All ones written. melonDS: DISP3DCNT "(val & 0x4FFF) | (DispCnt & 0x3000)", then bits 12 and 13 cleared by the
    // ones written there; ALPHA_TEST_REF "val & 0x1F"; CLEAR_COLOR, CLEAR_DEPTH with CLRIMAGE_OFFSET, FOG_COLOR, the
    // toon table and the edge colours as written; FOG_OFFSET "val & 0x7FFF"; the fog table's entries "& 0x7F"
    memset(sIo, 0xff, sizeof(sIo));
    pbw_digest_regs3d_from_io(&port, sIo);
    memset(&melon, 0, sizeof(melon));
    melon.disp3dcnt = 0x4fff;
    melon.alphaTestRef = 0x1f;
    melon.clearColor = 0xffffffffu;
    melon.clearDepth = 0xffff;
    melon.clearOffset = 0xffff;
    melon.fogColor = 0xffffffffu;
    melon.fogOffset = 0x7fff;
    for (i = 0; i < 32; i++) {
        melon.fogTable[i] = 0x7f;
        melon.toonTable[i] = 0xffff;
    }
    for (i = 0; i < 8; i++) {
        melon.edgeColor[i] = 0xffff;
    }
    // By hand from GBATEK's bits
    expected[n++] = 0x4fff;
    expected[n++] = 0x1f;
    expected[n++] = 0x3f1fffffu;
    expected[n++] = 0x7fff;
    expected[n++] = 0xffff;
    expected[n++] = 0x001f7fffu;
    expected[n++] = 0x7fff;
    for (i = 0; i < 32; i++) {
        expected[n++] = 0x7f;
    }
    for (i = 0; i < 40; i++) {
        expected[n++] = 0x7fff;
    }
    CHECK(n == PBW_DIGEST_REGS3D_WORDS);
    CHECK(SameRegs3dButDisp3dcnt(&port, &melon));
    pbw_digest_regs3d_words(&port, portWords);
    pbw_digest_regs3d_words(&melon, melonWords);
    CHECK(SameWords("regs3d, port, all ones", portWords, expected, PBW_DIGEST_REGS3D_WORDS));
    CHECK(SameWords("regs3d, melonDS, all ones", melonWords, expected, PBW_DIGEST_REGS3D_WORDS));
    CHECK(melon.clearColor != expected[2] && melon.toonTable[0] != expected[7 + 32]);
    // The encoding masks whatever it is given
    memset(&melon, 0xff, sizeof(melon));
    pbw_digest_regs3d_words(&melon, melonWords);
    CHECK(SameWords("regs3d, all ones in every field", melonWords, expected, PBW_DIGEST_REGS3D_WORDS));

    // A value of its own in each register; melonDS's DISP3DCNT holding the RAM overflow (bit 13) that the port's
    // register bytes don't, which g3d.stats compares instead
    memset(sIo, 0, sizeof(sIo));
    Store16(0x04000060, 0x0f3b);
    Store8(0x04000340, 0x0c);
    Store32(0x04000350, 0x12345678);
    Store16(0x04000354, 0x1234);
    Store16(0x04000356, 0x5678);
    Store32(0x04000358, 0x00abcdef);
    Store16(0x0400035c, 0x0fed);
    for (i = 0; i < 32; i++) {
        Store8(0x04000360 + i, (uint32_t)i);
        Store16(0x04000380 + 2 * i, 0x100 + (uint32_t)i);
    }
    for (i = 0; i < 8; i++) {
        Store16(0x04000330 + 2 * i, 0x200 + (uint32_t)i);
    }
    pbw_digest_regs3d_from_io(&port, sIo);
    melon.disp3dcnt = 0x2f3b;
    melon.alphaTestRef = 0x0c;
    melon.clearColor = 0x12345678;
    melon.clearDepth = 0x1234;
    melon.clearOffset = 0x5678;
    melon.fogColor = 0x00abcdef;
    melon.fogOffset = 0x0fed;
    for (i = 0; i < 32; i++) {
        melon.fogTable[i] = (uint8_t)i;
        melon.toonTable[i] = (uint16_t)(0x100 + i);
    }
    for (i = 0; i < 8; i++) {
        melon.edgeColor[i] = (uint16_t)(0x200 + i);
    }
    n = 0;
    expected[n++] = 0x0f3b;
    expected[n++] = 0x0c;
    expected[n++] = 0x12145678;
    expected[n++] = 0x1234;
    expected[n++] = 0x5678;
    expected[n++] = 0x000b4def;
    expected[n++] = 0x0fed;
    for (i = 0; i < 32; i++) {
        expected[n++] = (uint32_t)i;
    }
    for (i = 0; i < 32; i++) {
        expected[n++] = 0x100 + (uint32_t)i;
    }
    for (i = 0; i < 8; i++) {
        expected[n++] = 0x200 + (uint32_t)i;
    }
    CHECK(SameRegs3dButDisp3dcnt(&port, &melon) && port.disp3dcnt != melon.disp3dcnt);
    pbw_digest_regs3d_words(&port, portWords);
    pbw_digest_regs3d_words(&melon, melonWords);
    CHECK(SameWords("regs3d fields, port", portWords, expected, PBW_DIGEST_REGS3D_WORDS));
    CHECK(SameWords("regs3d fields, melonDS", melonWords, expected, PBW_DIGEST_REGS3D_WORDS));
}

// --- The 3D render list

// melonDS's arithmetic, worked out by hand: ((Z * 0x4000 / W) + 0x3fff) * 0x200, cut to 32 bits, clamped
static void TestDepth(void) {
    CHECK(pbw_digest_zbuffer_depth(0, 0x1000) == 0x7ffe00);
    CHECK(pbw_digest_zbuffer_depth(0x1000, 0x1000) == 0xfffe00);
    CHECK(pbw_digest_zbuffer_depth(0x800, 0x1000) == 0xbffe00);
    // (-0x4000 + 0x3fff) * 0x200 = -0x200, clamped to 0; (0x8000 + 0x3fff) * 0x200 = 0x17ffe00, to 0xffffff
    CHECK(pbw_digest_zbuffer_depth(-0x1000, 0x1000) == 0);
    CHECK(pbw_digest_zbuffer_depth(0x2000, 0x1000) == 0xffffff);
    // W is 24 bits; W = 0 gives melonDS's 0x7ffe00
    CHECK(pbw_digest_zbuffer_depth(0x1000, 0x01001000) == 0xfffe00);
    CHECK(pbw_digest_zbuffer_depth(123, 0) == 0x7ffe00);
    CHECK(pbw_digest_zbuffer_depth(123, 0x01000000) == 0x7ffe00);
    // Cut to 32 bits before the clamp: 0x3ffffffffffe00 becomes -0x200, then 0
    CHECK(pbw_digest_zbuffer_depth(0x7fffffff, 1) == 0);
    // A clip W of 2^24 or more is truncated to 24 bits first, which can leave a small divisor and a product past 32
    // bits, where the cut decides the depth: 0x2007ffe00, 0x400007ffe00 and -0x1ff800200 all cut to 0x7ffe00, where
    // clamping the whole 64-bit product would give 0xffffff, 0xffffff and 0. Frigate's ToScreen (ge.c) cuts as melonDS
    // does, and g3d_ge's depth cut cases hold it to these values (digest.h)
    CHECK(pbw_digest_zbuffer_depth(0x400, 0x01000001) == 0x7ffe00);
    CHECK(pbw_digest_zbuffer_depth(0x800000, 0x01000010) == 0x7ffe00);
    CHECK(pbw_digest_zbuffer_depth(-0x400, 0x01000001) == 0x7ffe00);
}

// Vertices of a quad drawn as a strip of two triangles, as both programs' viewports leave them
static const PbwDigestVertex sQuad[4] = {
    { 10, 20, 0xbffe00, 0x1000, { 511, 255, 0 }, 0x0010, 0x0020 },
    { 100, 20, 0xbffe00, 0x1000, { 0, 511, 255 }, 0xfff0, 0x0020 },
    { 10, 120, 0x7ffe00, 0x1000, { 255, 0, 511 }, 0x0010, 0x0400 },
    { 100, 120, 0xfffe00, 0x1000, { 15, 31, 47 }, 0xfff0, 0x0400 },
};

static PbwDigestPolygon Triangle(uint32_t a, uint32_t b, uint32_t c) {
    PbwDigestPolygon polygon;

    memset(&polygon, 0, sizeof(polygon));
    polygon.attr = 0x001f00c0; // opaque, both faces drawn
    polygon.texParam = 0x1e9c0123;
    polygon.plttBase = 0x0042;
    polygon.vertexCount = 3;
    polygon.front = 1;
    polygon.vertices[0] = a;
    polygon.vertices[1] = b;
    polygon.vertices[2] = c;
    return polygon;
}

// A little-endian word of a group's canonical bytes. In g3d.list's, a polygon's words come after the list's 2 words and
// the polygons before it, 5 words and 9 a vertex each
static uint32_t Word(const PbwDigestBytes *bytes, size_t word) {
    const uint8_t *at = bytes->data + word * 4;

    return (uint32_t)at[0] | (uint32_t)at[1] << 8 | (uint32_t)at[2] << 16 | (uint32_t)at[3] << 24;
}

// A group's hash, which must be encodable
static uint64_t GroupHash(const PbwDigestFrame *frame, int group) {
    uint64_t hash = 0;

    CHECK(pbw_digest_group_hash(frame, group, &hash) == PBW_DIGEST_OK);
    return hash;
}

// A strip's second triangle shares two vertices with the first. The port's list shares them by index (ge.c); melonDS
// shares them by pointer, or copies them when the strip can't reuse them, so the same triangles come with other
// indices into other vertex arrays. The canonical words copy each polygon's vertices, and are the same
static void TestStripDereferenced(void) {
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestPolygon portPolygons[2];
    PbwDigestPolygon melonPolygons[2];
    PbwDigestVertex melonVertices[6];
    PbwDigestBytes portBytes = { 0 };
    PbwDigestBytes melonBytes = { 0 };
    PbwDigestVertex changed[4];

    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    portPolygons[0] = Triangle(0, 1, 2);
    portPolygons[1] = Triangle(2, 1, 3);
    port.list.polygons = portPolygons;
    port.list.polygonCount = 2;
    port.list.vertices = sQuad;
    port.list.vertexCount = 4;
    // melonDS's vertex RAM: the first triangle's three, then the second's own copies of the two it shares
    melonVertices[0] = sQuad[0];
    melonVertices[1] = sQuad[1];
    melonVertices[2] = sQuad[2];
    melonVertices[3] = sQuad[2];
    melonVertices[4] = sQuad[1];
    melonVertices[5] = sQuad[3];
    melonPolygons[0] = Triangle(0, 1, 2);
    melonPolygons[1] = Triangle(3, 4, 5);
    melon.list.polygons = melonPolygons;
    melon.list.polygonCount = 2;
    melon.list.vertices = melonVertices;
    melon.list.vertexCount = 6;

    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_OK);
    CHECK(pbw_digest_group_bytes(&melon, PBW_DIGEST_G3D_LIST, &melonBytes) == PBW_DIGEST_OK);
    CHECK(portBytes.size == 4 * (2 + 2 * (5 + 3 * 9)));
    CHECK(portBytes.size == melonBytes.size && memcmp(portBytes.data, melonBytes.data, portBytes.size) == 0);
    // The second triangle's first vertex is the quad's third, X 10 and Y 120, on both
    CHECK(Word(&portBytes, 2 + 32 + 5) == 10 && Word(&portBytes, 2 + 32 + 5 + 1) == 120);
    // Their indices differ: encoded without dereferencing, the lists wouldn't be equal
    CHECK(portPolygons[1].vertices[0] != melonPolygons[1].vertices[0]);

    // A shared vertex that changes changes both triangles' words
    memcpy(changed, sQuad, sizeof(changed));
    changed[1].x = 101;
    port.list.vertices = changed;
    portBytes.size = 0;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_OK);
    CHECK(Word(&portBytes, 2 + 5 + 9) == 101 && Word(&portBytes, 2 + 32 + 5 + 9) == 101);

    // A list that breaks its rules encodes nothing: an index past the vertices, more than 10 vertices, no polygons
    portBytes.size = 0;
    portPolygons[1].vertices[2] = 4;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_INVALID);
    CHECK(portBytes.size == 0);
    portPolygons[1] = Triangle(2, 1, 3);
    portPolygons[1].vertexCount = PBW_DIGEST_POLYGON_VERTICES + 1;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_INVALID);
    port.list.polygons = NULL;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_INVALID);
    CHECK(portBytes.size == 0);
    pbw_digest_bytes_free(&portBytes);
    pbw_digest_bytes_free(&melonBytes);
}

// W = 0: melonDS puts the vertex at (0, 0) with the depth 0x7ffe00 (GPU3D.cpp: "z = 0x7FFE00") and marks the polygon
// degenerate; the port puts it at the viewport's origin, (0, 0) for the whole screen, with the same depth (ge.c's
// ToScreen). The canonical depth is 0 on both; X, Y and W are compared as they are. The port's side here has the depth
// 0, which ToScreen gave until it took melonDS's value, so that the two depths differ and the check shows the encoding
// making any depth 0 there. The other vertices' depths are the Z-buffer's on both, though this list W-buffers
// (SWAP_BUFFERS bit 1), where melonDS's FinalZ would hold W
static void TestWZero(void) {
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestVertex portVertices[3];
    PbwDigestVertex melonVertices[3];
    PbwDigestPolygon polygon = Triangle(0, 1, 2);
    PbwDigestBytes portBytes = { 0 };
    PbwDigestBytes melonBytes = { 0 };
    int i;

    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    for (i = 0; i < 3; i++) {
        portVertices[i] = sQuad[i];
        melonVertices[i] = sQuad[i];
    }
    // The port's ToScreen: Z 0x800 over W 0x1000 gives (0x2000 + 0x3fff) * 0x200
    portVertices[0].depth = 0xbffe00;
    melonVertices[0].depth = pbw_digest_zbuffer_depth(0x800, 0x1000);
    portVertices[2].x = 0;
    portVertices[2].y = 0;
    portVertices[2].w = 0;
    portVertices[2].depth = 0;
    melonVertices[2].x = 0;
    melonVertices[2].y = 0;
    melonVertices[2].w = 0;
    melonVertices[2].depth = pbw_digest_zbuffer_depth(0x400, 0);
    port.list.polygons = &polygon;
    port.list.polygonCount = 1;
    port.list.vertices = portVertices;
    port.list.vertexCount = 3;
    port.list.swapParam = 2;
    melon.list = port.list;
    melon.list.vertices = melonVertices;

    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_G3D_LIST, &portBytes) == PBW_DIGEST_OK);
    CHECK(pbw_digest_group_bytes(&melon, PBW_DIGEST_G3D_LIST, &melonBytes) == PBW_DIGEST_OK);
    CHECK(portBytes.size == melonBytes.size && memcmp(portBytes.data, melonBytes.data, portBytes.size) == 0);
    CHECK(Word(&melonBytes, 2 + 5 + 9 * 2 + 2) == 0);
    CHECK(Word(&melonBytes, 2 + 5 + 2) == 0xbffe00);
    // The two sides' depths for that vertex differ, so the encoding is what makes their words equal
    CHECK(melonVertices[2].depth == 0x7ffe00 && melonVertices[2].depth != portVertices[2].depth);
    pbw_digest_bytes_free(&portBytes);
    pbw_digest_bytes_free(&melonBytes);
}

// A triangle whose every field has a value of its own, as the canonical form keeps it, and g3d.list's words for it one
// by one: the polygon count and the SWAP_BUFFERS bits; the polygon's POLYGON_ATTR, TEXIMAGE_PARAM, PLTT_BASE, vertex
// count and facing; then each vertex's X, Y, depth, W, red, green, blue, S and T. A field dropped, moved or read from
// another one fails
static const PbwDigestVertex sTriangle[3] = {
    { 10, 20, 0xbffe00, 0x001000, { 511, 255, 31 }, 0x0010, 0x0020 },
    { 300, 191, 0x7ffe00, 0x123456, { 47, 63, 79 }, 0xfff0, 0x0400 },
    { 511, 0, 0xfffe00, 0xffffff, { 0, 31, 495 }, 0x8000, 0x7fff },
};
static const uint32_t sTriangleWords[2 + 5 + 3 * 9] = {
    1,   2,   0x3f1f80c0, 0x1e9c0123, 0x1042, 3,   1,                   // the list, the polygon
    10,  20,  0xbffe00,   0x001000,   511,    255, 31,  0x0010, 0x0020, // its first vertex
    300, 191, 0x7ffe00,   0x123456,   47,     63,  79,  0xfff0, 0x0400, // its second
    511, 0,   0xfffe00,   0xffffff,   0,      31,  495, 0x8000, 0x7fff, // its third
};

// A one-triangle list over vertices, with a polygon's fields as given
static void SetTriangleList(PbwDigestList *list, PbwDigestPolygon *polygon, const PbwDigestVertex *vertices,
                            uint32_t plttBase, uint32_t front, uint32_t swapParam) {
    *polygon = Triangle(0, 1, 2);
    polygon->attr = 0x3f1f80c0;
    polygon->plttBase = plttBase;
    polygon->front = front;
    memset(list, 0, sizeof(*list));
    list->polygons = polygon;
    list->polygonCount = 1;
    list->vertices = vertices;
    list->vertexCount = 3;
    list->swapParam = swapParam;
}

// The words of a list, checked against the expected ones
static void CheckListWords(const char *what, const PbwDigestList *list, const uint32_t *expected, size_t count) {
    PbwDigestFrame frame;
    PbwDigestBytes bytes = { 0 };
    uint32_t words[2 + 5 + 3 * 9];
    size_t i;

    memset(&frame, 0, sizeof(frame));
    frame.list = *list;
    CHECK(pbw_digest_group_bytes(&frame, PBW_DIGEST_G3D_LIST, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == count * 4 && count <= sizeof(words) / sizeof(words[0]));
    if (bytes.size == count * 4 && count <= sizeof(words) / sizeof(words[0])) {
        for (i = 0; i < count; i++) {
            words[i] = Word(&bytes, i);
        }
        CHECK(SameWords(what, words, expected, (int)count));
    }
    pbw_digest_bytes_free(&bytes);
}

static void TestListWords(void) {
    PbwDigestList list;
    PbwDigestPolygon polygon;

    SetTriangleList(&list, &polygon, sTriangle, 0x1042, 1, 2);
    CheckListWords("g3d.list, word by word", &list, sTriangleWords, sizeof(sTriangleWords) / sizeof(sTriangleWords[0]));
}

// Each field keeps the bits the hardware has, whatever else either program's copy holds: S and T 16 bits, sign-extended
// in one program (the port's int16_t widened) and not in the other; X 9 bits and Y 8; the depth and W 24; PLTT_BASE 13;
// the SWAP_BUFFERS parameter 2; the facing 0 or 1. Bits above those, set in one program's copy here, don't count
static void TestListMasks(void) {
    PbwDigestVertex wide[3];
    PbwDigestList list;
    PbwDigestPolygon polygon;
    int i;

    memcpy(wide, sTriangle, sizeof(wide));
    for (i = 0; i < 3; i++) {
        wide[i].x |= 0xfffffe00u;
        wide[i].y |= 0xffffff00u;
        wide[i].depth |= 0xff000000u;
        wide[i].w |= 0x7f000000u;
        // Sign-extended from bit 15, as an int16_t widens
        wide[i].s = (wide[i].s ^ 0x8000u) - 0x8000u;
        wide[i].t = (wide[i].t ^ 0x8000u) - 0x8000u;
    }
    // The second and third vertices' S, 0xfff0 and 0x8000, are negative: sign-extended, they aren't their words
    CHECK(wide[1].s == 0xfffffff0u && wide[2].s == 0xffff8000u && wide[1].s != sTriangleWords[7 + 9 + 7]);
    SetTriangleList(&list, &polygon, wide, 0x1042 | 0x2000, 5, 2 | 4);
    CheckListWords("g3d.list, bits above each field's", &list, sTriangleWords,
                   sizeof(sTriangleWords) / sizeof(sTriangleWords[0]));
}

// g3d.stats: the list's polygon count, and whether polygon or vertex RAM overflowed, which melonDS keeps as DISP3DCNT's
// bit 13 (GPU3D::DispCnt & 0x2000) and the port as a flag (port_ge_ram_overflow): any nonzero value is 1
static void TestStats(void) {
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestBytes bytes = { 0 };

    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    port.list.polygonCount = 2048;
    melon.list.polygonCount = 2048;
    port.list.ramOverflow = 1;
    melon.list.ramOverflow = 0x2000;
    CHECK(GroupHash(&port, PBW_DIGEST_G3D_STATS) == GroupHash(&melon, PBW_DIGEST_G3D_STATS));
    CHECK(pbw_digest_group_bytes(&melon, PBW_DIGEST_G3D_STATS, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 8 && Word(&bytes, 0) == 2048 && Word(&bytes, 1) == 1);
    // RAM that overflowed in one program only, and a polygon count apart, each show
    melon.list.ramOverflow = 0;
    CHECK(GroupHash(&port, PBW_DIGEST_G3D_STATS) != GroupHash(&melon, PBW_DIGEST_G3D_STATS));
    melon.list.ramOverflow = 0x2000;
    melon.list.polygonCount = 2047;
    CHECK(GroupHash(&port, PBW_DIGEST_G3D_STATS) != GroupHash(&melon, PBW_DIGEST_G3D_STATS));
    pbw_digest_bytes_free(&bytes);
}

// --- VRAM and capture

// Both programs' VRAM: melonDS's banks one after another (GPU::VRAM_A to VRAM_I), the port's pages wherever its views
// keep them, here in reverse order, and a page it has nowhere (NULL) where melonDS's holds zeros
#define VRAM_PAGES 41
static uint8_t sMelonVram[VRAM_PAGES * PBW_DIGEST_VRAM_PAGE_SIZE];
static uint8_t sPortVram[VRAM_PAGES * PBW_DIGEST_VRAM_PAGE_SIZE];

static void SetVram(PbwDigestFrame *melon, PbwDigestFrame *port) {
    int first = 0;
    int bank;
    unsigned page;

    for (bank = 0; bank < PBW_DIGEST_VRAM_BANKS; bank++) {
        for (page = 0; page < pbw_digest_vram_pages(bank); page++) {
            int at = first + (int)page;
            melon->vram.pages[bank][page] = sMelonVram + (size_t)at * PBW_DIGEST_VRAM_PAGE_SIZE;
            port->vram.pages[bank][page] = sPortVram + (size_t)(VRAM_PAGES - 1 - at) * PBW_DIGEST_VRAM_PAGE_SIZE;
            memcpy((uint8_t *)port->vram.pages[bank][page], melon->vram.pages[bank][page], PBW_DIGEST_VRAM_PAGE_SIZE);
        }
        first += (int)pbw_digest_vram_pages(bank);
    }
    // Bank I's page zero on melonDS, nowhere on the port
    memset(sMelonVram + (size_t)(VRAM_PAGES - 1) * PBW_DIGEST_VRAM_PAGE_SIZE, 0, PBW_DIGEST_VRAM_PAGE_SIZE);
    port->vram.pages[PBW_DIGEST_VRAM_I][0] = NULL;
}

static uint8_t *BankByte(const PbwDigestFrame *frame, int bank, uint32_t offset) {
    return (uint8_t *)frame->vram.pages[bank][offset / PBW_DIGEST_VRAM_PAGE_SIZE] + offset % PBW_DIGEST_VRAM_PAGE_SIZE;
}

// The title screen captures its 3D into bank D each frame (src/ov162/title.c): 256x192 pixels at offset 0x18000,
// which wraps around the bank's end, so 0x18000-0x1ffff and 0x00000-0x0ffff hold rasterized pixels that differ between
// the two programs' rasterizers. Those bytes are zeroed on both sides; the rest of the bank is compared
static void TestCaptureZeroed(void) {
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestBytes bytes = { 0 };
    uint32_t offset;
    int bank;
    int differs = 0;

    for (offset = 0; offset < sizeof(sMelonVram); offset++) {
        sMelonVram[offset] = (uint8_t)(offset * 7 + (offset >> 13));
    }
    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    SetVram(&melon, &port);
    for (bank = 0; bank < PBW_DIGEST_VRAM_BANKS; bank++) {
        CHECK(GroupHash(&port, bank) == GroupHash(&melon, bank));
    }
    melon.banks3d.holds3d[PBW_DIGEST_VRAM_D] = 1;
    melon.banks3d.offset[PBW_DIGEST_VRAM_D] = 0x18000;
    melon.banks3d.size[PBW_DIGEST_VRAM_D] = 0x18000;
    melon.banks3d.hash[PBW_DIGEST_VRAM_D] = 0x1234; // of the region's bytes: its own on each side, never compared
    port.banks3d = melon.banks3d;
    port.banks3d.hash[PBW_DIGEST_VRAM_D] = 0x5678;
    // Each program's rasterized pixels in the region
    for (offset = 0; offset < 0x18000; offset++) {
        uint32_t at = (0x18000 + offset) & 0x1ffff;
        *BankByte(&port, PBW_DIGEST_VRAM_D, at) = (uint8_t)(offset * 13);
        *BankByte(&melon, PBW_DIGEST_VRAM_D, at) = (uint8_t)(offset * 13 + (offset % 61 == 0));
        differs |= *BankByte(&port, PBW_DIGEST_VRAM_D, at) != *BankByte(&melon, PBW_DIGEST_VRAM_D, at);
    }
    CHECK(differs);
    CHECK(GroupHash(&port, PBW_DIGEST_VRAM_D) == GroupHash(&melon, PBW_DIGEST_VRAM_D));
    CHECK(GroupHash(&port, PBW_DIGEST_CAPTURE) == GroupHash(&melon, PBW_DIGEST_CAPTURE));
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_D, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 0x20000);
    CHECK(bytes.data[0x00000] == 0 && bytes.data[0x0ffff] == 0 && bytes.data[0x18000] == 0 && bytes.data[0x1ffff] == 0);
    CHECK(bytes.data[0x10000] == *BankByte(&melon, PBW_DIGEST_VRAM_D, 0x10000));
    CHECK(bytes.data[0x17fff] == *BankByte(&melon, PBW_DIGEST_VRAM_D, 0x17fff));
    // Outside the region, a difference shows
    (*BankByte(&port, PBW_DIGEST_VRAM_D, 0x12345))++;
    CHECK(GroupHash(&port, PBW_DIGEST_VRAM_D) != GroupHash(&melon, PBW_DIGEST_VRAM_D));
    (*BankByte(&port, PBW_DIGEST_VRAM_D, 0x12345))--;
    // A region at any offset, cut where it starts and ends
    bytes.size = 0;
    port.banks3d.offset[PBW_DIGEST_VRAM_D] = 0x1234;
    port.banks3d.size[PBW_DIGEST_VRAM_D] = 0x100;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_D, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.data[0x1233] == *BankByte(&port, PBW_DIGEST_VRAM_D, 0x1233) && bytes.data[0x1234] == 0);
    CHECK(bytes.data[0x1333] == 0 && bytes.data[0x1334] == *BankByte(&port, PBW_DIGEST_VRAM_D, 0x1334));
    port.banks3d = melon.banks3d;

    // A bank that held 3D once keeps the region's offset and size in PbwBanks3d (display3d.c's checks clear only
    // holds3d): nothing is zeroed in it, and the capture group's words for it are 0
    melon.banks3d.holds3d[PBW_DIGEST_VRAM_C] = 0;
    melon.banks3d.offset[PBW_DIGEST_VRAM_C] = 0x8000;
    melon.banks3d.size[PBW_DIGEST_VRAM_C] = 0x8000;
    *BankByte(&melon, PBW_DIGEST_VRAM_C, 0x8000) ^= 0xff;
    *BankByte(&port, PBW_DIGEST_VRAM_C, 0x8000) ^= 0x0f;
    CHECK(GroupHash(&port, PBW_DIGEST_VRAM_C) != GroupHash(&melon, PBW_DIGEST_VRAM_C));
    CHECK(GroupHash(&port, PBW_DIGEST_CAPTURE) == GroupHash(&melon, PBW_DIGEST_CAPTURE));
    // A region outside its bank, or larger than it, is no region
    port.banks3d.offset[PBW_DIGEST_VRAM_D] = 0x20000;
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_D, &bytes) == PBW_DIGEST_INVALID);
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_CAPTURE, &bytes) == PBW_DIGEST_INVALID);
    port.banks3d.offset[PBW_DIGEST_VRAM_D] = 0;
    port.banks3d.size[PBW_DIGEST_VRAM_D] = 0x20001;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_D, &bytes) == PBW_DIGEST_INVALID);
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_CAPTURE, &bytes) == PBW_DIGEST_INVALID);
    CHECK(bytes.size == 0);
    pbw_digest_bytes_free(&bytes);
}

// The capture group: whether the unit ran, and with what; when it didn't, what the registers held is no part of it
static void TestCaptureGroup(void) {
    PbwDigestFrame a;
    PbwDigestFrame b;
    PbwDigestBytes bytes = { 0 };

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.capture.dispcapcnt = 0x00330000; // read mid-frame, without the enable latched
    a.capture.dispcntA = 0x00020000;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) == GroupHash(&b, PBW_DIGEST_CAPTURE));
    // A capture that ran: DISPCAPCNT's bits that act (0xef3f1f1f), engine A's DISPCNT, its destination in the LCDC
    a.capture.ran = 1;
    a.capture.dispcapcnt = 0xffffffffu;
    a.capture.destInLcdc = 5;
    CHECK(pbw_digest_group_bytes(&a, PBW_DIGEST_CAPTURE, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 16 * 4 && Word(&bytes, 0) == 1 && Word(&bytes, 1) == 0xef3f1f1fu);
    CHECK(Word(&bytes, 2) == 0x00020000 && Word(&bytes, 3) == 1);
    b.capture = a.capture;
    b.capture.dispcapcnt = 0xef3f1f1fu;
    b.capture.destInLcdc = 1;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) == GroupHash(&b, PBW_DIGEST_CAPTURE));
    // A destination outside the LCDC is another capture
    b.capture.destInLcdc = 0;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) != GroupHash(&b, PBW_DIGEST_CAPTURE));
    b.capture.destInLcdc = 1;

    // Then three words a bank, A to D: whether it holds 3D (any nonzero being 1), its region's offset and its size
    a.banks3d.holds3d[PBW_DIGEST_VRAM_C] = 7;
    a.banks3d.offset[PBW_DIGEST_VRAM_C] = 0x8000;
    a.banks3d.size[PBW_DIGEST_VRAM_C] = 0x6000;
    b.banks3d = a.banks3d;
    b.banks3d.holds3d[PBW_DIGEST_VRAM_C] = 1;
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&a, PBW_DIGEST_CAPTURE, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == 16 * 4 && Word(&bytes, 4 + 3 * 2) == 1 && Word(&bytes, 4 + 3 * 2 + 1) == 0x8000 &&
          Word(&bytes, 4 + 3 * 2 + 2) == 0x6000);
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) == GroupHash(&b, PBW_DIGEST_CAPTURE));
    // A bank that holds 3D in one program only, or a region at another offset or of another size, is another capture
    b.banks3d.holds3d[PBW_DIGEST_VRAM_C] = 0;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) != GroupHash(&b, PBW_DIGEST_CAPTURE));
    b.banks3d = a.banks3d;
    b.banks3d.offset[PBW_DIGEST_VRAM_C] = 0x9000;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) != GroupHash(&b, PBW_DIGEST_CAPTURE));
    b.banks3d = a.banks3d;
    b.banks3d.size[PBW_DIGEST_VRAM_C] = 0x7000;
    CHECK(GroupHash(&a, PBW_DIGEST_CAPTURE) != GroupHash(&b, PBW_DIGEST_CAPTURE));
    pbw_digest_bytes_free(&bytes);
}

// --- The screens

static uint32_t sScreens[2][2][PBW_DIGEST_SCREEN_WIDTH * PBW_DIGEST_SCREEN_HEIGHT];

// The DS's 6 bits of each channel, widened as both programs widen them; alpha as each program has it
static uint32_t Pixel(uint32_t alpha, uint32_t r, uint32_t g, uint32_t b) {
    return alpha << 24 | (r << 2 | r >> 4) << 16 | (g << 2 | g >> 4) << 8 | (b << 2 | b >> 4);
}

static void TestScreens(void) {
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestBytes bytes = { 0 };
    uint64_t hash;
    int i;

    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    for (i = 0; i < PBW_DIGEST_SCREEN_WIDTH * PBW_DIGEST_SCREEN_HEIGHT; i++) {
        uint32_t r = (uint32_t)i % 64;
        uint32_t g = (uint32_t)i / 64 % 64;
        uint32_t b = (uint32_t)i / 4096 % 64;
        sScreens[0][0][i] = Pixel(0xff, r, g, b);
        sScreens[1][0][i] = Pixel(0x00, r, g, b);
        sScreens[0][1][i] = Pixel(0xff, b, r, g);
        sScreens[1][1][i] = Pixel(0x1f, b, r, g);
    }
    port.screens[0] = sScreens[0][0];
    port.screens[1] = sScreens[0][1];
    melon.screens[0] = sScreens[1][0];
    melon.screens[1] = sScreens[1][1];
    CHECK(GroupHash(&port, PBW_DIGEST_SCREEN_TOP) == GroupHash(&melon, PBW_DIGEST_SCREEN_TOP));
    CHECK(GroupHash(&port, PBW_DIGEST_SCREEN_BOTTOM) == GroupHash(&melon, PBW_DIGEST_SCREEN_BOTTOM));
    CHECK(GroupHash(&port, PBW_DIGEST_SCREEN_TOP) != GroupHash(&port, PBW_DIGEST_SCREEN_BOTTOM));
    // A pixel's 6-bit channels, a byte each, row by row
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_SCREEN_TOP, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == PBW_DIGEST_SCREEN_WIDTH * PBW_DIGEST_SCREEN_HEIGHT * 3);
    CHECK(bytes.data[3 * 8259] == 3 && bytes.data[3 * 8259 + 1] == 1 && bytes.data[3 * 8259 + 2] == 2);
    // One channel a step off on one pixel shows
    sScreens[1][0][12345] = Pixel(0x00, 12345 % 64, 12345 / 64 % 64, 12345 / 4096 % 64 + 1);
    CHECK(GroupHash(&port, PBW_DIGEST_SCREEN_TOP) != GroupHash(&melon, PBW_DIGEST_SCREEN_TOP));
    // A program without the screens has no such group
    port.screens[1] = NULL;
    CHECK(pbw_digest_group_hash(&port, PBW_DIGEST_SCREEN_BOTTOM, &hash) == PBW_DIGEST_ABSENT);
    pbw_digest_bytes_free(&bytes);
}

// --- Palettes and OAM

// pal and oam: 2 KiB each as stored, engine A's 1 KiB and then engine B's from 0x400. A byte changed at either end of
// either engine's half changes the group
static void TestPaletteAndOam(void) {
    static uint8_t palettes[2][PBW_DIGEST_PALETTE_SIZE];
    static uint8_t oam[2][PBW_DIGEST_OAM_SIZE];
    static const uint32_t offsets[4] = { 0x000, 0x3ff, 0x400, 0x7ff };
    PbwDigestFrame port;
    PbwDigestFrame melon;
    PbwDigestBytes bytes = { 0 };
    size_t i;

    for (i = 0; i < PBW_DIGEST_PALETTE_SIZE; i++) {
        palettes[0][i] = (uint8_t)(i * 5 + 1);
        palettes[1][i] = (uint8_t)(i * 5 + 1);
    }
    for (i = 0; i < PBW_DIGEST_OAM_SIZE; i++) {
        oam[0][i] = (uint8_t)(i * 3 + 7);
        oam[1][i] = (uint8_t)(i * 3 + 7);
    }
    memset(&port, 0, sizeof(port));
    memset(&melon, 0, sizeof(melon));
    port.palette = palettes[0];
    port.oam = oam[0];
    melon.palette = palettes[1];
    melon.oam = oam[1];
    CHECK(GroupHash(&port, PBW_DIGEST_PAL) == GroupHash(&melon, PBW_DIGEST_PAL));
    CHECK(GroupHash(&port, PBW_DIGEST_OAM) == GroupHash(&melon, PBW_DIGEST_OAM));
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_PAL, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == PBW_DIGEST_PALETTE_SIZE && memcmp(bytes.data, palettes[0], PBW_DIGEST_PALETTE_SIZE) == 0);
    bytes.size = 0;
    CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_OAM, &bytes) == PBW_DIGEST_OK);
    CHECK(bytes.size == PBW_DIGEST_OAM_SIZE && memcmp(bytes.data, oam[0], PBW_DIGEST_OAM_SIZE) == 0);
    for (i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        palettes[0][offsets[i]]++;
        CHECK(GroupHash(&port, PBW_DIGEST_PAL) != GroupHash(&melon, PBW_DIGEST_PAL));
        palettes[0][offsets[i]]--;
        oam[0][offsets[i]]++;
        CHECK(GroupHash(&port, PBW_DIGEST_OAM) != GroupHash(&melon, PBW_DIGEST_OAM));
        oam[0][offsets[i]]--;
    }
    pbw_digest_bytes_free(&bytes);
}

// --- Whole frames, records and streams

// The groups a record has, as a count; and whether exactly one group, only, is flagged in a per-group array
static int Present(const PbwDigestRecord *record) {
    int count = 0;
    int g;

    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        count += record->present[g] != 0;
    }
    return count;
}

static int OnlyGroup(const uint8_t flags[PBW_DIGEST_GROUPS], int group) {
    int g;

    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        if ((flags[g] != 0) != (g == group)) {
            return 0;
        }
    }
    return 1;
}

// How many groups a per-group array flags
static int Flagged(const uint8_t flags[PBW_DIGEST_GROUPS]) {
    int count = 0;
    int g;

    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        count += flags[g] != 0;
    }
    return count;
}

static uint8_t sPalettes[2][PBW_DIGEST_PALETTE_SIZE];
static uint8_t sOam[2][PBW_DIGEST_OAM_SIZE];

// The same made-up state in both programs' shapes, varied by a frame number: the 2D and 3D registers written as the
// game writes them (the port's) and as melonDS keeps them, VRAM laid out as each keeps it, a strip in each one's
// vertex RAM, the same palettes, OAM and screens
static void MakeFrames(PbwDigestFrame *melon, PbwDigestFrame *port, uint32_t frame, PbwDigestPolygon polygons[2][2],
                       PbwDigestVertex melonVertices[6]) {
    int i;

    memset(melon, 0, sizeof(*melon));
    memset(port, 0, sizeof(*port));
    memset(sIo, 0, sizeof(sIo));
    // Engine A: BG3 scrolled by the frame; BLDY 31 (16 on melonDS); BG2X negative; WIN0H; engine B: DISPCNT with bits
    // engine B ignores
    Store32(0x04000000, 0x00011f13);
    Store16(0x0400001c, frame & 0x1ff);
    Store16(0x04000054, 0x1f);
    Store32(0x04000028, 0x0ff00000);
    Store16(0x04000040, 0x20e0);
    Store32(0x04001000, 0x0001ff1b);
    Store16(0x04000304, 0x820f);
    Store8(0x04000247, 0x03);
    Store16(0x04000060, 0x0001);
    Store32(0x04000350, 0x80007fff);
    pbw_digest_regs2d_from_io(&port->regs2d[0], sIo, PBW_DIGEST_ENGINE_A);
    pbw_digest_regs2d_from_io(&port->regs2d[1], sIo + 0x1000, PBW_DIGEST_ENGINE_B);
    pbw_digest_sys_from_io(&port->sys, sIo);
    pbw_digest_regs3d_from_io(&port->regs3d, sIo);
    melon->regs2d[0].dispcnt = 0x00011f13;
    melon->regs2d[0].bgHofs[3] = (uint16_t)(frame & 0x1ff);
    melon->regs2d[0].bldy = 16;
    melon->regs2d[0].bgX[0] = 0xfff00000u;
    melon->regs2d[0].win0[0] = 0x20;
    melon->regs2d[0].win0[1] = 0xe0;
    melon->regs2d[1].dispcnt = 0x0001ff13;
    melon->sys.powcnt1 = 0x820f;
    melon->sys.wramcnt = 0x03;
    melon->regs3d.disp3dcnt = 0x0001;
    melon->regs3d.clearColor = 0x80007fff;

    SetVram(melon, port);
    for (i = 0; i < PBW_DIGEST_VRAM_BANKS; i++) {
        port->vram.vramcnt[i] = 0x81;
        melon->vram.vramcnt[i] = 0x81;
    }
    memset(sPalettes[0], (int)frame, sizeof(sPalettes[0]));
    memset(sPalettes[1], (int)frame, sizeof(sPalettes[1]));
    memset(sOam[0], 0x5a, sizeof(sOam[0]));
    memset(sOam[1], 0x5a, sizeof(sOam[1]));
    port->palette = sPalettes[0];
    melon->palette = sPalettes[1];
    port->oam = sOam[0];
    melon->oam = sOam[1];

    polygons[0][0] = Triangle(0, 1, 2);
    polygons[0][1] = Triangle(2, 1, 3);
    polygons[1][0] = Triangle(0, 1, 2);
    polygons[1][1] = Triangle(3, 4, 5);
    melonVertices[0] = sQuad[0];
    melonVertices[1] = sQuad[1];
    melonVertices[2] = sQuad[2];
    melonVertices[3] = sQuad[2];
    melonVertices[4] = sQuad[1];
    melonVertices[5] = sQuad[3];
    port->list.polygons = polygons[0];
    port->list.polygonCount = 2;
    port->list.vertices = sQuad;
    port->list.vertexCount = 4;
    port->list.swapParam = 1;
    melon->list.polygons = polygons[1];
    melon->list.polygonCount = 2;
    melon->list.vertices = melonVertices;
    melon->list.vertexCount = 6;
    melon->list.swapParam = 1;

    port->screens[0] = sScreens[0][0];
    port->screens[1] = sScreens[0][1];
    melon->screens[0] = sScreens[0][0];
    melon->screens[1] = sScreens[0][1];
}

static void TestRecords(void) {
    PbwDigestFrame melon;
    PbwDigestFrame port;
    PbwDigestPolygon polygons[2][2];
    PbwDigestVertex melonVertices[6];
    PbwDigestRecord melonRecord;
    PbwDigestRecord portRecord;
    uint8_t differs[PBW_DIGEST_GROUPS];
    int group = -1;
    int g;

    MakeFrames(&melon, &port, 7, polygons, melonVertices);
    CHECK(pbw_digest_record_frame(&melonRecord, &melon, &group) == PBW_DIGEST_OK);
    CHECK(pbw_digest_record_frame(&portRecord, &port, &group) == PBW_DIGEST_OK);
    CHECK(Present(&melonRecord) == PBW_DIGEST_GROUPS && Present(&portRecord) == PBW_DIGEST_GROUPS);
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        if (melonRecord.hash[g] != portRecord.hash[g]) {
            fprintf(stderr, "records: %s differs\n", pbw_digest_group_name(g));
        }
    }
    CHECK(pbw_digest_record_differences(&melonRecord, &portRecord, NULL) == 0);
    // A group's hash is the hash of its canonical bytes, page by page or not
    {
        PbwDigestBytes bytes = { 0 };
        CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_VRAM_A, &bytes) == PBW_DIGEST_OK);
        CHECK(pbw_digest_hash(bytes.data, bytes.size) == portRecord.hash[PBW_DIGEST_VRAM_A]);
        bytes.size = 0;
        CHECK(pbw_digest_group_bytes(&port, PBW_DIGEST_REGS2D_A, &bytes) == PBW_DIGEST_OK);
        CHECK(bytes.size == PBW_DIGEST_REGS2D_WORDS * 4);
        CHECK(pbw_digest_hash(bytes.data, bytes.size) == portRecord.hash[PBW_DIGEST_REGS2D_A]);
        pbw_digest_bytes_free(&bytes);
    }
    // The palettes a step apart: pal differs, and only pal
    sPalettes[0][0x22]++;
    CHECK(pbw_digest_record_frame(&portRecord, &port, &group) == PBW_DIGEST_OK);
    CHECK(pbw_digest_record_differences(&melonRecord, &portRecord, differs) == 1 && OnlyGroup(differs, PBW_DIGEST_PAL));
    // Groups a program doesn't have: absent, so their hashes aren't compared; but the record lacks them, which against
    // a record that has them is a divergence (pbw_digest_compare_streams), and the other way round isn't
    port.oam = NULL;
    port.palette = NULL;
    CHECK(pbw_digest_record_frame(&portRecord, &port, &group) == PBW_DIGEST_OK);
    CHECK(!portRecord.present[PBW_DIGEST_OAM] && !portRecord.present[PBW_DIGEST_PAL] &&
          Present(&portRecord) == PBW_DIGEST_GROUPS - 2);
    CHECK(pbw_digest_record_differences(&melonRecord, &portRecord, NULL) == 0);
    CHECK(pbw_digest_record_missing(&melonRecord, &portRecord, differs) == 2 && differs[PBW_DIGEST_OAM] &&
          differs[PBW_DIGEST_PAL] && differs[PBW_DIGEST_VRAM_A] == 0);
    CHECK(pbw_digest_record_missing(&portRecord, &melonRecord, differs) == 0 && differs[PBW_DIGEST_PAL] == 0);
    // A group that can't be encoded stops the record and says which, and the record then holds no group: neither the
    // groups before the bad one nor what the record held before, a whole frame's here
    polygons[0][0].vertices[1] = 9;
    CHECK(Present(&melonRecord) == PBW_DIGEST_GROUPS);
    CHECK(pbw_digest_record_frame(&melonRecord, &port, &group) == PBW_DIGEST_INVALID && group == PBW_DIGEST_G3D_LIST);
    CHECK(Present(&melonRecord) == 0 && melonRecord.hash[PBW_DIGEST_VRAM_A] == 0 &&
          melonRecord.hash[PBW_DIGEST_SCREEN_BOTTOM] == 0);
    CHECK(pbw_digest_record_frame(&portRecord, &port, &group) == PBW_DIGEST_INVALID && group == PBW_DIGEST_G3D_LIST);
    CHECK(pbw_digest_group_name(PBW_DIGEST_G3D_LIST) != NULL &&
          pbw_digest_group_find(pbw_digest_group_name(PBW_DIGEST_G3D_LIST)) == PBW_DIGEST_G3D_LIST);
    CHECK(pbw_digest_group_name(PBW_DIGEST_GROUPS) == NULL && pbw_digest_group_find("vram.J") == -1);
}

// A stream of the frames count..count+n-1, from one program's shape; palette names a count whose palettes are a
// step off
static FILE *WriteStream(int melonShape, uint32_t first, uint32_t n, uint32_t palette) {
    FILE *file = tmpfile();
    uint32_t count;

    CHECK(file != NULL);
    if (file == NULL) {
        return NULL;
    }
    CHECK(pbw_digest_write_header(file) == 0);
    for (count = first; count < first + n; count++) {
        PbwDigestFrame melon;
        PbwDigestFrame port;
        PbwDigestPolygon polygons[2][2];
        PbwDigestVertex melonVertices[6];
        PbwDigestRecord record;
        MakeFrames(&melon, &port, count, polygons, melonVertices);
        if (count == palette) {
            sPalettes[0][0x100]++;
            sPalettes[1][0x100]++;
        }
        memset(&record, 0, sizeof(record));
        record.count = count;
        record.frame = count * 2 + (melonShape ? 5 : 0);
        record.vblanks = melonShape ? 2 : 1;
        CHECK(pbw_digest_record_frame(&record, melonShape ? &melon : &port, NULL) == PBW_DIGEST_OK);
        CHECK(pbw_digest_write_record(file, &record) == 0);
    }
    rewind(file);
    return file;
}

// A stream from text
static FILE *TextStream(const char *text) {
    FILE *file = tmpfile();

    CHECK(file != NULL);
    if (file != NULL) {
        fputs(text, file);
        rewind(file);
    }
    return file;
}

static int Compare(FILE *expected, FILE *actual, PbwDigestComparison *comparison) {
    int result = -2;

    if (expected != NULL && actual != NULL) {
        result = pbw_digest_compare_streams(expected, actual, comparison);
    }
    if (expected != NULL) {
        fclose(expected);
    }
    if (actual != NULL) {
        fclose(actual);
    }
    return result;
}

static void TestStreams(void) {
    // The header of a stream with three groups, and a row of them
    static const char header[] = "#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n";
    PbwDigestComparison comparison;
    PbwDigestReader reader;
    PbwDigestRecord record;
    FILE *file;
    int everyGroupThrice = 1;
    int g;

    // The melonDS-shaped and port-shaped streams of the same frames are equal; the video frames and VBlanks differ.
    // Every group was compared at each of the three counts
    CHECK(Compare(WriteStream(1, 100, 3, 0), WriteStream(0, 100, 3, 0), &comparison) == 0);
    CHECK(!comparison.diverged && comparison.compared == 3 && comparison.onlyExpected == 0 &&
          comparison.onlyActual == 0);
    for (g = 0; g < PBW_DIGEST_GROUPS; g++) {
        everyGroupThrice &= comparison.groupCompared[g] == 3;
    }
    CHECK(everyGroupThrice && Flagged(comparison.differs) == 0 && Flagged(comparison.missing) == 0);
    // A palette a step off at count 101 is the first divergence, in pal alone
    CHECK(Compare(WriteStream(1, 100, 3, 0), WriteStream(0, 100, 3, 101), &comparison) == 0);
    CHECK(comparison.diverged && comparison.compared == 2 && OnlyGroup(comparison.differs, PBW_DIGEST_PAL));
    CHECK(comparison.expected.count == 101 && comparison.actual.count == 101 && comparison.expected.frame == 207 &&
          comparison.actual.frame == 202 && comparison.expected.vblanks == 2);
    // Counts that only one stream has are counted
    CHECK(Compare(WriteStream(1, 100, 4, 0), WriteStream(0, 101, 4, 0), &comparison) == 0);
    CHECK(!comparison.diverged && comparison.compared == 3 && comparison.onlyExpected == 1 &&
          comparison.onlyActual == 1);

    // A round trip: the record read is the one written
    file = WriteStream(0, 41, 1, 0);
    if (file != NULL) {
        PbwDigestFrame melon;
        PbwDigestFrame port;
        PbwDigestPolygon polygons[2][2];
        PbwDigestVertex melonVertices[6];
        PbwDigestRecord written;
        MakeFrames(&melon, &port, 41, polygons, melonVertices);
        CHECK(pbw_digest_record_frame(&written, &port, NULL) == PBW_DIGEST_OK);
        CHECK(pbw_digest_reader_open(&reader, file) == 0);
        CHECK(pbw_digest_reader_next(&reader, &record) == 1);
        CHECK(record.count == 41 && record.frame == 82 && record.vblanks == 1);
        CHECK(memcmp(record.present, written.present, sizeof(record.present)) == 0);
        CHECK(memcmp(record.hash, written.hash, sizeof(record.hash)) == 0);
        CHECK(pbw_digest_reader_next(&reader, &record) == 0);
        fclose(file);
    }

    // A header may leave groups out and order them as it likes; "-" is a group not taken, which isn't compared
    file = TextStream("#pbwdigest 1\ncount\tframe\tvblanks\toam\tpal\n5\t9\t1\t-\t00000000000000aB\n");
    if (file != NULL) {
        CHECK(pbw_digest_reader_open(&reader, file) == 0);
        CHECK(pbw_digest_reader_next(&reader, &record) == 1);
        CHECK(OnlyGroup(record.present, PBW_DIGEST_PAL) && record.hash[PBW_DIGEST_PAL] == 0xab);
        fclose(file);
    }
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\n7\t7\t1\t0000000000000001\t-\n"),
                  TextStream("#pbwdigest 1\ncount\tframe\tvblanks\toam\tpal\n7\t9\t1\t0000000000000002\t"
                             "0000000000000001\n"),
                  &comparison) == 0);
    CHECK(!comparison.diverged && comparison.compared == 1);
    CHECK(comparison.groupCompared[PBW_DIGEST_PAL] == 1 && comparison.groupCompared[PBW_DIGEST_OAM] == 0);

    // The expected stream names what is held. An actual stream that lacks a group the expected record has diverges
    // there, whether its header leaves the group out or its row writes "-" for it: a writer that dropped a group, or
    // couldn't take it, never passes
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n"
                             "7\t7\t1\t0000000000000001\t0000000000000002\n"),
                  TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\n7\t9\t1\t0000000000000001\n"),
                  &comparison) == 0);
    CHECK(comparison.diverged && comparison.compared == 1 && comparison.expected.count == 7);
    CHECK(OnlyGroup(comparison.missing, PBW_DIGEST_G3D_LIST) && Flagged(comparison.differs) == 0);
    CHECK(comparison.groupCompared[PBW_DIGEST_PAL] == 1 && comparison.groupCompared[PBW_DIGEST_G3D_LIST] == 0);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n"
                             "7\t7\t1\t0000000000000001\t0000000000000002\n"),
                  TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n7\t9\t1\t-\t0000000000000002\n"),
                  &comparison) == 0);
    CHECK(comparison.diverged && OnlyGroup(comparison.missing, PBW_DIGEST_PAL) && Flagged(comparison.differs) == 0);
    // A count where one group differs and another is missing names both
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n"
                             "7\t7\t1\t0000000000000001\t0000000000000002\n"),
                  TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n7\t9\t1\t-\t0000000000000003\n"),
                  &comparison) == 0);
    CHECK(comparison.diverged && OnlyGroup(comparison.missing, PBW_DIGEST_PAL) &&
          OnlyGroup(comparison.differs, PBW_DIGEST_G3D_LIST));
    // A group the expected stream leaves out or writes "-" for isn't compared, and groupCompared shows it never was
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\n7\t7\t1\t0000000000000001\t-\n"
                             "8\t8\t1\t0000000000000003\t-\n"),
                  TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tg3d.list\tsys\n"
                             "7\t9\t1\t0000000000000001\t0000000000000005\t0000000000000006\n"
                             "8\t10\t1\t0000000000000003\t0000000000000005\t-\n"),
                  &comparison) == 0);
    CHECK(!comparison.diverged && comparison.compared == 2 && comparison.groupCompared[PBW_DIGEST_PAL] == 2 &&
          comparison.groupCompared[PBW_DIGEST_G3D_LIST] == 0 && comparison.groupCompared[PBW_DIGEST_SYS] == 0);

    // Malformed streams: each one says where
    CHECK(Compare(TextStream("#pbwdigest 2\ncount\tframe\tvblanks\n"), TextStream(header), &comparison) == -1);
    CHECK(strstr(comparison.error, "expected stream") != NULL && strstr(comparison.error, "version") != NULL);
    CHECK(Compare(TextStream(header), TextStream("#pbwframes 2\n"), &comparison) == -1);
    CHECK(strstr(comparison.error, "actual stream") != NULL);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tvram.J\n"), TextStream(header), &comparison) ==
          -1);
    CHECK(strstr(comparison.error, "vram.J") != NULL);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\tpal\n"), TextStream(header), &comparison) ==
          -1);
    CHECK(Compare(TextStream("#pbwdigest 1\nframe\tcount\tvblanks\n"), TextStream(header), &comparison) == -1);
    CHECK(Compare(TextStream("#pbwdigest 1\n"), TextStream(header), &comparison) == -1);
    // A row with a field too few, a hash that isn't 16 hex digits, a count that isn't a number
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n1\t1\t1\t-\t-\n"), TextStream(header),
                  &comparison) == -1);
    CHECK(strstr(comparison.error, "line 3") != NULL);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n1\t1\t1\t-\t-\t123\n"),
                  TextStream(header), &comparison) == -1);
    CHECK(strstr(comparison.error, "sys") != NULL);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n1\t1\t1\t-\t-\t00000000000000001\n"),
                  TextStream(header), &comparison) == -1);
    CHECK(strstr(comparison.error, "not 16 hex digits") != NULL);
    CHECK(Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n-1\t1\t1\t-\t-\t-\n"),
                  TextStream(header), &comparison) == -1);
    // One row per count, in rising order: a count written twice is the pairing's to resolve, not this comparison's, and
    // a count that falls, 5 then 3, is as malformed, in either stream
    CHECK(Compare(
              TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n3\t1\t1\t-\t-\t-\n3\t2\t1\t-\t-\t-\n"),
              TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n3\t1\t1\t-\t-\t-\n"), &comparison) == -1);
    CHECK(strstr(comparison.error, "rising order") != NULL);
    CHECK(Compare(
              TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n5\t1\t1\t-\t-\t-\n3\t2\t1\t-\t-\t-\n"),
              TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n5\t1\t1\t-\t-\t-\n"), &comparison) == -1);
    CHECK(strstr(comparison.error, "expected stream") != NULL &&
          strstr(comparison.error, "count 3 after count 5") != NULL);
    CHECK(
        Compare(TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n5\t1\t1\t-\t-\t-\n6\t2\t1\t-\t-\t-\n"),
                TextStream("#pbwdigest 1\ncount\tframe\tvblanks\tpal\toam\tsys\n5\t1\t1\t-\t-\t-\n3\t2\t1\t-\t-\t-\n"),
                &comparison) == -1);
    CHECK(strstr(comparison.error, "actual stream") != NULL &&
          strstr(comparison.error, "count 3 after count 5") != NULL);
}

int main(void) {
    TestHash();
    TestRegs2dMasks();
    TestRegs2dFields();
    TestRegs2dSignExtension();
    TestRegs2dBldy();
    TestRegs2dWindows();
    TestSysAndMap();
    TestRegs3d();
    TestDepth();
    TestStripDereferenced();
    TestWZero();
    TestListWords();
    TestListMasks();
    TestStats();
    TestCaptureZeroed();
    TestCaptureGroup();
    TestScreens();
    TestPaletteAndOam();
    TestRecords();
    TestStreams();
    if (sFailed != 0) {
        fprintf(stderr, "%d of %d checks failed\n", sFailed, sChecks);
        return 1;
    }
    printf("%d checks passed\n", sChecks);
    return 0;
}
