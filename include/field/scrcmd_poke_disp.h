#ifndef POKEBW2_FIELD_SCRCMD_POKE_DISP_H
#define POKEBW2_FIELD_SCRCMD_POKE_DISP_H

// Overlay 36's scrcmd_poke_disp.c (a descriptive name): script commands of the field, such as a Pokémon shown on the
// screen, the records and the TV. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"

BOOL Scrcmd_PokeDisp_CallPokemonPreview(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_IsOneShotDRObtained(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_CGearControlWarning(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_RecordAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_RecordGet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_JoinAvenueRecordActivity(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_TVGenProgramMsgID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_TVCheckCommercial(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_TVGenCommercialMsgID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokeDisp_StartSwapNpcEvent(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKE_DISP_H
