// The splash around an actor wading through water: field effect 16. The name is the ROM's own, from
// GFL_HeapAllocate's file argument.
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    u16 resGroup;
    u16 obj;
} FieldEffectSplash;

// What a splash starts from
typedef struct {
    FieldEffectSplash *splash;
    FieldActor *actor;
} SplashTaskParams;

typedef struct {
    FieldEffectSplash *splash;
    FieldActor *actor;
    FieldActorIdentity identity;
    u16 unkE;
    u16 obj;
} SplashTask;

static void func_ov036_021be9c0(FieldEffectSplash *splash);
static void FldeffSplash_UnloadModel(FieldEffectSplash *splash);
static void FldeffSplash_InitModelObj(FieldEffectTask *task, void *work);
static void func_ov036_021beaac(FieldEffectTask *task, void *work);
static void FldeffSplash_UpdateModelObj(FieldEffectTask *task, void *work);
static void FldeffSplash_DrawModelObj(FieldEffectTask *task, void *work);

static const FieldEffectTaskVTable SPLASH_TASK_VTABLE = {
    sizeof(SplashTask),          FldeffSplash_InitModelObj, func_ov036_021beaac,
    FldeffSplash_UpdateModelObj, FldeffSplash_DrawModelObj,
};

void *func_ov036_021be988(FieldEffects *effects, HeapID heapId) {
    FieldEffectSplash *splash = GFL_HeapAllocate(heapId, sizeof(FieldEffectSplash), TRUE, "fldeff_splash.c", 91);

    splash->effects = effects;
    func_ov036_021be9c0(splash);
    return splash;
}

void func_ov036_021be9ac(FieldEffects *effects, void *data) {
    FldeffSplash_UnloadModel(data);
    GFL_HeapFree(data);
}

static void func_ov036_021be9c0(FieldEffectSplash *splash) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splash->effects);
    ArcTool *arc = FieldEffects_GetArc(splash->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 95);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 110);
    splash->resGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
    splash->obj = FieldG3DObjSystem_AddObj(objSys, splash->resGroup, 0, NULL);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, splash->obj, TRUE);
}

static void FldeffSplash_UnloadModel(FieldEffectSplash *splash) {
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splash->effects);

    FieldG3DObjSystem_FreeObj(objSys, splash->obj);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup);
}

void func_ov036_021bea3c(FieldEffects *effects, FieldActor *actor) {
    SplashTaskParams params;

    params.splash = FieldEffects_GetHandleData(effects, 16);
    params.actor = actor;
    FieldEffects_TCBCreate(effects, &SPLASH_TASK_VTABLE, NULL, 0, &params, 0);
}

static void FldeffSplash_InitModelObj(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    SplashTaskParams *params = FieldEffects_GetObjectParam(task);
    VecFx32 pos;

    *(SplashTaskParams *)splashTask = *params;
    FldAct_GetIdentity(splashTask->actor, &splashTask->identity);
    CopyActorWPos(splashTask->actor, &pos);
    pos.z += FX32_CONST(8);
    FieldEffects_SetObjectPos(task, &pos);
    splashTask->obj = splashTask->splash->obj;
}

static void func_ov036_021beaac(FieldEffectTask *task, void *work) {
}

static void FldeffSplash_UpdateModelObj(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    FieldG3DObjSystem *objSys;
    u16 obj;
    VecFx32 pos;

    if (FldAct_CheckIdentity(splashTask->actor, &splashTask->identity) == FALSE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    if (FldAct_IsSplashed(splashTask->actor) == FALSE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    obj = splashTask->obj;
    objSys = FieldEffects_GetFieldMapData(splashTask->splash->effects);
    FieldG3DObjSystem_GetObjTransform(objSys, obj);
    if (FieldG3DObjSystem_GetObjectRedrawFlag(objSys, obj) == FALSE) {
        FieldG3DObjSystem_StepObjAnmLoop(objSys, obj, FX32_ONE);
        FieldG3DObjSystem_SetObjectRedrawFlag(objSys, obj, TRUE);
    }
    CopyActorWPos(splashTask->actor, &pos);
    pos.z += FX32_CONST(8);
    FieldEffects_SetObjectPos(task, &pos);
}

static void FldeffSplash_DrawModelObj(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    u16 obj = splashTask->obj;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splashTask->splash->effects);
    SRTMatrix *transform = FieldG3DObjSystem_GetObjTransform(objSys, obj);
    VecFx32 pos;

    FieldEffects_GetObjectPos(task, &pos);
    transform->translation = pos;
    FieldG3DObjSystem_DrawObj(objSys, obj);
}
