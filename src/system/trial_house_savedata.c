#include "types.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "save/trial_house.h"

// The save block of the Battle Institute (the trial house): the results of the challenges, and the data of the file
// that follows the battle examination's. The ROM has no string for the file, so the name is descriptive. Block 0x3f.

#define SAVE_BLOCK_TRIAL_HOUSE 0x3f

u32 TrialHouseSave_GetSize(void) {
    return sizeof(TrialHouseSave);
}

void TrialHouseSave_Init(TrialHouseSave *save) {
    MI_CpuFill8(save, 0, sizeof(TrialHouseSave));
    WbtSave_Init(save->unk5C);
}

TrialHouseSave *TrialHouseSave_Get(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRIAL_HOUSE);
}
