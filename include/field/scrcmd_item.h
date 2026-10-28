#ifndef POKEBW2_FIELD_SCRCMD_ITEM_H
#define POKEBW2_FIELD_SCRCMD_ITEM_H

// Overlay 36's scrcmd_item.c: the script commands of items. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Item_ItemAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemSub(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemCheckSpace(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemCheckAmount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemIsTMHM(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemGetPocket(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemGetClass(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Item_ItemGetTMCount(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ITEM_H
