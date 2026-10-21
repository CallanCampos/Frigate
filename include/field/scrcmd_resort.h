#ifndef POKEBW2_FIELD_SCRCMD_RESORT_H
#define POKEBW2_FIELD_SCRCMD_RESORT_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Join Avenue's script plugin (plugin 8), which RESORT_SCRIPT_COMMANDS lists. Overlay 59's
// scrcmd_resort.c and scrcmd_medalinfo.c have them, and while the shops are open, overlay 60 takes overlay 59's place
// and runs its own command from the first entry's address

BOOL ResortCmd_StartTalk(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_EndTalk(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetScriptValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_MakeData(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetWord(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_RemovePerson(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SaveShopId(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_RaffleShopItem(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetFriendship(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_RaiseLevel(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_ChangeEffortValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_ShortenEggSteps(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_PayAndSetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetPersonVisit(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_AddShopExperience(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_AddPopularity(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetOccupantsLocked(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_HideActors(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_Visit(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_CheckRank100Unlock(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_NameEntry(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_MarkRaffled(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetNPCValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetOccupantWords(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetNPCWord(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_DescribeOccupant(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetGoodsValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_SetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_GetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_StartSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_IsSceneAnimDone(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_EndSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_AddEntryOrRecord(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_ShowRecords(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_AddRecordPerson(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_RecreatePeople(VM *vm, FieldScriptEnv *env);
BOOL ResortCmd_ShowMedalInfo(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_RESORT_H
