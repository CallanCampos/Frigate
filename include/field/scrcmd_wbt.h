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
BOOL func_ov056_021e7620(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7674(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_CheckRegulation(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e76f4(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7710(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7750(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e77b0(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7808(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_MakeRentalParty(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7894(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7988(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e79c0(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e79f0(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7a30(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7a4c(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7a9c(VM *vm, FieldScriptEnv *env);
BOOL func_ov056_021e7b00(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_Download(VM *vm, FieldScriptEnv *env);

// Overlay 57
BOOL func_ov057_021e760c(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7630(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e76a8(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e770c(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e777c(VM *vm, FieldScriptEnv *env);
BOOL WbtCmd_MakeOpponentParty(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7814(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7850(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7878(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7894(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e78a4(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e78c8(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e78fc(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7948(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e79e8(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e79ec(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e79fc(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7b20(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7b60(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7ba0(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7bb4(VM *vm, FieldScriptEnv *env);
BOOL func_ov057_021e7bec(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_WBT_H
