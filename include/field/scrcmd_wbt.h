#ifndef POKEBW2_FIELD_SCRCMD_WBT_H
#define POKEBW2_FIELD_SCRCMD_WBT_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Pokémon World Tournament's script plugins, 6 for the entrance and 7 for the stadium, which
// WBT_SCRIPT_COMMANDS lists. Overlay 55 has the commands both plugins use, and overlays 56 and 57 the others

// The tournament's system, which the plugins' commands work on
WbtSystem *WbtCmd_GetSystem(FieldScriptEnv *env);

// Overlay 56
BOOL WbtCmd_AwardBattlePoints(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_IsTournamentAvailable(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_WouldOpenTournament(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CheckRegulation(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_GetRegulationId(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CheckEntryParties(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CallEntryPartySelect(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e77b0(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SetUseBattleBox(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_MakeRentalParty(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_GetSlotTrainerObjCodes(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_GetSlot(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CountDownloads(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SetDownloadSlot(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_GetDownloadSlot(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowDownloads(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowWinRecord(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_Download(VM *vm, FieldScriptEnv *env);

// Overlay 57
BOOL WbtCmd_GetOpponentObjCode(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowOpponentLine(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CallBattle(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowOpponentLine7(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_WordSetOpponentName(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_MakeOpponentParty(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_GetRoundRemaining(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowPartyList(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ClosePartyList(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ResetPicks(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_RandomPick(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SetPick(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_WordSetPickSpecies(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ListPartyPokemon(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_Dummy(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SwapPickedPokemon(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SetNamePlates(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_ShowPortraits(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_LoadPortraitScene(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_PlayStadiumIntro(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_SwitchLights(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_PlayCrowd(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_WBT_H
