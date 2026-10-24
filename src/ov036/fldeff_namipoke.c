// The Pokémon a field actor surfs on and its wake, and the splashes around it: field effects 3 and 8. The name is the
// ROM's own, from GFL_HeapAllocate's file argument ("namipoke" is the surfing Pokémon). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// How far behind the Pokémon its wake trails at most, and the wake's scale, in thousandths of its full size, below
// which it hides
#define WAKE_MAX_DISTANCE FX32_CONST(64)
#define WAKE_MIN_SCALE 40

// The direction the wake has while it is hidden
#define WAKE_DIR_NONE 9

#define SPLASH_KIND_COUNT 5

// Field effect 3: the Pokémon and its wake, which every task shares
typedef struct {
    FieldEffects *effects;
    u16 pokeResGroup;
    u16 wakeResGroup;
    u16 pokeObj;
    u16 wakeObj;
} FieldEffectNamipoke;

// What the Pokémon's task starts from
typedef struct {
    FieldEffectNamipoke *namipoke;
    FieldActor *actor;
    u16 uid;
    u16 zoneId;
    u16 dir;
    VecFx32 pos;
} NamipokeTaskParams;

typedef struct {
    u16 hidden;
    u16 dir;
    VecFx32 pos;
    // Where the actor was on the last update
    VecFx32 actorPos;
    u16 objIdx;
    VecFx32 scale;
    VecFx32 offset;
} NamipokeWake;

typedef struct {
    u32 unk0;
    u32 unk4;
    u8 unk8;
    u8 mode;
    u8 hideWake;
    u16 dir;
    // The height the Pokémon bobs to, and how fast
    fx32 bob;
    fx32 bobSpeed;
    u16 objIdx;
    NamipokeTaskParams params;
    NamipokeWake wake;
} NamipokeTask;

// Field effect 8: the splashes, a resource group of each kind
typedef struct {
    FieldEffects *effects;
    u16 resGroup0;
    u16 resGroup1;
    u16 resGroup3;
    u16 resGroup2;
    u16 resGroup4;
} FieldEffectNamipokeSplash;

// What a splash's task starts from
typedef struct {
    BOOL endWithAnim;
    FieldEffectNamipokeSplash *splash;
    u32 kind;
    // The task of the Pokémon the splash follows, or NULL
    FieldEffectTask *parent;
} SplashTaskParams;

typedef struct {
    VecFx32 offset;
    BOOL animEnded;
    u16 objIdx;
    SplashTaskParams params;
} SplashTask;

static void FldeffNamipoke_LoadResources(FieldEffectNamipoke *namipoke);
static void FldeffNamipoke_FreeResources(FieldEffectNamipoke *namipoke);
static fx32 FldeffNamipoke_GetWakeScale(fx32 distance);
static fx32 FldeffNamipoke_GetWakeLength(const VecFx32 *actorPos, const VecFx32 *wakePos, u16 dir);
static void FldeffNamipoke_Init(FieldEffectTask *task, void *work);
static void func_ov036_021a45f8(FieldEffectTask *task, void *work);
static void FldeffNamipoke_PlaceObjects(FieldEffectTask *task, NamipokeTask *namiTask);
static void FldeffNamipoke_UpdateGrid(FieldEffectTask *task, void *work);
static void FldeffNamipoke_DrawGrid(FieldEffectTask *task, void *work);
static void FldeffNamipoke_UpdateRail(FieldEffectTask *task, void *work);
static void FldeffNamipoke_DrawRail(FieldEffectTask *task, void *work);
static void FldeffNamipokeSplash_LoadResources(FieldEffectNamipokeSplash *splash);
static void FldeffNamipokeSplash_FreeResources(FieldEffectNamipokeSplash *splash);
static void FldeffNamipokeSplash_Init(FieldEffectTask *task, void *work);
static void FldeffNamipokeSplash_Exit(FieldEffectTask *task, void *work);
static void FldeffNamipokeSplash_Update(FieldEffectTask *task, void *work);
static void FldeffNamipokeSplash_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable SPLASH_TASK_VTABLE = {
    sizeof(SplashTask),
    FldeffNamipokeSplash_Init,
    FldeffNamipokeSplash_Exit,
    FldeffNamipokeSplash_Update,
    FldeffNamipokeSplash_Draw,
};

// clang-format on

// The Pokémon of an actor on the grid
// clang-format off
static const FieldEffectTaskVTable NAMIPOKE_TASK_VTABLE = {
    sizeof(NamipokeTask),
    FldeffNamipoke_Init,
    func_ov036_021a45f8,
    FldeffNamipoke_UpdateGrid,
    FldeffNamipoke_DrawGrid,
};

// clang-format on

// The Pokémon of an actor on a rail
// clang-format off
static const FieldEffectTaskVTable NAMIPOKE_RAIL_TASK_VTABLE = {
    sizeof(NamipokeTask),
    FldeffNamipoke_Init,
    func_ov036_021a45f8,
    FldeffNamipoke_UpdateRail,
    FldeffNamipoke_DrawRail,
};

// clang-format on

void *FldeffNamipoke_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectNamipoke *namipoke =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectNamipoke), TRUE, "fldeff_namipoke.c", 172);

    namipoke->effects = effects;
    FldeffNamipoke_LoadResources(namipoke);
    return namipoke;
}

void func_ov036_021a43a8(FieldEffects *effects, void *data) {
    FldeffNamipoke_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffNamipoke_LoadResources(FieldEffectNamipoke *namipoke) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(namipoke->effects);
    ArcTool *arc = FieldEffects_GetArc(namipoke->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 49);
    namipoke->pokeResGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
    namipoke->pokeObj = FieldG3DObjSystem_AddObj(objSys, namipoke->pokeResGroup, 0, NULL);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, namipoke->pokeObj, TRUE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 54);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 111);
    FieldG3DObjResRequest_AddAnm(&request, 140);
    namipoke->wakeResGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
    namipoke->wakeObj = FieldG3DObjSystem_AddObj(objSys, namipoke->wakeResGroup, 0, NULL);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, namipoke->wakeObj, TRUE);
}

static void FldeffNamipoke_FreeResources(FieldEffectNamipoke *namipoke) {
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(namipoke->effects);

    FieldG3DObjSystem_FreeObj(objSys, namipoke->pokeObj);
    FieldG3DObjSystem_FreeObj(objSys, namipoke->wakeObj);
    FieldG3DObjSystem_FreeResGroup(objSys, namipoke->pokeResGroup);
    FieldG3DObjSystem_FreeResGroup(objSys, namipoke->wakeResGroup);
}

FieldEffectTask *FldeffNamipoke_Start(FieldEffects *effects, u16 dir, const VecFx32 *pos, FieldActor *actor, u32 mode) {
    NamipokeTaskParams params;

    params.namipoke = FieldEffects_GetHandleData(effects, 3);
    params.actor = actor;
    params.uid = GetActorUID(actor);
    params.zoneId = GetActorZoneID(actor);
    params.dir = dir;
    params.pos = *pos;
    if (CheckActorFlag(actor, 0x2000)) {
        return FieldEffects_TCBCreate(effects, &NAMIPOKE_RAIL_TASK_VTABLE, &params.pos, mode, &params, 0);
    }
    return FieldEffects_TCBCreate(effects, &NAMIPOKE_TASK_VTABLE, &params.pos, mode, &params, 0);
}

void FldeffNamipoke_SetFollowMode(FieldEffectTask *task, u8 mode) {
    if (task != NULL) {
        NamipokeTask *namiTask = FieldEffects_GetObjectWork(task);

        namiTask->mode = mode;
    }
}

void FldeffNamipoke_SetWakeEnabled(FieldEffectTask *task, BOOL showWake) {
    if (task != NULL) {
        NamipokeTask *namiTask = FieldEffects_GetObjectWork(task);

        if (showWake == TRUE) {
            namiTask->hideWake = FALSE;
        } else {
            namiTask->hideWake = TRUE;
        }
    }
}

// The wake's scale for its distance behind the Pokémon
static fx32 FldeffNamipoke_GetWakeScale(fx32 distance) {
    if (distance > WAKE_MAX_DISTANCE) {
        distance = WAKE_MAX_DISTANCE;
    }
    return distance / (WAKE_MAX_DISTANCE / 100) * WAKE_MIN_SCALE;
}

// How far the wake is behind the actor, along the direction it faces
static fx32 FldeffNamipoke_GetWakeLength(const VecFx32 *actorPos, const VecFx32 *wakePos, u16 dir) {
    fx32 distance = 0;

    switch (dir) {
    case DIR_UP:
        distance = wakePos->z - actorPos->z;
        break;
    case DIR_DOWN:
        distance = actorPos->z - wakePos->z;
        break;
    case DIR_LEFT:
        distance = wakePos->x - actorPos->x;
        break;
    case DIR_RIGHT:
        distance = actorPos->x - wakePos->x;
        break;
    }
    return distance;
}

static void FldeffNamipoke_Init(FieldEffectTask *task, void *work) {
    NamipokeTask *namiTask = work;
    NamipokeTaskParams *params = FieldEffects_GetObjectParam(task);
    NamipokeWake *wake;

    namiTask->params = *params;
    FieldEffects_SetObjectPos(task, &params->pos);
    namiTask->dir = params->dir;
    // The game fetches the 3D object system here and doesn't use it
    FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects);
    namiTask->objIdx = namiTask->params.namipoke->pokeObj;
    namiTask->mode = FieldEffects_GetObjectMode(task);
    namiTask->bob = FX32_ONE;
    namiTask->bobSpeed = FX32_CONST(0.25);
    wake = &namiTask->wake;
    wake->objIdx = namiTask->params.namipoke->wakeObj;
    wake->hidden = TRUE;
    wake->dir = WAKE_DIR_NONE;
    wake->scale.x = FX32_ONE;
    wake->scale.y = FX32_ONE;
    wake->scale.z = FX32_ONE;
    FieldEffects_UpdateObject(task);
}

static void func_ov036_021a45f8(FieldEffectTask *task, void *work) {
}

// Turn the Pokémon and its wake on the grid to the direction it faces, and put them where the task is
static void FldeffNamipoke_PlaceObjects(FieldEffectTask *task, NamipokeTask *namiTask) {
    u16 pokeRotations[4][3] = {
        { 0, 0x8000, 0 },
        { 0, 0, 0 },
        { 0, 0xc000, 0 },
        { 0, 0x4000, 0 },
    };
    u16 *rotation = pokeRotations[namiTask->dir];
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects);
    SRTMatrix *transform = FieldG3DObjSystem_GetObjTransform(objSys, namiTask->objIdx);

    MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &transform->rotation);
    FieldEffects_GetObjectPos(task, &transform->translation);
    if (namiTask->wake.hidden == FALSE) {
        NamipokeWake *wake = &namiTask->wake;
        VecFx32 wakeOffsets[4] = {
            { 0, FX32_CONST(-4), FX32_CONST(14) },
            { 0, FX32_CONST(-4), FX32_CONST(-14) },
            { FX32_CONST(14), FX32_CONST(-4), 0 },
            { FX32_CONST(-14), FX32_CONST(-4), 0 },
        };
        u16 wakeRotations[4][3] = {
            { 0, 0, 0 },
            { 0, 0x8000, 0 },
            { 0, 0x4000, 0 },
            { 0, 0xc000, 0 },
        };
        VecFx32 *offset = &wakeOffsets[namiTask->dir];

        rotation = wakeRotations[namiTask->dir];
        transform = FieldG3DObjSystem_GetObjTransform(objSys, wake->objIdx);
        MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &transform->rotation);
        FieldEffects_GetObjectPos(task, &transform->translation);
        transform->translation.x += wake->offset.x + offset->x;
        transform->translation.y += wake->offset.y + offset->y;
        transform->translation.z += wake->offset.z + offset->z;
        transform->scale = wake->scale;
        FieldG3DObjSystem_SetObjectHidden(objSys, wake->objIdx, FALSE);
    } else {
        FieldG3DObjSystem_SetObjectHidden(objSys, namiTask->wake.objIdx, TRUE);
    }
}

// On the grid: bob under the actor, and trail the wake behind it
static void FldeffNamipoke_UpdateGrid(FieldEffectTask *task, void *work) {
    VecFx32 offset = { 0, 0, 0 };
    VecFx32 pos;
    VecFx32 actorPos;
    NamipokeTask *namiTask = work;
    u16 objCode;

    if (namiTask->mode == 0) {
        FldeffNamipoke_PlaceObjects(task, namiTask);
        return;
    }
    namiTask->dir = GetActorFaceDir(namiTask->params.actor);
    if (namiTask->mode == 1) {
        namiTask->bob += namiTask->bobSpeed;
        if (namiTask->bob >= FX32_CONST(4)) {
            namiTask->bob = FX32_CONST(4);
            namiTask->bobSpeed = -namiTask->bobSpeed;
        } else if (namiTask->bob <= FX32_ONE) {
            namiTask->bob = FX32_ONE;
            namiTask->bobSpeed = -namiTask->bobSpeed;
        }
        offset.x = 0;
        offset.y = namiTask->bob + FX32_ONE;
        offset.z = FX32_CONST(5);
        // Object codes 160 and 161 sit higher on the Pokémon
        objCode = FldAct_GetObjCode(namiTask->params.actor);
        if (objCode == 160 || objCode == 161) {
            offset.y = namiTask->bob + FX32_CONST(8);
        }
        FldAct_SetOuterDrawOffset(namiTask->params.actor, &offset);
    } else if (namiTask->mode == 2) {
        FldAct_GetOuterDrawOffset(namiTask->params.actor, &offset);
    }
    CopyActorPosAllAdd(namiTask->params.actor, &pos);
    pos.x -= offset.x;
    pos.y -= offset.y;
    pos.z -= offset.z;
    if (namiTask->mode != 2) {
        pos.y += namiTask->bob - FX32_ONE;
    }
    FieldEffects_SetObjectPos(task, &pos);
    if (namiTask->mode == 2 || namiTask->hideWake == TRUE) {
        NamipokeWake *wake = &namiTask->wake;

        wake->dir = WAKE_DIR_NONE;
        if (namiTask->hideWake == TRUE) {
            wake->hidden = TRUE;
        }
    } else {
        NamipokeWake *wake = &namiTask->wake;
        fx32 distance;
        fx32 scale;

        FieldG3DObjSystem_StepObjAnmLoop(FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects), wake->objIdx,
                                         FX32_ONE);
        CopyActorWPos(namiTask->params.actor, &actorPos);
        if (namiTask->dir != wake->dir) {
            wake->dir = namiTask->dir;
            wake->pos = actorPos;
        } else if (actorPos.x == wake->actorPos.x && actorPos.y == wake->actorPos.y && actorPos.z == wake->actorPos.z) {
            // The actor stopped: the wake catches up
            switch (wake->dir) {
            case DIR_UP:
                wake->pos.z -= FX32_CONST(4);
                if (wake->pos.z < actorPos.z) {
                    wake->pos.z = actorPos.z;
                }
                break;
            case DIR_DOWN:
                wake->pos.z += FX32_CONST(4);
                if (wake->pos.z > actorPos.z) {
                    wake->pos.z = actorPos.z;
                }
                break;
            case DIR_LEFT:
                wake->pos.x -= FX32_CONST(4);
                if (wake->pos.x < actorPos.x) {
                    wake->pos.x = actorPos.x;
                }
                break;
            case DIR_RIGHT:
                wake->pos.x += FX32_CONST(4);
                if (wake->pos.x > actorPos.x) {
                    wake->pos.x = actorPos.x;
                }
                break;
            }
        }
        distance = FldeffNamipoke_GetWakeLength(&actorPos, &wake->pos, wake->dir);
        if (distance >= WAKE_MAX_DISTANCE) {
            switch (wake->dir) {
            case DIR_UP:
                wake->pos.z = actorPos.z + WAKE_MAX_DISTANCE;
                break;
            case DIR_DOWN:
                wake->pos.z = actorPos.z - WAKE_MAX_DISTANCE;
                break;
            case DIR_LEFT:
                wake->pos.x = actorPos.x + WAKE_MAX_DISTANCE;
                break;
            case DIR_RIGHT:
                wake->pos.x = actorPos.x - WAKE_MAX_DISTANCE;
                break;
            }
        }
        scale = FldeffNamipoke_GetWakeScale(distance);
        if (scale < WAKE_MIN_SCALE) {
            wake->hidden = TRUE;
        } else {
            wake->hidden = FALSE;
            wake->scale.x = scale;
            wake->scale.y = scale;
            wake->scale.z = scale;
        }
        wake->actorPos = actorPos;
    }
    FldeffNamipoke_PlaceObjects(task, namiTask);
}

static void FldeffNamipoke_DrawGrid(FieldEffectTask *task, void *work) {
    NamipokeTask *namiTask = work;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects);

    FieldG3DObjSystem_DrawObj(objSys, namiTask->objIdx);
    FieldG3DObjSystem_DrawObj(objSys, namiTask->wake.objIdx);
}

// On a rail: the same along the rail's directions
static void FldeffNamipoke_UpdateRail(FieldEffectTask *task, void *work) {
    VecFx32 offset = { 0, 0, 0 };
    VecFx32 pos;
    VecFx32 actorPos;
    VecFx16 up;
    VecFx16 forward;
    NamipokeTask *namiTask = work;

    if (namiTask->mode == 0) {
        return;
    }
    namiTask->dir = GetActorFaceDir(namiTask->params.actor);
    RailActor_GetDrawOffsetByDir(namiTask->params.actor, DIR_UP, &up);
    RailActor_GetDrawOffsetByDir(namiTask->params.actor, namiTask->dir, &forward);
    up.y = 0;
    VEC_Fx16Normalize(&up, &up);
    forward.y = 0;
    VEC_Fx16Normalize(&forward, &forward);
    if (namiTask->mode == 1) {
        namiTask->bob += namiTask->bobSpeed;
        if (namiTask->bob >= FX32_CONST(4)) {
            namiTask->bob = FX32_CONST(4);
            namiTask->bobSpeed = -namiTask->bobSpeed;
        } else if (namiTask->bob <= FX32_ONE) {
            namiTask->bob = FX32_ONE;
            namiTask->bobSpeed = -namiTask->bobSpeed;
        }
        offset.x = FX_Mul(-up.x, FX32_CONST(7));
        offset.y = namiTask->bob + FX32_CONST(5);
        offset.z = FX_Mul(-up.z, FX32_CONST(7));
        FldAct_SetOuterDrawOffset(namiTask->params.actor, &offset);
    }
    CopyActorPosAllAdd(namiTask->params.actor, &pos);
    pos.x -= offset.x;
    pos.y -= offset.y;
    pos.z -= offset.z;
    pos.y += namiTask->bob - FX32_ONE;
    FieldEffects_SetObjectPos(task, &pos);
    if (namiTask->mode == 2 || namiTask->hideWake == TRUE) {
        NamipokeWake *wake = &namiTask->wake;

        wake->dir = WAKE_DIR_NONE;
        if (namiTask->hideWake == TRUE) {
            wake->hidden = TRUE;
        }
    } else {
        NamipokeWake *wake = &namiTask->wake;
        fx32 distance;
        fx32 scale;

        FieldG3DObjSystem_StepObjAnmLoop(FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects), wake->objIdx,
                                         FX32_ONE);
        CopyActorWPos(namiTask->params.actor, &actorPos);
        if (namiTask->dir != wake->dir) {
            wake->dir = namiTask->dir;
            wake->pos = actorPos;
        } else if (actorPos.x == wake->actorPos.x && actorPos.y == wake->actorPos.y && actorPos.z == wake->actorPos.z) {
            // The actor stopped: the wake catches up
            wake->pos.x += FX_Mul(forward.x, FX32_CONST(4));
            wake->pos.z += FX_Mul(forward.z, FX32_CONST(4));
            wake->pos.y = actorPos.y;
            if (forward.x > 0) {
                if (wake->pos.x > actorPos.x) {
                    wake->pos.x = actorPos.x;
                }
            } else if (forward.x < 0) {
                if (wake->pos.x < actorPos.x) {
                    wake->pos.x = actorPos.x;
                }
            }
            if (forward.z > 0) {
                if (wake->pos.z > actorPos.z) {
                    wake->pos.z = actorPos.z;
                }
            } else if (forward.z < 0) {
                if (wake->pos.z < actorPos.z) {
                    wake->pos.z = actorPos.z;
                }
            }
        }
        distance = VEC_Distance(&actorPos, &wake->pos);
        if (distance >= WAKE_MAX_DISTANCE) {
            wake->pos.x = actorPos.x - FX_Mul(forward.x, WAKE_MAX_DISTANCE);
            wake->pos.z = actorPos.z - FX_Mul(forward.z, WAKE_MAX_DISTANCE);
        }
        scale = FldeffNamipoke_GetWakeScale(distance);
        if (scale <= WAKE_MIN_SCALE) {
            wake->hidden = TRUE;
        } else {
            wake->hidden = FALSE;
            wake->scale.x = scale;
            wake->scale.y = scale;
            wake->scale.z = scale;
        }
        wake->actorPos = actorPos;
    }
}

static void FldeffNamipoke_DrawRail(FieldEffectTask *task, void *work) {
    VecFx32 forward;
    VecFx32 back;
    MtxFx33 pokeRotation;
    MtxFx33 wakeRotation;
    VecFx16 dir;
    FieldG3DObjSystem *objSys;
    SRTMatrix *transform;
    NamipokeTask *namiTask = work;
    NamipokeWake *wake = &namiTask->wake;

    RailActor_GetDrawOffsetByDir(namiTask->params.actor, namiTask->dir, &dir);
    forward.x = dir.x;
    forward.y = 0;
    forward.z = dir.z;
    back.x = -dir.x;
    back.y = 0;
    back.z = -dir.z;
    MAT3_RotationDir(&forward, &pokeRotation);
    MAT3_RotationDir(&back, &wakeRotation);
    objSys = FieldEffects_GetFieldMapData(namiTask->params.namipoke->effects);
    transform = FieldG3DObjSystem_GetObjTransform(objSys, namiTask->objIdx);
    transform->rotation = pokeRotation;
    FieldEffects_GetObjectPos(task, &transform->translation);
    if (namiTask->wake.hidden == FALSE) {
        FieldG3DObjSystem_SetObjectHidden(objSys, wake->objIdx, FALSE);
        transform = FieldG3DObjSystem_GetObjTransform(objSys, wake->objIdx);
        transform->rotation = wakeRotation;
        FieldEffects_GetObjectPos(task, &transform->translation);
        transform->translation.x += wake->offset.x - FX_Mul(forward.x, FX32_CONST(14));
        transform->translation.y += wake->offset.y - FX32_CONST(4) - FX_Mul(forward.y, FX32_CONST(14));
        transform->translation.z += wake->offset.z - FX_Mul(forward.z, FX32_CONST(14));
        transform->scale = wake->scale;
    } else {
        FieldG3DObjSystem_SetObjectHidden(objSys, wake->objIdx, TRUE);
    }
    FieldG3DObjSystem_DrawObj(objSys, namiTask->objIdx);
    FieldG3DObjSystem_DrawObj(objSys, wake->objIdx);
}

void *FldeffNamipokeSplash_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectNamipokeSplash *splash =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectNamipokeSplash), TRUE, "fldeff_namipoke.c", 975);

    splash->effects = effects;
    FldeffNamipokeSplash_LoadResources(splash);
    return splash;
}

void func_ov036_021a4d48(FieldEffects *effects, void *data) {
    FldeffNamipokeSplash_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffNamipokeSplash_LoadResources(FieldEffectNamipokeSplash *splash) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splash->effects);
    ArcTool *arc = FieldEffects_GetArc(splash->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 55);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 135);
    FieldG3DObjResRequest_AddAnm(&request, 112);
    splash->resGroup0 = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 58);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 115);
    FieldG3DObjResRequest_AddAnm(&request, 103);
    splash->resGroup1 = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 57);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 114);
    FieldG3DObjResRequest_AddAnm(&request, 102);
    splash->resGroup2 = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 59);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 116);
    FieldG3DObjResRequest_AddAnm(&request, 104);
    splash->resGroup3 = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 56);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 113);
    FieldG3DObjResRequest_AddAnm(&request, 101);
    splash->resGroup4 = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
}

static void FldeffNamipokeSplash_FreeResources(FieldEffectNamipokeSplash *splash) {
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splash->effects);

    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup0);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup1);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup2);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup3);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup4);
}

FieldEffectTask *FldeffNamipokeSplash_StartOnEffect(FieldEffects *effects, u32 kind, FieldEffectTask *parent) {
    SplashTaskParams params;

    params.splash = FieldEffects_GetHandleData(effects, 8);
    params.kind = kind;
    params.parent = parent;
    params.endWithAnim = FALSE;
    return FieldEffects_TCBCreate(effects, &SPLASH_TASK_VTABLE, NULL, 0, &params, 1);
}

FieldEffectTask *FldeffNamipokeSplash_StartAtPos(FieldEffects *effects, u32 kind, const VecFx32 *pos) {
    SplashTaskParams params;

    params.splash = FieldEffects_GetHandleData(effects, 8);
    params.kind = kind;
    params.parent = NULL;
    params.endWithAnim = TRUE;
    return FieldEffects_TCBCreate(effects, &SPLASH_TASK_VTABLE, pos, 0, &params, 1);
}

BOOL FldeffNamipokeSplash_IsFinished(FieldEffectTask *task) {
    if (task != NULL) {
        SplashTask *splashTask = FieldEffects_GetObjectWork(task);

        return splashTask->animEnded;
    }
    return TRUE;
}

static void FldeffNamipokeSplash_Init(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    SplashTaskParams *params = FieldEffects_GetObjectParam(task);
    FieldG3DObjSystem *objSys;

    splashTask->params = *params;
    objSys = FieldEffects_GetFieldMapData(splashTask->params.splash->effects);
    switch (splashTask->params.kind) {
    case 0:
        splashTask->objIdx = FieldG3DObjSystem_AddObj(objSys, splashTask->params.splash->resGroup0, 0, NULL);
        break;
    case 1:
        splashTask->objIdx = FieldG3DObjSystem_AddObj(objSys, splashTask->params.splash->resGroup1, 0, NULL);
        break;
    case 2:
        splashTask->objIdx = FieldG3DObjSystem_AddObj(objSys, splashTask->params.splash->resGroup2, 0, NULL);
        break;
    case 3:
        splashTask->objIdx = FieldG3DObjSystem_AddObj(objSys, splashTask->params.splash->resGroup3, 0, NULL);
        break;
    case 4:
        splashTask->objIdx = FieldG3DObjSystem_AddObj(objSys, splashTask->params.splash->resGroup4, 0, NULL);
        break;
    }
    if (splashTask->params.parent != NULL) {
        VecFx32 pos;
        VecFx32 offsets[SPLASH_KIND_COUNT] = {
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, FX32_CONST(-4), 0 }, { 0, 0, 0 }, { 0, 0, 0 },
        };

        splashTask->offset = offsets[splashTask->params.kind];
        FieldEffects_GetObjectPos(splashTask->params.parent, &pos);
        pos.x += splashTask->offset.x;
        pos.y += splashTask->offset.y;
        pos.z += splashTask->offset.z;
        FieldEffects_SetObjectPos(task, &pos);
    }
}

static void FldeffNamipokeSplash_Exit(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;

    FieldG3DObjSystem_FreeObj(FieldEffects_GetFieldMapData(splashTask->params.splash->effects), splashTask->objIdx);
}

// Play the animation, once for kind 0 and over and over for the others, and follow the Pokémon
static void FldeffNamipokeSplash_Update(FieldEffectTask *task, void *work) {
    VecFx32 pos;
    SplashTask *splashTask = work;
    FieldG3DObjSystem *objSys;

    if (splashTask->animEnded == TRUE && splashTask->params.endWithAnim == TRUE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    objSys = FieldEffects_GetFieldMapData(splashTask->params.splash->effects);
    if (splashTask->params.kind == 0) {
        if (splashTask->animEnded == FALSE && !FieldG3DObjSystem_StepObjAnm(objSys, splashTask->objIdx, FX32_ONE)) {
            splashTask->animEnded = TRUE;
        }
    } else {
        FieldG3DObjSystem_StepObjAnmLoop(objSys, splashTask->objIdx, FX32_ONE);
    }
    if (splashTask->params.parent != NULL) {
        FieldEffects_GetObjectPos(splashTask->params.parent, &pos);
        pos.x += splashTask->offset.x;
        pos.y += splashTask->offset.y;
        pos.z += splashTask->offset.z;
        FieldEffects_SetObjectPos(task, &pos);
    }
}

static void FldeffNamipokeSplash_Draw(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    SRTMatrix *transform = FieldG3DObjSystem_GetObjTransform(
        FieldEffects_GetFieldMapData(splashTask->params.splash->effects), splashTask->objIdx);

    FieldEffects_GetObjectPos(task, &transform->translation);
}
