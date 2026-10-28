// The script commands of the message windows: the system, money, info, sign and checker windows, the balloons over
// actors and Trainers, the numbered windows, and the loading and formatting of the script's messages. The ROM has no
// name for the file; scrcmd_msg.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names, and command 0x230's, are our descriptive guesses.
#include "types.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "constants/field_script.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "field/event_trainer_eye.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_msg.h"
#include "field/talkmsgwin.h"
#include "gfl/g3d.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/time_icon.h"
#include "system/vm.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"

// The rows of the system and info windows at the top and the bottom of the screen
#define MSG_WIN_ROW_TOP 1
#define MSG_WIN_ROW_BOTTOM 0x13

static StrBuf *setupFormattedTextBuffer(FieldScriptEnv *env, StrBuf *dest, u32 fileNo, u32 msgId);
static StrBuf *LoadFieldScriptMessage(FieldScriptEnv *env, u32 fileNo, u32 msgId);

// The up axis the balloon's offset is computed against
static const VecFx32 ACTOR_MSGWIN_CALC_AXIS_Y = { 0, FX32_ONE, 0 };

// The offset of a balloon from its actor, by the half of the screen the actor is in and the column of the screen
// clang-format off
static const VecFx32 ACTOR_MSGWIN_ORIGIN_OFFSET_TABLE[2][4] = {
    {
        {0, FX32_CONST(16), FX32_CONST(-8)},
        {FX32_CONST(-8), FX32_CONST(12), FX32_CONST(-8)},
        {FX32_CONST(8), FX32_CONST(12), FX32_CONST(-8)},
        {0, FX32_CONST(16), FX32_CONST(-8)},
    },
    {
        {0, FX32_CONST(12), FX32_CONST(-8)},
        {FX32_CONST(-8), FX32_CONST(12), FX32_CONST(-8)},
        {FX32_CONST(8), FX32_CONST(12), FX32_CONST(-8)},
        {0, FX32_CONST(12), FX32_CONST(8)},
    },
};

// clang-format on

// Opens the system message window at the top or the bottom of the screen, or for MSG_WIN_POS_AUTO on the side of the
// balloon over an actor
static void SystemMsgWin_Open(FieldScriptEnv *env, u8 pos) {
    ScriptFieldWork *fieldWork;
    MsgData *msgData;
    FieldMsgWin *window;
    u32 row = MSG_WIN_ROW_TOP;

    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SYSTEM_MSG)) {
        MsgBGPrinter_ClearInfoSysWin(getMapDisplayInfoPtr(env));
        return;
    }
    fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    msgData = GetFieldScriptMsgData(env);
    if (pos != MSG_WIN_POS_TOP) {
        if (pos == MSG_WIN_POS_BOTTOM) {
            row = MSG_WIN_ROW_BOTTOM;
        } else if (pos == MSG_WIN_POS_AUTO) {
            switch (ActorMsgWin_GetPosActual(env)) {
            case MSG_WIN_POS_TOP:
            case MSG_WIN_POS_TOP_LEFT:
            case MSG_WIN_POS_TOP_RIGHT:
                pos = MSG_WIN_POS_TOP;
                break;
            case MSG_WIN_POS_BOTTOM:
            case MSG_WIN_POS_BOTTOM_LEFT:
            case MSG_WIN_POS_BOTTOM_RIGHT:
                row = MSG_WIN_ROW_BOTTOM;
                pos = MSG_WIN_POS_BOTTOM;
                break;
            // A position past the balloons', at the bottom as any other
            case 7:
                row = MSG_WIN_ROW_BOTTOM;
                pos = MSG_WIN_POS_BOTTOM;
                break;
            default:
                row = MSG_WIN_ROW_BOTTOM;
                pos = MSG_WIN_POS_BOTTOM;
                break;
            }
        } else {
            row = MSG_WIN_ROW_BOTTOM;
            pos = MSG_WIN_POS_BOTTOM;
        }
    }
    window = MsgBGPrinter_CreateInfoSysWin(fieldWork->msgBGSys, msgData, row);
    ActorMsgWin_SetPosActual(env, pos);
    setMapDisplayInfoPtr(env, window);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_SYSTEM_MSG);
}

static void SystemMsgWin_Close(FieldScriptEnv *env) {
    WaitIcon *icon;

    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SYSTEM_MSG)) {
        MsgBGPrinter_DeleteInfoSysWin(getMapDisplayInfoPtr(env));
        icon = GetSpecialMessageIconPtr(env);
        if (icon != NULL) {
            WaitIcon_Free(icon);
            SetSpecialMessageIconPtr(env, NULL);
        }
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_SYSTEM_MSG);
    }
}

static BOOL ScriptNative_SystemMsgWinWait(VM *vm, void *env) {
    if (MsgBGPrinter_InfoSysWinMain(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Msg_SystemMsg(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    u16 msgId = ScriptReadAny(vm, env);
    StrBuf *strbuf;

    SystemMsgWin_Open(env, VM_Read16(vm));
    strbuf = LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId);
    MsgBGPrinter_InfoSysWinPrintStr(getMapDisplayInfoPtr(env), 0, 0, strbuf);
    VM_SetNativeCallback(vm, ScriptNative_SystemMsgWinWait);
    return TRUE;
}

BOOL Scrcmd_Msg_SystemMsgAsync(VM *vm, FieldScriptEnv *env) {
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    FieldMsgWin *window;

    SystemMsgWin_Open(env, VM_Read16(vm));
    LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId);
    window = getMapDisplayInfoPtr(env);
    MsgBGPrinter_ClearInfoSysWin(window);
    MsgBGPrinter_InfoSysWinPrintStrNow(window, 0, 0, strbuf);
    return FALSE;
}

BOOL Scrcmd_Msg_MsgSetLoadingSpinner(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = Field_GetHeapID(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 hide = ScriptReadAny(vm, env);
    WaitIcon *icon;
    BmpWin *window;

    if (!FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SYSTEM_MSG)) {
        return FALSE;
    }
    icon = GetSpecialMessageIconPtr(env);
    if (icon != NULL) {
        WaitIcon_Free(icon);
        SetSpecialMessageIconPtr(env, NULL);
    }
    if (hide == 0) {
        window = MsgBGPrinter_GetInfoSysWinBmpWin(getMapDisplayInfoPtr(env));
        SetSpecialMessageIconPtr(env, WaitIcon_Create(GFL_VBlankGetTCBMgr(), window, 0xf, 0x10, heapId));
    }
    return FALSE;
}

BOOL Scrcmd_Msg_SystemMsgClose(VM *vm, FieldScriptEnv *env) {
    SystemMsgWin_Close(env);
    return FALSE;
}

BOOL FieldScriptSubEventFinish_SystemMsg(FinishScriptSubEventsWork *work, u32 *state) {
    SystemMsgWin_Close(work->env);
    return TRUE;
}

BOOL Scrcmd_Msg_MoneyWinDisp(VM *vm, FieldScriptEnv *env) {
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    GameData_GetPlayerState(gameData);
    Field *field = GSYS_GetField(gsys);
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    MoneyWin *moneyWin = Field_GetMoneyWin(field);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    HeapID heapId = Field_GetHeapID(field);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    StrBuf *strbuf = GFL_StrBufCreate(0x80, heapId);
    u32 cash = getCash(getTrainerCardDataBlkAddress(gameData));

    if (moneyWin != NULL) {
        MsgBGPrinter_DeleteMsgWin(moneyWin);
    }
    moneyWin = FieldMsgBG_CreateMoneyWin(msgBGSys, msgData, x - 9, y, 9, 2);
    Field_SetMoneyWin(field, moneyWin);
    WordSetNumber(wordSet, 2, cash, 7, 1, TRUE);
    GFL_MsgDataLoadStrbuf(msgData, Global10520_Text_Empty_2, text);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    MsgBGPrinter_MsgWinPrintStr(moneyWin, 0, 0, strbuf);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

BOOL Scrcmd_Msg_MoneyWinUpdate(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    GameData_GetPlayerState(gameData);
    Field *field = GSYS_GetField(gsys);
    MoneyWin *moneyWin = Field_GetMoneyWin(field);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    HeapID heapId = Field_GetHeapID(field);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    StrBuf *strbuf = GFL_StrBufCreate(0x80, heapId);

    WordSetNumber(wordSet, 2, getCash(getTrainerCardDataBlkAddress(gameData)), 7, 1, TRUE);
    GFL_MsgDataLoadStrbuf(msgData, Global10520_Text_Empty_2, text);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    MsgBGPrinter_ClearMsgWin(moneyWin);
    MsgBGPrinter_MsgWinPrintStr(moneyWin, 0, 0, strbuf);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

BOOL Scrcmd_Msg_MoneyWinClose(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    MoneyWin *moneyWin = Field_GetMoneyWin(field);

    if (moneyWin != NULL) {
        MsgBGPrinter_DeleteMsgWin(moneyWin);
        Field_SetMoneyWin(field, NULL);
    }
    return FALSE;
}

// The offset of a balloon over an actor at pos, by where on the screen the actor is. The field camera is passed along
// from GetAttachmentOffset but not used
static void ActorMsgWin_CalcAttachmentOffsetDynamic(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera,
                                                    FieldCamera *camera) {
    VecFx32 world = *pos;
    VecFx32 dir;
    VecFx32 flatDir;
    VecFx32 side;
    VecFx32 up;
    VecFx32 camPos;
    VecFx32 target;
    int x;
    int y;
    u32 tileX;
    u32 column;

    GFL_G3DCameraFlush(g3dCamera);
    GFL_G3DCameraGetLookatTarget(g3dCamera, &target);
    GFL_G3DCameraGetLookatPos(g3dCamera, &camPos);
    VEC_Subtract(&target, &camPos, &dir);
    flatDir = dir;
    flatDir.y = 0;
    VEC_Normalize(&flatDir, &flatDir);
    VEC_CrossProduct(&flatDir, &ACTOR_MSGWIN_CALC_AXIS_Y, &side);
    VEC_CrossProduct(&dir, &side, &up);
    VEC_Normalize(&up, &up);
    world.x += up.x;
    world.y += up.y;
    world.z += up.z;
    NNS_G3dWorldPosToScrPos(&world, &x, &y);
    tileX = (u32)x / 8;
    if (tileX < 10) {
        column = 0;
    } else if (tileX >= 10 && tileX <= 15) {
        column = 1;
    } else if (tileX >= 16 && tileX < 22) {
        column = 2;
    } else {
        column = 3;
    }
    // y becomes the half of the screen, in rows of 12 tiles
    y = (u32)y / 8 / 12;
    if (column < 4 && (u32)y < 2) {
        *offset = ACTOR_MSGWIN_ORIGIN_OFFSET_TABLE[y][column];
    } else {
        VecFx32 zero = { 0, 0, 0 };

        *offset = zero;
    }
}

// Turns vec from the camera's space to the world's
static void ActorMsgWin_MulVecInvCamRot(VecFx32 *vec, G3DCamera *camera) {
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 target;
    MtxFx43 lookAt;
    MtxFx33 rot;

    GFL_G3DCameraGetLookatPos(camera, &camPos);
    GFL_G3DCameraGetLookatUpVector(camera, &camUp);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    MTX_LookAt(&camPos, &camUp, &target, &lookAt);
    MI_Copy36B(&lookAt, &rot);
    MTX_Inverse33(&rot, &rot);
    MTX_MultVec33(vec, &rot, vec);
}

static void ActorMsgWin_GetAttachmentOffset(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera,
                                            FieldCamera *camera, u8 winPos) {
    u32 column = 0;
    u32 half = 0;

    switch (winPos) {
    case MSG_WIN_POS_TOP_LEFT:
        column = 1;
        break;
    case MSG_WIN_POS_BOTTOM_LEFT:
        column = 1;
        half = 1;
        break;
    case MSG_WIN_POS_TOP_RIGHT:
        column = 2;
        break;
    case MSG_WIN_POS_BOTTOM_RIGHT:
        column = 2;
        half = 1;
        break;
    case MSG_WIN_POS_AUTO:
    case MSG_WIN_POS_TOP:
    case MSG_WIN_POS_BOTTOM:
        ActorMsgWin_CalcAttachmentOffsetDynamic(pos, offset, g3dCamera, camera);
        ActorMsgWin_MulVecInvCamRot(offset, g3dCamera);
        return;
    }
    *offset = ACTOR_MSGWIN_ORIGIN_OFFSET_TABLE[half][column];
    ActorMsgWin_MulVecInvCamRot(offset, g3dCamera);
}

void ActorMsgWin_GetBalloonOffset(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera, FieldCamera *camera,
                                  u8 winPos) {
    ActorMsgWin_GetAttachmentOffset(pos, offset, g3dCamera, camera, winPos);
}

// The balloon (ACTOR_MSG_WIN_) and the side of its tail (TALKMSG_WIN_TAIL_) of a position
void ActorMsgWin_GetBalloonTailParams(u32 winPos, u32 *type, u32 *tail) {
    *type = ACTOR_MSG_WIN_TOP;
    *tail = TALKMSG_WIN_TAIL_NONE;
    switch (winPos) {
    case MSG_WIN_POS_TOP_LEFT:
        *tail = TALKMSG_WIN_TAIL_BOTTOM_LEFT;
        break;
    case MSG_WIN_POS_BOTTOM_LEFT:
        *type = ACTOR_MSG_WIN_BOTTOM;
        *tail = TALKMSG_WIN_TAIL_TOP_LEFT;
        break;
    case MSG_WIN_POS_TOP_RIGHT:
        *tail = TALKMSG_WIN_TAIL_BOTTOM_RIGHT;
        break;
    case MSG_WIN_POS_BOTTOM_RIGHT:
        *type = ACTOR_MSG_WIN_BOTTOM;
        *tail = TALKMSG_WIN_TAIL_TOP_RIGHT;
        break;
    case MSG_WIN_POS_BOTTOM:
        *type = ACTOR_MSG_WIN_BOTTOM;
        break;
    case MSG_WIN_POS_AUTO:
        *type = ACTOR_MSG_WIN_BOTTOM;
        *tail = TALKMSG_WIN_TAIL_BOTTOM;
        break;
    }
}

u8 ActorMsgWin_CalcWinPosAuto(FieldActor *player, const VecFx32 *pos) {
    VecFx32 playerPos;
    u8 winPos = MSG_WIN_POS_BOTTOM;

    CopyActorWPos(player, &playerPos);
    if (pos->z < playerPos.z) {
        winPos = MSG_WIN_POS_TOP;
    }
    return winPos;
}

static BOOL ScriptNative_ActorMsgWinWaitClosed(VM *vm, void *env) {
    if (MsgBGPrinter_WaitTalkWinClose(getMapDisplayInfoPtr(env)) == TRUE) {
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_ACTOR_MSG);
        return TRUE;
    }
    return FALSE;
}

static void ActorMsgWin_CloseAsync(VM *vm, FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        MsgBGPrinter_DeleteTalkWin(getMapDisplayInfoPtr(env));
        VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWaitClosed);
    }
}

static void ActorMsgWin_CloseImmediate(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        MsgBGPrinter_ClearTalkWin(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_ACTOR_MSG);
    }
}

static void ActorMsgWin_CreateWindow(FieldScriptEnv *env, FieldMsgBG *msgBGSys, u32 type, const VecFx32 *pos,
                                     StrBuf *strbuf, u32 frameType, u32 tail) {
    setMapDisplayInfoPtr(env, ActorMsgWin_CheckAndCreate(msgBGSys, type, pos, strbuf, frameType, tail));
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_ACTOR_MSG);
}

// Moves the balloon with its actor. force recomputes the offset even when the actor hasn't moved
static void ActorMsgWin_CalcAttachmentPos(FieldScriptEnv *env, BOOL force) {
    MMSys *mmSys = GetScrEnvActorSys(env);
    ScriptActorWork *actorWork = GetFieldScriptActorWk(env);
    FieldActor *actor = FindFieldActor(mmSys, actorWork->actorId);
    VecFx32 pos;
    FieldCamera *camera;
    G3DCamera *g3dCamera;

    if (actor != NULL) {
        GetActorFaceDir(actor);
        CopyActorWPos(actor, &pos);
        if (force == TRUE || pos.x != actorWork->actorPos.x || pos.y != actorWork->actorPos.y ||
            pos.z != actorWork->actorPos.z) {
            actorWork->actorPos = pos;
            camera = Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
            g3dCamera = FieldCamera_GetG3DCamera(camera);
            ActorMsgWin_GetAttachmentOffset(&actorWork->actorPos, &actorWork->offset, g3dCamera, camera,
                                            ActorMsgWin_GetPos(env));
        }
        actorWork->pos.x = actorWork->actorPos.x + actorWork->offset.x;
        actorWork->pos.y = actorWork->actorPos.y + actorWork->offset.y;
        actorWork->pos.z = actorWork->actorPos.z + actorWork->offset.z;
    }
}

// Shows strbuf in a balloon over the actor, at pos, or where CalcWinPosAuto puts it for MSG_WIN_POS_AUTO
static BOOL SetupActorMessageWindow(FieldScriptEnv *env, StrBuf *strbuf, u16 actorId, u16 pos, u32 frameType) {
    u16 winPos = pos;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    MMSys *mmSys = GetScrEnvActorSys(env);
    FieldActor *player = FindFieldActor(mmSys, ACTOR_UID_PLAYER);
    FieldActor *actor = FindFieldActor(mmSys, actorId);
    ScriptActorWork *actorWork;
    VecFx32 actorPos;
    u32 type;
    u32 tail;

    if (actor == NULL) {
        return FALSE;
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        MsgBGPrinter_TalkWinPrintStr(getMapDisplayInfoPtr(env), strbuf);
    } else {
        actorWork = GetFieldScriptActorWk(env);
        MI_CpuFill8(actorWork, 0, sizeof(ScriptActorWork));
        actorWork->actorId = actorId;
        if (pos == MSG_WIN_POS_AUTO) {
            CopyActorWPos(actor, &actorPos);
            winPos = ActorMsgWin_CalcWinPosAuto(player, &actorPos);
        }
        ActorMsgWin_GetBalloonTailParams(winPos, &type, &tail);
        actorWork->winType = type;
        ActorMsgWin_SetPosActual(env, winPos);
        ActorMsgWin_SetPos(env, pos);
        ActorMsgWin_CalcAttachmentPos(env, TRUE);
        ActorMsgWin_CreateWindow(env, fieldWork->msgBGSys, type, &actorWork->pos, strbuf, frameType, tail);
    }
    return TRUE;
}

static BOOL ScriptNative_ActorMsgWinWait(VM *vm, void *env) {
    ActorMsgWin *msgWin = getMapDisplayInfoPtr(env);

    ActorMsgWin_CalcAttachmentPos(env, FALSE);
    return MsgBGPrinter_TalkWinMain(msgWin) == TRUE;
}

BOOL CallFieldActorMessageDispStrBuf(VM *vm, FieldScriptEnv *env, StrBuf *message, u16 actorId, u16 pos,
                                     u32 frameType) {
    if (SetupActorMessageWindow(env, message, actorId, pos, frameType) == TRUE) {
        VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
        return TRUE;
    }
    return FALSE;
}

static BOOL CallFieldActorMessageDisp(VM *vm, FieldScriptEnv *env, u16 fileNo, u16 msgId, u16 actorId, u16 pos,
                                      u32 frameType) {
    if (SetupActorMessageWindow(env, LoadFieldScriptMessage(env, fileNo, msgId), actorId, pos, frameType) == TRUE) {
        VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Msg_ActorMsg(VM *vm, FieldScriptEnv *env) {
    u16 fileNo = ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 frameType = ScriptReadAny(vm, env);

    return CallFieldActorMessageDisp(vm, env, fileNo, msgId, actorId, pos, frameType);
}

BOOL Scrcmd_Msg_ParentActorMsg(VM *vm, FieldScriptEnv *env) {
    FieldActor *actor = ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env));
    u16 fileNo = ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 frameType = ScriptReadAny(vm, env);

    if (actor == NULL) {
        return TRUE;
    }
    return CallFieldActorMessageDisp(vm, env, fileNo, msgId, (u8)GetActorUID(actor), pos, frameType);
}

// Shows the first or the second message
static BOOL CallFieldActorMessageDispSelect(VM *vm, FieldScriptEnv *env, u8 second) {
    // A message file, which the command ignores
    ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 secondMsgId = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 frameType = ScriptReadAny(vm, env);

    if (second) {
        msgId = secondMsgId;
    }
    return CallFieldActorMessageDisp(vm, env, MSGFILE_SCRIPT, msgId, actorId, pos, frameType);
}

BOOL Scrcmd_Msg_FunfestDispSalesmanMessage(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u32 count = FesGimmick_IsOnSale(Field_GetFesGimmick(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    u16 actorId = ScriptWork_ResolveHybridValue(work, FieldScriptEnv_GetGameData(env), 0x8011);
    u16 msgId = ScriptReadAny(vm, env);
    u16 step = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);

    msgId += count * step;
    return CallFieldActorMessageDisp(vm, env, MSGFILE_SCRIPT, msgId, actorId, pos, TALKMSG_WIN_FRAME_NORMAL);
}

BOOL Scrcmd_Msg_ActorMsgGendered(VM *vm, FieldScriptEnv *env) {
    PlayerState *playerState = GameData_GetPlayerState(FieldScriptEnv_GetGameData(env));

    return CallFieldActorMessageDispSelect(vm, env, getTrainerGender(&playerState->playerInfo) != 0);
}

BOOL Scrcmd_Msg_ActorMsgVersioned(VM *vm, FieldScriptEnv *env) {
#ifdef BLACK2
    return CallFieldActorMessageDispSelect(vm, env, TRUE);
#else
    return CallFieldActorMessageDispSelect(vm, env, FALSE);
#endif
}

BOOL Scrcmd_Msg_ActorMsgClose(VM *vm, FieldScriptEnv *env) {
    ActorMsgWin_CloseAsync(vm, env);
    return TRUE;
}

BOOL FieldScriptSubEventFinish_ActorMsg(FinishScriptSubEventsWork *work, u32 *state) {
    ActorMsgWin_CloseImmediate(work->env);
    return TRUE;
}

BOOL Scrcmd_Msg_TrainerSayMessage(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 trainerId = ScriptReadAny(vm, env);
    u16 msgType = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 scriptId = FldAct_GetSCRID(ScriptWork_GetParentActor(work));
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(work);
    ScriptActorWork *actorWork;
    FieldPlayer *player;
    MMSys *mmSys;
    FieldActor *playerActor;
    VecFx32 actorPos;
    u8 winPos;
    u16 type;

    if (scriptId == SCRID_SPECIAL_TRAINER) {
        trainerId += 0x500;
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        return TRUE;
    }
    TrainerMsg_Load(trainerId, msgType, strbuf, FieldScriptEnv_GetHeapID(env));
    if (FindFieldActor(GetScrEnvActorSys(env), actorId) == NULL) {
        actorId = ACTOR_UID_PLAYER;
    }
    actorWork = GetFieldScriptActorWk(env);
    type = ACTOR_MSG_WIN_TOP;
    MI_CpuFill8(actorWork, 0, sizeof(ScriptActorWork));
    actorWork->actorId = actorId;
    player = Field_GetPlayer(fieldWork->field);
    mmSys = GetScrEnvActorSys(env);
    playerActor = FieldPlayer_GetActor(player);
    CopyActorWPos(FindFieldActor(mmSys, actorWork->actorId), &actorPos);
    winPos = ActorMsgWin_CalcWinPosAuto(playerActor, &actorPos);
    if (winPos != MSG_WIN_POS_TOP) {
        type = ACTOR_MSG_WIN_BOTTOM;
    }
    actorWork->winType = type;
    ActorMsgWin_CalcAttachmentPos(env, TRUE);
    ActorMsgWin_CreateWindow(env, fieldWork->msgBGSys, type, &actorWork->pos, strbuf, TALKMSG_WIN_FRAME_NORMAL,
                             TALKMSG_WIN_TAIL_NONE);
    ActorMsgWin_SetPosActual(env, winPos);
    VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
    return TRUE;
}

static BOOL ScriptNative_InfoMsgWinWait(VM *vm, void *env) {
    if (MsgBGPrinter_PlainWinMain(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL displayText(VM *vm, FieldScriptEnv *env, u32 frameType) {
    ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    u8 pos = VM_Read8(vm);

    return loadMsgBox(vm, env, LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId), frameType, pos);
}

// Shows message in the info window at the top (MSG_WIN_POS_TOP) or the bottom of the screen, framed in a
// TALKMSG_WIN_FRAME_ style
BOOL loadMsgBox(VM *vm, FieldScriptEnv *env, StrBuf *message, u32 frameType, u8 pos) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    InfoMsgWin *window;
    u16 row = MSG_WIN_ROW_TOP;

    if (pos != MSG_WIN_POS_TOP) {
        row = MSG_WIN_ROW_BOTTOM;
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MSG)) {
        window = getMapDisplayInfoPtr(env);
        MsgBGPrinter_ClearPlainWin(window);
    } else {
        window = MsgBGPrinter_CreatePlainWin(fieldWork->msgBGSys, NULL, frameType, 1, row, 0x1e, 4);
        setMapDisplayInfoPtr(env, window);
    }
    MsgBGPrinter_PlainWinPrintStream(window, 8, 0, message);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_MSG);
    VM_SetNativeCallback(vm, ScriptNative_InfoMsgWinWait);
    return TRUE;
}

static void InfoMsgWin_Close(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MSG)) {
        MsgBGPrinter_DeletePlainWin(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_MSG);
    }
}

BOOL Scrcmd_Msg_InfoMsg(VM *vm, FieldScriptEnv *env) {
    return displayText(vm, env, TALKMSG_WIN_FRAME_NORMAL);
}

BOOL Scrcmd_Msg_ScreamMsg(VM *vm, FieldScriptEnv *env) {
    return displayText(vm, env, TALKMSG_WIN_FRAME_SPIKY);
}

BOOL Scrcmd_Msg_InfoMsgClose(VM *vm, FieldScriptEnv *env) {
    InfoMsgWin_Close(env);
    return FALSE;
}

BOOL FieldScriptSubEventFinish_InfoMsg(FinishScriptSubEventsWork *work, u32 *state) {
    InfoMsgWin_Close(work->env);
    return TRUE;
}

BOOL Scrcmd_Msg_MultiMsg(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u8 width;
    u8 height = 2;
    StrBuf *strbuf = GFL_StrBufCreate(0x40, FieldScriptEnv_GetHeapID(env));

    setupFormattedTextBuffer(env, strbuf, MSGFILE_SCRIPT, msgId);
    CalcMsgWindowDimensions(fieldWork->msgBGSys, strbuf, &width, &height);
    if (x + width > 31) {
        x = 31 - width;
    }
    if (y + height > 24) {
        y = height = 4;
    }
    MsgBGPrinter_AddSubMsgWinStr(fieldWork->msgBGSys, strbuf, index, x, y, width, height);
    GFL_StrBufFree(strbuf);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_MULTI_MSG);
    return FALSE;
}

BOOL Scrcmd_Msg_MsgWinCloseNo(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    MsgBGPrinter_DeleteSubMsgWin(fieldWork->msgBGSys, VM_Read16(vm));
    if (!MsgBGPrinter_ExistsSubMsgWin(fieldWork->msgBGSys)) {
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_MULTI_MSG);
    }
    return FALSE;
}

static void NumberedMsgWins_CloseAll(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MULTI_MSG)) {
        ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

        MsgBGPrinter_DeleteAllSubMsgWin(fieldWork->msgBGSys);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_MULTI_MSG);
    }
}

BOOL FieldScriptSubEventFinish_MultiMsg(FinishScriptSubEventsWork *work, u32 *state) {
    NumberedMsgWins_CloseAll(work->env);
    return TRUE;
}

static BOOL ScriptNative_SignMsgWinWait(VM *vm, void *env) {
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(FieldScriptEnv_GetScriptWork(env));

    if (MsgBGPrinter_BgWinMain(getMapDisplayInfoPtr(env), strbuf) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void SignMsgWin_Close(FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Font *font;

    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SIGN)) {
        MsgBGPrinter_DeleteBgWin(getMapDisplayInfoPtr(env));
        font = ScriptWork_GetSignFont(work);
        ScriptWork_SetSignFont(work, NULL);
        if (font != NULL) {
            GFL_FontFree(font);
        }
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_SIGN);
    }
}

BOOL Scrcmd_Msg_MsgPlaceSign(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 type = VM_Read16(vm);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    GetFieldScriptMsgData(env);

    setMapDisplayInfoPtr(env, MsgBGPrinter_CreateBgWin(fieldWork->msgBGSys, type));
    LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId);
    VM_SetNativeCallback(vm, ScriptNative_SignMsgWinWait);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_SIGN);
    return TRUE;
}

// A sign in font 4, which only the Abyssal Ruins' scripts use
BOOL Scrcmd_Msg_MsgPlaceSignFont4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 msgId = ScriptReadAny(vm, env);
    u16 type = VM_Read16(vm);
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    ScriptFieldWork *fieldWork2 = ScriptWork_GetFieldWork(work);
    GetFieldScriptMsgData(env);
    Font *font = GFL_FontCreate(ARCID_FONT, 4, 0, FALSE, heapId);

    ScriptWork_SetSignFont(work, font);
    setMapDisplayInfoPtr(env, MsgBGPrinter_CreateBgWinFont(fieldWork2->msgBGSys, type, font));
    LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId);
    VM_SetNativeCallback(vm, ScriptNative_SignMsgWinWait);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_SIGN);
    return TRUE;
}

BOOL Scrcmd_Msg_MsgPlaceSignClose(VM *vm, FieldScriptEnv *env) {
    SignMsgWin_Close(env);
    return FALSE;
}

BOOL FieldScriptSubEventFinish_Sign(FinishScriptSubEventsWork *work, u32 *state) {
    SignMsgWin_Close(work->env);
    return TRUE;
}

static void CheckerMsgWin_Close(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_CHECKER_MSG)) {
        CheckerMsgWin_Free(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_CHECKER_MSG);
    }
}

static BOOL ScriptNative_CheckerMsgWinWait(VM *vm, void *env) {
    if (CheckerMsgWin_Update(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Msg_CheckerMsg(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 msgId = VM_Read16(vm);
    u8 x = VM_Read8(vm);
    u8 y = VM_Read8(vm);
    u16 type = VM_Read16(vm);
    StrBuf *strbuf = LoadFieldScriptMessage(env, MSGFILE_SCRIPT, msgId);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 width = CheckerMsgWin_CalcWidth(fieldWork->msgBGSys, strbuf, 4);
    u16 height = CheckerMsgWin_CalcHeight(fieldWork->msgBGSys, strbuf, 4);
    CheckerMsgWin *window = CheckerMsgWin_Create(fieldWork->msgBGSys, type, x, y, width, height);

    setMapDisplayInfoPtr(env, window);
    CheckerMsgWin_PrintStrBuf(window, 4, 4, strbuf);
    VM_SetNativeCallback(vm, ScriptNative_CheckerMsgWinWait);
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_CHECKER_MSG);
    return TRUE;
}

BOOL Scrcmd_Msg_CheckerMsgClose(VM *vm, FieldScriptEnv *env) {
    CheckerMsgWin_Close(env);
    return FALSE;
}

BOOL FieldScriptSubEventFinish_CheckerMsg(FinishScriptSubEventsWork *work, u32 *state) {
    CheckerMsgWin_Close(work->env);
    return TRUE;
}

BOOL Scrcmd_Msg_MsgWinCloseAll(VM *vm, FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SYSTEM_MSG)) {
        SystemMsgWin_Close(env);
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MSG)) {
        InfoMsgWin_Close(env);
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SIGN)) {
        SignMsgWin_Close(env);
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_CHECKER_MSG)) {
        CheckerMsgWin_Close(env);
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        ActorMsgWin_CloseAsync(vm, env);
    }
    return TRUE;
}

// Waits for A or B, and skips the open window's printing to the end
static BOOL ScriptNative_AdvanceMessages(VM *vm, void *env) {
    u32 keys = GCTX_HIDGetPressedKeys();
    void *window = getMapDisplayInfoPtr(env);

    if (keys & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        return TRUE;
    }
    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_SYSTEM_MSG)) {
        MsgBGPrinter_SysWinDrawCursor(window);
    } else if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MSG)) {
        MsgBGPrinter_PlainWinDrawCursor(window);
    } else if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_ACTOR_MSG)) {
        MsgBGPrinter_TalkWinDrawCursor(window);
    } else {
        return TRUE;
    }
    return FALSE;
}

BOOL Scrcmd_Msg_MsgWaitAdvance(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_AdvanceMessages);
    return TRUE;
}

BOOL Scrcmd_Msg_MsgSetAutoscrolls(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    if (VM_Read16(vm)) {
        MsgBGPrinter_SetAutoMode(fieldWork->msgBGSys, TRUE);
        FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_AUTOSCROLL);
    } else {
        MsgBGPrinter_SetAutoMode(fieldWork->msgBGSys, FALSE);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_AUTOSCROLL);
    }
    return FALSE;
}

BOOL ActorMsgWin_CleanupAutoMode(FinishScriptSubEventsWork *work, u32 *state) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work->scriptWork);

    if (FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_AUTOSCROLL)) {
        MsgBGPrinter_SetAutoMode(fieldWork->msgBGSys, FALSE);
        FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_AUTOSCROLL);
    }
    return TRUE;
}

// Loads a message of the script's message data (MSGFILE_SCRIPT) or another file, and expands its words into dest
static StrBuf *setupFormattedTextBuffer(FieldScriptEnv *env, StrBuf *dest, u32 fileNo, u32 msgId) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MsgData *msgData = GetFieldScriptMsgData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    MsgData *fileMsgData;

    if (fileNo == MSGFILE_SCRIPT) {
        GFL_MsgDataLoadStrbuf(msgData, msgId, text);
    } else {
        fileMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, fileNo, FieldScriptEnv_GetHeapID(env));
        GFL_MsgDataLoadStrbuf(fileMsgData, msgId, text);
        GFL_MsgDataFree(fileMsgData);
    }
    GFL_WordSetFormatStrbuf(wordSet, dest, text);
    return dest;
}

// The message in the script's main string buffer
static StrBuf *LoadFieldScriptMessage(FieldScriptEnv *env, u32 fileNo, u32 msgId) {
    return setupFormattedTextBuffer(env, ScriptWork_GetMainStrBuf(FieldScriptEnv_GetScriptWork(env)), fileNo, msgId);
}
