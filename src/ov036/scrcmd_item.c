// The script commands of items: adding to and taking from the Bag, counting, and an item's pocket and kind. The ROM
// has no name for the file; scrcmd_item.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/items.h"
#include "field/field_script.h"
#include "field/scrcmd_item.h"
#include "pml/item.h"
#include "save/bag.h"
#include "system/game_data.h"
#include "system/vm.h"

static u16 isMoveMachine(u16 item);

BOOL Scrcmd_Item_ItemAdd(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    u16 item = ScriptReadAny(vm, env);
    u16 count = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = BagSave_AddItem(bag, item, count, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemSub(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    u16 item = ScriptReadAny(vm, env);
    u16 count = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = BagSave_SubItem(bag, item, count, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemCheckSpace(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    u16 item = ScriptReadAny(vm, env);
    u16 count = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = BagSave_CheckAvailItemSpace(bag, item, count, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemCheckAmount(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    u16 item = ScriptReadAny(vm, env);
    u16 count = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = BagSave_CheckAmount(bag, item, count, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemGetCount(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = BagSave_GetItemCountByID(bag, item, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemIsTMHM(VM *vm, FieldScriptEnv *env) {
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = isMoveMachine(item);
    return FALSE;
}

BOOL Scrcmd_Item_ItemGetPocket(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = GetItemParam(item, ITEM_PARAM_FIELD_POCKET, heapId);
    return FALSE;
}

BOOL Scrcmd_Item_ItemGetClass(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 item = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = GetItemParam(item, ITEM_PARAM_KIND, heapId);
    return FALSE;
}

// How many of the TMs the player has
BOOL Scrcmd_Item_ItemGetTMCount(VM *vm, FieldScriptEnv *env) {
    u16 count;
    u16 item;
    BagSave *bag = GameData_GetBag(FieldScriptEnv_GetGameData(env));
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    u16 *result = ScriptReadVar(vm, env);

    count = 0;
    for (item = ITEM_TM01; item <= ITEM_TM95; item++) {
        if (PML_ItemIsTM(item) && BagSave_CheckAmount(bag, item, 1, heapId)) {
            count++;
        }
    }
    *result = count;
    return FALSE;
}

static u16 isMoveMachine(u16 item) {
    return PML_ItemIsTMHM(item);
}
