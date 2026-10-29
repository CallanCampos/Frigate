#ifndef POKEBW2_FIELD_SCRCMD_WORD_H
#define POKEBW2_FIELD_SCRCMD_WORD_H

// Overlay 36's scrcmd_word.c: the script commands that set the words of messages. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x207, 0x208, 0x275
// and 0x298

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Word_WordSetPlayerName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetLoadRivalName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetItemName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetItemNameWithArticle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetItemNameEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetTMMoveName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetMoveName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPokeTypeName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPokeSpeciesWithArticle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetItemPocketName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPartyPokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPartyPokeName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetDaycarePokeSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetDaycarePokeName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetNumber(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPlaceName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetTrendName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetCountry(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetHobbyName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetSurveyAnswer(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPassPowerName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetTrainerClassName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetTrainerClassNameWithArticle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetPasserbyCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetThankedCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetMedalName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetMedalRank(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetEmptyFileMessage(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetLoadAbility(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetLoadNature(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetFunfestMission(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Word_WordSetLoadJoinAvenueName(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_WORD_H
