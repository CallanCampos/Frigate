// The script commands of menus: the yes/no window and the list menus. The ROM has no name for the file; scrcmd_menu.c
// is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/scrcmd_menu.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

typedef struct {
    FieldScriptEnv *env;
    // Set to whether the player chose no
    u16 *result;
    ListMenuUI *window;
} YesNoEvent;

typedef struct {
    FieldScriptEnv *env;
    BOOL ex;
} ListMenuEvent;

static GameEventReturnCode ScrcmdMsgwin_YesNoEventMain(GameEvent *event, u32 *state, void *data) {
    YesNoEvent *yesNo = data;

    FieldScriptEnv_GetScriptWork(yesNo->env);
    switch (*state) {
    case 0:
        FieldScriptEnv_SetWaitCounter(yesNo->env, 1);
        (*state)++;
    case 1:
        if (FieldScriptEnv_UpdateWaitCounter(yesNo->env) == TRUE) {
            yesNo->window = MsgBGPrinter_CreateYesNoMenu(
                Field_GetMsgBGSys(GSYS_GetField(FieldScriptEnv_GetGameSystem(yesNo->env))), 0);
            (*state)++;
        }
        break;
    case 2:
        switch (MsgBGPrinter_YesNoMenuMain(yesNo->window)) {
        case 0:
            *yesNo->result = FALSE;
            (*state)++;
            break;
        case 2:
            break;
        default:
            *yesNo->result = TRUE;
            (*state)++;
            break;
        }
        break;
    case 3:
        MsgBGPrinter_DeleteMenuFunc(yesNo->window);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *ScrcmdMsgwin_CreateYesNoEvent(GameSystem *gsys, FieldScriptEnv *env, u16 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, ScrcmdMsgwin_YesNoEventMain, sizeof(YesNoEvent));
    YesNoEvent *yesNo = GameEvent_GetData(event);

    yesNo->env = env;
    yesNo->result = result;
    return event;
}

BOOL s0047_YesNoWin(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, ScrcmdMsgwin_CreateYesNoEvent(FieldScriptEnv_GetGameSystem(env), env, result));
    return TRUE;
}

static GameEventReturnCode ListMenuEvent_Callback(GameEvent *event, u32 *state, void *data) {
    ListMenuEvent *menu = data;
    BOOL done;

    switch (*state) {
    case 0:
        FieldScriptEnv_SetWaitCounter(menu->env, 1);
        (*state)++;
    case 1:
        if (FieldScriptEnv_UpdateWaitCounter(menu->env) == TRUE) {
            FieldScriptEnv_ShowListMenu(menu->env);
            (*state)++;
        }
        break;
    case 2:
        if (menu->ex) {
            done = FieldScriptEnv_UpdateListMenuEx(menu->env);
        } else {
            done = FieldScriptEnv_UpdateListMenu(menu->env);
        }
        if (done == TRUE) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *CreateListMenuCallEvent(GameSystem *gsys, FieldScriptEnv *env, BOOL ex) {
    GameEvent *event = GameEvent_Create(gsys, NULL, ListMenuEvent_Callback, sizeof(ListMenuEvent));
    ListMenuEvent *menu = GameEvent_GetData(event);

    menu->env = env;
    menu->ex = ex;
    return event;
}

BOOL s00AD_ListMenuInitCommon(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u8 x = VM_Read8(vm);
    u8 y = VM_Read8(vm);
    u16 cursor = ScriptReadAny(vm, env);
    u8 flags = VM_Read8(vm);
    u16 *result = ScriptWork_GetWkAddr(work, gameData, VM_Read16(vm));

    InitListMenu(env, x, y, cursor, flags, 0, result, ScriptWork_GetWordSet(work), NULL);
    return TRUE;
}

BOOL s00AE_ListMenu_AnchorTopLeft(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u8 x = VM_Read8(vm);
    u8 y = VM_Read8(vm);
    u16 cursor = ScriptReadAny(vm, env);
    u8 flags = VM_Read8(vm);
    u16 *result = ScriptWork_GetWkAddr(work, gameData, VM_Read16(vm));
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    InitListMenu(env, x, y, cursor, flags, 0, result, wordSet, GetFieldScriptMsgData(env));
    return TRUE;
}

BOOL s00B2_ListMenu_AnchorTopRight(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u8 x = VM_Read8(vm);
    u8 y = VM_Read8(vm);
    u16 cursor = ScriptReadAny(vm, env);
    u8 flags = VM_Read8(vm);
    u16 *result = ScriptWork_GetWkAddr(work, gameData, VM_Read16(vm));
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    InitListMenu(env, x, y, cursor, flags, 1, result, wordSet, GetFieldScriptMsgData(env));
    return TRUE;
}

BOOL s00AF_ListMenuAdd(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 messageId = VM_Read16(vm);
    u16 descriptionId = VM_Read16(vm);
    u16 value = ScriptReadAny(vm, env);
    StrBuf *expanded = ScriptWork_GetMainStrBuf(work);

    AddItemToListMenu(env, messageId, descriptionId, value, expanded, ScriptWork_GetAltStrBuf(work));
    return FALSE;
}

BOOL s00B0_ListMenuShow(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, CreateListMenuCallEvent(FieldScriptEnv_GetGameSystem(env), env, FALSE));
    return TRUE;
}

BOOL s00B1_ListMenuShow2(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, CreateListMenuCallEvent(FieldScriptEnv_GetGameSystem(env), env, FALSE));
    return TRUE;
}
