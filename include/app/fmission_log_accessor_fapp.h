#ifndef POKEBW2_APP_FMISSION_LOG_ACCESSOR_FAPP_H
#define POKEBW2_APP_FMISSION_LOG_ACCESSOR_FAPP_H

// What the Funfest mission screen (overlay 309) takes from the festival: the last lines of the mission's log and the
// mission's result, named after the ROM's "fmission_log_accessor_fapp.c"

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/fest_mission_beacon.h"

#define FMISSION_LOG_FAPP_COUNT 3

// A line of the log: a player and the score
typedef struct {
    StrBuf *name;
    u16 score;
    u16 trainerId;
} FMissionLogFAppEntry;

// The log's last lines, the newest first
typedef struct {
    int count;
    FMissionLogFAppEntry entries[FMISSION_LOG_FAPP_COUNT];
} FMissionLogFApp;

// How a mission ended, and what the player gets for it
typedef struct {
    u8 cleared;
    // Which of the Entralink's levels the mission raises, and by how much it did
    u8 isBlack;
    u8 levels;
    // Whether the mission's records in the save were beaten
    u8 newCount : 1;
    u8 newBest : 1;
    u32 total;
    u16 seconds;
    // The reward and how many of it the mission gives, and how many more the score adds
    u16 item;
    u16 itemCount;
    u16 bonusCount;
    // The second reward
    u16 item2;
    u16 item2Count;
} FMissionAppResult;

FMissionLogFApp *FMissionLogFApp_Create(HeapID heapId);
void FMissionLogFApp_Free(FMissionLogFApp *accessor);
// Reads the log's last lines; FALSE when the log is empty
BOOL FMissionLogFApp_Read(const FestMissionLog *log, FMissionLogFApp *accessor);
// Ends the mission: fills result, and keeps its records, medals and Entralink level in the save
void FMissionLogFApp_EndMission(LinkFestival *festival, FMissionAppResult *result);

#endif // POKEBW2_APP_FMISSION_LOG_ACCESSOR_FAPP_H
