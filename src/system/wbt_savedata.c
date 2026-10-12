#include "types.h"
#include "save/wbt_save.h"
#include "gfl/std.h"
#include "save/trial_house.h"

// The Pokémon World Tournament's part of the Trial House's save block: how often each tournament was won, the
// Trainers beaten and three slots. The ROM has no string for the file, so the name is descriptive.

void func_0200fe80(void *data) {
    WbtSave *wbt = data;
    int i;

    sys_memset(wbt, 0, sizeof(WbtSave));
    for (i = 0; i < WBT_SLOT_COUNT; i++) {
        wbt->slots[i] = WBT_SLOT_NONE;
    }
}

void *func_0200fea0(SaveControl *save) {
    return func_0200f1b8(save)->unk5C;
}

u16 func_0200feac(void *save, u32 tournament) {
    WbtSave *wbt = save;

    return wbt->counts[tournament];
}

void func_0200feb4(void *save, u32 tournament) {
    WbtSave *wbt = save;

    if (wbt->counts[tournament] < WBT_COUNT_MAX) {
        wbt->counts[tournament]++;
    }
}

BOOL func_0200fec8(WbtSave *wbt, int trainer) {
    int mask = 1 << (trainer % 8);
    u8 beaten = wbt->beaten[trainer / 8] & mask;

    if (beaten) {
        return TRUE;
    }
    return FALSE;
}

void func_0200fefc(void *save, int trainer) {
    WbtSave *wbt = save;

    wbt->beaten[trainer / 8] |= 1 << (trainer % 8);
}

void func_0200ff28(void *save, u8 index, u16 value) {
    WbtSave *wbt = save;

    wbt->slots[index] = value;
}

u16 func_0200ff34(void *save, u8 index) {
    WbtSave *wbt = save;

    return wbt->slots[index];
}
