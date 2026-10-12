#include "types.h"
#include "save/player_savedata.h"
#include "field/field_status.h"
#include "field/pair_sys.h"
#include "field/player_state.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block with the player's place, the places to return to, the follower and the step counters. The ROM has no
// string for the file, so the name is a guess. Block 0x1c.

#define SAVE_BLOCK_PLAYER 0x1c
#define STEP_COUNTER_NONE 0xffff
#define STEP_COUNTER_MAX 9999

u32 PlayerSave_GetBlockSize(void) {
    return sizeof(PlayerSave);
}

void PlayerSave_Init(PlayerSave *playerSave) {
    sys_memset(playerSave, 0, sizeof(PlayerSave));
    playerSave->stepCounter = STEP_COUNTER_NONE;
    // The zone whose place name a new game starts with
    playerSave->placeNameZoneId = 0x267;
}

ZoneSpawnInfo *PlayerSave_GetNowSpawnZone(PlayerSave *playerSave) {
    return &playerSave->nowSpawn;
}

ZoneSpawnInfo *PlayerSave_GetNextSpawnZone(PlayerSave *playerSave) {
    return &playerSave->nextSpawn;
}

ZoneSpawnInfo *PlayerSave_GetEscapeRopeZone(PlayerSave *playerSave) {
    return &playerSave->escapeRopeSpawn;
}

u16 *PlayerSave_GetReturnZoneIDPtr(PlayerSave *playerSave) {
    return &playerSave->returnZoneId;
}

PlayerSave *SaveControl_GetPlayerSave(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_PLAYER);
}

void SaveControl_SavePlayerState(SaveControl *save, const PlayerState *state) {
    PlayerSave *playerSave = SaveControl_GetPlayerSave(save);

    playerSave->location.zoneId = state->zoneId;
    playerSave->location.pos = state->position;
    playerSave->location.railPos = state->railPos;
    playerSave->location.unk18 = state->rotation;
    playerSave->location.unk1A = state->isRail;
    playerSave->location.exState = state->exState;
}

void SaveControl_LoadPlayerState(SaveControl *save, PlayerState *state) {
    PlayerSave *playerSave = SaveControl_GetPlayerSave(save);

    state->zoneId = playerSave->location.zoneId;
    state->position = playerSave->location.pos;
    state->railPos = playerSave->location.railPos;
    state->rotation = playerSave->location.unk18;
    state->isRail = playerSave->location.unk1A;
    state->exState = playerSave->location.exState;
}

void func_02008fb8(SaveControl *save, SaveLocation *location) {
    PlayerSave *playerSave = SaveControl_GetPlayerSave(save);
    *location = playerSave->location;
}

void PlayerSave_SaveFieldStatus(PlayerSave *playerSave, FieldStatus *status) {
    playerSave->flashUsed = FieldStatus_CheckFlashUsed(status);
}

void PlayerSave_LoadFieldStatus(PlayerSave *playerSave, FieldStatus *status) {
    FieldStatus_SetFlashUsed(status, playerSave->flashUsed);
}

void PlayerSave_SaveSeason(PlayerSave *playerSave, u8 season) {
    playerSave->season = season;
}

void PlayerSave_LoadSeason(PlayerSave *playerSave, u8 *season) {
    *season = playerSave->season;
}

void PlayerSave_SaveWeather(PlayerSave *playerSave, u8 weather) {
    playerSave->weather = weather;
}

void PlayerSave_LoadWeather(PlayerSave *playerSave, u8 *weather) {
    *weather = playerSave->weather;
}

void PlayerSave_SaveFollowWk(PlayerSave *playerSave, const FieldFollowWk *wk) {
    playerSave->followIndex = wk->index;
    playerSave->followIsPersistent = wk->isPersistent;
    playerSave->followOriginActorId = wk->originActorId;
    playerSave->followObjCode = wk->objCode;
    playerSave->followScrId = wk->scrId;
    playerSave->followTrainerId = wk->trainerId;
}

void PlayerSave_LoadFollowWk(PlayerSave *playerSave, FieldFollowWk *wk) {
    wk->index = playerSave->followIndex;
    wk->isPersistent = playerSave->followIsPersistent;
    wk->originActorId = playerSave->followOriginActorId;
    wk->objCode = playerSave->followObjCode;
    wk->scrId = playerSave->followScrId;
    wk->trainerId = playerSave->followTrainerId;
}

void PlayerSave_SetEggCycleSubsteps(PlayerSave *playerSave, u32 substeps) {
    playerSave->eggCycleSubsteps = substeps;
}

void PlayerSave_GetEggCycleSubsteps(PlayerSave *playerSave, u32 *substeps) {
    *substeps = playerSave->eggCycleSubsteps;
}

u16 PlayerSave_GetHappinesStepCounter(PlayerSave *playerSave) {
    return playerSave->happinessStepCounter;
}

void PlayerSave_SetHappinessStepCounter(PlayerSave *playerSave, u16 count) {
    playerSave->happinessStepCounter = count;
}

void PlayerSave_SetAbyssalRuinsStepCounter(PlayerSave *playerSave, u16 count) {
    playerSave->abyssalRuinsStepCounter = count;
}

u16 PlayerSave_GetAbyssalRuinsStepCounter(PlayerSave *playerSave) {
    return playerSave->abyssalRuinsStepCounter;
}

u16 PlayerSave_GetPlaceNameZoneID(PlayerSave *playerSave) {
    return playerSave->placeNameZoneId;
}

void PlayerSave_SetPlaceNameZoneID(PlayerSave *playerSave, u16 zoneId) {
    playerSave->placeNameZoneId = zoneId;
}

void PlayerSave_IncrementStepCounter(PlayerSave *playerSave) {
    if (playerSave->stepCounter != STEP_COUNTER_NONE && playerSave->stepCounter < STEP_COUNTER_MAX) {
        playerSave->stepCounter++;
    }
}

u16 PlayerSave_GetStepCounter(PlayerSave *playerSave) {
    if (playerSave->stepCounter == STEP_COUNTER_NONE) {
        return 0;
    }
    return playerSave->stepCounter;
}

void PlayerSave_BeginStepCounter(PlayerSave *playerSave) {
    if (playerSave->stepCounter == STEP_COUNTER_NONE) {
        playerSave->stepCounter = 0;
    }
}

void PlayerSave_EndStepCounter(PlayerSave *playerSave) {
    playerSave->stepCounter = STEP_COUNTER_NONE;
}

BOOL PlayerSave_HasStepCounter(PlayerSave *playerSave) {
    if (playerSave->stepCounter == STEP_COUNTER_NONE) {
        return FALSE;
    }
    return TRUE;
}
