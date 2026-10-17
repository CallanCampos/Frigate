// Tests of display capture (port/gfx/capture.h): port_capture_line's sources, blend, offsets, sizes and missing banks,
// and port_capture_draw_line's frame latch, with the platform layer's VRAM and register services stubbed. Expected
// halfwords are worked out by hand from GBATEK's DISPCAPCNT fields and the rules capture.h credits to melonDS.
#include "gfx/capture.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include <stdio.h>
#include <string.h>

#define PAGE_BYTES 0x4000u
#define ENABLE 0x80000000u
#define SIZE_256x192 (3u << 20)
#define SIZE_256x64 (1u << 20)
#define SOURCE_A_3D (1u << 24)
#define SOURCE_B_FIFO (1u << 25)
#define SOURCE_B (1u << 29)
#define SOURCE_BLEND (2u << 29)
#define WRITE_OFFSET(n) ((uint32_t)(n) << 18)
#define READ_OFFSET(n) ((uint32_t)(n) << 26)
#define DISPCNT_VRAM_DISPLAY (2u << 16)

static int sChecks;
static int sFailures;

static uint8_t sDstMemory[PORT_CAPTURE_BANK_PAGES][PAGE_BYTES];
static uint8_t sSrcMemory[PORT_CAPTURE_BANK_PAGES][PAGE_BYTES];
static uint8_t *sDst[PORT_CAPTURE_BANK_PAGES];
static const uint8_t *sSrc[PORT_CAPTURE_BANK_PAGES];
static const uint8_t *const sNoBank[PORT_CAPTURE_BANK_PAGES] = { NULL };
static uint32_t sLineA[256];
static uint32_t sLine3d[256];

// The platform layer's services that port_capture_draw_line uses: the LCDC view's pages (bank D's only), the write
// note, the register memory and source A
const uint8_t port_vram_bank_first_page[PORT_VRAM_BANK_COUNT] = { 0 };
static uint32_t sDispcapcnt;
static int sNotes;

uint8_t *port_vram_slot_ptr(PortVramView view, int slot) {
    (void)view;
    return slot >= 0 && slot < PORT_CAPTURE_BANK_PAGES ? sDstMemory[slot] : NULL;
}

void port_vram_note_write(uint32_t address, uint32_t size) {
    (void)address;
    (void)size;
    sNotes++;
}

void *port_hw_pointer(uint32_t address) {
    (void)address;
    return &sDispcapcnt;
}

const uint32_t *port_ds2d_capture_source_a(void) {
    return sLineA;
}

static void Expect(const char *what, unsigned got, unsigned want) {
    sChecks++;
    if (got != want) {
        sFailures++;
        printf("FAIL %s: got 0x%x, want 0x%x\n", what, got, want);
    }
}

static uint16_t Half(uint8_t memory[][PAGE_BYTES], uint32_t index) {
    uint16_t value;

    memcpy(&value, &memory[index / 0x2000][(index % 0x2000) * 2], sizeof(value));
    return value;
}

static void SetHalf(uint8_t memory[][PAGE_BYTES], uint32_t index, uint16_t value) {
    memcpy(&memory[index / 0x2000][(index % 0x2000) * 2], &value, sizeof(value));
}

static void Reset(void) {
    memset(sDstMemory, 0xEE, sizeof(sDstMemory));
    memset(sSrcMemory, 0, sizeof(sSrcMemory));
    for (int i = 0; i < PORT_CAPTURE_BANK_PAGES; i++) {
        sDst[i] = sDstMemory[i];
        sSrc[i] = sSrcMemory[i];
    }
}

static unsigned Capture(uint32_t dispcapcnt, uint32_t dispcnt, int line, const uint32_t *lineA, const uint32_t *line3d,
                        const uint8_t *const *src) {
    return port_capture_line(dispcapcnt, dispcnt, line, lineA, line3d, sDst, src);
}

// Source A, the composed picture: 6-bit channels lose their low bit and the pixel is opaque
static void TestSourceA(void) {
    Reset();
    sLineA[0] = 63u << 16 | 32u << 8 | 1u;
    Expect("source A writes a whole line", Capture(ENABLE | SIZE_256x192, 0, 5, sLineA, NULL, sSrc), 256);
    Expect("source A: 31, 16, 0, opaque", Half(sDstMemory, 5 * 256), 0x821F);
    Reset();
    Capture(SIZE_256x192, 0, 0, NULL, NULL, sSrc);
    Expect("no composed line reads as transparent black", Half(sDstMemory, 0), 0x0000);
}

// Source A, the 3D layer: the top 5 bits of each 8-bit channel, opaque when the alpha isn't 0 (a transparent pixel
// keeps its colour)
static void TestSource3d(void) {
    Reset();
    sLine3d[0] = 0xF8081018u;
    sLine3d[1] = 0x00FF0000u;
    Capture(ENABLE | SIZE_256x192 | SOURCE_A_3D, 0, 0, sLineA, sLine3d, sSrc);
    Expect("3D: 1, 2, 3, opaque", Half(sDstMemory, 0), 0x8C41);
    Expect("3D: transparent red", Half(sDstMemory, 1), 0x001F);
    Capture(ENABLE | SIZE_256x192 | SOURCE_A_3D, 0, 1, sLineA, NULL, sSrc);
    Expect("3D: no picture", Half(sDstMemory, 256), 0x0000);
}

// Source B, a VRAM bank: read at DISPCAPCNT's read offset, which VRAM display mode ignores; the main memory display
// FIFO and an unmapped bank read 0
static void TestSourceB(void) {
    Reset();
    SetHalf(sSrcMemory, 2u * 0x4000 + 7 * 256, 0x1234);
    SetHalf(sSrcMemory, 7 * 256, 0x4321);
    Capture(SOURCE_B | READ_OFFSET(2) | SIZE_256x192, 1u << 16, 7, sLineA, NULL, sSrc);
    Expect("source B at the read offset", Half(sDstMemory, 7 * 256), 0x1234);
    Capture(SOURCE_B | READ_OFFSET(2) | SIZE_256x192, DISPCNT_VRAM_DISPLAY, 7, sLineA, NULL, sSrc);
    Expect("VRAM display mode ignores the read offset", Half(sDstMemory, 7 * 256), 0x4321);
    Capture(SOURCE_B | SOURCE_B_FIFO | SIZE_256x192, 1u << 16, 7, sLineA, NULL, sSrc);
    Expect("the display FIFO reads 0", Half(sDstMemory, 7 * 256), 0x0000);
    Reset();
    Capture(SOURCE_B | SIZE_256x192, 1u << 16, 7, sLineA, NULL, sNoBank);
    Expect("an unmapped bank reads 0", Half(sDstMemory, 7 * 256), 0x0000);
}

// The blend: (A * EVA + B * EVB + 8) / 16 per channel, each factor at most 16, a source counting only where it is
// opaque, the result at most 31 and opaque where a counting source is
static void TestBlend(void) {
    Reset();
    sLineA[0] = 62u << 16 | 0u << 8 | 20u; // 31, 0, 10
    SetHalf(sSrcMemory, 0, 0x8000 | 1 | 31 << 5 | 20 << 10);
    Capture(SOURCE_BLEND | 8 | 8 << 8 | SIZE_256x192, 0, 0, sLineA, NULL, sSrc);
    // r (248 + 8 + 8) / 16 = 16, g (0 + 248 + 8) / 16 = 16, b (80 + 160 + 8) / 16 = 15
    Expect("blend 8/8", Half(sDstMemory, 0), 0x8000 | 16 | 16 << 5 | 15 << 10);

    sLineA[0] = 62u << 16 | 62u << 8 | 62u;
    Capture(SOURCE_BLEND | 31 | 0 << 8 | SIZE_256x192, 0, 0, sLineA, NULL, sSrc);
    Expect("EVA above 16 counts as 16", Half(sDstMemory, 0), 0xFFFF);
    Capture(SOURCE_BLEND | 16 | 16 << 8 | SIZE_256x192, 0, 0, sLineA, NULL, sSrc);
    // 31 * 16 + 1 * 16 + 8 = 520 / 16 = 32, cut to 31; green 31 + 31; blue 31 + 20
    Expect("the sum is cut to 31", Half(sDstMemory, 0), 0xFFFF);

    SetHalf(sSrcMemory, 0, 31 | 31 << 5 | 31 << 10);
    Capture(SOURCE_BLEND | 0 | 16 << 8 | SIZE_256x192, 0, 0, sLineA, NULL, sSrc);
    Expect("a transparent B doesn't count", Half(sDstMemory, 0), 0x0000);
    Capture(SOURCE_BLEND | 4 | 16 << 8 | SIZE_256x192, 0, 0, NULL, NULL, sSrc);
    Expect("no A and a transparent B", Half(sDstMemory, 0), 0x0000);
}

// The write offset in steps of 32 KiB, and lines that wrap from the bank's end to its start
static void TestOffsetsAndSizes(void) {
    Reset();
    sLineA[0] = 62u << 16;
    Capture(WRITE_OFFSET(1) | SIZE_256x192, 0, 3, sLineA, NULL, sSrc);
    Expect("write offset 1", Half(sDstMemory, 0x4000 + 3 * 256), 0x801F);
    Capture(WRITE_OFFSET(3) | SIZE_256x192, 0, 191, sLineA, NULL, sSrc);
    // 0xC000 + 191 * 256 = 0x17F00, which wraps to 0x7F00
    Expect("a line past the bank's end wraps", Half(sDstMemory, 0x7F00), 0x801F);
    Capture(0, 0, 10, sLineA, NULL, sSrc);
    Expect("128x128 lines are 128 wide", Half(sDstMemory, 10 * 128), 0x801F);
    Expect("128x128 writes 128 halfwords", Capture(0, 0, 10, sLineA, NULL, sSrc), 128);
    Expect("a line below the size writes nothing", Capture(SIZE_256x64, 0, 64, sLineA, NULL, sSrc), 0);
    Expect("a line below the size", Half(sDstMemory, 64 * 256), 0xEEEE);
    Expect("a negative line writes nothing", Capture(SIZE_256x192, 0, -1, sLineA, NULL, sSrc), 0);
    sDst[3] = NULL;
    Expect("an unmapped destination writes nothing", Capture(SIZE_256x192, 0, 100, sLineA, NULL, sSrc), 0);
    Expect("an unmapped destination", Half(sDstMemory, 100 * 256), 0xEEEE);
}

// The frame: the enable bit is latched at line 0, each line is written as it is drawn, and the end of line 191 clears
// the bit in the register
static void TestFrame(void) {
    uint8_t io[0x70] = { 0 };
    PortDs2dSource engineA;
    uint32_t dispcapcnt = ENABLE | SIZE_256x192 | (3u << 16);

    memset(&engineA, 0, sizeof(engineA));
    engineA.io = io;
    Reset();
    sLineA[0] = 62u << 16;
    port_capture_reset();
    memcpy(io + 0x64, &dispcapcnt, sizeof(dispcapcnt));
    sNotes = 0;
    port_capture_draw_line(&engineA, 1);
    Expect("no capture before a frame's line 0", sNotes, 0);
    for (int line = 0; line < 192; line++) {
        port_capture_draw_line(&engineA, line);
    }
    Expect("each line notes its write", sNotes, 192);
    Expect("the last line", Half(sDstMemory, 191 * 256), 0x801F);
    Expect("the enable bit is cleared after line 191", sDispcapcnt, dispcapcnt & ~ENABLE);
    memcpy(io + 0x64, &sDispcapcnt, sizeof(sDispcapcnt));
    sNotes = 0;
    port_capture_draw_line(&engineA, 0);
    Expect("a cleared bit captures nothing", sNotes, 0);
}

int main(void) {
    TestSourceA();
    TestSource3d();
    TestSourceB();
    TestBlend();
    TestOffsetsAndSizes();
    TestFrame();
    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
