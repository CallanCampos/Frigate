// The grass that shakes where a field actor steps: a billboard per step that plays the shaking and stays until the
// actor leaves the tile. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

#define GRASS_KIND_COUNT 8

typedef struct {
    FieldEffects *effects;
    BlActSys *blact;
    u16 matIndices[GRASS_KIND_COUNT];
} FieldEffectGrass;

// What a step's grass starts from
typedef struct {
    FieldEffectGrass *grass;
    FieldActor *actor;
    int gx;
    int gz;
    u16 uid;
    u16 zoneId;
    VecFx32 pos;
    int kind;
} GrassTaskParams;

typedef struct {
    int state;
    GrassTaskParams params;
    FieldActorIdentity identity;
    u16 unk2E;
    u16 actorIndex;
} GrassTask;

static void Grass_LoadTextures(FieldEffectGrass *grass);
static void Grass_FreeTextures(FieldEffectGrass *grass);
static void GrassTask_Init(FieldEffectTask *task, void *work);
static void GrassTask_Delete(FieldEffectTask *task, void *work);
static void GrassTask_Update(FieldEffectTask *task, void *work);
static void func_ov036_021a4320(FieldEffectTask *task, void *work);
static BOOL GrassTask_IsActorOnTile(GrassTask *grassTask);

// Whether each kind of grass ends as soon as the actor walks on, and the step of its animation it settles on
static const u8 GRASS_ENDS_ON_LEAVE[GRASS_KIND_COUNT] = { FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE };

static const u16 GRASS_REST_STEPS[GRASS_KIND_COUNT] = { 3, 3, 5, 5, 3, 3, 3, 3 };

// clang-format off
static const FieldEffectTaskVTable GRASS_TASK_VTABLE = {
    sizeof(GrassTask),
    GrassTask_Init,
    GrassTask_Delete,
    GrassTask_Update,
    func_ov036_021a4320,
};

// clang-format on

static const BlActAnimStep GRASS_ANIM_SHAKE[] = {
    { 0, 0, 0, 3 }, { 1, 0, 0, 3 }, { 2, 0, 0, 3 }, { 3, 0, 0, 4 }, { BLACT_ANIM_CMD_END, 0, 0, 0 },
};

static const BlActAnimStep GRASS_ANIM_SHAKE_LONG[] = {
    { 0, 0, 0, 2 },
    { 1, 0, 0, 2 },
    { 0, 0, 0, 2 },
    { 1, 0, 0, 2 },
    { 0, 0, 0, 2 },
    { 1, 0, 0, 2 },
    { BLACT_ANIM_CMD_END, 0, 0, 0 },
};

// Each kind's texture in each season
// clang-format off
static const u16 GRASS_TEXTURES[GRASS_KIND_COUNT][4] = {
    { 1, 2, 3, 4 },
    { 6, 7, 8, 9 },
    { 11, 12, 13, 14 },
    { 15, 16, 17, 18 },
    { 5, 5, 5, 5 },
    { 10, 10, 10, 10 },
    { 1, 1, 1, 1 },
    { 6, 6, 6, 6 },
};

// clang-format on

static const BlActAnimStep *GRASS_ANIMS[] = { GRASS_ANIM_SHAKE };

static const BlActAnimStep *GRASS_ANIMS_LONG[] = { GRASS_ANIM_SHAKE_LONG };

static const BlActAnimStep **GRASS_KIND_ANIMS[GRASS_KIND_COUNT] = {
    GRASS_ANIMS, GRASS_ANIMS, GRASS_ANIMS_LONG, GRASS_ANIMS_LONG, GRASS_ANIMS, GRASS_ANIMS, GRASS_ANIMS, GRASS_ANIMS,
};

void *loadGrassEffects(FieldEffects *effects, HeapID heapId) {
    FieldEffectGrass *grass = GFL_HeapAllocate(heapId, sizeof(FieldEffectGrass), TRUE, "fldeff_grass.c", 108);

    grass->effects = effects;
    grass->blact = Field_GetWildEffectBlAct(FieldEffects_GetField(effects));
    Grass_LoadTextures(grass);
    return grass;
}

void freeGrassEffects(FieldEffects *effects, void *data) {
    Grass_FreeTextures(data);
    GFL_HeapFree(data);
}

static void Grass_LoadTextures(FieldEffectGrass *grass) {
    int i;
    BlActMatRequest request;
    ArcTool *arc = FieldEffects_GetArc(grass->effects);
    Field_GetGameSystem(FieldEffects_GetField(grass->effects));
    u32 season = FieldEffects_GetSeason(grass->effects);

    request.format = 0;
    request.size = 36;
    request.faceWidth = 32;
    request.faceHeight = 32;
    request.mode = BLACT_LOAD_TRIM;
    for (i = 0; i < GRASS_KIND_COUNT; i++) {
        request.texResource = GFL_G3DSysReadArcToolResource(arc, GRASS_TEXTURES[i][season]);
        FieldEffects_ApplyLuminanceTable(grass->effects, request.texResource);
        grass->matIndices[i] = BlActSys_ExecMatLoadRequests(grass->blact, &request, 1);
    }
}

static void Grass_FreeTextures(FieldEffectGrass *grass) {
    int i;

    for (i = 0; i < GRASS_KIND_COUNT; i++) {
        BlActSys_FreeMaterials(grass->blact, grass->matIndices[i], 1);
    }
}

// Shake the grass of the given kind on the actor's tile; it settles at once unless animate
void GrassTask_Start(FieldEffects *effects, FieldActor *actor, BOOL animate, int kind) {
    VecFx32 pos;
    GrassTaskParams params;
    fx32 height;

    if (kind < GRASS_KIND_COUNT) {
        params.grass = FieldEffects_GetHandleData(effects, 2);
        params.actor = actor;
        params.kind = kind;
        params.uid = GetActorUID(actor);
        params.zoneId = GetActorZoneID(actor);
        params.gx = GetGPosX(actor);
        params.gz = GetGPosZ(actor);
        ConvGXZToVector(params.gx, params.gz, &pos);
        pos.y = GetActorPosY(actor);
        // The step back in z is written in both branches, which sets the registers
        if (CheckActorFlag(actor, 0x2000)) {
            pos.y += FX32_CONST(7);
            pos.z -= FX32_CONST(2);
        } else {
            GetHeightFromMap(actor, &pos, &height);
            pos.y = height + FX32_CONST(9);
            pos.z -= FX32_CONST(2);
        }
        params.pos = pos;
        FieldEffects_TCBCreate(effects, &GRASS_TASK_VTABLE, NULL, animate, &params, 0);
    }
}

static void GrassTask_Init(FieldEffectTask *task, void *work) {
    GrassTask *grassTask = work;
    GrassTaskParams *params = FieldEffects_GetObjectParam(task);
    BlActActorRequest request;
    u16 kind;
    BlActSys *blact;
    u16 actorIndex;

    grassTask->params = *params;
    FldAct_GetIdentity(params->actor, &grassTask->identity);
    FieldEffects_SetObjectPos(task, &params->pos);
    kind = grassTask->params.kind;
    request.texMat = grassTask->params.grass->matIndices[kind];
    request.scaleX = FX32_CONST(2);
    request.scaleY = FX32_CONST(2);
    request.alpha = 31;
    request.visible = TRUE;
    request.lights = 1;
    request.pos = params->pos;
    request.callback = NULL;
    request.callbackData = grassTask;
    blact = grassTask->params.grass->blact;
    grassTask->actorIndex = BlActSys_ExecActorRequests(blact, 0, &request, 1, 0);
    if (grassTask->actorIndex == 0xffff) {
        FieldEffects_DeleteObject(task);
        return;
    }
    BlActSys_SetActorAnims(blact, grassTask->actorIndex, GRASS_KIND_ANIMS[kind], 1);
    actorIndex = grassTask->actorIndex;
    BlActSys_SetActorAnim(blact, actorIndex, 0);
    BlActSys_SetActorAnimating(blact, actorIndex, TRUE);
    if (FieldEffects_GetObjectMode(task) == FALSE) {
        grassTask->state = 1;
    }
    FieldEffects_UpdateObject(task);
}

static void GrassTask_Delete(FieldEffectTask *task, void *work) {
    GrassTask *grassTask = work;
    BlActSys *blact = grassTask->params.grass->blact;

    if (grassTask->actorIndex != 0xffff) {
        BlActSys_DeleteActors(blact, grassTask->actorIndex, 1);
    }
}

static void GrassTask_Update(FieldEffectTask *task, void *work) {
    u16 cmd;
    GrassTask *grassTask = work;
    BlActSys *blact = grassTask->params.grass->blact;

    switch (grassTask->state) {
    case 0:
        // Shaking
        if (GRASS_ENDS_ON_LEAVE[grassTask->params.kind] == TRUE) {
            FieldActor *actor = grassTask->params.actor;

            if (FldAct_CheckIdentity(actor, &grassTask->identity) == TRUE && !GrassTask_IsActorOnTile(grassTask) &&
                GetActorMotionDir(actor) == 1) {
                FieldEffects_DeleteObject(task);
                return;
            }
        }
        if (BlActSys_GetActorLastCmd(blact, grassTask->actorIndex, &cmd)) {
            grassTask->state++;
        }
        break;
    case 1:
        // Settle
        if (!FldAct_CheckIdentity(grassTask->params.actor, &grassTask->identity)) {
            FieldEffects_DeleteObject(task);
            return;
        }
        BlActSys_SetActorStep(blact, grassTask->actorIndex, GRASS_REST_STEPS[grassTask->params.kind]);
        grassTask->state++;
        break;
    case 2:
        grassTask->state++;
    case 3:
        // Stay until the actor leaves the tile
        if (!FldAct_CheckIdentity(grassTask->params.actor, &grassTask->identity)) {
            FieldEffects_DeleteObject(task);
            return;
        }
        if (!GrassTask_IsActorOnTile(grassTask)) {
            FieldEffects_DeleteObject(task);
        }
        break;
    }
}

static void func_ov036_021a4320(FieldEffectTask *task, void *work) {
}

// Whether the actor is still on the grass's tile, or standing still on its first tile
static BOOL GrassTask_IsActorOnTile(GrassTask *grassTask) {
    FieldActor *actor = grassTask->params.actor;
    s16 gx = GetGPosX(actor);
    s16 gz = GetGPosZ(actor);
    u8 endsOnLeave = GRASS_ENDS_ON_LEAVE[grassTask->params.kind];

    if (grassTask->params.gx != gx || grassTask->params.gz != gz) {
        if (endsOnLeave == FALSE && GetActorMotionDir(actor) == 0) {
            gx = FldAct_GetInitGPosX(actor);
            gz = FldAct_GetInitGPosZ(actor);
            if (grassTask->params.gx == gx && grassTask->params.gz == gz) {
                return TRUE;
            }
        }
        return FALSE;
    }
    return TRUE;
}
