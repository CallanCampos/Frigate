#ifndef POKEBW2_FIELD_SCRCMD_NET_H
#define POKEBW2_FIELD_SCRCMD_NET_H

// Overlay 36's scrcmd_net.c: the script commands of the network. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x15B and 0x15C

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Net_GameCommGetStatus(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Net_WifiListHasValidProfile(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Net_WifiListCountFriends(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Net_NetConnectWiFiClub(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Net_NetConnectGTS(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Net_NetConnectGTSNegotiation(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_NET_H
