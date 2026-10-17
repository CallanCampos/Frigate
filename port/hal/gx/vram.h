// The DS's nine VRAM banks on the host (PLAN.md §4.5, the reviewed design of port/spikes/vram-alias). VRAMCNT puts each
// bank into one place: the LCDC view the CPU reaches at 0x06800000, a 2D engine's BG or OBJ view, or a slot only the
// GPU reads (texture images, texture palettes, extended palettes). Every place is a view, a block of host memory laid
// out as the DS lays out that place, and VRAMCNT decides which bank page appears in each 16 KiB slot of each view.
//
// The five CPU views sit at their DS offsets inside the window that PORT_HW_ADDR gives region 0x06 (port_hw_mem_06,
// port/hal/hw), so HW_BG_VRAM, HW_LCDC_VRAM_A, G2_GetBG0CharPtr() and the game's own VRAM pointers reach them
// unchanged. Two backends apply a new layout:
//   - aliasing (macOS and Linux): one shared memory object holds the banks, and each slot of a view is a mapping of the
//     bank page it shows, so a write through one view is in the bank wherever it goes next, mirrors included;
//   - copying (everywhere): each bank page lives in one place, and a remap moves the bytes. A mirror slot has bytes of
//     its own, a copy of the page made when the slot takes it: loads and stores through the mirror reach those, so a
//     load matches the DS only until the next store to the page or the mirror, and a store never reaches the bank
//     (the spike's README: of the stores through mirrors, only uniform fills of a whole view match the DS).
// A slot with no bank reads 0 and keeps a stray write only until its mapping changes again, in both backends.
//
// The SDK writes VRAMCNT only in gx_vramcnt.c and gx_state.c (GX_InitGXState); port/hal/gx/vramcnt.c wraps their
// functions and applies the registers after each call (port/cmake/gx.cmake renames the originals). The first use sets
// the views up. Plain C types only, so that the renderers (port/gfx) and tests can include this header without the
// decomp's.
#ifndef PORT_HAL_GX_VRAM_H
#define PORT_HAL_GX_VRAM_H

#include <stddef.h>
#include <stdint.h>

// The unit of every placement: the smallest banks (F, G, I) and every bank offset are multiples of it
#define PORT_VRAM_SLOT_SIZE 0x4000
#define PORT_VRAM_SLOT_SHIFT 14

typedef enum PortVramBank {
    PORT_VRAM_BANK_A,
    PORT_VRAM_BANK_B,
    PORT_VRAM_BANK_C,
    PORT_VRAM_BANK_D,
    PORT_VRAM_BANK_E,
    PORT_VRAM_BANK_F,
    PORT_VRAM_BANK_G,
    PORT_VRAM_BANK_H,
    PORT_VRAM_BANK_I,
    PORT_VRAM_BANK_COUNT,
    PORT_VRAM_BANK_NONE = 0xff,
} PortVramBank;

// The places a bank can be. The first five are the CPU views, at their DS addresses; the rest only the GPU reads, so
// game code never holds pointers into them (the renderers do, through port_vram_slot_ptr)
typedef enum PortVramView {
    PORT_VRAM_VIEW_LCDC,     // 0x06800000, 656 KiB: every bank at a fixed offset when its MST is 0
    PORT_VRAM_VIEW_BG_A,     // 0x06000000, 512 KiB: main engine BG
    PORT_VRAM_VIEW_OBJ_A,    // 0x06400000, 256 KiB: main engine OBJ
    PORT_VRAM_VIEW_BG_B,     // 0x06200000, 128 KiB: sub engine BG
    PORT_VRAM_VIEW_OBJ_B,    // 0x06600000, 128 KiB: sub engine OBJ
    PORT_VRAM_VIEW_TEX,      // texture image slots 0-3 of 128 KiB (2 and 3 also the clear image), 512 KiB
    PORT_VRAM_VIEW_TEXPLTT,  // texture palette slots 0-5 of 16 KiB, 96 KiB
    PORT_VRAM_VIEW_BGEXT_A,  // main BG extended palette slots 0-3 of 8 KiB, 32 KiB
    PORT_VRAM_VIEW_OBJEXT_A, // main OBJ extended palette, 8 KiB used of one 16 KiB slot
    PORT_VRAM_VIEW_BGEXT_B,  // sub BG extended palette slots 0-3, 32 KiB
    PORT_VRAM_VIEW_OBJEXT_B, // sub OBJ extended palette, 8 KiB used of one 16 KiB slot
    PORT_VRAM_VIEW_COUNT,
    PORT_VRAM_CPU_VIEW_COUNT = PORT_VRAM_VIEW_TEX,
} PortVramView;

// The LCDC view has one slot per bank page, so a page's index there names it everywhere
#define PORT_VRAM_PAGE_COUNT 41
#define PORT_VRAM_BANKS_SIZE (PORT_VRAM_PAGE_COUNT * PORT_VRAM_SLOT_SIZE)
#define PORT_VRAM_MAX_SLOTS PORT_VRAM_PAGE_COUNT

// One 16 KiB slot of a view: which bank page is there. A mirror is a further place of a page already placed in the
// same view (F and G twice in a main engine view, H twice and I four or eight times in a sub engine view)
typedef struct PortVramSlot {
    uint8_t bank;
    uint8_t page;
    uint8_t mirror;
} PortVramSlot;

typedef struct PortVramLayout {
    PortVramSlot slots[PORT_VRAM_VIEW_COUNT][PORT_VRAM_MAX_SLOTS];
    // Slots two banks claimed at once, where the DS would OR their reads; the lower bank wins here. The SDK makes these
    // only for a moment inside a GX_SetBankFor* function (the new banks are written before the old ones go back to
    // LCDC), and the wrappers apply after the whole call, so the layouts the game sees have none
    int overlaps;
} PortVramLayout;

extern const uint32_t port_vram_bank_size[PORT_VRAM_BANK_COUNT];
extern const uint8_t port_vram_bank_first_page[PORT_VRAM_BANK_COUNT];
extern const uint32_t port_vram_view_size[PORT_VRAM_VIEW_COUNT];
// The DS address of each CPU view, and 0 for the views the CPU can't reach
extern const uint32_t port_vram_view_ds_address[PORT_VRAM_VIEW_COUNT];
extern const char *const port_vram_view_name[PORT_VRAM_VIEW_COUNT];

// Fills a layout from the nine VRAMCNT bytes (A-I), following GBATEK's "DS Memory Control - VRAM" and the mirrors of
// melonDS, the port's oracle. Plain C; port_vram_layout() is the one in force
void port_vram_layout_compute(const uint8_t vramcnt[PORT_VRAM_BANK_COUNT], PortVramLayout *layout);

static inline int port_vram_view_slot_count(PortVramView view) {
    return (int)(port_vram_view_size[view] >> PORT_VRAM_SLOT_SHIFT);
}

typedef enum PortVramBackend {
    PORT_VRAM_BACKEND_SHM,        // a POSIX shared memory object (memfd on Linux) mapped with mmap(MAP_FIXED)
    PORT_VRAM_BACKEND_MACH_ENTRY, // macOS: a Mach named memory entry mapped with mach_vm_map(VM_FLAGS_OVERWRITE)
    PORT_VRAM_BACKEND_MACH_REMAP, // macOS: pages of an anonymous bank window shared with mach_vm_remap(copy = FALSE)
    PORT_VRAM_BACKEND_COPY,       // anywhere: views are plain memory and a remap moves the bank's bytes
    PORT_VRAM_BACKEND_COUNT,
} PortVramBackend;

// The names the --vram-backend option takes: shm, mach-entry, mach-remap, copy
extern const char *const port_vram_backend_name[PORT_VRAM_BACKEND_COUNT];

// Whether a backend can run on this host: the aliasing ones need pages of 16 KiB or less and a page-aligned window
int port_vram_backend_available(PortVramBackend backend);

// Sets the views up with a backend, every bank empty and in no place, and returns 0, or an errno value (ENOTSUP when
// the backend can't run here). Any earlier setup is shut down first. The HAL calls it the first time a bank is needed,
// with the backend --vram-backend names, or shm where it can run and copying elsewhere; tests call it themselves
int port_vram_init(PortVramBackend backend);
// Gives the views back as plain zeroed memory and frees the banks: the next use sets them up again
void port_vram_shutdown(void);
PortVramBackend port_vram_backend(void);

// Applies the VRAMCNT registers (0x04000240-0x04000249, WRAMCNT at 0x04000247 left out) as the SDK left them. The
// wrappers of port/hal/gx/vramcnt.c call it after each SDK call that writes them
void port_vram_sync(void);
// Applies nine VRAMCNT bytes (A-I) without the registers: save states and tests
void port_vram_set_all_vramcnt(const uint8_t vramcnt[PORT_VRAM_BANK_COUNT]);
uint8_t port_vram_vramcnt(PortVramBank bank);
const PortVramLayout *port_vram_layout(void);

// The base of a view, which stays the same while the setup lasts: a CPU view's is its DS address in the window. A
// GPU-only view has none under copying (NULL), whose pages live elsewhere; read those with port_vram_slot_ptr
uint8_t *port_vram_view(PortVramView view);
// Where a slot's bytes are for the renderers: NULL for a slot with no bank. Mirrors resolve to the page's storage, so
// this is right for every backend, and the only right way to read the GPU-only views
uint8_t *port_vram_slot_ptr(PortVramView view, int slot);
// Where a bank page's bytes are now, whatever its place (save states, debugging)
uint8_t *port_vram_bank_page(PortVramBank bank, int page);

// The host memory a CPU access to a DS address reaches, with the DS's mirrors of the CPU views folded in: BG-A
// every 512 KiB up to 0x06200000, BG-B every 128 KiB, OBJ-A every 256 KiB, OBJ-B every 128 KiB. Anything else (LCDC
// past bank I, other regions) is the address's plain window memory (port_hw_pointer). For the HAL's own copies
void *port_vram_cpu_pointer(uint32_t address);

// Generations, for caches of what the GPU reads. A page's generation changes each time it leaves or enters a place, or
// a write the HAL sees reaches it; a slot's each time the page there changes or a write the HAL sees reaches that
// page. The HAL sees its own loads (GX_LoadTex, GX_LoadTexPltt, the clear image, the main engine's extended palettes)
// and what calls port_vram_note_write; it doesn't see game stores, MI copies or DMA into a CPU view. Those can't reach
// a GPU-only slot: a bank there is in no CPU view, so a bank written by the CPU comes back to a texture or extended
// palette slot only through a remap, which changes the slot's generation. The CPU views' generations don't follow CPU
// stores; the 2D renderer reads them live
uint32_t port_vram_page_generation(PortVramBank bank, int page);
uint32_t port_vram_slot_generation(PortVramView view, int slot);
// A write of size bytes from a DS address, made by the HAL (a load, display capture): bumps the generations of the
// pages it reaches and of their slots
void port_vram_note_write(uint32_t address, uint32_t size);

// The 3D engine's memory for its renderer (phase 2): the texture image space in 32 pages of 16 KiB (GBATEK's slots
// 0-3 are pages 0-7, 8-15, 16-23 and 24-31; the clear image's color is slot 2, its depth slot 3) and the texture
// palette space in 6 pages; NULL where no bank is. Their generations change as port_vram_slot_generation's
#define PORT_VRAM_TEX_PAGES 32
#define PORT_VRAM_TEXPLTT_PAGES 6
const uint8_t *port_vram_tex_page(int page);
const uint8_t *port_vram_texpltt_page(int page);
uint32_t port_vram_tex_generation(int page);
uint32_t port_vram_texpltt_generation(int page);

// Fills a 2D source (port/gfx/ref/ds2d.h) for engine 0 (A) or 1 (B) from the current mapping: the BG and OBJ slots,
// the extended palette slots and, for engine A, the LCDC slots (engine B gets NULL there), and the live register,
// palette and OAM memory of the engine (io, palette, oam). bg0_3d_line is left as it is
struct PortDs2dSource;
void port_vram_fill_source(struct PortDs2dSource *src, int engine);

typedef struct PortVramStats {
    uint32_t remaps;       // applies that changed the layout
    uint32_t os_calls;     // mapping calls made (aliasing backends)
    uint64_t bytes_copied; // bytes moved (copy backend)
    uint64_t bytes_zeroed; // bytes cleared (copy backend)
    uint32_t overlaps;     // applies that left two banks on one slot (the SDK never should, between its calls)
} PortVramStats;
const PortVramStats *port_vram_stats(void);

#endif // PORT_HAL_GX_VRAM_H
