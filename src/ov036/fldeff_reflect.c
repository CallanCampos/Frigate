// The reflection of an actor on water and shiny floors. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. AdjustFieldCameraPitchForJoinAve's name is from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/zones.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
} FieldEffectReflect;

typedef struct {
    FieldEffectReflect *effect;
    u32 type;
    FieldActor *actor;
    MMSys *mmSys;
    BlActSys *blActSys;
} ReflectTaskParams;

typedef struct {
    // The swing of the reflection's width, around base
    s32 base;
    u32 scaleY;
    s32 swing;
    s32 swingSpeed;
    u8 created;
    u8 initialized;
    ReflectTaskParams params;
    // The material's flags and the reflection's billboard actor, as FldBlAct_AddActorPosOrReserve gives them
    u16 blAct[2];
    FieldActorIdentity identity;
} ReflectTask;

static void FldeffReflect_InitResource(FieldEffectReflect *effect);
static void FldeffReflect_DeleteResource(FieldEffectReflect *effect);
static void ReflectTask_Init(FieldEffectTask *task, void *data);
static void ReflectTask_Free(FieldEffectTask *task, void *data);
static void ReflectTask_Update(FieldEffectTask *task, void *data);
static void ReflectTask_Draw(FieldEffectTask *task, void *data);
static void ReflectTask_UpdateBillboard(u16 blActIdx, void *data);
static s32 ReflectTask_GetScaleYOffset(u32 zoneId);
static fx32 ReflectTask_GetZoneHeightOffset(u32 zoneId, u32 billboardSize);
static void ReflectTask_AdjustJoinAvenueHeight(FieldCamera *camera, u32 zoneId, u32 billboardSize, VecFx32 *pos);
static void ReflectTask_AdjustJoinAvenueDepth(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos);
static void AdjustFieldCameraPitchForJoinAve(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos);

static const FieldEffectTaskVTable REFLECT_TASK_VTABLE;

static const fx32 REFLECT_TYPE_OFFSET_Y[3] = { -0x19000, -0x21000, -0x19000 };
static const fx32 REFLECT_BILLBOARD_OFFSET_Y[3] = { 0, 0, 0 };
static const fx32 REFLECT_BILLBOARD_OFFSET_Z[3] = { 0xc000, 0x6000, 0x19000 };

void *FldeffReflect_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectReflect *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectReflect), TRUE, "fldeff_reflect.c", 131);

    effect->effects = effects;
    FldeffReflect_InitResource(effect);
    return effect;
}

void FldeffReflect_Delete(FieldEffects *effects, void *effect) {
    FldeffReflect_DeleteResource(effect);
    GFL_HeapFree(effect);
}

static void FldeffReflect_InitResource(FieldEffectReflect *effect) {
}

static void FldeffReflect_DeleteResource(FieldEffectReflect *effect) {
}

void ReflectTask_SetMMdl(MMSys *mmSys, FieldActor *actor, FieldEffects *effects, u32 type) {
    ReflectTaskParams params;

    FieldEffects_GetField(effects);
    params.type = type;
    params.actor = actor;
    params.mmSys = mmSys;
    params.blActSys = FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(mmSys));
    params.effect = FieldEffects_GetHandleData(effects, 6);
    FieldEffects_TCBCreate(effects, &REFLECT_TASK_VTABLE, NULL, 0, &params, 0);
}

static void ReflectTask_Init(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;
    ReflectTaskParams *params = FieldEffects_GetObjectParam(task);

    work->params = *params;
    work->swingSpeed = 0x40;
    work->base = 0;
    work->swing = 0;
    if (work->params.type == 2) {
        work->swingSpeed = 0;
    }
    FldAct_GetIdentity(params->actor, &work->identity);
}

static void ReflectTask_Free(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;

    if (work->created) {
        work->created = FALSE;
        FldBlAct_DeleteActorPos(work->params.mmSys, work->blAct);
    }
}

static void ReflectTask_Update(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;
    u32 blActIdx;
    u16 x;
    u16 y;

    if (!FldAct_CheckIdentity(work->params.actor, &work->identity) || !FldAct_CheckReflectingFlag(work->params.actor)) {
        FieldEffects_DeleteObject(task);
        return;
    }
    blActIdx = FldAct_GetBlActIdx(work->params.actor, 0);
    if (!work->created) {
        if (blActIdx == 0xffff) {
            return;
        }
        {
            // Created at the origin; ReflectTask_UpdateBillboard places it below the actor
            VecFx32 pos = { 0, 0, 0 };

            FldBlAct_AddActorPosOrReserve(work->params.mmSys, GetActorMdlInfo(work->params.actor), work->blAct, &pos,
                                          ReflectTask_UpdateBillboard, work, func_ov012_02166fc0(work->params.mmSys));
        }
        work->created = TRUE;
    }
    if (work->blAct[1] != 0xffff) {
        if (!work->initialized) {
            FldBlAct_GetActorSize(work->params.actor, &x, &y);
            work->base = x;
            work->scaleY = y;
            work->swing = x;
            work->initialized = TRUE;
        }
        work->swing += work->swingSpeed;
        if (work->swing >= work->base + 0x400) {
            work->swing = work->base + 0x400;
            work->swingSpeed = -work->swingSpeed;
            return;
        }
        if (work->swing <= work->base - 0x400) {
            work->swing = work->base - 0x400;
            work->swingSpeed = -work->swingSpeed;
        }
    }
}

static void ReflectTask_Draw(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;

    if (FldAct_CheckIdentity(work->params.actor, &work->identity) == 1 && work->created && work->blAct[1] != 0xffff) {
        ReflectTask_UpdateBillboard(work->blAct[1], work);
    }
}

static void ReflectTask_UpdateBillboard(u16 blActIdx, void *data) {
    ReflectTask *work = data;
    BlActSys *sys = work->params.blActSys;
    BlActScene *scene = BlActSys_GetScene(sys);
    u32 actorIdx = FldAct_GetBlActIdx(work->params.actor, 0);
    u32 zoneId;
    u16 visible;
    FieldActor *actor;
    const FieldActorConfig *config;
    VecFx32 pos;
    VecFx32 offset;
    fx32 height;
    VecFx32 target;
    FieldCamera *camera;
    u32 self;
    u32 other;
    BOOL flip;
    s16 rot[4];

    if (blActIdx == 0xffff) {
        return;
    }
    zoneId = Field_GetPlayerStateZoneID(FieldEffects_GetField(work->params.effect->effects));
    visible = BlActSys_IsActorVisible(sys, actorIdx);
    if (visible && func_ov012_0216754c(work->params.actor)) {
        visible = 0;
    }
    BlActSys_SetActorVisible(sys, blActIdx, visible);
    BlActSys_SetActorAnimating(sys, blActIdx, 0);
    actor = work->params.actor;
    config = GetActorMdlInfo(actor);
    CopyActorWPos(actor, &pos);
    pos.z += REFLECT_BILLBOARD_OFFSET_Z[config->billboardSize];
    CopyActorWPosOffset(actor, &offset);
    pos.x += offset.x;
    pos.z -= offset.z;
    if (GetHeightFromMap(actor, &pos, &height) == 1) {
        pos.y = height;
    }
    pos.y += REFLECT_TYPE_OFFSET_Y[work->params.type];
    FldAct_GetOuterDrawOffset(actor, &offset);
    pos.x += offset.x;
    pos.y -= offset.y;
    pos.z -= offset.z;
    pos.y += REFLECT_BILLBOARD_OFFSET_Y[config->billboardSize];
    if (config->billboardSize == 2) {
        if (GetTileFlags(GetCachedTileUnderActor(actor)) & 0x40) {
            pos.y -= 0x3800;
        } else {
            pos.y -= 0x8000;
        }
    }
    pos.y += ReflectTask_GetZoneHeightOffset(zoneId, config->billboardSize);
    camera = Field_GetCameraSystem(FieldEffects_GetField(work->params.effect->effects));
    ReflectTask_AdjustJoinAvenueHeight(camera, zoneId, config->billboardSize, &pos);
    camera = Field_GetCameraSystem(FieldEffects_GetField(work->params.effect->effects));
    GFL_G3DCameraGetLookatTarget(FieldCamera_GetG3DCamera(camera), &target);
    if (zoneId == ZONE_JOIN_AVENUE) {
        AdjustFieldCameraPitchForJoinAve(camera, zoneId, &target, &pos);
    } else {
        pos.x += (pos.x / FX32_ONE - target.x / FX32_ONE) * 0x12a;
    }
    ReflectTask_AdjustJoinAvenueDepth(camera, zoneId, &target, &pos);
    BlActScene_SetActorPos(scene, blActIdx, &pos);
    flip = TRUE;
    self = BlActSys_GetSceneActor(sys, actorIdx);
    other = BlActSys_GetSceneActor(sys, blActIdx);
    rot[3] = work->swing;
    rot[2] = work->scaleY;
    rot[2] += (s16)ReflectTask_GetScaleYOffset(zoneId);
    rot[1] = 0;
    rot[0] = 0;
    BlActScene_GetActorMaterial(scene, self, (u16 *)&rot[1]);
    BlActScene_SetActorMaterial(scene, other, (u16 *)&rot[1]);
    BlActScene_GetActorFace(scene, self, (u16 *)&rot[0]);
    BlActScene_SetActorFace(scene, other, (u16 *)&rot[0]);
    BlActScene_SetActorScale(scene, other, &rot[3], &rot[2]);
    BlActScene_SetActorFlipT(scene, other, &flip);
    flip = BlActScene_GetActorFlipS(scene, self);
    BlActScene_SetActorFlipS(scene, other, &flip);
}

static s32 ReflectTask_GetScaleYOffset(u32 zoneId) {
    return -0x100;
}

static fx32 ReflectTask_GetZoneHeightOffset(u32 zoneId, u32 billboardSize) {
    fx32 offset = 0;

    if (zoneId == ZONE_ICIRRUS_CITY || zoneId == ZONE_ROUTE_8) {
        if (billboardSize == 0) {
            offset = -0x800;
        }
    } else if (zoneId == ZONE_POKEMON_LEAGUE_8) {
        if (billboardSize == 0) {
            offset = 2 << 10;
        }
    } else if (zoneId == ZONE_ROUTE_10_9) {
        if (billboardSize == 0) {
            offset = -0x5800;
        }
    } else {
        if (billboardSize == 0) {
            offset = 6 << 10;
        }
    }
    return offset;
}

static void ReflectTask_AdjustJoinAvenueHeight(FieldCamera *camera, u32 zoneId, u32 billboardSize, VecFx32 *pos) {
    u32 drop;

    if (zoneId == ZONE_JOIN_AVENUE) {
        drop = 0x8000;
        if (FieldCamera_CoordsGetPitch(camera) <= 4000) {
            drop += drop / 4;
        } else if (FieldCamera_CoordsGetPitch(camera) > 10000) {
            drop -= drop / 2;
        }
        if (billboardSize == 0) {
            pos->z -= 0xc000;
            pos->y -= drop;
        }
    }
}

static void ReflectTask_AdjustJoinAvenueDepth(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos) {
    if (zoneId == ZONE_JOIN_AVENUE) {
        pos->z += (pos->z / FX32_ONE - target->z / FX32_ONE) * 0xea;
    }
}

static void AdjustFieldCameraPitchForJoinAve(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos) {
    s32 targetX;
    s32 posX;
    s32 scale;

    if (zoneId == ZONE_JOIN_AVENUE) {
        targetX = target->x / FX32_ONE;
        scale = 0x12a;
        posX = pos->x / FX32_ONE;
        if (FieldCamera_CoordsGetPitch(camera) > 10000) {
            scale -= 0x230;
        }
        pos->x += (posX - targetX) * scale;
    }
}

static const FieldEffectTaskVTable REFLECT_TASK_VTABLE = {
    sizeof(ReflectTask), ReflectTask_Init, ReflectTask_Free, ReflectTask_Update, ReflectTask_Draw,
};
