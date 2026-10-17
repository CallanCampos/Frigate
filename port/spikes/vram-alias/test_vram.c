// Tests of the VRAM views: the layout against every SDK setting written out from GBATEK, then each backend against
// what the DS does when a bank moves (and against a reference model, over random VRAMCNT writes and stores). Usage:
//   test_vram layout
//   test_vram <shm|mach-entry|mach-remap|copy> [static | exit-live]
// "static" gives the views a static array instead of reserved memory: 16 KiB-aligned for the aliasing backends, and
// only 8-byte aligned for the copy backend, as a Windows static array would be. "exit-live" exits with a static
// region's views still mapped, for LeakSanitizer to scan on Linux. A backend that can't run on this host is reported
// and skipped
#include "sdk_settings.h"
#include "vram.h"
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int gFailures;
static const char *gContext = "";

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            fprintf(stderr, "%s:%d: [%s] check failed: %s\n", __FILE__, __LINE__, gContext, #cond);                    \
            gFailures++;                                                                                               \
        }                                                                                                              \
    } while (0)

// The views of the static-region runs: a link-time address, as HW_BG_VRAM would have with a static array
static _Alignas(VRAM_STATIC_REGION_ALIGN) uint8_t gStaticRegion[VRAM_REGION_SIZE];
// The copy backend's static region: 8 bytes past a 16-byte boundary, so never page-aligned
static _Alignas(16) uint8_t gOddRegion[VRAM_REGION_SIZE + 16];

static void *static_region(VramBackend backend) {
    return backend == VRAM_BACKEND_COPY ? gOddRegion + 8 : gStaticRegion;
}

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

static bool slot_is(const VramLayout *l, VramView view, int slot, int bank, int page, int mirror) {
    VramSlot s = l->slots[view][slot];
    return s.bank == bank && s.page == page && s.mirror == mirror;
}

static int count_placed(const VramLayout *l) {
    int n = 0;
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < VRAM_MAX_SLOTS; slot++) {
            n += l->slots[view][slot].bank != VRAM_BANK_NONE;
        }
    }
    return n;
}

// Each SDK setting alone, from every register at 0: its banks are where GBATEK puts them, mirrors included, and
// nowhere else
static void test_sdk_settings(void) {
    int rows = 0;

    for (size_t i = 0; i < sizeof(SDK_SETTINGS) / sizeof(SDK_SETTINGS[0]); i++) {
        const SdkSetting *set = &SDK_SETTINGS[i];
        uint8_t c[VRAM_BANK_COUNT] = { 0 };
        VramLayout l;
        int expected = 0;

        gContext = set->name;
        for (int w = 0; w < 4 && set->writes[w].bank != 0; w++) {
            c[set->writes[w].bank - 'A'] = set->writes[w].value;
        }
        vram_layout_compute(c, &l);
        CHECK(l.overlaps == 0);
        for (int k = 0; k < 8 && set->places[k].bank != 0; k++) {
            const SdkPlace *p = &set->places[k];
            int bank = p->bank - 'A';
            int pages = p->pages != 0 ? p->pages : (int)(gVramBankSize[bank] >> VRAM_SLOT_SHIFT);
            int first = (int)(p->offset >> VRAM_SLOT_SHIFT);

            CHECK(p->offset % VRAM_SLOT_SIZE == 0);
            CHECK(first + pages <= vram_view_slot_count(p->view));
            for (int page = 0; page < pages && first + page < VRAM_MAX_SLOTS; page++) {
                CHECK(slot_is(&l, p->view, first + page, bank, page, p->mirror));
            }
            expected += pages;
        }
        CHECK(count_placed(&l) == expected);
        rows++;
    }
    CHECK(rows == 85 + VRAM_BANK_COUNT);
}

// The SDK's settings come from sdk_settings.h. Here: every register at 0 and in LCDC, a collision, and invariants over
// every value of every register (including the ones the SDK never writes, which only these invariants cover)
static void test_layout(void) {
    uint8_t c[VRAM_BANK_COUNT] = { 0 };
    VramLayout l;

    test_sdk_settings();

    gContext = "layout";
    vram_layout_compute(c, &l);
    CHECK(count_placed(&l) == 0);

    memset(c, LCDC, sizeof(c));
    vram_layout_compute(c, &l);
    CHECK(count_placed(&l) == VRAM_PAGE_COUNT);
    CHECK(l.overlaps == 0);

    // Sub extended palettes and C given to the ARM7 (nowhere on the host) at once
    memset(c, 0, sizeof(c));
    c[VRAM_BANK_H] = cnt(2, 0);
    c[VRAM_BANK_I] = cnt(3, 0);
    c[VRAM_BANK_C] = cnt(2, 0);
    vram_layout_compute(c, &l);
    CHECK(slot_is(&l, VRAM_VIEW_BGEXT_B, 0, VRAM_BANK_H, 0, 0));
    CHECK(slot_is(&l, VRAM_VIEW_BGEXT_B, 1, VRAM_BANK_H, 1, 0));
    CHECK(slot_is(&l, VRAM_VIEW_OBJEXT_B, 0, VRAM_BANK_I, 0, 0));
    CHECK(count_placed(&l) == 3);

    // Two banks on one slot: the lower bank keeps it, and the layout counts the collision
    memset(c, 0, sizeof(c));
    c[VRAM_BANK_A] = cnt(1, 0);
    c[VRAM_BANK_E] = cnt(1, 0);
    vram_layout_compute(c, &l);
    CHECK(l.overlaps == 4);
    CHECK(slot_is(&l, VRAM_VIEW_BG_A, 0, VRAM_BANK_A, 0, 0));

    // Every value of every register: slots stay inside their view, a page is placed once plus mirrors, and a bank
    // alone never collides with itself. These are invariants only: where a value the SDK never writes lands is not
    // checked against GBATEK
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        for (int v = 0; v < 256; v++) {
            bool seen[VRAM_PAGE_COUNT] = { false };
            memset(c, 0, sizeof(c));
            c[b] = (uint8_t)v;
            vram_layout_compute(c, &l);
            CHECK(l.overlaps == 0);
            for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
                for (int slot = 0; slot < VRAM_MAX_SLOTS; slot++) {
                    VramSlot s = l.slots[view][slot];
                    if (s.bank == VRAM_BANK_NONE) {
                        continue;
                    }
                    int p = gVramBankFirstPage[s.bank] + s.page;
                    CHECK(slot < vram_view_slot_count(view));
                    CHECK(s.bank == b);
                    CHECK(s.mirror == seen[p]);
                    seen[p] = true;
                }
            }
        }
    }
}

// Catching the fault of a slot with no access
static sigjmp_buf gFaultJump;

static void on_fault(int sig) {
    siglongjmp(gFaultJump, sig);
}

static bool store_faults(volatile uint8_t *p) {
    struct sigaction sa, oldSegv, oldBus;
    bool faulted;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_fault;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &oldSegv);
    sigaction(SIGBUS, &sa, &oldBus);
    if (sigsetjmp(gFaultJump, 1) == 0) {
        *p = 1;
        faulted = false;
    } else {
        faulted = true;
    }
    sigaction(SIGSEGV, &oldSegv, NULL);
    sigaction(SIGBUS, &oldBus, NULL);
    return faulted;
}

static void set_all(Vram *v, uint8_t value) {
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        vram_set_vramcnt(v, b, value);
    }
}

static uint8_t *lcdc_bank(Vram *v, VramBank b) {
    return vram_view(v, VRAM_VIEW_LCDC) + ((size_t)gVramBankFirstPage[b] << VRAM_SLOT_SHIFT);
}

// Every bank through LCDC, each with its own pattern (seed bank + 1)
static void fill_banks(Vram *v) {
    set_all(v, LCDC);
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        fill_pattern(lcdc_bank(v, b), gVramBankSize[b], b + 1);
    }
}

// The pattern across pages that need not be contiguous: a bank's pages, or the slots of a view the renderer reads
static bool pages_have_pattern(uint8_t *const *pages, int count, uint32_t seed) {
    static uint8_t copy[VRAM_BANKS_SIZE];
    for (int i = 0; i < count; i++) {
        if (pages[i] == NULL) {
            return false;
        }
        memcpy(copy + ((size_t)i << VRAM_SLOT_SHIFT), pages[i], VRAM_SLOT_SIZE);
    }
    return has_pattern(copy, (size_t)count << VRAM_SLOT_SHIFT, seed);
}

static bool bank_has_pattern(Vram *v, VramBank b, uint32_t seed) {
    uint8_t *pages[8];
    int count = (int)(gVramBankSize[b] >> VRAM_SLOT_SHIFT);
    for (int p = 0; p < count; p++) {
        pages[p] = vram_bank_page(v, b, p);
    }
    return pages_have_pattern(pages, count, seed);
}

static bool slots_have_pattern(Vram *v, VramView view, int first, int count, uint32_t seed) {
    uint8_t *pages[VRAM_MAX_SLOTS];
    for (int i = 0; i < count; i++) {
        pages[i] = vram_slot_ptr(v, view, first + i);
    }
    return pages_have_pattern(pages, count, seed);
}

static void test_alignment(Vram *v) {
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    size_t align = vram_region_alignment(vram_backend(v));

    gContext = "alignment";
    CHECK(vram_region_size() == VRAM_REGION_SIZE);
    CHECK(vram_backend(v) == VRAM_BACKEND_COPY ? align == 8 : align == page);
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        CHECK((uintptr_t)vram_view(v, view) % align == 0);
        CHECK(gVramViewSize[view] % VRAM_SLOT_SIZE == 0);
    }
}

// A caller's region that isn't page-aligned (as a static array on Windows, where COFF allows at most 8 KiB): the
// aliasing backends refuse it, the copy backend takes it (run_backend runs the copy suite on one)
static void test_region_rules(VramBackend backend) {
    Vram *v;

    gContext = "region rules";
    if (backend == VRAM_BACKEND_COPY) {
        return;
    }
    errno = 0;
    v = vram_create(backend, 0, gOddRegion + 8);
    CHECK(v == NULL);
    CHECK(errno == EINVAL);
    vram_destroy(v);
}

// The guard slot after a view: no access in a reserved region, an accessible zero page in a caller's region, which is
// usually in .bss, where LeakSanitizer reads every word at exit
static void test_guards(Vram *v, bool staticRegion) {
    uint8_t *guard = vram_view(v, VRAM_VIEW_BG_A) + gVramViewSize[VRAM_VIEW_BG_A];

    gContext = "guards";
    if (vram_backend(v) == VRAM_BACKEND_COPY) {
        return;
    }
    CHECK(store_faults(guard) == !staticRegion);
}

// A write through BG-A is in the bank when it moves to LCDC, and the other way round
static void test_move(Vram *v) {
    uint8_t *bg = vram_view(v, VRAM_VIEW_BG_A);

    gContext = "move";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    CHECK(has_pattern(bg, 0x20000, VRAM_BANK_A + 1));
    CHECK(all_bytes(lcdc_bank(v, VRAM_BANK_A), 0x20000, 0));
    fill_pattern(bg, 0x20000, 100);
    vram_set_vramcnt(v, VRAM_BANK_A, LCDC);
    CHECK(has_pattern(lcdc_bank(v, VRAM_BANK_A), 0x20000, 100));
    CHECK(all_bytes(bg, 0x20000, 0));

    // B at BG-A's second 128 KiB (OFS 1)
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(1, 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, VRAM_BANK_B + 1));
    CHECK(all_bytes(bg, 0x20000, 0));

    // A and B trade places: each goes where the other was (the copy backend parks one of them)
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(1, 1));
    {
        uint8_t swap[VRAM_BANK_COUNT];
        for (int b = 0; b < VRAM_BANK_COUNT; b++) {
            swap[b] = vram_get_vramcnt(v, b);
        }
        swap[VRAM_BANK_A] = cnt(1, 1);
        swap[VRAM_BANK_B] = cnt(1, 0);
        vram_set_all_vramcnt(v, swap);
    }
    CHECK(has_pattern(bg, 0x20000, VRAM_BANK_B + 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, VRAM_BANK_A + 1));
}

// Slots with no bank read 0 and absorb writes. codein.c and error_window_screen.c set GX_VRAM_BG_128_A and then clear
// the whole 512 KiB BG-A view (MIi_CpuClear32(0, HW_BG_VRAM, HW_BG_VRAM_SIZE)): the 384 KiB with no bank must take the
// stores, and the banks elsewhere must come through untouched
static void test_unmapped(Vram *v) {
    uint8_t *bg = vram_view(v, VRAM_VIEW_BG_A);
    uint8_t *lcdc = vram_view(v, VRAM_VIEW_LCDC);

    gContext = "unmapped";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    memset(bg, 0xcd, gVramViewSize[VRAM_VIEW_BG_A]);
    for (int b = VRAM_BANK_B; b < VRAM_BANK_COUNT; b++) {
        CHECK(bank_has_pattern(v, b, b + 1));
    }
    vram_set_vramcnt(v, VRAM_BANK_A, LCDC);
    CHECK(all_bytes(lcdc_bank(v, VRAM_BANK_A), 0x20000, 0xcd));
    // The slots A left read 0 again; the ones no bank ever took keep the stray bytes until their mapping changes
    // (the DS would read 0 there: the one deviation, and only for a read after a stray write)
    CHECK(all_bytes(bg, 0x20000, 0));
    CHECK(all_bytes(bg + 0x20000, 0x60000, 0xcd));
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(1, 1));
    CHECK(has_pattern(bg + 0x20000, 0x20000, VRAM_BANK_B + 1));

    // A robustness check, not a BW2 case: every whole-LCDC clear in BW2 (GFL_BGSysInitVRAM, geonet.c,
    // game_clear_save.c, battle_lose_message.c, event_mapchange.c's ClearLCDCVram) first puts every bank in LCDC. Here
    // A is in BG-A and C in texture slot 2 while all of LCDC is cleared
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    vram_set_vramcnt(v, VRAM_BANK_C, cnt(3, 2));
    memset(lcdc, 0, gVramViewSize[VRAM_VIEW_LCDC]);
    CHECK(has_pattern(bg, 0x20000, VRAM_BANK_A + 1));
    CHECK(has_pattern(vram_slot_ptr(v, VRAM_VIEW_TEX, 16), VRAM_SLOT_SIZE, VRAM_BANK_C + 1));
    CHECK(bank_has_pattern(v, VRAM_BANK_C, VRAM_BANK_C + 1));
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_B, 0), VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_I, 0), VRAM_SLOT_SIZE, 0));
}

// A bank keeps its bytes while it is disabled, given to the ARM7 or in a slot only the GPU reads
static void test_keep(Vram *v) {
    gContext = "keep";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_C, 0);
    CHECK(all_bytes(lcdc_bank(v, VRAM_BANK_C), 0x20000, 0));
    vram_set_vramcnt(v, VRAM_BANK_C, cnt(2, 0));
    CHECK(vram_slot_ptr(v, VRAM_VIEW_TEX, 16) == NULL);
    vram_set_vramcnt(v, VRAM_BANK_C, cnt(3, 2));
    CHECK(slots_have_pattern(v, VRAM_VIEW_TEX, 16, 8, VRAM_BANK_C + 1));
    vram_set_vramcnt(v, VRAM_BANK_C, LCDC);
    CHECK(has_pattern(lcdc_bank(v, VRAM_BANK_C), 0x20000, VRAM_BANK_C + 1));

    // E's upper half isn't in the extended palette slots but survives them
    vram_set_vramcnt(v, VRAM_BANK_E, cnt(4, 0));
    CHECK(vram_slot_ptr(v, VRAM_VIEW_BGEXT_A, 1) != NULL);
    vram_set_vramcnt(v, VRAM_BANK_E, LCDC);
    CHECK(has_pattern(lcdc_bank(v, VRAM_BANK_E), 0x10000, VRAM_BANK_E + 1));
}

// F at OFS 0 is at 0x06000000 and 0x06008000; I as sub OBJ fills the 128 KiB view eight times
static void test_mirrors(Vram *v) {
    uint8_t *bg = vram_view(v, VRAM_VIEW_BG_A);
    bool alias = vram_backend(v) != VRAM_BACKEND_COPY;

    gContext = "mirrors";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_F, cnt(1, 0));
    CHECK(has_pattern(bg, VRAM_SLOT_SIZE, VRAM_BANK_F + 1));
    fill_pattern(bg, VRAM_SLOT_SIZE, 300);
    CHECK(vram_slot_ptr(v, VRAM_VIEW_BG_A, 1) == NULL);
    CHECK(has_pattern(vram_slot_ptr(v, VRAM_VIEW_BG_A, 2), VRAM_SLOT_SIZE, 300));
    if (alias) {
        // Aliasing makes the mirror live for the CPU too
        CHECK(has_pattern(bg + 2 * VRAM_SLOT_SIZE, VRAM_SLOT_SIZE, 300));
        fill_pattern(bg + 2 * VRAM_SLOT_SIZE, VRAM_SLOT_SIZE, 301);
        CHECK(has_pattern(bg, VRAM_SLOT_SIZE, 301));
    }
    vram_set_vramcnt(v, VRAM_BANK_I, cnt(2, 0));
    for (int s = 0; s < 8; s++) {
        CHECK(has_pattern(vram_slot_ptr(v, VRAM_VIEW_OBJ_B, s), VRAM_SLOT_SIZE, VRAM_BANK_I + 1));
    }
}

// Stores through mirrors. codein.c sets GX_VRAM_SUB_BG_32_H (also at 0x06210000) and GX_VRAM_SUB_OBJ_16_I (eight
// times in OBJ-B), then clears all 128 KiB of both sub views, as error_window_screen.c does for sub OBJ: stores through
// every mirror. A uniform fill of a whole view leaves the same bank bytes in every backend. A store through a mirror
// alone reaches the bank only when aliasing, as on the DS; the copy backend keeps it in the mirror slot's own bytes
// until that slot's mapping changes, and the renderer and the bank never see it
static void test_mirror_stores(Vram *v) {
    uint8_t *bgB = vram_view(v, VRAM_VIEW_BG_B);
    uint8_t *objB = vram_view(v, VRAM_VIEW_OBJ_B);
    bool alias = vram_backend(v) != VRAM_BACKEND_COPY;

    gContext = "mirror stores";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_H, cnt(1, 0));
    vram_set_vramcnt(v, VRAM_BANK_I, cnt(2, 0));
    memset(bgB, 0, gVramViewSize[VRAM_VIEW_BG_B]);
    memset(objB, 0, gVramViewSize[VRAM_VIEW_OBJ_B]);
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_H, 0), VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_H, 1), VRAM_SLOT_SIZE, 0));
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_I, 0), VRAM_SLOT_SIZE, 0));
    memset(objB, 0x5a, gVramViewSize[VRAM_VIEW_OBJ_B]);
    CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_I, 0), VRAM_SLOT_SIZE, 0x5a));

    // OBJ-B's second slot alone, as a character load past I's 16 KiB would store
    fill_pattern(objB + VRAM_SLOT_SIZE, VRAM_SLOT_SIZE, 600);
    if (alias) {
        CHECK(has_pattern(vram_bank_page(v, VRAM_BANK_I, 0), VRAM_SLOT_SIZE, 600));
        CHECK(has_pattern(objB, VRAM_SLOT_SIZE, 600));
    } else {
        CHECK(all_bytes(vram_bank_page(v, VRAM_BANK_I, 0), VRAM_SLOT_SIZE, 0x5a));
        CHECK(all_bytes(vram_slot_ptr(v, VRAM_VIEW_OBJ_B, 1), VRAM_SLOT_SIZE, 0x5a));
        CHECK(has_pattern(objB + VRAM_SLOT_SIZE, VRAM_SLOT_SIZE, 600));
    }
    vram_set_vramcnt(v, VRAM_BANK_I, LCDC);
    CHECK(has_pattern(lcdc_bank(v, VRAM_BANK_I), VRAM_SLOT_SIZE, 600) == alias);
    CHECK(all_bytes(objB, gVramViewSize[VRAM_VIEW_OBJ_B], 0));
}

// GX_SetBankForBG(GX_VRAM_BG_128_A) after GX_VRAM_BG_64_E: the SDK writes A's VRAMCNT while E is still a BG bank, then
// puts E back in LCDC, so for one register write two banks claim BG-A's first 64 KiB
static void test_sdk_switch(Vram *v) {
    uint8_t *bg = vram_view(v, VRAM_VIEW_BG_A);

    gContext = "sdk switch";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_E, cnt(1, 0));
    fill_pattern(bg, 0x10000, 400);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    CHECK(vram_layout(v)->overlaps == 4);
    // GX_VRAMCNT_SetLCDC_ of the free banks: E and the ones already there
    for (int b = VRAM_BANK_B; b < VRAM_BANK_COUNT; b++) {
        vram_set_vramcnt(v, b, LCDC);
    }
    CHECK(vram_layout(v)->overlaps == 0);
    CHECK(has_pattern(bg, 0x20000, VRAM_BANK_A + 1));
    CHECK(has_pattern(lcdc_bank(v, VRAM_BANK_E), 0x10000, 400));
}

// GX_BeginLoadTex and GX_EndLoadTex: the texture banks go to LCDC for the upload and come back, which a texture cache
// sees in the page generations
static void test_texture_upload(Vram *v) {
    uint32_t before[16];

    gContext = "texture upload";
    fill_banks(v);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(3, 0));
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(3, 1));
    for (int s = 0; s < 16; s++) {
        before[s] = vram_page_generation(v, s / 8, s % 8);
    }
    vram_set_vramcnt(v, VRAM_BANK_A, LCDC);
    vram_set_vramcnt(v, VRAM_BANK_B, LCDC);
    fill_pattern(lcdc_bank(v, VRAM_BANK_A), 0x40000, 500);
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(3, 0));
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(3, 1));
    for (int s = 0; s < 16; s++) {
        CHECK(vram_page_generation(v, s / 8, s % 8) != before[s]);
    }
    CHECK(slots_have_pattern(v, VRAM_VIEW_TEX, 0, 16, 500));
    if (vram_backend(v) != VRAM_BACKEND_COPY) {
        CHECK(has_pattern(vram_view(v, VRAM_VIEW_TEX), 0x40000, 500));
    }
}

// The guard after each view, and the debugging mode where slots with no bank fault
static void test_faults(VramBackend backend, void *region) {
    Vram *v;

    gContext = "faults";
    if (backend == VRAM_BACKEND_COPY) {
        return;
    }
    v = vram_create(backend, VRAM_FLAG_TRAP_UNMAPPED, region);
    CHECK(v != NULL);
    if (v == NULL) {
        return;
    }
    CHECK(store_faults(vram_view(v, VRAM_VIEW_BG_A)));
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    CHECK(!store_faults(vram_view(v, VRAM_VIEW_BG_A)));
    CHECK(store_faults(vram_view(v, VRAM_VIEW_BG_A) + 0x20000));
    CHECK(store_faults(vram_view(v, VRAM_VIEW_BG_A) + gVramViewSize[VRAM_VIEW_BG_A]));
    vram_destroy(v);
}

// The reference: bank bytes, plus the own bytes of each slot that shows no bank page to the CPU, which go back to 0
// when the slot's mapping changes. Those are the slots with no bank, and with the copy backend the mirror slots too:
// there a store through a mirror stays in the slot
typedef struct Model {
    bool copy;
    uint8_t vramcnt[VRAM_BANK_COUNT];
    VramLayout layout;
    uint8_t banks[VRAM_BANKS_SIZE];
    uint8_t *loose[VRAM_VIEW_COUNT];
} Model;

static bool model_own_bytes(const Model *m, VramSlot s) {
    return s.bank == VRAM_BANK_NONE || (m->copy && s.mirror);
}

static void model_set(Model *m, int bank, uint8_t value) {
    VramLayout next;

    m->vramcnt[bank] = value;
    vram_layout_compute(m->vramcnt, &next);
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            VramSlot a = m->layout.slots[view][slot];
            VramSlot b = next.slots[view][slot];
            bool changed = a.bank != b.bank || a.page != b.page || a.mirror != b.mirror;
            if (changed && model_own_bytes(m, b)) {
                memset(m->loose[view] + ((size_t)slot << VRAM_SLOT_SHIFT), 0, VRAM_SLOT_SIZE);
            }
        }
    }
    m->layout = next;
}

// The bank bytes a slot shows the renderer
static uint8_t *model_bank(Model *m, VramSlot s, size_t offset) {
    return m->banks + ((size_t)(gVramBankFirstPage[s.bank] + s.page) << VRAM_SLOT_SHIFT) +
           (offset & (VRAM_SLOT_SIZE - 1));
}

// The bytes a CPU access at a view offset reaches
static uint8_t *model_byte(Model *m, int view, size_t offset) {
    VramSlot s = m->layout.slots[view][offset >> VRAM_SLOT_SHIFT];
    if (model_own_bytes(m, s)) {
        return m->loose[view] + offset;
    }
    return model_bank(m, s, offset);
}

static void model_check(Model *m, Vram *v) {
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            size_t offset = (size_t)slot << VRAM_SLOT_SHIFT;
            VramSlot s = m->layout.slots[view][slot];
            CHECK(memcmp(&s, &vram_layout(v)->slots[view][slot], sizeof(s)) == 0);
            if (s.bank == VRAM_BANK_NONE) {
                CHECK(vram_slot_ptr(v, view, slot) == NULL);
            } else {
                CHECK(memcmp(vram_slot_ptr(v, view, slot), model_bank(m, s, offset), VRAM_SLOT_SIZE) == 0);
            }
            // The copy backend keeps the pages of the GPU-only views outside the views
            if (!m->copy || view < VRAM_CPU_VIEW_COUNT) {
                CHECK(memcmp(vram_view(v, view) + offset, model_byte(m, view, offset), VRAM_SLOT_SIZE) == 0);
            }
        }
    }
    for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
        int bank = 0;
        while (bank + 1 < VRAM_BANK_COUNT && gVramBankFirstPage[bank + 1] <= p) {
            bank++;
        }
        CHECK(memcmp(vram_bank_page(v, bank, p - gVramBankFirstPage[bank]), m->banks + ((size_t)p << VRAM_SLOT_SHIFT),
                     VRAM_SLOT_SIZE) == 0);
    }
}

// Random VRAMCNT writes and random stores through the CPU views, mirrors included, a quarter of them zeros, with the
// backend compared to the model after every remap. The model holds each backend to its own rule for mirrors
// (test_mirror_stores): the bank when aliasing, the slot's own bytes when copying
static void test_random(Vram *v) {
    Model *m = calloc(1, sizeof(*m));
    uint32_t state = 12345;
    int failuresBefore = gFailures;
    int mirrorStores = 0;

    gContext = "random";
    m->copy = vram_backend(v) == VRAM_BACKEND_COPY;
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        m->loose[view] = calloc(1, gVramViewSize[view]);
    }
    // Start from the Vram's state: all banks in LCDC and zero
    set_all(v, LCDC);
    memset(vram_view(v, VRAM_VIEW_LCDC), 0, gVramViewSize[VRAM_VIEW_LCDC]);
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        model_set(m, b, LCDC);
    }
    // Slots no bank took since the earlier tests still hold what those wrote
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        memcpy(m->loose[view], vram_view(v, view), gVramViewSize[view]);
    }
    model_check(m, v);

    for (int step = 0; step < 3000 && gFailures == failuresBefore; step++) {
        uint32_t r = next_word(&state);
        if (r % 5 == 0) {
            int bank = (int)(next_word(&state) % VRAM_BANK_COUNT);
            uint32_t pick = next_word(&state) % 10;
            uint8_t value = pick < 5 ? LCDC : pick == 5 ? 0 : (uint8_t)(next_word(&state) | 0x80);
            vram_set_vramcnt(v, bank, value);
            model_set(m, bank, value);
            model_check(m, v);
        } else {
            int view = (int)(next_word(&state) % VRAM_CPU_VIEW_COUNT);
            size_t slot = next_word(&state) % (uint32_t)vram_view_slot_count(view);
            size_t start = (next_word(&state) % VRAM_SLOT_SIZE) & ~(size_t)3;
            size_t len = 4 + (next_word(&state) % 2048);
            uint8_t value = next_word(&state) % 4 == 0 ? 0 : (uint8_t)next_word(&state);
            if (start + len > VRAM_SLOT_SIZE) {
                len = VRAM_SLOT_SIZE - start;
            }
            mirrorStores += m->layout.slots[view][slot].mirror;
            memset(vram_view(v, view) + (slot << VRAM_SLOT_SHIFT) + start, value, len);
            memset(model_byte(m, view, (slot << VRAM_SLOT_SHIFT) + start), value, len);
        }
    }
    model_check(m, v);
    // The walk must actually store through mirrors for the rule above to be tested
    CHECK(mirrorStores > 20);
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        free(m->loose[view]);
    }
    free(m);
}

static int create_failed(VramBackend backend) {
    if (errno == ENOTSUP) {
        printf("%s: not available on this host, skipped\n", gVramBackendName[backend]);
        return 77;
    }
    perror("vram_create");
    return 1;
}

static int run_backend(VramBackend backend, bool staticRegion) {
    void *region = staticRegion ? static_region(backend) : NULL;
    Vram *v = vram_create(backend, 0, region);

    if (v == NULL) {
        return create_failed(backend);
    }
    test_alignment(v);
    test_guards(v, staticRegion);
    test_move(v);
    test_unmapped(v);
    test_keep(v);
    test_mirrors(v);
    test_mirror_stores(v);
    test_sdk_switch(v);
    test_texture_upload(v);
    test_random(v);
    printf("%s%s: %u remaps, %u mapping calls, %llu bytes copied, %llu bytes zeroed\n", gVramBackendName[backend],
           staticRegion ? " (static region)" : "", vram_stats(v)->remaps, vram_stats(v)->os_calls,
           (unsigned long long)vram_stats(v)->bytes_copied, (unsigned long long)vram_stats(v)->bytes_zeroed);
    vram_destroy(v);
    test_faults(backend, region);
    test_region_rules(backend);
    return 0;
}

// Exits with a static region's views still mapped, as a game that quits without tearing the HAL down leaves them. On
// Linux under ASan, LeakSanitizer then reads every word of .bss: a page there without access would crash it. The Vram
// stays reachable from a global, so it isn't reported as a leak
static Vram *gLiveVram;

static int run_exit_live(VramBackend backend) {
    gLiveVram = vram_create(backend, 0, static_region(backend));
    if (gLiveVram == NULL) {
        return create_failed(backend);
    }
    // codein.c's setting: banks in BG, OBJ, sub BG and sub OBJ with mirrors, texture and texture palette slots
    fill_banks(gLiveVram);
    vram_set_vramcnt(gLiveVram, VRAM_BANK_A, cnt(1, 0));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_B, cnt(3, 0));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_C, cnt(3, 1));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_E, cnt(2, 0));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_F, cnt(3, 0));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_G, cnt(3, 1));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_H, cnt(1, 0));
    vram_set_vramcnt(gLiveVram, VRAM_BANK_I, cnt(2, 0));
    printf("%s: exiting with the views mapped\n", gVramBackendName[backend]);
    return 0;
}

int main(int argc, char **argv) {
    int status = 0;

    if (argc >= 2 && strcmp(argv[1], "layout") == 0) {
        test_layout();
    } else if (argc >= 2) {
        int backend = -1;
        for (int i = 0; i < VRAM_BACKEND_COUNT; i++) {
            if (strcmp(argv[1], gVramBackendName[i]) == 0) {
                backend = i;
            }
        }
        if (backend < 0) {
            fprintf(stderr, "unknown backend %s\n", argv[1]);
            return 2;
        }
        if (argc >= 3 && strcmp(argv[2], "exit-live") == 0) {
            return run_exit_live(backend);
        }
        status = run_backend(backend, argc >= 3 && strcmp(argv[2], "static") == 0);
    } else {
        fprintf(stderr, "usage: test_vram layout | <backend> [static | exit-live]\n");
        return 2;
    }
    if (gFailures != 0) {
        fprintf(stderr, "%d checks failed\n", gFailures);
        return 1;
    }
    if (status == 0) {
        printf("ok\n");
    }
    return status;
}
