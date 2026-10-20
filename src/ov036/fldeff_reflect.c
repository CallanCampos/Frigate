// The reflection of an actor on water and shiny floors. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. AdjustFieldCameraPitchForJoinAve's name is from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
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
    // The material's flags and the reflection's billboard actor, as func_ov036_02193b14 gives them
    u16 blAct[2];
    FieldActorIdentity identity;
} ReflectTask;

static void func_ov036_021b49a4(FieldEffectReflect *effect);
static void func_ov036_021b49a8(FieldEffectReflect *effect);
static void func_ov036_021b49f4(FieldEffectTask *task, void *work);
static void func_ov036_021b4a30(FieldEffectTask *task, void *work);
static void func_ov036_021b4a48(FieldEffectTask *task, void *work);
static void func_ov036_021b4b18(FieldEffectTask *task, void *work);
static void func_ov036_021b4b44(u16 blActIdx, void *data);
static s32 func_ov036_021b4d98(u32 zoneId);
static fx32 func_ov036_021b4da0(u32 zoneId, u32 billboardSize);
static void func_ov036_021b4de8(FieldCamera *camera, u32 zoneId, u32 billboardSize, VecFx32 *pos);
static void func_ov036_021b4e3c(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos);
static void AdjustFieldCameraPitchForJoinAve(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos);

static const FieldEffectTaskVTable REFLECT_TASK_VTABLE;

static const fx32 REFLECT_TYPE_OFFSET_Y[3] = { -0x19000, -0x21000, -0x19000 };
static const fx32 REFLECT_BILLBOARD_OFFSET_Y[3] = { 0, 0, 0 };
static const fx32 REFLECT_BILLBOARD_OFFSET_Z[3] = { 0xc000, 0x6000, 0x19000 };

void *func_ov036_021b496c(FieldEffects *effects, HeapID heapId) {
    FieldEffectReflect *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectReflect), TRUE, "fldeff_reflect.c", 131);

    effect->effects = effects;
    func_ov036_021b49a4(effect);
    return effect;
}

void func_ov036_021b4990(FieldEffects *effects, void *effect) {
    func_ov036_021b49a8(effect);
    GFL_HeapFree(effect);
}

static void func_ov036_021b49a4(FieldEffectReflect *effect) {
}

static void func_ov036_021b49a8(FieldEffectReflect *effect) {
}

void func_ov036_021b49ac(MMSys *mmSys, FieldActor *actor, void *effects, u32 type) {
    ReflectTaskParams params;

    FieldEffects_GetField(effects);
    params.type = type;
    params.actor = actor;
    params.mmSys = mmSys;
    params.blActSys = FieldBlAct_GetBlActSys(FldActSys_GetFieldBlAct(mmSys));
    params.effect = FieldEffects_GetHandleData(effects, 6);
    FieldEffects_TCBCreate(effects, &REFLECT_TASK_VTABLE, NULL, 0, &params, 0);
}

static void func_ov036_021b49f4(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;
    ReflectTaskParams *params = func_ov036_021a3ac8(task);

    work->params = *params;
    work->swingSpeed = 0x40;
    work->base = 0;
    work->swing = 0;
    if (work->params.type == 2) {
        work->swingSpeed = 0;
    }
    FldAct_GetIdentity(params->actor, &work->identity);
}

static void func_ov036_021b4a30(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;

    if (work->created) {
        work->created = FALSE;
        func_ov036_02193b7c(work->params.mmSys, work->blAct);
    }
}

static void func_ov036_021b4a48(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;
    u32 blActIdx;
    VecFx32 unused;
    u16 x;
    u16 y;

    if (!FldAct_CheckIdentity(work->params.actor, &work->identity) || !FldAct_CheckReflectingFlag(work->params.actor)) {
        func_ov036_021a3a70(task);
        return;
    }
    blActIdx = FldAct_GetBlActIdx(work->params.actor, 0);
    if (!work->created) {
        if (blActIdx == 0xffff) {
            return;
        }
        unused.x = 0;
        unused.y = 0;
        unused.z = 0;
        func_ov036_02193b14(work->params.mmSys, GetActorMdlInfo(work->params.actor), work->blAct, &unused,
                            func_ov036_021b4b44, work, func_ov012_02166fc0(work->params.mmSys));
        work->created = TRUE;
    }
    if (work->blAct[1] != 0xffff) {
        if (!work->initialized) {
            func_ov036_02193bd0(work->params.actor, &x, &y);
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

static void func_ov036_021b4b18(FieldEffectTask *task, void *data) {
    ReflectTask *work = data;

    if (FldAct_CheckIdentity(work->params.actor, &work->identity) == 1 && work->created && work->blAct[1] != 0xffff) {
        func_ov036_021b4b44(work->blAct[1], work);
    }
}

static void func_ov036_021b4b44(u16 blActIdx, void *data) {
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
    visible = BlActSys_IsActorVisible(sys, (u16)actorIdx);
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
    pos.y += func_ov036_021b4da0(zoneId, config->billboardSize);
    camera = Field_GetCameraSystem(FieldEffects_GetField(work->params.effect->effects));
    func_ov036_021b4de8(camera, zoneId, config->billboardSize, &pos);
    camera = Field_GetCameraSystem(FieldEffects_GetField(work->params.effect->effects));
    GFL_G3DCameraGetLookatTarget(FieldCamera_GetG3DCamera(camera), &target);
    if (zoneId == 0x1ea) {
        AdjustFieldCameraPitchForJoinAve(camera, zoneId, &target, &pos);
    } else {
        pos.x += (pos.x / FX32_ONE - target.x / FX32_ONE) * 0x12a;
    }
    func_ov036_021b4e3c(camera, zoneId, &target, &pos);
    BlActScene_SetActorPos(scene, blActIdx, &pos);
    flip = TRUE;
    self = BlActSys_GetSceneActor(sys, (u16)actorIdx);
    other = BlActSys_GetSceneActor(sys, blActIdx);
    rot[3] = work->swing;
    rot[2] = work->scaleY;
    rot[2] += (s16)func_ov036_021b4d98(zoneId);
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

static s32 func_ov036_021b4d98(u32 zoneId) {
    return -0x100;
}

static fx32 func_ov036_021b4da0(u32 zoneId, u32 billboardSize) {
    fx32 offset = 0;

    if (zoneId == 0x71 || zoneId == 0x159) {
        if (billboardSize == 0) {
            offset = -0x800;
        }
    } else if (zoneId == 0x8f) {
        if (billboardSize == 0) {
            offset = 2 << 10;
        }
    } else if (zoneId == 0x16b) {
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

static void func_ov036_021b4de8(FieldCamera *camera, u32 zoneId, u32 billboardSize, VecFx32 *pos) {
    u32 drop;

    if (zoneId == 0x1ea) {
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

static void func_ov036_021b4e3c(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos) {
    if (zoneId == 0x1ea) {
        pos->z += (pos->z / FX32_ONE - target->z / FX32_ONE) * 0xea;
    }
}

static void AdjustFieldCameraPitchForJoinAve(FieldCamera *camera, u32 zoneId, const VecFx32 *target, VecFx32 *pos) {
    s32 targetX;
    s32 posX;
    s32 scale;

    if (zoneId == 0x1ea) {
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
    sizeof(ReflectTask), func_ov036_021b49f4, func_ov036_021b4a30, func_ov036_021b4a48, func_ov036_021b4b18,
};
