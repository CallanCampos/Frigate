#ifndef POKEBW2_FIELD_SCRCMD_RESORT_H
#define POKEBW2_FIELD_SCRCMD_RESORT_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Join Avenue's script plugin (plugin 8), which RESORT_SCRIPT_COMMANDS lists. Overlay 59's
// scrcmd_resort.c and scrcmd_medalinfo.c have them, and while the shops are open, overlay 60 takes overlay 59's place
// and runs its own command from the first entry's address

BOOL Scrcmd_Resort_StartTalk(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_EndTalk(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetScriptValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_MakeData(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetWord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RemovePerson(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SaveShopId(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RaffleShopItem(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetFriendship(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RaiseLevel(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ChangeEffortValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ShortenEggSteps(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_PayAndSetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetPersonVisit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddShopExperience(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddPopularity(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOccupantsLocked(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_HideActors(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_Visit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_CheckRank100Unlock(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_NameEntry(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_MarkRaffled(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetNPCValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOccupantWords(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetNPCWord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_DescribeOccupant(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetGoodsValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_StartSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_IsSceneAnimDone(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_EndSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddEntryOrRecord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ShowRecords(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddRecordPerson(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RecreatePeople(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medalinfo_ShowMedalInfo(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_RESORT_H
