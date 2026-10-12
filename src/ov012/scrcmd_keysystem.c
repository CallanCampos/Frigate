// Script commands of the Key System: the game's difficulty and the Unova Link's key unlocking. The name is the file's
// own; s02AF_GameGetDifficulty and s02B0_CallUnovaLinkKeyUnlock are swan's names
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

// What the Unova Link starts on
typedef struct {
    u32 unk00;
    GameData *gameData;
    u32 mode;
} UnovaLinkKeyParam;

static void func_ov012_0216a8cc(void *param);

BOOL func_ov012_0216a82c(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    result = ScriptReadVar(vm, env);
    *result = func_02017220(gameData);
    return FALSE;
}

BOOL s02AF_GameGetDifficulty(VM *vm, FieldScriptEnv *env) {
    KeyInfoSave *keyInfo;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    keyInfo = getKeyInfoSaveBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    result = ScriptReadVar(vm, env);
    *result = GetGameDifficulty(keyInfo);
    return FALSE;
}

BOOL func_ov012_0216a894(VM *vm, FieldScriptEnv *env) {
    KeyInfoSave *keyInfo;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    keyInfo = getKeyInfoSaveBlk(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    result = ScriptReadVar(vm, env);
    *result = func_020105a0(keyInfo);
    return FALSE;
}

static void func_ov012_0216a8cc(void *param) {
    GFL_HeapFree(param);
}

BOOL s02B0_CallUnovaLinkKeyUnlock(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    u16 mode = ScriptReadAny(vm, env);
    UnovaLinkKeyParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(UnovaLinkKeyParam), TRUE, "scrcmd_keysystem.c", 140);

    param->gameData = gameData;
    param->unk00 = 0;
    param->mode = mode;
    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_CreateWithCallback(gsys, field, OVERLAY_ID(332), &UNOVA_LINK_PROC_FUNCTIONS, param,
                                             func_ov012_0216a8cc, param));
    return TRUE;
}
