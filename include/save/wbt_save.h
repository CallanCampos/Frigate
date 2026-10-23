#ifndef POKEBW2_SAVE_WBT_SAVE_H
#define POKEBW2_SAVE_WBT_SAVE_H

#include "types.h"
#include "save/extra_block_savedata.h"
#include "struct_decls.h"

// The Pokémon World Tournament's save data, from GameData_GetWbtSave. It counts to 9999 for each of 29 tournaments
// (wbt_savedata.c, a descriptive name). It is 0x5c bytes into the Trial House's save block, 0x3f

#define WBT_TOURNAMENT_COUNT 29
#define WBT_COUNT_MAX 9999
#define WBT_SLOT_COUNT 3
#define WBT_SLOT_NONE 0xff

struct WbtSave {
    u16 counts[WBT_TOURNAMENT_COUNT];
    // A bit for each Trainer beaten
    u8 beaten[8];
    u16 slots[WBT_SLOT_COUNT];
    u8 padding48[4];
};

// WbtSave_Init, which clears it, is declared in save/trial_house.h, whose block it is part of
WbtSave *TrialHouseSave_GetWbtSave(SaveControl *save);
u16 WbtSave_GetCount(WbtSave *save, u32 tournament);
void WbtSave_IncrementCount(WbtSave *save, u32 tournament);
// Sets the count of a tournament's slot, and a bit of the table of Trainers beaten
void WbtSave_SetSlot(WbtSave *save, u8 index, u16 value);
BOOL WbtSave_IsTrainerBeaten(WbtSave *wbt, int trainer);
void WbtSave_SetTrainerBeaten(WbtSave *save, int trainer);
u16 WbtSave_GetSlot(WbtSave *save, u8 index);
// Whether the checksum of a downloaded tournament's extra save block holds
BOOL WbtDownloadData_IsCRCValid(WbtDownloadData *download);

#endif // POKEBW2_SAVE_WBT_SAVE_H
