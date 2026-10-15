// The emotion bubbles that pop up over a field actor's head: "!", "?", a music note and "...", by kind. The name is the
// ROM's own, from GFL_HeapAllocate's file argument ("gyoe" is a cry of surprise).
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

enum {
    GYOE_KIND_COUNT = 4,
};

typedef struct {
    FieldEffects *effects;
    void *resources[GYOE_KIND_COUNT];
    G3DModel *models[GYOE_KIND_COUNT];
    G3DActor *actors[GYOE_KIND_COUNT];
    // How far the bubbles are drawn toward the camera, so that nothing hides them
    u16 depthOffset;
} FieldEffectGyoe;

// What a bubble starts from
typedef struct {
    u32 kind;
    BOOL playSE;
    FieldEffectGyoe *gyoe;
    FieldActor *actor;
    FieldActorIdentity identity;
    Field *field;
} GyoeTaskParams;

typedef struct {
    GyoeTaskParams params;
    u32 state;
    int timer;
    BOOL done;
    // The bubble's hop above its resting height, and how fast it rises
    fx32 height;
    fx32 speed;
} GyoeTask;

static void func_ov036_021b3ea8(FieldEffectGyoe *gyoe);
static void func_ov036_021b3ef0(FieldEffectGyoe *gyoe);
static void func_ov036_021b3fc8(FieldEffectTask *task, void *work);
static void func_ov036_021b3ff4(FieldEffectTask *task, void *work);
static void func_ov036_021b3ff8(FieldEffectTask *task, void *work);
static void func_ov036_021b4034(FieldEffectTask *task, void *work);
static void func_ov036_021b4078(FieldEffectTask *task, void *work);
static void func_ov036_021b4164(u32 kind);
static void func_ov036_021b41a0(G3DCamera *camera, const VecFx32 *src, VecFx32 *dest);
static void func_ov036_021b41f4(GyoeTask *gyoeTask, VecFx32 *pos);

// The models of the bubbles in the field effects' archive, by kind
static const u32 GYOE_MODEL_FILES[GYOE_KIND_COUNT] = { 50, 51, 52, 53 };

// A bubble that stays when it is done, until its owner ends it
static const FieldEffectTaskVTable GYOE_TASK_VTABLE = {
    sizeof(GyoeTask),
    func_ov036_021b3fc8,
    func_ov036_021b3ff4,
    func_ov036_021b3ff8,
    func_ov036_021b4078,
};

// A bubble that ends itself when it is done
static const FieldEffectTaskVTable GYOE_AUTO_END_TASK_VTABLE = {
    sizeof(GyoeTask),
    func_ov036_021b3fc8,
    func_ov036_021b3ff4,
    func_ov036_021b4034,
    func_ov036_021b4078,
};

void *func_ov036_021b3e50(FieldEffects *effects, HeapID heapId) {
    FieldEffectGyoe *gyoe = GFL_HeapAllocate(heapId, sizeof(FieldEffectGyoe), TRUE, "fldeff_gyoe.c", 111);
    int i;

    gyoe->effects = effects;
    func_ov036_021b3ea8(gyoe);
    for (i = 0; i < GYOE_KIND_COUNT; i++) {
        NNS_G3dMdlSetMdlFogEnableFlagAll(GFL_G3DMdlGetEngineModel(gyoe->models[i])->resMdl, FALSE);
    }
    return gyoe;
}

void func_ov036_021b3e94(FieldEffects *effects, void *data) {
    func_ov036_021b3ef0(data);
    GFL_HeapFree(data);
}

static void func_ov036_021b3ea8(FieldEffectGyoe *gyoe) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(gyoe->effects);

    for (i = 0; i < GYOE_KIND_COUNT; i++) {
        gyoe->resources[i] = GFL_G3DSysReadArcToolResource(arc, GYOE_MODEL_FILES[i]);
        GFL_G3DResUploadTexData(gyoe->resources[i]);
        gyoe->models[i] = GFL_G3DMdlCreate(gyoe->resources[i], 0, gyoe->resources[i]);
        gyoe->actors[i] = GFL_G3DActorCreate(gyoe->models[i], NULL, 0);
    }
    gyoe->depthOffset = 4;
}

static void func_ov036_021b3ef0(FieldEffectGyoe *gyoe) {
    int i;

    for (i = 0; i < GYOE_KIND_COUNT; i++) {
        GFL_G3DActorFree(gyoe->actors[i]);
        GFL_G3DMdlFree(gyoe->models[i]);
        GFL_G3DResFree(gyoe->resources[i]);
    }
}

FieldEffectTask *func_ov036_021b3f14(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE) {
    GyoeTaskParams params;

    params.kind = kind;
    params.playSE = playSE;
    params.gyoe = FieldEffects_GetHandleData(effects, 4);
    params.actor = actor;
    func_ov012_02167d88(actor, &params.identity);
    params.field = FieldEffects_GetField(effects);
    if (playSE == TRUE) {
        func_ov036_021b4164(kind);
    }
    return FieldEffects_TCBCreate(effects, &GYOE_TASK_VTABLE, NULL, 0, &params, 0);
}

FieldEffectTask *func_ov036_021b3f64(FieldEffects *effects, FieldActor *actor, u32 kind, BOOL playSE) {
    GyoeTaskParams params;

    params.kind = kind;
    params.playSE = playSE;
    params.gyoe = FieldEffects_GetHandleData(effects, 4);
    params.actor = actor;
    func_ov012_02167d88(actor, &params.identity);
    params.field = FieldEffects_GetField(effects);
    if (playSE == TRUE) {
        func_ov036_021b4164(kind);
    }
    return FieldEffects_TCBCreate(effects, &GYOE_AUTO_END_TASK_VTABLE, NULL, 0, &params, 0);
}

BOOL func_ov036_021b3fb4(FieldEffectTask *task) {
    if (task != NULL) {
        GyoeTask *gyoeTask = func_ov036_021a3afc(task);

        return gyoeTask->done;
    }
    return TRUE;
}

static void func_ov036_021b3fc8(FieldEffectTask *task, void *work) {
    GyoeTask *gyoeTask = work;
    GyoeTaskParams *params = func_ov036_021a3ac8(task);

    gyoeTask->params = *params;
    gyoeTask->speed = FX32_CONST(6);
    func_ov036_021a3a94(task);
}

static void func_ov036_021b3ff4(FieldEffectTask *task, void *work) {
}

// Follow the actor; when it is gone, the bubble is done
static void func_ov036_021b3ff8(FieldEffectTask *task, void *work) {
    GyoeTask *gyoeTask = work;
    VecFx32 pos;

    if (!func_ov012_02167da8(gyoeTask->params.actor, &gyoeTask->params.identity)) {
        gyoeTask->done = TRUE;
        return;
    }
    CopyActorWPos(gyoeTask->params.actor, &pos);
    func_ov036_021b41f4(gyoeTask, &pos);
    func_ov036_021a3ae8(task, &pos);
}

// Follow the actor, and end once the bubble is done or the actor is gone
static void func_ov036_021b4034(FieldEffectTask *task, void *work) {
    GyoeTask *gyoeTask = work;
    VecFx32 pos;

    if (gyoeTask->done == TRUE || !func_ov012_02167da8(gyoeTask->params.actor, &gyoeTask->params.identity)) {
        func_ov036_021a3a70(task);
        return;
    }
    CopyActorWPos(gyoeTask->params.actor, &pos);
    func_ov036_021b41f4(gyoeTask, &pos);
    func_ov036_021a3ae8(task, &pos);
}

// Draw the bubble moved toward the camera by its depth offset, through the projection matrix
static void func_ov036_021b4078(FieldEffectTask *task, void *work) {
    GyoeTask *gyoeTask = work;
    SRTMatrix status = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    const MtxFx44 *proj = NNS_G3dGlbGetProjectionMtx();
    MtxFx44 saved;
    MtxFx44 mtx;
    fx32 offset = FX32_CONST(gyoeTask->params.gyoe->depthOffset);

    saved = *proj;
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], offset);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3DFlushRenderState();
    NNS_G3DWaitFIFO();
    MTX_Identity33_(&status.rotation);
    func_ov036_021a3ad4(task, &status.translation);
    GFL_G3DSysDrawObjBBoxCull(gyoeTask->params.gyoe->actors[gyoeTask->params.kind], &status);
    NNS_G3dGlbSetProjectionMtx(&saved);
    NNS_G3DFlushRenderState();
    NNS_G3DWaitFIFO();
}

static void func_ov036_021b4164(u32 kind) {
    u32 se = 0;

    switch (kind) {
    case 0:
        se = SEQ_SE_FLD_07;
        break;
    case 1:
        se = SEQ_SE_SYS_62;
        break;
    case 2:
        se = SEQ_SE_SYS_63;
        break;
    case 3:
        return;
    }
    GFL_SndSEPlay(se);
}

// Turns src from the camera's view space into the world's
static void func_ov036_021b41a0(G3DCamera *camera, const VecFx32 *src, VecFx32 *dest) {
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
    MAT3_Invert(&rot, &rot);
    MTX_MultVec33(src, &rot, dest);
}

// The bubble hops up and comes back down, then stays for 30 frames, above the actor's head as the camera sees it
static void func_ov036_021b41f4(GyoeTask *gyoeTask, VecFx32 *pos) {
    VecFx32 vec;
    G3DCamera *camera;

    switch (gyoeTask->state) {
    case 0:
        gyoeTask->height += gyoeTask->speed;
        if (gyoeTask->height != 0) {
            gyoeTask->speed -= FX32_CONST(2);
        } else {
            gyoeTask->speed = 0;
            gyoeTask->state++;
        }
        break;
    case 1:
        gyoeTask->timer++;
        if (gyoeTask->timer >= 30) {
            gyoeTask->state++;
            gyoeTask->done = TRUE;
        }
        break;
    }
    camera = FieldCamera_GetG3DCamera(Field_GetCameraSystem(gyoeTask->params.field));
    {
        VecFx32 offset = { 0, gyoeTask->height + FX32_CONST(22), 0 };

        vec = offset;
    }
    func_ov036_021b41a0(camera, &vec, &vec);
    VEC_Add(&vec, pos, pos);
}
