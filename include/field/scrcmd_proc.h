#ifndef POKEBW2_FIELD_SCRCMD_PROC_H
#define POKEBW2_FIELD_SCRCMD_PROC_H

// Overlay 12's scrcmd_proc.c: the script commands that leave the field for another screen, such as the bag, the mailbox
// or the Pokédex diplomas, and come back. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Proc_FieldOpen(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_FieldClose(VM *vm, FieldScriptEnv *env);
void CreateScrCmdOverlayProcess(VM *vm, FieldScriptEnv *env, s32 overlayId, const GameProcFunctions *functions,
                                void *resource, void (*cleanup)(ScriptOverlayWork *), BagScriptResult *data);
BOOL WaitScrCmdOverlayProcess(VM *vm, void *data);
BOOL Scrcmd_Proc_RTFreeUserHeap(VM *vm, FieldScriptEnv *env);
void CallBag_ReadResult(ScriptOverlayWork *work);
BOOL Scrcmd_Proc_CallBag(VM *vm, FieldScriptEnv *env);
// Called before the Pokédex diplomas
void CallMailbox_ReadResult(void *arg);
BOOL Scrcmd_Proc_CallMailbox(VM *vm, FieldScriptEnv *env);
void MoveReminder_ReadResult(void *arg);
BOOL Scrcmd_Proc_MoveReminderCallMoveSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_CallMonolith(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_Call3DDemo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_CallPokedexDiploma(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_callPoke3Select(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_Nop(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_NetConnectWiFiBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_NetConnectBattleVideo(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157b7c(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157bb4(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02157bdc(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PROC_H
