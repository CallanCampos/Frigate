#ifndef POKEBW2_FIELD_SCRCMD_PROC_H
#define POKEBW2_FIELD_SCRCMD_PROC_H

// Overlay 12's scrcmd_proc.c: the script commands that leave the field for another screen, such as the bag, the mailbox
// or the Pokédex diplomas, and come back. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s014A_FieldOpen(VM *vm, FieldScriptEnv *env);
BOOL s014B_FieldClose(VM *vm, FieldScriptEnv *env);
void CreateScrCmdOverlayProcess(VM *vm, FieldScriptEnv *env, s32 overlayId, const GameProcFunctions *functions,
                                void *resource, void (*cleanup)(ScriptOverlayWork *), BagScriptResult *data);
BOOL WaitScrCmdOverlayProcess(VM *vm, void *data);
BOOL s014C_RTFreeUserHeap(VM *vm, FieldScriptEnv *env);
void CallBag_ReadResult(ScriptOverlayWork *work);
BOOL s014E_CallBag(VM *vm, FieldScriptEnv *env);
// Called before the Pokédex diplomas
void CallMailbox_ReadResult(void *arg);
BOOL s0150_CallMailbox(VM *vm, FieldScriptEnv *env);
void MoveReminder_ReadResult(void *arg);
BOOL s01D6_MoveReminderCallMoveSelect(VM *vm, FieldScriptEnv *env);
BOOL s016E_CallMonolith(VM *vm, FieldScriptEnv *env);
BOOL s0154_Call3DDemo(VM *vm, FieldScriptEnv *env);
BOOL s0151_CallPokedexDiploma(VM *vm, FieldScriptEnv *env);
BOOL s0153_callPoke3Select(VM *vm, FieldScriptEnv *env);
BOOL s015F_Nop(VM *vm, FieldScriptEnv *env);
BOOL s0160_NetConnectWiFiBattle(VM *vm, FieldScriptEnv *env);
BOOL s0161_NetConnectBattleVideo(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157b7c(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157bb4(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157bdc(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PROC_H
