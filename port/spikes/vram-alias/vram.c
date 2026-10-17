// The Vram object: views in one region, a layout from VRAMCNT, and the two ways to apply a new layout. Aliasing maps
// the bank pages into the slots that changed. Copying keeps each bank page in one place (its first slot in a CPU view,
// or else its parking page) and moves the bytes when that place changes. The DS never shows a bank in two views at
// once, so one home per page is enough for every CPU access except a store through a mirror (README.md)
#include "vram.h"
#include "vram_os.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const char *const gVramBackendName[VRAM_BACKEND_COUNT] = { "shm", "mach-entry", "mach-remap", "copy" };

struct Vram {
    VramBackend backend;
    unsigned flags;
    uint8_t *region;
    bool regionOwned;
    uint8_t *views[VRAM_VIEW_COUNT];
    uint8_t vramcnt[VRAM_BANK_COUNT];
    VramLayout layout;
    VramLayout next;
    uint32_t generation[VRAM_PAGE_COUNT];
    VramStats stats;
    VramOs os;
    // Copy backend: the parking pages and where each bank page is now
    uint8_t *park;
    uint8_t *home[VRAM_PAGE_COUNT];
};

// The region: a guard slot, then each view followed by a guard slot. In the aliasing backends a guard of a reserved
// region has no access, so running off the end of a view faults instead of writing into the next one. A caller's
// region keeps its guards accessible unless trap mode is on (vram_create says why)
size_t vram_region_size(void) {
    size_t size = VRAM_SLOT_SIZE;

    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        size += gVramViewSize[view] + VRAM_SLOT_SIZE;
    }
    return size;
}

size_t vram_region_alignment(VramBackend backend) {
    if (backend == VRAM_BACKEND_COPY) {
        return 8;
    }
    return vram_os_page_size();
}

static bool same_slot(VramSlot a, VramSlot b) {
    return a.bank == b.bank && a.page == b.page && a.mirror == b.mirror;
}

static int page_index(VramSlot s) {
    return gVramBankFirstPage[s.bank] + s.page;
}

// Whether slot t, k slots after s, continues s's run: the next page of the same bank, or no bank after no bank
static bool continues_run(VramSlot s, int k, VramSlot t) {
    if (s.bank == VRAM_BANK_NONE) {
        return t.bank == VRAM_BANK_NONE;
    }
    return t.bank == s.bank && t.page == s.page + k;
}

static int alias_map_run(Vram *vram, VramView view, int slot, int count, VramSlot s) {
    uint8_t *addr = vram->views[view] + ((size_t)slot << VRAM_SLOT_SHIFT);
    size_t len = (size_t)count << VRAM_SLOT_SHIFT;

    if (s.bank == VRAM_BANK_NONE) {
        return vram_os_map_anon(&vram->os, addr, len, (vram->flags & VRAM_FLAG_TRAP_UNMAPPED) == 0);
    }
    return vram_os_map_banks(&vram->os, addr, len, (size_t)page_index(s) << VRAM_SLOT_SHIFT);
}

// Maps every changed slot, one call per run of slots that continue each other. A slot with no bank gets fresh zero
// pages: it reads 0 as on the DS, and a write to it is kept only until its mapping next changes
static int alias_apply(Vram *vram, const VramLayout *next) {
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        const VramSlot *cur = vram->layout.slots[view];
        const VramSlot *nxt = next->slots[view];
        int count = vram_view_slot_count(view);
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
            err = alias_map_run(vram, view, i, j - i, nxt[i]);
            if (err != 0) {
                return err;
            }
            i = j;
        }
    }
    vram->stats.os_calls = vram->os.calls;
    return 0;
}

static uint8_t *park_page(Vram *vram, int p) {
    return vram->park + ((size_t)p << VRAM_SLOT_SHIFT);
}

static void copy_page(Vram *vram, int p, uint8_t *to) {
    memcpy(to, vram->home[p], VRAM_SLOT_SIZE);
    vram->home[p] = to;
    vram->stats.bytes_copied += VRAM_SLOT_SIZE;
}

// Whether a page other than p still has to move out of a slot
static bool slot_taken(const Vram *vram, const bool moving[VRAM_PAGE_COUNT], const uint8_t *slot, int p) {
    for (int q = 0; q < VRAM_PAGE_COUNT; q++) {
        if (q != p && moving[q] && vram->home[q] == slot) {
            return true;
        }
    }
    return false;
}

// Moves the bank pages whose home changes, each straight to its new home once no other page is still there. Only
// pages that trade slots go through a parking page. A page in the GPU-only views lives in its parking page: game code
// never reaches those views, and the renderer reads them through vram_slot_ptr. Then the CPU slots that lost their page
// are cleared; mirror slots stay clear, as here only vram_slot_ptr shows a mirror
static void copy_apply(Vram *vram, const VramLayout *next) {
    uint8_t *newHome[VRAM_PAGE_COUNT];
    bool moving[VRAM_PAGE_COUNT];
    int left = 0;

    for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
        newHome[p] = park_page(vram, p);
    }
    for (int view = 0; view < VRAM_CPU_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            VramSlot s = next->slots[view][slot];
            if (s.bank != VRAM_BANK_NONE && !s.mirror) {
                newHome[page_index(s)] = vram->views[view] + ((size_t)slot << VRAM_SLOT_SHIFT);
            }
        }
    }
    for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
        moving[p] = vram->home[p] != newHome[p];
        left += moving[p];
    }
    while (left > 0) {
        int moved = 0;

        for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
            // A page's parking page is only ever its own, so it is always free
            if (moving[p] && !slot_taken(vram, moving, newHome[p], p)) {
                copy_page(vram, p, newHome[p]);
                moving[p] = false;
                moved++;
            }
        }
        if (moved == 0) {
            // Every page left waits for another: a cycle. Parking one of them frees its slot
            for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
                if (moving[p] && vram->home[p] != park_page(vram, p)) {
                    copy_page(vram, p, park_page(vram, p));
                    break;
                }
            }
        }
        left -= moved;
    }
    for (int view = 0; view < VRAM_CPU_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            VramSlot s = next->slots[view][slot];
            if (!same_slot(vram->layout.slots[view][slot], s) && (s.bank == VRAM_BANK_NONE || s.mirror)) {
                memset(vram->views[view] + ((size_t)slot << VRAM_SLOT_SHIFT), 0, VRAM_SLOT_SIZE);
                vram->stats.bytes_zeroed += VRAM_SLOT_SIZE;
            }
        }
    }
}

// Every page that leaves or enters a slot gets a new generation
static void bump_generations(Vram *vram, const VramLayout *next) {
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            VramSlot a = vram->layout.slots[view][slot];
            VramSlot b = next->slots[view][slot];
            if (same_slot(a, b)) {
                continue;
            }
            if (a.bank != VRAM_BANK_NONE) {
                vram->generation[page_index(a)]++;
            }
            if (b.bank != VRAM_BANK_NONE) {
                vram->generation[page_index(b)]++;
            }
        }
    }
}

static void apply(Vram *vram) {
    vram_layout_compute(vram->vramcnt, &vram->next);
    if (memcmp(vram->next.slots, vram->layout.slots, sizeof(vram->layout.slots)) == 0) {
        vram->layout.overlaps = vram->next.overlaps;
        return;
    }
    vram->stats.remaps++;
    bump_generations(vram, &vram->next);
    if (vram->backend == VRAM_BACKEND_COPY) {
        copy_apply(vram, &vram->next);
    } else if (alias_apply(vram, &vram->next) != 0) {
        // A failed mapping call leaves the views in a state no layout describes; the HAL can only stop
        abort();
    }
    vram->layout = vram->next;
}

Vram *vram_create(VramBackend backend, unsigned flags, void *region) {
    size_t size = vram_region_size();
    Vram *vram;
    uint8_t *at;
    int err = 0;

    if (backend >= VRAM_BACKEND_COUNT || !vram_os_backend_available(backend)) {
        errno = ENOTSUP;
        return NULL;
    }
    if (region != NULL && (uintptr_t)region % vram_region_alignment(backend) != 0) {
        errno = EINVAL;
        return NULL;
    }
    vram = calloc(1, sizeof(*vram));
    if (vram == NULL) {
        return NULL;
    }
    vram->backend = backend;
    vram->flags = flags;
    vram->os.backend = backend;
    vram->os.fd = -1;
    vram->region = region;

    if (backend == VRAM_BACKEND_COPY) {
        // Plain memory and no OS calls: this backend runs anywhere C does. calloc, not aligned_alloc, which the UCRT
        // doesn't have; malloc's alignment is enough here
        if (vram->region == NULL) {
            vram->region = calloc(1, size);
            vram->regionOwned = true;
        }
        vram->park = calloc(1, VRAM_BANKS_SIZE);
        if (vram->region == NULL || vram->park == NULL) {
            err = ENOMEM;
            goto fail;
        }
        memset(vram->region, 0, size);
        for (int p = 0; p < VRAM_PAGE_COUNT; p++) {
            vram->home[p] = vram->park + ((size_t)p << VRAM_SLOT_SHIFT);
        }
    } else {
        if (vram->region == NULL) {
            err = vram_os_reserve(size, &vram->region);
            if (err != 0) {
                goto fail;
            }
            vram->regionOwned = true;
        } else {
            // A caller's region is usually a static array, and on Linux LeakSanitizer (on by default under ASan) reads
            // every word of .data and .bss at exit without checking access. So its guards stay accessible zero pages,
            // unless trap mode asks for faults. A reserved region has no access anywhere yet
            err = vram_os_map_anon(&vram->os, vram->region, size, (flags & VRAM_FLAG_TRAP_UNMAPPED) == 0);
            if (err != 0) {
                goto fail;
            }
        }
        err = vram_os_banks_create(&vram->os);
        if (err != 0) {
            goto fail;
        }
    }

    at = vram->region + VRAM_SLOT_SIZE;
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        vram->views[view] = at;
        at += gVramViewSize[view] + VRAM_SLOT_SIZE;
        // Every slot starts with no bank, as every VRAMCNT starts at 0
        if (backend != VRAM_BACKEND_COPY) {
            err = vram_os_map_anon(&vram->os, vram->views[view], gVramViewSize[view],
                                   (flags & VRAM_FLAG_TRAP_UNMAPPED) == 0);
            if (err != 0) {
                goto fail;
            }
        }
    }
    vram_layout_compute(vram->vramcnt, &vram->layout);
    vram->stats.os_calls = vram->os.calls;
    return vram;

fail:
    vram_destroy(vram);
    errno = err;
    return NULL;
}

void vram_destroy(Vram *vram) {
    if (vram == NULL) {
        return;
    }
    if (vram->backend == VRAM_BACKEND_COPY) {
        if (vram->regionOwned) {
            free(vram->region);
        }
        free(vram->park);
    } else {
        if (vram->region != NULL) {
            if (vram->regionOwned) {
                vram_os_release(vram->region, vram_region_size());
            } else {
                // Give the caller's region back as plain zeroed memory
                vram_os_map_anon(&vram->os, vram->region, vram_region_size(), true);
            }
        }
        vram_os_banks_destroy(&vram->os);
    }
    free(vram);
}

uint8_t *vram_view(Vram *vram, VramView view) {
    return vram->views[view];
}

void vram_set_vramcnt(Vram *vram, VramBank bank, uint8_t value) {
    vram->vramcnt[bank] = value;
    apply(vram);
}

void vram_set_all_vramcnt(Vram *vram, const uint8_t vramcnt[VRAM_BANK_COUNT]) {
    memcpy(vram->vramcnt, vramcnt, VRAM_BANK_COUNT);
    apply(vram);
}

uint8_t vram_get_vramcnt(const Vram *vram, VramBank bank) {
    return vram->vramcnt[bank];
}

const VramLayout *vram_layout(const Vram *vram) {
    return &vram->layout;
}

uint8_t *vram_slot_ptr(Vram *vram, VramView view, int slot) {
    VramSlot s = vram->layout.slots[view][slot];

    if (s.bank == VRAM_BANK_NONE) {
        return NULL;
    }
    if (vram->backend == VRAM_BACKEND_COPY) {
        return vram->home[page_index(s)];
    }
    return vram->views[view] + ((size_t)slot << VRAM_SLOT_SHIFT);
}

uint8_t *vram_bank_page(Vram *vram, VramBank bank, int page) {
    int p = gVramBankFirstPage[bank] + page;

    if (vram->backend == VRAM_BACKEND_COPY) {
        return vram->home[p];
    }
    return vram->os.window + ((size_t)p << VRAM_SLOT_SHIFT);
}

uint32_t vram_page_generation(const Vram *vram, VramBank bank, int page) {
    return vram->generation[gVramBankFirstPage[bank] + page];
}

const VramStats *vram_stats(const Vram *vram) {
    return &vram->stats;
}

VramBackend vram_backend(const Vram *vram) {
    return vram->backend;
}
