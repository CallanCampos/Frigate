#include "types.h"
#include "gfl/std.h"
#include "save/trial_house.h"
#include "save/wbt_save.h"

// The Pokémon World Tournament's part of the Trial House's save block: how often each tournament was won, the
// Trainers beaten and three slots. The ROM has no string for the file, so the name is descriptive.

void WbtSave_Init(WbtSave *wbt) {
    int i;

    MI_CpuFill8(wbt, 0, sizeof(WbtSave));
    for (i = 0; i < WBT_SLOT_COUNT; i++) {
        wbt->slots[i] = WBT_SLOT_NONE;
    }
}

WbtSave *TrialHouseSave_GetWbtSave(SaveControl *save) {
    return &TrialHouseSave_Get(save)->wbt;
}

u16 WbtSave_GetCount(WbtSave *wbt, u32 tournament) {
    return wbt->counts[tournament];
}

void WbtSave_IncrementCount(WbtSave *wbt, u32 tournament) {
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

void WbtSave_SetTrainerBeaten(WbtSave *wbt, int trainer) {
    wbt->beaten[trainer / 8] |= 1 << (trainer % 8);
}

void WbtSave_SetSlot(WbtSave *wbt, u8 index, u16 value) {
    wbt->slots[index] = value;
}

u16 WbtSave_GetSlot(WbtSave *wbt, u8 index) {
    return wbt->slots[index];
}
