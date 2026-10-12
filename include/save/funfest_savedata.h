#ifndef POKEBW2_SAVE_FUNFEST_SAVEDATA_H
#define POKEBW2_SAVE_FUNFEST_SAVEDATA_H

#include "types.h"

// Block 0x46, the Funfest missions' records (funfest_savedata.c, a descriptive name). The block's getter and the
// functions that read it for the Unova Link's records and the mission list are declared in save/save_control.h,
// where they take the block as a void *

#define FUNFEST_MISSION_COUNT 55
// The levels of a mission, of which the record keeps none above FUNFEST_MISSION_LEVEL_MAX - 1
#define FUNFEST_MISSION_LEVEL_COUNT 5
#define FUNFEST_MISSION_LEVEL_MAX 4
#define FUNFEST_COUNT_MAX 9999

// A mission's record
typedef struct {
    // The two values that func_02010ebc and func_02010f14 give, which only grow
    u32 count : 14;
    u32 best : 14;
    // The highest level cleared, up to FUNFEST_MISSION_LEVEL_MAX - 1
    u32 level : 3;
    u32 isNew : 1;
} FunfestMissionRecord;

typedef struct FunfestSave {
    FunfestMissionRecord missions[FUNFEST_MISSION_COUNT];
    u8 unkDC[0x14];
    // Missions hosted, joined and completed
    u16 hosted;
    u16 joined;
    u16 completed;
    u16 unkF6;
    u8 unkF8[2];
    // The most participants of a mission, up to 100
    u8 mostParticipants;
    u8 paddingFB;
} FunfestSave; // size: 0xfc

// The block's size and its initial state, with every mission new
u32 func_02010dc0(void);
void func_02010dc4(FunfestSave *save);
// Count one more mission completed, up to 9999
void func_02010e58(FunfestSave *save);
// Keeps the most participants, up to 100
void func_02010e80(FunfestSave *save, u32 count);
// Adds one to the count that func_02010e94 gives, up to 9999
void func_02010e9c(FunfestSave *save);
// Keep a mission's count and best value, up to 9999, if they are more than the record's: TRUE when they were kept
BOOL func_02010ed4(FunfestSave *save, u32 mission, u32 count);
BOOL func_02010f2c(FunfestSave *save, u32 mission, u32 best);
// Keeps the level cleared of a mission if it is higher than the record's
void func_02010f80(FunfestSave *save, u32 mission, int level);
// A byte for each of two slots, up to 99
u8 func_02010fd4(FunfestSave *save, int index);
void func_02010fe4(FunfestSave *save, int index, u32 value);

#endif // POKEBW2_SAVE_FUNFEST_SAVEDATA_H
