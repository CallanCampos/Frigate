#ifndef POKEBW2_FIELD_SCRIPT_NETWORK_H
#define POKEBW2_FIELD_SCRIPT_NETWORK_H

#include "types.h"
#include "struct_decls.h"

extern const char data_ov012_0216e254[];

BOOL ScriptNative_WaitGameCommDisconnect(VM *vm, void *env);
BOOL s0139_GameCommDisconnect(VM *vm, FieldScriptEnv *env);
BOOL s013B_GameCommCheckDSiWiFi(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRIPT_NETWORK_H
