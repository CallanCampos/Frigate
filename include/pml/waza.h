#ifndef POKEBW2_PML_WAZA_H
#define POKEBW2_PML_WAZA_H

#include "types.h"
#include "gfl/heap.h"

// Move data

// What PML_MoveGetParam reads. Type, category, power and effect are swan's names
enum {
    MOVE_PARAM_TYPE,
    MOVE_PARAM_QUALITY,
    MOVE_PARAM_CATEGORY,
    MOVE_PARAM_POWER,
    // 100 for a move that never misses
    MOVE_PARAM_ACCURACY,
    MOVE_PARAM_PP,
    MOVE_PARAM_PRIORITY,
    // 0 for a move that always lands a critical hit
    MOVE_PARAM_CRIT_STAGE,
    MOVE_PARAM_HIT_MAX,
    MOVE_PARAM_HIT_MIN,
    MOVE_PARAM_FLINCH_CHANCE,
    // The condition the move inflicts, its chance, and how long it lasts
    MOVE_PARAM_CONDITION,
    MOVE_PARAM_CONDITION_CHANCE,
    MOVE_PARAM_CONDITION_DURATION,
    MOVE_PARAM_CONDITION_MIN_TURNS,
    MOVE_PARAM_CONDITION_MAX_TURNS,
    // The first stat the move changes, by how many stages, and its chance. 19 to 24 are meant for the second and
    // third, but read the wrong entries
    MOVE_PARAM_STAT_CHANGE_STAT,
    MOVE_PARAM_STAT_CHANGE_STAGE,
    MOVE_PARAM_STAT_CHANGE_CHANCE,
    // The share of the damage that the user recovers, and the share of its HP
    MOVE_PARAM_DRAIN = 25,
    MOVE_PARAM_HEAL,
    MOVE_PARAM_TARGET,
    MOVE_PARAM_EFFECT,
    // The share of the damage that the user takes, and the share of its HP that it loses
    MOVE_PARAM_RECOIL = 30,
    MOVE_PARAM_HP_COST,
};

// How long the condition a move inflicts lasts, by its type
typedef struct {
    u16 type : 4;
    u16 min : 6;
    u16 max : 6;
} MoveConditionParam;

u16 PML_MoveGetBasePower(u16 move);
BOOL getMoveFlag(u16 move, u32 flag);
BOOL PML_MoveIsDamaging(u16 move);
s32 PML_MoveGetCategory(u16 move);
s32 PML_MoveGetQuality(u16 move);
u8 PML_MoveGetType(u16 move);
// How many stats the move changes
u8 PML_MoveGetStatChangeStat(u16 move);
// The stat of one of the move's stat changes, 0 if it has none there, and its stages
u32 PML_MoveGetStatChangeStage(u16 move, u32 index, s32 *stage);
u8 PML_MoveGetMaxPP(u16 move, u8 bonus);
s32 PML_MoveGetParam(u16 move, u32 param);
BOOL PML_MoveIsAlwaysHit(u16 move);
BOOL PML_MoveIsAlwaysCrit(u16 move);
MoveConditionParam PML_MoveGetConditionParam(u16 move);
u32 GetMoveWeather(u16 move);
BOOL PML_MoveIsDataCachePresent(void);
void PML_MoveInitDataCache(s32 count, HeapID heapId);
void PML_MoveFreeDataCache(void);

#endif // POKEBW2_PML_WAZA_H
