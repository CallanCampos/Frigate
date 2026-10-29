// The sphere model of the darkness of a dark cave, which field_flash.c scales and fades, and the script commands that
// play a field effect. The name is the ROM's string
#include "types.h"
#include "field/encounter_effect.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/fld_wipe_3dobj.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"
#include "system/game_system.h"

struct FieldFlashG3D {
    G3DResource *resource;
    G3DActor *actor;
};

// The scripts' ID of a field effect and the field effect it starts
typedef struct {
    u16 scriptId;
    u16 effectId;
} FieldEffectScriptEntry;

static const FieldEffectScriptEntry FIELD_EFFECT_SCRIPT_ENTRIES[31] = {
    { 0x268, 0x0 },  { 0x269, 0x27 }, { 0x26e, 0x28 }, { 0x1d2, 0x29 }, { 0x1b5, 0x1 },  { 0x1ba, 0x2a },
    { 0x1bf, 0x2b }, { 0x1c2, 0x2c }, { 0x1d1, 0x2d }, { 0x1d7, 0x2e }, { 0x26d, 0x41 }, { 0x272, 0x30 },
    { 0x242, 0x31 }, { 0x26f, 0x32 }, { 0x270, 0x33 }, { 0x271, 0x34 }, { 0x243, 0x35 }, { 0x273, 0x45 },
    { 0x276, 0x46 }, { 0x277, 0x47 }, { 0x278, 0x48 }, { 0x279, 0x49 }, { 0x27a, 0x4a }, { 0x27b, 0x4b },
    { 0x274, 0x4c }, { 0x275, 0x4d }, { 0x27c, 0x41 }, { 0x27d, 0x30 }, { 0x1c5, 0x70 }, { 0x1ca, 0x71 },
    { 0x27e, 0x72 },
};

FieldFlashG3D *FieldFlashG3D_Create(HeapID heapId) {
    FieldFlashG3D *flash = GFL_HeapAllocate(heapId, sizeof(FieldFlashG3D), TRUE, "fld_wipe_3dobj.c", 36);

    flash->resource = GFL_G3DSysReadArcSysResource(73, 0);
    flash->actor = GFL_G3DActorCreate(GFL_G3DMdlCreate(flash->resource, 0, NULL), NULL, 0);
    return flash;
}

void FieldFlashG3D_Free(FieldFlashG3D *flash) {
    GFL_G3DMdlFree(GFL_G3DActorGetMdl(flash->actor));
    GFL_G3DActorFree(flash->actor);
    GFL_G3DResFree(flash->resource);
    GFL_HeapFree(flash);
}

void FieldFlashG3D_Draw(FieldFlashG3D *flash, fx32 scale, u32 alpha) {
    SRTMatrix transform;
    FxLookAt lookAt;
    VecFx32 dir;
    NNSG3dResMdl *mdl;
    fx32 height;

    if (scale != 0) {
        GFL_G3DSysMtxGetViewLookAt(&lookAt);
        transform.translation = lookAt.position;
        VEC_Subtract(&lookAt.target, &lookAt.position, &dir);
        VEC_Normalize(&dir, &dir);
        height = dir.y > 0 ? dir.y : -dir.y;
        transform.scale.x = scale;
        transform.scale.y = FX_Mul(scale, height);
        transform.scale.z = FX32_CONST(4);
        MI_Copy36B(NNS_G3dGlbGetInvV(), &transform.rotation);
        mdl = GFL_G3DMdlGetEngineModel(GFL_G3DActorGetMdl(flash->actor))->resMdl;
        NNS_G3dMdlSetMdlLightEnableFlag(mdl, 0, 0);
        NNS_G3dMdlSetMdlPolygonID(mdl, 0, 0);
        NNS_G3dMdlSetMdlCullMode(mdl, 0, GX_CULL_BACK);
        NNS_G3dMdlSetMdlAlpha(mdl, 0, alpha);
        NNS_G3dMdlSetMdlPolygonMode(mdl, 0, GX_POLYGONMODE_MODULATE);
        GFL_G3DSysDrawObj(flash->actor, &transform);
    }
}

BOOL Scrcmd_FldWipe3dobj_PlayFieldEffect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    void *g3dCi = Field_Get3DCi(GSYS_GetField(gsys));
    u16 effectId = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = EventFieldEffect_Create(gsys, g3dCi, effectId);

    if (event != NULL) {
        ScriptWork_CallEvent(work, event);
    }
    return TRUE;
}

BOOL Scrcmd_FldWipe3dobj_PlayHMCutInEffect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    void *g3dCi = Field_Get3DCi(GSYS_GetField(gsys));
    u16 partySlot = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = EventFieldEffect_CreatePokeSprite(gsys, g3dCi, partySlot);

    if (event != NULL) {
        ScriptWork_CallEvent(work, event);
    }
    return TRUE;
}

BOOL Scrcmd_FldWipe3dobj_PlayAlderFlyEffect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    void *g3dCi = Field_Get3DCi(GSYS_GetField(gsys));
    u16 param = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = EventFieldEffect_CreateAlderFly(gsys, g3dCi, param);

    if (event != NULL) {
        ScriptWork_CallEvent(work, event);
    }
    return TRUE;
}

BOOL Scrcmd_FldWipe3dobj_FieldEffect(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    void *g3dCi = Field_Get3DCi(GSYS_GetField(gsys));
    u16 scriptId = ScriptReadAny(vm, env);
    BOOL found = FALSE;
    u16 effectId;
    u32 i;

    for (i = 0; i < 31; i++) {
        if (scriptId == FIELD_EFFECT_SCRIPT_ENTRIES[i].scriptId) {
            found = TRUE;
            effectId = FIELD_EFFECT_SCRIPT_ENTRIES[i].effectId;
            break;
        }
    }
    if (found) {
        ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
        GameEvent *event = EventFieldEffect_Create(gsys, g3dCi, effectId);

        if (event != NULL) {
            ScriptWork_CallEvent(work, event);
        }
    }
    return TRUE;
}
