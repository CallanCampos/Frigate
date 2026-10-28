#ifndef POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H
#define POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H

// Overlay 12's scrcmd_download_data.c (a descriptive name): script commands 0x2B2 to 0x2B7, which read the downloaded
// data the save keeps

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_DownloadData_MemoryLinkCheckFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_DownloadData_MemoryLinkGetVar(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_DownloadData_MemoryLinkIsPlayerLinked(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_DownloadData_WordSetMemoryLinkPlayerName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_DownloadData_MemoryLinkCheckMusical(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_DownloadData_MemoryLinkAddMusicalProps(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_DOWNLOAD_DATA_H
