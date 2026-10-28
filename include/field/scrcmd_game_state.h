#ifndef POKEBW2_FIELD_SCRCMD_GAME_STATE_H
#define POKEBW2_FIELD_SCRCMD_GAME_STATE_H

// Overlay 12's scrcmd_game_state.c (a descriptive name): the script commands of the game's state: the version, random
// numbers, the clock, the Trainer Card, zones, the Pokédex, saving, the Trainer flags and battle results. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_GameState_GameGetVersion(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_ConfigToggleKanji(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_ConfigGetKanji(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_Random(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTGetTextFile(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardGetStars(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTCGetDayPart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTCGetWeekDay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTCGetDate(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTCGetTime(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTCGetSeason(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardGetBirthDate(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardGetSex(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardHasBadge(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardAddBadge(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerCardGetBadgeCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_SaveDataGetStatus(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_RTGetZoneID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_FieldSetTeleportZone(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_FieldSetNextZoneHere(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_FieldSetNextZone(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_MapReplaceSetEvent(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_MapReplaceIsEventSet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexRegist(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexIsRegist(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexGiveNational(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexHaveNational(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexEnable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_PokeDexEnableHabitatList(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_SetSoftResetBlocked(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_SetOneShotDRObtained(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_IsOneShotDRObtained_02F2(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_SaveDataCheckRequired(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_GiveRunningShoes(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_BoxUnlockWallpapers(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_GetTrainerViewGroupAndMarkSeen(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_StartMenuIsItemUnlocked(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_StartMenuUnlockItem(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_StartMenuIsItemSelected(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_HOFCheckIntegrity(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_ObjInitPointGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_ObjInitWarpGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_ObjInitNPCGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerFlagSet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerFlagReset(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerFlagGet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_CallTrainerLose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_TrainerBattleIsVictory(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_CallWildLose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_WildBattleIsVictory(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_WildBattleGetResult(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02156104(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02156128(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_GameState_FieldOpenRestoreLCD(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_GAME_STATE_H
