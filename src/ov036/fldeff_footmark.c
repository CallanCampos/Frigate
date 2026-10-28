// The footprints and tracks that actors leave in sand, snow and deep sand, which fade after a while. The name is the
// ROM's own, from GFL_HeapAllocate's file argument.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

enum {
    FOOTMARK_TEXTURE_COUNT = 26,
};

typedef struct {
    FieldEffects *effects;
    BillboardActorSys *billboardActor;
    u16 mats[FOOTMARK_TEXTURE_COUNT];
    // How far deep sand tracks sink
    fx32 sinkDepth;
} FieldEffectFootmark;

// A mark stays a while, then fades out
enum {
    FOOTMARK_STATE_WAIT,
    FOOTMARK_STATE_FADE,
};

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

static void Footmark_LoadTextures(FieldEffectFootmark *footmark);
static void Footmark_FreeTextures(FieldEffectFootmark *footmark);
static u32 Footmark_GetTexture(FieldEffectFootmark *footmark, u32 kind, u32 dir, u32 prevDir);
static void FootmarkTask_Init(FieldEffectTask *task, void *work);
static void FootmarkTask_Delete(FieldEffectTask *task, void *work);
static void FootmarkTask_Update(FieldEffectTask *task, void *work);
static void FootmarkTask_Draw(FieldEffectTask *task, void *work);

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
    FootmarkTask_Init,
    FootmarkTask_Delete,
    FootmarkTask_Update,
    FootmarkTask_Draw,
};

// clang-format on

// The textures in the field effects' archive
static const u16 FOOTMARK_TEXTURE_FILES[FOOTMARK_TEXTURE_COUNT] = {
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 39, 40, 41, 42, 43, 44, 33, 34, 35, 36, 37, 38,
};

void *Footmark_Init(FieldEffects *effects, HeapID heapId) {
    FieldEffectFootmark *footmark =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectFootmark), TRUE, "fldeff_footmark.c", 157);

    footmark->effects = effects;
    footmark->billboardActor = Field_GetEffectBillboardActor(FieldEffects_GetField(effects));
    Footmark_LoadTextures(footmark);
    footmark->sinkDepth = FX32_CONST(2);
    return footmark;
}

void Footmark_Delete(FieldEffects *effects, void *data) {
    Footmark_FreeTextures(data);
    GFL_HeapFree(data);
}

static void Footmark_LoadTextures(FieldEffectFootmark *footmark) {
    const u16 *file = FOOTMARK_TEXTURE_FILES;
    ArcTool *arc = FieldEffects_GetArc(footmark->effects);
    BillboardActorMatRequest request;
    int i;

    request.format = 2;
    request.size = 17;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BILLBOARD_ACTOR_LOAD_TRIM;
    for (i = 0; i < FOOTMARK_TEXTURE_COUNT; i++) {
        request.texResource = GFL_G3DSysReadArcToolResource(arc, *file);
        FieldEffects_ApplyLuminanceTable(footmark->effects, request.texResource);
        footmark->mats[i] = BillboardActorSys_ExecMatLoadRequests(footmark->billboardActor, &request, 1);
        file++;
    }
}

static void Footmark_FreeTextures(FieldEffectFootmark *footmark) {
    int i;

    for (i = 0; i < FOOTMARK_TEXTURE_COUNT; i++) {
        BillboardActorSys_FreeMaterials(footmark->billboardActor, footmark->mats[i], 1);
    }
}

// The texture of a mark of a kind, which the caller picks by the ground and the footprint type of the actor's model
static u32 Footmark_GetTexture(FieldEffectFootmark *footmark, u32 kind, u32 dir, u32 prevDir) {
    u32 texture = 0;
    u32 track;

    switch (kind) {
    case FOOTMARK_KIND_SAND:
        texture = dir;
        break;
    case FOOTMARK_KIND_SAND_TRACKS:
        texture = FOOTMARK_TRACK_TEXTURES[prevDir][dir];
        break;
    case FOOTMARK_KIND_SNOW:
        texture = dir + 10;
        break;
    case FOOTMARK_KIND_SNOW_TRACKS:
        track = FOOTMARK_TRACK_TEXTURES[prevDir][dir] - 4;
        texture = track + 14;
        break;
    case FOOTMARK_KIND_DEEP_SAND_TRACKS:
        track = FOOTMARK_TRACK_TEXTURES[prevDir][dir] - 4;
        texture = track + 20;
        break;
    }
    return texture;
}

void Footmark_SetActor(FieldActor *actor, FieldEffects *effects, u32 kind) {
    FootmarkTaskParams params;
    VecFx32 pos;
    u16 texture;
    FieldEffectFootmark *footmark = FieldEffects_GetHandleData(effects, FLDEFF_FOOTMARK);

    params.footmark = footmark;
    texture = Footmark_GetTexture(footmark, kind, GetActorFaceDir(actor), GetActorLastFaceDir(actor));
    CopyActorWPos(actor, &pos);
    ConvGXZToVector(FldAct_GetInitGPosX(actor), FldAct_GetInitGPosZ(actor), &pos);
    // Footprints to the left or right, whose texture is the direction, sit a little further back.
    // The retail deep-sand branch tests directional IDs outside its 20-25 track texture range.
    if (kind == FOOTMARK_KIND_SAND && (texture == DIR_LEFT || texture == DIR_RIGHT)) {
        pos.z -= FX32_CONST(2);
    } else if (kind == FOOTMARK_KIND_DEEP_SAND_TRACKS && (texture == DIR_LEFT || texture == DIR_RIGHT)) {
        pos.z -= FX32_CONST(2);
    }
    if (kind == FOOTMARK_KIND_DEEP_SAND_TRACKS) {
        pos.y -= footmark->sinkDepth;
    }
    params.texture = texture;
    FieldEffects_TCBCreate(effects, &FOOTMARK_TASK_VTABLE, &pos, 0, &params, 0);
}

static void FootmarkTask_Init(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    FootmarkTaskParams *params = FieldEffects_GetObjectParam(task);
    BillboardActorRequest request;

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
    FieldEffects_GetObjectPos(task, &request.pos);
    fmTask->actor = BillboardActorSys_ExecActorRequests(fmTask->params.footmark->billboardActor, 0, &request, 1, 3);
    if (fmTask->actor == BILLBOARD_ACTOR_NONE) {
        FieldEffects_DeleteObject(task);
    }
}

static void FootmarkTask_Delete(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    BillboardActorSys *billboardActor = fmTask->params.footmark->billboardActor;

    if (fmTask->actor != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys_DeleteActors(billboardActor, fmTask->actor, 1);
    }
}

// Stay for 16 frames, then fade out
static void FootmarkTask_Update(FieldEffectTask *task, void *work) {
    FootmarkTask *fmTask = work;
    u8 alpha;
    BillboardActorSys *billboardActor;
    BillboardActorScene *scene;
    u16 sceneActor;

    switch (fmTask->state) {
    case FOOTMARK_STATE_WAIT:
        fmTask->timer++;
        if (fmTask->timer >= 16) {
            fmTask->state++;
        }
        break;
    case FOOTMARK_STATE_FADE:
        fmTask->alpha -= 2;
        if (fmTask->alpha < 0) {
            FieldEffects_DeleteObject(task);
            return;
        }
        alpha = fmTask->alpha;
        billboardActor = fmTask->params.footmark->billboardActor;
        scene = BillboardActorSys_GetScene(billboardActor);
        sceneActor = BillboardActorSys_GetSceneActor(billboardActor, fmTask->actor);
        BillboardActorScene_SetActorAlpha(scene, sceneActor, &alpha);
        break;
    }
}

static void FootmarkTask_Draw(FieldEffectTask *task, void *work) {
}
