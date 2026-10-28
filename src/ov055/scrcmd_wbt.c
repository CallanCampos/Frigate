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
    WbtSave *save = GameData_GetWbtSave(FieldScriptEnv_GetGameData(env));
    int record = WbtTournament_GetRecord(WbtSystem_GetTournament(sys), WbtSystem_GetType(sys));

    if (record < 29) {
        WbtSave_IncrementCount(save, record);
    }
}

// The save's count of wins of the tournament, of all tournaments for 0, or of every type for the Type Expert
// Tournament with type 17. It stops at 9999
static u16 WbtCmd_CountWins(FieldScriptEnv *env, int tournament, u8 type) {
    u32 total = 0;
    WbtSave *save = GameData_GetWbtSave(FieldScriptEnv_GetGameData(env));
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

static BOOL Scrcmd_Wbt_Create(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    BOOL flag = EventWork_FlagGet(GameData_GetEventWork(gameData), 2441);
    WbtSystem **sys = GameData_GetWbtSystemPtr(gameData);

    *sys = WbtSystem_Create(HEAPID_GAMEEVENT, flag);
    WbtMakeMatch_LoadBattleBoxParty(*sys, gameData);
    return FALSE;
}

static BOOL Scrcmd_Wbt_Free(VM *vm, FieldScriptEnv *env) {
    WbtSystem **sys = GameData_GetWbtSystemPtr(FieldScriptEnv_GetGameData(env));

    WbtSystem_Free(*sys);
    *sys = NULL;
    return FALSE;
}

static BOOL Scrcmd_Wbt_IsCreated(VM *vm, FieldScriptEnv *env) {
    WbtSystem **sys = GameData_GetWbtSystemPtr(FieldScriptEnv_GetGameData(env));
    u16 *var = ScriptReadVar(vm, env);

    *var = *sys != NULL ? TRUE : FALSE;
    return FALSE;
}

static BOOL Scrcmd_Wbt_SetRound(VM *vm, FieldScriptEnv *env) {
    u16 round = ScriptReadAny(vm, env);

    WbtSystem_SetRound(WbtCmd_GetSystem(env), round);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetRound(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetRound(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Wbt_MakeMatch(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    WbtSystem_SetPickedParty(sys, gameData);
    WbtMakeMatch_Make(sys, gameData);
    return FALSE;
}

static BOOL Scrcmd_Wbt_SetStyle(VM *vm, FieldScriptEnv *env) {
    u16 style = ScriptReadAny(vm, env);

    WbtSystem_SetStyle(WbtCmd_GetSystem(env), style);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetStyle(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetStyle(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Wbt_SetTournament(VM *vm, FieldScriptEnv *env) {
    u16 tournament = ScriptReadAny(vm, env);

    WbtSystem_SetTournament(WbtCmd_GetSystem(env), tournament);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetTournament(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetTournament(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Wbt_SetType(VM *vm, FieldScriptEnv *env) {
    u16 value = ScriptReadAny(vm, env);

    WbtSystem_SetType(WbtCmd_GetSystem(env), value);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetType(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetType(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Wbt_SetCategory(VM *vm, FieldScriptEnv *env) {
    u16 value = ScriptReadAny(vm, env);

    WbtSystem_SetCategory(WbtCmd_GetSystem(env), value);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetCategory(VM *vm, FieldScriptEnv *env) {
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetCategory(WbtCmd_GetSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Wbt_RecordWin(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);

    WbtSystem_GetRound(sys);
    WbtCmd_IncrementWinCount(sys, env);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetWinCount(VM *vm, FieldScriptEnv *env) {
    u16 tournament = ScriptReadAny(vm, env);
    u16 type = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtCmd_CountWins(env, tournament, type);
    return FALSE;
}

static BOOL Scrcmd_Wbt_GetEntryCount(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtSystem_GetEntryCount(sys);
    return FALSE;
}

// Puts the tournament's name in a word of the script's WordSet
static BOOL Scrcmd_Wbt_SetTournamentNameWord(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    WbtSystem *sys = WbtCmd_GetSystem(env);
    u8 index = VM_Read8(vm);
    u16 tournament = ScriptReadAny(vm, env);
    StrBuf *strbuf = ScriptWork_GetAltStrBuf(work);

    WbtSystem_LoadTournamentName(sys, tournament, strbuf);
    GFL_WordSetSetStrbuf(wordSet, index, strbuf, 2, 1, 2);
    return FALSE;
}

// Shows the tournament's Trainers
static BOOL Scrcmd_Wbt_ShowTrainerList(VM *vm, FieldScriptEnv *env) {
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
    Scrcmd_Wbt_Create,
    Scrcmd_Wbt_Free,
    Scrcmd_Wbt_IsCreated,
    Scrcmd_Wbt_SetRound,
    Scrcmd_Wbt_GetRound,
    Scrcmd_Wbt_MakeMatch,
    Scrcmd_WbtEntrance_IsTournamentAvailable,
    Scrcmd_WbtEntrance_WouldOpenTournament,
    Scrcmd_WbtEntrance_CheckRegulation,
    Scrcmd_Wbt_SetStyle,
    Scrcmd_Wbt_GetStyle,
    Scrcmd_Wbt_SetTournament,
    Scrcmd_Wbt_GetTournament,
    Scrcmd_Wbt_SetType,
    Scrcmd_Wbt_GetType,
    Scrcmd_Wbt_SetCategory,
    Scrcmd_Wbt_GetCategory,
    Scrcmd_Wbt_RecordWin,
    Scrcmd_Wbt_GetWinCount,
    Scrcmd_WbtStadium_GetRoundRemaining,
    Scrcmd_WbtEntrance_AwardBattlePoints,
    NULL,
    Scrcmd_WbtEntrance_GetSlot,
    Scrcmd_WbtEntrance_GetSlotTrainerObjCodes,
    Scrcmd_Wbt_GetEntryCount,
    Scrcmd_WbtEntrance_CountDownloads,
    Scrcmd_WbtEntrance_SetDownloadSlot,
    Scrcmd_WbtEntrance_GetDownloadSlot,
    Scrcmd_WbtEntrance_SetUseBattleBox,
    Scrcmd_WbtStadium_ShowPartyList,
    Scrcmd_WbtStadium_ClosePartyList,
    Scrcmd_WbtStadium_ResetPicks,
    Scrcmd_WbtStadium_RandomPick,
    Scrcmd_WbtStadium_SetPick,
    Scrcmd_WbtStadium_WordSetPickSpecies,
    Scrcmd_WbtStadium_ListPartyPokemon,
    Scrcmd_WbtStadium_Dummy,
    Scrcmd_WbtStadium_SwapPickedPokemon,
    Scrcmd_WbtEntrance_MakeRentalParty,
    Scrcmd_WbtStadium_MakeOpponentParty,
    Scrcmd_WbtStadium_GetOpponentObjCode,
    Scrcmd_WbtStadium_ShowOpponentLine,
    Scrcmd_WbtStadium_CallBattle,
    Scrcmd_WbtStadium_ShowOpponentLine7,
    Scrcmd_Wbt_SetTournamentNameWord,
    Scrcmd_WbtStadium_WordSetOpponentName,
    Scrcmd_WbtEntrance_CallEntryPartySelect,
    Scrcmd_Wbt_ShowTrainerList,
    Scrcmd_WbtEntrance_CallPokeSelect,
    Scrcmd_WbtEntrance_ShowDownloads,
    Scrcmd_WbtEntrance_ShowWinRecord,
    Scrcmd_WbtEntrance_Download,
    Scrcmd_WbtStadium_ShowPortraits,
    Scrcmd_WbtStadium_LoadPortraitScene,
    Scrcmd_WbtStadium_PlayStadiumIntro,
    Scrcmd_WbtStadium_SwitchLights,
    Scrcmd_WbtStadium_PlayCrowd,
    Scrcmd_WbtEntrance_GetRegulationId,
    func_ov056_021e77b0,
    Scrcmd_WbtEntrance_CheckEntryParties,
    Scrcmd_WbtStadium_SetNamePlates,
    (FieldScriptCommand)0xFFFFFFFF,
};
