#ifndef POKEBW2_FIELD_SCRCMD_WBT_H
#define POKEBW2_FIELD_SCRCMD_WBT_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Pokémon World Tournament's script plugins, 6 for the entrance and 7 for the stadium, which
// WBT_SCRIPT_COMMANDS lists. Overlay 55 has the commands both plugins use, and overlays 56 and 57 the others

// The tournament's system, which the plugins' commands work on
WbtSystem *WbtCmd_GetSystem(FieldScriptEnv *env);

// Overlay 56
BOOL Scrcmd_WbtEntrance_AwardBattlePoints(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_IsTournamentAvailable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_WouldOpenTournament(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_CheckRegulation(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_GetRegulationId(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_CheckEntryParties(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_CallEntryPartySelect(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e77b0(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_SetUseBattleBox(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_MakeRentalParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_GetSlotTrainerObjCodes(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_GetSlot(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_CountDownloads(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_SetDownloadSlot(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_GetDownloadSlot(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_CallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_ShowDownloads(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_ShowWinRecord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtEntrance_Download(VM *vm, FieldScriptEnv *env);

// Overlay 57
BOOL Scrcmd_WbtStadium_GetOpponentObjCode(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ShowOpponentLine(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_CallBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ShowOpponentLine7(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_WordSetOpponentName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_MakeOpponentParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_GetRoundRemaining(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ShowPartyList(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ClosePartyList(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ResetPicks(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_RandomPick(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_SetPick(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_WordSetPickSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ListPartyPokemon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_Dummy(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_SwapPickedPokemon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_SetNamePlates(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_ShowPortraits(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_LoadPortraitScene(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_PlayStadiumIntro(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_SwitchLights(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_WbtStadium_PlayCrowd(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_WBT_H
