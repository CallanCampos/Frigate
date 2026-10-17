#ifndef POKEBW2_FIELD_SCRCMD_FIELD_H
#define POKEBW2_FIELD_SCRCMD_FIELD_H

// Overlay 36's scrcmd_field.c (a descriptive name), not written yet: script commands of the field, such as its
// subscreen, the records and the TV. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"

BOOL s0198_FieldSubscreenChange(VM *vm, FieldScriptEnv *env);
BOOL s0199_FieldSubscreenReturn(VM *vm, FieldScriptEnv *env);
// FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS's entry for the subscreen
BOOL FieldScriptSubEventFinish_Subscreen(FinishScriptSubEventsWork *work, u32 *state);
BOOL func_ov036_021c730c(VM *vm, FieldScriptEnv *env);
BOOL s01FA_PepQuizGenerate(VM *vm, FieldScriptEnv *env);
BOOL s0214_CallPokemonPreview(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021c7698(VM *vm, FieldScriptEnv *env);
BOOL s019A_CGearControlWarning(VM *vm, FieldScriptEnv *env);
BOOL s0227_RecordAdd(VM *vm, FieldScriptEnv *env);
BOOL s0228_RecordGet(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021c783c(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021c784c(VM *vm, FieldScriptEnv *env);
BOOL s02BC_TVCheckCommercial(VM *vm, FieldScriptEnv *env);
BOOL s02BD_TVGenCommercialMsgID(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021c7ad0(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_FIELD_H
