// Display capture (port/gfx/capture.h)
#include "gfx/capture.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include <string.h>

#define REG_DISPCAPCNT 0x04000064u
#define IO_DISPCNT 0x00
#define IO_DISPCAPCNT 0x64
// DISPCAPCNT's fields (GBATEK): EVA (0-4) and EVB (8-12), the destination bank (16-17) and write offset (18-19) in
// steps of 32 KiB, the size (20-21), source A (24: 3D only), source B (25: the main memory display FIFO), the read
// offset (26-27), the source (29-30: A, B, or the blend of both) and the enable bit (31)
#define DISPCAPCNT_SOURCE_A_3D (1u << 24)
#define DISPCAPCNT_SOURCE_B_FIFO (1u << 25)
#define DISPCAPCNT_ENABLE 0x80000000u
#define LCDC_BASE 0x06800000u
#define LCDC_BANK_SIZE 0x20000u
#define PAGE_HALFWORDS 0x2000u
#define VISIBLE_LINES 192

// Whether DISPCAPCNT's enable bit was set at the start of this frame's line 0: the capture runs for the frame then
static int sLatched;

static unsigned Min16(unsigned value) {
    return value > 16 ? 16 : value;
}

// The width and height of DISPCAPCNT's capture size: 128x128, 256x64, 256x128 or 256x192
static void CaptureSize(uint32_t dispcapcnt, unsigned *width, unsigned *height) {
    static const unsigned widths[4] = { 128, 256, 256, 256 };
    static const unsigned heights[4] = { 128, 64, 128, 192 };
    unsigned size = (dispcapcnt >> 20) & 3;

    *width = widths[size];
    *height = heights[size];
}

// The first halfword a line of the capture writes: the write offset, in steps of 32 KiB, then the lines before it
static uint32_t WriteStart(uint32_t dispcapcnt, unsigned width, int line) {
    return ((((dispcapcnt >> 18) & 3) << 14) + (uint32_t)line * width) & (PORT_CAPTURE_BANK_HALFWORDS - 1);
}

static uint16_t ReadHalf(const uint8_t *const pages[PORT_CAPTURE_BANK_PAGES], uint32_t index) {
    uint16_t value;

    memcpy(&value, pages[index / PAGE_HALFWORDS] + (index % PAGE_HALFWORDS) * 2, sizeof(value));
    return value;
}

static void WriteHalf(uint8_t *const pages[PORT_CAPTURE_BANK_PAGES], uint32_t index, uint16_t value) {
    memcpy(pages[index / PAGE_HALFWORDS] + (index % PAGE_HALFWORDS) * 2, &value, sizeof(value));
}

static int BankMapped(const uint8_t *const pages[PORT_CAPTURE_BANK_PAGES]) {
    for (int i = 0; i < PORT_CAPTURE_BANK_PAGES; i++) {
        if (pages[i] == NULL) {
            return 0;
        }
    }
    return 1;
}

unsigned port_capture_line(uint32_t dispcapcnt, uint32_t dispcnt, int line, const uint32_t *source_a,
                           const uint32_t *line_3d, uint8_t *const dst[PORT_CAPTURE_BANK_PAGES],
                           const uint8_t *const src_b[PORT_CAPTURE_BANK_PAGES]) {
    unsigned width;
    unsigned height;
    unsigned eva = Min16(dispcapcnt & 0x1F);
    unsigned evb = Min16((dispcapcnt >> 8) & 0x1F);
    unsigned source = (dispcapcnt >> 29) & 3;
    // Source B from VRAM reads the bank DISPCNT's VRAM display field names, at DISPCAPCNT's read offset unless engine
    // A is in VRAM display mode (melonDS); the main memory display FIFO isn't implemented (ds2d.h), so it reads 0
    int fromFifo = (dispcapcnt & DISPCAPCNT_SOURCE_B_FIFO) != 0;
    int haveB = fromFifo || BankMapped(src_b);
    uint32_t dstIndex;
    uint32_t srcBIndex = (uint32_t)line * 256;

    CaptureSize(dispcapcnt, &width, &height);
    if (line < 0 || (unsigned)line >= height || !BankMapped((const uint8_t *const *)dst)) {
        return 0;
    }
    dstIndex = WriteStart(dispcapcnt, width, line);
    if (((dispcnt >> 16) & 3) != 2) {
        srcBIndex += ((dispcapcnt >> 26) & 3) << 14;
    }
    srcBIndex &= PORT_CAPTURE_BANK_HALFWORDS - 1;

    for (unsigned x = 0; x < width; x++) {
        unsigned rA = 0;
        unsigned gA = 0;
        unsigned bA = 0;
        unsigned aA = 0;
        uint16_t valueB = 0;
        uint16_t out;

        // Source A in 5-bit channels. The composed picture is opaque everywhere, the backdrop included; a 3D pixel is
        // opaque when its alpha isn't 0 (melonDS)
        if (dispcapcnt & DISPCAPCNT_SOURCE_A_3D) {
            uint32_t dot = line_3d != NULL ? line_3d[x] : 0;

            rA = (dot >> 19) & 0x1F;
            gA = (dot >> 11) & 0x1F;
            bA = (dot >> 3) & 0x1F;
            aA = (dot >> 27) != 0;
        } else if (source_a != NULL) {
            uint32_t dot = source_a[x];

            rA = (dot >> 17) & 0x1F;
            gA = (dot >> 9) & 0x1F;
            bA = (dot >> 1) & 0x1F;
            aA = 1;
        }
        if (haveB && !fromFifo) {
            valueB = ReadHalf(src_b, srcBIndex);
        }

        if (source == 0) {
            out = (uint16_t)(rA | gA << 5 | bA << 10 | aA << 15);
        } else if (source == 1) {
            out = valueB;
        } else {
            // The blend, (A * alphaA * EVA + B * alphaB * EVB) / 16 with each channel at most 31; melonDS rounds it
            // to nearest. The result is opaque where a source that counts (its factor isn't 0) is
            unsigned aB = valueB >> 15;
            unsigned r = (rA * aA * eva + (valueB & 0x1F) * aB * evb + 8) >> 4;
            unsigned g = (gA * aA * eva + ((valueB >> 5) & 0x1F) * aB * evb + 8) >> 4;
            unsigned b = (bA * aA * eva + ((valueB >> 10) & 0x1F) * aB * evb + 8) >> 4;
            unsigned a = (eva != 0 && aA) || (evb != 0 && aB);

            out = (uint16_t)((r > 31 ? 31 : r) | (g > 31 ? 31 : g) << 5 | (b > 31 ? 31 : b) << 10 | a << 15);
        }
        WriteHalf(dst, dstIndex, out);
        dstIndex = (dstIndex + 1) & (PORT_CAPTURE_BANK_HALFWORDS - 1);
        srcBIndex = (srcBIndex + 1) & (PORT_CAPTURE_BANK_HALFWORDS - 1);
    }
    return width;
}

// The pages of bank A-D in the LCDC view, or all NULL when the bank isn't there
static void LcdcBankPages(unsigned bank, uint8_t *pages[PORT_CAPTURE_BANK_PAGES]) {
    int first = port_vram_bank_first_page[bank];

    for (int i = 0; i < PORT_CAPTURE_BANK_PAGES; i++) {
        pages[i] = port_vram_slot_ptr(PORT_VRAM_VIEW_LCDC, first + i);
        if (pages[i] == NULL) {
            memset(pages, 0, sizeof(pages[0]) * PORT_CAPTURE_BANK_PAGES);
            return;
        }
    }
}

static uint32_t Read32(const uint8_t *bytes) {
    uint32_t value;

    memcpy(&value, bytes, sizeof(value));
    return value;
}

void port_capture_draw_line(const PortDs2dSource *engine_a, int line) {
    uint8_t *dst[PORT_CAPTURE_BANK_PAGES];
    uint8_t *srcB[PORT_CAPTURE_BANK_PAGES];
    uint32_t dispcapcnt;
    uint32_t dispcnt;
    unsigned bank;
    unsigned width;
    unsigned height;
    unsigned written;

    if (line < 0 || line >= VISIBLE_LINES) {
        return;
    }
    dispcapcnt = Read32(engine_a->io + IO_DISPCAPCNT);
    if (line == 0) {
        sLatched = (dispcapcnt & DISPCAPCNT_ENABLE) != 0;
    }
    if (!sLatched) {
        return;
    }
    dispcnt = Read32(engine_a->io + IO_DISPCNT);
    bank = (dispcapcnt >> 16) & 3;
    LcdcBankPages(bank, dst);
    LcdcBankPages((dispcnt >> 18) & 3, srcB);
    written = port_capture_line(dispcapcnt, dispcnt, line, port_ds2d_capture_source_a(), engine_a->bg0_3d_line, dst,
                                (const uint8_t *const *)srcB);
    if (written != 0) {
        // The write, for the caches of what the GPU reads (port/hal/gx/vram.h), in two parts if it wrapped
        uint32_t start;
        uint32_t first;

        CaptureSize(dispcapcnt, &width, &height);
        start = WriteStart(dispcapcnt, width, line);
        first = PORT_CAPTURE_BANK_HALFWORDS - start < written ? PORT_CAPTURE_BANK_HALFWORDS - start : written;
        port_vram_note_write(LCDC_BASE + bank * LCDC_BANK_SIZE + start * 2, first * 2);
        if (first < written) {
            port_vram_note_write(LCDC_BASE + bank * LCDC_BANK_SIZE, (written - first) * 2);
        }
    }
    if (line == VISIBLE_LINES - 1) {
        // The capture is over: the hardware clears the enable bit at the VBlank, before the game's handler can set it
        // for the next frame
        uint32_t cleared = dispcapcnt & ~DISPCAPCNT_ENABLE;

        memcpy(port_hw_pointer(REG_DISPCAPCNT), &cleared, sizeof(cleared));
        sLatched = 0;
    }
}

void port_capture_reset(void) {
    sLatched = 0;
}
