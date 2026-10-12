#include "types.h"
#include "field/event_wbt.h"
#include "field/event_wbt_wifi.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/wbt_makematch.h"
#include "field/scrcmd_wbt.h"
#include "field/wbt.h"
#include "field/zone.h"
#include "gfl/overlay.h"
#include "save/bsubway_save.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/wbt_save.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The commands of the Pokémon World Tournament's entrance (plugin 6) that overlay 55 doesn't have

static BSubwayScoreData *func_ov056_021e75c0(FieldScriptEnv *env) {
    return SaveControl_GetBlockPtr(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)), SAVE_BLOCK_BSUBWAY_SCORE);
}

// Awards the Battle Points for winning the tournament, and sets a variable to them
BOOL WbtCmd_AwardBattlePoints(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 *var = ScriptReadVar(vm, env);
    u16 battlePoints = WbtMakeMatch_GetBattlePoints(sys);

    if (battlePoints != 0) {
        BSubwayScoreData_AddBattlePoints(func_ov056_021e75c0(env), battlePoints);
        RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env))), 0x21,
                  battlePoints);
    }
    *var = battlePoints;
    return FALSE;
}

BOOL func_ov056_021e7620(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 tournament = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    if (func_ov055_021e67a0(tournament, func_ov055_021e5cbc(sys)) == TRUE &&
        func_ov055_021e66dc(gameData, tournament) == TRUE) {
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Sets a variable to whether winning the current tournament would open a tournament
BOOL func_ov056_021e7674(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 tournament = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = func_ov055_021e66fc(gameData, func_ov055_021e5ca4(sys), tournament);
    return FALSE;
}

// Sets a variable to whether the player's party meets the tournament's regulation
BOOL WbtCmd_CheckRegulation(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = func_ov055_021e5800(env);
    Regulation *regulation = func_ov055_021e5f78(sys);

    *var = PokeRegulation_CheckPartyCanEnter(regulation, WbtMakeMatch_GetParty(sys, FieldScriptEnv_GetGameData(env))) == 0 ? TRUE : FALSE;
    return FALSE;
}

BOOL func_ov056_021e76f4(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = func_ov055_021e5f38(func_ov055_021e5800(env));
    return FALSE;
}

BOOL func_ov056_021e7710(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *var = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);

    *var = func_ov036_021aece0(gsys, 1, func_ov055_021e5f78(sys), heapId);
    return FALSE;
}

BOOL func_ov056_021e7750(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *var = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    GameEvent *event = func_ov036_021aebf0(gsys, 1, func_ov055_021e5f78(sys), var, heapId);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov056_021e77b0(VM *vm, FieldScriptEnv *env) {
    static const u16 sTable[] = { 29, 48, 30, 49, 31, 50, 32, 51, 33, 52, 34, 53, 35, 54, 36, 55, 37, 48 };
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = func_ov055_021e5800(env);
    u32 value = 48;
    u32 i;

    if (func_ov055_021e5ca4(sys) == 3) {
        value = 60;
    } else {
        u8 regulation = func_ov055_021e5f38(sys);

        for (i = 0; i < NELEMS(sTable); i += 2) {
            if (regulation == sTable[i]) {
                value = sTable[i + 1];
                break;
            }
        }
        if (func_ov055_021e5ca4(sys) == 2) {
            value += 4;
        }
    }
    *var = value;
    return FALSE;
}

BOOL func_ov056_021e7808(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u8 value;

    FieldScriptEnv_GetGameData(env);
    switch (ScriptReadAny(vm, env)) {
    case 0:
    case 2:
    case 3:
    default:
        value = 0;
        break;
    case 1:
        value = 1;
        break;
    }
    func_ov055_021e5cf0(sys, value);
    return FALSE;
}

// Makes the rental party
BOOL WbtCmd_MakeRentalParty(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    u16 placeName = ZoneData_GetPlaceNameID(GetScriptEnvZoneID(env));
    u16 value = ScriptReadAny(vm, env);

    func_ov055_021e5d14(sys, value);
    func_ov055_021e713c(sys, value, playerInfo, placeName);
    return FALSE;
}

// Sets three variables to the Trainers that the save's three entries name, for those with unk8_0 of at least 3
BOOL func_ov056_021e7894(VM *vm, FieldScriptEnv *env) {
    void *save = func_020179f8(FieldScriptEnv_GetGameData(env));
    u16 *var1 = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    u16 *var3 = ScriptReadVar(vm, env);
    u16 index1 = func_0200ff34(save, 0);
    u16 index2 = func_0200ff34(save, 1);
    u16 index3 = func_0200ff34(save, 2);
    WbtTrainers *trainers = func_ov055_021e63e0(FieldScriptEnv_GetHeapID(env));

    *var1 = 0;
    *var2 = 0;
    *var3 = 0;
    if (func_ov055_021e6458(trainers, index1) && func_ov055_021e6470(trainers, index1) >= 3) {
        *var1 = func_ov055_021e6468(trainers, index1);
    }
    if (func_ov055_021e6458(trainers, index2) && func_ov055_021e6470(trainers, index2) >= 3) {
        *var2 = func_ov055_021e6468(trainers, index2);
    }
    if (func_ov055_021e6458(trainers, index3) && func_ov055_021e6470(trainers, index3) >= 3) {
        *var3 = func_ov055_021e6468(trainers, index3);
    }
    func_ov055_021e6438(trainers);
    return FALSE;
}

BOOL func_ov056_021e7988(VM *vm, FieldScriptEnv *env) {
    void *save = func_020179f8(FieldScriptEnv_GetGameData(env));
    u16 index = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = func_0200ff34(save, index);
    return FALSE;
}

BOOL func_ov056_021e79c0(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtMakeMatch_CountDownloads(sys, gameData);
    return FALSE;
}

BOOL func_ov056_021e79f0(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WbtSystem *sys = func_ov055_021e5800(env);

    func_ov055_021e5cd0(sys, ScriptReadAny(vm, env));
    if (func_ov055_021e5ca4(sys) == 3) {
        WbtMakeMatch_LoadDownload(sys, gameData);
    }
    return FALSE;
}

BOOL func_ov056_021e7a30(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = func_ov055_021e5cd4(func_ov055_021e5800(env));
    return FALSE;
}

BOOL func_ov056_021e7a4c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = func_ov055_021e5800(env);

    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtPokeSelect_Create, sys));
    func_ov055_021e5cd8(sys, var);
    return TRUE;
}

BOOL func_ov056_021e7a9c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 *var1 = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    WbtOv326Param2 *param = func_ov055_021e6ac8(heapId, gsys, 2, var1, var2);

    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtDownload_Create, param));
    return TRUE;
}

BOOL func_ov056_021e7b00(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WbtOv326Param *param;

    GSYS_GetField(gsys);
    param = func_ov055_021e6a64(heapId, gsys, ScriptReadVar(vm, env));
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtWinRecord_Create, param));
    return TRUE;
}

// Downloads a tournament
BOOL WbtCmd_Download(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work;

    FieldScriptEnv_GetHeapID(env);
    work = FieldScriptEnv_GetScriptWork(env);
    ScriptReadAny(vm, env);
    ScriptReadVar(vm, env);
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(5), EventWbtWifi_Create, NULL));
    return TRUE;
}
