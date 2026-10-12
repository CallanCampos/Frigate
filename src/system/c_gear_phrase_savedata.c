#include "types.h"
#include "system/pms.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"

// The C-Gear's phrases, at the start of the C-Gear's save block, which c_gear_savedata.c owns: their initial state
// and their accessors. The ROM has no string for the file, so the name is a guess. Block 0x20.

#define SAVE_BLOCK_CGEAR 0x20

void CGearPhrases_Init(PMSData *phrases) {
    func_02029c68(&phrases[0]);
    func_02029bfc(&phrases[1], 0);
    func_02029bfc(&phrases[2], 1);
    func_02029bfc(&phrases[3], 2);
    func_02029bfc(&phrases[4], 3);
}

void *CGearPhrases_GetBlock(SaveControl *save) {
    return getCGearDataBlkAddress(save);
}

void *getCGearDataBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);
}

void CGearPhrases_Get(void *cgear, u32 index, PMSData *sentence) {
    PMSData *phrases = cgear;

    *sentence = phrases[index];
}

void CGearPhrases_Set(void *cgear, u32 index, const PMSData *sentence) {
    PMSData *phrases = cgear;

    phrases[index] = *sentence;
}
