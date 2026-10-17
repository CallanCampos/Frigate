// From VRAMCNT to the slots of every view (port/spikes/vram-alias, reviewed). The places are GBATEK's ("DS Memory
// Control - VRAM"); the mirrors of the small banks are the ones melonDS maps (GPU.cpp, MapVRAM_FG, MapVRAM_H and
// MapVRAM_I), which the oracle tests compare against. Each VRAMCNT byte holds MST in bits 0-2 (bit 2 unused for A, B,
// H and I), OFS in bits 3-4 and the enable bit 7
#include "hal/gx/vram.h"
#include <stdbool.h>

const uint32_t port_vram_bank_size[PORT_VRAM_BANK_COUNT] = {
    0x20000, 0x20000, 0x20000, 0x20000, 0x10000, 0x4000, 0x4000, 0x8000, 0x4000,
};

// The LCDC offsets of the banks (0x06800000 + 0x20000 * n for A-D, then E 0x06880000 ... I 0x068A0000) in slots
const uint8_t port_vram_bank_first_page[PORT_VRAM_BANK_COUNT] = { 0, 8, 16, 24, 32, 36, 37, 38, 40 };

const uint32_t port_vram_view_size[PORT_VRAM_VIEW_COUNT] = {
    PORT_VRAM_BANKS_SIZE, 0x80000, 0x40000, 0x20000, 0x20000, 0x80000, 0x18000, 0x8000, 0x4000, 0x8000, 0x4000,
};

const uint32_t port_vram_view_ds_address[PORT_VRAM_VIEW_COUNT] = {
    0x06800000, 0x06000000, 0x06400000, 0x06200000, 0x06600000, 0, 0, 0, 0, 0, 0,
};

const char *const port_vram_view_name[PORT_VRAM_VIEW_COUNT] = {
    "LCDC", "BG-A", "OBJ-A", "BG-B", "OBJ-B", "TEX", "TEXPLTT", "BGEXT-A", "OBJEXT-A", "BGEXT-B", "OBJEXT-B",
};

// Puts one page of a bank into one slot. The first bank to claim a slot keeps it
static void place(PortVramLayout *layout, bool placed[PORT_VRAM_PAGE_COUNT], PortVramView view, int slot, int bank,
                  int page) {
    PortVramSlot *s = &layout->slots[view][slot];
    int index = port_vram_bank_first_page[bank] + page;

    if (s->bank != PORT_VRAM_BANK_NONE) {
        layout->overlaps++;
        return;
    }
    s->bank = (uint8_t)bank;
    s->page = (uint8_t)page;
    s->mirror = placed[index];
    placed[index] = true;
}

// Puts a run of a bank's pages into consecutive slots, from its first page
static void place_run(PortVramLayout *layout, bool placed[PORT_VRAM_PAGE_COUNT], PortVramView view, int slot, int bank,
                      int count) {
    for (int i = 0; i < count; i++) {
        place(layout, placed, view, slot + i, bank, i);
    }
}

void port_vram_layout_compute(const uint8_t vramcnt[PORT_VRAM_BANK_COUNT], PortVramLayout *layout) {
    bool placed[PORT_VRAM_PAGE_COUNT] = { false };

    for (int view = 0; view < PORT_VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < PORT_VRAM_MAX_SLOTS; slot++) {
            layout->slots[view][slot] = (PortVramSlot){ PORT_VRAM_BANK_NONE, 0, 0 };
        }
    }
    layout->overlaps = 0;

    for (int bank = 0; bank < PORT_VRAM_BANK_COUNT; bank++) {
        uint8_t cnt = vramcnt[bank];
        // A, B, H and I have a two-bit MST: their bit 2 is unused
        bool shortMst = bank == PORT_VRAM_BANK_A || bank == PORT_VRAM_BANK_B || bank == PORT_VRAM_BANK_H ||
                        bank == PORT_VRAM_BANK_I;
        int mst = cnt & (shortMst ? 3 : 7);
        int ofs = (cnt >> 3) & 3;
        int pages = (int)(port_vram_bank_size[bank] >> PORT_VRAM_SLOT_SHIFT);
        // F and G go to slot OFS.0 + 4 * OFS.1 of a main engine view or of the texture palettes
        int fgBase = (ofs & 1) + ((ofs & 2) << 1);

        // Bit 7 enables the bank; a disabled bank is nowhere and keeps its contents
        if ((cnt & 0x80) == 0) {
            continue;
        }
        if (mst == 0) {
            place_run(layout, placed, PORT_VRAM_VIEW_LCDC, port_vram_bank_first_page[bank], bank, pages);
            continue;
        }
        switch (bank) {
        case PORT_VRAM_BANK_A:
        case PORT_VRAM_BANK_B:
            switch (mst) {
            case 1:
                place_run(layout, placed, PORT_VRAM_VIEW_BG_A, ofs * 8, bank, pages);
                break;
            case 2:
                place_run(layout, placed, PORT_VRAM_VIEW_OBJ_A, (ofs & 1) * 8, bank, pages);
                break;
            case 3:
                place_run(layout, placed, PORT_VRAM_VIEW_TEX, ofs * 8, bank, pages);
                break;
            }
            break;
        case PORT_VRAM_BANK_C:
        case PORT_VRAM_BANK_D:
            // MST 2 gives C and D to the ARM7 as work RAM, which the host doesn't have: the bank is nowhere
            if (mst == 1) {
                place_run(layout, placed, PORT_VRAM_VIEW_BG_A, ofs * 8, bank, pages);
            } else if (mst == 3) {
                place_run(layout, placed, PORT_VRAM_VIEW_TEX, ofs * 8, bank, pages);
            } else if (mst == 4) {
                place_run(layout, placed, bank == PORT_VRAM_BANK_C ? PORT_VRAM_VIEW_BG_B : PORT_VRAM_VIEW_OBJ_B, 0,
                          bank, pages);
            }
            break;
        case PORT_VRAM_BANK_E:
            if (mst == 1) {
                place_run(layout, placed, PORT_VRAM_VIEW_BG_A, 0, bank, pages);
            } else if (mst == 2) {
                place_run(layout, placed, PORT_VRAM_VIEW_OBJ_A, 0, bank, pages);
            } else if (mst == 3) {
                place_run(layout, placed, PORT_VRAM_VIEW_TEXPLTT, 0, bank, pages);
            } else if (mst == 4) {
                // Extended palette slots 0-3 take only E's lower 32 KiB
                place_run(layout, placed, PORT_VRAM_VIEW_BGEXT_A, 0, bank, 2);
            }
            break;
        case PORT_VRAM_BANK_F:
        case PORT_VRAM_BANK_G:
            if (mst == 1 || mst == 2) {
                PortVramView view = mst == 1 ? PORT_VRAM_VIEW_BG_A : PORT_VRAM_VIEW_OBJ_A;
                place(layout, placed, view, fgBase, bank, 0);
                place(layout, placed, view, fgBase + 2, bank, 0);
            } else if (mst == 3) {
                place(layout, placed, PORT_VRAM_VIEW_TEXPLTT, fgBase, bank, 0);
            } else if (mst == 4) {
                // OFS 0 gives extended palette slots 0-1, OFS 1 slots 2-3
                place(layout, placed, PORT_VRAM_VIEW_BGEXT_A, ofs & 1, bank, 0);
            } else if (mst == 5) {
                place(layout, placed, PORT_VRAM_VIEW_OBJEXT_A, 0, bank, 0);
            }
            break;
        case PORT_VRAM_BANK_H:
            if (mst == 1) {
                // 0x06200000, mirrored at 0x06210000
                place_run(layout, placed, PORT_VRAM_VIEW_BG_B, 0, bank, pages);
                place_run(layout, placed, PORT_VRAM_VIEW_BG_B, 4, bank, pages);
            } else if (mst == 2) {
                place_run(layout, placed, PORT_VRAM_VIEW_BGEXT_B, 0, bank, pages);
            }
            break;
        case PORT_VRAM_BANK_I:
            if (mst == 1) {
                // 0x06208000, mirrored at 0x0620C000, 0x06218000 and 0x0621C000
                static const int slots[4] = { 2, 3, 6, 7 };
                for (int i = 0; i < 4; i++) {
                    place(layout, placed, PORT_VRAM_VIEW_BG_B, slots[i], bank, 0);
                }
            } else if (mst == 2) {
                // 0x06600000, mirrored through the whole 128 KiB view
                for (int i = 0; i < 8; i++) {
                    place(layout, placed, PORT_VRAM_VIEW_OBJ_B, i, bank, 0);
                }
            } else if (mst == 3) {
                place(layout, placed, PORT_VRAM_VIEW_OBJEXT_B, 0, bank, 0);
            }
            break;
        }
    }
}
