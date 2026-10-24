// Tests of port/hal/gx: the VRAM banks in the region 0x06 window, the SDK's VRAMCNT functions through the HAL's
// wrappers, the 3D and extended palette loads, the geometry FIFO copies, the geometry command log's reads of a command
// port (port/hal/hw/hw.c), which SPL's particles do, the renderer's source and the field's building transition's clear
// of LCDC VRAM. The conformance
// tests are the vram-alias spike's (port/spikes/vram-alias/test_vram.c), here on the real window, with the CPU views
// at their DS addresses. Usage:
//   port_gx_tests layout                 the layout against GBATEK's places, without the SDK
//   port_gx_tests backend <name>         one backend (shm, mach-entry, mach-remap, copy) against the DS's behaviour
//                                        and a reference model; exits 77 when the backend can't run here
//   port_gx_tests <scenario> [backend]   an SDK scenario (sdk_settings, wrappers, tex_upload, texpltt_upload,
//                                        clear_image, ext_palettes, fifo, fill_source, cpu_pointer, default_backend,
//                                        display, lcdc_clear, parameter_read),
//                                        run as the game's main() on the OS core, with the backend given or the
//                                        default one
#include "gfx/ge/ge.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/gx_display.h"
#include "hal/gx/vram.h"
#include "hal/host/host.h"
#include "hal/hw/hw.h"
#include "hal/os/port_os.h"
#include "lib/spl/src/spl_internal.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "spl/spl.h"
#include "tests/gx/sdk_settings.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sFailures;
static const char *sContext = "";

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "%s:%d: [%s] check failed: %s\n", __FILE__, __LINE__, sContext, #cond);                    \
            sFailures++;                                                                                               \
        }                                                                                                              \
    } while (0)

#define LCDC 0x80

static uint8_t cnt(int mst, int ofs) {
    return (uint8_t)(0x80 | (ofs << 3) | mst);
}

static uint32_t next_word(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static void fill_pattern(uint8_t *p, size_t len, uint32_t seed) {
    uint32_t state = seed;
    for (size_t i = 0; i < len; i += 4) {
        uint32_t w = next_word(&state);
        memcpy(p + i, &w, 4);
    }
}

static bool has_pattern(const uint8_t *p, size_t len, uint32_t seed) {
    uint32_t state = seed;
    if (p == NULL) {
        return false;
    }
    for (size_t i = 0; i < len; i += 4) {
        uint32_t w = next_word(&state);
        if (memcmp(p + i, &w, 4) != 0) {
            return false;
        }
    }
    return true;
}

static bool all_bytes(const uint8_t *p, size_t len, uint8_t value) {
    for (size_t i = 0; i < len; i++) {
        if (p[i] != value) {
            return false;
        }
    }
    return true;
}

static bool slot_is(const PortVramLayout *l, PortVramView view, int slot, int bank, int page, int mirror) {
    PortVramSlot s = l->slots[view][slot];
    return s.bank == bank && s.page == page && s.mirror == mirror;
}

static int count_placed(const PortVramLayout *l) {
    int n = 0;
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < PORT_VRAM_MAX_SLOTS; slot++) {
            n += l->slots[view][slot].bank != PORT_VRAM_BANK_NONE;
        }
    }
    return n;
}

// Whether a layout has a setting's places, mirrors included, and nothing else
static void check_places(const PortVramLayout *l, const SdkSetting *set) {
    int expected = 0;

    for (int k = 0; k < 8 && set->places[k].bank != 0; k++) {
        const SdkPlace *p = &set->places[k];
        int bank = p->bank - 'A';
        int pages = p->pages != 0 ? p->pages : (int)(port_vram_bank_size[bank] >> PORT_VRAM_SLOT_SHIFT);
        int first = (int)(p->offset >> PORT_VRAM_SLOT_SHIFT);

        CHECK(p->offset % PORT_VRAM_SLOT_SIZE == 0);
        CHECK(first + pages <= port_vram_view_slot_count(p->view));
        for (int page = 0; page < pages && first + page < PORT_VRAM_MAX_SLOTS; page++) {
            CHECK(slot_is(l, p->view, first + page, bank, page, p->mirror));
        }
        expected += pages;
    }
    CHECK(count_placed(l) == expected);
}

// ---- The layout alone ----

// Each SDK setting's VRAMCNT bytes, from every register at 0: its banks are where GBATEK puts them, and nowhere else.
// Then every register at 0 and in LCDC, a collision, and invariants over every value of every register, which are all
// that covers the values the SDK never writes
static void test_layout(void) {
    uint8_t c[PORT_VRAM_BANK_COUNT] = { 0 };
    PortVramLayout l;
    int rows = 0;

    for (size_t i = 0; i < NELEMS(SDK_SETTINGS); i++) {
        const SdkSetting *set = &SDK_SETTINGS[i];
        uint8_t w[PORT_VRAM_BANK_COUNT] = { 0 };

        sContext = set->name;
        for (int k = 0; k < 4 && set->writes[k].bank != 0; k++) {
            w[set->writes[k].bank - 'A'] = set->writes[k].value;
        }
        port_vram_layout_compute(w, &l);
        CHECK(l.overlaps == 0);
        check_places(&l, set);
        rows++;
    }
    CHECK(rows == 85 + PORT_VRAM_BANK_COUNT);

    sContext = "layout";
    port_vram_layout_compute(c, &l);
    CHECK(count_placed(&l) == 0);
    memset(c, LCDC, sizeof(c));
    port_vram_layout_compute(c, &l);
    CHECK(count_placed(&l) == PORT_VRAM_PAGE_COUNT);
    CHECK(l.overlaps == 0);

    // Sub extended palettes and C given to the ARM7 (nowhere on the host) at once
    memset(c, 0, sizeof(c));
    c[PORT_VRAM_BANK_H] = cnt(2, 0);
    c[PORT_VRAM_BANK_I] = cnt(3, 0);
    c[PORT_VRAM_BANK_C] = cnt(2, 0);
    port_vram_layout_compute(c, &l);
    CHECK(slot_is(&l, PORT_VRAM_VIEW_BGEXT_B, 0, PORT_VRAM_BANK_H, 0, 0));
    CHECK(slot_is(&l, PORT_VRAM_VIEW_BGEXT_B, 1, PORT_VRAM_BANK_H, 1, 0));
    CHECK(slot_is(&l, PORT_VRAM_VIEW_OBJEXT_B, 0, PORT_VRAM_BANK_I, 0, 0));
    CHECK(count_placed(&l) == 3);

    // Two banks on one slot: the lower bank keeps it, and the layout counts the collision
    memset(c, 0, sizeof(c));
    c[PORT_VRAM_BANK_A] = cnt(1, 0);
    c[PORT_VRAM_BANK_E] = cnt(1, 0);
    port_vram_layout_compute(c, &l);
    CHECK(l.overlaps == 4);
    CHECK(slot_is(&l, PORT_VRAM_VIEW_BG_A, 0, PORT_VRAM_BANK_A, 0, 0));

    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        for (int v = 0; v < 256; v++) {
            bool seen[PORT_VRAM_PAGE_COUNT] = { false };
            memset(c, 0, sizeof(c));
            c[b] = (uint8_t)v;
            port_vram_layout_compute(c, &l);
            CHECK(l.overlaps == 0);
            for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
                for (int slot = 0; slot < PORT_VRAM_MAX_SLOTS; slot++) {
                    PortVramSlot s = l.slots[view][slot];
                    if (s.bank == PORT_VRAM_BANK_NONE) {
                        continue;
                    }
                    int p = port_vram_bank_first_page[s.bank] + s.page;
                    CHECK(slot < port_vram_view_slot_count(view));
                    CHECK(s.bank == b);
                    CHECK(s.mirror == seen[p]);
                    seen[p] = true;
                }
            }
        }
    }
}

// ---- Each backend, on the real window ----

static void set_cnt(int bank, uint8_t value) {
    uint8_t c[PORT_VRAM_BANK_COUNT];
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        c[b] = port_vram_vramcnt(b);
    }
    c[bank] = value;
    port_vram_set_all_vramcnt(c);
}

static void set_all(uint8_t value) {
    uint8_t c[PORT_VRAM_BANK_COUNT];
    memset(c, value, sizeof(c));
    port_vram_set_all_vramcnt(c);
}

static uint8_t *lcdc_bank(PortVramBank b) {
    return port_vram_view(PORT_VRAM_VIEW_LCDC) + ((size_t)port_vram_bank_first_page[b] << PORT_VRAM_SLOT_SHIFT);
}

// Every bank through LCDC, each with its own pattern (seed bank + 1)
static void fill_banks(void) {
    set_all(LCDC);
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        fill_pattern(lcdc_bank(b), port_vram_bank_size[b], b + 1);
    }
}

// The pattern across pages that need not be contiguous: a bank's pages, or the slots of a view the renderer reads
static bool pages_have_pattern(const uint8_t *const *pages, int count, uint32_t seed) {
    static uint8_t copy[PORT_VRAM_BANKS_SIZE];
    for (int i = 0; i < count; i++) {
        if (pages[i] == NULL) {
            return false;
        }
        memcpy(copy + ((size_t)i << PORT_VRAM_SLOT_SHIFT), pages[i], PORT_VRAM_SLOT_SIZE);
    }
    return has_pattern(copy, (size_t)count << PORT_VRAM_SLOT_SHIFT, seed);
}

static bool bank_has_pattern(PortVramBank b, uint32_t seed) {
    const uint8_t *pages[8];
    int count = (int)(port_vram_bank_size[b] >> PORT_VRAM_SLOT_SHIFT);
    for (int p = 0; p < count; p++) {
        pages[p] = port_vram_bank_page(b, p);
    }
    return pages_have_pattern(pages, count, seed);
}

static bool slots_have_pattern(PortVramView view, int first, int count, uint32_t seed) {
    const uint8_t *pages[PORT_VRAM_MAX_SLOTS];
    for (int i = 0; i < count; i++) {
        pages[i] = port_vram_slot_ptr(view, first + i);
    }
    return pages_have_pattern(pages, count, seed);
}

// The CPU views are where the SDK's constants put them, in the window PORT_HW_ADDR gives region 0x06
static void test_views(void) {
    sContext = "views";
    CHECK(port_vram_view(PORT_VRAM_VIEW_BG_A) == (uint8_t *)HW_BG_VRAM);
    CHECK(port_vram_view(PORT_VRAM_VIEW_BG_B) == (uint8_t *)HW_DB_BG_VRAM);
    CHECK(port_vram_view(PORT_VRAM_VIEW_OBJ_A) == (uint8_t *)HW_OBJ_VRAM);
    CHECK(port_vram_view(PORT_VRAM_VIEW_OBJ_B) == (uint8_t *)HW_DB_OBJ_VRAM);
    CHECK(port_vram_view(PORT_VRAM_VIEW_LCDC) == (uint8_t *)HW_LCDC_VRAM);
    CHECK(lcdc_bank(PORT_VRAM_BANK_E) == (uint8_t *)HW_LCDC_VRAM_E);
    CHECK(lcdc_bank(PORT_VRAM_BANK_I) == (uint8_t *)HW_LCDC_VRAM_I);
    CHECK(port_vram_view(PORT_VRAM_VIEW_LCDC) == port_hw_pointer(0x06800000));
    // Each view starts a slot of region 0x06's window, as the aliasing backends map them, and a page of the host's: the
    // window is aligned to PORT_HOST_PAGE_ALIGNMENT (port/hal/host/host.h), 16 KiB on Apple Silicon and Linux, 4 KiB
    // on Windows, which copies the banks, and in an x86_64 Mach-O program (port/hal/hw/hw.c). The page the system
    // reports at run time must divide it too, or the aliasing backends would skip and the game copy the banks with no
    // test failing; a page larger than a slot (64 KiB on some arm64 Linux kernels) can't alias whatever the window's
    // alignment, so there the banks are copied by design (port/hal/gx/vram_os.c)
    uintptr_t window = (uintptr_t)port_hw_pointer(0x06000000);
    size_t page = port_host_page_size();
    for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
        CHECK(((uintptr_t)port_vram_view(view) - window) % PORT_VRAM_SLOT_SIZE == 0);
        CHECK((uintptr_t)port_vram_view(view) % PORT_HOST_PAGE_ALIGNMENT == 0);
        CHECK(page > PORT_VRAM_SLOT_SIZE || (uintptr_t)port_vram_view(view) % page == 0);
    }
}

// A write through BG-A is in the bank when it moves to LCDC, and the other way round
static void test_move(void) {
    uint8_t *bg = port_vram_view(PORT_VRAM_VIEW_BG_A);

    sContext = "move";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_A, cnt(1, 0));
    CHECK(has_pattern(bg, 0x20000, PORT_VRAM_BANK_A + 1));
    CHECK(all_bytes(lcdc_bank(PORT_VRAM_BANK_A), 0x20000, 0));
    fill_pattern(bg, 0x20000, 100);
    set_cnt(PORT_VRAM_BANK_A, LCDC);
    CHECK(has_pattern(lcdc_bank(PORT_VRAM_BANK_A), 0x20000, 100));
    CHECK(all_bytes(bg, 0x20000, 0));

    // B at BG-A's second 128 KiB (OFS 1)
    set_cnt(PORT_VRAM_BANK_B, cnt(1, 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, PORT_VRAM_BANK_B + 1));
    CHECK(all_bytes(bg, 0x20000, 0));

    // A and B trade places: each goes where the other was (the copy backend parks one of them)
    fill_banks();
    set_cnt(PORT_VRAM_BANK_A, cnt(1, 0));
    set_cnt(PORT_VRAM_BANK_B, cnt(1, 1));
    {
        uint8_t swap[PORT_VRAM_BANK_COUNT];
        for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
            swap[b] = port_vram_vramcnt(b);
        }
        swap[PORT_VRAM_BANK_A] = cnt(1, 1);
        swap[PORT_VRAM_BANK_B] = cnt(1, 0);
        port_vram_set_all_vramcnt(swap);
    }
    CHECK(has_pattern(bg, 0x20000, PORT_VRAM_BANK_B + 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, PORT_VRAM_BANK_A + 1));
}

// Slots with no bank read 0 and absorb writes. codein.c and error_window_screen.c set GX_VRAM_BG_128_A and then clear
// the whole 512 KiB BG-A view: the 384 KiB with no bank must take the stores, and the banks elsewhere must come through
static void test_unmapped(void) {
    uint8_t *bg = port_vram_view(PORT_VRAM_VIEW_BG_A);
    uint8_t *lcdc = port_vram_view(PORT_VRAM_VIEW_LCDC);

    sContext = "unmapped";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_A, cnt(1, 0));
    memset(bg, 0xcd, port_vram_view_size[PORT_VRAM_VIEW_BG_A]);
    for (int b = PORT_VRAM_BANK_B; b < PORT_VRAM_BANK_COUNT; b++) {
        CHECK(bank_has_pattern(b, b + 1));
    }
    set_cnt(PORT_VRAM_BANK_A, LCDC);
    CHECK(all_bytes(lcdc_bank(PORT_VRAM_BANK_A), 0x20000, 0xcd));
    // The slots A left read 0 again; the ones no bank ever took keep the stray bytes until their mapping changes
    CHECK(all_bytes(bg, 0x20000, 0));
    CHECK(all_bytes(bg + 0x20000, 0x60000, 0xcd));
    set_cnt(PORT_VRAM_BANK_B, cnt(1, 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, PORT_VRAM_BANK_B + 1));

    // A is in BG-A and C in texture slot 2 while all of LCDC is cleared
    fill_banks();
    set_cnt(PORT_VRAM_BANK_A, cnt(1, 0));
    set_cnt(PORT_VRAM_BANK_C, cnt(3, 2));
    memset(lcdc, 0, port_vram_view_size[PORT_VRAM_VIEW_LCDC]);
    CHECK(has_pattern(bg, 0x20000, PORT_VRAM_BANK_A + 1));
    CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_TEX, 16), PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_C + 1));
    CHECK(bank_has_pattern(PORT_VRAM_BANK_C, PORT_VRAM_BANK_C + 1));
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_B, 0), PORT_VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_I, 0), PORT_VRAM_SLOT_SIZE, 0));
}

// A bank keeps its bytes while it is disabled, given to the ARM7 or in a slot only the GPU reads
static void test_keep(void) {
    sContext = "keep";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_C, 0);
    CHECK(all_bytes(lcdc_bank(PORT_VRAM_BANK_C), 0x20000, 0));
    set_cnt(PORT_VRAM_BANK_C, cnt(2, 0));
    CHECK(port_vram_slot_ptr(PORT_VRAM_VIEW_TEX, 16) == NULL);
    set_cnt(PORT_VRAM_BANK_C, cnt(3, 2));
    CHECK(slots_have_pattern(PORT_VRAM_VIEW_TEX, 16, 8, PORT_VRAM_BANK_C + 1));
    set_cnt(PORT_VRAM_BANK_C, LCDC);
    CHECK(has_pattern(lcdc_bank(PORT_VRAM_BANK_C), 0x20000, PORT_VRAM_BANK_C + 1));

    // E's upper half isn't in the extended palette slots but survives them
    set_cnt(PORT_VRAM_BANK_E, cnt(4, 0));
    CHECK(port_vram_slot_ptr(PORT_VRAM_VIEW_BGEXT_A, 1) != NULL);
    set_cnt(PORT_VRAM_BANK_E, LCDC);
    CHECK(has_pattern(lcdc_bank(PORT_VRAM_BANK_E), 0x10000, PORT_VRAM_BANK_E + 1));
}

// F at OFS 0 is at 0x06000000 and 0x06008000; I as sub OBJ fills the 128 KiB view eight times. Right after the remap a
// load through a mirror sees the bank in every backend; after a store, only aliasing keeps the two places one
static void test_mirrors(void) {
    uint8_t *bg = port_vram_view(PORT_VRAM_VIEW_BG_A);
    bool alias = port_vram_backend() != PORT_VRAM_BACKEND_COPY;

    sContext = "mirrors";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_F, cnt(1, 0));
    CHECK(has_pattern(bg, PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_F + 1));
    CHECK(has_pattern(bg + 2 * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_F + 1));
    fill_pattern(bg, PORT_VRAM_SLOT_SIZE, 300);
    CHECK(port_vram_slot_ptr(PORT_VRAM_VIEW_BG_A, 1) == NULL);
    CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_BG_A, 2), PORT_VRAM_SLOT_SIZE, 300));
    if (alias) {
        // Aliasing makes the mirror live for the CPU too
        CHECK(has_pattern(bg + 2 * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, 300));
        fill_pattern(bg + 2 * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, 301);
        CHECK(has_pattern(bg, PORT_VRAM_SLOT_SIZE, 301));
    } else {
        // Copying: the mirror keeps the copy it took, and a store through it stays there
        CHECK(has_pattern(bg + 2 * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_F + 1));
        fill_pattern(bg + 2 * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, 301);
        CHECK(has_pattern(bg, PORT_VRAM_SLOT_SIZE, 300));
        CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_BG_A, 2), PORT_VRAM_SLOT_SIZE, 300));
    }
    set_cnt(PORT_VRAM_BANK_I, cnt(2, 0));
    for (int s = 0; s < 8; s++) {
        uint8_t *objB = port_vram_view(PORT_VRAM_VIEW_OBJ_B) + ((size_t)s << PORT_VRAM_SLOT_SHIFT);
        CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_OBJ_B, s), PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_I + 1));
        CHECK(has_pattern(objB, PORT_VRAM_SLOT_SIZE, PORT_VRAM_BANK_I + 1));
    }
}

// Stores through mirrors (codein.c's clears of the sub views with H and I mapped): a uniform fill of a whole view
// leaves the same bank bytes in every backend; a store through a mirror alone reaches the bank only when aliasing
static void test_mirror_stores(void) {
    uint8_t *bgB = port_vram_view(PORT_VRAM_VIEW_BG_B);
    uint8_t *objB = port_vram_view(PORT_VRAM_VIEW_OBJ_B);
    bool alias = port_vram_backend() != PORT_VRAM_BACKEND_COPY;

    sContext = "mirror stores";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_H, cnt(1, 0));
    set_cnt(PORT_VRAM_BANK_I, cnt(2, 0));
    memset(bgB, 0, port_vram_view_size[PORT_VRAM_VIEW_BG_B]);
    memset(objB, 0, port_vram_view_size[PORT_VRAM_VIEW_OBJ_B]);
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_H, 0), PORT_VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_H, 1), PORT_VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_I, 0), PORT_VRAM_SLOT_SIZE, 0));
    memset(objB, 0x5a, port_vram_view_size[PORT_VRAM_VIEW_OBJ_B]);
    CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_I, 0), PORT_VRAM_SLOT_SIZE, 0x5a));

    // OBJ-B's second slot alone, as a character load past I's 16 KiB would store
    fill_pattern(objB + PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, 600);
    if (alias) {
        CHECK(has_pattern(port_vram_bank_page(PORT_VRAM_BANK_I, 0), PORT_VRAM_SLOT_SIZE, 600));
        CHECK(has_pattern(objB, PORT_VRAM_SLOT_SIZE, 600));
    } else {
        CHECK(all_bytes(port_vram_bank_page(PORT_VRAM_BANK_I, 0), PORT_VRAM_SLOT_SIZE, 0x5a));
        CHECK(all_bytes(port_vram_slot_ptr(PORT_VRAM_VIEW_OBJ_B, 1), PORT_VRAM_SLOT_SIZE, 0x5a));
        CHECK(has_pattern(objB + PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE, 600));
    }
    set_cnt(PORT_VRAM_BANK_I, LCDC);
    CHECK(has_pattern(lcdc_bank(PORT_VRAM_BANK_I), PORT_VRAM_SLOT_SIZE, 600) == alias);
    CHECK(all_bytes(objB, port_vram_view_size[PORT_VRAM_VIEW_OBJ_B], 0));
}

// The texture banks go to LCDC for an upload and come back, which a texture cache sees in the page and slot
// generations
static void test_generations(void) {
    uint32_t pageBefore[16];
    uint32_t slotBefore[16];

    sContext = "generations";
    fill_banks();
    set_cnt(PORT_VRAM_BANK_A, cnt(3, 0));
    set_cnt(PORT_VRAM_BANK_B, cnt(3, 1));
    for (int s = 0; s < 16; s++) {
        pageBefore[s] = port_vram_page_generation(s / 8, s % 8);
        slotBefore[s] = port_vram_tex_generation(s);
    }
    set_cnt(PORT_VRAM_BANK_A, LCDC);
    set_cnt(PORT_VRAM_BANK_B, LCDC);
    fill_pattern(lcdc_bank(PORT_VRAM_BANK_A), 0x40000, 500);
    set_cnt(PORT_VRAM_BANK_A, cnt(3, 0));
    set_cnt(PORT_VRAM_BANK_B, cnt(3, 1));
    for (int s = 0; s < 16; s++) {
        CHECK(port_vram_page_generation(s / 8, s % 8) != pageBefore[s]);
        CHECK(port_vram_tex_generation(s) != slotBefore[s]);
        CHECK(port_vram_tex_page(s) != NULL);
    }
    CHECK(port_vram_tex_page(16) == NULL);
    CHECK(slots_have_pattern(PORT_VRAM_VIEW_TEX, 0, 16, 500));

    // A write the HAL is told about: the page and its slot change generation, and the page after it doesn't
    set_cnt(PORT_VRAM_BANK_E, LCDC);
    pageBefore[0] = port_vram_page_generation(PORT_VRAM_BANK_E, 0);
    pageBefore[1] = port_vram_page_generation(PORT_VRAM_BANK_E, 1);
    slotBefore[0] = port_vram_slot_generation(PORT_VRAM_VIEW_LCDC, 32);
    port_vram_note_write(0x06880000, 0x10);
    CHECK(port_vram_page_generation(PORT_VRAM_BANK_E, 0) != pageBefore[0]);
    CHECK(port_vram_slot_generation(PORT_VRAM_VIEW_LCDC, 32) != slotBefore[0]);
    CHECK(port_vram_page_generation(PORT_VRAM_BANK_E, 1) == pageBefore[1]);
}

// The reference: bank bytes, plus the own bytes of each slot whose CPU accesses don't reach a bank, set when the slot's
// mapping changes: the slots with no bank, which go back to 0, and with the copy backend the mirror slots, which take a
// copy of their page
typedef struct Model {
    bool copy;
    uint8_t vramcnt[PORT_VRAM_BANK_COUNT];
    PortVramLayout layout;
    uint8_t banks[PORT_VRAM_BANKS_SIZE];
    uint8_t *loose[PORT_VRAM_VIEW_COUNT];
} Model;

static bool model_own_bytes(const Model *m, PortVramSlot s) {
    return s.bank == PORT_VRAM_BANK_NONE || (m->copy && s.mirror);
}

static uint8_t *model_bank(Model *m, PortVramSlot s, size_t offset) {
    return m->banks + ((size_t)(port_vram_bank_first_page[s.bank] + s.page) << PORT_VRAM_SLOT_SHIFT) +
           (offset & (PORT_VRAM_SLOT_SIZE - 1));
}

static void model_set(Model *m, int bank, uint8_t value) {
    PortVramLayout next;

    m->vramcnt[bank] = value;
    port_vram_layout_compute(m->vramcnt, &next);
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < port_vram_view_slot_count(view); slot++) {
            PortVramSlot a = m->layout.slots[view][slot];
            PortVramSlot b = next.slots[view][slot];
            bool changed = a.bank != b.bank || a.page != b.page || a.mirror != b.mirror;
            uint8_t *own = m->loose[view] + ((size_t)slot << PORT_VRAM_SLOT_SHIFT);
            if (changed && b.bank == PORT_VRAM_BANK_NONE) {
                memset(own, 0, PORT_VRAM_SLOT_SIZE);
            } else if (changed && model_own_bytes(m, b)) {
                memcpy(own, model_bank(m, b, 0), PORT_VRAM_SLOT_SIZE);
            }
        }
    }
    m->layout = next;
}

static uint8_t *model_byte(Model *m, int view, size_t offset) {
    PortVramSlot s = m->layout.slots[view][offset >> PORT_VRAM_SLOT_SHIFT];
    if (model_own_bytes(m, s)) {
        return m->loose[view] + offset;
    }
    return model_bank(m, s, offset);
}

static void model_check(Model *m) {
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < port_vram_view_slot_count(view); slot++) {
            size_t offset = (size_t)slot << PORT_VRAM_SLOT_SHIFT;
            PortVramSlot s = m->layout.slots[view][slot];
            CHECK(memcmp(&s, &port_vram_layout()->slots[view][slot], sizeof(s)) == 0);
            if (s.bank == PORT_VRAM_BANK_NONE) {
                CHECK(port_vram_slot_ptr(view, slot) == NULL);
            } else {
                CHECK(memcmp(port_vram_slot_ptr(view, slot), model_bank(m, s, offset), PORT_VRAM_SLOT_SIZE) == 0);
            }
            // The copy backend keeps the pages of the GPU-only views outside any view
            if (!m->copy || view < PORT_VRAM_CPU_VIEW_COUNT) {
                CHECK(memcmp(port_vram_view(view) + offset, model_byte(m, view, offset), PORT_VRAM_SLOT_SIZE) == 0);
            }
        }
    }
    for (int p = 0; p < PORT_VRAM_PAGE_COUNT; p++) {
        int bank = 0;
        while (bank + 1 < PORT_VRAM_BANK_COUNT && port_vram_bank_first_page[bank + 1] <= p) {
            bank++;
        }
        CHECK(memcmp(port_vram_bank_page(bank, p - port_vram_bank_first_page[bank]),
                     m->banks + ((size_t)p << PORT_VRAM_SLOT_SHIFT), PORT_VRAM_SLOT_SIZE) == 0);
    }
}

// Random VRAMCNT writes and random stores through the CPU views at their DS addresses, mirrors included, a quarter of
// them zeros, with the backend compared to the model after every remap
static void test_random(void) {
    Model *m = calloc(1, sizeof(*m));
    uint32_t state = 12345;
    int failuresBefore = sFailures;
    int mirrorStores = 0;

    sContext = "random";
    m->copy = port_vram_backend() == PORT_VRAM_BACKEND_COPY;
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        m->loose[view] = calloc(1, port_vram_view_size[view]);
    }
    set_all(LCDC);
    memset(port_vram_view(PORT_VRAM_VIEW_LCDC), 0, port_vram_view_size[PORT_VRAM_VIEW_LCDC]);
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        model_set(m, b, LCDC);
    }
    // Slots no bank took since the earlier tests still hold what those wrote
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        if (port_vram_view(view) != NULL) {
            memcpy(m->loose[view], port_vram_view(view), port_vram_view_size[view]);
        }
    }
    model_check(m);

    for (int step = 0; step < 3000 && sFailures == failuresBefore; step++) {
        uint32_t r = next_word(&state);
        if (r % 5 == 0) {
            int bank = (int)(next_word(&state) % PORT_VRAM_BANK_COUNT);
            uint32_t pick = next_word(&state) % 10;
            uint8_t value = pick < 5 ? LCDC : pick == 5 ? 0 : (uint8_t)(next_word(&state) | 0x80);
            set_cnt(bank, value);
            model_set(m, bank, value);
            model_check(m);
        } else {
            int view = (int)(next_word(&state) % PORT_VRAM_CPU_VIEW_COUNT);
            size_t slot = next_word(&state) % (uint32_t)port_vram_view_slot_count(view);
            size_t start = (next_word(&state) % PORT_VRAM_SLOT_SIZE) & ~(size_t)3;
            size_t len = 4 + (next_word(&state) % 2048);
            uint8_t value = next_word(&state) % 4 == 0 ? 0 : (uint8_t)next_word(&state);
            uint32_t address =
                port_vram_view_ds_address[view] + (uint32_t)(slot << PORT_VRAM_SLOT_SHIFT) + (uint32_t)start;
            if (start + len > PORT_VRAM_SLOT_SIZE) {
                len = PORT_VRAM_SLOT_SIZE - start;
            }
            mirrorStores += m->layout.slots[view][slot].mirror;
            // As game code stores: through the DS address
            memset((uint8_t *)PORT_HW_ADDR(address), value, len);
            memset(model_byte(m, view, (slot << PORT_VRAM_SLOT_SHIFT) + start), value, len);
        }
    }
    model_check(m);
    CHECK(mirrorStores > 20);
    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        free(m->loose[view]);
    }
    free(m);
}

static int find_backend(const char *name) {
    for (int b = 0; b < PORT_VRAM_BACKEND_COUNT; b++) {
        if (strcmp(name, port_vram_backend_name[b]) == 0) {
            return b;
        }
    }
    return -1;
}

static int run_backend(const char *name) {
    int backend = find_backend(name);
    int err;

    if (backend < 0) {
        fprintf(stderr, "no backend named %s\n", name);
        return 2;
    }
    if (!port_vram_backend_available(backend)) {
        printf("%s: not available on this host, skipped\n", name);
        return 77;
    }
    err = port_vram_init(backend);
    if (err != 0) {
        fprintf(stderr, "%s: port_vram_init failed: %s\n", name, strerror(err));
        return 1;
    }
    test_views();
    test_move();
    test_unmapped();
    test_keep();
    test_mirrors();
    test_mirror_stores();
    test_generations();
    test_random();
    printf("%s: %u remaps, %u mapping calls, %llu bytes copied, %llu bytes zeroed\n", name, port_vram_stats()->remaps,
           port_vram_stats()->os_calls, (unsigned long long)port_vram_stats()->bytes_copied,
           (unsigned long long)port_vram_stats()->bytes_zeroed);
    port_vram_shutdown();
    // Shut down, the views are the window's plain zeroed memory
    sContext = "shutdown";
    CHECK(all_bytes((uint8_t *)HW_BG_VRAM, 0x80000, 0));
    CHECK(all_bytes((uint8_t *)HW_LCDC_VRAM, PORT_VRAM_BANKS_SIZE, 0));
    return 0;
}

// ---- The SDK through the wrappers ----

// Every page's generation and that of its LCDC slot, to see which pages the HAL's own loads reached
typedef struct Generations {
    uint32_t page[PORT_VRAM_PAGE_COUNT];
    uint32_t lcdc[PORT_VRAM_PAGE_COUNT];
} Generations;

static void take_generations(Generations *g) {
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        for (int p = 0; p < (int)(port_vram_bank_size[b] >> PORT_VRAM_SLOT_SHIFT); p++) {
            int i = port_vram_bank_first_page[b] + p;
            g->page[i] = port_vram_page_generation(b, p);
            g->lcdc[i] = port_vram_slot_generation(PORT_VRAM_VIEW_LCDC, i);
        }
    }
}

// The pages of a GXVRam mask of banks, as bits of their LCDC slots
static uint64_t bank_pages(uint32_t banks) {
    uint64_t pages = 0;
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        if (banks & (1u << b)) {
            uint64_t count = port_vram_bank_size[b] >> PORT_VRAM_SLOT_SHIFT;
            pages |= ((1ull << count) - 1) << port_vram_bank_first_page[b];
        }
    }
    return pages;
}

// Between a Begin and its End, where no remap happens: the loads since before was taken changed the generations of
// the pages given (bits of their LCDC slots) and of no other page. A texture cache learns of a load into a page it
// holds only this way
static void check_loaded(const Generations *before, uint64_t pages) {
    Generations now;

    take_generations(&now);
    for (int i = 0; i < PORT_VRAM_PAGE_COUNT; i++) {
        bool loaded = (pages >> i) & 1;
        CHECK((now.page[i] != before->page[i]) == loaded);
        CHECK((now.lcdc[i] != before->lcdc[i]) == loaded);
    }
}

// After an End, when no bank has been in LCDC or a CPU view since GX_InitGXState but the loaded ones, which left LCDC
// at the End and had their slots cleared: every byte of those views is still 0, so no load reached past its banks
static void check_views_clear(void) {
    for (int view = 0; view < PORT_VRAM_CPU_VIEW_COUNT; view++) {
        CHECK(all_bytes(port_vram_view(view), port_vram_view_size[view], 0));
    }
}

// Every bank's bytes to 0, and every bank nowhere, as GX_InitGXState leaves them
static void clear_banks(void) {
    GX_SetBankForLCDC(GX_VRAM_LCDC_ALL);
    memset((uint8_t *)HW_LCDC_VRAM, 0, PORT_VRAM_BANKS_SIZE);
    GX_InitGXState();
}

static void sdk_set(SdkUse use, uint32_t banks) {
    switch (use) {
    case SDK_USE_LCDC:
        GX_SetBankForLCDC(banks);
        break;
    case SDK_USE_BG:
        GX_SetBankForBG((GXVRamBG)banks);
        break;
    case SDK_USE_OBJ:
        GX_SetBankForOBJ((GXVRamOBJ)banks);
        break;
    case SDK_USE_BGEXTPLTT:
        GX_SetBankForBGExtPltt((GXVRamBGExtPltt)banks);
        break;
    case SDK_USE_OBJEXTPLTT:
        GX_SetBankForOBJExtPltt((GXVRamOBJExtPltt)banks);
        break;
    case SDK_USE_TEX:
        GX_SetBankForTex((GXVRamTex)banks);
        break;
    case SDK_USE_TEXPLTT:
        GX_SetBankForTexPltt((GXVRamTexPltt)banks);
        break;
    case SDK_USE_CLEARIMAGE:
        GX_SetBankForClearImage((GXVRamClearImage)banks);
        break;
    case SDK_USE_ARM7:
        GX_SetBankForARM7((GXVRamARM7)banks);
        break;
    case SDK_USE_SUBBG:
        GX_SetBankForSubBG((GXVRamSubBG)banks);
        break;
    case SDK_USE_SUBOBJ:
        GX_SetBankForSubOBJ((GXVRamSubOBJ)banks);
        break;
    case SDK_USE_SUBBGEXTPLTT:
        GX_SetBankForSubBGExtPltt((GXVRamSubBGExtPltt)banks);
        break;
    case SDK_USE_SUBOBJEXTPLTT:
        GX_SetBankForSubOBJExtPltt((GXVRamSubOBJExtPltt)banks);
        break;
    }
}

static uint8_t vramcnt_register(int bank) {
    static const uint32_t address[PORT_VRAM_BANK_COUNT] = {
        0x04000240, 0x04000241, 0x04000242, 0x04000243, 0x04000244, 0x04000245, 0x04000246, 0x04000248, 0x04000249,
    };
    return *(const uint8_t *)port_hw_pointer(address[bank]);
}

// Every SDK setting through the real gx_vramcnt.c and the wrappers: the registers hold the row's bytes, and the views
// show its places, from GX_InitGXState's all-zero start
static void scenario_sdk_settings(void) {
    int rows = 0;

    for (size_t i = 0; i < NELEMS(SDK_SETTINGS); i++) {
        const SdkSetting *set = &SDK_SETTINGS[i];
        uint8_t expected[PORT_VRAM_BANK_COUNT] = { 0 };

        sContext = set->name;
        GX_InitGXState();
        CHECK(count_placed(port_vram_layout()) == 0);
        sdk_set(set->use, set->banks);
        for (int k = 0; k < 4 && set->writes[k].bank != 0; k++) {
            expected[set->writes[k].bank - 'A'] = set->writes[k].value;
        }
        for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
            CHECK(vramcnt_register(b) == expected[b]);
            CHECK(port_vram_vramcnt(b) == expected[b]);
        }
        CHECK(port_vram_layout()->overlaps == 0);
        check_places(port_vram_layout(), set);
        rows++;
    }
    CHECK(rows == 85 + PORT_VRAM_BANK_COUNT);
}

// What the wrappers do for the game: each SDK call that moves banks moves the bytes with them before it returns
static void scenario_wrappers(void) {
    uint8_t *bg = (uint8_t *)HW_BG_VRAM;
    uint8_t *lcdcA = (uint8_t *)HW_LCDC_VRAM_A;
    uint32_t stats;

    sContext = "init";
    GX_InitGXState();
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        CHECK(port_vram_vramcnt(b) == 0);
    }
    CHECK(count_placed(port_vram_layout()) == 0);

    // Every bank to LCDC, a pattern in each
    sContext = "lcdc";
    GX_SetBankForLCDC(GX_VRAM_LCDC_ALL);
    CHECK(GX_GetBankForLCDC() == GX_VRAM_LCDC_ALL);
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        fill_pattern(lcdc_bank(b), port_vram_bank_size[b], 10 + b);
    }

    // A to BG: the BG pointers of the SDK see A's bytes, and LCDC no longer does
    sContext = "bg";
    GX_SetBankForBG(GX_VRAM_BG_128_A);
    CHECK(GX_GetBankForLCDC() == (GX_VRAM_LCDC_ALL & ~GX_VRAM_A));
    CHECK(has_pattern(bg, 0x20000, 10));
    CHECK(G2_GetBG0CharPtr() == bg);
    CHECK(all_bytes(lcdcA, 0x20000, 0));
    fill_pattern(bg, 0x20000, 20);

    // E's 64 KiB as BG, then A again: the SDK writes A's VRAMCNT while E still holds BG-A's first 64 KiB, but the
    // wrapper applies after the call, when E is back in LCDC
    sContext = "bg switch";
    stats = port_vram_stats()->overlaps;
    GX_SetBankForBG(GX_VRAM_BG_64_E);
    CHECK(has_pattern(bg, 0x10000, 14));
    CHECK(has_pattern(lcdcA, 0x20000, 20));
    GX_SetBankForBG(GX_VRAM_BG_128_A);
    CHECK(has_pattern(bg, 0x20000, 20));
    CHECK(has_pattern((uint8_t *)HW_LCDC_VRAM_E, 0x10000, 14));
    CHECK(port_vram_stats()->overlaps == stats);
    CHECK(port_vram_layout()->overlaps == 0);

    // Reset takes the banks back to LCDC and says which
    sContext = "reset";
    CHECK(GX_ResetBankForBG() == GX_VRAM_A);
    CHECK(has_pattern(lcdcA, 0x20000, 20));
    CHECK(all_bytes(bg, 0x20000, 0));

    // Disable turns them off: they are nowhere, and keep their bytes
    sContext = "disable";
    GX_DisableBankForLCDC();
    CHECK(port_vram_vramcnt(PORT_VRAM_BANK_A) == 0);
    CHECK(all_bytes(lcdcA, 0x20000, 0));
    CHECK(port_vram_slot_ptr(PORT_VRAM_VIEW_LCDC, 0) == NULL);
    GX_SetBankForLCDC(GX_VRAM_A);
    CHECK(has_pattern(lcdcA, 0x20000, 20));

    // The sub engine: H as sub BG is at 0x06200000 and again at 0x06210000; I as sub OBJ fills its 128 KiB
    sContext = "sub";
    GX_SetBankForLCDC(GX_VRAM_H | GX_VRAM_I);
    fill_pattern((uint8_t *)HW_LCDC_VRAM_H, 0x8000, 30);
    fill_pattern((uint8_t *)HW_LCDC_VRAM_I, 0x4000, 31);
    GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H);
    GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I);
    CHECK(GX_GetBankForSubOBJ() == GX_VRAM_I);
    CHECK(has_pattern((uint8_t *)HW_DB_BG_VRAM, 0x8000, 30));
    CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_BG_B, 4), 0x4000, 30));
    CHECK(has_pattern((uint8_t *)HW_DB_OBJ_VRAM, 0x4000, 31));
    CHECK(has_pattern(port_vram_slot_ptr(PORT_VRAM_VIEW_OBJ_B, 7), 0x4000, 31));
    CHECK(has_pattern((uint8_t *)HW_DB_BG_VRAM + 0x10000, 0x8000, 30));
    GX_DisableBankForSubBG();
    GX_DisableBankForSubOBJ();
    CHECK(all_bytes((uint8_t *)HW_DB_BG_VRAM, 0x20000, 0));
    CHECK(all_bytes((uint8_t *)HW_DB_OBJ_VRAM, 0x20000, 0));

    // Textures: the use's banks leave LCDC, and Reset brings them back
    sContext = "tex";
    GX_SetBankForTex(GX_VRAM_TEX_01_BC);
    CHECK(GX_GetBankForTex() == (GX_VRAM_B | GX_VRAM_C));
    CHECK(has_pattern(port_vram_tex_page(0), PORT_VRAM_SLOT_SIZE, 11));
    CHECK(has_pattern(port_vram_tex_page(8), PORT_VRAM_SLOT_SIZE, 12));
    CHECK(all_bytes((uint8_t *)HW_LCDC_VRAM_B, 0x40000, 0));
    CHECK(GX_ResetBankForTex() == (GX_VRAM_B | GX_VRAM_C));
    CHECK(has_pattern((uint8_t *)HW_LCDC_VRAM_B, 0x20000, 11));
    CHECK(port_vram_tex_page(0) == NULL);

    // The ARM7's banks are nowhere the ARM9 sees
    sContext = "arm7";
    GX_SetBankForARM7(GX_VRAM_ARM7_128_D);
    CHECK(all_bytes((uint8_t *)HW_LCDC_VRAM_D, 0x20000, 0));
    GX_DisableBankForARM7();
    GX_SetBankForLCDC(GX_VRAM_D);
    CHECK(has_pattern((uint8_t *)HW_LCDC_VRAM_D, 0x20000, 13));

    // WRAMCNT is the SDK's register write and nothing else
    sContext = "wram";
    {
        PortVramLayout before = *port_vram_layout();
        MI_SetWramBank(MI_WRAM_ARM7_ALL);
        CHECK(*(const uint8_t *)port_hw_pointer(0x04000247) == MI_WRAM_ARM7_ALL);
        CHECK(memcmp(&before, port_vram_layout(), sizeof(before)) == 0);
    }

    // GX_InitGXState puts every bank nowhere again
    sContext = "init again";
    GX_InitGXState();
    CHECK(count_placed(port_vram_layout()) == 0);
    CHECK(all_bytes((uint8_t *)HW_LCDC_VRAM, PORT_VRAM_BANKS_SIZE, 0));
    CHECK(port_vram_stats()->overlaps == 0);
}

// The texture uploads of GX_BeginLoadTex/GX_LoadTex/GX_EndLoadTex, for every setting of the texture banks: what a
// load puts at an offset of the texture space is at that offset of the texture slots once the banks are back, across
// banks that aren't next to each other in LCDC too, nothing else was written, and the loads themselves changed the
// generations of the pages they wrote
static void scenario_tex_upload(void) {
    static const GXVRamTex settings[] = {
        GX_VRAM_TEX_0_A,     GX_VRAM_TEX_0_B,     GX_VRAM_TEX_0_C,     GX_VRAM_TEX_0_D,     GX_VRAM_TEX_01_AB,
        GX_VRAM_TEX_01_AC,   GX_VRAM_TEX_01_AD,   GX_VRAM_TEX_01_BC,   GX_VRAM_TEX_01_BD,   GX_VRAM_TEX_01_CD,
        GX_VRAM_TEX_012_ABC, GX_VRAM_TEX_012_ABD, GX_VRAM_TEX_012_ACD, GX_VRAM_TEX_012_BCD, GX_VRAM_TEX_0123_ABCD,
    };
    static uint8_t image[0x80000];

    fill_pattern(image, sizeof(image), 700);
    for (size_t i = 0; i < NELEMS(settings); i++) {
        int banks = __builtin_popcount(settings[i]);
        uint32_t space = (uint32_t)banks * 0x20000;
        uint32_t before[PORT_VRAM_TEX_PAGES];
        Generations loads;
        static char context[64];

        snprintf(context, sizeof(context), "texture banks 0x%x", settings[i]);
        sContext = context;
        GX_InitGXState();
        GX_SetBankForTex(settings[i]);
        for (int p = 0; p < PORT_VRAM_TEX_PAGES; p++) {
            before[p] = port_vram_tex_generation(p);
            CHECK((port_vram_tex_page(p) != NULL) == ((uint32_t)p * PORT_VRAM_SLOT_SIZE < space));
        }

        GX_BeginLoadTex();
        CHECK(GX_GetBankForTex() == 0);
        CHECK(port_vram_tex_page(0) == NULL);
        take_generations(&loads);
        // The whole space in three loads: up to 0x100 before the first bank's end, a load across that end, the rest
        GX_LoadTex(image, 0, 0x20000 - 0x100);
        GX_LoadTex(image + 0x20000 - 0x100, 0x20000 - 0x100, banks > 1 ? 0x200 : 0x100);
        if (banks > 1) {
            GX_LoadTex(image + 0x20100, 0x20100, space - 0x20100);
        }
        check_loaded(&loads, bank_pages(settings[i]));
        GX_EndLoadTex();
        check_views_clear();

        CHECK(GX_GetBankForTex() == (u32)settings[i]);
        for (int p = 0; p < PORT_VRAM_TEX_PAGES; p++) {
            const uint8_t *page = port_vram_tex_page(p);
            if ((uint32_t)p * PORT_VRAM_SLOT_SIZE < space) {
                CHECK(page != NULL && memcmp(page, image + (size_t)p * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE) == 0);
                CHECK(port_vram_tex_generation(p) != before[p]);
            } else {
                CHECK(page == NULL);
            }
        }
        CHECK(count_placed(port_vram_layout()) == banks * 8);
    }
}

// The texture palette uploads, for every setting of the palette banks
static void scenario_texpltt_upload(void) {
    static const GXVRamTexPltt settings[] = {
        GX_VRAM_TEXPLTT_0_F,    GX_VRAM_TEXPLTT_0_G,      GX_VRAM_TEXPLTT_01_FG,
        GX_VRAM_TEXPLTT_0123_E, GX_VRAM_TEXPLTT_01234_EF, GX_VRAM_TEXPLTT_012345_EFG,
    };
    static const int pages[] = { 1, 1, 2, 4, 5, 6 };
    static uint8_t palettes[0x18000];

    fill_pattern(palettes, sizeof(palettes), 800);
    for (size_t i = 0; i < NELEMS(settings); i++) {
        uint32_t size = (uint32_t)pages[i] * PORT_VRAM_SLOT_SIZE;
        uint32_t before[PORT_VRAM_TEXPLTT_PAGES];
        Generations loads;
        static char context[64];

        snprintf(context, sizeof(context), "texture palette banks 0x%x", settings[i]);
        sContext = context;
        GX_InitGXState();
        GX_SetBankForTexPltt(settings[i]);
        for (int p = 0; p < PORT_VRAM_TEXPLTT_PAGES; p++) {
            before[p] = port_vram_texpltt_generation(p);
        }
        GX_BeginLoadTexPltt();
        take_generations(&loads);
        // A model fade's small load, then the rest
        GX_LoadTexPltt(palettes, 0, 0x20);
        GX_LoadTexPltt(palettes + 0x20, 0x20, size - 0x20);
        check_loaded(&loads, bank_pages(settings[i]));
        GX_EndLoadTexPltt();
        check_views_clear();
        for (int p = 0; p < PORT_VRAM_TEXPLTT_PAGES; p++) {
            const uint8_t *page = port_vram_texpltt_page(p);
            if (p < pages[i]) {
                CHECK(page != NULL &&
                      memcmp(page, palettes + (size_t)p * PORT_VRAM_SLOT_SIZE, PORT_VRAM_SLOT_SIZE) == 0);
                CHECK(port_vram_texpltt_generation(p) != before[p]);
            } else {
                CHECK(page == NULL);
            }
        }
    }
}

// The rear plane image: the color image goes to texture slot 2 and the depth to slot 3, where the banks are
static void scenario_clear_image(void) {
    static const struct {
        GXVRamClearImage banks;
        bool color;
    } settings[] = {
        { GX_VRAM_CLEARIMAGE_256_AB, true }, { GX_VRAM_CLEARIMAGE_256_CD, true }, { GX_VRAM_CLEARDEPTH_128_A, false },
        { GX_VRAM_CLEARDEPTH_128_B, false }, { GX_VRAM_CLEARDEPTH_128_C, false }, { GX_VRAM_CLEARDEPTH_128_D, false },
    };
    static uint8_t color[0x20000];
    static uint8_t depth[0x20000];

    fill_pattern(color, sizeof(color), 900);
    fill_pattern(depth, sizeof(depth), 901);
    for (size_t i = 0; i < NELEMS(settings); i++) {
        Generations loads;
        static char context[64];

        snprintf(context, sizeof(context), "clear image banks 0x%x", settings[i].banks);
        sContext = context;
        clear_banks();
        GX_SetBankForClearImage(settings[i].banks);
        GX_BeginLoadClearImage();
        take_generations(&loads);
        GX_LoadClearImageColor(color, sizeof(color));
        GX_LoadClearImageDepth(depth, sizeof(depth));
        // Every page of the banks was written; a single bank's color image went where no bank is, which no page sees
        check_loaded(&loads, bank_pages(settings[i].banks));
        GX_EndLoadClearImage();
        CHECK(slots_have_pattern(PORT_VRAM_VIEW_TEX, 24, 8, 901));
        if (settings[i].color) {
            CHECK(slots_have_pattern(PORT_VRAM_VIEW_TEX, 16, 8, 900));
        } else {
            CHECK(port_vram_tex_page(16) == NULL);
        }
        CHECK(count_placed(port_vram_layout()) == __builtin_popcount(settings[i].banks) * 8);
        // A single bank takes its color image at the bank before it, which no bank holds now: the stores were
        // absorbed, and nothing reached a bank
        GX_SetBankForLCDC(GX_VRAM_LCDC_ALL & ~settings[i].banks);
        if (!settings[i].color) {
            for (int b = PORT_VRAM_BANK_A; b <= PORT_VRAM_BANK_D; b++) {
                if (!(settings[i].banks & (1 << b))) {
                    CHECK(all_bytes(lcdc_bank(b), 0x20000, 0));
                }
            }
        }
    }
}

// The main engine's extended palettes (the HAL's GX_*LoadBGExtPltt and GX_*LoadOBJExtPltt) and the sub engine's (the
// decomp's GXS_ ones are left to the DMA, which the host doesn't have yet; only their banks are checked), as the 2D
// renderer's source shows them
static void scenario_ext_palettes(void) {
    static const struct {
        GXVRamBGExtPltt banks;
        int first; // the first 8 KiB extended palette slot the banks hold
        int count;
    } bgSettings[] = {
        { GX_VRAM_BGEXTPLTT_0123_E, 0, 4 },
        { GX_VRAM_BGEXTPLTT_01_F, 0, 2 },
        { GX_VRAM_BGEXTPLTT_23_G, 2, 2 },
        { GX_VRAM_BGEXTPLTT_0123_FG, 0, 4 },
    };
    static const GXVRamOBJExtPltt objSettings[] = { GX_VRAM_OBJEXTPLTT_0_F, GX_VRAM_OBJEXTPLTT_0_G };
    static uint8_t palettes[0x8000];
    PortDs2dSource src;

    fill_pattern(palettes, sizeof(palettes), 1000);
    for (size_t i = 0; i < NELEMS(bgSettings); i++) {
        uint32_t offset = (uint32_t)bgSettings[i].first * 0x2000;
        uint32_t size = (uint32_t)bgSettings[i].count * 0x2000;
        uint32_t before = port_vram_slot_generation(PORT_VRAM_VIEW_BGEXT_A, bgSettings[i].first / 2);
        int firstPage = port_vram_bank_first_page[__builtin_ctz(bgSettings[i].banks)];
        Generations loads;
        static char context[64];

        snprintf(context, sizeof(context), "BG extended palette banks 0x%x", bgSettings[i].banks);
        sContext = context;
        GX_InitGXState();
        GX_SetBankForBGExtPltt(bgSettings[i].banks);
        GX_BeginLoadBGExtPltt();
        take_generations(&loads);
        GX_LoadBGExtPltt(palettes + offset, offset, size);
        // The load fills its banks from the first one's start, two 8 KiB palette slots to a page: E's first two pages
        check_loaded(&loads, ((1ull << (size / PORT_VRAM_SLOT_SIZE)) - 1) << firstPage);
        GX_EndLoadBGExtPltt();
        check_views_clear();
        memset(&src, 0, sizeof(src));
        port_vram_fill_source(&src, 0);
        for (int s = 0; s < 4; s++) {
            bool held = s >= bgSettings[i].first && s < bgSettings[i].first + bgSettings[i].count;
            CHECK((src.bg_ext_palette[s] != NULL) == held);
            if (held) {
                CHECK(memcmp(src.bg_ext_palette[s], palettes + (size_t)s * 0x2000, 0x2000) == 0);
            }
        }
        CHECK(port_vram_slot_generation(PORT_VRAM_VIEW_BGEXT_A, bgSettings[i].first / 2) != before);
        CHECK((reg_GX_DISPCNT & REG_GX_DISPCNT_BG_MASK) != 0);
    }
    for (size_t i = 0; i < NELEMS(objSettings); i++) {
        Generations loads;
        static char context[64];

        snprintf(context, sizeof(context), "OBJ extended palette banks 0x%x", objSettings[i]);
        sContext = context;
        GX_InitGXState();
        GX_SetBankForOBJExtPltt(objSettings[i]);
        GX_BeginLoadOBJExtPltt();
        take_generations(&loads);
        GX_LoadOBJExtPltt(palettes, 0, 0x2000);
        check_loaded(&loads, bank_pages(objSettings[i]));
        GX_EndLoadOBJExtPltt();
        check_views_clear();
        memset(&src, 0, sizeof(src));
        port_vram_fill_source(&src, 0);
        CHECK(src.obj_ext_palette != NULL && memcmp(src.obj_ext_palette, palettes, 0x2000) == 0);
    }

    sContext = "sub extended palettes";
    fill_pattern(palettes, sizeof(palettes), 1100);
    GX_InitGXState();
    GX_SetBankForLCDC(GX_VRAM_H | GX_VRAM_I);
    memcpy((uint8_t *)HW_LCDC_VRAM_H, palettes, 0x8000);
    fill_pattern((uint8_t *)HW_LCDC_VRAM_I, 0x2000, 1101);
    GX_SetBankForSubBGExtPltt(GX_VRAM_SUB_BGEXTPLTT_0123_H);
    GX_SetBankForSubOBJExtPltt(GX_VRAM_SUB_OBJEXTPLTT_0_I);
    memset(&src, 0, sizeof(src));
    port_vram_fill_source(&src, 1);
    for (int s = 0; s < 4; s++) {
        CHECK(src.bg_ext_palette[s] != NULL &&
              memcmp(src.bg_ext_palette[s], palettes + (size_t)s * 0x2000, 0x2000) == 0);
    }
    CHECK(has_pattern(src.obj_ext_palette, 0x2000, 1101));
}

// The renderer's source for both engines from one setting of every use
static void scenario_fill_source(void) {
    PortDs2dSource src;

    sContext = "fill source";
    GX_InitGXState();
    GX_SetBankForLCDC(GX_VRAM_LCDC_ALL);
    for (int b = 0; b < PORT_VRAM_BANK_COUNT; b++) {
        fill_pattern(lcdc_bank(b), port_vram_bank_size[b], 1200 + b);
    }
    GX_SetBankForBG(GX_VRAM_BG_256_AB);
    GX_SetBankForOBJ(GX_VRAM_OBJ_64_E);
    GX_SetBankForBGExtPltt(GX_VRAM_BGEXTPLTT_0123_FG);
    GX_SetBankForSubBG(GX_VRAM_SUB_BG_32_H);
    GX_SetBankForSubOBJ(GX_VRAM_SUB_OBJ_16_I);

    memset(&src, 0, sizeof(src));
    port_vram_fill_source(&src, 0);
    CHECK(src.io == port_hw_pointer(0x04000000));
    CHECK(src.palette == port_hw_pointer(0x05000000));
    CHECK(src.oam == port_hw_pointer(0x07000000));
    CHECK(src.bg0_3d_line == NULL);
    for (int i = 0; i < 32; i++) {
        if (i < 16) {
            CHECK(src.bg_slots[i] == (uint8_t *)HW_BG_VRAM + (size_t)i * PORT_VRAM_SLOT_SIZE);
        } else {
            CHECK(src.bg_slots[i] == NULL);
        }
    }
    CHECK(pages_have_pattern(src.bg_slots, 8, 1200));
    CHECK(pages_have_pattern(src.bg_slots + 8, 8, 1201));
    for (int i = 0; i < 16; i++) {
        CHECK((src.obj_slots[i] != NULL) == (i < 4));
    }
    CHECK(pages_have_pattern(src.obj_slots, 4, 1204));
    CHECK(has_pattern(src.bg_ext_palette[0], 0x2000, 1205));
    CHECK(src.bg_ext_palette[1] == src.bg_ext_palette[0] + 0x2000);
    CHECK(has_pattern(src.bg_ext_palette[2], 0x2000, 1206));
    CHECK(src.obj_ext_palette == NULL);
    // LCDC holds C and D only
    for (int i = 0; i < PORT_VRAM_PAGE_COUNT; i++) {
        CHECK((src.lcdc_slots[i] != NULL) == (i >= 16 && i < 32));
    }
    CHECK(pages_have_pattern(src.lcdc_slots + 16, 8, 1202));

    memset(&src, 0, sizeof(src));
    port_vram_fill_source(&src, 1);
    CHECK(src.io == port_hw_pointer(0x04001000));
    CHECK(src.palette == port_hw_pointer(0x05000400));
    CHECK(src.oam == port_hw_pointer(0x07000400));
    // H at slots 0-1 and their mirror 4-5; I is OBJ, eight times
    for (int i = 0; i < 32; i++) {
        CHECK((src.bg_slots[i] != NULL) == (i == 0 || i == 1 || i == 4 || i == 5));
    }
    CHECK(pages_have_pattern(src.bg_slots, 2, 1207));
    CHECK(pages_have_pattern(src.bg_slots + 4, 2, 1207));
    for (int i = 0; i < 16; i++) {
        CHECK((src.obj_slots[i] != NULL) == (i < 8));
        if (i < 8) {
            CHECK(has_pattern(src.obj_slots[i], PORT_VRAM_SLOT_SIZE, 1208));
        }
    }
    for (int i = 0; i < PORT_VRAM_PAGE_COUNT; i++) {
        CHECK(src.lcdc_slots[i] == NULL);
    }
    for (int i = 0; i < 4; i++) {
        CHECK(src.bg_ext_palette[i] == NULL);
    }
}

// The geometry FIFO copies of the matrix commands and G3X_ClearFifo, as the command log shows them
static void scenario_fifo(void) {
    MtxFx43 m43;
    MtxFx44 m44;
    const PortG3Write *log;
    size_t count;
    u32 plain[8];
    u32 words[16];

    sContext = "fifo";
    for (int i = 0; i < 16; i++) {
        words[i] = 0x1000u * (u32)(i + 1) + 7;
    }
    memcpy(&m43, words, sizeof(m43));
    memcpy(&m44, words, sizeof(m44));
    port_hw_g3_take(&count);

    G3_LoadMtx43(&m43);
    log = port_hw_g3_take(&count);
    CHECK(count == 13);
    if (count == 13) {
        CHECK(log[0].port == 0x04000400 && log[0].value == G3OP_MTX_LOAD_4x3);
        for (int i = 0; i < 12; i++) {
            CHECK(log[1 + i].port == 0x04000400 && log[1 + i].value == words[i]);
        }
    }

    G3_MultMtx44(&m44);
    log = port_hw_g3_take(&count);
    CHECK(count == 17);
    if (count == 17) {
        CHECK(log[0].value == G3OP_MTX_MULT_4x4);
        for (int i = 0; i < 16; i++) {
            CHECK(log[1 + i].port == 0x04000400 && log[1 + i].value == words[i]);
        }
    }

    G3X_ClearFifo();
    log = port_hw_g3_take(&count);
    CHECK(count == 128);
    for (size_t i = 0; i < count; i++) {
        CHECK(log[i].port == 0x04000400 && log[i].value == 0);
    }

    // To memory instead of the port: each STM starts again at dest, so the last one's words stay
    memset(plain, 0xff, sizeof(plain));
    GX_SendFifo48B(words, plain);
    CHECK(plain[0] == words[9] && plain[1] == words[10] && plain[2] == words[11] && plain[3] == 0xffffffff);
    GX_SendFifo64B(words, plain);
    for (int i = 0; i < 8; i++) {
        CHECK(plain[i] == words[8 + i]);
    }
    GXi_NopClearFifo128_(plain);
    CHECK(plain[0] == 0 && plain[3] == 0 && plain[4] == words[12]);
    log = port_hw_g3_take(&count);
    CHECK(count == 0);
}

// A read of a geometry command port. The ports are write-only, and the DS reads 0 from one and sends nothing, but the
// register macro's access can't tell a read from a write: SPL reads POLYGON_ATTR back after setting each particle's
// (lib/spl/src/spl_draw.c, "(void)reg_G3_POLYGON_ATTR;"), and when the read was logged as a POLYGON_ATTR of 0, which
// draws neither face, every particle of the game vanished (the trainers' cut-in sparks, the Poké Balls' bursts, the
// moves' effects). The log keeps one POLYGON_ATTR with the value written, merges only accesses in a row to a port that
// stores its parameter, and SPL's own billboard draw leaves its quad in the geometry engine
static void scenario_parameter_read(void) {
    static const MtxFx43 identity = { { { FX32_ONE, 0, 0 }, { 0, FX32_ONE, 0 }, { 0, 0, FX32_ONE }, { 0, 0, 0 } } };
    const u32 attr = GX_PACK_POLYGONATTR_PARAM(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 5, 31, 0);
    const PortG3Write *log;
    size_t count;
    SPLResourceHeader header;
    SPLResource resource;
    SPLEmitter emitter;
    SPLManager manager;
    SPLParticle particle;
    PortGe *ge;

    sContext = "parameter read";
    port_hw_g3_take(&count);

    // The write and the read, as spl_draw.c has them
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 5, 31, 0);
    (void)reg_G3_POLYGON_ATTR;
    G3_Begin(GX_BEGIN_QUADS);
    log = port_hw_g3_take(&count);
    CHECK(count == 2);
    if (count == 2) {
        CHECK(log[0].port == 0x040004a4 && log[0].value == attr);
        CHECK(log[1].port == 0x04000500 && log[1].value == GX_BEGIN_QUADS);
    }

    // Two writes in a row to a port that stores its parameter: one entry, the second's value
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 1, 10, 0);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 5, 31, 0);
    reg_G3_TEXIMAGE_PARAM = 0x1234;
    reg_G3_TEXIMAGE_PARAM = 0x5678;
    reg_G3_TEXPLTT_BASE = 0x11;
    reg_G3_TEXPLTT_BASE = 0x22;
    log = port_hw_g3_take(&count);
    CHECK(count == 3);
    if (count == 3) {
        CHECK(log[0].port == 0x040004a4 && log[0].value == attr);
        CHECK(log[1].port == 0x040004a8 && log[1].value == 0x5678);
        CHECK(log[2].port == 0x040004ac && log[2].value == 0x22);
    }

    // With another command between, each its own; a command that acts at each write is never merged
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 1, 10, 0);
    G3_Begin(GX_BEGIN_QUADS);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 5, 31, 0);
    G3_Identity();
    G3_Identity();
    log = port_hw_g3_take(&count);
    CHECK(count == 5);
    if (count == 5) {
        CHECK(log[0].port == 0x040004a4 && log[2].port == 0x040004a4 && log[2].value == attr);
        CHECK(log[3].port == 0x04000454 && log[4].port == 0x04000454);
    }

    // SPL's billboard of one opaque particle at the origin, in front of an identity camera, through the engine
    memset(&header, 0, sizeof(header));
    memset(&resource, 0, sizeof(resource));
    memset(&emitter, 0, sizeof(emitter));
    memset(&manager, 0, sizeof(manager));
    memset(&particle, 0, sizeof(particle));
    header.aspectRatio = FX16_ONE;
    header.scaleAnimDir = SPL_SCALE_ANIM_DIR_XY;
    resource.header = &header;
    emitter.resource = &resource;
    emitter.color = GX_RGB(31, 31, 31);
    emitter.textureS = FX16_ONE;
    emitter.textureT = FX16_ONE;
    manager.drawEmitter = &emitter;
    manager.viewMatrix = &identity;
    particle.baseAlpha = 31;
    particle.animAlpha = 31;
    particle.polygonId = 5;
    particle.baseScale = FX32_ONE / 4;
    particle.animScale = FX16_ONE;
    particle.color = GX_RGB(31, 31, 31);
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    G3_ViewPort(0, 0, 255, 191);
    SPLDraw_Billboard(&manager, &particle);
    log = port_hw_g3_take(&count);
    ge = port_ge_create();
    CHECK(ge != NULL);
    if (ge != NULL) {
        const PortGeList *list;

        port_ge_run(ge, log, count);
        list = port_ge_building_list(ge);
        CHECK(list->polygon_count == 1);
        if (list->polygon_count == 1) {
            CHECK(list->polygons[0].attr == attr);
        }
        port_ge_destroy(ge);
    }
}

// POWCNT1 as the SDK sets it: GX_Init's power and display swap, then the other screen order and the sub engine off
static void scenario_display(void) {
    PortGxDisplay display;

    sContext = "display";
    reg_GX_POWCNT = 0;
    port_gx_display(&display);
    CHECK(!display.lcdOn && display.topEngine == 1 && !display.enginePowered[0] && !display.enginePowered[1]);
    reg_GX_POWCNT |= REG_GX_POWCNT_DSEL_MASK;
    GX_SetPower(GX_POWER_ALL);
    reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
    port_gx_display(&display);
    CHECK(display.lcdOn && display.topEngine == 0 && display.enginePowered[0] && display.enginePowered[1]);
    CHECK(GX_GetDispSelect() == GX_DISP_SELECT_MAIN_SUB);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GX_SetPower(GX_POWER_ALL & ~0x200);
    port_gx_display(&display);
    CHECK(display.lcdOn && display.topEngine == 1 && display.enginePowered[0] && !display.enginePowered[1]);
}

// The DS's mirrors of the CPU views, as the HAL's own copies fold them. Each view has an address that a fold of half
// its width would send elsewhere (0x061ffffc, 0x06230000, 0x06460000 and 0x065e0000, 0x067f0000)
static void scenario_cpu_pointer(void) {
    sContext = "cpu pointer";
    CHECK(port_vram_cpu_pointer(0x06000000) == (void *)HW_BG_VRAM);
    CHECK(port_vram_cpu_pointer(0x06080010) == (uint8_t *)HW_BG_VRAM + 0x10);
    CHECK(port_vram_cpu_pointer(0x061ffffc) == (uint8_t *)HW_BG_VRAM + 0x7fffc);
    CHECK(port_vram_cpu_pointer(0x06220000) == (void *)HW_DB_BG_VRAM);
    CHECK(port_vram_cpu_pointer(0x06230000) == (uint8_t *)HW_DB_BG_VRAM + 0x10000);
    CHECK(port_vram_cpu_pointer(0x06440004) == (uint8_t *)HW_OBJ_VRAM + 4);
    CHECK(port_vram_cpu_pointer(0x06460000) == (uint8_t *)HW_OBJ_VRAM + 0x20000);
    CHECK(port_vram_cpu_pointer(0x065e0000) == (uint8_t *)HW_OBJ_VRAM + 0x20000);
    CHECK(port_vram_cpu_pointer(0x067e0000) == (void *)HW_DB_OBJ_VRAM);
    CHECK(port_vram_cpu_pointer(0x067f0000) == (uint8_t *)HW_DB_OBJ_VRAM + 0x10000);
    CHECK(port_vram_cpu_pointer(0x068a0000) == (void *)HW_LCDC_VRAM_I);
    CHECK(port_vram_cpu_pointer(0x068a4000) == port_hw_pointer(0x068a4000));
    CHECK(port_vram_cpu_pointer(0x02000000) == port_hw_pointer(0x02000000));
}

// The building transition's second event (src/ov036/field_transition.c, func_ov036_021b8e10) gives bank D to LCDC,
// clears 128 KiB from HW_LCDC_VRAM, turns LCDC off and gives D to BG. The decomp's literal there, 0x6800000, is no
// host address (game-changes.md); HW_LCDC_VRAM is the same DS address, bank A's place in LCDC, not D's. So the clear
// reaches A when A is in LCDC, and nothing otherwise, and leaves D as it was, on the DS and through the host's views
static void BuildingTransitionClear(void) {
    GX_SetBankForLCDC(8);
    MIi_CpuClearFast(0, (void *)HW_LCDC_VRAM, 0x20000);
    GX_DisableBankForLCDC();
    GX_SetBankForBG(GX_VRAM_BG_128_D);
}

static void scenario_lcdc_clear(void) {
    uint8_t *lcdcA = (uint8_t *)HW_LCDC_VRAM_A;

    // A in a texture slot, D in BG, each with its pattern
    sContext = "A elsewhere";
    GX_InitGXState();
    GX_SetBankForLCDC(GX_VRAM_A | GX_VRAM_D);
    fill_pattern(lcdcA, 0x20000, 40);
    fill_pattern((uint8_t *)HW_LCDC_VRAM_D, 0x20000, 41);
    GX_SetBankForTex(GX_VRAM_TEX_0_A);
    GX_SetBankForBG(GX_VRAM_BG_128_D);
    BuildingTransitionClear();
    CHECK(has_pattern((uint8_t *)HW_BG_VRAM, 0x20000, 41));
    CHECK(GX_ResetBankForTex() == GX_VRAM_A);
    CHECK(has_pattern(lcdcA, 0x20000, 40));

    // A in LCDC: the clear is A's
    sContext = "A in LCDC";
    CHECK(GX_GetBankForLCDC() == GX_VRAM_A);
    BuildingTransitionClear();
    CHECK(has_pattern((uint8_t *)HW_BG_VRAM, 0x20000, 41));
    GX_SetBankForLCDC(GX_VRAM_A);
    CHECK(all_bytes(lcdcA, 0x20000, 0));
}

// With nothing set up, the first SDK call sets the views up with the default backend: shm where it runs
static void scenario_default_backend(void) {
    sContext = "default backend";
    port_vram_shutdown();
    GX_InitGXState();
    if (port_vram_backend_available(PORT_VRAM_BACKEND_SHM)) {
        CHECK(port_vram_backend() == PORT_VRAM_BACKEND_SHM);
    } else {
        CHECK(port_vram_backend() == PORT_VRAM_BACKEND_COPY);
    }
    GX_SetBankForBG(GX_VRAM_BG_128_C);
    *(volatile u32 *)HW_BG_VRAM = 0x12345678;
    GX_SetBankForBG(GX_VRAM_BG_NONE);
    CHECK(*(const u32 *)HW_LCDC_VRAM_C == 0x12345678);
}

typedef struct Scenario {
    const char *name;
    void (*run)(void);
} Scenario;

static const Scenario sScenarios[] = {
    { "sdk_settings", scenario_sdk_settings },
    { "wrappers", scenario_wrappers },
    { "tex_upload", scenario_tex_upload },
    { "texpltt_upload", scenario_texpltt_upload },
    { "clear_image", scenario_clear_image },
    { "ext_palettes", scenario_ext_palettes },
    { "fill_source", scenario_fill_source },
    { "fifo", scenario_fifo },
    { "cpu_pointer", scenario_cpu_pointer },
    { "default_backend", scenario_default_backend },
    { "display", scenario_display },
    { "lcdc_clear", scenario_lcdc_clear },
    { "parameter_read", scenario_parameter_read },
};

static const Scenario *sRunning;

// The scenario runs as the game's main() on the OS core, as the SDK's calls expect
static void ScenarioMain(void) {
    sRunning->run();
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc >= 2 && strcmp(argv[1], "layout") == 0) {
        test_layout();
    } else if (argc >= 3 && strcmp(argv[1], "backend") == 0) {
        int status = run_backend(argv[2]);
        if (status != 0) {
            return status;
        }
    } else if (argc >= 2) {
        for (size_t i = 0; i < NELEMS(sScenarios); i++) {
            if (strcmp(argv[1], sScenarios[i].name) == 0) {
                sRunning = &sScenarios[i];
            }
        }
        if (sRunning == NULL) {
            fprintf(stderr, "no scenario named %s\n", argv[1]);
            return 2;
        }
        if (argc >= 3) {
            int backend = find_backend(argv[2]);
            if (backend < 0) {
                fprintf(stderr, "no backend named %s\n", argv[2]);
                return 2;
            }
            if (!port_vram_backend_available(backend)) {
                printf("%s: not available on this host, skipped\n", argv[2]);
                return 77;
            }
            if (port_vram_init(backend) != 0) {
                fprintf(stderr, "%s: port_vram_init failed\n", argv[2]);
                return 1;
            }
        }
        if (port_os_run(ScenarioMain) != PORT_OS_RETURNED) {
            fprintf(stderr, "%s: the run didn't end normally\n", argv[1]);
            return 1;
        }
        printf("%s (%s backend)\n", argv[1], port_vram_backend_name[port_vram_backend()]);
    } else {
        fprintf(stderr, "usage: port_gx_tests layout | backend <name> | <scenario> [backend]\n");
        return 2;
    }
    if (sFailures != 0) {
        fprintf(stderr, "%d checks failed\n", sFailures);
        return 1;
    }
    printf("ok\n");
    return 0;
}
