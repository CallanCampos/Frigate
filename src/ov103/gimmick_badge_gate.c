#include "types.h"
#include "field/badge_gate.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "nitro/fx.h"
#include "field/field_task.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// An actor of one of the gimmick's scenes
typedef struct {
    u32 scene;
    u32 actor;
    u32 unk08;
    FieldExpObjSystem *expObj;
} BadgeGateActor;

typedef struct {
    u16 unk00;
    FieldExpObjSystem *expObj;
    Field *field;
    EventWork *eventWork;
    // The actors of scenes 0, 1 and 2
    BadgeGateActor actors0[8];
    BadgeGateActor actors1[9];
    BadgeGateActor actors2[9];
    u32 last;
} BadgeGateWork;

typedef struct {
    BadgeGateWork *gimmickWork;
    u16 badge;
    u32 state;
    u8 unk0c[0x10];
} BadgeGateCheckEventData;

typedef struct {
    BadgeGateWork *gimmickWork;
    FieldCamera *camera;
    // The actor of the last gate's scene
    BadgeGateActor lastActor;
    u32 state;
    // Whether each camera shake is over
    u16 shakeDone[2];
    VecFx32 eyeOffset;
    VecFx32 targetOffset;
    Field *field;
} BadgeGateLastEventData;

void func_ov103_021eecfc(BadgeGateWork *work);
BOOL func_ov103_021eefbc(u32 badge, EventWork *work);
GameEventReturnCode BadgeGate_CheckEvent(GameEvent *event, u32 *state, void *data);
void func_ov103_021ef188(BadgeGateCheckEventData *data);
BadgeGateActor *func_ov103_021ef1ac(BadgeGateWork *work, u32 index, u32 scene);
void func_ov103_021ef1dc(BadgeGateActor *actor, u16 anm, BOOL paused);
fx32 func_ov103_021ef200(BadgeGateWork *work);
GameEventReturnCode BadgeGate_LastGateEvent(GameEvent *event, u32 *state, void *data);
void func_ov103_021ef5dc(BadgeGateLastEventData *data);
void func_ov103_021ef518(BadgeGateLastEventData *data);
void func_ov103_021ef630(BadgeGateActor *actor, FieldExpObjSystem *system);
void func_ov103_021ef788(FieldExpObjSystem *system);

// A shake of the camera during the last gate's event, between two of its frames
typedef struct {
    u32 start;
    u32 end;
    u32 period;
    fx32 amplitudeX;
    fx32 amplitudeY;
} BadgeGateCameraShake;

// The archive of the gimmick's models and animations
#define ARC_BADGE_GATE 284

static const G3DSceneAnimationSetup sScene2Animations7[] = { { 15, 0 } };
static const G3DSceneAnimationSetup sScene2Animations0[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sScene2Animations6[] = { { 13, 0 } };
static const G3DSceneAnimationSetup sScene2Animations2[] = { { 5, 0 } };
static const G3DSceneAnimationSetup sScene2Animations8[] = { { 17, 0 } };
static const G3DSceneAnimationSetup sScene0Animations5[] = { { 16, 0 }, { 17, 0 } };
static const G3DSceneAnimationSetup sScene2Animations5[] = { { 11, 0 } };
static const G3DSceneAnimationSetup sScene0Animations0[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sScene0Animations7[] = { { 22, 0 }, { 23, 0 } };
static const G3DSceneAnimationSetup sScene0Animations2[] = { { 7, 0 }, { 8, 0 } };
static const G3DSceneAnimationSetup sScene0Animations4[] = { { 13, 0 }, { 14, 0 } };
static const G3DSceneAnimationSetup sScene0Animations6[] = { { 19, 0 }, { 20, 0 } };
static const G3DSceneAnimationSetup sScene2Animations4[] = { { 9, 0 } };
static const G3DSceneAnimationSetup sScene2Animations1[] = { { 3, 0 } };

static const G3DSceneResourceSetup sScene1Resources[] = {
    { ARC_BADGE_GATE, 1, 0 },  { ARC_BADGE_GATE, 4, 0 },  { ARC_BADGE_GATE, 7, 0 },
    { ARC_BADGE_GATE, 10, 0 }, { ARC_BADGE_GATE, 13, 0 }, { ARC_BADGE_GATE, 16, 0 },
    { ARC_BADGE_GATE, 19, 0 }, { ARC_BADGE_GATE, 22, 0 }, { ARC_BADGE_GATE, 25, 0 },
};

static const G3DSceneAnimationSetup sScene0Animations3[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sScene2Animations3[] = { { 7, 0 } };
static const G3DSceneAnimationSetup sScene0Animations1[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sScene3Animations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const G3DSceneActorSetup sScene3Actors[] = {
    { 0, 0, 0, 0, sScene3Animations, NELEMS(sScene3Animations) },
};

static const G3DSceneResourceSetup sScene0Resources[] = {
    { ARC_BADGE_GATE, 0, 0 },  { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 3, 0 },
    { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 6, 0 },  { ARC_BADGE_GATE, 27, 0 },
    { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 9, 0 },  { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 },
    { ARC_BADGE_GATE, 12, 0 }, { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 15, 0 },
    { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 18, 0 }, { ARC_BADGE_GATE, 27, 0 },
    { ARC_BADGE_GATE, 29, 0 }, { ARC_BADGE_GATE, 21, 0 }, { ARC_BADGE_GATE, 27, 0 }, { ARC_BADGE_GATE, 29, 0 },
};

static const G3DSceneActorSetup sScene0Actors[] = {
    { 0, 0, 0, 0, sScene0Animations0, NELEMS(sScene0Animations0) },
    { 3, 0, 0, 0, sScene0Animations1, NELEMS(sScene0Animations1) },
    { 6, 0, 0, 0, sScene0Animations2, NELEMS(sScene0Animations2) },
    { 9, 0, 0, 0, sScene0Animations3, NELEMS(sScene0Animations3) },
    { 12, 0, 0, 0, sScene0Animations4, NELEMS(sScene0Animations4) },
    { 15, 0, 0, 0, sScene0Animations5, NELEMS(sScene0Animations5) },
    { 18, 0, 0, 0, sScene0Animations6, NELEMS(sScene0Animations6) },
    { 21, 0, 0, 0, sScene0Animations7, NELEMS(sScene0Animations7) },
};

static const G3DSceneActorSetup sScene1Actors[] = {
    { 0, 0, 0, 0, NULL, 0 }, { 1, 0, 0, 0, NULL, 0 }, { 2, 0, 0, 0, NULL, 0 },
    { 3, 0, 0, 0, NULL, 0 }, { 4, 0, 0, 0, NULL, 0 }, { 5, 0, 0, 0, NULL, 0 },
    { 6, 0, 0, 0, NULL, 0 }, { 7, 0, 0, 0, NULL, 0 }, { 8, 0, 8, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sScene2Resources[] = {
    { ARC_BADGE_GATE, 2, 0 },  { ARC_BADGE_GATE, 31, 0 }, { ARC_BADGE_GATE, 5, 0 },  { ARC_BADGE_GATE, 31, 0 },
    { ARC_BADGE_GATE, 8, 0 },  { ARC_BADGE_GATE, 31, 0 }, { ARC_BADGE_GATE, 11, 0 }, { ARC_BADGE_GATE, 31, 0 },
    { ARC_BADGE_GATE, 14, 0 }, { ARC_BADGE_GATE, 31, 0 }, { ARC_BADGE_GATE, 17, 0 }, { ARC_BADGE_GATE, 31, 0 },
    { ARC_BADGE_GATE, 20, 0 }, { ARC_BADGE_GATE, 31, 0 }, { ARC_BADGE_GATE, 23, 0 }, { ARC_BADGE_GATE, 31, 0 },
    { ARC_BADGE_GATE, 26, 0 }, { ARC_BADGE_GATE, 32, 0 },
};

static const G3DSceneResourceSetup sScene3Resources[] = {
    { ARC_BADGE_GATE, 24, 0 },
    { ARC_BADGE_GATE, 28, 0 },
    { ARC_BADGE_GATE, 30, 0 },
    { ARC_BADGE_GATE, 32, 0 },
};

static const G3DSceneActorSetup sScene2Actors[] = {
    { 0, 0, 0, 0, sScene2Animations0, NELEMS(sScene2Animations0) },
    { 2, 0, 0, 0, sScene2Animations1, NELEMS(sScene2Animations1) },
    { 4, 0, 0, 0, sScene2Animations2, NELEMS(sScene2Animations2) },
    { 6, 0, 0, 0, sScene2Animations3, NELEMS(sScene2Animations3) },
    { 8, 0, 0, 0, sScene2Animations4, NELEMS(sScene2Animations4) },
    { 10, 0, 0, 0, sScene2Animations5, NELEMS(sScene2Animations5) },
    { 12, 0, 0, 0, sScene2Animations6, NELEMS(sScene2Animations6) },
    { 14, 0, 0, 0, sScene2Animations7, NELEMS(sScene2Animations7) },
    { 16, 0, 16, 0, sScene2Animations8, NELEMS(sScene2Animations8) },
};

static const G3DSceneSetup sScene0 = { sScene0Resources, NELEMS(sScene0Resources), sScene0Actors, NELEMS(sScene0Actors) };
static const G3DSceneSetup sScene1 = { sScene1Resources, NELEMS(sScene1Resources), sScene1Actors, NELEMS(sScene1Actors) };

static const BadgeGateCameraShake sCameraShakes[2] = {
    { 138, 315, 4, 1, 1 },
    { 385, 430, 4, 2, 2 },
};

static const G3DSceneSetup sScene2 = { sScene2Resources, NELEMS(sScene2Resources), sScene2Actors, NELEMS(sScene2Actors) };
static const G3DSceneSetup sScene3 = { sScene3Resources, NELEMS(sScene3Resources), sScene3Actors, NELEMS(sScene3Actors) };

// The event work of each gate, which is 2 once its badge has been checked
static const u16 sGateWorks[9] = { 0x40e4, 0x40e5, 0x40e6, 0x40e7, 0x40e8, 0x40e9, 0x40ea, 0x40eb, 0x40ec };

// The size of a grid square of the field
#define GRID_SIZE FX32_CONST(16)

// The position of each gate's actors, in grid squares
static const VecFx32 sGatePositions[9] = {
    { 77 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE }, { 72 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE },
    { 67 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE }, { 62 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE },
    { 57 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE }, { 52 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE },
    { 47 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE }, { 42 * GRID_SIZE, 5 * GRID_SIZE, 53 * GRID_SIZE },
    { 36 * GRID_SIZE, 5 * GRID_SIZE, 47 * GRID_SIZE },
};

void func_ov103_021eec80(Field *field) {
    HeapID heapId;
    GameData *gameData;
    BadgeGateWork *work;

    heapId = Field_GetHeapID(field);
    gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    GameData_GetGimmickState(gameData);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(BadgeGateWork));
    work->unk00 = 0x15;
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    work->eventWork = GameData_GetEventWork(gameData);
    work->last = 0;
    LoadFieldExpandObjData(work->expObj, &sScene0, 0);
    LoadFieldExpandObjData(work->expObj, &sScene1, 1);
    LoadFieldExpandObjData(work->expObj, &sScene2, 2);
    func_ov103_021eecfc(work);
}

void func_ov103_021eecfc(BadgeGateWork *work) {
    s32 i;
    s32 j;
    BadgeGateActor *actor;
    FieldExpObjAnm *anm0;
    FieldExpObjAnm *anm1;

    for (i = 0; i < 8; i++) {
        actor = &work->actors0[i];
        actor->scene = 0;
        actor->actor = i;
        actor->expObj = work->expObj;
    }
    for (i = 0; i < 9; i++) {
        actor = &work->actors1[i];
        actor->scene = 1;
        actor->actor = i;
        actor->expObj = work->expObj;
    }
    for (i = 0; i < 9; i++) {
        actor = &work->actors2[i];
        actor->scene = 2;
        actor->actor = i;
        actor->expObj = work->expObj;
    }
    for (j = 0; j < 8; j++) {
        actor = &work->actors0[j];
        FieldExpObj_GetActorMatrixPtr(actor->expObj, 0, actor->actor)->translation = sGatePositions[j];
        FieldExpObj_SetActorHidden(actor->expObj, 0, actor->actor, TRUE);
        func_ov036_021b8248(actor->expObj, 0, actor->actor, 1);
        anm0 = FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 0);
        FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 0, TRUE);
        FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 0, 0);
        FieldExpObjAnm_SetLooped(anm0, FALSE);
        func_ov103_021ef1dc(actor, 0, TRUE);
        anm1 = FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 1);
        func_ov103_021ef1dc(actor, 1, TRUE);
        FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 1, TRUE);
        FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 1, 0);
        FieldExpObjAnm_SetLooped(anm1, FALSE);
    }
    for (j = 0; j < 9; j++) {
        actor = &work->actors1[j];
        FieldExpObj_GetActorMatrixPtr(actor->expObj, 1, actor->actor)->translation = sGatePositions[j];
        if (func_ov103_021eefbc(j, work->eventWork)) {
            FieldExpObj_SetActorHidden(actor->expObj, actor->scene, actor->actor, TRUE);
        } else {
            FieldExpObj_SetActorHidden(actor->expObj, actor->scene, actor->actor, FALSE);
        }
        func_ov036_021b8248(actor->expObj, 1, actor->actor, 1);
    }
    for (i = 0; i < 9; i++) {
        actor = &work->actors2[i];
        FieldExpObj_GetActorMatrixPtr(actor->expObj, actor->scene, actor->actor)->translation = sGatePositions[i];
        FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 0, TRUE);
        if (func_ov103_021eefbc(i, work->eventWork)) {
            func_ov103_021ef1dc(actor, 0, FALSE);
            FieldExpObj_SetActorHidden(actor->expObj, actor->scene, actor->actor, FALSE);
        } else {
            func_ov103_021ef1dc(actor, 0, TRUE);
            FieldExpObj_SetActorHidden(actor->expObj, actor->scene, actor->actor, TRUE);
        }
        func_ov036_021b8248(actor->expObj, actor->scene, actor->actor, 1);
        FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 0, 0);
        FieldExpObjAnm_SetLooped(FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 0), TRUE);
    }
}


BOOL func_ov103_021eefbc(u32 badge, EventWork *work) {
    u16 *value;

    value = EventWork_GetWkPtr(work, sGateWorks[badge]);
    return *value == 2;
}

void func_ov103_021eefe0(Field *field) {
    BadgeGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    FieldExpObj_FreeScene(work->expObj, 1);
    FieldExpObj_FreeScene(work->expObj, 2);
    FieldExpObj_FreeScene(work->expObj, 0);
    Field_DeleteGimmickWorkBlock(field, 0);
}

void func_ov103_021ef010(Field *field) {
    BadgeGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    FieldExpObj_StepAllAnimations(work->expObj);
}

GameEvent *BadgeGate_CreateCheckEvent(GameSystem *gsys, u8 badge) {
    Field *field;
    GameEvent *event;
    BadgeGateCheckEventData *data;

    field = GSYS_GetField(gsys);
    event = GameEvent_Create(gsys, NULL, BadgeGate_CheckEvent, sizeof(BadgeGateCheckEventData));
    data = GameEvent_GetData(event);
    GSYS_GetGameData(gsys);
    sys_memset(data, 0, sizeof(BadgeGateCheckEventData));
    data->badge = badge;
    data->state = 0;
    data->gimmickWork = Field_GetGimmickWorkBlock(field, 0);
    return event;
}

GameEventReturnCode BadgeGate_CheckEvent(GameEvent *event, u32 *state, void *arg) {
    BadgeGateCheckEventData *data;
    BadgeGateActor *entry0;
    BadgeGateActor *entry1;
    BadgeGateActor *entry2;
    FieldExpObjAnm *anm;
    fx32 frame;

    data = arg;
    entry0 = func_ov103_021ef1ac(data->gimmickWork, data->badge, 0);
    entry1 = func_ov103_021ef1ac(data->gimmickWork, data->badge, 1);
    entry2 = func_ov103_021ef1ac(data->gimmickWork, data->badge, 2);
    func_ov103_021ef188(data);
    data->state++;

    switch (*state) {
    case 0:
        FieldExpObj_SetActorHidden(entry1->expObj, entry1->scene, entry1->actor, TRUE);
        FieldExpObj_SetActorHidden(entry0->expObj, entry0->scene, entry0->actor, FALSE);
        func_ov103_021ef1dc(entry0, 0, FALSE);
        func_ov103_021ef1dc(entry0, 1, FALSE);
        (*state)++;
        break;
    case 1:
        anm = FieldExpObj_GetAnmInfo(entry0->expObj, entry0->scene, entry0->actor, 0);
        if (FieldExpObjAnm_IsPlaybackFinished(anm) == TRUE) {
            func_ov103_021ef1dc(entry0, 0, TRUE);
            (*state)++;
            return GAMEEVENT_CONTINUE_DIRECT;
        }
        break;
    case 2:
        FieldExpObj_SetActorHidden(entry0->expObj, entry0->scene, entry0->actor, TRUE);
        FieldExpObj_SetActorHidden(entry2->expObj, entry2->scene, entry2->actor, FALSE);
        frame = func_ov103_021ef200(data->gimmickWork);
        FieldExpObj_SetAnmFrame(entry2->expObj, entry2->scene, entry2->actor, 0, frame);
        func_ov103_021ef1dc(entry2, 0, FALSE);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void func_ov103_021ef188(BadgeGateCheckEventData *data) {
    if (data->state == 0) {
        GFL_SndSEPlay(0x8a0);
    }
    if (data->state == 0x32) {
        GFL_SndSEPlay(0x89b);
    }
}

BadgeGateActor *func_ov103_021ef1ac(BadgeGateWork *work, u32 index, u32 scene) {
    switch (scene) {
    case 0:
        return &work->actors0[index];
    case 1:
        return &work->actors1[index];
    case 2:
        return &work->actors2[index];
    default:
        return NULL;
    }
}

void func_ov103_021ef1dc(BadgeGateActor *actor, u16 anm, BOOL paused) {
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, anm), paused);
}

fx32 func_ov103_021ef200(BadgeGateWork *work) {
    s32 i;
    BadgeGateActor *entry;
    FieldExpObjAnm *anm;

    for (i = 0; i < 8; i++) {
        entry = func_ov103_021ef1ac(work, i, 2);
        anm = FieldExpObj_GetAnmInfo(entry->expObj, entry->scene, entry->actor, 0);
        if (!func_ov036_021b84ec(anm)) {
            return func_ov036_021b8520(entry->expObj, entry->scene, entry->actor, 0);
        }
    }
    return 0;
}

GameEvent *BadgeGate_CreateLastGateEvent(GameSystem *gsys) {
    Field *field;
    GameEvent *event;
    BadgeGateLastEventData *data;

    field = GSYS_GetField(gsys);
    event = GameEvent_Create(gsys, NULL, BadgeGate_LastGateEvent, sizeof(BadgeGateLastEventData));
    data = GameEvent_GetData(event);
    GSYS_GetGameData(gsys);
    sys_memset(data, 0, sizeof(BadgeGateLastEventData));
    data->gimmickWork = Field_GetGimmickWorkBlock(field, 0);
    data->camera = Field_GetCameraSystem(field);
    data->field = field;
    data->state = 0;
    FieldCamera_CoordsGetEyeOffset(data->camera, &data->eyeOffset);
    FieldCamera_CoordsGetTargetOffset(data->camera, &data->targetOffset);
    return event;
}

GameEventReturnCode BadgeGate_LastGateEvent(GameEvent *event, u32 *state, void *arg) {
    BadgeGateLastEventData *data = arg;
    BadgeGateActor *gate = func_ov103_021ef1ac(data->gimmickWork, 8, 1);
    BadgeGateActor *open = func_ov103_021ef1ac(data->gimmickWork, 8, 2);
    FieldTaskManager *tasks;
    FieldEvCameraAnimationSetup setup;
    u32 i;

    func_ov103_021ef518(data);
    switch (*state) {
    case 4:
    case 5:
    case 6:
        func_ov103_021ef5dc(data);
        data->state++;
        break;
    }
    switch (*state) {
    case 0:
        tasks = Field_GetTaskManager(data->field);
        FieldTaskManager_AddTask(tasks, FieldFadeTask_Create(data->field, 3, 0, 16, 2), 0);
        (*state)++;
        break;
    case 1:
        if (GFL_FadeIsRunning()) {
            break;
        }
        setup.targetCoords.pitch = 0x1dd8;
        setup.targetCoords.yaw = 0;
        setup.targetCoords.distance = 0x18d000;
        setup.targetCoords.targetPos.x = 0x1f8000;
        setup.targetCoords.targetPos.y = 0x6005f;
        setup.targetCoords.targetPos.z = 0x26e000;
        setup.flags.animatePitch = TRUE;
        setup.flags.animateYaw = TRUE;
        setup.flags.animateTargetDistance = TRUE;
        setup.flags.animateTargetPos = TRUE;
        setup.flags.animateExtraTranslation = FALSE;
        setup.flags.animateFOV = FALSE;
        FieldCameraAnm_SetAnimation(data->camera, &setup, 1);
        func_ov103_021ef630(&data->lastActor, data->gimmickWork->expObj);
        (*state)++;
        break;
    case 2:
        tasks = Field_GetTaskManager(data->field);
        FieldTaskManager_AddTask(tasks, FieldFadeTask_Create(data->field, 3, 16, 0, 2), 0);
        (*state)++;
        break;
    case 3:
        if (GFL_FadeIsRunning()) {
            break;
        }
        FieldExpObj_SetActorHidden(gate->expObj, gate->scene, gate->actor, TRUE);
        FieldExpObj_SetActorHidden(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, FALSE);
        func_ov103_021ef1dc(&data->lastActor, 1, FALSE);
        (*state)++;
        break;
    case 4:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, 1)) == TRUE) {
            func_ov103_021ef1dc(&data->lastActor, 1, TRUE);
            FieldExpObj_SetAnm(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, 1, FALSE);
            FieldExpObj_SetAnm(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, 2, TRUE);
            func_ov103_021ef1dc(&data->lastActor, 2, FALSE);
            func_ov103_021ef1dc(&data->lastActor, 0, FALSE);
            FieldExpObj_SetAnmFrame(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, 2, func_ov103_021ef200(data->gimmickWork));
            (*state)++;
        }
        break;
    case 5:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, 0)) == TRUE) {
            func_ov103_021ef1dc(&data->lastActor, 0, TRUE);
            (*state)++;
        }
        break;
    case 6:
        for (i = 0; i < 2; i++) {
            if (data->shakeDone[i] == 0) {
                return GAMEEVENT_CONTINUE;
            }
        }
        FieldExpObj_SetActorHidden(open->expObj, open->scene, open->actor, FALSE);
        FieldExpObj_SetAnmFrame(open->expObj, open->scene, open->actor, 0, func_ov103_021ef200(data->gimmickWork));
        func_ov103_021ef1dc(open, 0, FALSE);
        FieldExpObj_SetActorHidden(data->lastActor.expObj, data->lastActor.scene, data->lastActor.actor, TRUE);
        func_ov103_021ef788(data->gimmickWork->expObj);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}


void func_ov103_021ef518(BadgeGateLastEventData *data) {
    u32 i;
    VecFx32 eye;
    VecFx32 target;
    const BadgeGateCameraShake *shake;
    u16 angle;

    for (i = 0; i < 2; i++) {
        eye = data->eyeOffset;
        target = data->eyeOffset;
        shake = &sCameraShakes[i];
        if (data->state >= shake->start && data->state <= shake->end) {
            angle = ((data->state - shake->start) << 16) / shake->period;
            eye.x += FX_SinIdx(angle) * shake->amplitudeX;
            eye.y += FX_CosIdx(angle) * shake->amplitudeY;
            target.x += FX_SinIdx(angle) * shake->amplitudeX;
            target.y += FX_CosIdx(angle) * shake->amplitudeY;
            FieldCamera_CoordsSetEyeOffset(data->camera, &eye);
            FieldCamera_CoordsSetTargetOffset(data->camera, &target);
            data->shakeDone[i] = FALSE;
            if (data->state >= shake->end) {
                FieldCamera_CoordsSetEyeOffset(data->camera, &data->eyeOffset);
                data->shakeDone[i] = TRUE;
            }
        }
    }
}


void func_ov103_021ef5dc(BadgeGateLastEventData *data) {
    if (data->state == 100) {
        GFL_SndSEPlay(0x89d);
    }
    if (data->state == 0) {
        GFL_SndSEPlay(0x89c);
    }
    if (data->state == 140) {
        GFL_SndSEPlay(0x89e);
        GFL_SndSEPlay(0x8a1);
    }
    if (data->state == 0x14a) {
        GFL_SndSEPlay(0x89f);
    }
}

void func_ov103_021ef630(BadgeGateActor *actor, FieldExpObjSystem *system) {
    FieldExpObjAnm *anm;
    SRTMatrix *matrix;

    FieldExpObj_AddScene(system, &sScene3, 3);
    actor->scene = 3;
    actor->actor = 0;
    actor->expObj = system;
    matrix = FieldExpObj_GetActorMatrixPtr(actor->expObj, 3, actor->actor);
    matrix->translation = sGatePositions[8];
    FieldExpObj_SetActorHidden(actor->expObj, 3, actor->actor, TRUE);
    func_ov036_021b8248(actor->expObj, 3, actor->actor, 1);
    anm = FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 0);
    FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 0, TRUE);
    FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 0, 0);
    FieldExpObjAnm_SetLooped(anm, FALSE);
    func_ov103_021ef1dc(actor, 0, TRUE);
    anm = FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 1);
    func_ov103_021ef1dc(actor, 1, TRUE);
    FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 1, TRUE);
    FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 1, 0);
    FieldExpObjAnm_SetLooped(anm, FALSE);
    func_ov103_021ef1dc(actor, 2, TRUE);
    FieldExpObj_SetAnm(actor->expObj, actor->scene, actor->actor, 2, FALSE);
    FieldExpObj_SetAnmFrame(actor->expObj, actor->scene, actor->actor, 2, 0);
    FieldExpObjAnm_SetLooped(FieldExpObj_GetAnmInfo(actor->expObj, actor->scene, actor->actor, 2), TRUE);
}

void func_ov103_021ef788(FieldExpObjSystem *system) {
    FieldExpObj_FreeScene(system, 3);
}
