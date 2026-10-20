// field_fade.c: the field's screen fade, and the field events that fade or move the screen as the player goes through
// the world: sinking into quicksand, falling down a hole, a spin, an Escape Rope, Dig and Teleport. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "field/field_event.h"
#include "field/field_g3d_mapper.h"
#include "field/field_prop.h"
#include "field/field_task.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "system/game_event.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 slow;
    u32 waitDone;
    u32 type;
    u32 stage;
} FieldFadeWork;

static void FieldFadeTCB_Callback(TCB *tcb, void *data);

void FieldFadeTCB_Start(GameSystem *gsys, Field *field, u32 type, u32 slow, u32 waitDone) {
    FieldFadeWork *wk;
    TCBManager *mgr = Field_GetTCBMgr(field);

    wk = GFL_HeapAllocate(4, sizeof(FieldFadeWork), TRUE, "field_fade.c", 56);
    GFL_TCBMgrAddTask(mgr, FieldFadeTCB_Callback, wk, 0);
    Field_SetFadeFlag(field, TRUE);
    wk->stage = 0;
    wk->gsys = gsys;
    wk->field = field;
    wk->slow = slow;
    wk->waitDone = waitDone;
    wk->type = type;
}

static void FieldFadeTCB_Callback(TCB *tcb, void *data) {
    FieldFadeWork *wk = data;

    switch (wk->stage) {
    case 0: {
        s32 speed;
        s32 start;
        s32 end;
        u32 mode;

        if (wk->type <= 1) {
            speed = -1;
        } else {
            speed = 4;
        }
        if (wk->type == 0 || wk->type == 2) {
            u32 enabled;

            start = 0x10;
            end = 0;
            enabled = GFL_BGSysGetEnabledBGsA();
            FieldG2D_SetLCDConfig();
            GFL_BGSysSetEnabledBGsA(enabled);
            FieldG2D_Prepare3DSurface(wk->field);
        } else {
            start = 0;
            end = 0x10;
        }
        if (wk->slow == 0) {
            mode = 3;
        } else {
            mode = 0xc;
        }
        GFL_FadeSet(mode, start, end, speed);
        wk->stage++;
        break;
    }
    case 1: {
        BOOL done = FALSE;

        if (wk->waitDone == 1) {
            done = TRUE;
        }
        if (!GFL_FadeIsRunning()) {
            done = TRUE;
        }
        if (done) {
            Field_SetFadeFlag(wk->field, FALSE);
            GFL_TCBRemove(tcb);
            GFL_HeapFree(wk);
        }
        break;
    }
    }
}

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 transformType;
} FallEventWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 transformType;
    FieldCameraAnimation anim;
    FieldCameraAnimationController *controller;
    u32 hasWarp;
    u32 warpA;
    u32 warpB;
} SpinEventWork;

static GameEventReturnCode EventQuicksandArrive_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov036_021b9764(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov036_021b98e4(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov036_021b9a64(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPlayerSpinDown_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov036_021b9c90(GameEvent *event, u32 *state, void *data);

GameEvent *EventQuicksandArrive_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_Callback, 0x68);
    FallEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

GameEvent *func_ov036_021b95ac(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged, u32 prevSeason,
                               u32 nextSeason) {
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021b9764, 0x68);
    SpinEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    wk->hasWarp = seasonChanged;
    wk->warpA = prevSeason;
    wk->warpB = nextSeason;
    return event;
}

GameEvent *func_ov036_021b95e0(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged, u32 prevSeason,
                               u32 nextSeason) {
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021b98e4, 0x68);
    SpinEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    wk->hasWarp = seasonChanged;
    wk->warpA = prevSeason;
    wk->warpB = nextSeason;
    return event;
}

GameEvent *func_ov036_021b9614(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021b9a64, 0x68);
    SpinEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

GameEvent *EventPlayerSpinDown_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventPlayerSpinDown_Callback, 0x68);
    FallEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

GameEvent *func_ov036_021b9664(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021b9c90, 0x68);
    FallEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

static GameEventReturnCode EventQuicksandArrive_Callback(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    FieldCamera *camera = Field_GetCameraSystem(wk->field);

    switch (*state) {
    case 0:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        {
            VecFx32 offset = { 0, 0xfa000, 0 };
            FieldActor *actor;
            FieldTask *task;

            actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));
            SetActorWPosOffset(actor, &offset);
            FldAct_SetAcmd(actor, 1);
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(wk->gsys, wk->field, 0, 1, 1, 0, 0));
            actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));
            task = FieldActorFallTask_Create(wk->field, actor, 0x28, 0xfa);
            FieldTaskManager_AddTask(Field_GetTaskManager(wk->field), task, 0);
        }
        (*state)++;
        break;
    case 1:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            (*state)++;
        }
        break;
    case 2:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021b986c(SpinEventWork *wk);
static void func_ov036_021b97f0(SpinEventWork *wk);
static GameEvent *func_ov036_021b9890(SpinEventWork *wk);
static void func_ov036_021b9858(SpinEventWork *wk);

static GameEventReturnCode func_ov036_021b9764(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case 0:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 1:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case 2:
        if (wk->hasWarp == 0) {
            func_ov036_021b986c(wk);
            func_ov036_021b97f0(wk);
        }
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, func_ov036_021b9890(wk));
        (*state)++;
        break;
    case 4:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case 5:
        func_ov036_021b9858(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021b97f0(SpinEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.unk48 = TRUE;
    wk->anim.unk4C = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
    wk->anim.src = wk->anim.dst;
    wk->anim.src.distance -= 0x64000;
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void func_ov036_021b9858(SpinEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static void func_ov036_021b986c(SpinEventWork *wk) {
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);
    FieldTask *task = FieldActorSpinTask_CreatePlayerDeaccel(wk->field, 0x3c, 8, 1);

    FieldTaskManager_AddTask(taskManager, task, 0);
}

static void func_ov036_021b98d4(void *data);

static GameEvent *func_ov036_021b9890(SpinEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->hasWarp != 0) {
        return func_ov036_021b8800(gsys, field, wk->warpA, wk->warpB, func_ov036_021b98d4, wk);
    }
    return CallFieldMapEntranceInTransition(gsys, field, 1, 1, 1, 0, 0);
}

static void func_ov036_021b98d4(void *data) {
    SpinEventWork *wk = data;

    func_ov036_021b986c(wk);
    func_ov036_021b97f0(wk);
}

static void func_ov036_021b9970(SpinEventWork *wk);
static GameEvent *func_ov036_021b9a10(SpinEventWork *wk);
static void func_ov036_021b99d8(SpinEventWork *wk);
static void func_ov036_021b99ec(SpinEventWork *wk);

static GameEventReturnCode func_ov036_021b98e4(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case 0:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 1:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case 2:
        if (wk->hasWarp == 0) {
            func_ov036_021b99ec(wk);
            func_ov036_021b9970(wk);
        }
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, func_ov036_021b9a10(wk));
        (*state)++;
        break;
    case 4:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case 5:
        func_ov036_021b99d8(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021b9970(SpinEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.unk48 = TRUE;
    wk->anim.unk4C = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
    wk->anim.src = wk->anim.dst;
    wk->anim.src.distance -= 0x64000;
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void func_ov036_021b99d8(SpinEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static void func_ov036_021b99ec(SpinEventWork *wk) {
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);
    FieldTask *task = FieldActorSpinTask_CreatePlayerDeaccel(wk->field, 0x3c, 8, 1);

    FieldTaskManager_AddTask(taskManager, task, 0);
}

static void func_ov036_021b9a54(void *data);

static GameEvent *func_ov036_021b9a10(SpinEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->hasWarp != 0) {
        return func_ov036_021b8800(gsys, field, wk->warpA, wk->warpB, func_ov036_021b9a54, wk);
    }
    return CallFieldMapEntranceInTransition(gsys, field, 1, 1, 1, 0, 0);
}

static void func_ov036_021b9a54(void *data) {
    SpinEventWork *wk = data;

    func_ov036_021b99ec(wk);
    func_ov036_021b9970(wk);
}

static GameEventReturnCode func_ov036_021b9a64(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case 0:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case 1:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case 2: {
        FieldCameraAnimationController *controller;

        FieldTaskManager_AddTask(taskManager, FieldActorSpinTask_CreatePlayerDeaccel(field, 0x3c, 8, 1), 0);
        controller = Field_CreateCameraAnimationController(field);
        wk->anim.frames = 0x3c;
        wk->anim.unk48 = TRUE;
        wk->anim.unk4C = TRUE;
        wk->controller = controller;
        FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
        FieldCameraAnimationController_PrepareCamera(wk->controller);
        FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
        wk->anim.src = wk->anim.dst;
        wk->anim.src.distance -= 0x64000;
        FieldCameraAnimationController_CommitSrcCoords(wk->controller);
        FieldCameraAnimationController_StartAnimation(wk->controller);
        (*state)++;
        break;
    }
    case 3:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 0, 1, 1, 0, 0));
        (*state)++;
        break;
    case 4:
        if (FieldTaskManager_IsIdle(taskManager)) {
            func_ov036_021a3e74(FieldPlayer_GetActor(Field_GetPlayer(field)), Field_GetFieldEffects(field));
            (*state)++;
        }
        break;
    case 5:
        FieldCameraAnimationController_FinishCamera(wk->controller);
        FieldCameraAnimationController_FreeAnimation(wk->controller);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventPlayerSpinDown_Callback(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);

    switch (*state) {
    case 0:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        {
            VecFx32 offset;
            FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(field));

            offset.y = 0xa000;
            offset.x = 0;
            offset.z = 0;
            SetActorWPosOffset(actor, &offset);
        }
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(wk->gsys, field, 0, 1, 1, 0, 0));
        {
            VecFx32 move;
            FieldTask *spin;
            FieldTask *moveTask;
            FieldTaskManager *taskManager;

            move.y = 0x96000;
            move.x = 0;
            move.z = 0;
            spin = FieldActorSpinTask_CreatePlayer(field, 0x18, 3);
            moveTask = FieldActorMoveTask_CreatePlayer(field, -0x18, &move);
            taskManager = Field_GetTaskManager(field);
            FieldTaskManager_AddTask(taskManager, spin, 0);
            FieldTaskManager_AddTask(taskManager, moveTask, 0);
        }
        (*state)++;
        break;
    case 1:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(field))) {
            (*state)++;
        }
        break;
    case 2:
        func_ov036_021a3e74(FieldPlayer_GetActor(Field_GetPlayer(field)), Field_GetFieldEffects(wk->field));
        GFL_SndSEPlay(0x67b);
        (*state)++;
        break;
    case 3:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode func_ov036_021b9c90(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));

    switch (*state) {
    case 0:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        FldAct_SetAcmd(actor, 0x44);
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(wk->gsys, wk->field, 0, 1, 1, 0, 0));
        (*state)++;
        break;
    case 1:
        if (FldAct_CheckAcmdEnd(actor) == 1) {
            FldAct_EndAcmd(actor);
            (*state)++;
        }
        break;
    case 2:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 transformType;
    u32 frame;
    BOOL teleportFlag;
    VecFx32 pos;
    FieldPropHandle *propHandle;
    FieldCameraAnimation anim;
    FieldCameraAnimationController *controller;
    s32 timer;
    BOOL seasonChanged;
} EffectEventWork;

static GameEventReturnCode EventQuicksandDrawIn_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventEscapeRope_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventDig_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventTeleportEffect_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov036_021ba4bc(GameEvent *event, u32 *state, void *data);

GameEvent *EventQuicksandDrawIn_Create(GameEvent *parent, GameSystem *gsys, Field *field, VecFx32 *pos) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandDrawIn_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->propHandle = NULL;
    wk->pos.x = pos->x;
    wk->pos.y = pos->y;
    wk->pos.z = pos->z;
    return event;
}

GameEvent *EventEscapeRope_Create(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventEscapeRope_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->timer = 0;
    wk->seasonChanged = seasonChanged;
    return event;
}

GameEvent *EventDig_Create(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventDig_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->timer = 0;
    wk->seasonChanged = seasonChanged;
    return event;
}

GameEvent *EventTeleportEffect_Create(GameEvent *parent, GameSystem *gsys, Field *field, BOOL a3) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventTeleportEffect_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->teleportFlag = a3;
    return event;
}

GameEvent *func_ov036_021b9df8(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, func_ov036_021ba4bc, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    return event;
}

static GameEventReturnCode EventQuicksandDrawIn_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldPlayer *player = Field_GetPlayer(wk->field);
    FieldActor *actor = FieldPlayer_GetActor(player);
    FieldEffects *effects = Field_GetFieldEffects(wk->field);
    FieldCamera *camera = Field_GetCameraSystem(wk->field);

    switch (*state) {
    case 0:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        *state = 1;
        break;
    case 1: {
        FieldPropSystem *propSystem;
        FieldPropAreaBounds bounds;
        FieldChunkPropHolder **props;
        u32 count;

        GFL_SEPlayKeepVol(0x680, GFL_SndSeqGetPlayerIndex(0x680));
        propSystem = FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(wk->field));
        bounds.minX = wk->pos.x - 0x20000;
        bounds.maxX = wk->pos.x + 0x20000;
        bounds.minZ = wk->pos.z - 0x20000;
        bounds.maxZ = wk->pos.z + 0x20000;
        props = FieldPropSystem_FindPropsInArea(propSystem, &bounds, 2, &count);
        if (*props != NULL) {
            wk->propHandle = FieldPropSystem_CreateHandleFromExisting(propSystem, *props);
            FieldPropHandle_CallAnmCmd(wk->propHandle, 0, 0);
        } else {
            wk->propHandle = NULL;
        }
        GFL_HeapFree(props);
        *state = 2;
        break;
    }
    case 2: {
        VecFx32 playerPos;
        s32 dx;
        s32 dz;
        s32 duration;

        FieldPlayer_GetWPos(player, &playerPos);
        dx = (playerPos.x >> 12) / 16 - (wk->pos.x >> 12) / 16;
        dz = (playerPos.z >> 12) / 16 - (wk->pos.z >> 12) / 16;
        if (dx < 0) {
            dx = -dx;
        }
        if (dz < 0) {
            dz = -dz;
        }
        if (dx == 0 && dz == 0) {
            duration = 0;
        } else if (dx < 2 && dz < 2) {
            duration = 0x3c;
        } else {
            duration = 0x78;
        }
        if (duration > 0) {
            FieldTask *task = FieldPlayerMoveTask_Create(wk->field, duration, &wk->pos);

            FieldTaskManager_AddTask(Field_GetTaskManager(wk->field), task, 0);
            *state = 3;
        } else {
            *state = 4;
        }
        wk->frame = 0;
        break;
    }
    case 3: {
        u32 keys;

        if (wk->propHandle != NULL && FieldPropHandle_IsAnmIdle(wk->propHandle, 0)) {
            FieldPropHandle_CallAnmCmd(wk->propHandle, 0, 0);
        }
        keys = GCTX_HIDGetHeldKeys();
        if (keys & 0x40) {
            FldAct_SetAcmd(actor, 0x24);
        }
        if (keys & 0x80) {
            FldAct_SetAcmd(actor, 0x25);
        }
        if (keys & 0x20) {
            FldAct_SetAcmd(actor, 0x26);
        }
        if (keys & 0x10) {
            FldAct_SetAcmd(actor, 0x27);
        }
        if (wk->frame++ % 10 == 0) {
            func_ov036_021a3e74(actor, effects);
        }
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            *state = 4;
        }
        break;
    }
    case 4: {
        VecFx32 move;
        FieldTask *moveTask;
        FieldTask *spin;
        FieldTask *zoom;
        FieldTask *fade;
        FieldTaskManager *taskManager;

        GFL_SndSEPlay(0x79f);
        SetActorMovementFlag(actor, 0x8000);
        move.x = 0;
        move.z = 0;
        move.y = -0x1de000;
        moveTask = FieldActorMoveTask_CreatePlayer(wk->field, 0x22, &move);
        spin = FieldActorSpinTask_CreatePlayer(wk->field, 0x22, 5);
        zoom = FieldCameraMoveTaskZoom_Create(wk->field, 0x1e, 0xb4000);
        fade = FieldFadeTask_Create(wk->field, 3, 0, 0x10, 0);
        taskManager = Field_GetTaskManager(wk->field);
        FieldTaskManager_AddTask(taskManager, moveTask, 0);
        FieldTaskManager_AddTask(taskManager, spin, 0);
        FieldTaskManager_AddTask(taskManager, zoom, 0);
        FieldTaskManager_AddTask(taskManager, fade, (u32)zoom);
        wk->frame = 0;
        *state = 5;
        break;
    }
    case 5:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            *state = 6;
        }
        break;
    case 6:
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(0x680));
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        if (wk->propHandle != NULL) {
            FieldPropHandle_Free(wk->propHandle);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021ba13c(EffectEventWork *wk);
static void func_ov036_021ba178(EffectEventWork *wk);
static void func_ov036_021ba1e0(EffectEventWork *wk);
static GameEvent *func_ov036_021ba1f4(EffectEventWork *wk);
static void func_ov036_021ba298(EffectEventWork *wk);
static void func_ov036_021ba2f4(EffectEventWork *wk);
static void func_ov036_021ba35c(EffectEventWork *wk);
static GameEvent *func_ov036_021ba370(EffectEventWork *wk);

static GameEventReturnCode EventEscapeRope_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);

    switch (*state) {
    case 0:
        func_ov036_021ba13c(wk);
        func_ov036_021ba178(wk);
        (*state)++;
        break;
    case 1:
        if (wk->timer++ > 0x23) {
            GameEvent_ChainNext(event, func_ov036_021ba1f4(wk));
            (*state)++;
            break;
        }
    case 2:
        if (FieldTaskManager_IsIdle(taskManager)) {
            GFL_SndSEPlay(0x559);
            (*state)++;
        }
        break;
    case 3:
        func_ov036_021ba1e0(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021ba13c(EffectEventWork *wk) {
    Field *field = wk->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 3);
    FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);

    FieldTaskManager_AddTask(taskManager, accel, 0);
    FieldTaskManager_AddTask(taskManager, spin, (u32)accel);
}

static void func_ov036_021ba178(EffectEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.unk48 = TRUE;
    wk->anim.unk4C = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
    wk->anim.dst = wk->anim.src;
    wk->anim.dst.distance -= 0x64000;
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void func_ov036_021ba1e0(EffectEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static GameEvent *func_ov036_021ba1f4(EffectEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->seasonChanged) {
        return CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
    }
    return CallFieldMapEntranceOutTransitionDefault(gsys, field, 1, 0);
}

static GameEventReturnCode EventDig_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);

    switch (*state) {
    case 0:
        func_ov036_021ba298(wk);
        func_ov036_021ba2f4(wk);
        GFL_SndSEPlay(0x6df);
        (*state)++;
        break;
    case 1:
        if (wk->timer++ > 0x23) {
            GameEvent_ChainNext(event, func_ov036_021ba370(wk));
            (*state)++;
            break;
        }
    case 2:
        if (FieldTaskManager_IsIdle(taskManager)) {
            GFL_SndSEPlay(0x559);
            (*state)++;
        }
        break;
    case 3:
        func_ov036_021ba35c(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void func_ov036_021ba298(EffectEventWork *wk) {
    Field *field = wk->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    FieldTask *effect = FieldActorEffectTask_Create(field, FieldPlayer_GetActor(Field_GetPlayer(field)), 0x28, 6);
    FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 4);
    FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);

    FieldTaskManager_AddTask(taskManager, effect, 0);
    FieldTaskManager_AddTask(taskManager, accel, 0);
    FieldTaskManager_AddTask(taskManager, spin, (u32)accel);
}

static void func_ov036_021ba2f4(EffectEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.unk48 = TRUE;
    wk->anim.unk4C = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
    wk->anim.dst = wk->anim.src;
    wk->anim.dst.distance -= 0x64000;
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void func_ov036_021ba35c(EffectEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static GameEvent *func_ov036_021ba370(EffectEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->seasonChanged) {
        return CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
    }
    return CallFieldMapEntranceOutTransitionDefault(gsys, field, 1, 0);
}

static GameEventReturnCode EventTeleportEffect_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera;
    FieldTaskManager *taskManager;

    Field_GetPlayer(field);
    camera = Field_GetCameraSystem(wk->field);
    taskManager = Field_GetTaskManager(field);
    switch (*state) {
    case 0: {
        FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 3);
        FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);
        FieldTask *wait = FieldWaitTask_Create(field, 0x1e);
        FieldTask *fade = FieldFadeTask_Create(field, 3, 0, 0x10, 1);
        FieldCameraAnimationController *controller;

        FieldTaskManager_AddTask(taskManager, accel, 0);
        FieldTaskManager_AddTask(taskManager, spin, (u32)accel);
        FieldTaskManager_AddTask(taskManager, wait, 0);
        FieldTaskManager_AddTask(taskManager, fade, (u32)wait);
        if (wk->teleportFlag) {
            GFL_SndSEPlay(0x6e7);
        } else {
            GFL_SndSEPlay(0x676);
        }
        controller = Field_CreateCameraAnimationController(field);
        wk->anim.frames = 0x3c;
        wk->anim.unk48 = TRUE;
        wk->anim.unk4C = TRUE;
        wk->controller = controller;
        FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
        FieldCameraAnimationController_PrepareCamera(wk->controller);
        FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
        wk->anim.dst = wk->anim.src;
        wk->anim.dst.distance -= 0x64000;
        FieldCameraAnimationController_CommitSrcCoords(wk->controller);
        FieldCameraAnimationController_StartAnimation(wk->controller);
        (*state)++;
        break;
    }
    case 1:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case 2:
        FieldCameraAnimationController_FinishCamera(wk->controller);
        FieldCameraAnimationController_FreeAnimation(wk->controller);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode func_ov036_021ba4bc(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera;

    Field_GetPlayer(field);
    camera = Field_GetCameraSystem(field);
    switch (*state) {
    case 0: {
        VecFx32 move;
        FieldTask *spin;
        FieldTask *fade;
        FieldTask *moveTask;
        FieldTaskManager *taskManager;

        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        move.y = 0x96000;
        move.x = 0;
        move.z = 0;
        spin = FieldActorSpinTask_CreatePlayer(field, 0x18, 3);
        fade = FieldFadeTask_Create(field, 3, 0, 0x10, 1);
        moveTask = FieldActorMoveTask_CreatePlayer(field, 0x18, &move);
        taskManager = Field_GetTaskManager(field);
        FieldTaskManager_AddTask(taskManager, spin, 0);
        FieldTaskManager_AddTask(taskManager, moveTask, 0);
        FieldTaskManager_AddTask(taskManager, fade, (u32)moveTask);
        GFL_SndSEPlay(0x676);
        (*state)++;
        break;
    }
    case 1:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(field)) && !GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

typedef struct {
    s32 x;
    s32 z;
    s32 width;
    s32 height;
    s32 unk10;
    s32 enabled;
} FieldFadeRect;

BOOL func_ov036_021ba594(s32 x, s32 z, const void *entry) {
    const FieldFadeRect *rect = entry;
    s32 left;
    s32 width;
    s32 top;
    s32 height;
    s32 right;
    s32 bottom;

    if (rect->enabled == 0) {
        return FALSE;
    }
    left = rect->x;
    width = rect->width;
    top = rect->z;
    height = rect->height;
    right = left + width - 1;
    bottom = top + height - 1;
    if (left <= x && x <= right && top <= z && z <= bottom) {
        return TRUE;
    }
    return FALSE;
}
