#ifndef POKEBW2_FIELD_SCRCMD_POKE_DISP_H
#define POKEBW2_FIELD_SCRCMD_POKE_DISP_H

// Overlay 36's scrcmd_poke_disp.c (a descriptive name): script commands of the field, such as a Pokémon shown on the
// screen, the records and the TV. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"

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

#endif // POKEBW2_FIELD_SCRCMD_POKE_DISP_H
