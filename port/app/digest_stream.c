// The digest stream (--digest FILE, port/app/digest_stream.h): at each VBlank, from the program's frame hook, the state
// of the DS-native path that every screen of the game is drawn from, read where the platform layer keeps it, goes to
// the stream's writer (port/tools/oracle/digest.c), which pbw-oracle's --digest shares. What it reads, by group of
// the digest (port/tools/oracle/README.md, "Digests"): the VRAM banks page by page and VRAMCNT (port/hal/gx/vram.h);
// the palettes and OAM, the 2D engines' registers, POWCNT1 and WRAMCNT, and the 3D rendering registers, from the I/O
// registers' bytes (port/hal/hw/hw.h); the render list the geometry engine's swap has just made the one the next frame
// draws (port/gfx/ge/ge.h); the frame's display capture as port/gfx/capture.c noted it at line 96; and both screens as
// frames.tsv hashes them (port/gfx/screens.h). It tracks which banks hold a capture of 3D exactly as pbw-oracle tracks
// melonDS's, with the same plain C (port/tools/oracle/display3d.c) at the same three moments: as each frame starts,
// before line 0; at line 96, the capture's record; and at the VBlank. It reads the game's display state and changes
// nothing: no pixel, register or memory of the game's. Nothing it reads is presentation, so a digest run is a run of
// the native 256x192 path, as replays and run directories are (ADR 0030).
#include "app/digest_stream.h"
#include "gfx/capture.h"
#include "gfx/ge/ge.h"
#include "gfx/ge/ge_hal.h"
#include "gfx/screens.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/gx/vram.h"
#include "hal/hw/hw.h"
#include "tools/oracle/digest.h"
#include "tools/oracle/display3d.h"
#include <stdio.h>
#include <string.h>

// The I/O registers: engine A's 2D registers, the 3D rendering registers, POWCNT1 and WRAMCNT from the first, engine
// B's 2D registers from the second; the standard palettes and OAM
#define REG_IO 0x04000000u
#define REG_IO_ENGINE_B 0x04001000u
#define PALETTES 0x05000000u
#define OAM 0x07000000u

static FILE *sFile;
static const char *sPath;
static int sFailed;
static PbwDigestWriter sWriter;

// What banks A-D hold of 3D (display3d.h), and what they held as the frame being drawn started
static PbwBanks3d sBanks;
static uint8_t sHolds3dAtStart[PBW_DISPLAY_BANKS];
// Banks A-D in one block each, for a backend that keeps a bank's pages apart (copying)
static uint8_t sBankCopies[PBW_DISPLAY_BANKS][PBW_DISPLAY_BANK_SIZE];

// The render list in the digest's form, with the list's own vertex indices
static PbwDigestPolygon sPolygons[PORT_GE_MAX_POLYGONS];
static PbwDigestVertex sVertices[PORT_GE_MAX_VERTICES];

// The frame given to the writer, kept here rather than on the dispatcher's stack
static PbwDigestFrame sFrame;

// Banks A-D's 128 KiB each, as display3d.c hashes the regions captures wrote: the bank itself where the backend keeps
// its pages one after another (the aliasing ones, port/hal/gx/vram.h), else a copy of its pages
static void BankMemory(const uint8_t *memory[PBW_DISPLAY_BANKS]) {
    for (int bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        const uint8_t *first = port_vram_bank_page((PortVramBank)bank, 0);
        int together = 1;

        for (int page = 1; page < PORT_CAPTURE_BANK_PAGES; page++) {
            together &= (uintptr_t)port_vram_bank_page((PortVramBank)bank, page) ==
                        (uintptr_t)first + (uintptr_t)page * PORT_VRAM_SLOT_SIZE;
        }
        if (together) {
            memory[bank] = first;
            continue;
        }
        for (int page = 0; page < PORT_CAPTURE_BANK_PAGES; page++) {
            memcpy(sBankCopies[bank] + (size_t)page * PORT_VRAM_SLOT_SIZE,
                   port_vram_bank_page((PortVramBank)bank, page), PORT_VRAM_SLOT_SIZE);
        }
        memory[bank] = sBankCopies[bank];
    }
}

// The screens' frame-start hook, before line 0: a bank whose captured region the CPU or a DMA wrote since its capture
// holds no 3D any more, and what the banks hold now is what the frame starts with, as pbw-oracle checks before
// melonDS's line 0
static void FrameStart(void) {
    const uint8_t *memory[PBW_DISPLAY_BANKS];

    BankMemory(memory);
    pbw_display3d_check(&sBanks, memory);
    memcpy(sHolds3dAtStart, sBanks.holds3d, sizeof(sHolds3dAtStart));
}

// The render list the next frame draws, which the swap at this VBlank made (port_g3d_vblank runs before the program's
// frame hook), in the digest's form: each polygon with its vertices as indices into the list's vertices
static void FillList(PbwDigestList *out) {
    struct PortGe *ge = port_g3d_engine();
    const PortGeList *list = port_ge_render_list(ge);

    for (uint32_t i = 0; i < list->polygon_count; i++) {
        const PortGePolygon *polygon = &list->polygons[i];
        PbwDigestPolygon *to = &sPolygons[i];

        to->attr = polygon->attr;
        to->texParam = polygon->tex_param;
        to->plttBase = polygon->pltt_base;
        to->vertexCount = polygon->vertex_count;
        to->front = (polygon->flags & PORT_GE_POLYGON_FRONT) != 0;
        for (uint32_t j = 0; j < polygon->vertex_count && j < PBW_DIGEST_POLYGON_VERTICES; j++) {
            to->vertices[j] = polygon->vertices[j];
        }
    }
    for (uint32_t i = 0; i < list->vertex_count; i++) {
        const PortGeVertex *vertex = &list->vertices[i];
        PbwDigestVertex *to = &sVertices[i];

        to->x = vertex->x;
        to->y = vertex->y;
        to->depth = (uint32_t)vertex->z;
        to->w = (uint32_t)vertex->w;
        for (int c = 0; c < 3; c++) {
            to->color[c] = vertex->color[c];
        }
        to->s = (uint16_t)vertex->s;
        to->t = (uint16_t)vertex->t;
    }
    out->polygons = sPolygons;
    out->polygonCount = list->polygon_count;
    out->vertices = sVertices;
    out->vertexCount = list->vertex_count;
    out->swapParam = list->swap_param;
    out->ramOverflow = port_ge_ram_overflow(ge) ? 1 : 0;
}

int port_digest_stream_open(const char *path) {
    sFile = fopen(path, "w");
    if (sFile == NULL) {
        fprintf(stderr, "port: --digest: can't write %s\n", path);
        return 0;
    }
    if (pbw_digest_writer_open(&sWriter, sFile) != 0) {
        fprintf(stderr, "port: --digest: %s: %s\n", path, sWriter.error);
        fclose(sFile);
        sFile = NULL;
        return 0;
    }
    sPath = path;
    sFailed = 0;
    // Nothing holds 3D at power-on
    pbw_display3d_init(&sBanks);
    memset(sHolds3dAtStart, 0, sizeof(sHolds3dAtStart));
    port_screens_frame_start_hook = FrameStart;
    return 1;
}

int port_digest_stream_active(void) {
    return sFile != NULL;
}

int port_digest_stream_frame(uint32_t frame) {
    const uint8_t *io = port_hw_pointer(REG_IO);
    const PortCaptureRecord *record = port_capture_record();
    const uint8_t *memory[PBW_DISPLAY_BANKS];

    if (sFile == NULL || sFailed) {
        return sFile != NULL ? 0 : 1;
    }
    memset(&sFrame, 0, sizeof(sFrame));
    for (int bank = 0; bank < PBW_DIGEST_VRAM_BANKS; bank++) {
        for (unsigned page = 0; page < pbw_digest_vram_pages(bank); page++) {
            sFrame.vram.pages[bank][page] = port_vram_bank_page((PortVramBank)bank, (int)page);
        }
        sFrame.vram.vramcnt[bank] = port_vram_vramcnt((PortVramBank)bank);
    }
    sFrame.palette = port_hw_pointer(PALETTES);
    sFrame.oam = port_hw_pointer(OAM);
    pbw_digest_regs2d_from_io(&sFrame.regs2d[PBW_DIGEST_ENGINE_A], io, PBW_DIGEST_ENGINE_A);
    pbw_digest_regs2d_from_io(&sFrame.regs2d[PBW_DIGEST_ENGINE_B], port_hw_pointer(REG_IO_ENGINE_B),
                              PBW_DIGEST_ENGINE_B);
    pbw_digest_sys_from_io(&sFrame.sys, io);
    // The bytes port_g3d_vblank has just decoded into the renderer's own copy, at the same VBlank
    pbw_digest_regs3d_from_io(&sFrame.regs3d, io);
    // The frame's capture as the unit had it at line 96, and what it left of 3D in the banks
    sFrame.capture.ran = record->ran;
    sFrame.capture.dispcapcnt = record->dispcapcnt;
    sFrame.capture.dispcntA = record->dispcnt_a;
    sFrame.capture.destInLcdc = record->dest_in_lcdc;
    BankMemory(memory);
    pbw_display3d_capture(&sBanks, &sFrame.capture, sHolds3dAtStart, memory);
    sFrame.banks3d = sBanks;
    FillList(&sFrame.list);
    sFrame.screens[0] = port_screens_pixels(PORT_SCREEN_TOP);
    sFrame.screens[1] = port_screens_pixels(PORT_SCREEN_BOTTOM);
    if (pbw_digest_writer_vblank(&sWriter, port_gfl_frame_count(), frame, &sFrame) != 0) {
        fprintf(stderr, "port: --digest: %s: %s\n", sPath, sWriter.error);
        sFailed = 1;
        return 0;
    }
    return 1;
}

int port_digest_stream_close(void) {
    int ok;

    if (sFile == NULL) {
        return 1;
    }
    port_screens_frame_start_hook = NULL;
    ok = pbw_digest_writer_close(&sWriter) == 0;
    ok = fclose(sFile) == 0 && ok;
    if (!ok && !sFailed) {
        fprintf(stderr, "port: --digest: can't write %s\n", sPath);
    }
    sFile = NULL;
    return ok && !sFailed;
}
