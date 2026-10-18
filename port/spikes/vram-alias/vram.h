// Frozen phase-0 spike (port/spikes/README.md), outside Frigate's build: the live code is port/hal/gx/vram.h; change
// that, not this copy.
// VRAM bank mapping for the host, the spike of PLAN.md §4.5 "VRAM mapping by page aliasing". The DS has nine VRAM
// banks, and VRAMCNT puts each one into one place: the LCDC view the CPU reaches at 0x06800000, a 2D engine's BG or OBJ
// view, or a slot only the GPU reads (texture images, texture palettes, extended palettes). On the host every place is
// a view, one contiguous block of host memory laid out as the DS lays out that place, and VRAMCNT decides which bank
// page appears in each 16 KiB slot of each view. A write through one view is then visible through the view the bank
// moves to next, as on the DS.
//
// The layout part (vram_layout_compute) is plain C. The Vram object applies a layout to host memory with one of the
// backends below: three that alias pages with the virtual memory system, and one that moves bank contents on remap.
#ifndef VRAM_H
#define VRAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The unit of every placement: the smallest bank (F, G, I) and every bank offset are multiples of it
#define VRAM_SLOT_SIZE 0x4000
#define VRAM_SLOT_SHIFT 14

typedef enum VramBank {
    VRAM_BANK_A,
    VRAM_BANK_B,
    VRAM_BANK_C,
    VRAM_BANK_D,
    VRAM_BANK_E,
    VRAM_BANK_F,
    VRAM_BANK_G,
    VRAM_BANK_H,
    VRAM_BANK_I,
    VRAM_BANK_COUNT,
    VRAM_BANK_NONE = 0xff,
} VramBank;

// The places a bank can be. The first five (VRAM_CPU_VIEW_COUNT) are the CPU views at their DS addresses; the rest only
// the GPU reads, so game code never holds pointers into them (the renderer does)
typedef enum VramView {
    VRAM_VIEW_LCDC,     // 0x06800000, 656 KiB: every bank at a fixed offset when its MST is 0
    VRAM_VIEW_BG_A,     // 0x06000000, 512 KiB: main engine BG
    VRAM_VIEW_OBJ_A,    // 0x06400000, 256 KiB: main engine OBJ
    VRAM_VIEW_BG_B,     // 0x06200000, 128 KiB: sub engine BG
    VRAM_VIEW_OBJ_B,    // 0x06600000, 128 KiB: sub engine OBJ
    VRAM_VIEW_TEX,      // texture image slots 0-3 (2-3 also the clear image), 512 KiB
    VRAM_VIEW_TEXPLTT,  // texture palette slots 0-5, 96 KiB
    VRAM_VIEW_BGEXT_A,  // main BG extended palette slots 0-3, 32 KiB
    VRAM_VIEW_OBJEXT_A, // main OBJ extended palette, 8 KiB used of one 16 KiB slot
    VRAM_VIEW_BGEXT_B,  // sub BG extended palette slots 0-3, 32 KiB
    VRAM_VIEW_OBJEXT_B, // sub OBJ extended palette, 8 KiB used of one 16 KiB slot
    VRAM_VIEW_COUNT,
    VRAM_CPU_VIEW_COUNT = VRAM_VIEW_TEX,
} VramView;

// The LCDC view has one slot per bank page, so a bank page's index in it names the page everywhere
#define VRAM_PAGE_COUNT 41
#define VRAM_BANKS_SIZE (VRAM_PAGE_COUNT * VRAM_SLOT_SIZE)
#define VRAM_MAX_SLOTS VRAM_PAGE_COUNT
// The views take 149 slots; a guard slot before each and after the last makes 161
#define VRAM_REGION_SIZE (161 * VRAM_SLOT_SIZE)

// One 16 KiB slot of a view: which bank page is there. A mirror is a further place of a page already placed in the
// same view (F and G twice in a main engine view, H twice and I four or eight times in a sub engine view)
typedef struct VramSlot {
    uint8_t bank;
    uint8_t page;
    uint8_t mirror;
} VramSlot;

typedef struct VramLayout {
    VramSlot slots[VRAM_VIEW_COUNT][VRAM_MAX_SLOTS];
    // Slots two banks claimed at once. The SDK makes these only for a moment inside a GX_SetBankFor* function (the
    // new banks are written before the old ones go back to LCDC); the DS would OR their reads. The lower bank wins
    int overlaps;
} VramLayout;

extern const uint32_t gVramBankSize[VRAM_BANK_COUNT];
extern const uint8_t gVramBankFirstPage[VRAM_BANK_COUNT];
extern const uint32_t gVramViewSize[VRAM_VIEW_COUNT];
extern const uint32_t gVramViewDsAddress[VRAM_VIEW_COUNT]; // 0 for the views the CPU can't reach
extern const char *const gVramViewName[VRAM_VIEW_COUNT];

// Fills a layout from the nine VRAMCNT bytes (A-I), following GBATEK's "DS Memory Control - VRAM" and melonDS's mirrors
void vram_layout_compute(const uint8_t vramcnt[VRAM_BANK_COUNT], VramLayout *layout);

static inline int vram_view_slot_count(VramView view) {
    return (int)(gVramViewSize[view] >> VRAM_SLOT_SHIFT);
}

typedef enum VramBackend {
    VRAM_BACKEND_SHM,        // a POSIX shared memory object (memfd on Linux) mapped with mmap(MAP_SHARED | MAP_FIXED)
    VRAM_BACKEND_MACH_ENTRY, // macOS: a Mach named memory entry mapped with mach_vm_map(VM_FLAGS_OVERWRITE)
    VRAM_BACKEND_MACH_REMAP, // macOS: pages of an anonymous bank window shared with mach_vm_remap(copy = FALSE)
    VRAM_BACKEND_COPY,       // any platform: views are plain memory and a remap moves the bank's bytes
    VRAM_BACKEND_COUNT,
} VramBackend;

extern const char *const gVramBackendName[VRAM_BACKEND_COUNT];

enum {
    // Slots with no bank fault on access instead of reading zero and absorbing writes (aliasing backends only), and the
    // guard slots of a caller's region fault too. A debugging aid: BW2 clears whole CPU views while only part of them
    // is mapped (HW_BG_VRAM_SIZE with GX_VRAM_BG_128_A in codein.c), so the game itself must run without it. With a
    // static region it leaves pages without access in .bss, which LeakSanitizer reads at exit: destroy the Vram first
    VRAM_FLAG_TRAP_UNMAPPED = 1 << 0,
};

typedef struct VramStats {
    uint32_t remaps;       // vram_set_vramcnt calls that changed the layout
    uint32_t os_calls;     // mapping calls made (aliasing backends)
    uint64_t bytes_copied; // bytes moved (copy backend)
    uint64_t bytes_zeroed; // bytes cleared (copy backend)
} VramStats;

typedef struct Vram Vram;

// The size of the memory the views live in, and the alignment it needs for a backend: the host page for the aliasing
// backends, which map pages into it, and 8 bytes for copying, which only needs every access game code makes to be
// aligned. A caller may pass its own region (a static array, so that the views have link-time addresses); otherwise
// the Vram reserves or allocates it
size_t vram_region_size(void);
size_t vram_region_alignment(VramBackend backend);

// The alignment to declare a static region with. COFF (Windows) can't align a static object to more than 8 KiB
// (clang: "requested alignment must be 8192 bytes or smaller"), and Windows can't map pages over an executable's own
// memory anyway, so there a static region is only for the copy backend
#if defined(_WIN32)
#define VRAM_STATIC_REGION_ALIGN 8
#else
#define VRAM_STATIC_REGION_ALIGN VRAM_SLOT_SIZE
#endif

// Returns NULL and sets errno when the backend can't run here (ENOTSUP: another OS, or host pages larger than 16 KiB)
Vram *vram_create(VramBackend backend, unsigned flags, void *region);
void vram_destroy(Vram *vram);

// The base of a view. It stays the same for the Vram's life, whatever VRAMCNT says
uint8_t *vram_view(Vram *vram, VramView view);

// One VRAMCNT register write (bank A-I), applied before it returns, as on the DS
void vram_set_vramcnt(Vram *vram, VramBank bank, uint8_t value);
// All nine at once (a save state load)
void vram_set_all_vramcnt(Vram *vram, const uint8_t vramcnt[VRAM_BANK_COUNT]);
uint8_t vram_get_vramcnt(const Vram *vram, VramBank bank);
const VramLayout *vram_layout(const Vram *vram);

// Where a slot's bytes are for the renderer and capture: NULL for a slot with no bank. Mirrors resolve to the page's
// storage, so this is right for every backend. Reading view + offset directly is right for mirrors only when aliasing,
// and for the GPU-only views only when aliasing: the copy backend keeps their pages outside the views
uint8_t *vram_slot_ptr(Vram *vram, VramView view, int slot);
// Where a bank page's bytes are now (for save states and debugging), whatever its mapping
uint8_t *vram_bank_page(Vram *vram, VramBank bank, int page);

// Bumped each time a bank page leaves or enters a place. Texture and extended palette slots can only be written while
// their bank is in LCDC mode, so a GPU cache keyed on these generations sees every change to them
uint32_t vram_page_generation(const Vram *vram, VramBank bank, int page);

const VramStats *vram_stats(const Vram *vram);
VramBackend vram_backend(const Vram *vram);

#endif // VRAM_H
