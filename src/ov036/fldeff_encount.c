// The phenomena of effect encounters: the rustling grass, dust clouds, rippling water and shadows of flying Pokémon
// that mark a rare encounter, as 3D models with their sound. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_effect.h"
#include "field/field_map.h"
#include "field/field_sound.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"

// The effects of the phenomena, the slots of ENCOUNT_EFFECT_SPECS, in the order of field_effect.h's FLDEFF_ENCOUNT_*
enum {
    ENCOUNT_SPEC_GRASS,
    ENCOUNT_SPEC_REALLY_TALL_GRASS,
    ENCOUNT_SPEC_DUST_CLOUD,
    ENCOUNT_SPEC_WATER_0,
    ENCOUNT_SPEC_WATER_1,
    ENCOUNT_SPEC_FLYING_SHADOW,
    ENCOUNT_SPEC_MAX,
};

// The models and animations of one kind of phenomenon. A kind can have variants, such as one per season, whose files
// follow the first at variantStep apart
typedef struct {
    u8 anmCount;
    u8 variantCount : 3;
    u8 variantStep : 3;
    u8 bySeason : 1;
    u8 byExterior : 1;
    u16 se;
    u16 mdlId;
    u16 anmIds[4];
} EncountEffectSpec;

typedef struct {
    G3DActor *actor;
    G3DAnimation *anms[4];
    G3DModel *mdl;
} EncountEffectObj;

typedef struct {
    FieldEffects *effects;
    u32 unk04;
    G3DResource *mdlRes[4];
    G3DResource *anmRes[4];
    EncountEffectObj objs[4];
    EncountEffectSpec spec;
} FieldEffectEncount;

// What a phenomenon starts from
typedef struct {
    EncountSystem *system;
    FieldEffectEncount *effect;
    MMSys *actorSys;
    u32 unk0C;
    u16 x;
    u16 z;
    u32 effectIndex;
    u8 variant;
    VecFx32 pos;
} EncountTaskParams;

typedef struct {
    u16 unk00;
    // Set while the phenomenon is still, and while it is hidden
    u8 paused;
    u8 hidden;
    EncountTaskParams params;
    u8 unk2C[8];
    EncountSystem *system;
} EncountTask;

static FieldEffectEncount *EncountTask_Create(FieldEffects *effects, HeapID heapId, u32 kind);
static void EncountTask_LoadKind(FieldEffectEncount *effect, u32 kind);
static void EncountEffectObj_Create(EncountEffectObj *obj, FieldEffectEncount *effect, u8 variant);
static void EncountEffectObj_Free(EncountEffectObj *obj, FieldEffectEncount *effect);
static void EncountTask_LoadKind0Models(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc);
static void EncountTask_LoadModels(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc);
static void EncountEffect_FreeResources(FieldEffectEncount *effect);
static void EncountTask_Init(FieldEffectTask *task, void *work);
static void EncountTask_Exit(FieldEffectTask *task, void *work);
static void EncountTask_Update(FieldEffectTask *task, void *work);
static void EncountTask_PlaySound(EncountTask *encountTask, FieldEffectEncount *effect);
static void EncountTask_Draw(FieldEffectTask *task, void *work);

// The volume of the phenomenon's sound by the player's distance, in steps of 5 tiles
static const u8 ENCOUNT_EFFECT_VOLUMES[4] = { 127, 90, 60, 40 };

// The effect of each kind of phenomenon; the first three are variants of the first effect
static const u8 ENCOUNT_EFFECT_INDICES[ENCOUNT_EFFECT_NONE] = {
    ENCOUNT_SPEC_GRASS,      ENCOUNT_SPEC_GRASS,   ENCOUNT_SPEC_GRASS,   ENCOUNT_SPEC_REALLY_TALL_GRASS,
    ENCOUNT_SPEC_DUST_CLOUD, ENCOUNT_SPEC_WATER_0, ENCOUNT_SPEC_WATER_1, ENCOUNT_SPEC_FLYING_SHADOW,
};

// clang-format off
static const FieldEffectTaskVTable ENCOUNT_TASK_VTABLE = {
    sizeof(EncountTask),
    EncountTask_Init,
    EncountTask_Exit,
    EncountTask_Update,
    EncountTask_Draw,
};

// clang-format on

// clang-format off
static EncountEffectSpec ENCOUNT_EFFECT_SPECS[ENCOUNT_SPEC_MAX] = {
    { 1, 3, 4, TRUE, FALSE, SEQ_SE_FLD_70, 60, { 141, 0, 0, 0 } },
    { 1, 1, 4, TRUE, FALSE, SEQ_SE_FLD_70, 68, { 149, 0, 0, 0 } },
    { 2, 1, 1, FALSE, FALSE, SEQ_SE_FLD_73, 76, { 154, 123, 0, 0 } },
    { 1, 1, 2, FALSE, TRUE, SEQ_SE_FLD_71, 72, { 119, 0, 0, 0 } },
    { 1, 1, 1, FALSE, FALSE, SEQ_SE_FLD_71, 74, { 121, 0, 0, 0 } },
    { 3, 1, 1, FALSE, FALSE, SEQ_SE_FLD_72, 75, { 153, 139, 122, 0 } },
};

// clang-format on

void *EncountTask_CreateGrass(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_GRASS);
}

void *EncountTask_CreateReallyTallGrass(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_REALLY_TALL_GRASS);
}

void *EncountTask_CreateDustCloud(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_DUST_CLOUD);
}

void *EncountTask_CreateWater0(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_WATER_0);
}

void *EncountTask_CreateWater1(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_WATER_1);
}

void *EncountTask_CreateFlyingShadow(FieldEffects *effects, HeapID heapId) {
    return EncountTask_Create(effects, heapId, ENCOUNT_SPEC_FLYING_SHADOW);
}

void EncountTask_Delete(FieldEffects *effects, void *data) {
    EncountEffect_FreeResources(data);
    GFL_HeapFree(data);
}

static FieldEffectEncount *EncountTask_Create(FieldEffects *effects, HeapID heapId, u32 kind) {
    FieldEffectEncount *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectEncount), TRUE, "fldeff_encount.c", 286);

    effect->effects = effects;
    EncountTask_LoadKind(effect, kind);
    return effect;
}

static void EncountTask_LoadKind(FieldEffectEncount *effect, u32 kind) {
    ArcTool *arc;

    effect->spec = ENCOUNT_EFFECT_SPECS[kind];
    arc = FieldEffects_GetArc(effect->effects);
    if (kind == ENCOUNT_SPEC_GRASS) {
        EncountTask_LoadKind0Models(effect, &effect->spec, arc);
    } else {
        EncountTask_LoadModels(effect, &effect->spec, arc);
    }
}

static void EncountEffectObj_Create(EncountEffectObj *obj, FieldEffectEncount *effect, u8 variant) {
    int i;

    obj->mdl = GFL_G3DMdlCreate(effect->mdlRes[variant], 0, effect->mdlRes[variant]);
    for (i = 0; i < effect->spec.anmCount; i++) {
        obj->anms[i] = GFL_G3DAnmCreate(obj->mdl, effect->anmRes[variant * effect->spec.anmCount + i], 0);
    }
    obj->actor = GFL_G3DActorCreate(obj->mdl, obj->anms, effect->spec.anmCount);
    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DActorBindAnm(obj->actor, i);
    }
}

static void EncountEffectObj_Free(EncountEffectObj *obj, FieldEffectEncount *effect) {
    int i;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DAnmFree(obj->anms[i]);
    }
    GFL_G3DActorFree(obj->actor);
    GFL_G3DMdlFree(obj->mdl);
}

// The grass of the first kind: this season's, the next variant's, and the first
static void EncountTask_LoadKind0Models(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc) {
    u32 season = FieldEffects_GetSeason(effect->effects);

    effect->mdlRes[0] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId + season);
    GFL_G3DResUploadTexData(effect->mdlRes[0]);
    effect->anmRes[0] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0] + season);
    EncountEffectObj_Create(&effect->objs[0], effect, 0);
    season += spec->variantStep;
    effect->mdlRes[1] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId + season);
    GFL_G3DResUploadTexData(effect->mdlRes[1]);
    effect->anmRes[1] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0] + season);
    EncountEffectObj_Create(&effect->objs[1], effect, 1);
    effect->mdlRes[2] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId);
    GFL_G3DResUploadTexData(effect->mdlRes[2]);
    effect->anmRes[2] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0]);
    EncountEffectObj_Create(&effect->objs[2], effect, 2);
}

static void EncountTask_LoadModels(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc) {
    int i;
    int j;
    u32 offset = 0;
    u32 fileOffset;

    if (spec->bySeason) {
        offset = FieldEffects_GetSeason(effect->effects);
    } else if (spec->byExterior) {
        offset = FieldEffects_GetAreaIsExterior(effect->effects);
    }
    for (i = 0; i < spec->variantCount; i++) {
        fileOffset = offset + i * spec->variantStep;
        effect->mdlRes[i] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId + fileOffset);
        GFL_G3DResUploadTexData(effect->mdlRes[i]);
        for (j = 0; j < spec->anmCount; j++) {
            effect->anmRes[i * spec->anmCount + j] = GFL_G3DSysReadArcToolResource(arc, fileOffset + spec->anmIds[j]);
        }
        EncountEffectObj_Create(&effect->objs[i], effect, i);
    }
}

static void EncountEffect_FreeResources(FieldEffectEncount *effect) {
    int i;

    for (i = 0; i < effect->spec.variantCount; i++) {
        EncountEffectObj_Free(&effect->objs[i], effect);
    }
    for (i = 0; i < effect->spec.anmCount * effect->spec.variantCount; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    for (i = 0; i < effect->spec.variantCount; i++) {
        GFL_G3DResFreeTexData(effect->mdlRes[i]);
        GFL_G3DResFree(effect->mdlRes[i]);
    }
}

// Start the phenomenon of the given kind on the tile, only on the grid
FieldEffectTask *EncountTask_Start(EncountSystem *system, FieldEffects *effects, u16 x, u16 z, fx32 height, u32 kind) {
    Field *field = FieldEffects_GetField(effects);
    u32 effectIndex;
    FieldEffectEncount *effect;
    EncountTaskParams params;

    if (Field_GetResolvedControllerTypeID(field) != FIELDMAP_CTRL_TYPE_GRID) {
        return NULL;
    }
    effectIndex = ENCOUNT_EFFECT_INDICES[kind];
    effect = FieldEffects_GetHandleData(effects, FLDEFF_ENCOUNT_GRASS + effectIndex);
    if (effect == NULL) {
        return NULL;
    }
    MI_CpuFill8(&params, 0, sizeof(EncountTaskParams));
    params.system = system;
    params.effect = effect;
    params.actorSys = Field_GetActorSystem(field);
    params.x = x;
    params.z = z;
    params.effectIndex = effectIndex;
    if (effectIndex == ENCOUNT_SPEC_GRASS) {
        params.variant = kind;
    } else {
        params.variant = 0;
    }
    params.pos.x = (x << 16) + FX32_CONST(8);
    params.pos.z = (z << 16) + FX32_CONST(8);
    params.pos.y = height;
    return FieldEffects_TCBCreate(effects, &ENCOUNT_TASK_VTABLE, NULL, TRUE, &params, 0);
}

void EncountTask_SetPaused(FieldEffectTask *task, BOOL paused) {
    if (task != NULL) {
        EncountTask *encountTask = FieldEffects_GetObjectWork(task);

        encountTask->paused = paused;
    }
}

void EncountTask_SetDrawHidden(FieldEffectTask *task, BOOL hidden) {
    if (task != NULL) {
        EncountTask *encountTask = FieldEffects_GetObjectWork(task);

        encountTask->hidden = hidden;
    }
}

static void EncountTask_Init(FieldEffectTask *task, void *work) {
    EncountTask *encountTask = work;
    EncountTaskParams *params = FieldEffects_GetObjectParam(task);

    encountTask->system = params->system;
    encountTask->params = *params;
    FieldEffects_SetObjectPos(task, &params->pos);
    if (FieldEffects_GetObjectMode(task) == FALSE) {
        encountTask->unk00 = 1;
    }
    FieldEffects_UpdateObject(task);
}

static void EncountTask_Exit(FieldEffectTask *task, void *work) {
}

static void EncountTask_Update(FieldEffectTask *task, void *work) {
    int i;
    EncountTask *encountTask = work;
    FieldEffectEncount *effect = encountTask->params.effect;

    if (Field_CheckMapLoadFinished(FieldEffects_GetField(effect->effects)) && !encountTask->paused) {
        EncountTask_PlaySound(encountTask, effect);
        for (i = 0; i < effect->spec.anmCount; i++) {
            GFL_G3DActorStepAnmFrameLoop(effect->objs[encountTask->params.variant].actor, i, FX32_ONE);
        }
    }
}

// Play the sound at the start of each loop, quieter the further away the player is
static void EncountTask_PlaySound(EncountTask *encountTask, FieldEffectEncount *effect) {
    u16 distance = 0;
    fx32 frame;

    GFL_G3DActorGetAnmFrame(effect->objs[encountTask->params.variant].actor, 0, &frame);
    if (frame == 0 && EffectEncount_GetSpotDistance(encountTask->system, &distance)) {
        u8 volume;
        u16 se;
        FieldSound *sound;

        distance /= 5;
        if (distance > 3) {
            distance = 3;
        }
        volume = ENCOUNT_EFFECT_VOLUMES[distance];
        se = effect->spec.se;
        sound = GameData_GetFieldSoundSystem(encountTask->system->gameData);
        FieldSnd_PlayAmbienceEx(sound, se, volume);
    }
}

static void EncountTask_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    EncountTask *encountTask = work;
    FieldEffectEncount *effect = encountTask->params.effect;

    if (!encountTask->hidden) {
        MTX_Identity33_(&srt.rotation);
        FieldEffects_GetObjectPos(task, &srt.translation);
        GFL_G3DSysDrawObjBBoxCull(effect->objs[encountTask->params.variant].actor, &srt);
    }
}
