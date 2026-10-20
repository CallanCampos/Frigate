#include "types.h"
#include "gfl/std.h"
#include "save/funfest_savedata.h"
#include "save/save_control.h"

// The save block of the Funfest missions: how many were hosted, joined and completed, and each mission's record.
// The ROM has no string for the file, so the name is a guess. Block 0x46.

#define SAVE_BLOCK_FUNFEST 0x46
#define FUNFEST_PARTICIPANTS_MAX 100
#define FUNFEST_LEVEL_POINTS_COUNT 2
#define FUNFEST_LEVEL_POINTS_MAX 100

u32 FestRecords_GetSize(void) {
    return sizeof(FunfestSave);
}

void FestRecords_Init(FunfestSave *save) {
    int i;

    MI_CpuFill8(save, 0, sizeof(FunfestSave));
    for (i = 0; i < FUNFEST_MISSION_COUNT; i++) {
        save->missions[i].isNew = TRUE;
    }
}

void *FestRecords_GetFromSave(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_FUNFEST);
}

u16 FestRecords_GetHosted(void *a0) {
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

u16 FestRecords_GetJoined(void *a0) {
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

u16 FestRecords_GetCompleted(void *a0) {
    FunfestSave *save = a0;

    return save->completed;
}

void FestRecords_AddCompleted(FunfestSave *save) {
    if (save->completed < FUNFEST_COUNT_MAX) {
        save->completed++;
    }
}

u8 FestRecords_GetMostParticipants(void *a0) {
    FunfestSave *save = a0;

    return save->mostParticipants;
}

void FestRecords_UpdateMostParticipants(FunfestSave *save, u32 count) {
    if (count <= FUNFEST_PARTICIPANTS_MAX && save->mostParticipants < count) {
        save->mostParticipants = count;
    }
}

u16 func_02010e94(void *a0) {
    FunfestSave *save = a0;

    return save->topScores;
}

void func_02010e9c(FunfestSave *save) {
    if (save->topScores + 1 <= FUNFEST_COUNT_MAX) {
        save->topScores++;
    }
}

u16 FestRecords_GetMissionCount(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].count;
}

BOOL FestRecords_UpdateMissionCount(FunfestSave *save, u32 mission, u32 count) {
    if (mission >= FUNFEST_MISSION_COUNT || count > FUNFEST_COUNT_MAX) {
        return FALSE;
    }
    if (save->missions[mission].count < count) {
        save->missions[mission].count = count;
        return TRUE;
    }
    return FALSE;
}

u16 FestRecords_GetMissionBest(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].best;
}

BOOL FestRecords_UpdateMissionBest(FunfestSave *save, u32 mission, u32 best) {
    if (mission >= FUNFEST_MISSION_COUNT || best > FUNFEST_COUNT_MAX) {
        return FALSE;
    }
    if (save->missions[mission].best < best) {
        save->missions[mission].best = best;
        return TRUE;
    }
    return FALSE;
}

u32 FestRecords_GetMissionLevel(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return 0;
    }
    return save->missions[mission].level;
}

void FestRecords_UpdateMissionLevel(FunfestSave *save, u32 mission, int level) {
    if (mission < FUNFEST_MISSION_COUNT && level < FUNFEST_MISSION_LEVEL_COUNT) {
        if (level > save->missions[mission].level && level < FUNFEST_MISSION_LEVEL_MAX) {
            save->missions[mission].level = level;
        }
    }
}

BOOL FestRecords_IsMissionNew(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission >= FUNFEST_MISSION_COUNT) {
        return FALSE;
    }
    return save->missions[mission].isNew;
}

void FestRecords_ClearMissionNew(void *records, u8 mission) {
    FunfestSave *save = records;

    if (mission < FUNFEST_MISSION_COUNT) {
        save->missions[mission].isNew = FALSE;
    }
}

u8 func_02010fd4(FunfestSave *save, int index) {
    if (index >= FUNFEST_LEVEL_POINTS_COUNT) {
        return 0;
    }
    return save->levelPoints[index];
}

void func_02010fe4(FunfestSave *save, int index, u32 value) {
    if (index < FUNFEST_LEVEL_POINTS_COUNT && value < FUNFEST_LEVEL_POINTS_MAX) {
        save->levelPoints[index] = value;
    }
}
