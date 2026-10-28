#include "types.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/pleasure_boat.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

// The script plugin of the Royal Unova (plugin 2), commands from 1000

// What Scrcmd_PleasureBoat_SetTrainerInfo sets
enum {
    PLEASURE_BOAT_TRAINER_INFO_DEFEATED = 8,
};

// Sets up the cruise, if it isn't already, with the state of a flag
static BOOL Scrcmd_PleasureBoat_Create(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    EventWork *eventWork = GameData_GetEventWork(gameData);
    PleasureBoat **boat = GameData_GetPleasureBoatPtr(gameData);
    BOOL flag = EventWork_FlagGet(eventWork, ScriptReadAny(vm, env));

    if (*boat == NULL) {
        *boat = PleasureBoat_Create(flag);
    }
    return FALSE;
}

static BOOL Scrcmd_PleasureBoat_Free(VM *vm, FieldScriptEnv *env) {
    PleasureBoat_Free(GameData_GetPleasureBoatPtr(
        GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))))));
    return FALSE;
}

// Sets a variable to PLEASURE_BOAT_INFO_*
static BOOL Scrcmd_PleasureBoat_GetInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    PleasureBoat **boat = GameData_GetPleasureBoatPtr(
        GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)))));
    u16 info = VM_Read16(vm);
    u16 *var = ScriptReadVar(vm, env);

    *var = PleasureBoat_GetInfo(*boat, info);
    return FALSE;
}

static BOOL Scrcmd_PleasureBoat_AdvanceClock(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    PleasureBoat **boat = GameData_GetPleasureBoatPtr(
        GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)))));
    u16 steps = VM_Read16(vm);
    u16 stopBefore = VM_Read16(vm);

    PleasureBoat_AdvanceClock(*boat, steps, stopBefore);
    return FALSE;
}

// Sets something about a Trainer aboard, of which only PLEASURE_BOAT_TRAINER_INFO_DEFEATED does anything
static BOOL Scrcmd_PleasureBoat_SetTrainerInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    PleasureBoat **boat = GameData_GetPleasureBoatPtr(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 trainer = ScriptReadAny(vm, env);
    u16 info = VM_Read16(vm);
    u16 unused = VM_Read16(vm);
    u16 value = ScriptReadAny(vm, env);

    switch (info) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        break;
    case PLEASURE_BOAT_TRAINER_INFO_DEFEATED:
        PleasureBoat_SetTrainerDefeated(*boat, trainer, value == TRUE);
        break;
    }
    return FALSE;
}

static BOOL Scrcmd_PleasureBoat_StopClock(VM *vm, FieldScriptEnv *env) {
    PleasureBoat_StopClock(*GameData_GetPleasureBoatPtr(
        GSYS_GetGameData(Field_GetGameSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))))));
    return FALSE;
}

const FieldScriptCommand PLEASURE_BOAT_SCRIPT_COMMANDS[] = {
    Scrcmd_PleasureBoat_Create,       Scrcmd_PleasureBoat_Free,           Scrcmd_PleasureBoat_GetInfo,
    Scrcmd_PleasureBoat_AdvanceClock, Scrcmd_PleasureBoat_SetTrainerInfo, Scrcmd_PleasureBoat_StopClock,
    (FieldScriptCommand)0xFFFFFFFF,
};
