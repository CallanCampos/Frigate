#include "types.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"
#include "system/pms.h"

// The C-Gear's phrases, at the start of the C-Gear's save block, which c_gear_savedata.c owns: their initial state
// and their accessors. The ROM has no string for the file, so the name is a guess. Block 0x20.

void CGearPhrases_Init(PMSData *phrases) {
    PMSData_InitGreeting(&phrases[0]);
    PMSData_InitPreset(&phrases[1], 0);
    PMSData_InitPreset(&phrases[2], 1);
    PMSData_InitPreset(&phrases[3], 2);
    PMSData_InitPreset(&phrases[4], 3);
}

CGearSave *CGearPhrases_GetBlock(SaveControl *save) {
    return getCGearDataBlkAddress(save);
}

CGearSave *getCGearDataBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);
}

void CGearPhrases_Get(CGearSave *cgear, u32 index, PMSData *sentence) {
    *sentence = cgear->phrases[index];
}

void CGearPhrases_Set(CGearSave *cgear, u32 index, const PMSData *sentence) {
    cgear->phrases[index] = *sentence;
}
