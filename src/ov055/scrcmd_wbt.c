#include "types.h"
#include "field/event_wbt.h"
#include "field/field_script.h"
#include "field/scrcmd_wbt.h"
#include "field/wbt.h"
#include "field/wbt_makematch.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "save/event_work.h"
#include "save/wbt_save.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The commands of the Pokémon World Tournament's script plugins (plugins 6 and 7), from 1000, that both plugins use

WbtSystem *WbtCmd_GetSystem(FieldScriptEnv *env) {
    return *GameData_GetWbtSystemPtr(FieldScriptEnv_GetGameData(env));
}

// Counts a win of the tournament in the save
static void WbtCmd_IncrementWinCount(WbtSystem *sys, FieldScriptEnv *env) {
    void *save = func_020179f8(FieldScriptEnv_GetGameData(env));
    int record = WbtTournament_GetRecord(WbtSystem_GetTournament(sys), WbtSystem_GetType(sys));

    if (record < 29) {
        WbtSave_IncrementCount(save, record);
    }
}

// The save's count of wins of the tournament, of all tournaments for 0, or of every type for the Type Expert
// Tournament with type 17. It stops at 9999
static u16 WbtCmd_CountWins(FieldScriptEnv *env, int tournament, u8 type) {
    u32 total = 0;
    void *save = func_020179f8(FieldScriptEnv_GetGameData(env));
    u32 start;
    u32 end;
    u32 i;

    if (tournament == 0 || tournament >= 16) {
        start = 0;
        end = 29;
    } else if (tournament == 2) {
        if (type == 17) {
            start = total;
            end = 17;
        } else {
            start = type;
            end = start + 1;
        }
    } else {
        start = WbtTournament_GetRecord(tournament, total);
        end = start + 1;
    }
    for (i = start; i < end; i++) {
        total += WbtSave_GetCount(save, i);
        if (total >= 9999) {
            total = 9999;
            break;
        }
    }
    return total;
}

static BOOL WbtCmd_Create(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    BOOL flag = EventWork_FlagGet(GameData_GetEventWork(gameData), 2441);
    WbtSystem **sys = GameData_GetWbtSystemPtr(gameData);

    *sys = WbtSystem_Create(HEAPID_GAMEEVENT, flag);
    WbtMakeMatch_LoadBattleBoxParty(*sys, gameData);
    return FALSE;
}

static BOOL WbtCmd_Free(VM *vm, FieldScriptEnv *env) {
    WbtSystem **sys = GameData_GetWbtSystemPtr(FieldScriptEnv_GetGameData(env));

    WbtSystem_Free(*sys);
    *sys = NULL;
    return FALSE;
}

static BOOL WbtCmd_IsCreated(VM *vm, FieldScriptEnv *env) {
    WbtSystem **sys = GameData_GetWbtSystemPtr(FieldScriptEnv_GetGameData(env));
    u16 *var = ScriptReadVar(vm, env);

    *var = *sys != NULL ? TRUE : FALSE;
    return FALSE;
}

static BOOL WbtCmd_SetRound(VM *vm, FieldScriptEnv *env) {
    u16 round = ScriptReadAny(vm, env);

    WbtSystem_SetRound(WbtCmd_GetSystem(env), round);
    return FALSE;
}

static BOOL WbtCmd_GetRound(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetRound(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL WbtCmd_MakeMatch(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    WbtSystem_SetPickedParty(sys, gameData);
    WbtMakeMatch_Make(sys, gameData);
    return FALSE;
}

static BOOL WbtCmd_SetStyle(VM *vm, FieldScriptEnv *env) {
    u16 style = ScriptReadAny(vm, env);

    WbtSystem_SetStyle(WbtCmd_GetSystem(env), style);
    return FALSE;
}

static BOOL WbtCmd_GetStyle(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetStyle(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL WbtCmd_SetTournament(VM *vm, FieldScriptEnv *env) {
    u16 tournament = ScriptReadAny(vm, env);

    WbtSystem_SetTournament(WbtCmd_GetSystem(env), tournament);
    return FALSE;
}

static BOOL WbtCmd_GetTournament(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetTournament(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL WbtCmd_SetType(VM *vm, FieldScriptEnv *env) {
    u16 value = ScriptReadAny(vm, env);

    WbtSystem_SetType(WbtCmd_GetSystem(env), value);
    return FALSE;
}

static BOOL WbtCmd_GetType(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetType(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL WbtCmd_SetCategory(VM *vm, FieldScriptEnv *env) {
    u16 value = ScriptReadAny(vm, env);

    WbtSystem_SetCategory(WbtCmd_GetSystem(env), value);
    return FALSE;
}

static BOOL WbtCmd_GetCategory(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetCategory(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL WbtCmd_RecordWin(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);

    WbtSystem_GetRound(sys);
    WbtCmd_IncrementWinCount(sys, env);
    return FALSE;
}

static BOOL WbtCmd_GetWinCount(VM *vm, FieldScriptEnv *env) {
    u16 tournament = ScriptReadAny(vm, env);
    u16 type = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtCmd_CountWins(env, tournament, type);
    return FALSE;
}

static BOOL WbtCmd_GetEntryCount(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetEntryCount(sys);
    return FALSE;
}

// Puts the tournament's name in a word of the script's WordSet
static BOOL WbtCmd_SetTournamentNameWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u8 index = VM_Read8(vm);
    u16 tournament = ScriptReadAny(vm, env);
    StrBuf *strbuf = ScriptWork_GetAltStrBuf(work);

    WbtSystem_LoadTournamentName(sys, tournament, strbuf);
    func_0202437c(wordSet, index, strbuf, 2, 1, 2);
    return FALSE;
}

// Shows the tournament's Trainers
static BOOL WbtCmd_ShowTrainerList(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    HeapID heapId;
    WbtSystem *sys;
    WbtSetup *setup;

    ScriptReadAny(vm, env);
    work = FieldScriptEnv_GetScriptWork(env);
    gsys = ScriptWork_GetGameSystem(work);
    GSYS_GetField(gsys);
    heapId = FieldScriptEnv_GetHeapID(env);
    sys = WbtCmd_GetSystem(env);
    setup = WbtSetup_Create(heapId, gsys);
    WbtSetup_Load(gsys, sys, setup);
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtList_Create, setup));
    return TRUE;
}

const FieldScriptCommand WBT_SCRIPT_COMMANDS[] = {
    WbtCmd_Create,
    WbtCmd_Free,
    WbtCmd_IsCreated,
    WbtCmd_SetRound,
    WbtCmd_GetRound,
    WbtCmd_MakeMatch,
    WbtCmd_IsTournamentAvailable,
    WbtCmd_WouldOpenTournament,
    WbtCmd_CheckRegulation,
    WbtCmd_SetStyle,
    WbtCmd_GetStyle,
    WbtCmd_SetTournament,
    WbtCmd_GetTournament,
    WbtCmd_SetType,
    WbtCmd_GetType,
    WbtCmd_SetCategory,
    WbtCmd_GetCategory,
    WbtCmd_RecordWin,
    WbtCmd_GetWinCount,
    func_ov057_021e7814,
    WbtCmd_AwardBattlePoints,
    NULL,
    WbtCmd_GetSlot,
    WbtCmd_GetSlotTrainerObjCodes,
    WbtCmd_GetEntryCount,
    WbtCmd_CountDownloads,
    WbtCmd_SetDownloadSlot,
    WbtCmd_GetDownloadSlot,
    WbtCmd_SetUseBattleBox,
    func_ov057_021e7850,
    func_ov057_021e7878,
    func_ov057_021e7894,
    func_ov057_021e78a4,
    func_ov057_021e78c8,
    func_ov057_021e78fc,
    func_ov057_021e7948,
    func_ov057_021e79e8,
    func_ov057_021e79ec,
    WbtCmd_MakeRentalParty,
    WbtCmd_MakeOpponentParty,
    func_ov057_021e760c,
    func_ov057_021e7630,
    func_ov057_021e76a8,
    func_ov057_021e770c,
    WbtCmd_SetTournamentNameWord,
    func_ov057_021e777c,
    WbtCmd_CallEntryPartySelect,
    WbtCmd_ShowTrainerList,
    WbtCmd_CallPokeSelect,
    WbtCmd_ShowDownloads,
    WbtCmd_ShowWinRecord,
    WbtCmd_Download,
    func_ov057_021e7b20,
    func_ov057_021e7b60,
    func_ov057_021e7ba0,
    func_ov057_021e7bb4,
    func_ov057_021e7bec,
    WbtCmd_GetRegulationId,
    func_ov056_021e77b0,
    WbtCmd_CheckEntryParties,
    func_ov057_021e79fc,
    (FieldScriptCommand)0xFFFFFFFF,
};
