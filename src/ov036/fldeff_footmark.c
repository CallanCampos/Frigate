// The footprints and tracks that actors leave in sand, snow and deep sand, which fade after a while. The name is the
// ROM's own, from GFL_HeapAllocate's file argument.
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

enum {
    FOOTMARK_TEXTURE_COUNT = 26,
};

typedef struct {
    FieldEffects *effects;
    BlActSys *blact;
    u16 mats[FOOTMARK_TEXTURE_COUNT];
    // How far deep sand tracks sink
    fx32 sinkDepth;
} FieldEffectFootmark;

// What a mark starts from
typedef struct {
    FieldEffectFootmark *footmark;
    u16 texture;
} FootmarkTaskParams;

typedef struct {
    FootmarkTaskParams params;
    u16 actor;
    u16 state;
    u16 unk0C;
    s16 alpha;
    int timer;
} FootmarkTask;

static void func_ov036_021b4700(FieldEffectFootmark *footmark);
static void func_ov036_021b4758(FieldEffectFootmark *footmark);
static u32 func_ov036_021b4778(FieldEffectFootmark *footmark, u32 kind, u32 dir, u32 prevDir);
static void func_ov036_021b4878(FieldEffectTask *task, void *work);
static void func_ov036_021b48e8(FieldEffectTask *task, void *work);
static void func_ov036_021b4904(FieldEffectTask *task, void *work);
static void func_ov036_021b4968(FieldEffectTask *task, void *work);

// The tracks' texture, from 4, by the direction the actor came from and the one it goes to
static const u8 FOOTMARK_TRACK_TEXTURES[4][4] = {
    { 4, 4, 9, 8 },
    { 4, 4, 7, 6 },
    { 6, 8, 5, 5 },
    { 7, 9, 5, 5 },
};

// clang-format off
static const FieldEffectTaskVTable FOOTMARK_TASK_VTABLE = {
    sizeof(FootmarkTask),
    func_ov036_021b4878,
    func_ov036_021b48e8,
    func_ov036_021b4904,
    func_ov036_021b4968,
};

// clang-format on

// The textures in the field effects' archive
static const u16 FOOTMARK_TEXTURE_FILES[FOOTMARK_TEXTURE_COUNT] = {
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 39, 40, 41, 42, 43, 44, 33, 34, 35, 36, 37, 38,
};

void *func_ov036_021b46b0(FieldEffects *effects, HeapID heapId) {
    FieldEffectFootmark *footmark =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectFootmark), TRUE, "fldeff_footmark.c", 157);

    footmark->effects = effects;
    footmark->blact = Field_GetEffectBlAct(FieldEffects_GetField(effects));
    func_ov036_021b4700(footmark);
    footmark->sinkDepth = FX32_CONST(2);
    return footmark;
}

void func_ov036_021b46ec(FieldEffects *effects, void *data) {
    func_ov036_021b4758(data);
    GFL_HeapFree(data);
}

static void func_ov036_021b4700(FieldEffectFootmark *footmark) {
    const u16 *file = FOOTMARK_TEXTURE_FILES;
    ArcTool *arc = FieldEffects_GetArc(footmark->effects);
    BlActMatRequest request;
    int i;

    request.format = 2;
    request.size = 17;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BLACT_LOAD_TRIM;
    for (i = 0; i < FOOTMARK_TEXTURE_COUNT; i++) {
        request.texResource = GFL_G3DSysReadArcToolResource(arc, *file);
        FieldEffects_ApplyLuminanceTable(footmark->effects, request.texResource);
        footmark->mats[i] = BlActSys_ExecMatLoadRequests(footmark->blact, &request, 1);
        file++;
    }
}

static void func_ov036_021b4758(FieldEffectFootmark *footmark) {
    int i;

    for (i = 0; i < FOOTMARK_TEXTURE_COUNT; i++) {
        BlActSys_FreeMaterials(footmark->blact, footmark->mats[i], 1);
    }
}

// The texture of a mark of a kind, which the caller picks by the ground and the footprint type of the actor's model
static u32 func_ov036_021b4778(FieldEffectFootmark *footmark, u32 kind, u32 dir, u32 prevDir) {
    u32 texture = 0;
    u32 track;

    switch (kind) {
    case 0:
        texture = dir;
        break;
    case 1:
        texture = FOOTMARK_TRACK_TEXTURES[prevDir][dir];
        break;
    case 2:
        texture = dir + 10;
        break;
    case 3:
        track = FOOTMARK_TRACK_TEXTURES[prevDir][dir] - 4;
        texture = track + 14;
        break;
    case 4:
        track = FOOTMARK_TRACK_TEXTURES[prevDir][dir] - 4;
        texture = track + 20;
        break;
    }
    return texture;
}

void func_ov036_021b47c8(FieldActor *actor, FieldEffects *effects, u32 kind) {
    FootmarkTaskParams params;
    VecFx32 pos;
    u16 texture;
    FieldEffectFootmark *footmark = FieldEffects_GetHandleData(effects, 5);

    params.footmark = footmark;
    texture = func_ov036_021b4778(footmark, kind, GetActorFaceDir(actor), func_ov012_0216707c(actor));
    CopyActorWPos(actor, &pos);
    ConvGXZToVector(FldAct_GetInitGPosX(actor), FldAct_GetInitGPosZ(actor), &pos);
    // Footprints to the left or right, whose texture is the direction, sit a little further back.
    // BUG: Deep sand's textures start at 20, so the second test never passes and its tracks never move back
    if (kind == 0 && (texture == 2 || texture == 3)) {
        pos.z -= FX32_CONST(2);
    } else if (kind == 4 && (texture == 2 || texture == 3)) {
        pos.z -= FX32_CONST(2);
    }
    if (kind == 4) {
        pos.y -= footmark->sinkDepth;
    }
    params.texture = texture;
    FieldEffects_TCBCreate(effects, &FOOTMARK_TASK_VTABLE, &pos, 0, &params, 0);
}

static void func_ov036_021b4878(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    FootmarkTaskParams *params = func_ov036_021a3ac8(task);
    BlActActorRequest request;

    fmTask->params = *params;
    fmTask->alpha = 31;
    request.texMat = fmTask->params.footmark->mats[fmTask->params.texture];
    request.scaleX = FX16_ONE;
    request.scaleY = FX16_ONE;
    request.alpha = fmTask->alpha;
    request.visible = TRUE;
    request.lights = 1;
    request.callback = NULL;
    request.callbackData = fmTask;
    func_ov036_021a3ad4(task, &request.pos);
    fmTask->actor = BlActSys_ExecActorRequests(fmTask->params.footmark->blact, 0, &request, 1, 3);
    if (fmTask->actor == BLACT_NONE) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021b48e8(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    BlActSys *blact = fmTask->params.footmark->blact;

    if (fmTask->actor != BLACT_NONE) {
        BlActSys_DeleteActors(blact, fmTask->actor, 1);
    }
}

// Stay for 16 frames, then fade out
static void func_ov036_021b4904(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    u8 alpha;
    BlActSys *blact;
    BlActScene *scene;
    u16 sceneActor;

    switch (fmTask->state) {
    case 0:
        fmTask->timer++;
        if (fmTask->timer >= 16) {
            fmTask->state++;
        }
        break;
    case 1:
        fmTask->alpha -= 2;
        if (fmTask->alpha < 0) {
            func_ov036_021a3a70(task);
            return;
        }
        alpha = fmTask->alpha;
        blact = fmTask->params.footmark->blact;
        scene = BlActSys_GetScene(blact);
        sceneActor = func_0204f750(blact, fmTask->actor);
        func_0204ea78(scene, sceneActor, &alpha);
        break;
    }
}

static void func_ov036_021b4968(FieldEffectTask *task, void *work) {
}
