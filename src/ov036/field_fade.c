// field_fade.c: the field's screen fade, and the field events that fade or move the screen as the player goes through
// the world: sinking into quicksand, falling down a hole, a spin, an Escape Rope, Dig and Teleport. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
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
    u32 white;
    u32 skipWait;
    u32 type;
    u32 stage;
} FieldFadeWork;

// The stages of the field's fade task: start the fade, then wait for it unless the caller skips the wait
enum {
    FIELD_FADE_STAGE_START,
    FIELD_FADE_STAGE_WAIT,
};

static void FieldFadeTCB_Callback(TCB *tcb, void *data);

void FieldFadeTCB_Start(GameSystem *gsys, Field *field, u32 type, u32 white, u32 skipWait) {
    FieldFadeWork *wk;
    TCBManager *mgr = Field_GetTCBMgr(field);

    wk = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(FieldFadeWork), TRUE, "field_fade.c", 56);
    GFL_TCBMgrAddTask(mgr, FieldFadeTCB_Callback, wk, 0);
    Field_SetFadeFlag(field, TRUE);
    wk->stage = FIELD_FADE_STAGE_START;
    wk->gsys = gsys;
    wk->field = field;
    wk->white = white;
    wk->skipWait = skipWait;
    wk->type = type;
}

static void FieldFadeTCB_Callback(TCB *tcb, void *data) {
    FieldFadeWork *wk = data;

    switch (wk->stage) {
    case FIELD_FADE_STAGE_START: {
        s32 speed;
        s32 start;
        s32 end;
        u32 mode;

        if (wk->type <= FIELD_FADE_TYPE_OUT_QUICK) {
            speed = -1;
        } else {
            speed = 4;
        }
        if (wk->type == FIELD_FADE_TYPE_IN_QUICK || wk->type == FIELD_FADE_TYPE_IN) {
            u32 enabled;

            start = 16;
            end = 0;
            enabled = GFL_BGSysGetEnabledBGsA();
            FieldG2D_SetLCDConfig();
            GFL_BGSysSetEnabledBGsA(enabled);
            FieldG2D_Prepare3DSurface(wk->field);
        } else {
            start = 0;
            end = 16;
        }
        if (wk->white == FALSE) {
            mode = FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK;
        } else {
            mode = FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE;
        }
        GFL_FadeSet(mode, start, end, speed);
        wk->stage++;
        break;
    }
    case FIELD_FADE_STAGE_WAIT: {
        BOOL done = FALSE;

        if (wk->skipWait == TRUE) {
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
static GameEventReturnCode EventQuicksandArrive_RunSpinZoom(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventQuicksandArrive_RunSpinZoomAlt(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventQuicksandArrive_RunSpinZoomSmoke(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventPlayerSpinDown_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventQuicksandArrive_RunMoveAnime(GameEvent *event, u32 *state, void *data);

GameEvent *EventQuicksandArrive_Create(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_Callback, 0x68);
    FallEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

GameEvent *EventQuicksandArrive_CreateSpinZoom(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged,
                                               u32 prevSeason, u32 nextSeason) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_RunSpinZoom, 0x68);
    SpinEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    wk->hasWarp = seasonChanged;
    wk->warpA = prevSeason;
    wk->warpB = nextSeason;
    return event;
}

GameEvent *EventQuicksandArrive_CreateSpinZoomAlt(GameEvent *parent, GameSystem *gsys, Field *field, BOOL seasonChanged,
                                                  u32 prevSeason, u32 nextSeason) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_RunSpinZoomAlt, 0x68);
    SpinEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    wk->hasWarp = seasonChanged;
    wk->warpA = prevSeason;
    wk->warpB = nextSeason;
    return event;
}

GameEvent *EventQuicksandArrive_CreateSpinZoomSmoke(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_RunSpinZoomSmoke, 0x68);
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

GameEvent *EventQuicksandArrive_CreateMoveAnime(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandArrive_RunMoveAnime, 0x68);
    FallEventWork *wk = GameEvent_GetData(event);

    wk->field = field;
    wk->gsys = gsys;
    return event;
}

// The steps of the quicksand arrival's fall, its move animation and Dig's rising spin: switch the camera's transform
// and start the player's animation, wait for it, restore the camera's transform, end
enum {
    PLAYER_ANIME_START,
    PLAYER_ANIME_WAIT,
    PLAYER_ANIME_RESTORE_CAMERA,
    PLAYER_ANIME_END,
};

static GameEventReturnCode EventQuicksandArrive_Callback(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    FieldCamera *camera = Field_GetCameraSystem(wk->field);

    switch (*state) {
    case PLAYER_ANIME_START:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        {
            VecFx32 offset = { 0, FX32_CONST(250), 0 };
            FieldActor *actor;
            FieldTask *task;

            actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));
            SetActorWPosOffset(actor, &offset);
            FldAct_SetAcmd(actor, ACMD_FACE + DIR_DOWN);
            GameEvent_ChainNext(
                event, CallFieldMapEntranceInTransition(wk->gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 1, 1, 0, 0));
            actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));
            task = FieldActorFallTask_Create(wk->field, actor, 0x28, 0xfa);
            FieldTaskManager_AddTask(Field_GetTaskManager(wk->field), task, 0);
        }
        (*state)++;
        break;
    case PLAYER_ANIME_WAIT:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            (*state)++;
        }
        break;
    case PLAYER_ANIME_RESTORE_CAMERA:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case PLAYER_ANIME_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventQuicksandArrive_StartPlayerSpin(SpinEventWork *wk);
static void EventQuicksandArrive_StartZoomOut(SpinEventWork *wk);
static GameEvent *EventQuicksandArrive_CreateFadeIn(SpinEventWork *wk);
static void EventQuicksandArrive_EndZoomOut(SpinEventWork *wk);

// The steps of the quicksand arrival's spins: finish the camera's delay and wait for it, start the player's spin and
// the camera's zoom out (the season banner's callback starts them when the season changes), chain the fade in, wait
// for the tasks, end the zoom
enum {
    SPIN_ZOOM_FINISH_DELAY,
    SPIN_ZOOM_WAIT_DELAY,
    SPIN_ZOOM_START,
    SPIN_ZOOM_FADE_IN,
    SPIN_ZOOM_WAIT,
    SPIN_ZOOM_END,
};

static GameEventReturnCode EventQuicksandArrive_RunSpinZoom(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case SPIN_ZOOM_FINISH_DELAY:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT_DELAY:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case SPIN_ZOOM_START:
        if (wk->hasWarp == 0) {
            EventQuicksandArrive_StartPlayerSpin(wk);
            EventQuicksandArrive_StartZoomOut(wk);
        }
        (*state)++;
        break;
    case SPIN_ZOOM_FADE_IN:
        GameEvent_ChainNext(event, EventQuicksandArrive_CreateFadeIn(wk));
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case SPIN_ZOOM_END:
        EventQuicksandArrive_EndZoomOut(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventQuicksandArrive_StartZoomOut(SpinEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.releaseBind = TRUE;
    wk->anim.disableBoundary = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
    wk->anim.src = wk->anim.dst;
    wk->anim.src.distance -= FX32_CONST(100);
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void EventQuicksandArrive_EndZoomOut(SpinEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static void EventQuicksandArrive_StartPlayerSpin(SpinEventWork *wk) {
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);
    FieldTask *task = FieldActorSpinTask_CreatePlayerDeaccel(wk->field, 0x3c, 8, 1);

    FieldTaskManager_AddTask(taskManager, task, 0);
}

static void EventQuicksandArrive_OnScoreboardEnd(void *data);

static GameEvent *EventQuicksandArrive_CreateFadeIn(SpinEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->hasWarp != 0) {
        return EventGameTransition_CreateSeasonFadeInEventWithCallback(gsys, field, wk->warpA, wk->warpB,
                                                                       EventQuicksandArrive_OnScoreboardEnd, wk);
    }
    return CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_WHITE, 1, 1, 0, 0);
}

static void EventQuicksandArrive_OnScoreboardEnd(void *data) {
    SpinEventWork *wk = data;

    EventQuicksandArrive_StartPlayerSpin(wk);
    EventQuicksandArrive_StartZoomOut(wk);
}

static void EventQuicksandArrive_StartZoomOutAlt(SpinEventWork *wk);
static GameEvent *EventQuicksandArrive_CreateFadeInAlt(SpinEventWork *wk);
static void EventQuicksandArrive_EndZoomOutAlt(SpinEventWork *wk);
static void EventQuicksandArrive_StartPlayerSpinAlt(SpinEventWork *wk);

static GameEventReturnCode EventQuicksandArrive_RunSpinZoomAlt(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case SPIN_ZOOM_FINISH_DELAY:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT_DELAY:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case SPIN_ZOOM_START:
        if (wk->hasWarp == 0) {
            EventQuicksandArrive_StartPlayerSpinAlt(wk);
            EventQuicksandArrive_StartZoomOutAlt(wk);
        }
        (*state)++;
        break;
    case SPIN_ZOOM_FADE_IN:
        GameEvent_ChainNext(event, EventQuicksandArrive_CreateFadeInAlt(wk));
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case SPIN_ZOOM_END:
        EventQuicksandArrive_EndZoomOutAlt(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventQuicksandArrive_StartZoomOutAlt(SpinEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.releaseBind = TRUE;
    wk->anim.disableBoundary = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
    wk->anim.src = wk->anim.dst;
    wk->anim.src.distance -= FX32_CONST(100);
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void EventQuicksandArrive_EndZoomOutAlt(SpinEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static void EventQuicksandArrive_StartPlayerSpinAlt(SpinEventWork *wk) {
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);
    FieldTask *task = FieldActorSpinTask_CreatePlayerDeaccel(wk->field, 0x3c, 8, 1);

    FieldTaskManager_AddTask(taskManager, task, 0);
}

static void EventQuicksandArrive_OnScoreboardEndAlt(void *data);

static GameEvent *EventQuicksandArrive_CreateFadeInAlt(SpinEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->hasWarp != 0) {
        return EventGameTransition_CreateSeasonFadeInEventWithCallback(gsys, field, wk->warpA, wk->warpB,
                                                                       EventQuicksandArrive_OnScoreboardEndAlt, wk);
    }
    return CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_WHITE, 1, 1, 0, 0);
}

static void EventQuicksandArrive_OnScoreboardEndAlt(void *data) {
    SpinEventWork *wk = data;

    EventQuicksandArrive_StartPlayerSpinAlt(wk);
    EventQuicksandArrive_StartZoomOutAlt(wk);
}

static GameEventReturnCode EventQuicksandArrive_RunSpinZoomSmoke(GameEvent *event, u32 *state, void *data) {
    SpinEventWork *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case SPIN_ZOOM_FINISH_DELAY:
        FieldCamera_FinishDelay(camera);
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT_DELAY:
        if (!FieldCamera_IsDelayActive(camera)) {
            (*state)++;
        }
        break;
    case SPIN_ZOOM_START: {
        FieldCameraAnimationController *controller;

        FieldTaskManager_AddTask(taskManager, FieldActorSpinTask_CreatePlayerDeaccel(field, 0x3c, 8, 1), 0);
        controller = Field_CreateCameraAnimationController(field);
        wk->anim.frames = 0x3c;
        wk->anim.releaseBind = TRUE;
        wk->anim.disableBoundary = TRUE;
        wk->controller = controller;
        FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
        FieldCameraAnimationController_PrepareCamera(wk->controller);
        FieldCamera_GetAnimationCoords(camera, &wk->anim.dst);
        wk->anim.src = wk->anim.dst;
        wk->anim.src.distance -= FX32_CONST(100);
        FieldCameraAnimationController_CommitSrcCoords(wk->controller);
        FieldCameraAnimationController_StartAnimation(wk->controller);
        (*state)++;
        break;
    }
    case SPIN_ZOOM_FADE_IN:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 1, 1, 0, 0));
        (*state)++;
        break;
    case SPIN_ZOOM_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            FldeffDust_Start(FieldPlayer_GetActor(Field_GetPlayer(field)), Field_GetFieldEffects(field));
            (*state)++;
        }
        break;
    case SPIN_ZOOM_END:
        FieldCameraAnimationController_FinishCamera(wk->controller);
        FieldCameraAnimationController_FreeAnimation(wk->controller);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The steps of the player's spin down: start the spin and the drop, wait for them, land with dust and a sound, restore
// the camera's transform, end
enum {
    SPIN_DOWN_START,
    SPIN_DOWN_WAIT,
    SPIN_DOWN_LAND,
    SPIN_DOWN_RESTORE_CAMERA,
    SPIN_DOWN_END,
};

static GameEventReturnCode EventPlayerSpinDown_Callback(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera = Field_GetCameraSystem(field);

    switch (*state) {
    case SPIN_DOWN_START:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        {
            VecFx32 offset;
            FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(field));

            offset.y = FX32_CONST(10);
            offset.x = 0;
            offset.z = 0;
            SetActorWPosOffset(actor, &offset);
        }
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(wk->gsys, field, FIELD_TRANSITION_FADE_BLACK, 1, 1, 0, 0));
        {
            VecFx32 move;
            FieldTask *spin;
            FieldTask *moveTask;
            FieldTaskManager *taskManager;

            move.y = FX32_CONST(150);
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
    case SPIN_DOWN_WAIT:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(field))) {
            (*state)++;
        }
        break;
    case SPIN_DOWN_LAND:
        FldeffDust_Start(FieldPlayer_GetActor(Field_GetPlayer(field)), Field_GetFieldEffects(wk->field));
        GFL_SndSEPlay(SEQ_SE_FLD_10);
        (*state)++;
        break;
    case SPIN_DOWN_RESTORE_CAMERA:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case SPIN_DOWN_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventQuicksandArrive_RunMoveAnime(GameEvent *event, u32 *state, void *data) {
    FallEventWork *wk = data;
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldActor *actor = FieldPlayer_GetActor(Field_GetPlayer(wk->field));

    switch (*state) {
    case PLAYER_ANIME_START:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        FldAct_SetAcmd(actor, 0x44);
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(wk->gsys, wk->field, FIELD_TRANSITION_FADE_BLACK, 1, 1, 0, 0));
        (*state)++;
        break;
    case PLAYER_ANIME_WAIT:
        if (FldAct_CheckAcmdEnd(actor) == 1) {
            FldAct_EndAcmd(actor);
            (*state)++;
        }
        break;
    case PLAYER_ANIME_RESTORE_CAMERA:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case PLAYER_ANIME_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 transformType;
    s32 frame;
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
static GameEventReturnCode EventDig_RunRisingSpin(GameEvent *event, u32 *state, void *data);

GameEvent *EventQuicksandDrawIn_Create(GameEvent *parent, GameSystem *gsys, Field *field, const VecFx32 *pos) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventQuicksandDrawIn_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->propHandle = NULL;
    VEC_Set(&wk->pos, pos->x, pos->y, pos->z);
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

GameEvent *EventTeleportEffect_Create(GameEvent *parent, GameSystem *gsys, Field *field, BOOL teleport) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventTeleportEffect_Callback, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    wk->teleportFlag = teleport;
    return event;
}

GameEvent *EventDig_CreateRisingSpin(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, EventDig_RunRisingSpin, 0x7c);
    EffectEventWork *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->field = field;
    wk->frame = 0;
    return event;
}

// The steps of the quicksand's draw in: switch the camera's transform, find the quicksand's prop and play its
// animation, pull the player to the center if they are not on it, sink, wait for the sinking tasks, end
enum {
    QUICKSAND_DRAW_IN_START,
    QUICKSAND_DRAW_IN_FIND_PROP,
    QUICKSAND_DRAW_IN_CHECK_DISTANCE,
    QUICKSAND_DRAW_IN_PULL,
    QUICKSAND_DRAW_IN_SINK,
    QUICKSAND_DRAW_IN_WAIT,
    QUICKSAND_DRAW_IN_END,
};

static GameEventReturnCode EventQuicksandDrawIn_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldPlayer *player = Field_GetPlayer(wk->field);
    FieldActor *actor = FieldPlayer_GetActor(player);
    FieldEffects *effects = Field_GetFieldEffects(wk->field);
    FieldCamera *camera = Field_GetCameraSystem(wk->field);

    switch (*state) {
    case QUICKSAND_DRAW_IN_START:
        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        *state = QUICKSAND_DRAW_IN_FIND_PROP;
        break;
    case QUICKSAND_DRAW_IN_FIND_PROP: {
        FieldPropSystem *propSystem;
        FieldPropAreaBounds bounds;
        FieldChunkPropHolder **props;
        u32 count;

        GFL_SEPlayKeepVol(SEQ_SE_FLD_15, GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_15));
        propSystem = FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(wk->field));
        bounds.minX = wk->pos.x - FX32_CONST(32);
        bounds.maxX = wk->pos.x + FX32_CONST(32);
        bounds.minZ = wk->pos.z - FX32_CONST(32);
        bounds.maxZ = wk->pos.z + FX32_CONST(32);
        props = FieldPropSystem_FindPropsInArea(propSystem, &bounds, 2, &count);
        if (*props != NULL) {
            wk->propHandle = FieldPropSystem_CreateHandleFromExisting(propSystem, *props);
            FieldPropHandle_CallAnmCmd(wk->propHandle, 0, 0);
        } else {
            wk->propHandle = NULL;
        }
        GFL_HeapFree(props);
        *state = QUICKSAND_DRAW_IN_CHECK_DISTANCE;
        break;
    }
    case QUICKSAND_DRAW_IN_CHECK_DISTANCE: {
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
            *state = QUICKSAND_DRAW_IN_PULL;
        } else {
            *state = QUICKSAND_DRAW_IN_SINK;
        }
        wk->frame = 0;
        break;
    }
    case QUICKSAND_DRAW_IN_PULL: {
        u32 keys;

        if (wk->propHandle != NULL && FieldPropHandle_IsAnmIdle(wk->propHandle, 0)) {
            FieldPropHandle_CallAnmCmd(wk->propHandle, 0, 0);
        }
        keys = GCTX_HIDGetHeldKeys();
        if (keys & PAD_KEY_UP) {
            FldAct_SetAcmd(actor, ACMD_STAY_WALK_4F + DIR_UP);
        }
        if (keys & PAD_KEY_DOWN) {
            FldAct_SetAcmd(actor, ACMD_STAY_WALK_4F + DIR_DOWN);
        }
        if (keys & PAD_KEY_LEFT) {
            FldAct_SetAcmd(actor, ACMD_STAY_WALK_4F + DIR_LEFT);
        }
        if (keys & PAD_KEY_RIGHT) {
            FldAct_SetAcmd(actor, ACMD_STAY_WALK_4F + DIR_RIGHT);
        }
        if (wk->frame++ % 10 == 0) {
            FldeffDust_Start(actor, effects);
        }
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            *state = QUICKSAND_DRAW_IN_SINK;
        }
        break;
    }
    case QUICKSAND_DRAW_IN_SINK: {
        VecFx32 move;
        FieldTask *moveTask;
        FieldTask *spin;
        FieldTask *zoom;
        FieldTask *fade;
        FieldTaskManager *taskManager;

        GFL_SndSEPlay(SEQ_SE_FLD_147);
        SetActorMovementFlag(actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
        move.x = 0;
        move.y = FX32_CONST(-30);
        move.z = 0;
        moveTask = FieldActorMoveTask_CreatePlayer(wk->field, 0x22, &move);
        spin = FieldActorSpinTask_CreatePlayer(wk->field, 0x22, 5);
        zoom = FieldCameraMoveTaskZoom_Create(wk->field, 0x1e, FX32_CONST(180));
        fade = FieldFadeTask_Create(wk->field, FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        taskManager = Field_GetTaskManager(wk->field);
        FieldTaskManager_AddTask(taskManager, moveTask, 0);
        FieldTaskManager_AddTask(taskManager, spin, 0);
        FieldTaskManager_AddTask(taskManager, zoom, 0);
        FieldTaskManager_AddTask(taskManager, fade, zoom);
        wk->frame = 0;
        *state = QUICKSAND_DRAW_IN_WAIT;
        break;
    }
    case QUICKSAND_DRAW_IN_WAIT:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(wk->field))) {
            *state = QUICKSAND_DRAW_IN_END;
        }
        break;
    case QUICKSAND_DRAW_IN_END:
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_15));
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        if (wk->propHandle != NULL) {
            FieldPropHandle_Free(wk->propHandle);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventEscapeRope_StartSpinTasks(EffectEventWork *wk);
static void EventEscapeRope_StartCameraAnimation(EffectEventWork *wk);
static void EventEscapeRope_FreeCameraAnimation(EffectEventWork *wk);
static GameEvent *EventEscapeRope_CreateExitTransition(EffectEventWork *wk);
static void EventDig_StartEffectTasks(EffectEventWork *wk);
static void EventDig_StartCameraAnimation(EffectEventWork *wk);
static void EventDig_FreeCameraAnimation(EffectEventWork *wk);
static GameEvent *EventDig_CreateExitTransition(EffectEventWork *wk);

// The steps of an Escape Rope and of Dig: start the spin and the camera, chain the exit after 36 frames (until then
// fall into the wait), wait for the tasks, end
enum {
    ESCAPE_START,
    ESCAPE_CHAIN_EXIT,
    ESCAPE_WAIT,
    ESCAPE_END,
};

static GameEventReturnCode EventEscapeRope_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);

    switch (*state) {
    case ESCAPE_START:
        EventEscapeRope_StartSpinTasks(wk);
        EventEscapeRope_StartCameraAnimation(wk);
        (*state)++;
        break;
    case ESCAPE_CHAIN_EXIT:
        if (wk->timer++ > 0x23) {
            GameEvent_ChainNext(event, EventEscapeRope_CreateExitTransition(wk));
            (*state)++;
            break;
        }
    case ESCAPE_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            GFL_SndSEPlay(SEQ_SE_KAIDAN);
            (*state)++;
        }
        break;
    case ESCAPE_END:
        EventEscapeRope_FreeCameraAnimation(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventEscapeRope_StartSpinTasks(EffectEventWork *wk) {
    Field *field = wk->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 3);
    FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);

    FieldTaskManager_AddTask(taskManager, accel, 0);
    FieldTaskManager_AddTask(taskManager, spin, accel);
}

static void EventEscapeRope_StartCameraAnimation(EffectEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.releaseBind = TRUE;
    wk->anim.disableBoundary = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
    wk->anim.dst = wk->anim.src;
    wk->anim.dst.distance -= FX32_CONST(100);
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void EventEscapeRope_FreeCameraAnimation(EffectEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static GameEvent *EventEscapeRope_CreateExitTransition(EffectEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->seasonChanged) {
        return CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
    }
    return CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_WHITE, 0);
}

static GameEventReturnCode EventDig_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    FieldTaskManager *taskManager = Field_GetTaskManager(wk->field);

    switch (*state) {
    case ESCAPE_START:
        EventDig_StartEffectTasks(wk);
        EventDig_StartCameraAnimation(wk);
        GFL_SndSEPlay(SEQ_SE_FLD_80);
        (*state)++;
        break;
    case ESCAPE_CHAIN_EXIT:
        if (wk->timer++ > 0x23) {
            GameEvent_ChainNext(event, EventDig_CreateExitTransition(wk));
            (*state)++;
            break;
        }
    case ESCAPE_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            GFL_SndSEPlay(SEQ_SE_KAIDAN);
            (*state)++;
        }
        break;
    case ESCAPE_END:
        EventDig_FreeCameraAnimation(wk);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void EventDig_StartEffectTasks(EffectEventWork *wk) {
    Field *field = wk->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);
    FieldTask *effect = FieldActorEffectTask_Create(field, FieldPlayer_GetActor(Field_GetPlayer(field)), 0x28, 6);
    FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 4);
    FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);

    FieldTaskManager_AddTask(taskManager, effect, 0);
    FieldTaskManager_AddTask(taskManager, accel, 0);
    FieldTaskManager_AddTask(taskManager, spin, accel);
}

static void EventDig_StartCameraAnimation(EffectEventWork *wk) {
    FieldCamera *camera = Field_GetCameraSystem(wk->field);
    FieldCameraAnimationController *controller = Field_CreateCameraAnimationController(wk->field);

    wk->anim.frames = 0x3c;
    wk->anim.releaseBind = TRUE;
    wk->anim.disableBoundary = TRUE;
    wk->controller = controller;
    FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
    FieldCameraAnimationController_PrepareCamera(wk->controller);
    FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
    wk->anim.dst = wk->anim.src;
    wk->anim.dst.distance -= FX32_CONST(100);
    FieldCameraAnimationController_CommitSrcCoords(wk->controller);
    FieldCameraAnimationController_StartAnimation(wk->controller);
}

static void EventDig_FreeCameraAnimation(EffectEventWork *wk) {
    FieldCameraAnimationController_FinishCamera(wk->controller);
    FieldCameraAnimationController_FreeAnimation(wk->controller);
}

static GameEvent *EventDig_CreateExitTransition(EffectEventWork *wk) {
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    if (wk->seasonChanged) {
        return CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0);
    }
    return CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_WHITE, 0);
}

// The steps of Teleport's effect: start the spin, the fade and the camera, wait for them, end
enum {
    TELEPORT_START,
    TELEPORT_WAIT,
    TELEPORT_END,
};

static GameEventReturnCode EventTeleportEffect_Callback(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera;
    FieldTaskManager *taskManager;

    Field_GetPlayer(field);
    camera = Field_GetCameraSystem(wk->field);
    taskManager = Field_GetTaskManager(field);
    switch (*state) {
    case TELEPORT_START: {
        FieldTask *accel = FieldActorSpinTask_CreatePlayerAccel(field, 0x14, 3);
        FieldTask *spin = FieldActorSpinTask_CreatePlayer(field, 0x14, 5);
        FieldTask *wait = FieldWaitTask_Create(field, 0x1e);
        FieldTask *fade = FieldFadeTask_Create(field, FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        FieldCameraAnimationController *controller;

        FieldTaskManager_AddTask(taskManager, accel, 0);
        FieldTaskManager_AddTask(taskManager, spin, accel);
        FieldTaskManager_AddTask(taskManager, wait, 0);
        FieldTaskManager_AddTask(taskManager, fade, wait);
        if (wk->teleportFlag) {
            GFL_SndSEPlay(SEQ_SE_FLD_86);
        } else {
            GFL_SndSEPlay(SEQ_SE_FLD_05);
        }
        controller = Field_CreateCameraAnimationController(field);
        wk->anim.frames = 0x3c;
        wk->anim.releaseBind = TRUE;
        wk->anim.disableBoundary = TRUE;
        wk->controller = controller;
        FieldCameraAnimationController_SetAnimation(controller, &wk->anim);
        FieldCameraAnimationController_PrepareCamera(wk->controller);
        FieldCamera_GetAnimationCoords(camera, &wk->anim.src);
        wk->anim.dst = wk->anim.src;
        wk->anim.dst.distance -= FX32_CONST(100);
        FieldCameraAnimationController_CommitSrcCoords(wk->controller);
        FieldCameraAnimationController_StartAnimation(wk->controller);
        (*state)++;
        break;
    }
    case TELEPORT_WAIT:
        if (FieldTaskManager_IsIdle(taskManager)) {
            (*state)++;
        }
        break;
    case TELEPORT_END:
        FieldCameraAnimationController_FinishCamera(wk->controller);
        FieldCameraAnimationController_FreeAnimation(wk->controller);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventDig_RunRisingSpin(GameEvent *event, u32 *state, void *data) {
    EffectEventWork *wk = data;
    Field *field = wk->field;
    FieldCamera *camera;

    Field_GetPlayer(field);
    camera = Field_GetCameraSystem(field);
    switch (*state) {
    case PLAYER_ANIME_START: {
        VecFx32 move;
        FieldTask *spin;
        FieldTask *fade;
        FieldTask *moveTask;
        FieldTaskManager *taskManager;

        wk->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        move.y = FX32_CONST(150);
        move.x = 0;
        move.z = 0;
        spin = FieldActorSpinTask_CreatePlayer(field, 0x18, 3);
        fade = FieldFadeTask_Create(field, FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 1);
        moveTask = FieldActorMoveTask_CreatePlayer(field, 0x18, &move);
        taskManager = Field_GetTaskManager(field);
        FieldTaskManager_AddTask(taskManager, spin, 0);
        FieldTaskManager_AddTask(taskManager, moveTask, 0);
        FieldTaskManager_AddTask(taskManager, fade, moveTask);
        GFL_SndSEPlay(SEQ_SE_FLD_05);
        (*state)++;
        break;
    }
    case PLAYER_ANIME_WAIT:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(field)) && !GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case PLAYER_ANIME_RESTORE_CAMERA:
        FieldCamera_ChangeTransformType(camera, wk->transformType);
        (*state)++;
        break;
    case PLAYER_ANIME_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL HeightEx_IsPointInArea(s32 x, s32 z, const HeightExEntry *rect) {
    s32 left;
    s32 width;
    s32 top;
    s32 height;
    s32 right;
    s32 bottom;

    if (rect->inUse == FALSE) {
        return FALSE;
    }
    left = rect->x;
    width = rect->width;
    top = rect->z;
    height = rect->depth;
    right = left + width - 1;
    bottom = top + height - 1;
    if (left <= x && x <= right && top <= z && z <= bottom) {
        return TRUE;
    }
    return FALSE;
}
