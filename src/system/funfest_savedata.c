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

u16 FestRecords_GetHosted(FunfestSave *save) {
    return save->hosted;
}

u16 FestRecords_AddHosted(FunfestSave *save) {
    if (save->hosted < FUNFEST_COUNT_MAX) {
        save->hosted++;
    }
    return save->hosted;
}

u16 FestRecords_GetJoined(FunfestSave *save) {
    return save->joined;
}

u16 FestRecords_AddJoined(FunfestSave *save) {
    if (save->joined < FUNFEST_COUNT_MAX) {
        save->joined++;
    }
    return save->joined;
}

u16 FestRecords_GetCompleted(FunfestSave *save) {
    return save->completed;
}

void FestRecords_AddCompleted(FunfestSave *save) {
    if (save->completed < FUNFEST_COUNT_MAX) {
        save->completed++;
    }
}

u8 FestRecords_GetMostParticipants(FunfestSave *save) {
    return save->mostParticipants;
}

void FestRecords_UpdateMostParticipants(FunfestSave *save, u32 count) {
    if (count <= FUNFEST_PARTICIPANTS_MAX && save->mostParticipants < count) {
        save->mostParticipants = count;
    }
}

u16 FestRecords_GetTopScores(FunfestSave *save) {
    return save->topScores;
}

void FestRecords_AddTopScore(FunfestSave *save) {
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

u32 FestRecords_GetMissionLevel(FunfestSave *save, u8 mission) {
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

BOOL FestRecords_IsMissionNew(FunfestSave *save, u8 mission) {
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

u8 FestRecords_GetLevelPoints(FunfestSave *save, int index) {
    if (index >= FUNFEST_LEVEL_POINTS_COUNT) {
        return 0;
    }
    return save->levelPoints[index];
}

void FestRecords_SetLevelPoints(FunfestSave *save, int index, u32 value) {
    if (index < FUNFEST_LEVEL_POINTS_COUNT && value < FUNFEST_LEVEL_POINTS_MAX) {
        save->levelPoints[index] = value;
    }
}
