// The script commands of Trainers: the sighting event, the Trainer IDs and battle types, the battles and the music.
// The ROM has no name for the file; scrcmd_trainer.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "battle/trainer_data.h"
#include "field/event_battle.h"
#include "field/event_sound.h"
#include "field/event_trainer_eye.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/scrcmd_trainer.h"
#include "field/trainer_script.h"
#include "field/zone.h"
#include "save/records.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

static void RecordNimbasaStadiumBattle(FieldScriptEnv *env);

BOOL Scrcmd_Trainer_TrainerEyeEventInit(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 index = ScriptReadAny(vm, env);
    TrainerClashSlot *slot;

    if (index == 0) {
        slot = ScriptWork_GetTrainerState(work, 0);
    } else if (index == 1) {
        slot = ScriptWork_GetTrainerState(work, 1);
    }
    slot->eye = EventTrainerEye_CreateData(FieldScriptEnv_GetHeapID(env), fieldWork->field, &slot->data, index);
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerEyeEventStart(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    TrainerClashSlot *slot0 = ScriptWork_GetTrainerState(work, 0);
    TrainerClashSlot *slot1 = ScriptWork_GetTrainerState(work, 1);

    ScriptWork_CallEvent(work, EventTrainerEye_Create(FieldScriptEnv_GetGameSystem(env), slot0->eye, slot1->eye));
    return TRUE;
}

BOOL Scrcmd_Trainer_TrainerEyeGetBattleType(VM *vm, FieldScriptEnv *env) {
    TrainerClashSlot *slot = ScriptWork_GetTrainerState(FieldScriptEnv_GetScriptWork(env), 0);
    u16 *result = ScriptReadVar(vm, env);

    *result = slot->data.kind;
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerEyeGetTrainerID(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    TrainerClashSlot *slot = NULL;

    if (index == 0) {
        slot = ScriptWork_GetTrainerState(work, 0);
    } else if (index == 1) {
        slot = ScriptWork_GetTrainerState(work, 1);
    }
    *result = slot != NULL ? slot->data.trainerId : 0;
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerGetActorID(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 index = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    TrainerClashSlot *slot = NULL;

    if (index == 0) {
        slot = ScriptWork_GetTrainerState(work, 0);
    } else if (index == 1) {
        slot = ScriptWork_GetTrainerState(work, 1);
    }
    *result = slot != NULL ? GetActorUID(slot->data.actor) : 0;
    return FALSE;
}

BOOL Scrcmd_Trainer_ActorGetTrainerID(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetFieldWork(work);
    u16 scrId = ScriptWork_GetSCRID(work);
    u16 *result = ScriptReadVar(vm, env);

    *result = GetNPCTrainerIDFromSCRID(scrId);
    return FALSE;
}

BOOL Scrcmd_Trainer_CallTrainerBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetSCRID(work);
    u16 trainer1 = ScriptReadAny(vm, env);
    u16 trainer2 = ScriptReadAny(vm, env);
    u16 flags = ScriptReadAny(vm, env);
    u8 battleType = getBattleType(trainer1);
    GameSystem *gsys;
    ScriptFieldWork *fieldWork;

    if (trainer2 == 0 && isDoubleBattle(trainer1) == TRUE) {
        trainer2 = trainer1;
    }
    gsys = ScriptWork_GetGameSystem(work);
    fieldWork = ScriptWork_GetFieldWork(work);
    ScriptWork_CallEvent(
        work, EventTrainerBattleCall_Create(gsys, fieldWork->field, battleType, 0, trainer1, trainer2, flags));
    RecordNimbasaStadiumBattle(env);
    return TRUE;
}

BOOL Scrcmd_Trainer_CallTrainerMultiBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetSCRID(work);
    u16 partner = ScriptReadAny(vm, env);
    u16 trainer1 = ScriptReadAny(vm, env);
    u16 trainer2 = ScriptReadAny(vm, env);
    u16 flags = ScriptReadAny(vm, env);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);

    ScriptWork_CallEvent(work,
                         EventTrainerBattleCall_Create(gsys, fieldWork->field, 1, partner, trainer1, trainer2, flags));
    RecordNimbasaStadiumBattle(env);
    return TRUE;
}

// The messages of the Trainer's script: before, after and on defeat, for a single Trainer or either of a pair
BOOL Scrcmd_Trainer_TrainerGetMessageTypes(VM *vm, FieldScriptEnv *env) {
    u16 beforeMsg;
    u16 defeatMsg;
    u16 afterMsg;
    u16 scrId = ScriptWork_GetSCRID(FieldScriptEnv_GetScriptWork(env));
    u16 *before = ScriptReadVar(vm, env);
    u16 *defeat = ScriptReadVar(vm, env);
    u16 *after = ScriptReadVar(vm, env);

    // The original narrows both results to u16 before testing them
    if ((u16)isDoubleBattle(GetNPCTrainerIDFromSCRID(scrId)) == FALSE) {
        beforeMsg = 0;
        defeatMsg = 2;
        afterMsg = 24;
    } else if ((u16)isTrainerScrIdDoublePair2(scrId) == FALSE) {
        beforeMsg = 3;
        defeatMsg = 5;
        afterMsg = 6;
    } else {
        beforeMsg = 7;
        defeatMsg = 9;
        afterMsg = 10;
    }
    *before = beforeMsg;
    *defeat = defeatMsg;
    *after = afterMsg;
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerGetBattleType(VM *vm, FieldScriptEnv *env) {
    u16 trainerId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = getBattleType(trainerId);
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerBGMPlayPush(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 trainerId = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(
        work,
        EventBGMPlayPush_Create(gsys, GetTrainerClassEyeBGMID(TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS))));
    return TRUE;
}

BOOL Scrcmd_Trainer_TrainerClassBGMPlayPush(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 trainerClass = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventBGMPlayPush_Create(gsys, GetTrainerClassEyeBGMID(trainerClass)));
    return TRUE;
}

BOOL Scrcmd_Trainer_TrainerBGMPlay(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 trainerId = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(
        work, EventBGMPlay_Create(gsys, GetTrainerClassEyeBGMID(TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS))));
    return TRUE;
}

BOOL Scrcmd_Trainer_CallTrainerBattleEnd(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(
        work, CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys), FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
    return TRUE;
}

// Whether the Trainer's battle needs healing first (1), or gives an item (2)
BOOL Scrcmd_Trainer_TrainerGetFieldAction(VM *vm, FieldScriptEnv *env) {
    u16 action;
    u16 trainerId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (TrainerData_GetParam(trainerId, TRAINER_PARAM_HEALS)) {
        action = 1;
    } else if (TrainerData_GetParam(trainerId, TRAINER_PARAM_REWARD_ITEM)) {
        action = 2;
    } else {
        action = 0;
    }
    *result = action;
    return FALSE;
}

BOOL Scrcmd_Trainer_TrainerGetPrizeItem(VM *vm, FieldScriptEnv *env) {
    u16 trainerId = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = TrainerData_GetParam(trainerId, TRAINER_PARAM_REWARD_ITEM);
    return FALSE;
}

BOOL Scrcmd_Trainer_CallTradedPokemonBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetSCRID(work);
    u16 trainer1 = ScriptReadAny(vm, env);
    u16 trainer2 = ScriptReadAny(vm, env);
    u16 flags = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u8 battleType = getBattleType(trainer1);
    GameSystem *gsys;
    ScriptFieldWork *fieldWork;

    if (trainer2 == 0 && isDoubleBattle(trainer1) == TRUE) {
        trainer2 = trainer1;
    }
    gsys = ScriptWork_GetGameSystem(work);
    fieldWork = ScriptWork_GetFieldWork(work);
    ScriptWork_CallEvent(
        work, LoadTradedPokemonBattleStats(gsys, fieldWork->field, battleType, 0, trainer1, trainer2, flags, index));
    return TRUE;
}

// Count a battle in the Nimbasa City stadiums
static void RecordNimbasaStadiumBattle(FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    GameRecords *records = GameData_GetRecords(FieldScriptEnv_GetGameData(env));

    if (IsZoneNimbasaStadium(Field_GetPlayerStateZoneID(fieldWork->field))) {
        RecordAddOne(records, 32);
    }
}
