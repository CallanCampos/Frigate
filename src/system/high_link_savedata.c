#include "types.h"
#include "field/survey.h"
#include "gfl/std.h"
#include "save/high_link.h"
#include "save/playtime.h"
#include "save/save_control.h"

// The Entralink's save block: the player's levels, the three pass powers in use, the special pass powers received
// and the time played while a pass power ran. The ROM has no string for the file, so the name is a guess.
// Block 0x35.

#define SAVE_BLOCK_HIGH_LINK 0x35
#define HIGH_LINK_LEVEL_MAX 999

// The play time, in seconds, when the count of HighLinkSave_CountPlaySeconds started
static u64 sStartSeconds;

u32 getSizeofProbabilityTable(void) {
    return sizeof(HighLinkSave);
}

void probability(HighLinkSave *save) {
    int i;

    MI_CpuFill8(save, 0, sizeof(HighLinkSave));
    setupProbabilityTable(&save->levels);
    for (i = 0; i < HIGH_LINK_POWER_MAX; i++) {
        save->powers[i] = HIGH_LINK_POWER_NONE;
    }
}

void setupProbabilityTable(PassPowerLevel *levels) {
    MI_CpuFill8(levels, 0, sizeof(PassPowerLevel));
    probabilityLoop((SurveyProbabilityState *)levels);
}

u16 PassPowerLevel_GetWhite(void *levels) {
    return ((PassPowerLevel *)levels)->level1;
}

u16 PassPowerLevel_GetBlack(void *levels) {
    return ((PassPowerLevel *)levels)->level2;
}

// Adds to the White level, up to 999, and returns how much was added
int HighLinkSave_AddWhiteLevel(PassPowerLevel *levels, int add) {
    u16 old = levels->level1;

    if (old + add > HIGH_LINK_LEVEL_MAX) {
        levels->level1 = HIGH_LINK_LEVEL_MAX;
    } else {
        levels->level1 += add;
    }
    return levels->level1 - old;
}

int HighLinkSave_AddBlackLevel(PassPowerLevel *levels, int add) {
    u16 old = levels->level2;

    if (old + add > HIGH_LINK_LEVEL_MAX) {
        levels->level2 = HIGH_LINK_LEVEL_MAX;
    } else {
        levels->level2 += add;
    }
    return levels->level2 - old;
}

HighLinkSave *getHighLinkBlockAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_HIGH_LINK);
}

void HighLinkSave_SetLevels(SaveControl *save, const PassPowerLevel *levels) {
    getHighLinkBlockAddress(save)->levels = *levels;
}

void HighLinkSave_GetLevels(SaveControl *save, PassPowerLevel *levels) {
    *levels = getHighLinkBlockAddress(save)->levels;
}

void HighLinkSave_SetPassPower(HighLinkSave *save, u32 passPower, int index) {
    if (index < HIGH_LINK_POWER_MAX) {
        save->powers[index] = passPower;
    }
}

u32 HighLinkSave_GetPassPower(HighLinkSave *save, int index) {
    if (index >= HIGH_LINK_POWER_MAX) {
        return HIGH_LINK_POWER_NONE;
    }
    if (!PassPower_IsIDInvalid(save->powers[index])) {
        return save->powers[index];
    }
    save->powers[index] = HIGH_LINK_POWER_NONE;
    return save->powers[index];
}

void HighLinkSave_SetSpecialPowerFlag(HighLinkSave *save, u32 id) {
    int bit = PassPower_GetSPowerBaseID(id);

    if (bit != 0xff) {
        save->specialFlags[bit / 8] |= 1 << (bit % 8);
    }
}

void HighLinkSave_GetSpecialFlags(HighLinkSave *save, u8 *dest, u32 a2) {
    int i;

    for (i = 0; i < 2; i++) {
        dest[i] = save->specialFlags[i];
    }
}

u64 HighLinkSave_CountPlaySeconds(HighLinkSave *save, PlayTime *playTime, u32 mode) {
    u64 hours = PlayTime_GetHours(playTime);
    u64 minutes = PlayTime_GetMinutes(playTime);
    u64 seconds = PlayTime_GetSeconds(playTime);
    u64 now = minutes * 60 + hours * 3600 + seconds;

    switch (mode) {
    case 0:
        sStartSeconds = now;
        break;
    case 1:
        save->seconds += now - sStartSeconds;
        break;
    case 2:
        return save->seconds + (now - sStartSeconds);
    }
    return save->seconds;
}
