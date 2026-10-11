#include "types.h"
#include "app/name_entry.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/gimmick_plasma_frigate.h"
#include "field/gimmick_plasma_frigate_lights.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/str_tool.h"
#include "system/vm.h"

// The script plugin of the Plasma Frigate (plugin 12), commands from 1000. The first four drive the gimmick of zones
// 561 and 564 (overlay 117), the last three that of zones 553 and 563 (overlay 119)

// The frigate's passwords, one for each value of the player's ID modulo PASSWORD_COUNT
#define PASSWORD_COUNT 5u
#define PASSWORD_MAX_LENGTH 10

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    TrainerGameInfoSave *gameInfo;
    PlayerInfo *playerInfo;
    Field *field;
    // Set to TRUE if the password entered is the player's
    u16 *result;
    NameEntryParam *nameEntry;
    u16 heapId;
} PlasmaFrigatePassword;

static GameEvent *PlasmaFrigate_CreatePasswordEvent(GameSystem *gsys, u16 *result);
static u8 PlasmaFrigate_GetPasswordIndex(PlayerInfo *playerInfo);

static BOOL func_ov064_021e5800(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GimmickPlasmaFrigate_CallSwitchEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL func_ov064_021e5838(VM *vm, FieldScriptEnv *env) {
    u16 a;
    GameSystem *gsys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    a = VM_Read16(vm);
    GimmickPlasmaFrigate_SetLightsState(gsys, a, ScriptReadAny(vm, env) != TRUE);
    return FALSE;
}

static BOOL func_ov064_021e586c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickPlasmaFrigate_CallPlayGroupASecond(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL func_ov064_021e588c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickPlasmaFrigate_CallPlayGroupBSecond(gsys, VM_Read16(vm));
    return FALSE;
}

// Lets the player enter a password, and sets a variable to whether it is the player's
static BOOL PlasmaFrigateCmd_EnterPassword(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = PlasmaFrigate_CreatePasswordEvent(gsys, ScriptReadVar(vm, env));

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

// Sets a variable to which of the passwords is the player's
static BOOL PlasmaFrigateCmd_GetPasswordIndex(VM *vm, FieldScriptEnv *env) {
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    u16 *var = ScriptReadVar(vm, env);

    *var = PlasmaFrigate_GetPasswordIndex(playerInfo);
    return FALSE;
}

static BOOL func_ov064_021e5910(VM *vm, FieldScriptEnv *env) {
    u16 a;
    GameSystem *gsys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    a = VM_Read16(vm);
    func_ov119_021eecd0(gsys, a, ScriptReadAny(vm, env) != 0);
    return FALSE;
}

static BOOL func_ov064_021e5944(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_ov119_021eed10(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL func_ov064_021e5964(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_ov119_021eed34(gsys, VM_Read16(vm));
    return FALSE;
}

static void PlasmaFrigate_CheckPassword(PlasmaFrigatePassword *wk, NameEntryParam *nameEntry);
static GameEventReturnCode PlasmaFrigate_PasswordEvent(GameEvent *event, u32 *state, void *data);

static GameEvent *PlasmaFrigate_CreatePasswordEvent(GameSystem *gsys, u16 *result) {
    GameEvent *event = GameEvent_Create(gsys, NULL, PlasmaFrigate_PasswordEvent, sizeof(PlasmaFrigatePassword));
    PlasmaFrigatePassword *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->field = GSYS_GetField(gsys);
    wk->result = result;
    wk->heapId = HEAPID_GAMEEVENT;
    wk->gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(wk->gameData));
    wk->playerInfo = GetGameDataPlayerInfo(wk->gameData);
    wk->nameEntry = setupNameEntry(HEAPID_GAMEEVENT, 11, 0, 0, PASSWORD_MAX_LENGTH, NULL, wk->gameInfo);
    return event;
}

static GameEventReturnCode PlasmaFrigate_PasswordEvent(GameEvent *event, u32 *state, void *data) {
    PlasmaFrigatePassword *wk = data;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(wk->gsys, wk->field, OVERLAY_ID(280),
                                                                         &NAME_ENTRY_PROC_FUNCTIONS, wk->nameEntry));
        (*state)++;
        break;
    case 1:
        PlasmaFrigate_CheckPassword(wk, wk->nameEntry);
        func_ov012_02165ae8(wk->nameEntry);
        (*state)++;
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void PlasmaFrigate_CheckPassword(PlasmaFrigatePassword *wk, NameEntryParam *nameEntry) {
    StrBuf *password;
    MsgData *msgData;

    if (wk->result == NULL) {
        return;
    }
    if (func_ov012_02165b0c(nameEntry) == TRUE) {
        *wk->result = FALSE;
        return;
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SCRCMD_PLASMA_FRIGATE_SCRCMD_BSUBWAY, wk->heapId);
    password = GFL_MsgDataLoadStrbufNew(msgData, PlasmaFrigate_GetPasswordIndex(wk->playerInfo));
    if (GFL_StrBufCmpIgnoreAccents(nameEntry->name, password) == TRUE) {
        *wk->result = TRUE;
    } else {
        *wk->result = FALSE;
    }
    GFL_StrBufFree(password);
    GFL_MsgDataFree(msgData);
}

static u8 PlasmaFrigate_GetPasswordIndex(PlayerInfo *playerInfo) {
    return (u8)getIDAsUInt(playerInfo) % PASSWORD_COUNT;
}

const FieldScriptCommand PLASMA_FRIGATE_SCRIPT_COMMANDS[] = {
    func_ov064_021e5800,
    func_ov064_021e5838,
    func_ov064_021e586c,
    func_ov064_021e588c,
    PlasmaFrigateCmd_EnterPassword,
    PlasmaFrigateCmd_GetPasswordIndex,
    func_ov064_021e5910,
    func_ov064_021e5944,
    func_ov064_021e5964,
    (FieldScriptCommand)0xFFFFFFFF,
};
