#include "types.h"
#include "save/funfest_savedata.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block of the Funfest missions: how many were hosted, joined and completed, and each mission's record.
// The ROM has no string for the file, so the name is a guess. Block 0x46.

#define SAVE_BLOCK_FUNFEST 0x46
#define FUNFEST_PARTICIPANTS_MAX 100
#define FUNFEST_UNKF8_COUNT 2
#define FUNFEST_UNKF8_MAX 100

u32 func_02010dc0(void) {
    return sizeof(FunfestSave);
}

void func_02010dc4(FunfestSave *save) {
    int i;

    sys_memset(save, 0, sizeof(FunfestSave));
    for (i = 0; i < FUNFEST_MISSION_COUNT; i++) {
        save->missions[i].isNew = TRUE;
    }
}

void *func_02010dec(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_FUNFEST);
}

u16 func_02010df8(void *a0) {
    FunfestSave *save = a0;

    return save->hosted;
}

u16 FestRecords_AddHosted(void *records) {
    FunfestSave *save = records;

    if (save->hosted < FUNFEST_COUNT_MAX) {
        save->hosted++;
    }
    return save->hosted;
}

u16 func_02010e24(void *a0) {
    FunfestSave *save = a0;

    return save->joined;
}

u16 FestRecords_AddJoined(void *records) {
    FunfestSave *save = records;

    if (save->joined < FUNFEST_COUNT_MAX) {
        save->joined++;
    }
    return save->joined;
}

u16 func_02010e50(void *a0) {
    FunfestSave *save = a0;

    return save->completed;
}

void func_02010e58(FunfestSave *save) {
    if (save->completed < FUNFEST_COUNT_MAX) {
        save->completed++;
    }
}

u8 func_02010e78(void *a0) {
    FunfestSave *save = a0;

    return save->mostParticipants;
}

void func_02010e80(FunfestSave *save, u32 count) {
    if (count <= FUNFEST_PARTICIPANTS_MAX && save->mostParticipants < count) {
        save->mostParticipants = count;
    }
}

u16 func_02010e94(void *a0) {
    FunfestSave *save = a0;

    return save->unkF6;
}

void func_02010e9c(FunfestSave *save) {
    if (save->unkF6 + 1 <= FUNFEST_COUNT_MAX) {
        save->unkF6++;
    }
}

u16 func_02010ebc(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].count;
}

BOOL func_02010ed4(FunfestSave *save, u32 mission, u32 count) {
    if (mission >= FUNFEST_MISSION_COUNT || count > FUNFEST_COUNT_MAX) {
        return FALSE;
    }
    if (save->missions[mission].count < count) {
        save->missions[mission].count = count;
        return TRUE;
    }
    return FALSE;
}

u16 func_02010f14(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].best;
}

BOOL func_02010f2c(FunfestSave *save, u32 mission, u32 best) {
    if (mission >= FUNFEST_MISSION_COUNT || best > FUNFEST_COUNT_MAX) {
        return FALSE;
    }
    if (save->missions[mission].best < best) {
        save->missions[mission].best = best;
        return TRUE;
    }
    return FALSE;
}

u32 func_02010f6c(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].level;
}

void func_02010f80(FunfestSave *save, u32 mission, int level) {
    if (mission < FUNFEST_MISSION_COUNT && level < FUNFEST_MISSION_LEVEL_COUNT) {
        if (level > save->missions[mission].level && level < FUNFEST_MISSION_LEVEL_MAX) {
            save->missions[mission].level = level;
        }
    }
}

BOOL func_02010fb0(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return FALSE;
    }
    return save->missions[mission].isNew;
}

void func_02010fc0(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission < FUNFEST_MISSION_COUNT) {
        save->missions[mission].isNew = FALSE;
    }
}

u8 func_02010fd4(FunfestSave *save, int index) {
    if (index >= FUNFEST_UNKF8_COUNT) {
        return 0;
    }
    return save->unkF8[index];
}

void func_02010fe4(FunfestSave *save, int index, u32 value) {
    if (index < FUNFEST_UNKF8_COUNT && value < FUNFEST_UNKF8_MAX) {
        save->unkF8[index] = value;
    }
}
