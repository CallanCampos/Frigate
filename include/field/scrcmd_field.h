#ifndef POKEBW2_FIELD_SCRCMD_FIELD_H
#define POKEBW2_FIELD_SCRCMD_FIELD_H

// Overlay 36's scrcmd_field.c (a descriptive name): script commands of the field, its subscreen and a check of a
// sentence's two words against two set phrases. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"

BOOL Scrcmd_Field_FieldSubscreenChange(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Field_FieldSubscreenReturn(VM *vm, FieldScriptEnv *env);
// FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS's entry for the subscreen
BOOL FieldScriptSubEventFinish_Subscreen(FinishScriptSubEventsWork *work, u32 *state);
BOOL Scrcmd_Field_CheckPmsWordPair(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_FIELD_H
