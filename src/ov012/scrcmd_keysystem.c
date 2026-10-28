// Script commands of the Key System: the game's difficulty and the Unova Link's key unlocking. The name is the file's
// own; Scrcmd_Keysystem_GameGetDifficulty and Scrcmd_Keysystem_CallUnovaLinkKeyUnlock are swan's names
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "app/unova_link.h"
#include "field/field_script.h"
#include "field/scrcmd_keysystem.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "save/key_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

static void FreeUnovaLinkKeyParam(void *param);

BOOL Scrcmd_Keysystem_GameGetMyCity(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    result = ScriptReadVar(vm, env);
    *result = GameData_GetMyCity(gameData);
    return FALSE;
}

BOOL Scrcmd_Keysystem_GameGetDifficulty(VM *vm, FieldScriptEnv *env) {
    KeyInfoSave *keyInfo;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    keyInfo = getKeyInfoSaveBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    result = ScriptReadVar(vm, env);
    *result = GetGameDifficulty(keyInfo);
    return FALSE;
}

BOOL Scrcmd_Keysystem_KeyInfoGetChamberKey(VM *vm, FieldScriptEnv *env) {
    KeyInfoSave *keyInfo;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    keyInfo = getKeyInfoSaveBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    result = ScriptReadVar(vm, env);
    *result = KeyInfo_GetChamberKey(keyInfo);
    return FALSE;
}

static void FreeUnovaLinkKeyParam(void *param) {
    GFL_HeapFree(param);
}

BOOL Scrcmd_Keysystem_CallUnovaLinkKeyUnlock(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    u16 key = ScriptReadAny(vm, env);
    UnovaLinkParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(UnovaLinkParam), TRUE, "scrcmd_keysystem.c", 140);

    param->gameData = gameData;
    param->mode = UNOVA_LINK_MODE_GAME_CLEAR;
    param->key = key;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_ID(332),
                                                                                 &UNOVA_LINK_PROC_FUNCTIONS, param,
                                                                                 FreeUnovaLinkKeyParam, param));
    return TRUE;
}
