#include "types.h"
#include "field/event_wbt.h"
#include "field/event_wbt_wifi.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/scrcmd_wbt.h"
#include "field/wbt.h"
#include "field/wbt_makematch.h"
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

static BSubwayScoreData *WbtEntrance_GetBSubwayScoreData(FieldScriptEnv *env) {
    return SaveControl_GetBlockPtr(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)), SAVE_BLOCK_BSUBWAY_SCORE);
}

// Awards the Battle Points for winning the tournament, and sets a variable to them
BOOL Scrcmd_WbtEntrance_AwardBattlePoints(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u16 *var = ScriptReadVar(vm, env);
    u16 battlePoints = WbtMakeMatch_GetBattlePoints(sys);

    if (battlePoints != 0) {
        BSubwayScoreData_AddBattlePoints(WbtEntrance_GetBSubwayScoreData(env), battlePoints);
        RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env))), 0x21,
                  battlePoints);
    }
    *var = battlePoints;
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_IsTournamentAvailable(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u16 tournament = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    if (WbtTournament_IsInCategory(tournament, WbtSystem_GetCategory(sys)) == TRUE &&
        WbtTournament_IsOpen(gameData, tournament) == TRUE) {
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Sets a variable to whether winning the current tournament would open a tournament
BOOL Scrcmd_WbtEntrance_WouldOpenTournament(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 tournament = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtTournament_WouldOpen(gameData, WbtSystem_GetTournament(sys), tournament);
    return FALSE;
}

// Sets a variable to whether the player's party meets the tournament's regulation
BOOL Scrcmd_WbtEntrance_CheckRegulation(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = WbtCmd_GetSystem(env);
    Regulation *regulation = WbtSystem_GetRegulation(sys);

    *var =
        PokeRegulation_CheckPartyCanEnter(regulation, WbtMakeMatch_GetParty(sys, FieldScriptEnv_GetGameData(env))) == 0
            ? TRUE
            : FALSE;
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_GetRegulationId(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetRegulationId(WbtCmd_GetSystem(env));
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_CheckEntryParties(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *var = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);

    *var = Regulation_CheckTeams(gsys, 1, WbtSystem_GetRegulation(sys), heapId);
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_CallEntryPartySelect(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *var = ScriptReadVar(vm, env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    GameEvent *event = Regulation_CreatePartyCheckTask(gsys, 1, WbtSystem_GetRegulation(sys), var, heapId);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov056_021e77b0(VM *vm, FieldScriptEnv *env) {
    static const u16 sTable[] = { 29, 48, 30, 49, 31, 50, 32, 51, 33, 52, 34, 53, 35, 54, 36, 55, 37, 48 };
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u32 value = 48;
    unsigned int i;

    if (WbtSystem_GetTournament(sys) == 3) {
        value = 60;
    } else {
        u8 regulation = WbtSystem_GetRegulationId(sys);

        for (i = 0; i < NELEMS(sTable); i += 2) {
            if (regulation == sTable[i]) {
                value = sTable[i + 1];
                break;
            }
        }
        if (WbtSystem_GetTournament(sys) == 2) {
            value += 4;
        }
    }
    *var = value;
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_SetUseBattleBox(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
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
    WbtSystem_SetUseBattleBox(sys, value);
    return FALSE;
}

// Makes the rental party
BOOL Scrcmd_WbtEntrance_MakeRentalParty(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));
    u16 placeName = ZoneData_GetPlaceNameID(GetScriptEnvZoneID(env));
    u16 value = ScriptReadAny(vm, env);

    WbtSystem_SetRentalMode(sys, value);
    WbtParty_MakeRentalParty(sys, value, playerInfo, placeName);
    return FALSE;
}

// Sets three variables to the Trainers that the save's three entries name, for those with unk8_0 of at least 3
BOOL Scrcmd_WbtEntrance_GetSlotTrainerObjCodes(VM *vm, FieldScriptEnv *env) {
    WbtSave *save = GameData_GetWbtSave(FieldScriptEnv_GetGameData(env));
    u16 *var1 = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    u16 *var3 = ScriptReadVar(vm, env);
    u16 index1 = WbtSave_GetSlot(save, 0);
    u16 index2 = WbtSave_GetSlot(save, 1);
    u16 index3 = WbtSave_GetSlot(save, 2);
    WbtTrainers *trainers = WbtTrainers_Load(FieldScriptEnv_GetHeapID(env));

    *var1 = 0;
    *var2 = 0;
    *var3 = 0;
    if (WbtTrainers_IsValid(trainers, index1) && WbtTrainers_GetRank(trainers, index1) >= 3) {
        *var1 = WbtTrainers_GetObjCode(trainers, index1);
    }
    if (WbtTrainers_IsValid(trainers, index2) && WbtTrainers_GetRank(trainers, index2) >= 3) {
        *var2 = WbtTrainers_GetObjCode(trainers, index2);
    }
    if (WbtTrainers_IsValid(trainers, index3) && WbtTrainers_GetRank(trainers, index3) >= 3) {
        *var3 = WbtTrainers_GetObjCode(trainers, index3);
    }
    WbtTrainers_Free(trainers);
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_GetSlot(VM *vm, FieldScriptEnv *env) {
    WbtSave *save = GameData_GetWbtSave(FieldScriptEnv_GetGameData(env));
    u16 index = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSave_GetSlot(save, index);
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_CountDownloads(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtMakeMatch_CountDownloads(sys, gameData);
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_SetDownloadSlot(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WbtSystem *sys = WbtCmd_GetSystem(env);

    WbtSystem_SetDownloadSlot(sys, ScriptReadAny(vm, env));
    if (WbtSystem_GetTournament(sys) == 3) {
        WbtMakeMatch_LoadDownload(sys, gameData);
    }
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_GetDownloadSlot(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetDownloadSlot(WbtCmd_GetSystem(env));
    return FALSE;
}

BOOL Scrcmd_WbtEntrance_CallPokeSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = ScriptWork_GetGameSystem(work);
    u16 *var = ScriptReadVar(vm, env);
    WbtSystem *sys = WbtCmd_GetSystem(env);

    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtPokeSelect_Create, sys));
    WbtSystem_SetResultVar(sys, var);
    return TRUE;
}

BOOL Scrcmd_WbtEntrance_ShowDownloads(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 *var1 = ScriptReadVar(vm, env);
    u16 *var2 = ScriptReadVar(vm, env);
    WbtOv326Param2 *param = WbtSetup_CreateDownloadParam(heapId, gsys, 2, var1, var2);

    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtDownload_Create, param));
    return TRUE;
}

BOOL Scrcmd_WbtEntrance_ShowWinRecord(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WbtOv326Param *param;

    GSYS_GetField(gsys);
    param = WbtSetup_CreateWinRecordParam(heapId, gsys, ScriptReadVar(vm, env));
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtWinRecord_Create, param));
    return TRUE;
}

// Downloads a tournament
BOOL Scrcmd_WbtEntrance_Download(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work;

    FieldScriptEnv_GetHeapID(env);
    work = FieldScriptEnv_GetScriptWork(env);
    ScriptReadAny(vm, env);
    ScriptReadVar(vm, env);
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(5), EventWbtWifi_Create, NULL));
    return TRUE;
}
