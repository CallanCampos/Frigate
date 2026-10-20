// The script commands that set the words of messages: names of the player, rival, items, moves, Pokémon, places and
// the rest, and numbers. The ROM has no name for the file; scrcmd_word.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field.h"
#include "field/field_daycare.h"
#include "field/field_script.h"
#include "field/scrcmd_word.h"
#include "field/zone.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/join_avenue.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"
#include "system/wordset.h"

typedef void (*WordSetLoadFunc)(WordSet *wordSet, u32 index, u32 value);
// The trainer class loaders take the class as a u8
typedef void (*WordSetLoadByteFunc)(WordSet *wordSet, u32 index, u8 value);

static BOOL loadAllItemText(VM *vm, FieldScriptEnv *env, WordSetLoadFunc load);
static BOOL loadAllPkmNameText(VM *vm, FieldScriptEnv *env, WordSetLoadFunc load);
static BOOL WordSet_LoadTrainerClassFromScript(VM *vm, FieldScriptEnv *env, WordSetLoadByteFunc load);

BOOL s004C_WordSetPlayerName(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetFieldWork(work);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(FieldScriptEnv_GetGameData(env));

    WordSet *wordSet = ScriptWork_GetWordSet(work);

    copyVarForText(wordSet, VM_Read8(vm), playerInfo);
    return FALSE;
}

BOOL s0290_WordSetLoadRivalName(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptWork_GetFieldWork(work);
    RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);

    GFL_WordSetLoadStr(wordSet, index, getPtrToRivalName(rival));
    return FALSE;
}

static BOOL loadAllItemText(VM *vm, FieldScriptEnv *env, WordSetLoadFunc load) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    load(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s004D_WordSetItemName(VM *vm, FieldScriptEnv *env) {
    return loadAllItemText(vm, env, loadItemNameToStrbuf);
}

BOOL s004F_WordSetItemNameWithArticle(VM *vm, FieldScriptEnv *env) {
    return loadAllItemText(vm, env, loadItemTextNameToStrbuf);
}

// The item's name, plural when there are more than one, or with its article
BOOL s004E_WordSetItemNameEx(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 item = ScriptReadAny(vm, env);
    u16 count = ScriptReadAny(vm, env);
    u8 withArticle = VM_Read8(vm);

    if (count > 1) {
        loadItemsNameToStrbuf(wordSet, index, item);
    } else if (withArticle) {
        loadItemTextNameToStrbuf(wordSet, index, item);
    } else {
        loadItemNameToStrbuf(wordSet, index, item);
    }
    return FALSE;
}

BOOL s0050_WordSetTMMoveName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadMoveNameToStrbuf(wordSet, index, PML_ItemGetTMWazaID(ScriptReadAny(vm, env)));
    return FALSE;
}

BOOL s0051_WordSetMoveName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadMoveNameToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s0056_WordSetPokeTypeName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadTypeTextToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

static BOOL loadAllPkmNameText(VM *vm, FieldScriptEnv *env, WordSetLoadFunc load) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    load(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s0057_WordSetPokeSpecies(VM *vm, FieldScriptEnv *env) {
    return loadAllPkmNameText(vm, env, WordSet_LoadSpeciesName);
}

BOOL s0058_WordSetPokeSpeciesWithArticle(VM *vm, FieldScriptEnv *env) {
    return loadAllPkmNameText(vm, env, loadPokemonTextNameToStrbuf);
}

BOOL s0052_WordSetItemPocketName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 item = ScriptReadAny(vm, env);
    FieldScriptEnv_GetHeapID(env);

    loadBagPocketNameToStrbuf(wordSet, index,
                              BagSave_GetActualItemPocket(GameData_GetBag(FieldScriptEnv_GetGameData(env)), item));
    return FALSE;
}

BOOL s0053_WordSetPartyPokeSpecies(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);
    u16 slot = ScriptReadAny(vm, env);

    setPartyPokemonSpeciesNameToStrbuf(wordSet, index, PokeParty_GetPkm(GameData_GetParty(gameData), slot));
    return FALSE;
}

BOOL s0054_WordSetPartyPokeName(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);
    u16 slot = ScriptReadAny(vm, env);

    loadPokemonNicknameToStrbuf(wordSet, index, PokeParty_GetPkm(GameData_GetParty(gameData), slot));
    return FALSE;
}

BOOL s0055_WordSetDaycarePokeSpecies(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 slot = ScriptReadAny(vm, env);

    setPartyPokemonSpeciesNameToStrbuf(
        wordSet, index, DayCare_GetPkm(Field_GetDayCare(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))), slot));
    return FALSE;
}

BOOL s005B_WordSetDaycarePokeName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 slot = ScriptReadAny(vm, env);

    loadPokemonNicknameToStrbuf(
        wordSet, index, DayCare_GetPkm(Field_GetDayCare(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))), slot));
    return FALSE;
}

BOOL s005C_WordSetNumber(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 number = ScriptReadAny(vm, env);
    u16 digits = ScriptReadAny(vm, env);

    WordSetNumber(wordSet, index, number, digits, 0, TRUE);
    return FALSE;
}

BOOL s0059_WordSetPlaceName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadLocationNameToStrbuf(wordSet, index, ZoneData_GetPlaceNameID(ScriptReadAny(vm, env)));
    return FALSE;
}

BOOL s005A_WordSetTrendName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadSayingForDisplay(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s005E_WordSetCountry(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadCountryToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s005F_WordSetHobbyName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadHobbyNameToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s0063_WordSetSurveyAnswer(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 answer = ScriptReadAny(vm, env);

    loadQuestionnaireAnswerToStrbuf(wordSet, index, answer);
    return FALSE;
}

BOOL s0060_WordSetPassPowerName(VM *vm, FieldScriptEnv *env) {
    u8 index = VM_Read8(vm);
    u16 passPower = ScriptReadAny(vm, env);

    loadPassPowerToStrbuf(ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env)), index, passPower);
    return FALSE;
}

static BOOL WordSet_LoadTrainerClassFromScript(VM *vm, FieldScriptEnv *env, WordSetLoadByteFunc load) {
    u8 index = VM_Read8(vm);
    u16 value = ScriptReadAny(vm, env);

    load(ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env)), index, value);
    return FALSE;
}

BOOL s0061_WordSetTrainerClassName(VM *vm, FieldScriptEnv *env) {
    return WordSet_LoadTrainerClassFromScript(vm, env, loadTrainerTypeText);
}

BOOL s0062_WordSetTrainerClassNameWithArticle(VM *vm, FieldScriptEnv *env) {
    return WordSet_LoadTrainerClassFromScript(vm, env, loadTrainerTypeWithArticleToStrbuf);
}

BOOL func_ov036_021a7c64(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    TrainerGameInfoSave *info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);

    WordSetNumber(wordSet, index, func_0200c924(info), 9, 1, TRUE);
    return FALSE;
}

BOOL func_ov036_021a7cb4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    TrainerGameInfoSave *info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u8 index = VM_Read8(vm);

    WordSetNumber(wordSet, index, func_0200c90c(info), 9, 1, TRUE);
    return FALSE;
}

BOOL s026C_WordSetMedalName(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 medal = ScriptReadAny(vm, env);

    loadMedalNameToStrbuf(wordSet, index, medal);
    return FALSE;
}

BOOL s026D_WordSetMedalRank(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

#ifdef BLACK2
    loadMedalRankToStrbuf(wordSet, index, ScriptReadAny(vm, env), 23);
#else
    loadMedalRankToStrbuf(wordSet, index, ScriptReadAny(vm, env), 22);
#endif
    return FALSE;
}

BOOL func_ov036_021a7d70(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 messageId = ScriptReadAny(vm, env);

    loadFromEmptyFile(wordSet, index, messageId);
    return FALSE;
}

BOOL s0299_WordSetLoadAbility(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadAbilityNameToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s029A_WordSetLoadNature(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);

    loadNatureToStrbuf(wordSet, index, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov036_021a7e04(VM *vm, FieldScriptEnv *env) {
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    u8 index = VM_Read8(vm);
    u16 value = ScriptReadAny(vm, env);
    u16 arg = ScriptReadAny(vm, env);

    WordSet_LoadFestivalMission(wordSet, index, value, arg);
    return FALSE;
}

BOOL s029B_WordSetLoadJoinAvenueName(VM *vm, FieldScriptEnv *env) {
    u16 name[22];
    StrBuf *strBuf;
    WordSet *wordSet = ScriptWork_GetWordSet(FieldScriptEnv_GetScriptWork(env));
    JoinAvenueInfo *info =
        JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env))));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u8 index = VM_Read8(vm);

    JoinAvenue_GetParam(info, 0, name);
    strBuf = GFL_StrBufCreate(21, heapId);
    GFL_StrBufLoadString(strBuf, name);
    func_0202437c(wordSet, index, strBuf, 2, 1, 2);
    GFL_StrBufFree(strBuf);
    return FALSE;
}
