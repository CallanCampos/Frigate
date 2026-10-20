#include "types.h"
#include "gfl/std.h"
#include "save/trial_house.h"
#include "save/wbt_save.h"

// The Pokémon World Tournament's part of the Trial House's save block: how often each tournament was won, the
// Trainers beaten and three slots. The ROM has no string for the file, so the name is descriptive.

void WbtSave_Init(void *data) {
    WbtSave *wbt = data;
    int i;

    MI_CpuFill8(wbt, 0, sizeof(WbtSave));
    for (i = 0; i < WBT_SLOT_COUNT; i++) {
        wbt->slots[i] = WBT_SLOT_NONE;
    }
}

void *TrialHouseSave_GetWbtSave(SaveControl *save) {
    return TrialHouseSave_Get(save)->unk5C;
}

u16 WbtSave_GetCount(void *save, u32 tournament) {
    WbtSave *wbt = save;

    return wbt->counts[tournament];
}

void WbtSave_IncrementCount(void *save, u32 tournament) {
    WbtSave *wbt = save;

    if (wbt->counts[tournament] < WBT_COUNT_MAX) {
        wbt->counts[tournament]++;
    }
}

BOOL WbtSave_IsTrainerBeaten(WbtSave *wbt, int trainer) {
    int mask = 1 << (trainer % 8);
    u8 beaten = wbt->beaten[trainer / 8] & mask;

    if (beaten) {
        return TRUE;
    }
    return FALSE;
}

void WbtSave_SetTrainerBeaten(void *save, int trainer) {
    WbtSave *wbt = save;

    wbt->beaten[trainer / 8] |= 1 << (trainer % 8);
}

void WbtSave_SetSlot(void *save, u8 index, u16 value) {
    WbtSave *wbt = save;

    wbt->slots[index] = value;
}

u16 WbtSave_GetSlot(void *save, u8 index) {
    WbtSave *wbt = save;

    return wbt->slots[index];
}
