// Unit tests of port/gfx/ref, the CPU reference 2D renderer. Each test builds registers, VRAM, palettes and OAM by
// hand for one feature and checks exact pixels. The expected values are worked out by hand in the comments, from
// GBATEK's rules and, where ds2d.c says it follows melonDS's measurements, from those; none comes from running the
// renderer. Colours: a 15-bit channel c5 enters the engine as 6-bit c5 * 2 (bit 15 is ignored, as melonDS 1.1 does),
// and OUT6() widens the 6-bit result as the output does, (c6 << 2) | (c6 >> 4). No ROM is needed. It prints each
// failure and returns nonzero if there was one.
#include "gfx/ref/ds2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W8(c) ((((c) << 2) | ((c) >> 4)) & 0xFFu)
#define OUT6(r, g, b) (0xFF000000u | W8(r) << 16 | W8(g) << 8 | W8(b))
#define BLACK OUT6(0, 0, 0)
#define WHITE 0xFFFFFFFFu

typedef struct Scene {
    uint8_t io[0x80];
    uint8_t palette[1024];
    uint8_t oam[1024];
    uint8_t bg[512 * 1024];
    uint8_t obj[256 * 1024];
    uint8_t bgExt[4][8192];
    uint8_t objExt[8192];
    uint8_t lcdc[41 * 16384];
    uint32_t line3d[256];
    PortDs2dSource src;
    uint32_t out[256];
} Scene;

static Scene *s;
static int sEngine;
static int sLine;
static int sChecks;
static int sFailures;

static uint16_t Rgb(unsigned r, unsigned g, unsigned b) {
    return (uint16_t)(r | g << 5 | b << 10);
}

static void Put16(uint8_t *mem, uint32_t offset, uint16_t value) {
    mem[offset] = (uint8_t)value;
    mem[offset + 1] = (uint8_t)(value >> 8);
}

static void Io16(unsigned offset, uint16_t value) {
    Put16(s->io, offset, value);
}

static void Io32(unsigned offset, uint32_t value) {
    Put16(s->io, offset, (uint16_t)value);
    Put16(s->io, offset + 2, (uint16_t)(value >> 16));
}

static void BgPal(unsigned index, uint16_t color) {
    Put16(s->palette, index * 2, color);
}

static void ObjPal(unsigned index, uint16_t color) {
    Put16(s->palette, 512 + index * 2, color);
}

static void SetObj(unsigned i, uint16_t attr0, uint16_t attr1, uint16_t attr2) {
    Put16(s->oam, i * 8, attr0);
    Put16(s->oam, i * 8 + 2, attr1);
    Put16(s->oam, i * 8 + 4, attr2);
}

// Affine parameter group g: PA..PD are attribute 3 of OAM entries 4g..4g+3
static void SetObjAffine(unsigned g, int16_t pa, int16_t pb, int16_t pc, int16_t pd) {
    Put16(s->oam, g * 32 + 6, (uint16_t)pa);
    Put16(s->oam, g * 32 + 14, (uint16_t)pb);
    Put16(s->oam, g * 32 + 22, (uint16_t)pc);
    Put16(s->oam, g * 32 + 30, (uint16_t)pd);
}

// A 16-colour tile of one colour index
static void FillTile4(uint8_t *mem, uint32_t address, unsigned index) {
    memset(mem + address, (int)(index | index << 4), 32);
}

static void FillTile8(uint8_t *mem, uint32_t address, unsigned index) {
    memset(mem + address, (int)index, 64);
}

static void Setup(int engine) {
    int i;

    memset(s, 0, sizeof(*s));
    sEngine = engine;
    s->src.io = s->io;
    s->src.palette = s->palette;
    s->src.oam = s->oam;
    for (i = 0; i < (engine == 0 ? 32 : 8); i++) {
        s->src.bg_slots[i] = s->bg + i * 0x4000;
    }
    for (i = 0; i < (engine == 0 ? 16 : 8); i++) {
        s->src.obj_slots[i] = s->obj + i * 0x4000;
    }
    for (i = 0; i < 4; i++) {
        s->src.bg_ext_palette[i] = s->bgExt[i];
    }
    s->src.obj_ext_palette = s->objExt;
    for (i = 0; i < 41; i++) {
        s->src.lcdc_slots[i] = s->lcdc + i * 0x4000;
    }
    for (i = 0; i < 128; i++) {
        SetObj((unsigned)i, 0x0200, 0, 0); // every OBJ disabled
    }
    port_ds2d_reset(engine);
    port_ds2d_set_obj_cycle_budget(0, 0);
}

static void Line(int line) {
    sLine = line;
    port_ds2d_render_line(&s->src, sEngine, line, s->out);
}

// Renders a new frame from line 0 up to lastLine
static void Frame(int lastLine) {
    int line;

    port_ds2d_begin_frame(sEngine);
    for (line = 0; line <= lastLine; line++) {
        Line(line);
    }
}

static void Expect(const char *name, int x, uint32_t expected) {
    sChecks++;
    if (s->out[x] != expected) {
        printf("FAIL %s: engine %c line %d x %d is 0x%08x, expected 0x%08x\n", name, 'A' + sEngine, sLine, x,
               (unsigned)s->out[x], (unsigned)expected);
        sFailures++;
    }
}

static void ExpectRange(const char *name, int x0, int x1, uint32_t expected) {
    int x;

    for (x = x0; x <= x1; x++) {
        sChecks++;
        if (s->out[x] != expected) {
            printf("FAIL %s: engine %c line %d x %d (of %d..%d) is 0x%08x, expected 0x%08x\n", name, 'A' + sEngine,
                   sLine, x, x0, x1, (unsigned)s->out[x], (unsigned)expected);
            sFailures++;
            return;
        }
    }
}

static void ExpectTrue(const char *name, int condition) {
    sChecks++;
    if (!condition) {
        printf("FAIL %s\n", name);
        sFailures++;
    }
}

// --- Display modes, conversion, master brightness ---

static void TestDisplayOff(void) {
    int engine;

    for (engine = 0; engine < 2; engine++) {
        Setup(engine);
        Io32(0x00, 0);      // display mode 0
        Io16(0x6C, 0x8010); // master brightness down 16, which display off ignores
        BgPal(0, Rgb(5, 5, 5));
        Line(0);
        ExpectRange("display off is white", 0, 255, WHITE);
    }
}

static void TestBackdropAndConversion(void) {
    Setup(0);
    Io32(0x00, 0x10000); // mode 1, no layers
    BgPal(0, 0x7FFF);    // white: 62 per channel, widened to 0xFB
    Line(0);
    ExpectRange("15-bit white is 0xFB", 0, 255, 0xFFFBFBFBu);
    BgPal(0, 0x83E0); // green 31 with bit 15: still 62
    Line(1);
    ExpectRange("bit 15 doesn't reach green", 0, 255, 0xFF00FB00u);
    BgPal(0, 0x0001); // red 1 -> 2 -> 8
    Line(2);
    ExpectRange("red 1", 0, 255, 0xFF080000u);
    BgPal(0, Rgb(16, 8, 4)); // 32 -> 0x82, 16 -> 0x41, 8 -> 0x20
    Line(3);
    ExpectRange("mixed", 0, 255, 0xFF824120u);

    Setup(1);
    Io32(0x00, 0x10000);
    BgPal(0, Rgb(0, 0, 31));
    Line(0);
    ExpectRange("engine B backdrop", 0, 255, OUT6(0, 0, 62));
}

static void TestForcedBlank(void) {
    Setup(0);
    Io32(0x00, 0x10180); // mode 1, forced blank, BG0 on
    Io16(0x6C, 0x8008);  // down 8: 63 - ((63 * 8 + 15) >> 4) = 63 - 32 = 31
    Line(0);
    ExpectRange("forced blank is white, then master brightness", 0, 255, OUT6(31, 31, 31));
    ExpectTrue("OUT6(31,31,31) is 0xFF7D7D7D", OUT6(31, 31, 31) == 0xFF7D7D7Du);
}

static void TestMasterBrightness(void) {
    int engine;

    for (engine = 0; engine < 2; engine++) {
        Setup(engine);
        Io32(0x00, 0x10000);
        BgPal(0, Rgb(10, 20, 31)); // (20, 40, 62)
        // Up 4: 20 + (43 * 4 >> 4) = 30; 40 + (23 * 4 >> 4) = 45; 62 + (1 * 4 >> 4) = 62
        Io16(0x6C, 0x4004);
        Line(0);
        ExpectRange("brightness up 4", 0, 255, OUT6(30, 45, 62));
        // Up 31 counts as 16: every channel 63
        Io16(0x6C, 0x401F);
        Line(1);
        ExpectRange("brightness up 31 = 16", 0, 255, WHITE);
        // Down 4: 20 - ((80 + 15) >> 4) = 15; 40 - ((160 + 15) >> 4) = 30; 62 - ((248 + 15) >> 4) = 46
        Io16(0x6C, 0x8004);
        Line(2);
        ExpectRange("brightness down 4", 0, 255, OUT6(15, 30, 46));
        // Down 1: 20 - (35 >> 4) = 18; 40 - (55 >> 4) = 37; 62 - (77 >> 4) = 58
        Io16(0x6C, 0x8001);
        Line(3);
        ExpectRange("brightness down 1", 0, 255, OUT6(18, 37, 58));
        Io16(0x6C, 0xC010); // mode 3: none
        Line(4);
        ExpectRange("brightness mode 3", 0, 255, OUT6(20, 40, 62));
        Io16(0x6C, 0x0010); // mode 0: none
        Line(5);
        ExpectRange("brightness mode 0", 0, 255, OUT6(20, 40, 62));
    }
}

static void TestVramDisplay(void) {
    int i;

    Setup(0);
    Io32(0x00, 0x20000 | 1 << 18);                     // mode 2 from bank B (LCDC 0x06820000)
    Put16(s->lcdc, 0x20000 + 3 * 512 + 5 * 2, 0x801F); // line 3, x 5; bit 15 is ignored here
    Put16(s->lcdc, 0x20000 + 3 * 512 + 6 * 2, Rgb(0, 31, 0));
    Frame(3);
    Expect("VRAM display", 5, OUT6(62, 0, 0));
    Expect("VRAM display", 6, OUT6(0, 62, 0));
    Expect("VRAM display", 4, BLACK);
    Io16(0x6C, 0x4010); // master brightness applies
    Line(4);
    Line(5);
    ExpectRange("VRAM display brightness", 0, 255, WHITE);

    Io16(0x6C, 0);
    Io32(0x00, 0x20000 | 3 << 18); // bank D
    Put16(s->lcdc, 0x60000 + 7 * 512, Rgb(0, 0, 31));
    Frame(7);
    Expect("VRAM display bank D", 0, OUT6(0, 0, 62));
    for (i = 24; i < 32; i++) {
        s->src.lcdc_slots[i] = NULL; // bank D not in LCDC
    }
    Frame(7);
    Expect("VRAM display unmapped", 0, BLACK);

    Setup(1);
    Io32(0x00, 0x20000); // engine B has only bit 16: mode 0
    Line(0);
    ExpectRange("engine B mode 2 is mode 0", 0, 255, WHITE);
}

static void TestMainMemoryDisplay(void) {
    Setup(0);
    Io32(0x00, 0x30000);
    BgPal(0, Rgb(31, 31, 31));
    Line(0);
    ExpectRange("main memory display is black", 0, 255, BLACK);
    Setup(1);
    Io32(0x00, 0x30000); // engine B: bit 16 only, mode 1
    BgPal(0, Rgb(31, 0, 0));
    Line(0);
    ExpectRange("engine B mode 3 is mode 1", 0, 255, OUT6(62, 0, 0));
}

// --- Text BGs ---

// BG0, 16 colours, char base 1 (0x4000), screen base 2 (0x1000). Palette 3 colour i is red i. Tile 5: row 0 has
// indices 1..8, row 7 all 15; tile 6 all 9; tile 7 all 4. Map (0,0) is tile 5 and (31,0) tile 6, palette 3
static void SetupText4(void) {
    unsigned i;

    Setup(0);
    Io32(0x00, 0x10100);
    Io16(0x08, 1 << 2 | 2 << 8);
    for (i = 1; i < 16; i++) {
        BgPal(3 * 16 + i, Rgb(i, 0, 0));
    }
    s->bg[0x4000 + 5 * 32 + 0] = 0x21;
    s->bg[0x4000 + 5 * 32 + 1] = 0x43;
    s->bg[0x4000 + 5 * 32 + 2] = 0x65;
    s->bg[0x4000 + 5 * 32 + 3] = 0x87;
    memset(s->bg + 0x4000 + 5 * 32 + 7 * 4, 0xFF, 4);
    FillTile4(s->bg, 0x4000 + 6 * 32, 9);
    FillTile4(s->bg, 0x4000 + 7 * 32, 4);
    Put16(s->bg, 0x1000, 5 | 3 << 12);
    Put16(s->bg, 0x1000 + 31 * 2, 6 | 3 << 12);
}

static void TestText4(void) {
    int x;

    SetupText4();
    Line(0);
    for (x = 0; x < 8; x++) {
        Expect("text 4bpp", x, OUT6(2 * (x + 1), 0, 0));
    }
    ExpectRange("text 4bpp empty", 8, 247, BLACK);
    ExpectRange("text 4bpp tile 6", 248, 255, OUT6(18, 0, 0));

    Put16(s->bg, 0x1000, 5 | 0x400 | 3 << 12); // hflip: index 8 - x
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("text hflip", x, OUT6(2 * (8 - x), 0, 0));
    }
    Put16(s->bg, 0x1000, 5 | 0x800 | 3 << 12); // vflip: line 0 shows row 7, line 7 row 0
    Frame(7);
    for (x = 0; x < 8; x++) {
        Expect("text vflip row 0", x, OUT6(2 * (x + 1), 0, 0));
    }
    Frame(0);
    ExpectRange("text vflip row 7", 0, 7, OUT6(30, 0, 0));

    // HOFS 250: dots 0..5 are BG x 250..255 (tile 6), dots 6..13 tile 5
    Put16(s->bg, 0x1000, 5 | 3 << 12);
    Io16(0x10, 250);
    Frame(0);
    ExpectRange("text scroll wrap", 0, 5, OUT6(18, 0, 0));
    for (x = 6; x < 14; x++) {
        Expect("text scroll", x, OUT6(2 * (x - 5), 0, 0));
    }
    Expect("text scroll", 14, BLACK);
    // VOFS 504: line 8 is y (8 + 504) & 255 = 0; line 0 is y 248, an empty map row
    Io16(0x10, 0);
    Io16(0x12, 504);
    Frame(8);
    Expect("text vscroll wrap", 0, OUT6(2, 0, 0));
    Frame(0);
    Expect("text vscroll empty row", 0, BLACK);

    // 512x512: blocks 0 TL, 1 TR (+0x800), 2 BL (+0x1000), 3 BR (+0x1800)
    SetupText4();
    Io16(0x08, 1 << 2 | 2 << 8 | 3 << 14);
    Put16(s->bg, 0x1000 + 0x800, 5 | 3 << 12);
    Put16(s->bg, 0x1000 + 0x1000, 7 | 3 << 12);
    Put16(s->bg, 0x1000 + 0x1800, 6 | 3 << 12);
    Io16(0x10, 256);
    Frame(0);
    Expect("512x512 block 1", 0, OUT6(2, 0, 0));
    Expect("512x512 block 1", 7, OUT6(16, 0, 0));
    Io16(0x10, 0);
    Io16(0x12, 256);
    Frame(0);
    ExpectRange("512x512 block 2", 0, 7, OUT6(8, 0, 0));
    Io16(0x10, 256);
    Frame(0);
    ExpectRange("512x512 block 3", 0, 7, OUT6(18, 0, 0));
    // 256x512: block 1 is the lower half; HOFS 256 wraps to x 0
    Io16(0x08, 1 << 2 | 2 << 8 | 2 << 14);
    Io16(0x12, 256);
    Frame(0);
    Expect("256x512 lower half", 0, OUT6(2, 0, 0));
    // 512x256: block 1 is the right half; VOFS 256 wraps to y 0
    Io16(0x08, 1 << 2 | 2 << 8 | 1 << 14);
    Frame(0);
    Expect("512x256 right half", 0, OUT6(2, 0, 0));
}

// BG1, 256 colours: tile 2 row 0 has indices 0x10, 0x20, 0, 0, 0, 0, 0, 0xFF; map (0,0) is tile 2, palette 2
static void SetupText8(void) {
    Setup(0);
    Io32(0x00, 0x10200);
    Io16(0x0A, 0x80 | 1 << 2 | 2 << 8);
    s->bg[0x4000 + 2 * 64 + 0] = 0x10;
    s->bg[0x4000 + 2 * 64 + 1] = 0x20;
    s->bg[0x4000 + 2 * 64 + 7] = 0xFF;
    Put16(s->bg, 0x1000, 2 | 2 << 12);
    BgPal(0x10, Rgb(1, 2, 3));
    BgPal(0x20, Rgb(4, 5, 6));
    BgPal(0xFF, Rgb(7, 8, 9));
    Put16(s->bgExt[1], (2 * 256 + 0x10) * 2, Rgb(10, 11, 12));
    Put16(s->bgExt[1], (2 * 256 + 0x20) * 2, Rgb(13, 14, 15));
    Put16(s->bgExt[3], (2 * 256 + 0x10) * 2, Rgb(30, 29, 28));
}

static void TestText8(void) {
    SetupText8();
    Line(0);
    Expect("text 8bpp", 0, OUT6(2, 4, 6));
    Expect("text 8bpp", 1, OUT6(8, 10, 12));
    Expect("text 8bpp transparent", 2, BLACK);
    Expect("text 8bpp", 7, OUT6(14, 16, 18));
    Io32(0x00, 0x10200 | 1u << 30); // extended palettes: BG1 reads slot 1, palette 2
    Frame(0);
    Expect("text 8bpp ext slot 1", 0, OUT6(20, 22, 24));
    Expect("text 8bpp ext slot 1", 1, OUT6(26, 28, 30));
    Io16(0x0A, 0x80 | 1 << 2 | 2 << 8 | 1 << 13); // BG1CNT bit 13: slot 3
    Frame(0);
    Expect("text 8bpp ext slot 3", 0, OUT6(60, 58, 56));
    s->src.bg_ext_palette[3] = NULL; // unmapped: colour 0 is black, still opaque
    BgPal(0, Rgb(31, 31, 31));
    Frame(0);
    Expect("text 8bpp ext unmapped", 0, BLACK);
    Expect("text 8bpp ext unmapped, transparent", 2, OUT6(62, 62, 62));

    // Flips of a 256-colour tile. Row 7 is 0x20 then seven 0s. Vflip: line 0 shows row 7
    SetupText8();
    s->bg[0x4000 + 2 * 64 + 7 * 8] = 0x20;
    Put16(s->bg, 0x1000, 2 | 0x800 | 2 << 12);
    Line(0);
    Expect("text 8bpp vflip", 0, OUT6(8, 10, 12));
    Expect("text 8bpp vflip", 1, BLACK);
    Expect("text 8bpp vflip", 7, BLACK);
    // Hflip: row 0 backwards, 0xFF, 0, 0, 0, 0, 0, 0x20, 0x10
    Put16(s->bg, 0x1000, 2 | 0x400 | 2 << 12);
    Frame(0);
    Expect("text 8bpp hflip", 0, OUT6(14, 16, 18));
    Expect("text 8bpp hflip", 1, BLACK);
    Expect("text 8bpp hflip", 6, OUT6(8, 10, 12));
    Expect("text 8bpp hflip", 7, OUT6(2, 4, 6));

    // BG3 in 256 colours reads extended palette slot 3 (slot 2 holds another colour)
    SetupText8();
    Io32(0x00, 0x10800 | 1u << 30);
    Io16(0x0A, 0);
    Io16(0x0E, 0x80 | 1 << 2 | 2 << 8);
    Put16(s->bgExt[2], (2 * 256 + 0x10) * 2, Rgb(1, 1, 1));
    Line(0);
    Expect("text 8bpp BG3 ext slot 3", 0, OUT6(60, 58, 56));
    Io32(0x00, 0x10800);
    Frame(0);
    Expect("text 8bpp BG3 standard palette", 0, OUT6(2, 4, 6));
}

static void TestBases(void) {
    // Engine A: DISPCNT char base 1 (+0x10000) and screen base 2 (+0x20000) add to BGxCNT's
    Setup(0);
    Io32(0x00, 0x10100 | 1u << 24 | 2u << 27);
    Io16(0x08, 0);
    Put16(s->bg, 0x20000, 1);
    FillTile4(s->bg, 0x10000 + 32, 3);
    BgPal(3, Rgb(0, 31, 0));
    Line(0);
    ExpectRange("engine A DISPCNT bases", 0, 7, OUT6(0, 62, 0));

    // Engine B has no DISPCNT bases (and no display mode bit 17)
    Setup(1);
    Io32(0x00, 0x30100 | 1u << 24 | 2u << 27);
    Io16(0x08, 0);
    Put16(s->bg, 0, 1);
    FillTile4(s->bg, 32, 5);
    BgPal(5, Rgb(0, 0, 31));
    Line(0);
    ExpectRange("engine B ignores DISPCNT bases", 0, 7, OUT6(0, 0, 62));
    // Engine B's 128 KiB view wraps: char base 15 (0x3C000) reads 0x1C000
    Io16(0x08, 15 << 2 | 31 << 8);
    Put16(s->bg, 0xF800, 1);
    FillTile4(s->bg, 0x1C000 + 32, 5);
    Frame(0);
    ExpectRange("engine B wraps at 128 KiB", 0, 7, OUT6(0, 0, 62));
}

// --- Affine, extended and large BGs ---

// BG palette index i is (red i % 32, green i / 32)
static uint32_t Index(unsigned i) {
    return OUT6(2 * (i & 31), 2 * (i >> 5), 0);
}

static void SetIndexPalette(void) {
    unsigned i;

    for (i = 1; i < 256; i++) {
        BgPal(i, Rgb(i & 31, i >> 5, 0));
    }
}

// Mode 2, BG2 affine 128x128, map at 0, char base 1 (0x4000). Tile 1's dot (c, r) is index r * 16 + c + 1; tile 0 is
// all 200; map (0,0) is tile 1. Identity parameters, reference point (0, 0)
static void SetupAffine(void) {
    unsigned r;
    unsigned c;

    Setup(0);
    Io32(0x00, 0x10402);
    Io16(0x0C, 1 << 2);
    SetIndexPalette();
    FillTile8(s->bg, 0x4000, 200);
    for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            s->bg[0x4000 + 64 + r * 8 + c] = (uint8_t)(r * 16 + c + 1);
        }
    }
    s->bg[0] = 1;
    Io16(0x20, 0x100);
    Io16(0x26, 0x100);
}

static void TestAffine(void) {
    int x;

    SetupAffine();
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("affine identity", x, Index((unsigned)x + 1));
    }
    ExpectRange("affine tile 0", 8, 127, Index(200));
    ExpectRange("affine outside is transparent", 128, 255, BLACK);
    Io16(0x0C, 1 << 2 | 1 << 13); // wrap around
    Frame(0);
    for (x = 128; x < 136; x++) {
        Expect("affine wrap", x, Index((unsigned)x - 127));
    }
    Expect("affine wrap", 136, Index(200));

    // PA 0.5: dot x reads x / 2
    SetupAffine();
    Io16(0x20, 0x80);
    Frame(0);
    for (x = 0; x < 16; x++) {
        Expect("affine zoom", x, Index((unsigned)x / 2 + 1));
    }
    Expect("affine zoom", 16, Index(200));

    // Reference point (3, 2), PB 0, PD 1: line 0 reads (3, 2) = 36, line 1 (3, 3) = 52
    SetupAffine();
    Io32(0x28, 0x300);
    Io32(0x2C, 0x200);
    Frame(0);
    Expect("affine reference", 0, Index(36));
    Line(1);
    Expect("affine PD", 0, Index(52));
    // PB 1 as well: line 2 reads (5, 4) = 70
    SetupAffine();
    Io32(0x28, 0x300);
    Io32(0x2C, 0x200);
    Io16(0x22, 0x100);
    Frame(2);
    Expect("affine PB", 0, Index(70));
    // A write before line 3 (X = 0) starts it there: (0, 5) = 81; line 4 advances: (1, 6) = 98
    Io32(0x28, 0);
    Line(3);
    Expect("affine reference write", 0, Index(81));
    Line(4);
    Expect("affine after the write", 0, Index(98));
    // The same value again isn't seen by itself: line 5 is (2, 7) = 115
    Line(5);
    Expect("affine same value, no reload", 0, Index(115));
    // A new frame latches the registers again: (0, 2) = 33
    Frame(0);
    Expect("affine frame latch", 0, Index(33));
    // ...unless the platform layer reports the write: the same frame with the report before line 5 restarts it at
    // (0, 7) = 113
    SetupAffine();
    Io32(0x28, 0x300);
    Io32(0x2C, 0x200);
    Io16(0x22, 0x100);
    Frame(2);
    Io32(0x28, 0);
    Line(3);
    Line(4);
    port_ds2d_reload_reference(0, 2, 0);
    Line(5);
    Expect("affine reported reload", 0, Index(113));

    // Reference -1.0 (28-bit): dot 0 is outside, dot 1 reads column 0
    SetupAffine();
    Io32(0x28, 0x0FFFFF00);
    Frame(0);
    Expect("affine negative reference", 0, BLACK);
    Expect("affine negative reference", 1, Index(1));

    // A skipped line still advances: line 5 after line 0 reads row 5 = 81
    SetupAffine();
    port_ds2d_begin_frame(0);
    Line(0);
    Line(5);
    Expect("affine skipped lines", 0, Index(81));

    // While BG2 is off its reference point doesn't advance: line 2 reads row 1 = 17
    SetupAffine();
    port_ds2d_begin_frame(0);
    Line(0);
    Io32(0x00, 0x10002);
    Line(1);
    Io32(0x00, 0x10402);
    Line(2);
    Expect("affine off doesn't advance", 0, Index(17));

    // A rotation by 90 degrees: PA 0, PB -1, PC 1, PD 0 from (3, 0). Dot x of line 0 reads (3, x): row x of tile 1,
    // index x * 16 + 4, for x 0..7, then tile 0 down to y 127 and nothing below it. Each line moves X by PB: line 1
    // reads column 2, line 3 column 0, and line 4 column -1, outside
    SetupAffine();
    Io16(0x20, 0);
    Io16(0x22, (uint16_t)-0x100);
    Io16(0x24, 0x100);
    Io16(0x26, 0);
    Io32(0x28, 0x300);
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("affine rotation PC", x, Index((unsigned)x * 16 + 4));
    }
    ExpectRange("affine rotation PC", 8, 127, Index(200));
    ExpectRange("affine rotation PC outside", 128, 255, BLACK);
    Line(1);
    for (x = 0; x < 8; x++) {
        Expect("affine rotation line 1", x, Index((unsigned)x * 16 + 3));
    }
    Line(2);
    Line(3);
    for (x = 0; x < 8; x++) {
        Expect("affine rotation line 3", x, Index((unsigned)x * 16 + 1));
    }
    Line(4);
    ExpectRange("affine rotation line 4 outside", 0, 255, BLACK);

    // PA and PC together, the diagonal: PA 1, PC 1, PD 1. Line 0 dot x reads (x, x), index x * 17 + 1 in tile 1;
    // line 2 reads (x, x + 2), index x * 17 + 33 for x 0..5, and dot 6 reads (6, 8), tile 0
    SetupAffine();
    Io16(0x24, 0x100);
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("affine diagonal", x, Index((unsigned)x * 17 + 1));
    }
    Expect("affine diagonal", 8, Index(200));
    Line(1);
    Line(2);
    for (x = 0; x < 6; x++) {
        Expect("affine diagonal line 2", x, Index((unsigned)x * 17 + 33));
    }
    Expect("affine diagonal line 2", 6, Index(200));

    // BG3 has its own parameters (0x30..0x3F): a rotation by -90 degrees, PA 0, PB 1, PC -1, PD 0 from (0, 7). Line 0
    // dot x reads (0, 7 - x), index (7 - x) * 16 + 1, and dot 8 reads y -1, outside; line 1 reads column 1
    SetupAffine();
    Io32(0x00, 0x10802);
    Io16(0x0E, 1 << 2);
    Io16(0x32, 0x100);
    Io16(0x34, (uint16_t)-0x100);
    Io32(0x3C, 0x700);
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("affine BG3 rotation", x, Index((unsigned)(7 - x) * 16 + 1));
    }
    ExpectRange("affine BG3 rotation outside", 8, 255, BLACK);
    Line(1);
    for (x = 0; x < 8; x++) {
        Expect("affine BG3 rotation line 1", x, Index((unsigned)(7 - x) * 16 + 2));
    }

    // The reference registers have 28 bits: bits 28-31 are ignored, so 0xF0000100 is Y 1.0, row 1 (17)
    SetupAffine();
    Io32(0x2C, 0xF0000100u);
    Frame(0);
    Expect("affine reference bits 28-31", 0, Index(17));
    // Y 0x07FFFF00 (524287.0, the largest) plus PD 1 wraps to -524288.0. With wrap around, line 0 reads row
    // 524287 & 127 = 127 (tile 0) and line 1 row 0. The BG sizes all divide 2^20 dots, so the picture is the same as if
    // the point hadn't wrapped: this checks the edge, not the wrap
    Io16(0x0C, 1 << 2 | 1 << 13);
    Io32(0x2C, 0x07FFFF00);
    Frame(0);
    ExpectRange("affine reference at the top", 0, 7, Index(200));
    Line(1);
    for (x = 0; x < 8; x++) {
        Expect("affine reference past the top", x, Index((unsigned)x + 1));
    }

    // Engine A's DISPCNT character base (1, +0x10000) and screen base (2, +0x20000) apply to affine BGs too. The map at
    // 0x20000 starts with tile 2, whose dots at 0x14080 are all 77; tile 1 there is all 55 and tile 0 all 99. Without
    // the bases the map at 0 (tile 1) or the tiles at 0x4000 (no tile 2) would show
    SetupAffine();
    Io32(0x00, 0x10402 | 1u << 24 | 2u << 27);
    s->bg[0x20000] = 2;
    FillTile8(s->bg, 0x14000, 99);
    FillTile8(s->bg, 0x14000 + 64, 55);
    FillTile8(s->bg, 0x14000 + 128, 77);
    Frame(0);
    ExpectRange("affine DISPCNT bases", 0, 7, Index(77));
    ExpectRange("affine DISPCNT bases", 8, 127, Index(99));
    // The same in mode 5 with 16-bit map entries
    Io32(0x00, 0x10405 | 1u << 24 | 2u << 27);
    Put16(s->bg, 0x20000, 2);
    Frame(0);
    ExpectRange("extended map16 DISPCNT bases", 0, 7, Index(77));
    ExpectRange("extended map16 DISPCNT bases", 8, 127, Index(99));

    // A line rendered again starts from the state it started with: with PB 1 set before line 4, line 4 reads (0, 4)
    // (65) both times, and line 5 (1, 5) (82); a frame restarted at line 4 would read (4, 4)
    SetupAffine();
    Frame(3);
    Io16(0x22, 0x100);
    Line(4);
    Expect("line rendered", 0, Index(65));
    Line(4);
    Expect("line rendered again", 0, Index(65));
    Line(5);
    Expect("line after a line rendered again", 0, Index(82));
    // A reference write between the two counts for the second: with PB 1 from the start, line 4 reads (4, 4) (69);
    // X = 2 then gives (2, 4) (67) on the second render and (3, 5) (84) on line 5
    SetupAffine();
    Io16(0x22, 0x100);
    Frame(4);
    Expect("line rendered", 0, Index(69));
    Io32(0x28, 0x200);
    Line(4);
    Expect("line rendered again after a write", 0, Index(67));
    Line(5);
    Expect("line after a write and a line rendered again", 0, Index(84));
    // A lower line starts a new frame: line 2 after line 5 latches the registers again and reads (0, 2) (33)
    SetupAffine();
    Frame(5);
    Line(2);
    Expect("lower line starts a frame", 0, Index(33));
}

static void TestExtended(void) {
    unsigned r;
    unsigned c;
    int x;

    // Mode 5 BG2 with 16-bit entries: map (0,0) = tile 1, hflip, palette 5; extended slot 2 palette 5 colour i is
    // (i % 32, 0, 31)
    SetupAffine();
    Io32(0x00, 0x10405 | 1u << 30);
    Io16(0x0C, 1 << 2);
    s->bg[0] = 0;
    Put16(s->bg, 0, 1 | 0x400 | 5 << 12);
    for (r = 1; r < 256; r++) {
        Put16(s->bgExt[2], (5 * 256 + r) * 2, Rgb(r & 31, 0, 31));
    }
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("extended map16 hflip, ext palette", x, OUT6(2 * (8 - x), 0, 62));
    }
    Put16(s->bg, 0, 1 | 0x800 | 5 << 12); // vflip: row 7, index 113 + c
    Frame(0);
    Expect("extended map16 vflip", 0, OUT6(2 * (113 & 31), 0, 62));
    Io32(0x00, 0x10405); // standard palette
    Frame(0);
    Expect("extended map16 standard palette", 0, Index(113));
    // Wrap around (BG2CNT bit 13) from X 120: dots 0..7 read map column 15 (tile 0, 200), dot 8 reads x 128, which
    // wraps to 0 (tile 1, row 0, index 1), and dot 16 x 8 (tile 0); without it, dot 8 is outside
    Put16(s->bg, 0, 1);
    Io32(0x28, 120 << 8);
    Io16(0x0C, 1 << 2 | 1 << 13);
    Frame(0);
    ExpectRange("extended map16 wrap", 0, 7, Index(200));
    Expect("extended map16 wrap", 8, Index(1));
    Expect("extended map16 wrap", 16, Index(200));
    Io16(0x0C, 1 << 2);
    Frame(0);
    ExpectRange("extended map16 no wrap", 8, 255, BLACK);

    // BG3 with 16-bit map entries reads extended palette slot 3: palette 5 colour i is (i % 32, 31, 0) there, and
    // (i % 32, 0, 31) in slot 2
    SetupAffine();
    Io32(0x00, 0x10805 | 1u << 30);
    Io16(0x0E, 1 << 2);
    Io16(0x30, 0x100);
    Io16(0x36, 0x100);
    Put16(s->bg, 0, 1 | 5 << 12);
    for (r = 1; r < 256; r++) {
        Put16(s->bgExt[2], (5 * 256 + r) * 2, Rgb(r & 31, 0, 31));
        Put16(s->bgExt[3], (5 * 256 + r) * 2, Rgb(r & 31, 31, 0));
    }
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("extended map16 BG3 ext slot 3", x, OUT6(2 * (x + 1), 62, 0));
    }

    // BG3 256-colour bitmap 256x256 at BG3CNT screen base 2 * 16 KiB; DISPCNT's screen base doesn't apply
    Setup(0);
    SetIndexPalette();
    Io32(0x00, 0x10805 | 3u << 27);
    Io16(0x0E, 0x80 | 2 << 8 | 1 << 14);
    Io16(0x30, 0x100);
    Io16(0x36, 0x100);
    for (c = 0; c < 255; c++) {
        s->bg[0x8000 + 3 * 256 + c] = (uint8_t)(c + 1);
    }
    Frame(3);
    for (x = 0; x < 255; x++) {
        Expect("256-colour bitmap", x, Index((unsigned)x + 1));
    }
    Expect("256-colour bitmap transparent", 255, BLACK);

    // Direct colour: bit 15 is alpha
    Io16(0x0E, 0x84 | 2 << 8 | 1 << 14);
    Put16(s->bg, 0x8000, 0x801F);
    Put16(s->bg, 0x8002, 0x001F);
    Put16(s->bg, 0x8004, 0xFFFF);
    Frame(0);
    Expect("direct colour", 0, OUT6(62, 0, 0));
    Expect("direct colour alpha 0", 1, BLACK);
    Expect("direct colour", 2, OUT6(62, 62, 62));
    // Wrap around (BG3CNT bit 13) from X 254: dots 0 and 1 are x 254 and 255, dot 2 x 256, which wraps to 0; without
    // it, dot 2 is outside
    Put16(s->bg, 0x8000 + 254 * 2, 0x8000 | Rgb(1, 0, 0));
    Put16(s->bg, 0x8000 + 255 * 2, 0x8000 | Rgb(2, 0, 0));
    Io16(0x0E, 0x84 | 2 << 8 | 1 << 14 | 1 << 13);
    Io32(0x38, 254 << 8);
    Frame(0);
    Expect("direct colour wrap", 0, OUT6(2, 0, 0));
    Expect("direct colour wrap", 1, OUT6(4, 0, 0));
    Expect("direct colour wrap", 2, OUT6(62, 0, 0));
    Io16(0x0E, 0x84 | 2 << 8 | 1 << 14);
    Frame(0);
    Expect("direct colour no wrap", 1, OUT6(4, 0, 0));
    ExpectRange("direct colour no wrap", 2, 255, BLACK);

    // 512x256 256-colour bitmap wraps at 512 with BG3CNT bit 13
    Io16(0x0E, 0x80 | 2 << 8 | 2 << 14 | 1 << 13);
    s->bg[0x8000 + 510] = 3;
    s->bg[0x8000 + 511] = 4;
    s->bg[0x8000 + 0] = 5;
    Io32(0x38, 510 << 8);
    Frame(0);
    Expect("512x256 wrap", 0, Index(3));
    Expect("512x256 wrap", 1, Index(4));
    Expect("512x256 wrap", 2, Index(5));
    Io16(0x0E, 0x80 | 2 << 8 | 2 << 14);
    Frame(0);
    Expect("512x256 no wrap", 2, BLACK);
}

static void TestLarge(void) {
    // Mode 6 BG2: 1024x512, all of BG VRAM from 0; BGxCNT's bases don't apply
    Setup(0);
    SetIndexPalette();
    Io32(0x00, 0x10406);
    Io16(0x0C, 1 << 14 | 2 << 8 | 3 << 2);
    Io16(0x20, 0x100);
    Io16(0x26, 0x100);
    Io32(0x28, 1000 << 8);
    Io32(0x2C, 300 << 8);
    s->bg[300 * 1024 + 1000] = 7;
    s->bg[300 * 1024 + 1001] = 8;
    Frame(0);
    Expect("large 1024x512", 0, Index(7));
    Expect("large 1024x512", 1, Index(8));
    // 512x1024
    Io16(0x0C, 0);
    Io32(0x28, 500 << 8);
    Io32(0x2C, 1000 << 8);
    s->bg[1000 * 512 + 500] = 9;
    Frame(0);
    Expect("large 512x1024", 0, Index(9));
    // BG0 without the 3D bit shows nothing in mode 6
    Io32(0x00, 0x10106);
    Io16(0x08, 0);
    Put16(s->bg, 0, 1);
    FillTile4(s->bg, 32, 1);
    BgPal(1, Rgb(31, 31, 31));
    Frame(0);
    Expect("mode 6 BG0 2D shows nothing", 0, BLACK);
}

// --- The 3D layer ---

static void Setup3d(void) {
    Setup(0);
    Io32(0x00, 0x10108);
    s->src.bg0_3d_line = s->line3d;
}

static void Test3d(void) {
    int x;

    Setup3d();
    s->line3d[0] = 0xF8FC0000u; // alpha 31, red 0xFC -> 63
    s->line3d[1] = 0x00FC0000u; // alpha 0: transparent
    s->line3d[2] = 0x0800FC00u; // alpha 1 (8 >> 3): visible, green 63
    BgPal(0, Rgb(0, 0, 31));
    Line(0);
    Expect("3D opaque", 0, 0xFFFF0000u);
    Expect("3D transparent", 1, OUT6(0, 0, 62));
    Expect("3D low alpha", 2, OUT6(0, 63, 0));
    Expect("3D none", 3, OUT6(0, 0, 62));

    // BG0HOFS 10: dot x shows 3D x + 10; dots 246..255 are the transparent half
    for (x = 0; x < 256; x++) {
        s->line3d[x] = 0xF8000000u | (uint32_t)(x & 63) << 2; // blue (6 bits) x & 63
    }
    Io16(0x10, 10);
    Io16(0x12, 50); // VOFS doesn't apply to 3D
    Frame(0);
    Expect("3D scroll", 0, OUT6(0, 0, 10));
    Expect("3D scroll", 245, OUT6(0, 0, (255 & 63)));
    ExpectRange("3D scroll transparent half", 246, 255, OUT6(0, 0, 62));
    Io16(0x10, 500);
    Frame(0);
    ExpectRange("3D scroll from the left", 0, 11, OUT6(0, 0, 62));
    Expect("3D scroll from the left", 12, OUT6(0, 0, 0));
    Expect("3D scroll from the left", 13, OUT6(0, 0, 1));

    // Over a 2nd target the 3D alpha blends: alpha 15 (0x7B >> 3), blue 40 over the red backdrop (62, 0, 0):
    // EVA 16, EVB 16: red (62 * 16 + 16) >> 5 = 31, blue (40 * 16 + 16) >> 5 = 20
    Setup3d();
    BgPal(0, Rgb(31, 0, 0));
    s->line3d[0] = 0x7B0000A0u;
    s->line3d[1] = 0xF80000A0u; // alpha 31: unchanged
    s->line3d[2] = 0x040000A0u; // alpha 0, visible: EVA 1, EVB 31: red (62*31+16)>>5 = 60, blue (40+16)>>5 = 1
    s->line3d[3] = 0x7B0000A4u; // alpha 15, blue 41: (41 * 16 + 16) >> 5 = 21, where the rounding counts
    Io16(0x50, 0x2000);
    Line(0);
    Expect("3D blend", 0, OUT6(31, 0, 20));
    Expect("3D blend rounding", 3, OUT6(31, 0, 21));
    Expect("3D blend alpha 31", 1, OUT6(0, 0, 40));
    Expect("3D blend alpha 0", 2, OUT6(60, 0, 1));
    // The window's effect flag doesn't stop it: WIN0 on but empty, outside shows BG0 without effects
    Io32(0x00, 0x10108 | 1 << 13);
    Io16(0x4A, 0x01);
    Frame(0);
    Expect("3D blend ignores the window", 0, OUT6(31, 0, 20));
    // No 2nd target: the 3D colour
    Io32(0x00, 0x10108);
    Io16(0x50, 0x0000);
    Frame(0);
    Expect("3D no 2nd target", 0, OUT6(0, 0, 40));
    // Brightness up 8 with BG0 as 1st target: 0 + ((63 * 8 + 8) >> 4) = 32, 40 + ((23 * 8 + 8) >> 4) = 52
    Io16(0x50, 0x0081);
    Io16(0x54, 8);
    s->line3d[0] = 0xF80000A0u;
    Frame(0);
    Expect("3D brightness", 0, OUT6(32, 32, 52));
    // Mosaic doesn't apply to 3D
    Io16(0x50, 0);
    Io16(0x08, 0x40);
    Io16(0x4C, 0x0003);
    s->line3d[1] = 0xF8FC0000u;
    Frame(0);
    Expect("3D ignores mosaic", 1, OUT6(63, 0, 0));
}

// --- Mosaic ---

static void TestMosaic(void) {
    unsigned r;
    int line;

    // BG horizontal: size 4, blocks from the left edge
    SetupText4();
    Io16(0x08, 1 << 2 | 2 << 8 | 0x40);
    Io16(0x4C, 0x0003);
    Line(0);
    ExpectRange("BG mosaic H", 0, 3, OUT6(2, 0, 0));
    ExpectRange("BG mosaic H", 4, 7, OUT6(10, 0, 0));

    // BG vertical, size 3: tile 8 row r is all index r + 1; line L shows row L - L % 3
    SetupText4();
    for (r = 0; r < 8; r++) {
        memset(s->bg + 0x4000 + 8 * 32 + r * 4, (int)((r + 1) * 0x11), 4);
    }
    Put16(s->bg, 0x1000, 8 | 3 << 12);
    Io16(0x08, 1 << 2 | 2 << 8 | 0x40);
    Io16(0x4C, 0x0020);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 6; line++) {
        Line(line);
        Expect("BG mosaic V", 0, OUT6(2 * (unsigned)(line - line % 3 + 1), 0, 0));
    }
    Io16(0x08, 1 << 2 | 2 << 8);
    Frame(4);
    Expect("BG mosaic V off", 0, OUT6(10, 0, 0));

    // A new V size takes effect when the block ends: lines 0..2 row 0, then every line its own
    Io16(0x08, 1 << 2 | 2 << 8 | 0x40);
    Io16(0x4C, 0x0020);
    port_ds2d_begin_frame(0);
    Line(0);
    Io16(0x4C, 0x0000);
    Line(1);
    Expect("BG mosaic V size change", 0, OUT6(2, 0, 0));
    Line(2);
    Expect("BG mosaic V size change", 0, OUT6(2, 0, 0));
    Line(3);
    Expect("BG mosaic V size change", 0, OUT6(8, 0, 0));
    Line(4);
    Expect("BG mosaic V size change", 0, OUT6(10, 0, 0));

    // A line rendered again keeps its vertical mosaic block: size 3, lines 0, 1, 2, 2 again and 3 show rows 0, 0, 0, 0
    // and 3
    Io16(0x4C, 0x0020);
    port_ds2d_begin_frame(0);
    Line(0);
    Line(1);
    Line(2);
    Line(2);
    Expect("BG mosaic V line rendered again", 0, OUT6(2, 0, 0));
    Line(3);
    Expect("BG mosaic V after a line rendered again", 0, OUT6(8, 0, 0));

    // Affine vertical mosaic holds the reference point: size 2, lines 0 and 1 row 0, lines 2 and 3 row 2
    SetupAffine();
    Io16(0x0C, 1 << 2 | 0x40);
    Io16(0x4C, 0x0010);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 4; line++) {
        Line(line);
        Expect("affine mosaic V", 0, Index(line < 2 ? 1 : 33));
    }
    // Affine horizontal, size 2
    Io16(0x4C, 0x0001);
    Frame(0);
    Expect("affine mosaic H", 0, Index(1));
    Expect("affine mosaic H", 1, Index(1));
    Expect("affine mosaic H", 2, Index(3));
    Expect("affine mosaic H", 3, Index(3));
}

// --- Priorities ---

// BG0 all tile 1 (index 1, red), BG1 all tile 2 (index 2, green); OBJ tile 1 index 1 (blue), tile 2 index 2 (white),
// 2D mapping
static void SetupPriority(void) {
    unsigned i;

    Setup(0);
    Io32(0x00, 0x11300);
    FillTile4(s->bg, 0x4000 + 32, 1);
    FillTile4(s->bg, 0x4000 + 64, 2);
    for (i = 0; i < 32 * 32; i++) {
        Put16(s->bg, 0x1000 + i * 2, 1);
        Put16(s->bg, 0x1800 + i * 2, 2);
    }
    BgPal(1, Rgb(31, 0, 0));
    BgPal(2, Rgb(0, 31, 0));
    FillTile4(s->obj, 32, 1);
    FillTile4(s->obj, 64, 2);
    ObjPal(1, Rgb(0, 0, 31));
    ObjPal(2, Rgb(31, 31, 31));
}

static void SetBgPriorities(unsigned bg0, unsigned bg1) {
    Io16(0x08, (uint16_t)(bg0 | 1 << 2 | 2 << 8));
    Io16(0x0A, (uint16_t)(bg1 | 1 << 2 | 3 << 8));
}

static void TestPriority(void) {
    SetupPriority();
    SetBgPriorities(1, 0);
    Line(0);
    ExpectRange("BG1 priority 0 over BG0 priority 1", 0, 255, OUT6(0, 62, 0));
    SetBgPriorities(2, 2);
    Line(1);
    ExpectRange("equal priorities: BG0 in front", 0, 255, OUT6(62, 0, 0));
    SetBgPriorities(3, 1);
    Line(2);
    ExpectRange("BG1 priority 1 over BG0 priority 3", 0, 255, OUT6(0, 62, 0));

    // An OBJ is in front of a BG of its own priority, behind one of a better priority
    SetBgPriorities(2, 1);
    SetObj(0, 0, 0, 1 | 1 << 10);
    Frame(0);
    ExpectRange("OBJ priority 1 over BG1 priority 1", 0, 7, OUT6(0, 0, 62));
    Expect("OBJ priority 1 over BG1 priority 1", 8, OUT6(0, 62, 0));
    SetObj(0, 0, 0, 1 | 2 << 10);
    Frame(0);
    ExpectRange("OBJ priority 2 under BG1 priority 1", 0, 7, OUT6(0, 62, 0));

    // Between OBJs the BG-relative priority comes first, then the OAM number
    SetBgPriorities(3, 3);
    SetObj(0, 0, 0, 1 | 2 << 10);
    SetObj(1, 0, 4, 2 | 1 << 10);
    Frame(0);
    ExpectRange("OBJ priority before OAM number", 0, 3, OUT6(0, 0, 62));
    ExpectRange("OBJ priority before OAM number", 4, 11, OUT6(62, 62, 62));
    SetObj(0, 0, 0, 1 | 1 << 10);
    Frame(0);
    ExpectRange("equal OBJ priority: lower OAM number", 0, 7, OUT6(0, 0, 62));
    ExpectRange("equal OBJ priority: lower OAM number", 8, 11, OUT6(62, 62, 62));
    // No GBA-style hole: OAM 0 priority 3 under BG1 priority 1 doesn't hide OAM 1 priority 0
    SetBgPriorities(3, 1);
    SetObj(0, 0, 0, 1 | 3 << 10);
    SetObj(1, 0, 4, 2 | 0 << 10);
    Frame(0);
    ExpectRange("no OBJ priority hole", 0, 3, OUT6(0, 62, 0));
    ExpectRange("no OBJ priority hole", 4, 11, OUT6(62, 62, 62));
}

// --- OBJs ---

// OBJ palette p colour i is (red i, green p) for 16-colour OBJs
static uint32_t ObjColor(unsigned p, unsigned i) {
    return OUT6(2 * i, 2 * p, 0);
}

// A 16x16 16-colour OBJ, tile 4, palette 2, at (20, 10); 1D mapping with 32-byte boundary: its tiles k = 0..3 (TL,
// TR, BL, BR) at 128 + 32k are all index k + 1
static void SetupObj(void) {
    unsigned p;
    unsigned i;
    unsigned k;

    Setup(0);
    Io32(0x00, 0x11010);
    for (p = 0; p < 16; p++) {
        for (i = 1; i < 16; i++) {
            ObjPal(p * 16 + i, Rgb(i, p, 0));
        }
    }
    for (k = 0; k < 4; k++) {
        FillTile4(s->obj, 128 + 32 * k, k + 1);
    }
    SetObj(0, 10, 20 | 1 << 14, 4 | 2 << 12);
}

static void ExpectQuadrants(const char *name, int x, unsigned left, unsigned right) {
    ExpectRange(name, x, x + 7, ObjColor(2, left));
    ExpectRange(name, x + 8, x + 15, ObjColor(2, right));
}

static void TestObjNormal(void) {
    unsigned k;

    SetupObj();
    Frame(10);
    ExpectQuadrants("OBJ 1D", 20, 1, 2);
    Expect("OBJ 1D left of it", 19, BLACK);
    Expect("OBJ 1D right of it", 36, BLACK);
    Frame(18);
    ExpectQuadrants("OBJ 1D lower row", 20, 3, 4);
    Frame(26);
    ExpectRange("OBJ 1D below it", 0, 255, BLACK);

    // Boundary 128 bytes: tile 1 is the same 128
    Io32(0x00, 0x11010 | 2 << 20);
    SetObj(0, 10, 20 | 1 << 14, 1 | 2 << 12);
    Frame(10);
    ExpectQuadrants("OBJ 1D boundary 128", 20, 1, 2);

    // 2D mapping: rows of 32 tiles; tile 4's neighbours are 5, 36 and 37
    SetupObj();
    Io32(0x00, 0x11000);
    memset(s->obj, 0, 1024 * 2);
    FillTile4(s->obj, 4 * 32, 1);
    FillTile4(s->obj, 5 * 32, 2);
    FillTile4(s->obj, 36 * 32, 3);
    FillTile4(s->obj, 37 * 32, 4);
    Frame(10);
    ExpectQuadrants("OBJ 2D", 20, 1, 2);
    Frame(18);
    ExpectQuadrants("OBJ 2D lower row", 20, 3, 4);

    // 256 colours, 1D with 64-byte boundary: tile 2 is 128, tiles of 64 bytes; indices 16, 32, 48, 64
    SetupObj();
    Io32(0x00, 0x11010 | 1 << 20);
    for (k = 0; k < 4; k++) {
        FillTile8(s->obj, 128 + 64 * k, (k + 1) * 16);
    }
    for (k = 1; k <= 4; k++) {
        ObjPal(k * 16, Rgb(k, 0, 31));
        Put16(s->objExt, (3 * 256 + k * 16) * 2, Rgb(0, k, 31));
    }
    SetObj(0, 10 | 0x2000, 20 | 1 << 14, 2 | 3 << 12);
    Frame(18);
    ExpectRange("OBJ 256 colours", 20, 27, OUT6(6, 0, 62));
    ExpectRange("OBJ 256 colours", 28, 35, OUT6(8, 0, 62));
    Io32(0x00, 0x11010 | 1 << 20 | 1u << 31); // extended palette 3
    Frame(18);
    ExpectRange("OBJ 256 colours ext palette", 20, 27, OUT6(0, 6, 62));
    // 256 colours in 2D mapping: tile 4 at 128, its right neighbour 64 bytes on, the row below 1024 on
    Io32(0x00, 0x11000);
    memset(s->obj, 0, 4096);
    FillTile8(s->obj, 128, 16);
    FillTile8(s->obj, 192, 32);
    FillTile8(s->obj, 128 + 1024, 48);
    FillTile8(s->obj, 192 + 1024, 64);
    SetObj(0, 10 | 0x2000, 20 | 1 << 14, 4);
    Frame(18);
    ExpectRange("OBJ 256 colours 2D", 20, 27, OUT6(6, 0, 62));
    ExpectRange("OBJ 256 colours 2D", 28, 35, OUT6(8, 0, 62));

    // Flips
    SetupObj();
    SetObj(0, 10, 20 | 1 << 12 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectQuadrants("OBJ hflip", 20, 2, 1);
    SetObj(0, 10, 20 | 1 << 13 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectQuadrants("OBJ vflip", 20, 3, 4);

    // X 508 is -4: dots 0..3 are columns 4..7, dots 4..11 columns 8..15
    SetObj(0, 10, 508 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectRange("OBJ X wrap", 0, 3, ObjColor(2, 1));
    ExpectRange("OBJ X wrap", 4, 11, ObjColor(2, 2));
    ExpectRange("OBJ X wrap", 12, 255, BLACK);
    // X 250: dots 250..255 are columns 0..5
    SetObj(0, 10, 250 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectRange("OBJ off the right edge", 250, 255, ObjColor(2, 1));
    ExpectRange("OBJ off the right edge", 0, 249, BLACK);
    // Y 250: lines 250..255 and 0..9 are rows 0..15
    SetObj(0, 250, 20 | 1 << 14, 4 | 2 << 12);
    Frame(0);
    ExpectQuadrants("OBJ Y wrap row 6", 20, 1, 2);
    Frame(2);
    ExpectQuadrants("OBJ Y wrap row 8", 20, 3, 4);
    Frame(9);
    ExpectQuadrants("OBJ Y wrap row 15", 20, 3, 4);
    Frame(10);
    ExpectRange("OBJ Y wrap past it", 0, 255, BLACK);
    // Disabled
    SetObj(0, 10 | 0x200, 20 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectRange("OBJ disabled", 0, 255, BLACK);
    // OBJs off in DISPCNT
    SetObj(0, 10, 20 | 1 << 14, 4 | 2 << 12);
    Io32(0x00, 0x10010);
    Frame(10);
    ExpectRange("OBJs off", 0, 255, BLACK);

    // Engine B's 128 KiB OBJ view wraps: boundary 256, tile 512 is 0x20000, read at 0
    Setup(1);
    Io32(0x00, 0x11010 | 3 << 20);
    FillTile4(s->obj, 0, 5);
    ObjPal(5, Rgb(0, 31, 0));
    SetObj(0, 0, 0, 512);
    Line(0);
    ExpectRange("engine B OBJ view wraps", 0, 7, OUT6(0, 62, 0));
}

static void TestObjAffine(void) {
    unsigned k;
    int x;

    // Identity in group 1
    SetupObj();
    SetObjAffine(1, 0x100, 0, 0, 0x100);
    SetObj(0, 10 | 0x100, 40 | 1 << 9 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectQuadrants("affine OBJ identity", 40, 1, 2);
    // Double size: a 32x32 box with the texture in its middle, dots 48..63, lines 18..33
    SetObj(0, 10 | 0x300, 40 | 1 << 9 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    ExpectRange("affine OBJ double size top", 0, 255, BLACK);
    Frame(18);
    ExpectQuadrants("affine OBJ double size", 48, 1, 2);
    Expect("affine OBJ double size", 47, BLACK);
    Expect("affine OBJ double size", 64, BLACK);
    // Scale 2 (PA = PD = 0.5) in double size: box column c reads c / 2
    SetObjAffine(1, 0x80, 0, 0, 0x80);
    Frame(10);
    ExpectRange("affine OBJ zoom", 40, 55, ObjColor(2, 1));
    ExpectRange("affine OBJ zoom", 56, 71, ObjColor(2, 2));
    Frame(26);
    ExpectRange("affine OBJ zoom lower", 40, 55, ObjColor(2, 3));
    ExpectRange("affine OBJ zoom lower", 56, 71, ObjColor(2, 4));
    // PA = -1, not double size: column c reads (-256 (c - 8) + 2048) >> 8 = 16 - c, so column 0 is outside
    SetObjAffine(1, -0x100, 0, 0, 0x100);
    SetObj(0, 10 | 0x100, 40 | 1 << 9 | 1 << 14, 4 | 2 << 12);
    Frame(10);
    Expect("affine OBJ mirror", 40, BLACK);
    for (x = 41; x <= 48; x++) {
        Expect("affine OBJ mirror", x, ObjColor(2, 2));
    }
    ExpectRange("affine OBJ mirror", 49, 55, ObjColor(2, 1));

    // A rotation by 90 degrees, PA 0, PB -1, PC 1, PD 0: the dot dx, dy from the centre (48, 18) reads texel
    // (8 - dy, 8 + dx). Line 10 (dy -8) reads column 16, outside; line 11 (dy -7) column 15, rows 0..15 along the line,
    // so the TR quadrant (2) then BR (4); line 25 (dy 7) column 1, TL (1) then BL (3). PB alone decides the column and
    // PC alone the row
    SetObjAffine(1, 0, -0x100, 0x100, 0);
    Frame(10);
    ExpectRange("affine OBJ rotation, outside", 0, 255, BLACK);
    Line(11);
    ExpectRange("affine OBJ rotation PC", 40, 47, ObjColor(2, 2));
    ExpectRange("affine OBJ rotation PC", 48, 55, ObjColor(2, 4));
    Expect("affine OBJ rotation PC", 56, BLACK);
    Frame(25);
    ExpectRange("affine OBJ rotation PB", 40, 47, ObjColor(2, 1));
    ExpectRange("affine OBJ rotation PB", 48, 55, ObjColor(2, 3));

    // An affine 256-colour OBJ, 1D with 64-byte boundary: tile 2 is 128, tiles of 64 bytes, indices 16, 32, 48, 64 in
    // TL, TR, BL, BR. OBJ palette colour 16k is (k, 0, 31); extended palette 3's is (0, k, 31)
    SetupObj();
    Io32(0x00, 0x11010 | 1 << 20);
    for (k = 0; k < 4; k++) {
        FillTile8(s->obj, 128 + 64 * k, (k + 1) * 16);
    }
    for (k = 1; k <= 4; k++) {
        ObjPal(k * 16, Rgb(k, 0, 31));
        Put16(s->objExt, (3 * 256 + k * 16) * 2, Rgb(0, k, 31));
    }
    SetObjAffine(1, 0x100, 0, 0, 0x100);
    SetObj(0, 10 | 0x2000 | 0x100, 40 | 1 << 9 | 1 << 14, 2 | 3 << 12);
    Frame(10);
    ExpectRange("affine OBJ 256 colours", 40, 47, OUT6(2, 0, 62));
    ExpectRange("affine OBJ 256 colours", 48, 55, OUT6(4, 0, 62));
    Frame(18);
    ExpectRange("affine OBJ 256 colours", 40, 47, OUT6(6, 0, 62));
    ExpectRange("affine OBJ 256 colours", 48, 55, OUT6(8, 0, 62));
    Io32(0x00, 0x11010 | 1 << 20 | 1u << 31);
    Frame(18);
    ExpectRange("affine OBJ 256 colours ext palette", 40, 47, OUT6(0, 6, 62));
    ExpectRange("affine OBJ 256 colours ext palette", 48, 55, OUT6(0, 8, 62));
    // 2D mapping: tile 4 at 128, its right neighbour 64 bytes on, the row below 1024 on
    Io32(0x00, 0x11000);
    memset(s->obj, 0, 4096);
    FillTile8(s->obj, 128, 16);
    FillTile8(s->obj, 192, 32);
    FillTile8(s->obj, 128 + 1024, 48);
    FillTile8(s->obj, 192 + 1024, 64);
    SetObj(0, 10 | 0x2000 | 0x100, 40 | 1 << 9 | 1 << 14, 4);
    Frame(10);
    ExpectRange("affine OBJ 256 colours 2D", 40, 47, OUT6(2, 0, 62));
    ExpectRange("affine OBJ 256 colours 2D", 48, 55, OUT6(4, 0, 62));
    Frame(18);
    ExpectRange("affine OBJ 256 colours 2D", 40, 47, OUT6(6, 0, 62));
    ExpectRange("affine OBJ 256 colours 2D", 48, 55, OUT6(8, 0, 62));
}

static void TestObjBitmap(void) {
    unsigned x;

    // 8x8 bitmap OBJ, tile 0x31, alpha 15. 2D 128 dots wide: (0x31 & 0xF) * 0x10 + (0x31 & ~0xF) * 0x80 = 0x1810,
    // rows 256 bytes apart. Dot x of row 0 is red x + 1, dot 3 has alpha 0. Bitmap dots have bit 15 set, their
    // alpha, which doesn't reach the colour
    Setup(0);
    Io32(0x00, 0x11000);
    for (x = 0; x < 8; x++) {
        Put16(s->obj, 0x1810 + x * 2, (uint16_t)((x == 3 ? 0 : 0x8000) | Rgb(x + 1, 0, 0)));
        Put16(s->obj, 0x1810 + 256 + x * 2, 0x8000 | Rgb(0, x + 1, 0));
    }
    SetObj(0, 3 << 10, 0, 0x31 | 15 << 12);
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("bitmap OBJ 2D 128", (int)x, x == 3 ? BLACK : OUT6(2 * (x + 1), 0, 0));
    }
    Expect("bitmap OBJ 2D 128", 8, BLACK);
    Line(1);
    Expect("bitmap OBJ 2D 128 row 1", 2, OUT6(0, 6, 0));
    // Hflip: dot x shows dot 7 - x, so dot 4 is the transparent dot 3
    SetObj(0, 3 << 10, 1 << 12, 0x31 | 15 << 12);
    Frame(0);
    for (x = 0; x < 8; x++) {
        Expect("bitmap OBJ hflip", (int)x, x == 4 ? BLACK : OUT6(2 * (8 - x), 0, 0));
    }
    // Vflip: line 0 shows row 7, whose dot 0 is blue 5, and line 6 row 1
    Put16(s->obj, 0x1810 + 7 * 256, 0x8000 | Rgb(0, 0, 5));
    SetObj(0, 3 << 10, 1 << 13, 0x31 | 15 << 12);
    Frame(0);
    Expect("bitmap OBJ vflip", 0, OUT6(0, 0, 10));
    Frame(6);
    Expect("bitmap OBJ vflip row 1", 2, OUT6(0, 6, 0));
    SetObj(0, 3 << 10, 0, 0x31 | 15 << 12);

    // 2D 256 dots wide: (0x31 & 0x1F) * 0x10 + (0x31 & ~0x1F) * 0x80 = 0x1110, rows 512 bytes apart
    Io32(0x00, 0x11020);
    Put16(s->obj, 0x1110, 0x8000 | Rgb(0, 0, 9));
    Put16(s->obj, 0x1110 + 512 + 2, 0x8000 | Rgb(0, 0, 10));
    Frame(1);
    Expect("bitmap OBJ 2D 256 row 1", 1, OUT6(0, 0, 20));
    Frame(0);
    Expect("bitmap OBJ 2D 256", 0, OUT6(0, 0, 18));
    // 1D, 128-byte boundary: 0x31 * 128 = 0x1880, rows 16 bytes apart
    Io32(0x00, 0x11040);
    Put16(s->obj, 0x1880 + 16 + 4, 0x8000 | Rgb(11, 0, 0));
    Frame(1);
    Expect("bitmap OBJ 1D 128", 2, OUT6(22, 0, 0));
    // 1D, 256-byte boundary: 0x31 * 256 = 0x3100
    Io32(0x00, 0x11040 | 1 << 22);
    Put16(s->obj, 0x3100, 0x8000 | Rgb(12, 0, 0));
    Frame(0);
    Expect("bitmap OBJ 1D 256", 0, OUT6(24, 0, 0));
    // Alpha 0: not drawn
    SetObj(0, 3 << 10, 0, 0x31);
    Frame(0);
    Expect("bitmap OBJ alpha 0", 0, BLACK);
    // Affine with identity draws the same
    SetObjAffine(0, 0x100, 0, 0, 0x100);
    SetObj(0, 3 << 10 | 0x100, 0, 0x31 | 15 << 12);
    Frame(0);
    Expect("bitmap OBJ affine", 0, OUT6(24, 0, 0));

    // Engine B has no 256-byte bitmap boundary: 0x31 * 128 = 0x1880
    Setup(1);
    Io32(0x00, 0x11040 | 1 << 22);
    Put16(s->obj, 0x1880, 0x8000 | Rgb(13, 0, 0));
    SetObj(0, 3 << 10, 0, 0x31 | 15 << 12);
    Line(0);
    Expect("engine B bitmap OBJ 1D", 0, OUT6(26, 0, 0));

    // Alpha 7 is EVA 8, EVB 8 over a 2nd target: red 62 over the blue backdrop 62 gives (504 >> 4) = 31 each
    Setup(0);
    Io32(0x00, 0x11040);
    Put16(s->obj, 0x1880, 0x8000 | Rgb(31, 0, 0));
    BgPal(0, Rgb(0, 0, 31));
    SetObj(0, 3 << 10, 0, 0x31 | 7 << 12);
    Io16(0x50, 0x2000);
    Line(0);
    Expect("bitmap OBJ alpha blend", 0, OUT6(31, 0, 31));
    Io16(0x50, 0x0000);
    Frame(0);
    Expect("bitmap OBJ no 2nd target", 0, OUT6(62, 0, 0));
    SetObj(0, 3 << 10, 0, 0x31 | 15 << 12); // alpha 15 is EVA 16, EVB 0: (62 * 16 + 8) >> 4 = 62
    Io16(0x50, 0x2000);
    Frame(0);
    Expect("bitmap OBJ alpha 15", 0, OUT6(62, 0, 0));
}

static void TestObjWindow(void) {
    unsigned i;

    // BG0 all red; outside shows nothing, the OBJ window shows BG0. The window OBJ's row 0 is
    // indices 1, 1, 0, 1, 1, 1, 1, 1
    Setup(0);
    Io32(0x00, 0x19100);
    Io16(0x08, 1 << 2 | 2 << 8);
    FillTile4(s->bg, 0x4000 + 32, 1);
    for (i = 0; i < 32 * 32; i++) {
        Put16(s->bg, 0x1000 + i * 2, 1);
    }
    BgPal(1, Rgb(31, 0, 0));
    FillTile4(s->obj, 32, 1);
    s->obj[32 + 1] = 0x10; // dot 2 index 0, dot 3 index 1
    ObjPal(1, Rgb(0, 31, 0));
    SetObj(0, 2 << 10, 8, 1);
    Io16(0x4A, 0x0100);
    Line(0);
    ExpectRange("OBJ window", 0, 7, BLACK);
    ExpectRange("OBJ window", 8, 9, OUT6(62, 0, 0));
    Expect("OBJ window transparent dot", 10, BLACK);
    ExpectRange("OBJ window", 11, 15, OUT6(62, 0, 0));
    Expect("OBJ window", 16, BLACK);
    // The window OBJ isn't drawn itself
    Io16(0x4A, 0x1000);
    Frame(0);
    ExpectRange("OBJ window draws nothing", 0, 255, BLACK);
    // It needs OBJs on: without, every dot is outside
    Io32(0x00, 0x18100);
    Io16(0x4A, 0x0100);
    Frame(0);
    ExpectRange("OBJ window needs OBJs", 0, 255, BLACK);
}

static void TestSemiTransparent(void) {
    // A red 8x8 semi-transparent OBJ over the blue backdrop. EVA 12, EVB 4: red (62 * 12 + 8) >> 4 = 47, blue
    // (62 * 4 + 8) >> 4 = 16
    Setup(0);
    Io32(0x00, 0x11000);
    FillTile4(s->obj, 32, 1);
    ObjPal(1, Rgb(31, 0, 0));
    BgPal(0, Rgb(0, 0, 31));
    SetObj(0, 1 << 10, 0, 1);
    Io16(0x52, 12 | 4 << 8);
    Io16(0x50, 0x2000);
    Line(0);
    Expect("semi-transparent OBJ", 0, OUT6(47, 0, 16));
    Expect("semi-transparent OBJ", 8, OUT6(0, 0, 62));
    // Whatever BLDCNT's mode
    Io16(0x50, 0x2000 | 0xC0 | 0x10);
    Io16(0x54, 16);
    Line(1);
    Expect("semi-transparent OBJ over darken", 0, OUT6(47, 0, 16));
    // and the window's effect flag: WIN0 empty, outside shows the OBJ without effects
    Io32(0x00, 0x13000);
    Io16(0x4A, 0x10);
    Io16(0x50, 0x2000);
    Frame(0);
    Expect("semi-transparent OBJ ignores the window", 0, OUT6(47, 0, 16));
    // No 2nd target below: BLDCNT's effect if the OBJ is a 1st target
    Io32(0x00, 0x11000);
    Io16(0x50, 0x10 | 0xC0);
    Frame(0);
    Expect("semi-transparent OBJ darkened", 0, BLACK);
    Io16(0x50, 0xC0);
    Frame(0);
    Expect("semi-transparent OBJ not a 1st target", 0, OUT6(62, 0, 0));
    // A normal OBJ doesn't blend unless it is a 1st target
    SetObj(0, 0, 0, 1);
    Io16(0x50, 0x2000 | 0x40);
    Frame(0);
    Expect("normal OBJ, no 1st target", 0, OUT6(62, 0, 0));
}

static void TestObjMosaic(void) {
    unsigned r;
    int line;

    // An 8x8 mosaic OBJ at x 2 whose column c is index c + 1, but column 5 transparent; OBJ H size 4. Blocks start at
    // dots 0, 4, 8: dots 2-3 latch column 0, 4-7 column 2 (dot 7, the transparent column 5, too), 8-9 column 6
    Setup(0);
    Io32(0x00, 0x11000);
    s->obj[32] = 0x21;
    s->obj[33] = 0x43;
    s->obj[34] = 0x05;
    s->obj[35] = 0x87;
    for (r = 1; r < 16; r++) {
        ObjPal(r, Rgb(r, 0, 0));
    }
    SetObj(0, 0x1000, 2, 1);
    Io16(0x4C, 0x0300);
    Line(0);
    ExpectRange("OBJ mosaic H", 0, 1, BLACK);
    ExpectRange("OBJ mosaic H", 2, 3, OUT6(2, 0, 0));
    ExpectRange("OBJ mosaic H", 4, 7, OUT6(6, 0, 0));
    ExpectRange("OBJ mosaic H", 8, 9, OUT6(14, 0, 0));
    Expect("OBJ mosaic H", 10, BLACK);
    SetObj(0, 0, 2, 1); // without the mosaic flag
    Frame(0);
    Expect("OBJ no mosaic", 3, OUT6(4, 0, 0));
    Expect("OBJ no mosaic", 7, BLACK);
    // Inside a block, a mosaic dot of a better priority is latched: OBJ 0 (priority 2, red) at x 0 and OBJ 1
    // (priority 1, blue) at x 2, both uniform 8x8 mosaic OBJs. The OBJ line is red at 0-1 and blue at 2-9; dot 2 is
    // in the block started at 0 but blue's priority is better, so it shows blue, and so does dot 3
    FillTile4(s->obj, 64, 1);
    FillTile4(s->obj, 96, 2);
    ObjPal(16 + 1, Rgb(31, 0, 0));
    ObjPal(16 + 2, Rgb(0, 0, 31));
    SetObj(0, 0x1000, 0, 2 | 2 << 10 | 1 << 12);
    SetObj(1, 0x1000, 2, 3 | 1 << 10 | 1 << 12);
    Frame(0);
    ExpectRange("OBJ mosaic priority", 0, 1, OUT6(62, 0, 0));
    ExpectRange("OBJ mosaic priority", 2, 9, OUT6(0, 0, 62));
    Expect("OBJ mosaic priority", 10, BLACK);

    // An 8x8 mosaic OBJ at y 1 whose row r is index r + 1; OBJ V size 3. Blocks start at lines 0, 3, 6: lines 1-2
    // draw row 0 (the block's line 0 is above the OBJ), 3-5 row 2, 6-8 row 5
    Setup(0);
    Io32(0x00, 0x11000);
    for (r = 0; r < 8; r++) {
        memset(s->obj + 32 + r * 4, (int)((r + 1) * 0x11), 4);
        ObjPal(r + 1, Rgb(r + 1, 0, 0));
    }
    SetObj(0, 1 | 0x1000, 0, 1);
    Io16(0x4C, 0x2000);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 10; line++) {
        static const unsigned kRow[10] = { 99, 0, 0, 2, 2, 2, 5, 5, 5, 99 };

        Line(line);
        Expect("OBJ mosaic V", 0, kRow[line] == 99 ? BLACK : OUT6(2 * (kRow[line] + 1), 0, 0));
    }
}

// --- Windows ---

// BG0 all red (index 1), BG1 all green (index 2), BG1 behind; WIN0 shows BG0, outside nothing
static void SetupWindows(void) {
    unsigned i;

    Setup(0);
    Io16(0x08, 0 | 1 << 2 | 2 << 8);
    Io16(0x0A, 1 | 1 << 2 | 3 << 8);
    FillTile4(s->bg, 0x4000 + 32, 1);
    FillTile4(s->bg, 0x4000 + 64, 2);
    for (i = 0; i < 32 * 32; i++) {
        Put16(s->bg, 0x1000 + i * 2, 1);
        Put16(s->bg, 0x1800 + i * 2, 2);
    }
    BgPal(1, Rgb(31, 0, 0));
    BgPal(2, Rgb(0, 31, 0));
    Io32(0x00, 0x10100 | 1 << 13);
    Io16(0x48, 0x01);
    Io16(0x4A, 0x00);
}

static void TestWindows(void) {
    int line;
    uint32_t red = OUT6(62, 0, 0);

    // X 10..19, Y 5..7
    SetupWindows();
    Io16(0x40, 10 << 8 | 20);
    Io16(0x44, 5 << 8 | 8);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 10; line++) {
        Line(line);
        if (line >= 5 && line < 8) {
            Expect("window edges", 9, BLACK);
            ExpectRange("window edges", 10, 19, red);
            Expect("window edges", 20, BLACK);
        } else {
            ExpectRange("window outside its lines", 0, 255, BLACK);
        }
    }
    // X2 = 0 reaches the right edge
    Io16(0x40, 200 << 8 | 0);
    Io16(0x44, 0 << 8 | 192);
    Frame(1);
    ExpectRange("window X2 = 0", 0, 199, BLACK);
    ExpectRange("window X2 = 0", 200, 255, red);
    // X1 > X2 wraps into the next line: 200..255 and 0..49
    Io16(0x40, 200 << 8 | 50);
    Io16(0x44, 5 << 8 | 192);
    Frame(5);
    ExpectRange("window X1 > X2", 0, 49, red);
    ExpectRange("window X1 > X2", 50, 199, BLACK);
    ExpectRange("window X1 > X2", 200, 255, red);
    // X1 = X2 = 0: nothing
    Io16(0x40, 0);
    Frame(5);
    ExpectRange("window X1 = X2 = 0", 0, 255, BLACK);
    // A line that ends inside (X1 10, X2 0) carries into the next one: with X1 20 and X2 40 there, dots 0..39 are
    // inside, not 20..39
    port_ds2d_begin_frame(0);
    Io16(0x40, 10 << 8 | 0);
    Line(0);
    ExpectRange("window X2 = 0 line", 10, 255, red);
    Io16(0x40, 20 << 8 | 40);
    Line(1);
    ExpectRange("window carried into the next line", 0, 39, red);
    ExpectRange("window carried into the next line", 40, 255, BLACK);

    // Y1 > Y2: from line 100 through VBlank to line 9 of the next frame
    SetupWindows();
    Io16(0x40, 0 << 8 | 128);
    Io16(0x44, 100 << 8 | 10);
    Frame(191);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 192; line++) {
        Line(line);
        if (line == 5) {
            Expect("window Y1 > Y2 carries over", 0, red);
        } else if (line == 10) {
            Expect("window Y1 > Y2 ends at Y2", 0, BLACK);
        } else if (line == 100) {
            Expect("window Y1 > Y2 starts at Y1", 0, red);
            Expect("window Y1 > Y2 starts at Y1", 128, BLACK);
        }
    }

    // GBATEK's glitch: Y1 = 3 also matches VBlank line 259 (0x103), so the next frame starts inside
    SetupWindows();
    Io16(0x40, 0 << 8 | 128);
    Io16(0x44, 3 << 8 | 50);
    port_ds2d_begin_frame(0);
    for (line = 0; line < 192; line++) {
        Line(line);
        if (line == 0) {
            Expect("window first frame line 0", 0, BLACK);
        } else if (line == 3) {
            Expect("window first frame line 3", 0, red);
        }
    }
    Frame(0);
    Expect("window Y1 = 3 matches line 259", 0, red);

    // WIN0 before WIN1: WIN1 0..99 shows BG1, WIN0 50..59 BG0
    SetupWindows();
    Io32(0x00, 0x10300 | 3 << 13);
    Io16(0x40, 50 << 8 | 60);
    Io16(0x44, 0 << 8 | 192);
    Io16(0x42, 0 << 8 | 100);
    Io16(0x46, 0 << 8 | 192);
    Io16(0x48, 0x01 | 0x02 << 8);
    Frame(0);
    Expect("WIN0 before WIN1", 55, red);
    Expect("WIN1", 70, OUT6(0, 62, 0));
    Expect("WIN1", 49, OUT6(0, 62, 0));
    Expect("outside", 150, BLACK);

    // The effect flag: brighten 16 where the window allows effects
    SetupWindows();
    Io16(0x40, 10 << 8 | 20);
    Io16(0x44, 0 << 8 | 192);
    Io16(0x48, 0x01);
    Io16(0x4A, 0x21);
    Io16(0x50, 0x81);
    Io16(0x54, 16);
    Frame(0);
    Expect("window without effects", 15, red);
    Expect("outside with effects", 5, WHITE);
}

// --- Colour effects ---

// BG0 (priority 0) all colour 1, BG1 (priority 1) all colour 2
static void SetupBlend(uint16_t color1, uint16_t color2) {
    SetupWindows();
    Io32(0x00, 0x10300);
    BgPal(1, color1);
    BgPal(2, color2);
}

static void TestBlending(void) {
    // Red (62, 0, 0) over green (0, 62, 0)
    SetupBlend(Rgb(31, 0, 0), Rgb(0, 31, 0));
    Io16(0x50, 0x01 | 0x40 | 0x200);
    Io16(0x52, 16 | 16 << 8); // (62 * 16 + 8) >> 4 = 62
    Line(0);
    ExpectRange("alpha 16/16", 0, 255, OUT6(62, 62, 0));
    Io16(0x52, 31 | 31 << 8); // above 16 counts as 16
    Line(1);
    Expect("alpha 31/31", 0, OUT6(62, 62, 0));
    Io16(0x52, 1 | 0 << 8); // (62 + 8) >> 4 = 4
    Line(2);
    Expect("alpha 1/0", 0, OUT6(4, 0, 0));
    Io16(0x52, 6 | 10 << 8); // (62 * 6 + 8) >> 4 = 23, (62 * 10 + 8) >> 4 = 39
    Line(3);
    Expect("alpha 6/10", 0, OUT6(23, 39, 0));
    Io16(0x50, 0x01 | 0x40 | 0x400); // BG1 isn't a 2nd target
    Line(4);
    Expect("alpha without 2nd target", 0, OUT6(62, 0, 0));
    Io16(0x50, 0x02 | 0x40 | 0x200); // BG0 isn't a 1st target
    Line(5);
    Expect("alpha without 1st target", 0, OUT6(62, 0, 0));

    // Saturation: 40 and 40 at 16/16 are (640 + 640 + 8) >> 4 = 80, shown as 63
    SetupBlend(Rgb(20, 0, 0), Rgb(20, 0, 0));
    Io16(0x50, 0x01 | 0x40 | 0x200);
    Io16(0x52, 16 | 16 << 8);
    Line(0);
    Expect("alpha saturates", 0, OUT6(63, 0, 0));

    // Brightness up 3: 62 + ((1 * 3 + 8) >> 4) = 62, 0 + ((63 * 3 + 8) >> 4) = 12
    SetupBlend(Rgb(31, 0, 0), Rgb(0, 31, 0));
    Io16(0x50, 0x01 | 0x80);
    Io16(0x54, 3);
    Line(0);
    Expect("brighten 3", 0, OUT6(62, 12, 12));
    // Brightness down 3: 62 - ((62 * 3 + 7) >> 4) = 62 - 12 = 50
    Io16(0x50, 0x01 | 0xC0);
    Line(1);
    Expect("darken 3", 0, OUT6(50, 0, 0));
    // EVY above 16 counts as 16
    Io16(0x54, 20);
    Line(2);
    Expect("darken 20 = 16", 0, BLACK);
    Io16(0x50, 0x01 | 0x80);
    Line(3);
    Expect("brighten 20 = 16", 0, WHITE);
    // Rounding at small values: darken 1 on 62: 62 - ((62 + 7) >> 4) = 58; brighten 1 on 0: (63 + 8) >> 4 = 4
    Io16(0x54, 1);
    Line(4);
    Expect("brighten 1", 0, OUT6(62, 4, 4));
    Io16(0x50, 0x01 | 0xC0);
    Line(5);
    Expect("darken 1", 0, OUT6(58, 0, 0));
    // Where the biases count: darken 4 on 62 is 62 - ((248 + 7) >> 4) = 47 (a bias of 8 would give 46)
    Io16(0x54, 4);
    Line(6);
    Expect("darken bias", 0, OUT6(47, 0, 0));
    // Brighten 8 on (2, 0, 0): 2 + ((61 * 8 + 8) >> 4) = 33, 0 + ((63 * 8 + 8) >> 4) = 32 (a bias of 7 gives 32, 31)
    BgPal(1, Rgb(1, 0, 0));
    Io16(0x50, 0x01 | 0x80);
    Io16(0x54, 8);
    Line(7);
    Expect("brighten bias", 0, OUT6(33, 32, 32));

    // The backdrop as 1st target and as 2nd target
    SetupBlend(Rgb(31, 0, 0), Rgb(0, 31, 0));
    Io32(0x00, 0x10000);
    Io16(0x50, 0x20 | 0x80);
    Io16(0x54, 16);
    Line(0);
    ExpectRange("backdrop brightened", 0, 255, WHITE);
    Io32(0x00, 0x10100);
    BgPal(0, Rgb(0, 0, 31));
    Io16(0x50, 0x01 | 0x40 | 0x2000);
    Io16(0x52, 8 | 8 << 8); // (62 * 8 + 8) >> 4 = 31
    Line(1);
    Expect("backdrop as 2nd target", 0, OUT6(31, 0, 31));

    // An OBJ as 1st target over BG1 as 2nd: blue over green, 8/8
    SetupBlend(Rgb(31, 0, 0), Rgb(0, 31, 0));
    Io32(0x00, 0x11200);
    FillTile4(s->obj, 32, 1);
    ObjPal(1, Rgb(0, 0, 31));
    SetObj(0, 0, 0, 1);
    Io16(0x50, 0x10 | 0x40 | 0x200);
    Io16(0x52, 8 | 8 << 8);
    Line(0);
    Expect("OBJ alpha over BG1", 0, OUT6(0, 31, 31));
    Expect("BG1 alone", 8, OUT6(0, 62, 0));
}

// --- OBJ budget, capture hook ---

static void TestObjBudget(void) {
    unsigned i;

    // Two 64x64 OBJs on line 0, each 64 cycles in the GBA model
    Setup(0);
    Io32(0x00, 0x11010);
    for (i = 0; i < 64; i++) {
        FillTile4(s->obj, i * 32, 1);
    }
    ObjPal(1, Rgb(0, 0, 31));
    SetObj(0, 0, 0 | 3 << 14, 0);
    SetObj(1, 0, 100 | 3 << 14, 0);
    Line(0);
    Expect("no budget", 100, OUT6(0, 0, 62));
    port_ds2d_set_obj_cycle_budget(100, 100);
    Frame(0);
    Expect("budget 100: OBJ 0", 0, OUT6(0, 0, 62));
    Expect("budget 100: OBJ 1 dropped", 100, BLACK);
    port_ds2d_set_obj_cycle_budget(128, 128);
    Frame(0);
    Expect("budget 128: both", 100, OUT6(0, 0, 62));
    // H-blank free (DISPCNT bit 23) uses the second budget
    port_ds2d_set_obj_cycle_budget(1000, 100);
    Io32(0x00, 0x11010 | 1 << 23);
    Frame(0);
    Expect("H-blank free budget", 100, BLACK);
    // Off-screen OBJs cost too: OBJ 0 at X 300 is invisible
    Io32(0x00, 0x11010);
    port_ds2d_set_obj_cycle_budget(100, 100);
    SetObj(0, 0, 300 | 3 << 14, 0);
    Frame(0);
    Expect("off-screen OBJ costs", 100, BLACK);
    // An affine 64-wide OBJ costs 10 + 2 * 64 = 138
    SetObjAffine(0, 0x100, 0, 0, 0x100);
    SetObj(0, 0x100, 0 | 3 << 14, 0);
    SetObj(1, 0x200, 0, 0);
    port_ds2d_set_obj_cycle_budget(137, 137);
    Frame(0);
    Expect("affine cost 138 > 137", 0, BLACK);
    port_ds2d_set_obj_cycle_budget(138, 138);
    Frame(0);
    Expect("affine cost 138", 0, OUT6(0, 0, 62));
    port_ds2d_set_obj_cycle_budget(0, 0);
}

static void TestCapture(void) {
    const uint32_t *capture;

    Setup(0);
    Io32(0x00, 0x10000);
    BgPal(0, Rgb(31, 0, 0));
    Io16(0x6C, 0x4010);
    Line(0);
    capture = port_ds2d_capture_source_a();
    ExpectTrue("capture source A in mode 1", capture != NULL);
    ExpectTrue("capture source A is before master brightness", capture != NULL && capture[0] == 62u << 16);
    Expect("output after master brightness", 0, WHITE);
    Io32(0x00, 0x20000);
    Line(1);
    ExpectTrue("no capture source A in mode 2", port_ds2d_capture_source_a() == NULL);
    Io32(0x64, 1u << 31);
    Line(2);
    capture = port_ds2d_capture_source_a();
    ExpectTrue("capture source A while capturing", capture != NULL && capture[5] == 62u << 16);
}

int main(void) {
    s = calloc(1, sizeof(*s));
    if (s == NULL) {
        printf("out of memory\n");
        return 1;
    }
    TestDisplayOff();
    TestBackdropAndConversion();
    TestForcedBlank();
    TestMasterBrightness();
    TestVramDisplay();
    TestMainMemoryDisplay();
    TestText4();
    TestText8();
    TestBases();
    TestAffine();
    TestExtended();
    TestLarge();
    Test3d();
    TestMosaic();
    TestPriority();
    TestObjNormal();
    TestObjAffine();
    TestObjBitmap();
    TestObjWindow();
    TestSemiTransparent();
    TestObjMosaic();
    TestWindows();
    TestBlending();
    TestObjBudget();
    TestCapture();
    free(s);
    printf("%d checks, %d failures\n", sChecks, sFailures);
    return sFailures != 0;
}
