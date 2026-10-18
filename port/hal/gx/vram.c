// The VRAM banks on the host: the views in the region 0x06 window, the layout VRAMCNT gives them, and the two ways to
// apply a new layout (port/spikes/vram-alias, reviewed; this is its Vram object made the HAL's one instance, with the
// CPU views at their DS addresses). Aliasing maps the bank pages into the slots that changed. Copying keeps each bank
// page in one home (its first slot in a CPU view, or else its parking page) and moves the bytes when the home changes.
// The DS never shows a bank in two views at once, so one home per page is enough for every CPU access except one
// through a mirror. A mirror slot has bytes of its own, which loads and stores through it reach instead of the bank: a
// copy of the page made when the slot takes it, so a load there sees the bank's bytes until the next store to the page
// or to the mirror, and a store there never reaches the bank. (The spike cleared mirror slots instead, which made every
// load through one read 0; the copy costs the same bytes.)
//
// Where the views are: the CPU views at their DS offsets in port_hw_mem_06 (16 KiB-aligned, as the aliasing backends
// need). The GPU-only views, which nothing on the DS addresses, are a separate block of pages for the aliasing
// backends; the copy backend keeps their pages in the parking pages and needs none. The rest of the window (the DS's
// mirrors of the views, LCDC past bank I) stays plain memory.
#include "hal/gx/vram.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/vram_os.h"
#include "hal/host/host.h"
#include "hal/hw/hw.h"
#include "hal/os/port_app.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const port_vram_backend_name[PORT_VRAM_BACKEND_COUNT] = { "shm", "mach-entry", "mach-remap", "copy" };

// The DS's VRAM region and VRAMCNT: A-G at 0x04000240-0x04000246, then WRAMCNT, then H and I
#define VRAM_REGION 0x06000000u
#define REG_VRAMCNT 0x04000240u
static const uint8_t sVramCntOffset[PORT_VRAM_BANK_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 8, 9 };

typedef struct Vram {
    bool ready;
    PortVramBackend backend;
    uint8_t *views[PORT_VRAM_VIEW_COUNT];
    uint8_t vramcnt[PORT_VRAM_BANK_COUNT];
    PortVramLayout layout;
    PortVramLayout next;
    uint32_t pageGeneration[PORT_VRAM_PAGE_COUNT];
    uint32_t slotGeneration[PORT_VRAM_VIEW_COUNT][PORT_VRAM_MAX_SLOTS];
    PortVramStats stats;
    // Aliasing: the shared bank object, and the block of the GPU-only views
    PortVramOs os;
    uint8_t *gpuViews;
    size_t gpuViewsSize;
    // Copying: the parking pages and where each bank page is now
    uint8_t *park;
    uint8_t *home[PORT_VRAM_PAGE_COUNT];
} Vram;

static Vram sVram;

static bool same_slot(PortVramSlot a, PortVramSlot b) {
    return a.bank == b.bank && a.page == b.page && a.mirror == b.mirror;
}

static int page_index(PortVramSlot s) {
    return port_vram_bank_first_page[s.bank] + s.page;
}

static uint8_t *slot_address(PortVramView view, int slot) {
    return sVram.views[view] + ((size_t)slot << PORT_VRAM_SLOT_SHIFT);
}

// Whether slot t, k slots after s, continues s's run: the next page of the same bank, or no bank after no bank
static bool continues_run(PortVramSlot s, int k, PortVramSlot t) {
    if (s.bank == PORT_VRAM_BANK_NONE) {
        return t.bank == PORT_VRAM_BANK_NONE;
    }
    return t.bank == s.bank && t.page == s.page + k;
}

static int alias_map_run(PortVramView view, int slot, int count, PortVramSlot s) {
    uint8_t *addr = slot_address(view, slot);
    size_t len = (size_t)count << PORT_VRAM_SLOT_SHIFT;

    if (s.bank == PORT_VRAM_BANK_NONE) {
        return port_vram_os_map_zero(&sVram.os, addr, len);
    }
    return port_vram_os_map_banks(&sVram.os, addr, len, (size_t)page_index(s) << PORT_VRAM_SLOT_SHIFT);
}

// Maps every changed slot, one call per run of slots that continue each other. A slot with no bank gets fresh zero
// pages: it reads 0 as on the DS, and a write to it is kept only until its mapping next changes
static int alias_apply(const PortVramLayout *next) {
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        const PortVramSlot *cur = sVram.layout.slots[view];
        const PortVramSlot *nxt = next->slots[view];
        int count = port_vram_view_slot_count(view);
        int i = 0;

        while (i < count) {
            int j = i + 1;
            int err;

            if (same_slot(cur[i], nxt[i])) {
                i++;
                continue;
            }
            while (j < count && !same_slot(cur[j], nxt[j]) && continues_run(nxt[i], j - i, nxt[j])) {
                j++;
            }
            err = alias_map_run(view, i, j - i, nxt[i]);
            if (err != 0) {
                return err;
            }
            i = j;
        }
    }
    sVram.stats.os_calls = sVram.os.calls;
    return 0;
}

static uint8_t *park_page(int p) {
    return sVram.park + ((size_t)p << PORT_VRAM_SLOT_SHIFT);
}

static void copy_page(int p, uint8_t *to) {
    memcpy(to, sVram.home[p], PORT_VRAM_SLOT_SIZE);
    sVram.home[p] = to;
    sVram.stats.bytes_copied += PORT_VRAM_SLOT_SIZE;
}

// Whether a page other than p still has to move out of a slot
static bool slot_taken(const bool moving[PORT_VRAM_PAGE_COUNT], const uint8_t *slot, int p) {
    for (int q = 0; q < PORT_VRAM_PAGE_COUNT; q++) {
        if (q != p && moving[q] && sVram.home[q] == slot) {
            return true;
        }
    }
    return false;
}

// Moves the bank pages whose home changes, each straight to its new home once no other page is still there. Only
// pages that trade slots go through a parking page. A page in the GPU-only views lives in its parking page: game code
// never reaches those views, and the renderers read them through port_vram_slot_ptr. Then the CPU slots that lost their
// page are cleared, and each mirror slot that took a page gets a copy of it from the page's home, its own bytes from
// then on (port_vram_slot_ptr shows the page itself)
static void copy_apply(const PortVramLayout *next) {
    uint8_t *newHome[PORT_VRAM_PAGE_COUNT];
    bool moving[PORT_VRAM_PAGE_COUNT];
    int left = 0;

    for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
        newHome[p] = park_page(p);
    }
    for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
        for (int slot = 0; slot < port_vram_view_slot_count(view); slot++) {
            PortVramSlot s = next->slots[view][slot];
            if (s.bank != PORT_VRAM_BANK_NONE && !s.mirror) {
                newHome[page_index(s)] = slot_address(view, slot);
            }
        }
    }
    for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
        moving[p] = sVram.home[p] != newHome[p];
        left += moving[p];
    }
    while (left > 0) {
        int moved = 0;

        for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
            // A page's parking page is only ever its own, so it is always free
            if (moving[p] && !slot_taken(moving, newHome[p], p)) {
                copy_page(p, newHome[p]);
                moving[p] = false;
                moved++;
            }
        }
        if (moved == 0) {
            // Every page left waits for another: a cycle. Parking one of them frees its slot
            for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
                if (moving[p] && sVram.home[p] != park_page(p)) {
                    copy_page(p, park_page(p));
                    break;
                }
            }
        }
        left -= moved;
    }
    for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
        for (int slot = 0; slot < port_vram_view_slot_count(view); slot++) {
            PortVramSlot s = next->slots[view][slot];
            if (same_slot(sVram.layout.slots[view][slot], s)) {
                continue;
            }
            if (s.bank == PORT_VRAM_BANK_NONE) {
                memset(slot_address(view, slot), 0, PORT_VRAM_SLOT_SIZE);
                sVram.stats.bytes_zeroed += PORT_VRAM_SLOT_SIZE;
            } else if (s.mirror) {
                // Every page is in its new home now, and no home is a mirror slot
                memcpy(slot_address(view, slot), sVram.home[page_index(s)], PORT_VRAM_SLOT_SIZE);
                sVram.stats.bytes_copied += PORT_VRAM_SLOT_SIZE;
            }
        }
    }
}

// Every page that leaves or enters a slot gets a new generation, and so does the slot
static void bump_generations(const PortVramLayout *next) {
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < port_vram_view_slot_count(view); slot++) {
            PortVramSlot a = sVram.layout.slots[view][slot];
            PortVramSlot b = next->slots[view][slot];
            if (same_slot(a, b)) {
                continue;
            }
            sVram.slotGeneration[view][slot]++;
            if (a.bank != PORT_VRAM_BANK_NONE) {
                sVram.pageGeneration[page_index(a)]++;
            }
            if (b.bank != PORT_VRAM_BANK_NONE) {
                sVram.pageGeneration[page_index(b)]++;
            }
        }
    }
}

// Applies VRAMCNT as it now stands: works out the layout of every view, and when it differs from the current one,
// moves the pages that changed slot with the backend in use and gives them new generations. A mapping call that
// fails stops the program, since no layout describes the views left behind
static void apply(void) {
    port_vram_layout_compute(sVram.vramcnt, &sVram.next);
    if (sVram.next.overlaps != 0) {
        sVram.stats.overlaps++;
    }
    if (memcmp(sVram.next.slots, sVram.layout.slots, sizeof(sVram.layout.slots)) == 0) {
        sVram.layout.overlaps = sVram.next.overlaps;
        return;
    }
    sVram.stats.remaps++;
    bump_generations(&sVram.next);
    if (sVram.backend == PORT_VRAM_BACKEND_COPY) {
        copy_apply(&sVram.next);
    } else {
        int err = alias_apply(&sVram.next);
        if (err != 0) {
            // A failed mapping call leaves the views in a state no layout describes; nothing can go on from there
            fprintf(stderr, "port: VRAM: a mapping call failed (%s)\n", strerror(err));
            abort();
        }
    }
    sVram.layout = sVram.next;
}

int port_vram_backend_available(PortVramBackend backend) {
    if (backend >= PORT_VRAM_BACKEND_COUNT || !port_vram_os_backend_available(backend)) {
        return 0;
    }
    // The aliasing backends map pages over the window, which must start on a host page
    return backend == PORT_VRAM_BACKEND_COPY || (uintptr_t)port_hw_pointer(VRAM_REGION) % port_host_page_size() == 0;
}

void port_vram_shutdown(void) {
    if (!sVram.ready) {
        return;
    }
    if (sVram.backend == PORT_VRAM_BACKEND_COPY) {
        for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
            memset(sVram.views[view], 0, port_vram_view_size[view]);
        }
        free(sVram.park);
    } else {
        // The CPU views become the window's plain zeroed memory again
        for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
            port_vram_os_map_zero(&sVram.os, sVram.views[view], port_vram_view_size[view]);
        }
        if (sVram.gpuViews != NULL) {
            port_vram_os_release(sVram.gpuViews, sVram.gpuViewsSize);
        }
        port_vram_os_banks_destroy(&sVram.os);
    }
    memset(&sVram, 0, sizeof(sVram));
}

int port_vram_init(PortVramBackend backend) {
    int err = 0;

    port_vram_shutdown();
    if (!port_vram_backend_available(backend)) {
        return ENOTSUP;
    }
    sVram.backend = backend;
    sVram.os.backend = backend;
    sVram.os.fd = -1;
    for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
        sVram.views[view] = port_hw_pointer(port_vram_view_ds_address[view]);
    }

    if (backend == PORT_VRAM_BACKEND_COPY) {
        // Plain memory and no OS calls: this backend runs anywhere C does
        sVram.park = calloc(1, PORT_VRAM_BANKS_SIZE);
        if (sVram.park == NULL) {
            err = ENOMEM;
            goto fail;
        }
        for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
            sVram.home[p] = park_page(p);
        }
        for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
            memset(sVram.views[view], 0, port_vram_view_size[view]);
        }
    } else {
        uint8_t *at;

        err = port_vram_os_banks_create(&sVram.os);
        if (err != 0) {
            goto fail;
        }
        for (int view = PORT_VRAM_CPU_VIEW_COUNT; view < PORT_VRAM_VIEW_COUNT; view++) {
            sVram.gpuViewsSize += port_vram_view_size[view];
        }
        err = port_vram_os_allocate(sVram.gpuViewsSize, &sVram.gpuViews);
        if (err != 0) {
            sVram.gpuViews = NULL;
            goto fail;
        }
        at = sVram.gpuViews;
        for (int view = PORT_VRAM_CPU_VIEW_COUNT; view < PORT_VRAM_VIEW_COUNT; view++) {
            sVram.views[view] = at;
            at += port_vram_view_size[view];
        }
        // Every slot starts with no bank, as every VRAMCNT starts at 0: fresh zero pages, whatever the game or an
        // earlier setup left in the window
        for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
            err = port_vram_os_map_zero(&sVram.os, sVram.views[view], port_vram_view_size[view]);
            if (err != 0) {
                goto fail;
            }
        }
    }
    port_vram_layout_compute(sVram.vramcnt, &sVram.layout);
    sVram.stats.os_calls = sVram.os.calls;
    sVram.ready = true;
    return 0;

fail:
    sVram.ready = true;
    port_vram_shutdown();
    return err;
}

// The backend the game runs with: --vram-backend's, else shm where it can run (macOS, Linux with pages of 16 KiB or
// less), else copying
static PortVramBackend chosen_backend(void) {
    const char *name = port_app_option("vram-backend");

    if (name != NULL) {
        for (int b = 0; b < PORT_VRAM_BACKEND_COUNT; b++) {
            if (strcmp(name, port_vram_backend_name[b]) == 0) {
                if (port_vram_backend_available(b)) {
                    return b;
                }
                fprintf(stderr, "port: VRAM: the %s backend can't run here; copying instead\n", name);
                return PORT_VRAM_BACKEND_COPY;
            }
        }
        fprintf(stderr, "port: VRAM: no backend named %s (shm, mach-entry, mach-remap, copy)\n", name);
    }
    return port_vram_backend_available(PORT_VRAM_BACKEND_SHM) ? PORT_VRAM_BACKEND_SHM : PORT_VRAM_BACKEND_COPY;
}

// Sets the views up on first use
static void ensure_ready(void) {
    if (sVram.ready) {
        return;
    }
    PortVramBackend backend = chosen_backend();
    int err = port_vram_init(backend);
    if (err != 0 && backend != PORT_VRAM_BACKEND_COPY) {
        fprintf(stderr, "port: VRAM: the %s backend failed (%s); copying instead\n", port_vram_backend_name[backend],
                strerror(err));
        err = port_vram_init(PORT_VRAM_BACKEND_COPY);
    }
    if (err != 0) {
        fprintf(stderr, "port: VRAM: no backend could start (%s)\n", strerror(err));
        abort();
    }
}

PortVramBackend port_vram_backend(void) {
    ensure_ready();
    return sVram.backend;
}

void port_vram_sync(void) {
    const uint8_t *regs = port_hw_pointer(REG_VRAMCNT);
    uint8_t vramcnt[PORT_VRAM_BANK_COUNT];

    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        vramcnt[b] = regs[sVramCntOffset[b]];
    }
    port_vram_set_all_vramcnt(vramcnt);
}

void port_vram_set_all_vramcnt(const uint8_t vramcnt[PORT_VRAM_BANK_COUNT]) {
    ensure_ready();
    if (memcmp(sVram.vramcnt, vramcnt, PORT_VRAM_BANK_COUNT) == 0) {
        return;
    }
    memcpy(sVram.vramcnt, vramcnt, PORT_VRAM_BANK_COUNT);
    apply();
}

uint8_t port_vram_vramcnt(PortVramBank bank) {
    ensure_ready();
    return sVram.vramcnt[bank];
}

const PortVramLayout *port_vram_layout(void) {
    ensure_ready();
    return &sVram.layout;
}

uint8_t *port_vram_view(PortVramView view) {
    ensure_ready();
    return sVram.views[view];
}

uint8_t *port_vram_slot_ptr(PortVramView view, int slot) {
    PortVramSlot s;

    ensure_ready();
    s = sVram.layout.slots[view][slot];
    if (s.bank == PORT_VRAM_BANK_NONE) {
        return NULL;
    }
    if (sVram.backend == PORT_VRAM_BACKEND_COPY) {
        return sVram.home[page_index(s)];
    }
    return slot_address(view, slot);
}

uint8_t *port_vram_bank_page(PortVramBank bank, int page) {
    int p = port_vram_bank_first_page[bank] + page;

    ensure_ready();
    if (sVram.backend == PORT_VRAM_BACKEND_COPY) {
        return sVram.home[p];
    }
    return sVram.os.window + ((size_t)p << PORT_VRAM_SLOT_SHIFT);
}

// The CPU view and the offset in it of a DS address, with the DS's mirrors folded in, or -1
static int cpu_view_of(uint32_t address, uint32_t *offset) {
    uint32_t local = address - VRAM_REGION;

    if ((address >> 24) != (VRAM_REGION >> 24)) {
        return -1;
    }
    switch (local >> 21) {
    case 0:
        *offset = local & 0x7ffff;
        return PORT_VRAM_VIEW_BG_A;
    case 1:
        *offset = local & 0x1ffff;
        return PORT_VRAM_VIEW_BG_B;
    case 2:
        *offset = local & 0x3ffff;
        return PORT_VRAM_VIEW_OBJ_A;
    case 3:
        *offset = local & 0x1ffff;
        return PORT_VRAM_VIEW_OBJ_B;
    default:
        *offset = local - 0x800000;
        return *offset < PORT_VRAM_BANKS_SIZE ? PORT_VRAM_VIEW_LCDC : -1;
    }
}

void *port_vram_cpu_pointer(uint32_t address) {
    uint32_t offset;
    int view = cpu_view_of(address, &offset);

    if (view < 0) {
        return port_hw_pointer(address);
    }
    ensure_ready();
    return sVram.views[view] + offset;
}

void port_vram_note_write(uint32_t address, uint32_t size) {
    uint32_t end = address + size;

    ensure_ready();
    while (address < end) {
        uint32_t offset;
        int view = cpu_view_of(address, &offset);
        uint32_t chunk = PORT_VRAM_SLOT_SIZE - (address & (PORT_VRAM_SLOT_SIZE - 1));

        if (view >= 0) {
            PortVramSlot s = sVram.layout.slots[view][offset >> PORT_VRAM_SLOT_SHIFT];
            if (s.bank != PORT_VRAM_BANK_NONE) {
                int p = page_index(s);
                sVram.pageGeneration[p]++;
                // Every slot that shows the page, in every view
                for (int v = 0; v < PORT_VRAM_VIEW_COUNT; v++) {
                    for (int i = 0; i < port_vram_view_slot_count(v); i++) {
                        PortVramSlot t = sVram.layout.slots[v][i];
                        if (t.bank != PORT_VRAM_BANK_NONE && page_index(t) == p) {
                            sVram.slotGeneration[v][i]++;
                        }
                    }
                }
            }
        }
        if (chunk > end - address) {
            chunk = end - address;
        }
        address += chunk;
    }
}

uint32_t port_vram_page_generation(PortVramBank bank, int page) {
    ensure_ready();
    return sVram.pageGeneration[port_vram_bank_first_page[bank] + page];
}

uint32_t port_vram_slot_generation(PortVramView view, int slot) {
    ensure_ready();
    return sVram.slotGeneration[view][slot];
}

const uint8_t *port_vram_tex_page(int page) {
    return port_vram_slot_ptr(PORT_VRAM_VIEW_TEX, page);
}

const uint8_t *port_vram_texpltt_page(int page) {
    return port_vram_slot_ptr(PORT_VRAM_VIEW_TEXPLTT, page);
}

uint32_t port_vram_tex_generation(int page) {
    return port_vram_slot_generation(PORT_VRAM_VIEW_TEX, page);
}

uint32_t port_vram_texpltt_generation(int page) {
    return port_vram_slot_generation(PORT_VRAM_VIEW_TEXPLTT, page);
}

// An extended palette slot of 8 KiB: half of a 16 KiB slot of the view
static const uint8_t *ext_palette(PortVramView view, int slot8k) {
    const uint8_t *page = port_vram_slot_ptr(view, slot8k / 2);
    return page != NULL ? page + (slot8k % 2) * 0x2000 : NULL;
}

void port_vram_fill_source(PortDs2dSource *src, int engine) {
    PortVramView bg = engine == 0 ? PORT_VRAM_VIEW_BG_A : PORT_VRAM_VIEW_BG_B;
    PortVramView obj = engine == 0 ? PORT_VRAM_VIEW_OBJ_A : PORT_VRAM_VIEW_OBJ_B;
    PortVramView bgExt = engine == 0 ? PORT_VRAM_VIEW_BGEXT_A : PORT_VRAM_VIEW_BGEXT_B;
    PortVramView objExt = engine == 0 ? PORT_VRAM_VIEW_OBJEXT_A : PORT_VRAM_VIEW_OBJEXT_B;
    int count;

    ensure_ready();
    // The engine's registers from 0x04000000 or 0x04001000, its 1 KiB of palettes from 0x05000000 or 0x05000400 (BG,
    // then OBJ) and its OAM from 0x07000000 or 0x07000400
    src->io = port_hw_pointer(0x04000000u + (uint32_t)engine * 0x1000);
    src->palette = port_hw_pointer(0x05000000u + (uint32_t)engine * 0x400);
    src->oam = port_hw_pointer(0x07000000u + (uint32_t)engine * 0x400);

    count = port_vram_view_slot_count(bg);
    for (int i = 0; i < 32; i++) {
        src->bg_slots[i] = i < count ? port_vram_slot_ptr(bg, i) : NULL;
    }
    count = port_vram_view_slot_count(obj);
    for (int i = 0; i < 16; i++) {
        src->obj_slots[i] = i < count ? port_vram_slot_ptr(obj, i) : NULL;
    }
    for (int i = 0; i < 4; i++) {
        src->bg_ext_palette[i] = ext_palette(bgExt, i);
    }
    src->obj_ext_palette = port_vram_slot_ptr(objExt, 0);
    for (int i = 0; i < PORT_VRAM_PAGE_COUNT; i++) {
        src->lcdc_slots[i] = engine == 0 ? port_vram_slot_ptr(PORT_VRAM_VIEW_LCDC, i) : NULL;
    }
}

const PortVramStats *port_vram_stats(void) {
    ensure_ready();
    return &sVram.stats;
}
