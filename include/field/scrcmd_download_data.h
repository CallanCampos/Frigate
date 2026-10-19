#ifndef POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H
#define POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H

// Overlay 12's scrcmd_download_data.c (a descriptive name): script commands 0x2B2 to 0x2B7, which read the downloaded
// data the save keeps

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s02B2_MemoryLinkCheckFlag(VM *vm, FieldScriptEnv *env);
BOOL s02B3_MemoryLinkGetVar(VM *vm, FieldScriptEnv *env);
BOOL s02B4_MemoryLinkIsPlayerLinked(VM *vm, FieldScriptEnv *env);
BOOL s02B5_WordSetMemoryLinkPlayerName(VM *vm, FieldScriptEnv *env);
BOOL s02B6_MemoryLinkCheckMusical(VM *vm, FieldScriptEnv *env);
BOOL s02B7_MemoryLinkAddMusicalProps(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H
