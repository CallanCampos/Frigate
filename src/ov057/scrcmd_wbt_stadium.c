#include "types.h"
#include "field/event_wbt.h"
#include "field/field_script.h"
#include "field/wbt_battle.h"
#include "field/wbt_stadium_gimmick.h"
#include "field/scrcmd_wbt.h"
#include "field/wbt.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

// The commands of the Pokémon World Tournament's stadium (plugin 7) that overlay 55 doesn't have

// Puts the rival's name in a word of the WordSet
static void func_ov057_021e75c0(FieldScriptEnv *env, WordSet *wordSet, u32 index) {
    GFL_WordSetLoadStr(wordSet, index,
                       getPtrToRivalName(getHollow_RivalData(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)))));
}

// Puts the species of a Pokémon of the party in a word of the script's WordSet
static void func_ov057_021e75e4(FieldScriptEnv *env, PokeParty *party, u8 slot, u32 index) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));

    setPartyPokemonSpeciesNameToStrbuf(wordSet, index, PokeParty_GetPkm(party, slot));
}

BOOL func_ov057_021e760c(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 *var = ScriptReadVar(vm, env);

    *var = WbtBattle_GetOpponentObjCode(sys);
    return FALSE;
}

BOOL func_ov057_021e7630(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 message = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(work);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);

    WbtBattle_LoadOpponentLine(sys, message, text);
    func_ov057_021e75c0(env, wordSet, 1);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    return func_ov036_021a8eb4(vm, env, strbuf, value, 0, 0);
}

BOOL func_ov057_021e76a8(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 *var = ScriptReadVar(vm, env);

    GameData_SetLastBtlResult(gameData, 1);
    func_ov055_021e5cd8(sys, var);
    ScriptWork_CallEvent(work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(22), EventWbtBattle_Create, sys));
    return TRUE;
}

BOOL func_ov057_021e770c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(work);
    u16 value = ScriptReadAny(vm, env);
    WbtSystem *sys = func_ov055_021e5800(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);

    WbtBattle_LoadOpponentLine(sys, 7, text);
    func_ov057_021e75c0(env, wordSet, 1);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    return loadMsgBox(vm, env, strbuf, 0, value);
}

// Puts the opponent's name in a word of the script's WordSet
BOOL func_ov057_021e777c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    WbtSystem *sys = func_ov055_021e5800(env);
    u8 index = VM_Read8(vm);
    u32 unk = WbtBattle_GetOpponentFlag(sys);
    StrBuf *strbuf = ScriptWork_GetAltStrBuf(work);

    WbtBattle_GetOpponentName(sys, strbuf);
    func_0202437c(wordSet, index, strbuf, unk, 1, 2);
    return FALSE;
}

// Makes the opponent's party
BOOL WbtCmd_MakeOpponentParty(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    WbtSystem *sys = func_ov055_021e5800(env);
    WbtEntrant *opponent = WbtBattle_GetOpponent(sys);

    switch (func_ov055_021e5ca4(sys)) {
    case 12:
    case 13:
        func_ov055_021e71ec(sys);
        break;
    default:
        WbtBattle_MakeOpponentParty(sys, gsys, opponent);
        break;
    }
    return FALSE;
}

BOOL func_ov057_021e7814(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 round = func_ov055_021e5cc4(sys);
    u16 *var = ScriptReadVar(vm, env);
    u8 value = WbtBattle_GetRoundRemaining(sys, round);

    if (value == func_ov055_021e5d7c(sys)) {
        value = 0xff;
    }
    *var = value;
    return FALSE;
}

BOOL func_ov057_021e7850(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    void *work = WbtBattle_CreatePartyList(env, sys);

    func_ov055_021e5cfc(sys, work);
    WbtBattle_ShowPartyList(work);
    return FALSE;
}

BOOL func_ov057_021e7878(VM *vm, FieldScriptEnv *env) {
    void *work = func_ov055_021e5d08(func_ov055_021e5800(env));

    WbtBattle_ClosePartyList(work);
    WbtBattle_FreePartyList(work);
    return FALSE;
}

BOOL func_ov057_021e7894(VM *vm, FieldScriptEnv *env) {
    WbtBattle_ResetPicks(func_ov055_021e5800(env));
    return FALSE;
}

BOOL func_ov057_021e78a4(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);

    WbtBattle_RandomPick(sys, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov057_021e78c8(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u16 index = ScriptReadAny(vm, env);

    WbtBattle_SetPick(sys, index, ScriptReadAny(vm, env));
    return FALSE;
}

// Puts the species of one of the Pokémon that WbtBattle_GetPick picks in a word of the script's WordSet
BOOL func_ov057_021e78fc(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    u8 index = VM_Read8(vm);
    u16 which = ScriptReadAny(vm, env);
    u8 slot = WbtBattle_GetPick(sys, which);
    PokeParty *party;

    if (which <= 1) {
        party = func_ov055_021e5d68(sys);
    } else {
        party = func_ov055_021e5d74(sys);
    }
    func_ov057_021e75e4(env, party, slot, index);
    return FALSE;
}

// Adds each Pokémon of the party to the list menu
BOOL func_ov057_021e7948(VM *vm, FieldScriptEnv *env) {
    WbtSystem *sys = func_ov055_021e5800(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    int count = func_ov055_021e5d7c(sys);
    PokeParty *party = func_ov055_021e5d74(sys);
    u8 picked1 = WbtBattle_GetPick(sys, 2);
    u8 picked2 = WbtBattle_GetPick(sys, 3);
    int i;
    u32 message;
    u16 value;

    for (i = 0; i < count; i++) {
        value = i;
        message = 54;
        if (i == picked1 || i == picked2) {
            value = count;
            message = 55;
        }
        func_ov057_021e75e4(env, party, i, 0);
        AddItemToListMenu(env, message, 0xffff, value, strbuf, text);
    }
    return FALSE;
}

BOOL func_ov057_021e79e8(VM *vm, FieldScriptEnv *env) {
    return FALSE;
}

BOOL func_ov057_021e79ec(VM *vm, FieldScriptEnv *env) {
    WbtBattle_SwapPickedPokemon(func_ov055_021e5800(env));
    return FALSE;
}

BOOL func_ov057_021e79fc(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    HeapID heapId;
    ScriptWork *work;
    Field *field;
    WbtSystem *sys;
    u16 opponentUnk;
    StrBuf *name;
    StrBuf *playerName;
    StrBuf *strbuf;
    u16 playerUnk;
    u8 mode;

    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    heapId = FieldScriptEnv_GetHeapID(env);
    work = FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(gsys);
    sys = func_ov055_021e5800(env);
    mode = ScriptReadAny(vm, env);
    opponentUnk = func_ov055_021e5e2c(WbtBattle_GetOpponent(sys));
    playerUnk = func_ov055_021e5e2c(func_ov055_021e5d28(sys));
    switch (mode) {
    case 0:
        name = ScriptWork_GetAltStrBuf(work);
        strbuf = GFL_StrBufCreate(16, heapId);
        WbtBattle_GetOpponentName(sys, strbuf);
        GFL_StrBufClear(name);
        GFL_StrBufUncompress(name, strbuf);
        GFL_StrBufFree(strbuf);
        playerName = copyTrainerNameToNewStrbuf(GetGameDataPlayerInfo(gameData)->name, heapId);
        break;
    case 1: {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);

        name = ScriptWork_GetAltStrBuf(work);
        textCopy(playerInfo->name, name);
        playerName = NULL;
        opponentUnk = playerUnk;
        playerUnk = 0;
        break;
    }
    case 2:
        name = ScriptWork_GetAltStrBuf(work);
        strbuf = GFL_StrBufCreate(16, heapId);
        WbtBattle_GetOpponentName(sys, strbuf);
        GFL_StrBufClear(name);
        GFL_StrBufUncompress(name, strbuf);
        GFL_StrBufFree(strbuf);
        playerName = NULL;
        playerUnk = 0;
        break;
    default:
        name = NULL;
        playerName = NULL;
        opponentUnk = 0;
        playerUnk = 0;
        break;
    }
    WbtStadiumGimmick_SetNames(field, mode, name, playerName, opponentUnk, playerUnk);
    if (playerName != NULL) {
        GFL_StrBufFree(playerName);
    }
    return FALSE;
}

BOOL func_ov057_021e7b20(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field;
    u8 value;

    FieldScriptEnv_GetGameData(env);
    FieldScriptEnv_GetHeapID(env);
    field = GSYS_GetField(gsys);
    func_ov055_021e5800(env);
    value = ScriptReadAny(vm, env);
    WbtStadiumGimmick_ShowPortraits(field, value);
    return FALSE;
}

BOOL func_ov057_021e7b60(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u8 a1 = ScriptReadAny(vm, env);
    u8 a2 = ScriptReadAny(vm, env);

    if (a1 >= 21) {
        return FALSE;
    }
    WbtStadiumGimmick_LoadPortraitScene(field, a1, a2);
    return FALSE;
}

BOOL func_ov057_021e7ba0(VM *vm, FieldScriptEnv *env) {
    WbtStadiumGimmick_PlayIntro(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL func_ov057_021e7bb4(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u8 a1 = ScriptReadAny(vm, env);
    u8 a2 = ScriptReadAny(vm, env);

    WbtStadiumGimmick_SwitchLights(field, a1, a2);
    return FALSE;
}

BOOL func_ov057_021e7bec(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u8 value = ScriptReadAny(vm, env);

    WbtStadiumGimmick_PlayCrowd(field, value);
    return FALSE;
}
