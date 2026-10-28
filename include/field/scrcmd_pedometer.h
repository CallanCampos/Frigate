#ifndef POKEBW2_FIELD_SCRCMD_PEDOMETER_H
#define POKEBW2_FIELD_SCRCMD_PEDOMETER_H

// Overlay 12's scrcmd_pedometer.c (a descriptive name): the script commands of the step counter. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Pedometer_PedometerStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pedometer_PedometerEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pedometer_PedometerGet(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PEDOMETER_H
