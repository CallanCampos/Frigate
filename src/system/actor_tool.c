#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "system/actor_tool.h"
#include "system/palanm.h"

// Tools for actors: a work that hands out runs of the OBJ palette slots of each engine, and loads palettes into them,
// also into a palette fade's buffer. The names are ours

#define PAL_SLOT_FREE 0xff

struct ActorPalSlots {
    u32 unk0;
    // How many slots of each engine it hands out
    u8 count[2];
    // The first slot of the run each slot belongs to, or PAL_SLOT_FREE
    u8 slots[2][16];
};

static int ActorPalSlots_Alloc(ActorPalSlots *slots, int count, BOOL sub);
static void ActorPalSlots_Free(ActorPalSlots *slots, int first, BOOL sub);
static u32 ActorTool_LoadPalettes(ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count, u32 heapId);
static u32 ActorTool_LoadPalette(ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count, u32 heapId);
static u32 ActorTool_GetPalSlot(ActorPalSlots *slots, u32 palette, BOOL sub);

ActorPalSlots *ActorTool_CreatePalSlots(HeapID heapId, u8 mainCount, u8 subCount) {
    ActorPalSlots *slots = GFL_HeapAllocate(heapId, sizeof(ActorPalSlots), TRUE, "actor_tool.c", 81);
    int i, j;

    slots->count[0] = mainCount;
    slots->count[1] = subCount;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 16; j++) {
            slots->slots[i][j] = PAL_SLOT_FREE;
        }
    }
    return slots;
}

void ActorTool_DeletePalSlots(ActorPalSlots *slots) {
    GFL_HeapFree(slots);
}

static int ActorPalSlots_Alloc(ActorPalSlots *slots, int count, BOOL sub) {
    int engine = sub ? 1 : 0;
    int i, j;

    for (i = 0; i < slots->count[engine]; i++) {
        if (slots->slots[engine][i] == PAL_SLOT_FREE) {
            for (j = i; j < i + count; j++) {
                if (slots->slots[engine][j] != PAL_SLOT_FREE) {
                    i = j;
                    break;
                }
            }
            if (j == i + count) {
                for (j = i; j < i + count; j++) {
                    slots->slots[engine][j] = i;
                }
                return i;
            }
        }
    }
    return 0;
}

static void ActorPalSlots_Free(ActorPalSlots *slots, int first, BOOL sub) {
    int engine = sub ? 1 : 0;
    unsigned int i;

    for (i = first; first == slots->slots[engine][i]; i++) {
        slots->slots[engine][i] = PAL_SLOT_FREE;
    }
}

static u32 ActorTool_LoadPalettes(ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count, u32 heapId) {
    int first = ActorPalSlots_Alloc(slots, count, sub);

    return ClActSys_LoadPlttRange(arc, fileId, sub, first * 32, 0, count, heapId);
}

static u32 ActorTool_LoadPalette(ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count, u32 heapId) {
    int first = ActorPalSlots_Alloc(slots, count, sub);

    return ClActSys_LoadPlttEx(arc, fileId, sub, first * 32, heapId);
}

u32 ActorTool_LoadPalettesFade(PaletteFade *fade, ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count,
                               u32 heapId) {
    u16 vram;
    u32 palette;

    if (sub == FALSE) {
        vram = PALFADE_VRAM_MAIN_OBJ;
    } else {
        vram = PALFADE_VRAM_SUB_OBJ;
    }
    palette = ActorTool_LoadPalettes(slots, arc, fileId, sub, count, heapId);
    PaletteFade_LoadFromVRAM(fade, vram, ActorTool_GetPalSlot(slots, palette, sub) * 16, count * 32);
    return palette;
}

u32 ActorTool_LoadPaletteFade(PaletteFade *fade, ActorPalSlots *slots, ArcTool *arc, u32 fileId, BOOL sub, u32 count,
                              u32 heapId) {
    u16 vram;
    u32 palette;

    if (sub == FALSE) {
        vram = PALFADE_VRAM_MAIN_OBJ;
    } else {
        vram = PALFADE_VRAM_SUB_OBJ;
    }
    palette = ActorTool_LoadPalette(slots, arc, fileId, sub, count, heapId);
    PaletteFade_LoadFromVRAM(fade, vram, ActorTool_GetPalSlot(slots, palette, sub) * 16, count * 32);
    return palette;
}

static u32 ActorTool_GetPalSlot(ActorPalSlots *slots, u32 palette, BOOL sub) {
    return ClActSys_GetPlttVramAddr(palette, sub) / 32;
}

void ActorTool_FreePalette(ActorPalSlots *slots, u32 palette, BOOL sub) {
    ActorPalSlots_Free(slots, ActorTool_GetPalSlot(slots, palette, sub), sub);
    ClActSys_FreePltt(palette);
}
