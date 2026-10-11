// The script commands of the message windows: the system, money, info, sign and checker windows, the balloons over
// actors and Trainers, the numbered windows, and the loading and formatting of the script's messages. The ROM has no
// name for the file; scrcmd_msg.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "battle/trainer_data.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_script.h"
#include "field/player_state.h"
#include "field/scrcmd_msg.h"
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

// The message file of the running script, for setupFormattedTextBuffer and LoadFieldScriptMessage
#define SCRIPT_MSG_FILE 0x400

static StrBuf *setupFormattedTextBuffer(FieldScriptEnv *env, StrBuf *dest, u32 fileNo, u32 msgId);
static StrBuf *LoadFieldScriptMessage(FieldScriptEnv *env, u32 fileNo, u32 msgId);

// The up axis the balloon's offset is computed against
static const VecFx32 ACTOR_MSGWIN_CALC_AXIS_Y = {0, FX32_ONE, 0};

// The offset of a balloon from its actor, by the half of the screen the actor is in and the column of the screen
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

// Opens the system message window at the top (1) or the bottom (2) of the screen, or for 0 away from the balloon
// over an actor
static void SystemMsgWin_Open(FieldScriptEnv *env, u8 pos) {
    ScriptFieldWork *fieldWork;
    MsgData *msgData;
    void *window;
    u32 row = 1;

    if (FieldScriptSubEvent_IsRegistered(1)) {
        func_ov036_02188660(getMapDisplayInfoPtr(env));
        return;
    }
    fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    msgData = GetFieldScriptMsgData(env);
    if (pos == 1) {
        // The top, as row says
    } else if (pos == 2) {
        row = 0x13;
    } else if (pos == 0) {
        switch (ActorMsgWin_GetPosActual(env)) {
        case 1:
        case 3:
        case 5:
            pos = 1;
            break;
        case 2:
        case 4:
        case 6:
            row = 0x13;
            pos = 2;
            break;
        case 7:
            row = 0x13;
            pos = 2;
            break;
        default:
            row = 0x13;
            pos = 2;
            break;
        }
    } else {
        row = 0x13;
        pos = 2;
    }
    window = func_ov036_02188498(fieldWork->msgBGSys, msgData, row);
    ActorMsgWin_SetPosActual(env, pos);
    setMapDisplayInfoPtr(env, window);
    FieldScriptSubEvent_Register(1);
}

static void func_ov036_021a86b8(FieldScriptEnv *env) {
    WaitIcon *icon;

    if (FieldScriptSubEvent_IsRegistered(1)) {
        func_ov036_02188504(getMapDisplayInfoPtr(env));
        icon = func_ov012_021551c0(env);
        if (icon != NULL) {
            WaitIcon_Free(icon);
            SetSpecialMessageIconPtr(env, NULL);
        }
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(1);
    }
}

static BOOL ScriptNative_SystemMsgWinWait(VM *vm, void *env) {
    if (func_ov036_021885bc(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL s0034_SystemMsg(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 msgId = ScriptReadAny(vm, env);
    StrBuf *strbuf;

    SystemMsgWin_Open(env, VM_Read16(vm));
    strbuf = LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId);
    func_ov036_02188580(getMapDisplayInfoPtr(env), 0, 0, strbuf);
    VM_SetNativeCallback(vm, ScriptNative_SystemMsgWinWait);
    return TRUE;
}

BOOL s0035_SystemMsgAsync(VM *vm, FieldScriptEnv *env) {
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    void *window;

    SystemMsgWin_Open(env, VM_Read16(vm));
    LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId);
    window = getMapDisplayInfoPtr(env);
    func_ov036_02188660(window);
    func_ov036_02188680(window, 0, 0, strbuf);
    return FALSE;
}

BOOL s0037_MsgSetLoadingSpinner(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = Field_GetHeapID(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 hide = ScriptReadAny(vm, env);
    WaitIcon *icon;
    BmpWin *window;

    if (!FieldScriptSubEvent_IsRegistered(1)) {
        return FALSE;
    }
    icon = func_ov012_021551c0(env);
    if (icon != NULL) {
        WaitIcon_Free(icon);
        SetSpecialMessageIconPtr(env, NULL);
    }
    if (hide == 0) {
        window = func_ov036_021886b0(getMapDisplayInfoPtr(env));
        SetSpecialMessageIconPtr(env, WaitIcon_Create(GFL_VBlankGetTCBMgr(), window, 0xf, 0x10, heapId));
    }
    return FALSE;
}

BOOL s0036_InfoMsgClose(VM *vm, FieldScriptEnv *env) {
    func_ov036_021a86b8(env);
    return FALSE;
}

BOOL func_ov036_021a8844(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a86b8(work->env);
    return TRUE;
}

BOOL s0040_MoneyWinDisp(VM *vm, FieldScriptEnv *env) {
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    Field *field = GSYS_GetField(gsys);
    void *msgBGSys = Field_GetMsgBGSys(field);
    void *moneyWin = Field_GetMoneyWin(field);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    HeapID heapId = Field_GetHeapID(field);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    StrBuf *strbuf = GFL_StrBufCreate(0x80, heapId);
    u32 cash = getCash(getTrainerCardDataBlkAddress(gameData));

    if (moneyWin != NULL) {
        func_ov036_02187c1c(moneyWin);
    }
    moneyWin = FieldMsgBG_CreateMoneyWin(msgBGSys, msgData, x - 9, y, 9, 2);
    Field_SetMoneyWin(field, moneyWin);
    WordSetNumber(wordSet, 2, cash, 7, 1, TRUE);
    GFL_MsgDataLoadStrbuf(msgData, Global10520_Text_Empty_2, text);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    func_ov036_02187c4c(moneyWin, 0, 0, strbuf);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

BOOL s0042_MoneyWinUpdate(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    Field *field = GSYS_GetField(gsys);
    void *moneyWin = Field_GetMoneyWin(field);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    HeapID heapId = Field_GetHeapID(field);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, heapId);
    StrBuf *strbuf = GFL_StrBufCreate(0x80, heapId);

    WordSetNumber(wordSet, 2, getCash(getTrainerCardDataBlkAddress(gameData)), 7, 1, TRUE);
    GFL_MsgDataLoadStrbuf(msgData, Global10520_Text_Empty_2, text);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, text);
    func_ov036_02187c7c(moneyWin);
    func_ov036_02187c4c(moneyWin, 0, 0, strbuf);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

BOOL s0041_MoneyWinClose(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    void *moneyWin = Field_GetMoneyWin(field);

    if (moneyWin != NULL) {
        func_ov036_02187c1c(moneyWin);
        Field_SetMoneyWin(field, NULL);
    }
    return FALSE;
}

// The offset of a balloon over an actor at pos, by where on the screen the actor is
static void ActorMsgWin_CalcAttachmentOffsetDynamic(const VecFx32 *pos, VecFx32 *offset, G3DCamera *camera) {
    VecFx32 world = *pos;
    VecFx32 dir;
    VecFx32 flatDir;
    VecFx32 side;
    VecFx32 up;
    VecFx32 camPos;
    VecFx32 target;
    int x;
    int y;
    u32 column;
    u32 half;

    GFL_G3DCameraFlush(camera);
    GFL_G3DCameraGetLookatTarget(camera, &target);
    GFL_G3DCameraGetLookatPos(camera, &camPos);
    VEC_Subtract(&target, &camPos, &dir);
    flatDir = dir;
    flatDir.y = 0;
    vecfx_normalize(&flatDir, &flatDir);
    vecfx_cross(&flatDir, &ACTOR_MSGWIN_CALC_AXIS_Y, &side);
    vecfx_cross(&dir, &side, &up);
    vecfx_normalize(&up, &up);
    world.x += up.x;
    world.y += up.y;
    world.z += up.z;
    NNS_G3DProject(&world, &x, &y);
    if ((u32)x / 8 < 10) {
        column = 0;
    } else if ((u32)x / 8 >= 10 && (u32)x / 8 <= 15) {
        column = 1;
    } else if ((u32)x / 8 >= 16 && (u32)x / 8 < 22) {
        column = 2;
    } else {
        column = 3;
    }
    half = (u32)y / 8 / 12;
    if (column < 4 && half < 2) {
        *offset = ACTOR_MSGWIN_ORIGIN_OFFSET_TABLE[half][column];
    } else {
        VecFx32 zero = {0, 0, 0};

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
    MAT43_LookAt(&camPos, &camUp, &target, &lookAt);
    MI_Copy36B(&lookAt, &rot);
    MAT3_Invert(&rot, &rot);
    MAT3_MulVec(vec, &rot, vec);
}

static void ActorMsgWin_GetAttachmentOffset(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera,
                                            FieldCamera *camera, u8 winPos) {
    u32 half = 0;
    u32 column = 0;

    switch (winPos) {
    case 3:
        column = 1;
        break;
    case 4:
        column = 1;
        half = 1;
        break;
    case 5:
        column = 2;
        break;
    case 6:
        column = 2;
        half = 1;
        break;
    case 0:
    case 1:
    case 2:
        ActorMsgWin_CalcAttachmentOffsetDynamic(pos, offset, g3dCamera);
        ActorMsgWin_MulVecInvCamRot(offset, g3dCamera);
        return;
    }
    *offset = ACTOR_MSGWIN_ORIGIN_OFFSET_TABLE[half][column];
    ActorMsgWin_MulVecInvCamRot(offset, g3dCamera);
}

void func_ov036_021a8bec(const VecFx32 *pos, VecFx32 *offset, G3DCamera *g3dCamera, FieldCamera *camera, u8 winPos) {
    ActorMsgWin_GetAttachmentOffset(pos, offset, g3dCamera, camera, winPos);
}

void func_ov036_021a8c00(u32 winPos, u32 *a1, u32 *a2) {
    *a1 = 0;
    *a2 = 0;
    switch (winPos) {
    case 3:
        *a2 = 7;
        break;
    case 4:
        *a1 = 1;
        *a2 = 5;
        break;
    case 5:
        *a2 = 8;
        break;
    case 6:
        *a1 = 1;
        *a2 = 6;
        break;
    case 2:
        *a1 = 1;
        break;
    case 0:
        *a1 = 1;
        *a2 = 2;
        break;
    }
}

u8 ActorMsgWin_CalcWinPosAuto(FieldActor *player, const VecFx32 *pos) {
    VecFx32 playerPos;
    u8 winPos = 2;

    CopyActorWPos(player, &playerPos);
    if (pos->z < playerPos.z) {
        winPos = 1;
    }
    return winPos;
}

static BOOL func_ov036_021a8c74(VM *vm, void *env) {
    if (func_ov036_021887f4(getMapDisplayInfoPtr(env)) == TRUE) {
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(2);
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021a8c9c(VM *vm, FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(2)) {
        func_ov036_021887d4(getMapDisplayInfoPtr(env));
        VM_SetNativeCallback(vm, func_ov036_021a8c74);
    }
}

static void func_ov036_021a8cc4(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(2)) {
        func_ov036_02188818(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(2);
    }
}

static void ActorMsgWin_CreateWindow(FieldScriptEnv *env, void *msgBGSys, u32 a2, const VecFx32 *pos, StrBuf *strbuf,
                                     u32 a5, u32 a6) {
    setMapDisplayInfoPtr(env, ActorMsgWin_CheckAndCreate(msgBGSys, a2, pos, strbuf, a5, a6));
    FieldScriptSubEvent_Register(2);
}

// Moves the balloon with its actor. force recomputes the offset even when the actor hasn't moved
static void ActorMsgWin_CalcAttachmentPos(FieldScriptEnv *env, BOOL force) {
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    ScriptActorWork *actorWork = GetFieldScriptActorWk(env);
    FieldActor *actor = FindFieldActor(mmSys, actorWork->actorId);
    VecFx32 pos;
    FieldCamera *camera;
    G3DCamera *g3dCamera;

    if (actor != NULL) {
        GetActorFaceDir(actor);
        CopyActorWPos(actor, &pos);
        if (force == TRUE || pos.x != actorWork->actorPos.x || pos.y != actorWork->actorPos.y
            || pos.z != actorWork->actorPos.z) {
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

// Shows strbuf in a balloon over the actor, at pos, or where CalcWinPosAuto puts it for 0
static BOOL SetupActorMessageWindow(FieldScriptEnv *env, StrBuf *strbuf, u16 actorId, u32 pos, u32 a4) {
    u32 winPos = pos;
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    MMSys *mmSys = GetScrEnvMMdlSys(env);
    FieldActor *player = FindFieldActor(mmSys, 0xff);
    FieldActor *actor = FindFieldActor(mmSys, actorId);
    ScriptActorWork *actorWork;
    VecFx32 actorPos;
    u32 winX;
    u32 winY;

    if (actor == NULL) {
        return FALSE;
    }
    if (FieldScriptSubEvent_IsRegistered(2)) {
        func_ov036_02188844(getMapDisplayInfoPtr(env), strbuf);
    } else {
        actorWork = GetFieldScriptActorWk(env);
        sys_memset(actorWork, 0, sizeof(ScriptActorWork));
        actorWork->actorId = actorId;
        if (pos == 0) {
            CopyActorWPos(actor, &actorPos);
            winPos = ActorMsgWin_CalcWinPosAuto(player, &actorPos);
        }
        func_ov036_021a8c00(winPos, &winX, &winY);
        actorWork->winX = winX;
        ActorMsgWin_SetPosActual(env, winPos);
        ActorMsgWin_SetPos(env, pos);
        ActorMsgWin_CalcAttachmentPos(env, TRUE);
        ActorMsgWin_CreateWindow(env, fieldWork->msgBGSys, winX, &actorWork->pos, strbuf, a4, winY);
    }
    return TRUE;
}

static BOOL ScriptNative_ActorMsgWinWait(VM *vm, void *env) {
    void *msgWin = getMapDisplayInfoPtr(env);

    ActorMsgWin_CalcAttachmentPos(env, FALSE);
    BOOL done = FALSE;
    if (func_ov036_02188884(msgWin) == TRUE) {
        done = TRUE;
    }
    return done;
}

BOOL func_ov036_021a8eb4(VM *vm, FieldScriptEnv *env, StrBuf *message, u16 actorId, u16 pos, u32 a5) {
    if (SetupActorMessageWindow(env, message, actorId, pos, a5) == TRUE) {
        VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
        return TRUE;
    }
    return FALSE;
}

static BOOL CallFieldActorMessageDisp(VM *vm, FieldScriptEnv *env, u16 fileNo, u16 msgId, u16 actorId, u16 pos,
                                      u32 a6) {
    if (SetupActorMessageWindow(env, LoadFieldScriptMessage(env, fileNo, msgId), actorId, pos, a6) == TRUE) {
        VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
        return TRUE;
    }
    return FALSE;
}

BOOL s003C_ActorMsg(VM *vm, FieldScriptEnv *env) {
    u16 fileNo = ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 a6 = ScriptReadAny(vm, env);

    return CallFieldActorMessageDisp(vm, env, fileNo, msgId, actorId, pos, a6);
}

BOOL s003D_ParentActorMsg(VM *vm, FieldScriptEnv *env) {
    FieldActor *actor = ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env));
    u16 fileNo = ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 a6 = ScriptReadAny(vm, env);

    if (actor == NULL) {
        return TRUE;
    }
    return CallFieldActorMessageDisp(vm, env, fileNo, msgId, (u8)GetActorUID(actor), pos, a6);
}

// Shows the first or the second message
static BOOL func_ov036_021a8fd8(VM *vm, FieldScriptEnv *env, u8 second) {
    // A message file, which the command ignores
    u16 unused = ScriptReadAny(vm, env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 secondMsgId = ScriptReadAny(vm, env);
    u16 actorId = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);
    u16 a6 = ScriptReadAny(vm, env);

    if (second) {
        msgId = secondMsgId;
    }
    return CallFieldActorMessageDisp(vm, env, SCRIPT_MSG_FILE, msgId, actorId, pos, a6);
}

BOOL s0278_FunfestDispSalesmanMessage(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u32 count = func_ov036_021b67bc(Field_GetFesGimmick(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    u16 actorId = ScriptWork_ResolveHybridValue(work, FieldScriptEnv_GetGameData(env), 0x8011);
    u16 msgId = ScriptReadAny(vm, env);
    u16 step = ScriptReadAny(vm, env);
    u16 pos = ScriptReadAny(vm, env);

    msgId += (u16)(count * step);
    return CallFieldActorMessageDisp(vm, env, SCRIPT_MSG_FILE, msgId, actorId, pos, 0);
}

BOOL s0048_ActorMsgGendered(VM *vm, FieldScriptEnv *env) {
    PlayerState *playerState = GameData_GetPlayerState(FieldScriptEnv_GetGameData(env));

    return func_ov036_021a8fd8(vm, env, getTrainerGender(&playerState->playerInfo) != 0);
}

BOOL s0049_ActorMsgVersioned(VM *vm, FieldScriptEnv *env) {
#ifdef BLACK2
    return func_ov036_021a8fd8(vm, env, TRUE);
#else
    return func_ov036_021a8fd8(vm, env, FALSE);
#endif
}

BOOL s003E_ActorMsgClose(VM *vm, FieldScriptEnv *env) {
    func_ov036_021a8c9c(vm, env);
    return TRUE;
}

BOOL func_ov036_021a90fc(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a8cc4(work->env);
    return TRUE;
}

BOOL s0087_TrainerSayMessage(VM *vm, FieldScriptEnv *env) {
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
    u16 winX;

    if (scriptId == 0x29f9) {
        trainerId += 0x500;
    }
    if (FieldScriptSubEvent_IsRegistered(2)) {
        return TRUE;
    }
    TrainerMsg_Load(trainerId, msgType, strbuf, FieldScriptEnv_GetHeapID(env));
    if (FindFieldActor(GetScrEnvMMdlSys(env), actorId) == NULL) {
        actorId = 0xff;
    }
    actorWork = GetFieldScriptActorWk(env);
    winX = 0;
    sys_memset(actorWork, 0, sizeof(ScriptActorWork));
    actorWork->actorId = actorId;
    player = Field_GetPlayer(fieldWork->field);
    mmSys = GetScrEnvMMdlSys(env);
    playerActor = FieldPlayer_GetActor(player);
    CopyActorWPos(FindFieldActor(mmSys, actorWork->actorId), &actorPos);
    winPos = ActorMsgWin_CalcWinPosAuto(playerActor, &actorPos);
    if (winPos != 1) {
        winX = 1;
    }
    actorWork->winX = winX;
    ActorMsgWin_CalcAttachmentPos(env, TRUE);
    ActorMsgWin_CreateWindow(env, fieldWork->msgBGSys, winX, &actorWork->pos, strbuf, 0, 0);
    ActorMsgWin_SetPosActual(env, winPos);
    VM_SetNativeCallback(vm, ScriptNative_ActorMsgWinWait);
    return TRUE;
}

static BOOL func_ov036_021a9228(VM *vm, void *env) {
    if (func_ov036_02188bdc(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL displayText(VM *vm, FieldScriptEnv *env, u32 a2) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    u8 a4 = VM_Read8(vm);

    return loadMsgBox(vm, env, LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId), a2, a4);
}

BOOL loadMsgBox(VM *vm, FieldScriptEnv *env, StrBuf *message, u32 a3, u8 a4) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    void *window;
    u32 row = 1;

    if (a4 != 1) {
        row = 0x13;
    }
    if (FieldScriptSubEvent_IsRegistered(3)) {
        window = getMapDisplayInfoPtr(env);
        func_ov036_02188ae8(window);
    } else {
        window = func_ov036_02188a54(fieldWork->msgBGSys, 0, a3, 1, row, 0x1e, 4);
        setMapDisplayInfoPtr(env, window);
    }
    func_ov036_02188ba4(window, 8, 0, message);
    FieldScriptSubEvent_Register(3);
    VM_SetNativeCallback(vm, func_ov036_021a9228);
    return TRUE;
}

static void func_ov036_021a9300(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(3)) {
        func_ov036_02188ab0(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(3);
    }
}

BOOL s0038_InfoMsg(VM *vm, FieldScriptEnv *env) {
    return displayText(vm, env, 0);
}

BOOL s004A_ScreamMsg(VM *vm, FieldScriptEnv *env) {
    return displayText(vm, env, 1);
}

BOOL s0039_InfoMsgClose(VM *vm, FieldScriptEnv *env) {
    func_ov036_021a9300(env);
    return FALSE;
}

BOOL func_ov036_021a9350(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a9300(work->env);
    return TRUE;
}

BOOL s003A_MultiMsg(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    u16 msgId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 index = ScriptReadAny(vm, env);
    u8 width;
    u8 height = 2;
    StrBuf *strbuf = GFL_StrBufCreate(0x40, FieldScriptEnv_GetHeapID(env));

    setupFormattedTextBuffer(env, strbuf, SCRIPT_MSG_FILE, msgId);
    CalcMsgWindowDimensions(fieldWork->msgBGSys, strbuf, &width, &height);
    if (x + width > 31) {
        x = 31 - width;
    }
    if (y + height > 24) {
        y = height = 4;
    }
    func_ov036_02188ddc(fieldWork->msgBGSys, strbuf, index, x, y, width, height);
    GFL_StrBufFree(strbuf);
    FieldScriptSubEvent_Register(5);
    return FALSE;
}

BOOL s003B_MsgWinCloseNo(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    func_ov036_02188e90(fieldWork->msgBGSys, VM_Read16(vm));
    if (!func_ov036_02188ed0(fieldWork->msgBGSys)) {
        FieldScriptSubEvent_Unregister(5);
    }
    return FALSE;
}

static void func_ov036_021a9450(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(5)) {
        ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

        func_ov036_02188e9c(fieldWork->msgBGSys);
        FieldScriptSubEvent_Unregister(5);
    }
}

BOOL func_ov036_021a9478(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a9450(work->env);
    return TRUE;
}

static BOOL func_ov036_021a9484(VM *vm, void *env) {
    StrBuf *strbuf = ScriptWork_GetMainStrBuf(FieldScriptEnv_GetScriptWork(env));

    if (func_ov036_02189110(getMapDisplayInfoPtr(env), strbuf) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov036_021a94ac(FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Font *font;

    if (FieldScriptSubEvent_IsRegistered(4)) {
        func_ov036_0218903c(getMapDisplayInfoPtr(env));
        font = func_ov012_02153ed4(work);
        func_ov012_02153ed0(work, NULL);
        if (font != NULL) {
            GFL_FontFree(font);
        }
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(4);
    }
}

BOOL s0043_MsgPlaceSign(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 msgId = ScriptReadAny(vm, env);
    u16 type = VM_Read16(vm);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    MsgData *msgData = GetFieldScriptMsgData(env);

    setMapDisplayInfoPtr(env, func_ov036_02188f28(fieldWork->msgBGSys, type));
    LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId);
    VM_SetNativeCallback(vm, func_ov036_021a9484);
    FieldScriptSubEvent_Register(4);
    return TRUE;
}

// A sign in another font
BOOL func_ov036_021a9554(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 msgId = ScriptReadAny(vm, env);
    u16 type = VM_Read16(vm);
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    ScriptFieldWork *fieldWork2 = ScriptWork_GetFieldWork(work);
    MsgData *msgData = GetFieldScriptMsgData(env);
    Font *font = GFL_FontCreate(0x17, 4, 0, FALSE, heapId);

    func_ov012_02153ed0(work, font);
    setMapDisplayInfoPtr(env, func_ov036_02188f34(fieldWork2->msgBGSys, type, font));
    LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId);
    VM_SetNativeCallback(vm, func_ov036_021a9484);
    FieldScriptSubEvent_Register(4);
    return TRUE;
}

BOOL s0044_MsgPlaceSignClose(VM *vm, FieldScriptEnv *env) {
    func_ov036_021a94ac(env);
    return FALSE;
}

BOOL func_ov036_021a95ec(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a94ac(work->env);
    return TRUE;
}

static void func_ov036_021a95f8(FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(6)) {
        func_ov036_02189b50(getMapDisplayInfoPtr(env));
        setMapDisplayInfoPtr(env, NULL);
        FieldScriptSubEvent_Unregister(6);
    }
}

static BOOL func_ov036_021a9624(VM *vm, void *env) {
    if (func_ov036_02189c00(getMapDisplayInfoPtr(env)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL s0045_CheckerMsg(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 msgId = VM_Read16(vm);
    u8 x = VM_Read8(vm);
    u8 y = VM_Read8(vm);
    u16 type = VM_Read16(vm);
    StrBuf *strbuf = LoadFieldScriptMessage(env, SCRIPT_MSG_FILE, msgId);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 width = func_ov036_02189c34(fieldWork->msgBGSys, strbuf, 4);
    u16 height = func_ov036_02189c54(fieldWork->msgBGSys, strbuf, 4);
    void *window = func_ov036_02189a98(fieldWork->msgBGSys, type, x, y, width, height);

    setMapDisplayInfoPtr(env, window);
    func_ov036_02189bc4(window, 4, 4, strbuf);
    VM_SetNativeCallback(vm, func_ov036_021a9624);
    FieldScriptSubEvent_Register(6);
    return TRUE;
}

BOOL s0046_CheckerMsgClose(VM *vm, FieldScriptEnv *env) {
    func_ov036_021a95f8(env);
    return FALSE;
}

BOOL func_ov036_021a96ec(FinishScriptSubEventsWork *work, u32 *state) {
    func_ov036_021a95f8(work->env);
    return TRUE;
}

BOOL s003F_MsgWinCloseAll(VM *vm, FieldScriptEnv *env) {
    if (FieldScriptSubEvent_IsRegistered(1)) {
        func_ov036_021a86b8(env);
    }
    if (FieldScriptSubEvent_IsRegistered(3)) {
        func_ov036_021a9300(env);
    }
    if (FieldScriptSubEvent_IsRegistered(4)) {
        func_ov036_021a94ac(env);
    }
    if (FieldScriptSubEvent_IsRegistered(6)) {
        func_ov036_021a95f8(env);
    }
    if (FieldScriptSubEvent_IsRegistered(2)) {
        func_ov036_021a8c9c(vm, env);
    }
    return TRUE;
}

// Waits for A or B, and skips the open window's printing to the end
static BOOL ScriptNative_AdvanceMessages(VM *vm, void *env) {
    u32 keys = GCTX_HIDGetPressedKeys();
    void *window = getMapDisplayInfoPtr(env);

    if (keys & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GFL_SndSEPlay(0x547);
        return TRUE;
    }
    if (FieldScriptSubEvent_IsRegistered(1)) {
        func_ov036_02188630(window);
    } else if (FieldScriptSubEvent_IsRegistered(3)) {
        func_ov036_02188c90(window);
    } else if (FieldScriptSubEvent_IsRegistered(2)) {
        func_ov036_021889c8(window);
    } else {
        return TRUE;
    }
    return FALSE;
}

BOOL s004B_MsgWaitAdvance(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_AdvanceMessages);
    return TRUE;
}

BOOL s0033_MsgSetAutoscrolls(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));

    if (VM_Read16(vm)) {
        func_ov036_021879cc(fieldWork->msgBGSys, TRUE);
        FieldScriptSubEvent_Register(9);
    } else {
        func_ov036_021879cc(fieldWork->msgBGSys, FALSE);
        FieldScriptSubEvent_Unregister(9);
    }
    return FALSE;
}

BOOL func_ov036_021a9808(FinishScriptSubEventsWork *work, u32 *state) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work->scriptWork);

    if (FieldScriptSubEvent_IsRegistered(9)) {
        func_ov036_021879cc(fieldWork->msgBGSys, FALSE);
        FieldScriptSubEvent_Unregister(9);
    }
    return TRUE;
}

// Loads a message of the script's message data (SCRIPT_MSG_FILE) or another file, and expands its words into dest
static StrBuf *setupFormattedTextBuffer(FieldScriptEnv *env, StrBuf *dest, u32 fileNo, u32 msgId) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    MsgData *msgData = GetFieldScriptMsgData(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    StrBuf *text = ScriptWork_GetAltStrBuf(work);
    MsgData *fileMsgData;

    if (fileNo == SCRIPT_MSG_FILE) {
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
