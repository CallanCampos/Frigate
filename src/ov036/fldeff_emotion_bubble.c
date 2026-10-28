// The emotion bubbles that pop up over a field actor's head: "!", "?", a music note and "...", by kind. The ROM names
// it fldeff_gyoe.c, by GFL_HeapAllocate's file argument ("gyoe" is a cry of surprise).
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"

// A bubble hops up and comes back down, then stays for its wait, and is done
enum {
    EMOTION_BUBBLE_STATE_HOP,
    EMOTION_BUBBLE_STATE_WAIT,
    EMOTION_BUBBLE_STATE_DONE,
};

typedef struct {
    FieldEffects *effects;
    G3DResource *resources[EMOTION_BUBBLE_KIND_COUNT];
    G3DModel *models[EMOTION_BUBBLE_KIND_COUNT];
    G3DActor *actors[EMOTION_BUBBLE_KIND_COUNT];
    // How far the bubbles are drawn toward the camera, so that nothing hides them
    u16 depthOffset;
} FieldEffectEmotionBubble;

// What a bubble starts from
typedef struct {
    u32 kind;
    BOOL playSE;
    FieldEffectEmotionBubble *emotionBubble;
    FieldActor *actor;
    FieldActorIdentity identity;
    Field *field;
} EmotionBubbleTaskParams;

typedef struct {
    EmotionBubbleTaskParams params;
    u32 state;
    int timer;
    BOOL done;
    // The bubble's hop above its resting height, and how fast it rises
    fx32 height;
    fx32 speed;
} EmotionBubbleTask;

static void FldeffEmotionBubble_InitResource(FieldEffectEmotionBubble *emotionBubble);
static void FldeffEmotionBubble_DeleteResource(FieldEffectEmotionBubble *emotionBubble);
static void FldeffEmotionBubble_InitHeldTask(FieldEffectTask *task, void *work);
static void FldeffEmotionBubble_DeleteTask(FieldEffectTask *task, void *work);
static void FldeffEmotionBubble_UpdateHeldTask(FieldEffectTask *task, void *work);
static void FldeffEmotionBubble_UpdateTask(FieldEffectTask *task, void *work);
static void FldeffEmotionBubble_DrawTask(FieldEffectTask *task, void *work);
static void FldeffEmotionBubble_PlaySe(u32 kind);
static void FldeffEmotionBubble_RotateByCamera(G3DCamera *camera, const VecFx32 *src, VecFx32 *dest);
static void FldeffEmotionBubble_StepAnimation(EmotionBubbleTask *emotionBubbleTask, VecFx32 *pos);

// The models of the bubbles in the field effects' archive, by kind
static const u32 EMOTION_BUBBLE_MODEL_FILES[EMOTION_BUBBLE_KIND_COUNT] = { 50, 51, 52, 53 };

// A bubble that stays when it is done, until its owner ends it
// clang-format off
static const FieldEffectTaskVTable EMOTION_BUBBLE_TASK_VTABLE = {
    sizeof(EmotionBubbleTask),
    FldeffEmotionBubble_InitHeldTask,
    FldeffEmotionBubble_DeleteTask,
    FldeffEmotionBubble_UpdateHeldTask,
    FldeffEmotionBubble_DrawTask,
};

// clang-format on

// A bubble that ends itself when it is done
// clang-format off
static const FieldEffectTaskVTable EMOTION_BUBBLE_AUTO_END_TASK_VTABLE = {
    sizeof(EmotionBubbleTask),
    FldeffEmotionBubble_InitHeldTask,
    FldeffEmotionBubble_DeleteTask,
    FldeffEmotionBubble_UpdateTask,
    FldeffEmotionBubble_DrawTask,
};

// clang-format on

void *FldeffEmotionBubble_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectEmotionBubble *emotionBubble =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectEmotionBubble), TRUE, "fldeff_gyoe.c", 111);
    int i;

    emotionBubble->effects = effects;
    FldeffEmotionBubble_InitResource(emotionBubble);
    for (i = 0; i < EMOTION_BUBBLE_KIND_COUNT; i++) {
        NNS_G3dMdlSetMdlFogEnableFlagAll(GFL_G3DMdlGetEngineModel(emotionBubble->models[i])->resMdl, FALSE);
    }
    return emotionBubble;
}

void FldeffEmotionBubble_Delete(FieldEffects *effects, void *data) {
    FldeffEmotionBubble_DeleteResource(data);
    GFL_HeapFree(data);
}

static void FldeffEmotionBubble_InitResource(FieldEffectEmotionBubble *emotionBubble) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(emotionBubble->effects);

    for (i = 0; i < EMOTION_BUBBLE_KIND_COUNT; i++) {
        emotionBubble->resources[i] = GFL_G3DSysReadArcToolResource(arc, EMOTION_BUBBLE_MODEL_FILES[i]);
        GFL_G3DResUploadTexData(emotionBubble->resources[i]);
        emotionBubble->models[i] = GFL_G3DMdlCreate(emotionBubble->resources[i], 0, emotionBubble->resources[i]);
        emotionBubble->actors[i] = GFL_G3DActorCreate(emotionBubble->models[i], NULL, 0);
    }
    emotionBubble->depthOffset = 4;
}

static void FldeffEmotionBubble_DeleteResource(FieldEffectEmotionBubble *emotionBubble) {
    int i;

    for (i = 0; i < EMOTION_BUBBLE_KIND_COUNT; i++) {
        GFL_G3DActorFree(emotionBubble->actors[i]);
        GFL_G3DMdlFree(emotionBubble->models[i]);
        GFL_G3DResFree(emotionBubble->resources[i]);
    }
}

FieldEffectTask *FldeffEmotionBubble_SetActor(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE) {
    EmotionBubbleTaskParams params;

    params.kind = kind;
    params.playSE = playSE;
    params.emotionBubble = FieldEffects_GetHandleData(effects, FLDEFF_EMOTION_BUBBLE);
    params.actor = actor;
    FldAct_GetIdentity(actor, &params.identity);
    params.field = FieldEffects_GetField(effects);
    if (playSE == TRUE) {
        FldeffEmotionBubble_PlaySe(kind);
    }
    return FieldEffects_TCBCreate(effects, &EMOTION_BUBBLE_TASK_VTABLE, NULL, 0, &params, 0);
}

FieldEffectTask *FldeffEmotionBubble_StartHeld(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE) {
    EmotionBubbleTaskParams params;

    params.kind = kind;
    params.playSE = playSE;
    params.emotionBubble = FieldEffects_GetHandleData(effects, FLDEFF_EMOTION_BUBBLE);
    params.actor = actor;
    FldAct_GetIdentity(actor, &params.identity);
    params.field = FieldEffects_GetField(effects);
    if (playSE == TRUE) {
        FldeffEmotionBubble_PlaySe(kind);
    }
    return FieldEffects_TCBCreate(effects, &EMOTION_BUBBLE_AUTO_END_TASK_VTABLE, NULL, 0, &params, 0);
}

BOOL FldeffEmotionBubble_IsFinished(FieldEffectTask *task) {
    if (task != NULL) {
        EmotionBubbleTask *emotionBubbleTask = FieldEffects_GetObjectWork(task);

        return emotionBubbleTask->done;
    }
    return TRUE;
}

static void FldeffEmotionBubble_InitHeldTask(FieldEffectTask *task, void *work) {
    EmotionBubbleTask *emotionBubbleTask = work;
    EmotionBubbleTaskParams *params = FieldEffects_GetObjectParam(task);

    emotionBubbleTask->params = *params;
    emotionBubbleTask->speed = FX32_CONST(6);
    FieldEffects_UpdateObject(task);
}

static void FldeffEmotionBubble_DeleteTask(FieldEffectTask *task, void *work) {
}

// Follow the actor; when it is gone, the bubble is done
static void FldeffEmotionBubble_UpdateHeldTask(FieldEffectTask *task, void *work) {
    EmotionBubbleTask *emotionBubbleTask = work;
    VecFx32 pos;

    if (!FldAct_CheckIdentity(emotionBubbleTask->params.actor, &emotionBubbleTask->params.identity)) {
        emotionBubbleTask->done = TRUE;
        return;
    }
    CopyActorWPos(emotionBubbleTask->params.actor, &pos);
    FldeffEmotionBubble_StepAnimation(emotionBubbleTask, &pos);
    FieldEffects_SetObjectPos(task, &pos);
}

// Follow the actor, and end once the bubble is done or the actor is gone
static void FldeffEmotionBubble_UpdateTask(FieldEffectTask *task, void *work) {
    EmotionBubbleTask *emotionBubbleTask = work;
    VecFx32 pos;

    if (emotionBubbleTask->done == TRUE ||
        !FldAct_CheckIdentity(emotionBubbleTask->params.actor, &emotionBubbleTask->params.identity)) {
        FieldEffects_DeleteObject(task);
        return;
    }
    CopyActorWPos(emotionBubbleTask->params.actor, &pos);
    FldeffEmotionBubble_StepAnimation(emotionBubbleTask, &pos);
    FieldEffects_SetObjectPos(task, &pos);
}

// Draw the bubble moved toward the camera by its depth offset, through the projection matrix
static void FldeffEmotionBubble_DrawTask(FieldEffectTask *task, void *work) {
    EmotionBubbleTask *emotionBubbleTask = work;
    SRTMatrix status = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    const MtxFx44 *proj = NNS_G3dGlbGetProjectionMtx();
    MtxFx44 saved;
    MtxFx44 mtx;
    fx32 offset = FX32_CONST(emotionBubbleTask->params.emotionBubble->depthOffset);

    saved = *proj;
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], offset);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3dGlbFlushP();
    NNS_G3dGeFlushBuffer();
    MTX_Identity33_(&status.rotation);
    FieldEffects_GetObjectPos(task, &status.translation);
    GFL_G3DSysDrawObjBBoxCull(emotionBubbleTask->params.emotionBubble->actors[emotionBubbleTask->params.kind], &status);
    NNS_G3dGlbSetProjectionMtx(&saved);
    NNS_G3dGlbFlushP();
    NNS_G3dGeFlushBuffer();
}

static void FldeffEmotionBubble_PlaySe(u32 kind) {
    u32 se = 0;

    switch (kind) {
    case EMOTION_BUBBLE_EXCLAMATION:
        se = SEQ_SE_FLD_07;
        break;
    case EMOTION_BUBBLE_QUESTION:
        se = SEQ_SE_SYS_62;
        break;
    case EMOTION_BUBBLE_NOTE:
        se = SEQ_SE_SYS_63;
        break;
    case EMOTION_BUBBLE_ELLIPSIS:
        return;
    }
    GFL_SndSEPlay(se);
}

// Turns src from the camera's view space into the world's
static void FldeffEmotionBubble_RotateByCamera(G3DCamera *camera, const VecFx32 *src, VecFx32 *dest) {
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
    MTX_MultVec33(src, &rot, dest);
}

// The bubble hops up and comes back down, then stays for 30 frames, above the actor's head as the camera sees it
static void FldeffEmotionBubble_StepAnimation(EmotionBubbleTask *emotionBubbleTask, VecFx32 *pos) {
    VecFx32 vec;
    G3DCamera *camera;

    switch (emotionBubbleTask->state) {
    case EMOTION_BUBBLE_STATE_HOP:
        emotionBubbleTask->height += emotionBubbleTask->speed;
        if (emotionBubbleTask->height != 0) {
            emotionBubbleTask->speed -= FX32_CONST(2);
        } else {
            emotionBubbleTask->speed = 0;
            emotionBubbleTask->state++;
        }
        break;
    case EMOTION_BUBBLE_STATE_WAIT:
        emotionBubbleTask->timer++;
        if (emotionBubbleTask->timer >= 30) {
            emotionBubbleTask->state++;
            emotionBubbleTask->done = TRUE;
        }
        break;
    }
    camera = FieldCamera_GetG3DCamera(Field_GetCameraSystem(emotionBubbleTask->params.field));
    {
        VecFx32 offset = { 0, emotionBubbleTask->height + FX32_CONST(22), 0 };

        vec = offset;
    }
    FldeffEmotionBubble_RotateByCamera(camera, &vec, &vec);
    VEC_Add(&vec, pos, pos);
}
