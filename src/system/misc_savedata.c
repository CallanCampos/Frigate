#include "types.h"
#include "save/misc_savedata.h"
#include "field/rival_select.h"
#include "field/union_oneself.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/str_tool.h"

// A save block of several small things: the IDs of the trainers the player met last, a set of flags, the rival's
// name and what the Hidden Hollows hold. The ROM has no string for the file, so the name is a guess. Block 0x42.

#define SAVE_BLOCK_MISC 0x42

static const u32 slot_type[] = {
    1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
};

u32 getSizeofHollow_RivalData(void) {
    return sizeof(RivalDataSave);
}

void clearHollowRivalData(RivalDataSave *data) {
    sys_memset(data, 0, sizeof(RivalDataSave));
}

RivalDataSave *getHollow_RivalData(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_MISC);
}

// Puts a trainer at the end of the list of the trainers met last
void Misc_AddMetTrainer(RivalDataSave *rivalData, u32 id) {
    u32 i;

    for (i = 0; i < rivalData->metTrainerCount; i++) {
        if (id == rivalData->metTrainers[i]) {
            int after = rivalData->metTrainerCount - 1 - i;
            if (after > 0) {
                sys_memcpy(&rivalData->metTrainers[i + 1], &rivalData->metTrainers[i], after * sizeof(u32));
                rivalData->metTrainers[rivalData->metTrainerCount - 1] = id;
            }
            return;
        }
    }
    if (rivalData->metTrainerCount < MISC_MET_TRAINER_MAX) {
        rivalData->metTrainers[rivalData->metTrainerCount] = id;
        rivalData->metTrainerCount++;
    } else {
        sys_memcpy(&rivalData->metTrainers[1], &rivalData->metTrainers[0], (MISC_MET_TRAINER_MAX - 1) * sizeof(u32));
        rivalData->metTrainers[MISC_MET_TRAINER_MAX - 1] = id;
        rivalData->metTrainerCount = MISC_MET_TRAINER_MAX;
    }
}

BOOL Misc_IsMetTrainer(RivalDataSave *rival, u32 trainerId) {
    u32 i;

    for (i = 0; i < rival->metTrainerCount; i++) {
        if (trainerId == rival->metTrainers[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL Misc_GetFlag(RivalDataSave *data, int flag) {
    return (data->flags[flag / 32] >> (flag % 32)) & 1;
}

void copyRivalNameIntoHollowBlock(RivalDataSave *data, const u16 *name) {
    wcharsncpy(name, data->rivalName, MISC_RIVAL_NAME_LENGTH);
}

const u16 *getPtrToRivalName(RivalDataSave *data) {
    return data->rivalName;
}

void *func_0200f7d8(RivalDataSave *data) {
    return data->unkB4;
}

void Misc_SetFlag(RivalDataSave *data, int flag) {
    if (flag < MISC_FLAG_COUNT) {
        data->flags[flag / 32] |= 1 << (flag % 32);
    }
}

u32 getTypeOfFilledSlot(RivalDataSave *data, u32 hollow) {
    if (data->hollows[hollow].filled == FALSE) {
        return 0;
    }
    return slot_type[data->hollows[hollow].slot];
}

void setSlot(RivalDataSave *data, u32 hollow, u16 slot) {
    data->hollows[hollow].slot = slot;
}

void setGroup(RivalDataSave *data, u32 hollow, u16 group) {
    data->hollows[hollow].group = group;
}

void setHollowStatus(RivalDataSave *data, u32 hollow, BOOL filled) {
    data->hollows[hollow].filled = filled;
}

BOOL isHollowFilled(RivalDataSave *data, u32 hollow) {
    return data->hollows[hollow].filled;
}

u8 getSlotNumber(RivalDataSave *data, u32 hollow) {
    return data->hollows[hollow].slot;
}

u8 getGroupNumber(RivalDataSave *data, u32 hollow) {
    return data->hollows[hollow].group;
}

u8 getHollowGender(RivalDataSave *data, u32 hollow) {
    return data->hollows[hollow].gender;
}

void setHollowGender(RivalDataSave *data, u32 hollow, u16 gender) {
    data->hollows[hollow].gender = gender;
}

u8 getHollowNum(RivalDataSave *save) {
    return save->hollowNum;
}

void setHollowNum(RivalDataSave *data, u8 num) {
    data->hollowNum = num;
}
