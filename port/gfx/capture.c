// Display capture (port/gfx/capture.h): engine A's DISPCAPCNT, which writes a line of engine A's picture, the 3D
// layer, a VRAM bank or a blend of two into VRAM bank A to D as port/gfx/screens.c draws each line. The game uses it
// for motion blur, for 3D on both screens (src/gfl/double3Ddisp.c) and for the title screen's picture in VRAM D
// (src/ov162/title.c).
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

static int BankMapped(const uint8_t *const pages[PORT_CAPTURE_BANK_PAGES]) {
    for (int i = 0; i < PORT_CAPTURE_BANK_PAGES; i++) {
        if (pages[i] == NULL) {
            return 0;
        }
    }
    return 1;
}

// Source A's pixel x in 5-bit channels with its alpha in bit 15, as a capture of source A alone writes it. The composed
// picture is opaque everywhere, the backdrop included; a 3D pixel is opaque when its alpha isn't 0 (melonDS)
static uint16_t SourceA3d(const uint32_t *line_3d, unsigned x) {
    uint32_t dot = line_3d != NULL ? line_3d[x] : 0;

    return (uint16_t)(((dot >> 19) & 0x1F) | ((dot >> 11) & 0x1F) << 5 | ((dot >> 3) & 0x1F) << 10 |
                      (unsigned)((dot >> 27) != 0) << 15);
}

static uint16_t SourceAComposed(const uint32_t *source_a, unsigned x) {
    uint32_t dot = source_a[x];

    return (uint16_t)(((dot >> 17) & 0x1F) | ((dot >> 9) & 0x1F) << 5 | ((dot >> 1) & 0x1F) << 10 | 1u << 15);
}

// The blend of A and B, (A * alphaA * EVA + B * alphaB * EVB) / 16 with each channel at most 31; melonDS rounds it to
// nearest. The result is opaque where a source that counts (its factor isn't 0) is
static uint16_t Blend(uint16_t valueA, uint16_t valueB, unsigned eva, unsigned evb) {
    unsigned aA = valueA >> 15;
    unsigned aB = valueB >> 15;
    unsigned r = ((valueA & 0x1F) * aA * eva + (valueB & 0x1F) * aB * evb + 8) >> 4;
    unsigned g = (((valueA >> 5) & 0x1F) * aA * eva + ((valueB >> 5) & 0x1F) * aB * evb + 8) >> 4;
    unsigned b = (((valueA >> 10) & 0x1F) * aA * eva + ((valueB >> 10) & 0x1F) * aB * evb + 8) >> 4;
    unsigned a = (eva != 0 && aA) || (evb != 0 && aB);

    return (uint16_t)((r > 31 ? 31 : r) | (g > 31 ? 31 : g) << 5 | (b > 31 ? 31 : b) << 10 | a << 15);
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

    // A line is one run of halfwords in one page of each bank: it starts at a multiple of its width, 128 or 256, of
    // which a page (8,192 halfwords) and the bank are multiples. So it is made in a buffer, one source mode for the
    // whole line, and written at once
    uint16_t valuesA[256];
    uint16_t valuesB[256];
    uint16_t out[256];
    int sourceA3d = (dispcapcnt & DISPCAPCNT_SOURCE_A_3D) != 0;

    if (haveB && !fromFifo) {
        memcpy(valuesB, src_b[srcBIndex / PAGE_HALFWORDS] + (srcBIndex % PAGE_HALFWORDS) * 2, width * 2);
    } else {
        memset(valuesB, 0, width * 2);
    }
    if (source != 1) {
        if (sourceA3d) {
            for (unsigned x = 0; x < width; x++) {
                valuesA[x] = SourceA3d(line_3d, x);
            }
        } else if (source_a != NULL) {
            for (unsigned x = 0; x < width; x++) {
                valuesA[x] = SourceAComposed(source_a, x);
            }
        } else {
            memset(valuesA, 0, width * 2);
        }
    }
    if (source == 0) {
        memcpy(out, valuesA, width * 2);
    } else if (source == 1) {
        memcpy(out, valuesB, width * 2);
    } else {
        for (unsigned x = 0; x < width; x++) {
            out[x] = Blend(valuesA[x], valuesB[x], eva, evb);
        }
    }
    memcpy(dst[dstIndex / PAGE_HALFWORDS] + (dstIndex % PAGE_HALFWORDS) * 2, out, width * 2);
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
