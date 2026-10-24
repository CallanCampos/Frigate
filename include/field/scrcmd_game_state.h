#ifndef POKEBW2_FIELD_SCRCMD_GAME_STATE_H
#define POKEBW2_FIELD_SCRCMD_GAME_STATE_H

// Overlay 12's scrcmd_game_state.c (a descriptive name): the script commands of the game's state: the version, random
// numbers, the clock, the Trainer Card, zones, the Pokédex, saving, the Trainer flags and battle results. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s00E0_GameGetVersion(VM *vm, FieldScriptEnv *env);
BOOL s00C9_ConfigToggleKanji(VM *vm, FieldScriptEnv *env);
BOOL s00CA_ConfigGetKanji(VM *vm, FieldScriptEnv *env);
BOOL s00CB_Random(VM *vm, FieldScriptEnv *env);
BOOL s00CC_RTGetTextFile(VM *vm, FieldScriptEnv *env);
BOOL s00CE_TrainerCardGetStars(VM *vm, FieldScriptEnv *env);
BOOL s00CD_RTCGetDayPart(VM *vm, FieldScriptEnv *env);
BOOL s00CF_RTCGetWeekDay(VM *vm, FieldScriptEnv *env);
BOOL s00D0_RTCGetDate(VM *vm, FieldScriptEnv *env);
BOOL s00D1_RTCGetTime(VM *vm, FieldScriptEnv *env);
BOOL s00D2_RTCGetSeason(VM *vm, FieldScriptEnv *env);
BOOL s00D4_TrainerCardGetBirthDate(VM *vm, FieldScriptEnv *env);
BOOL s00E1_TrainerCardGetSex(VM *vm, FieldScriptEnv *env);
BOOL s00D5_TrainerCardHasBadge(VM *vm, FieldScriptEnv *env);
BOOL s00D6_TrainerCardAddBadge(VM *vm, FieldScriptEnv *env);
BOOL s00D7_TrainerCardGetBadgeCount(VM *vm, FieldScriptEnv *env);
BOOL s0138_SaveDataGetStatus(VM *vm, FieldScriptEnv *env);
BOOL s00D3_RTGetZoneID(VM *vm, FieldScriptEnv *env);
BOOL s00D9_FieldSetTeleportZone(VM *vm, FieldScriptEnv *env);
BOOL s00DB_FieldSetNextZoneHere(VM *vm, FieldScriptEnv *env);
BOOL s00DC_FieldSetNextZone(VM *vm, FieldScriptEnv *env);
BOOL s00DA_MapReplaceSetEvent(VM *vm, FieldScriptEnv *env);
BOOL s00D8_MapReplaceIsEventSet(VM *vm, FieldScriptEnv *env);
BOOL s00DE_PokeDexRegist(VM *vm, FieldScriptEnv *env);
BOOL s00DF_PokeDexIsRegist(VM *vm, FieldScriptEnv *env);
BOOL s00DD_PokeDexGetCount(VM *vm, FieldScriptEnv *env);
BOOL s01C6_PokeDexGiveNational(VM *vm, FieldScriptEnv *env);
BOOL s01C7_PokeDexHaveNational(VM *vm, FieldScriptEnv *env);
BOOL s01C8_PokeDexEnable(VM *vm, FieldScriptEnv *env);
BOOL s02D0_PokeDexEnableHabitatList(VM *vm, FieldScriptEnv *env);
BOOL s02ED_SetSoftResetBlocked(VM *vm, FieldScriptEnv *env);
BOOL s02F1_SetOneShotDRObtained(VM *vm, FieldScriptEnv *env);
BOOL s02F2_IsOneShotDRObtained(VM *vm, FieldScriptEnv *env);
BOOL s00E2_SaveDataCheckRequired(VM *vm, FieldScriptEnv *env);
BOOL s00E3_GiveRunningShoes(VM *vm, FieldScriptEnv *env);
BOOL s00E4_BoxUnlockWallpapers(VM *vm, FieldScriptEnv *env);
BOOL FieldScriptEnv_CmdGetTrainerViewGroupAndMarkSeen(VM *vm, FieldScriptEnv *env);
BOOL s00E6_StartMenuIsItemUnlocked(VM *vm, FieldScriptEnv *env);
BOOL s00E7_StartMenuUnlockItem(VM *vm, FieldScriptEnv *env);
BOOL s00E8_StartMenuIsItemSelected(VM *vm, FieldScriptEnv *env);
BOOL s00EA_HOFCheckIntegrity(VM *vm, FieldScriptEnv *env);
BOOL s01D7_ObjInitPointGPos(VM *vm, FieldScriptEnv *env);
BOOL s01D8_ObjInitWarpGPos(VM *vm, FieldScriptEnv *env);
BOOL s01D9_ObjInitNPCGPos(VM *vm, FieldScriptEnv *env);
BOOL s0095_TrainerFlagSet(VM *vm, FieldScriptEnv *env);
BOOL s0096_TrainerFlagReset(VM *vm, FieldScriptEnv *env);
BOOL s0097_TrainerFlagGet(VM *vm, FieldScriptEnv *env);
BOOL s008C_CallTrainerLose(VM *vm, FieldScriptEnv *env);
BOOL s008D_TrainerBattleIsVictory(VM *vm, FieldScriptEnv *env);
BOOL s0176_CallWildLose(VM *vm, FieldScriptEnv *env);
BOOL s0177_WildBattleIsVictory(VM *vm, FieldScriptEnv *env);
BOOL s0178_WildBattleGetResult(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02156104(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02156128(VM *vm, FieldScriptEnv *env);
BOOL s02D2_FieldOpenRestoreLCD(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_GAME_STATE_H
