// The phenomena of effect encounters: the rustling grass, dust clouds, rippling water and shadows of flying Pokémon
// that mark a rare encounter, as 3D models with their sound. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "field/field_sound.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"

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
    void *anms[4];
    G3DModel *mdl;
} EncountEffectObj;

typedef struct {
    FieldEffects *effects;
    u32 unk04;
    void *mdlRes[4];
    void *anmRes[4];
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

static void *func_ov036_021a5100(FieldEffects *effects, HeapID heapId, u32 kind);
static void func_ov036_021a5130(FieldEffectEncount *effect, u32 kind);
static void func_ov036_021a5178(EncountEffectObj *obj, FieldEffectEncount *effect, u8 variant);
static void func_ov036_021a51f0(EncountEffectObj *obj, FieldEffectEncount *effect);
static void func_ov036_021a5228(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc);
static void func_ov036_021a52b8(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc);
static void func_ov036_021a5368(FieldEffectEncount *effect);
static void func_ov036_021a54b8(FieldEffectTask *task, void *work);
static void func_ov036_021a54f8(FieldEffectTask *task, void *work);
static void func_ov036_021a54fc(FieldEffectTask *task, void *work);
static void func_ov036_021a5550(EncountTask *encountTask, FieldEffectEncount *effect);
static void func_ov036_021a55b8(FieldEffectTask *task, void *work);

// The volume of the phenomenon's sound by the player's distance, in steps of 5 tiles
static const u8 ENCOUNT_EFFECT_VOLUMES[4] = { 127, 90, 60, 40 };

// The effect of each kind of phenomenon; the first three are variants of the first effect
static const u8 ENCOUNT_EFFECT_INDICES[8] = { 0, 0, 0, 1, 2, 3, 4, 5 };

static const FieldEffectTaskVTable ENCOUNT_TASK_VTABLE = {
    sizeof(EncountTask),
    func_ov036_021a54b8,
    func_ov036_021a54f8,
    func_ov036_021a54fc,
    func_ov036_021a55b8,
};

static EncountEffectSpec ENCOUNT_EFFECT_SPECS[6] = {
    { 1, 3, 4, TRUE, FALSE, 1749, 60, { 141, 0, 0, 0 } },
    { 1, 1, 4, TRUE, FALSE, 1749, 68, { 149, 0, 0, 0 } },
    { 2, 1, 1, FALSE, FALSE, 1752, 76, { 154, 123, 0, 0 } },
    { 1, 1, 2, FALSE, TRUE, 1750, 72, { 119, 0, 0, 0 } },
    { 1, 1, 1, FALSE, FALSE, 1750, 74, { 121, 0, 0, 0 } },
    { 3, 1, 1, FALSE, FALSE, 1751, 75, { 153, 139, 122, 0 } },
};

void *func_ov036_021a50a4(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 0);
}

void *func_ov036_021a50b0(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 1);
}

void *func_ov036_021a50bc(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 2);
}

void *func_ov036_021a50c8(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 3);
}

void *func_ov036_021a50d4(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 4);
}

void *func_ov036_021a50e0(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5100(effects, heapId, 5);
}

void func_ov036_021a50ec(FieldEffects *effects, void *data) {
    func_ov036_021a5368(data);
    GFL_HeapFree(data);
}

static void *func_ov036_021a5100(FieldEffects *effects, HeapID heapId, u32 kind) {
    FieldEffectEncount *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectEncount), TRUE, "fldeff_encount.c", 286);

    effect->effects = effects;
    func_ov036_021a5130(effect, kind);
    return effect;
}

static void func_ov036_021a5130(FieldEffectEncount *effect, u32 kind) {
    ArcTool *arc;

    effect->spec = ENCOUNT_EFFECT_SPECS[kind];
    arc = FieldEffects_GetArc(effect->effects);
    if (kind == 0) {
        func_ov036_021a5228(effect, &effect->spec, arc);
    } else {
        func_ov036_021a52b8(effect, &effect->spec, arc);
    }
}

static void func_ov036_021a5178(EncountEffectObj *obj, FieldEffectEncount *effect, u8 variant) {
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

static void func_ov036_021a51f0(EncountEffectObj *obj, FieldEffectEncount *effect) {
    int i;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DAnmFree(obj->anms[i]);
    }
    GFL_G3DActorFree(obj->actor);
    GFL_G3DMdlFree(obj->mdl);
}

// The grass of the first kind: this season's, the next variant's, and the first
static void func_ov036_021a5228(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc) {
    u32 season = FieldEffects_GetSeason(effect->effects);

    effect->mdlRes[0] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId + season);
    GFL_G3DResUploadTexData(effect->mdlRes[0]);
    effect->anmRes[0] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0] + season);
    func_ov036_021a5178(&effect->objs[0], effect, 0);
    season += spec->variantStep;
    effect->mdlRes[1] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId + season);
    GFL_G3DResUploadTexData(effect->mdlRes[1]);
    effect->anmRes[1] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0] + season);
    func_ov036_021a5178(&effect->objs[1], effect, 1);
    effect->mdlRes[2] = GFL_G3DSysReadArcToolResource(arc, spec->mdlId);
    GFL_G3DResUploadTexData(effect->mdlRes[2]);
    effect->anmRes[2] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[0]);
    func_ov036_021a5178(&effect->objs[2], effect, 2);
}

static void func_ov036_021a52b8(FieldEffectEncount *effect, EncountEffectSpec *spec, ArcTool *arc) {
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
        func_ov036_021a5178(&effect->objs[i], effect, i);
    }
}

static void func_ov036_021a5368(FieldEffectEncount *effect) {
    int i;

    for (i = 0; i < effect->spec.variantCount; i++) {
        func_ov036_021a51f0(&effect->objs[i], effect);
    }
    for (i = 0; i < effect->spec.anmCount * effect->spec.variantCount; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    for (i = 0; i < effect->spec.variantCount; i++) {
        GFL_G3DResFreeTexData(effect->mdlRes[i]);
        GFL_G3DResFree(effect->mdlRes[i]);
    }
}

// Start the phenomenon of the given kind on the tile, unless the controls are of another type
FieldEffectTask *func_ov036_021a53f8(EncountSystem *system, FieldEffects *effects, u16 x, u16 z, fx32 height,
                                     u32 kind) {
    Field *field = FieldEffects_GetField(effects);
    u32 effectIndex;
    FieldEffectEncount *effect;
    EncountTaskParams params;

    if (Field_GetResolvedControllerTypeID(field) != 0) {
        return NULL;
    }
    effectIndex = ENCOUNT_EFFECT_INDICES[kind];
    effect = FieldEffects_GetHandleData(effects, effectIndex + 19);
    if (effect == NULL) {
        return NULL;
    }
    sys_memset(&params, 0, sizeof(EncountTaskParams));
    params.system = system;
    params.effect = effect;
    params.actorSys = Field_GetActorSystem(field);
    params.x = x;
    params.z = z;
    params.effectIndex = effectIndex;
    if (effectIndex == 0) {
        params.variant = kind;
    } else {
        params.variant = 0;
    }
    params.pos.x = (x << 16) + FX32_CONST(8);
    params.pos.z = (z << 16) + FX32_CONST(8);
    params.pos.y = height;
    return FieldEffects_TCBCreate(effects, &ENCOUNT_TASK_VTABLE, NULL, TRUE, &params, 0);
}

void func_ov036_021a5498(FieldEffectTask *task, BOOL paused) {
    if (task != NULL) {
        EncountTask *encountTask = func_ov036_021a3afc(task);

        encountTask->paused = paused;
    }
}

void func_ov036_021a54a8(FieldEffectTask *task, BOOL hidden) {
    if (task != NULL) {
        EncountTask *encountTask = func_ov036_021a3afc(task);

        encountTask->hidden = hidden;
    }
}

static void func_ov036_021a54b8(FieldEffectTask *task, void *work) {
    EncountTask *encountTask = work;
    EncountTaskParams *params = func_ov036_021a3ac8(task);

    encountTask->system = params->system;
    encountTask->params = *params;
    func_ov036_021a3ae8(task, &params->pos);
    if (func_ov036_021a3abc(task) == FALSE) {
        encountTask->unk00 = 1;
    }
    func_ov036_021a3a94(task);
}

static void func_ov036_021a54f8(FieldEffectTask *task, void *work) {
}

static void func_ov036_021a54fc(FieldEffectTask *task, void *work) {
    int i;
    EncountTask *encountTask = work;
    FieldEffectEncount *effect = encountTask->params.effect;

    if (Field_CheckMapLoadFinished(FieldEffects_GetField(effect->effects)) && !encountTask->paused) {
        func_ov036_021a5550(encountTask, effect);
        for (i = 0; i < effect->spec.anmCount; i++) {
            GFL_G3DActorStepAnmFrameLoop(effect->objs[encountTask->params.variant].actor, i, FX32_ONE);
        }
    }
}

// Play the sound at the start of each loop, quieter the further away the player is
static void func_ov036_021a5550(EncountTask *encountTask, FieldEffectEncount *effect) {
    u16 distance = 0;
    fx32 frame;

    GFL_G3DActorGetAnmFrame(effect->objs[encountTask->params.variant].actor, 0, &frame);
    if (frame == 0 && func_ov036_021a22f4(encountTask->system, &distance)) {
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

static void func_ov036_021a55b8(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    EncountTask *encountTask = work;
    FieldEffectEncount *effect = encountTask->params.effect;

    if (!encountTask->hidden) {
        MTX_Identity33_(&srt.rotation);
        func_ov036_021a3ad4(task, &srt.translation);
        GFL_G3DSysDrawObjBBoxCull(effect->objs[encountTask->params.variant].actor, &srt);
    }
}
