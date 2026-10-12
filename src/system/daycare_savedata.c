#include "types.h"
#include "save/box.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/save_control.h"

// The day care's save block: its two Pokémon with the steps each has walked, the flag of a waiting egg and the
// seed the egg is made from. The ROM has no string for the file, so the name is a guess (swan's DayCareSave
// functions). Block 0x32.

#define SAVE_BLOCK_DAY_CARE 0x32

static void DayCareSave_GenSeed(DayCareSave *dayCare);

void DayCareSave_SetSpawnFlag(DayCareSave *dayCare) {
    if (dayCare->eggSpawned != TRUE) {
        dayCare->eggSpawned = TRUE;
    }
}

void DayCareSave_ResetSeed(DayCareSave *dayCare) {
    DayCareSave_GenSeed(dayCare);
    dayCare->eggSpawned = FALSE;
}

PartyPkm *DayCareSave_GetPkm(DayCareSave *dayCare, int slot) {
    if (slot < 0 || slot >= DAY_CARE_SLOTS || dayCare->slots[slot].used != TRUE) {
        slot = 0;
    }
    return &dayCare->slots[slot].pkm;
}

void DayCareSave_AddPkm(DayCareSave *dayCare, int slot, const PartyPkm *pkm) {
    if (slot < 0 || slot >= DAY_CARE_SLOTS) {
        return;
    }
    if (dayCare->slots[slot].used == TRUE) {
        return;
    }
    dayCare->slots[slot].pkm = *pkm;
    dayCare->slots[slot].used = TRUE;
    dayCare->slots[slot].steps = 0;
}

void DayCareSave_DisablePkmStatus(DayCareSave *dayCare, int slot) {
    if (slot < 0 || slot >= DAY_CARE_SLOTS) {
        return;
    }
    if (dayCare->slots[slot].used == TRUE) {
        dayCare->slots[slot].used = FALSE;
    }
}

BOOL DayCareSave_CheckSpawnFlag(DayCareSave *dayCare) {
    return dayCare->eggSpawned;
}

BOOL DayCareSave_GetPkmStatus(DayCareSave *dayCare, u32 slot) {
    if (slot >= DAY_CARE_SLOTS) {
        return FALSE;
    }
    return dayCare->slots[slot].used;
}

u32 DayCareSave_GetPkmCount(DayCareSave *dayCare) {
    int i;
    u32 count = 0;

    for (i = 0; i < DAY_CARE_SLOTS; i++) {
        if (dayCare->slots[i].used == TRUE) {
            count++;
        }
    }
    return count;
}

u32 DayCareSave_GetPkmStepCounter(DayCareSave *dayCare, u32 slot) {
    if (slot >= DAY_CARE_SLOTS) {
        return 0;
    }
    return dayCare->slots[slot].steps;
}

u64 DayCareSave_GetSeed(DayCareSave *dayCare) {
    return dayCare->seed;
}

static void DayCareSave_GenSeed(DayCareSave *dayCare) {
    u64 high = GFL_RandomMT();
    u64 low = GFL_RandomMT();

    dayCare->seed = (high << 32) | low;
}

void DayCareSave_SwapSlots(DayCareSave *dayCare, int slot1, int slot2) {
    DayCareSlot temp;

    if (slot1 < 0 || slot1 >= DAY_CARE_SLOTS) {
        return;
    }
    if (slot2 < 0 || slot2 >= DAY_CARE_SLOTS) {
        return;
    }
    temp = dayCare->slots[slot1];
    dayCare->slots[slot1] = dayCare->slots[slot2];
    dayCare->slots[slot2] = temp;
}

void DayCareSave_UpdateStepCount(DayCareSave *dayCare, u32 steps) {
    int i;

    for (i = 0; i < DAY_CARE_SLOTS; i++) {
        if (dayCare->slots[i].used == TRUE) {
            u64 total = (u64)steps + dayCare->slots[i].steps;

            if (total > 0xffffffff) {
                total = 0xffffffff;
            }
            dayCare->slots[i].steps = total;
        }
    }
}

u32 getSizeofBreedingBlock(void) {
    return sizeof(DayCareSave);
}

void setEggArea(DayCareSave *dayCare) {
    sys_memset(dayCare, 0, sizeof(DayCareSave));
    DayCareSave_GenSeed(dayCare);
}

DayCareSave *getDaycareBlockAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_DAY_CARE);
}
