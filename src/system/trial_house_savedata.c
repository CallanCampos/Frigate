#include "types.h"
#include "save/trial_house.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block of the Battle Institute (the trial house): the results of the challenges, and the data of the file
// that follows the battle examination's. The ROM has no string for the file, so the name is descriptive. Block 0x3f.

#define SAVE_BLOCK_TRIAL_HOUSE 0x3f

u32 func_0200f19c(void) {
    return sizeof(TrialHouseSave);
}

void func_0200f1a0(TrialHouseSave *save) {
    sys_memset(save, 0, sizeof(TrialHouseSave));
    func_0200fe80(save->unk5C);
}

TrialHouseSave *func_0200f1b8(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRIAL_HOUSE);
}
