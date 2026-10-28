#ifndef POKEBW2_FIELD_SCRCMD_WORD_H
#define POKEBW2_FIELD_SCRCMD_WORD_H

// Overlay 36's scrcmd_word.c: the script commands that set the words of messages. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x207, 0x208, 0x275
// and 0x298

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s004C_WordSetPlayerName(VM *vm, FieldScriptEnv *env);
BOOL s0290_WordSetLoadRivalName(VM *vm, FieldScriptEnv *env);
BOOL s004D_WordSetItemName(VM *vm, FieldScriptEnv *env);
BOOL s004F_WordSetItemNameWithArticle(VM *vm, FieldScriptEnv *env);
BOOL s004E_WordSetItemNameEx(VM *vm, FieldScriptEnv *env);
BOOL s0050_WordSetTMMoveName(VM *vm, FieldScriptEnv *env);
BOOL s0051_WordSetMoveName(VM *vm, FieldScriptEnv *env);
BOOL s0056_WordSetPokeTypeName(VM *vm, FieldScriptEnv *env);
BOOL s0057_WordSetPokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL s0058_WordSetPokeSpeciesWithArticle(VM *vm, FieldScriptEnv *env);
BOOL s0052_WordSetItemPocketName(VM *vm, FieldScriptEnv *env);
BOOL s0053_WordSetPartyPokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL s0054_WordSetPartyPokeName(VM *vm, FieldScriptEnv *env);
BOOL s0055_WordSetDaycarePokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL s005B_WordSetDaycarePokeName(VM *vm, FieldScriptEnv *env);
BOOL s005C_WordSetNumber(VM *vm, FieldScriptEnv *env);
BOOL s0059_WordSetPlaceName(VM *vm, FieldScriptEnv *env);
BOOL s005A_WordSetTrendName(VM *vm, FieldScriptEnv *env);
BOOL s005E_WordSetCountry(VM *vm, FieldScriptEnv *env);
BOOL s005F_WordSetHobbyName(VM *vm, FieldScriptEnv *env);
BOOL s0063_WordSetSurveyAnswer(VM *vm, FieldScriptEnv *env);
BOOL s0060_WordSetPassPowerName(VM *vm, FieldScriptEnv *env);
BOOL s0061_WordSetTrainerClassName(VM *vm, FieldScriptEnv *env);
BOOL s0062_WordSetTrainerClassNameWithArticle(VM *vm, FieldScriptEnv *env);
BOOL s0207_WordSetPasserbyCount(VM *vm, FieldScriptEnv *env);
BOOL s0208_WordSetThankedCount(VM *vm, FieldScriptEnv *env);
BOOL s026C_WordSetMedalName(VM *vm, FieldScriptEnv *env);
BOOL s026D_WordSetMedalRank(VM *vm, FieldScriptEnv *env);
BOOL s0298_WordSetEmptyFileMessage(VM *vm, FieldScriptEnv *env);
BOOL s0299_WordSetLoadAbility(VM *vm, FieldScriptEnv *env);
BOOL s029A_WordSetLoadNature(VM *vm, FieldScriptEnv *env);
BOOL s0275_WordSetFunfestMission(VM *vm, FieldScriptEnv *env);
BOOL s029B_WordSetLoadJoinAvenueName(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_WORD_H
