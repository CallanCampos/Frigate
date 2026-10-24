// The puff of dust where a field actor lands or steps. The name is the ROM's own, from GFL_HeapAllocate's file
// argument ("kemuri" is smoke). Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    u16 resGroup;
} FieldEffectSmoke;

// What a puff starts from
typedef struct {
    FieldEffectSmoke *smoke;
    VecFx32 pos;
} SmokeTaskParams;

typedef struct {
    FieldEffectSmoke *smoke;
    FieldG3DObjSystem *objSys;
    u16 objIdx;
} SmokeTask;

static void func_ov036_021a3e18(FieldEffectSmoke *smoke);
static void FldeffKemuri_FreeResource(FieldEffectSmoke *smoke);
static void FldeffKemuri_Init(FieldEffectTask *task, void *work);
static void func_ov036_021a3fac(FieldEffectTask *task, void *work);
static void func_ov036_021a3fb8(FieldEffectTask *task, void *work);
static void func_ov036_021a3fd4(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable SMOKE_TASK_VTABLE = {
    sizeof(SmokeTask),
    FldeffKemuri_Init,
    func_ov036_021a3fac,
    func_ov036_021a3fb8,
    func_ov036_021a3fd4,
};

// clang-format on

void *playSmokeEffect(FieldEffects *effects, HeapID heapId) {
    FieldEffectSmoke *smoke = GFL_HeapAllocate(heapId, sizeof(FieldEffectSmoke), TRUE, "fldeff_kemuri.c", 82);

    smoke->effects = effects;
    func_ov036_021a3e18(smoke);
    return smoke;
}

void freeSmokeEffect(FieldEffects *effects, void *data) {
    FldeffKemuri_FreeResource(data);
    GFL_HeapFree(data);
}

static void func_ov036_021a3e18(FieldEffectSmoke *smoke) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(smoke->effects);
    ArcTool *arc = FieldEffects_GetArc(smoke->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 48);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 100);
    smoke->resGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
}

static void FldeffKemuri_FreeResource(FieldEffectSmoke *smoke) {
    FieldG3DObjSystem_FreeResGroup(FieldEffects_GetFieldMapData(smoke->effects), smoke->resGroup);
}

// A puff of dust under the actor
void FldeffKemuri_Start(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    SmokeTaskParams params;

    CopyActorWPos(actor, &pos);
    pos.y += FX32_ONE;
    pos.z += FX32_CONST(6);
    params.smoke = FieldEffects_GetHandleData(effects, 1);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &SMOKE_TASK_VTABLE, NULL, 0, &params, 0);
}

// A puff of dust in front of the actor
void FldeffKemuri_StartFacing(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    SmokeTaskParams params;
    u16 angle = FldActSys_GetCameraAngle(GetActorMModelSystem(actor));

    CopyActorWPos(actor, &pos);
    pos.y += FX32_ONE;
    pos.z += FX_Mul(FX_CosIdx(angle), FX32_CONST(6));
    pos.x += FX_Mul(FX_SinIdx(angle), FX32_CONST(6));
    params.smoke = FieldEffects_GetHandleData(effects, 1);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &SMOKE_TASK_VTABLE, NULL, 0, &params, 0);
}

static void FldeffKemuri_Init(FieldEffectTask *task, void *work) {
    SmokeTask *smokeTask = work;
    SmokeTaskParams *params = FieldEffects_GetObjectParam(task);

    smokeTask->smoke = params->smoke;
    smokeTask->objSys = FieldEffects_GetFieldMapData(smokeTask->smoke->effects);
    FieldEffects_SetObjectPos(task, &params->pos);
    smokeTask->objIdx = FieldG3DObjSystem_AddObj(smokeTask->objSys, smokeTask->smoke->resGroup, 0, &params->pos);
}

static void func_ov036_021a3fac(FieldEffectTask *task, void *work) {
    SmokeTask *smokeTask = work;

    FieldG3DObjSystem_FreeObj(smokeTask->objSys, smokeTask->objIdx);
}

// End when the animation does
static void func_ov036_021a3fb8(FieldEffectTask *task, void *work) {
    SmokeTask *smokeTask = work;

    if (!FieldG3DObjSystem_StepObjAnm(smokeTask->objSys, smokeTask->objIdx, FX32_ONE)) {
        FieldEffects_DeleteObject(task);
    }
}

static void func_ov036_021a3fd4(FieldEffectTask *task, void *work) {
}
