// The Pokémon a field actor surfs on and its wake, and the splashes around it: FLDEFF_SURF_POKEMON and
// FLDEFF_SURF_POKEMON_SPLASH. The ROM names it fldeff_namipoke.c, by GFL_HeapAllocate's file argument ("namipoke" is
// the surfing Pokémon). Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/obj_codes.h"
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

#define SPLASH_KIND_COUNT 5

// FLDEFF_SURF_POKEMON: the Pokémon and its wake, which every task shares
typedef struct {
    FieldEffects *effects;
    u16 pokeResGroup;
    u16 wakeResGroup;
    u16 pokeObj;
    u16 wakeObj;
} FieldEffectSurfPokemon;

// What the Pokémon's task starts from
typedef struct {
    FieldEffectSurfPokemon *surfPokemon;
    FieldActor *actor;
    u16 uid;
    u16 zoneId;
    u16 dir;
    VecFx32 pos;
} SurfPokemonTaskParams;

typedef struct {
    u16 hidden;
    u16 dir;
    VecFx32 pos;
    // Where the actor was on the last update
    VecFx32 actorPos;
    u16 objIdx;
    VecFx32 scale;
    VecFx32 offset;
} SurfPokemonWake;

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
    SurfPokemonTaskParams params;
    SurfPokemonWake wake;
} SurfPokemonTask;

// FLDEFF_SURF_POKEMON_SPLASH: the splashes, a resource group of each kind
typedef struct {
    FieldEffects *effects;
    u16 resGroup0;
    u16 resGroup1;
    u16 resGroup3;
    u16 resGroup2;
    u16 resGroup4;
} FieldEffectSurfPokemonSplash;

// What a splash's task starts from
typedef struct {
    BOOL endWithAnim;
    FieldEffectSurfPokemonSplash *splash;
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

static void FldeffSurfPokemon_LoadResources(FieldEffectSurfPokemon *surfPokemon);
static void FldeffSurfPokemon_FreeResources(FieldEffectSurfPokemon *surfPokemon);
static fx32 FldeffSurfPokemon_GetWakeScale(fx32 distance);
static fx32 FldeffSurfPokemon_GetWakeLength(const VecFx32 *actorPos, const VecFx32 *wakePos, u16 dir);
static void FldeffSurfPokemon_Init(FieldEffectTask *task, void *work);
static void FldeffSurfPokemon_Exit(FieldEffectTask *task, void *work);
static void FldeffSurfPokemon_PlaceObjects(FieldEffectTask *task, SurfPokemonTask *surfTask);
static void FldeffSurfPokemon_UpdateGrid(FieldEffectTask *task, void *work);
static void FldeffSurfPokemon_DrawGrid(FieldEffectTask *task, void *work);
static void FldeffSurfPokemon_UpdateRail(FieldEffectTask *task, void *work);
static void FldeffSurfPokemon_DrawRail(FieldEffectTask *task, void *work);
static void FldeffSurfPokemonSplash_LoadResources(FieldEffectSurfPokemonSplash *splash);
static void FldeffSurfPokemonSplash_FreeResources(FieldEffectSurfPokemonSplash *splash);
static void FldeffSurfPokemonSplash_Init(FieldEffectTask *task, void *work);
static void FldeffSurfPokemonSplash_Exit(FieldEffectTask *task, void *work);
static void FldeffSurfPokemonSplash_Update(FieldEffectTask *task, void *work);
static void FldeffSurfPokemonSplash_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable SPLASH_TASK_VTABLE = {
    sizeof(SplashTask),
    FldeffSurfPokemonSplash_Init,
    FldeffSurfPokemonSplash_Exit,
    FldeffSurfPokemonSplash_Update,
    FldeffSurfPokemonSplash_Draw,
};

// clang-format on

// The Pokémon of an actor on the grid
// clang-format off
static const FieldEffectTaskVTable SURF_POKEMON_TASK_VTABLE = {
    sizeof(SurfPokemonTask),
    FldeffSurfPokemon_Init,
    FldeffSurfPokemon_Exit,
    FldeffSurfPokemon_UpdateGrid,
    FldeffSurfPokemon_DrawGrid,
};

// clang-format on

// The Pokémon of an actor on a rail
// clang-format off
static const FieldEffectTaskVTable SURF_POKEMON_RAIL_TASK_VTABLE = {
    sizeof(SurfPokemonTask),
    FldeffSurfPokemon_Init,
    FldeffSurfPokemon_Exit,
    FldeffSurfPokemon_UpdateRail,
    FldeffSurfPokemon_DrawRail,
};

// clang-format on

void *FldeffSurfPokemon_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectSurfPokemon *surfPokemon =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectSurfPokemon), TRUE, "fldeff_namipoke.c", 172);

    surfPokemon->effects = effects;
    FldeffSurfPokemon_LoadResources(surfPokemon);
    return surfPokemon;
}

void FldeffSurfPokemon_Delete(FieldEffects *effects, void *data) {
    FldeffSurfPokemon_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffSurfPokemon_LoadResources(FieldEffectSurfPokemon *surfPokemon) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(surfPokemon->effects);
    ArcTool *arc = FieldEffects_GetArc(surfPokemon->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 49);
    surfPokemon->pokeResGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
    surfPokemon->pokeObj = FieldG3DObjSystem_AddObj(objSys, surfPokemon->pokeResGroup, 0, NULL);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, surfPokemon->pokeObj, TRUE);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 54);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 111);
    FieldG3DObjResRequest_AddAnm(&request, 140);
    surfPokemon->wakeResGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
    surfPokemon->wakeObj = FieldG3DObjSystem_AddObj(objSys, surfPokemon->wakeResGroup, 0, NULL);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, surfPokemon->wakeObj, TRUE);
}

static void FldeffSurfPokemon_FreeResources(FieldEffectSurfPokemon *surfPokemon) {
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(surfPokemon->effects);

    FieldG3DObjSystem_FreeObj(objSys, surfPokemon->pokeObj);
    FieldG3DObjSystem_FreeObj(objSys, surfPokemon->wakeObj);
    FieldG3DObjSystem_FreeResGroup(objSys, surfPokemon->pokeResGroup);
    FieldG3DObjSystem_FreeResGroup(objSys, surfPokemon->wakeResGroup);
}

FieldEffectTask *FldeffSurfPokemon_Start(FieldEffects *effects, u16 dir, const VecFx32 *pos, FieldActor *actor,
                                         u32 mode) {
    SurfPokemonTaskParams params;

    params.surfPokemon = FieldEffects_GetHandleData(effects, FLDEFF_SURF_POKEMON);
    params.actor = actor;
    params.uid = GetActorUID(actor);
    params.zoneId = GetActorZoneID(actor);
    params.dir = dir;
    params.pos = *pos;
    if (CheckActorFlag(actor, ACTOR_FLAG_RAIL)) {
        return FieldEffects_TCBCreate(effects, &SURF_POKEMON_RAIL_TASK_VTABLE, &params.pos, mode, &params, 0);
    }
    return FieldEffects_TCBCreate(effects, &SURF_POKEMON_TASK_VTABLE, &params.pos, mode, &params, 0);
}

void FldeffSurfPokemon_SetFollowMode(FieldEffectTask *task, u8 mode) {
    if (task != NULL) {
        SurfPokemonTask *surfTask = FieldEffects_GetObjectWork(task);

        surfTask->mode = mode;
    }
}

void FldeffSurfPokemon_SetWakeEnabled(FieldEffectTask *task, BOOL showWake) {
    if (task != NULL) {
        SurfPokemonTask *surfTask = FieldEffects_GetObjectWork(task);

        if (showWake == TRUE) {
            surfTask->hideWake = FALSE;
        } else {
            surfTask->hideWake = TRUE;
        }
    }
}

// The wake's scale for its distance behind the Pokémon
static fx32 FldeffSurfPokemon_GetWakeScale(fx32 distance) {
    if (distance > WAKE_MAX_DISTANCE) {
        distance = WAKE_MAX_DISTANCE;
    }
    return distance / (WAKE_MAX_DISTANCE / 100) * WAKE_MIN_SCALE;
}

// How far the wake is behind the actor, along the direction it faces
static fx32 FldeffSurfPokemon_GetWakeLength(const VecFx32 *actorPos, const VecFx32 *wakePos, u16 dir) {
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

static void FldeffSurfPokemon_Init(FieldEffectTask *task, void *work) {
    SurfPokemonTask *surfTask = work;
    SurfPokemonTaskParams *params = FieldEffects_GetObjectParam(task);
    SurfPokemonWake *wake;

    surfTask->params = *params;
    FieldEffects_SetObjectPos(task, &params->pos);
    surfTask->dir = params->dir;
    // The game fetches the 3D object system here and doesn't use it
    FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects);
    surfTask->objIdx = surfTask->params.surfPokemon->pokeObj;
    surfTask->mode = FieldEffects_GetObjectMode(task);
    surfTask->bob = FX32_ONE;
    surfTask->bobSpeed = FX32_CONST(0.25);
    wake = &surfTask->wake;
    wake->objIdx = surfTask->params.surfPokemon->wakeObj;
    wake->hidden = TRUE;
    wake->dir = DIR_NONE;
    wake->scale.x = FX32_ONE;
    wake->scale.y = FX32_ONE;
    wake->scale.z = FX32_ONE;
    FieldEffects_UpdateObject(task);
}

static void FldeffSurfPokemon_Exit(FieldEffectTask *task, void *work) {
}

// Turn the Pokémon and its wake on the grid to the direction it faces, and put them where the task is
static void FldeffSurfPokemon_PlaceObjects(FieldEffectTask *task, SurfPokemonTask *surfTask) {
    u16 pokeRotations[4][3] = {
        { 0, 0x8000, 0 },
        { 0, 0, 0 },
        { 0, 0xc000, 0 },
        { 0, 0x4000, 0 },
    };
    u16 *rotation = pokeRotations[surfTask->dir];
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects);
    SRTMatrix *transform = FieldG3DObjSystem_GetObjTransform(objSys, surfTask->objIdx);

    MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &transform->rotation);
    FieldEffects_GetObjectPos(task, &transform->translation);
    if (surfTask->wake.hidden == FALSE) {
        SurfPokemonWake *wake = &surfTask->wake;
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
        VecFx32 *offset = &wakeOffsets[surfTask->dir];

        rotation = wakeRotations[surfTask->dir];
        transform = FieldG3DObjSystem_GetObjTransform(objSys, wake->objIdx);
        MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2], &transform->rotation);
        FieldEffects_GetObjectPos(task, &transform->translation);
        transform->translation.x += wake->offset.x + offset->x;
        transform->translation.y += wake->offset.y + offset->y;
        transform->translation.z += wake->offset.z + offset->z;
        transform->scale = wake->scale;
        FieldG3DObjSystem_SetObjectHidden(objSys, wake->objIdx, FALSE);
    } else {
        FieldG3DObjSystem_SetObjectHidden(objSys, surfTask->wake.objIdx, TRUE);
    }
}

// On the grid: bob under the actor, and trail the wake behind it
static void FldeffSurfPokemon_UpdateGrid(FieldEffectTask *task, void *work) {
    VecFx32 offset = { 0, 0, 0 };
    VecFx32 pos;
    VecFx32 actorPos;
    SurfPokemonTask *surfTask = work;
    u16 objCode;

    if (surfTask->mode == SURF_POKEMON_FOLLOW_NONE) {
        FldeffSurfPokemon_PlaceObjects(task, surfTask);
        return;
    }
    surfTask->dir = GetActorFaceDir(surfTask->params.actor);
    if (surfTask->mode == SURF_POKEMON_FOLLOW_BOB) {
        surfTask->bob += surfTask->bobSpeed;
        if (surfTask->bob >= FX32_CONST(4)) {
            surfTask->bob = FX32_CONST(4);
            surfTask->bobSpeed = -surfTask->bobSpeed;
        } else if (surfTask->bob <= FX32_ONE) {
            surfTask->bob = FX32_ONE;
            surfTask->bobSpeed = -surfTask->bobSpeed;
        }
        offset.x = 0;
        offset.y = surfTask->bob + FX32_ONE;
        offset.z = FX32_CONST(5);
        // These object codes sit higher on the Pokémon
        objCode = FldAct_GetObjCode(surfTask->params.actor);
        if (objCode == OBJ_CODE_160 || objCode == OBJ_CODE_161) {
            offset.y = surfTask->bob + FX32_CONST(8);
        }
        FldAct_SetOuterDrawOffset(surfTask->params.actor, &offset);
    } else if (surfTask->mode == SURF_POKEMON_FOLLOW_KEEP_OFFSET) {
        FldAct_GetOuterDrawOffset(surfTask->params.actor, &offset);
    }
    CopyActorPosAllAdd(surfTask->params.actor, &pos);
    pos.x -= offset.x;
    pos.y -= offset.y;
    pos.z -= offset.z;
    if (surfTask->mode != SURF_POKEMON_FOLLOW_KEEP_OFFSET) {
        pos.y += surfTask->bob - FX32_ONE;
    }
    FieldEffects_SetObjectPos(task, &pos);
    if (surfTask->mode == SURF_POKEMON_FOLLOW_KEEP_OFFSET || surfTask->hideWake == TRUE) {
        SurfPokemonWake *wake = &surfTask->wake;

        wake->dir = DIR_NONE;
        if (surfTask->hideWake == TRUE) {
            wake->hidden = TRUE;
        }
    } else {
        SurfPokemonWake *wake = &surfTask->wake;
        fx32 distance;
        fx32 scale;

        FieldG3DObjSystem_StepObjAnmLoop(FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects),
                                         wake->objIdx, FX32_ONE);
        CopyActorWPos(surfTask->params.actor, &actorPos);
        if (surfTask->dir != wake->dir) {
            wake->dir = surfTask->dir;
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
        distance = FldeffSurfPokemon_GetWakeLength(&actorPos, &wake->pos, wake->dir);
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
        scale = FldeffSurfPokemon_GetWakeScale(distance);
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
    FldeffSurfPokemon_PlaceObjects(task, surfTask);
}

static void FldeffSurfPokemon_DrawGrid(FieldEffectTask *task, void *work) {
    SurfPokemonTask *surfTask = work;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects);

    FieldG3DObjSystem_DrawObj(objSys, surfTask->objIdx);
    FieldG3DObjSystem_DrawObj(objSys, surfTask->wake.objIdx);
}

// On a rail: the same along the rail's directions
static void FldeffSurfPokemon_UpdateRail(FieldEffectTask *task, void *work) {
    VecFx32 offset = { 0, 0, 0 };
    VecFx32 pos;
    VecFx32 actorPos;
    VecFx16 up;
    VecFx16 forward;
    SurfPokemonTask *surfTask = work;

    if (surfTask->mode == SURF_POKEMON_FOLLOW_NONE) {
        return;
    }
    surfTask->dir = GetActorFaceDir(surfTask->params.actor);
    RailActor_GetDrawOffsetByDir(surfTask->params.actor, DIR_UP, &up);
    RailActor_GetDrawOffsetByDir(surfTask->params.actor, surfTask->dir, &forward);
    up.y = 0;
    VEC_Fx16Normalize(&up, &up);
    forward.y = 0;
    VEC_Fx16Normalize(&forward, &forward);
    if (surfTask->mode == SURF_POKEMON_FOLLOW_BOB) {
        surfTask->bob += surfTask->bobSpeed;
        if (surfTask->bob >= FX32_CONST(4)) {
            surfTask->bob = FX32_CONST(4);
            surfTask->bobSpeed = -surfTask->bobSpeed;
        } else if (surfTask->bob <= FX32_ONE) {
            surfTask->bob = FX32_ONE;
            surfTask->bobSpeed = -surfTask->bobSpeed;
        }
        offset.x = FX_Mul(-up.x, FX32_CONST(7));
        offset.y = surfTask->bob + FX32_CONST(5);
        offset.z = FX_Mul(-up.z, FX32_CONST(7));
        FldAct_SetOuterDrawOffset(surfTask->params.actor, &offset);
    }
    CopyActorPosAllAdd(surfTask->params.actor, &pos);
    pos.x -= offset.x;
    pos.y -= offset.y;
    pos.z -= offset.z;
    pos.y += surfTask->bob - FX32_ONE;
    FieldEffects_SetObjectPos(task, &pos);
    if (surfTask->mode == SURF_POKEMON_FOLLOW_KEEP_OFFSET || surfTask->hideWake == TRUE) {
        SurfPokemonWake *wake = &surfTask->wake;

        wake->dir = DIR_NONE;
        if (surfTask->hideWake == TRUE) {
            wake->hidden = TRUE;
        }
    } else {
        SurfPokemonWake *wake = &surfTask->wake;
        fx32 distance;
        fx32 scale;

        FieldG3DObjSystem_StepObjAnmLoop(FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects),
                                         wake->objIdx, FX32_ONE);
        CopyActorWPos(surfTask->params.actor, &actorPos);
        if (surfTask->dir != wake->dir) {
            wake->dir = surfTask->dir;
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
        scale = FldeffSurfPokemon_GetWakeScale(distance);
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

static void FldeffSurfPokemon_DrawRail(FieldEffectTask *task, void *work) {
    VecFx32 forward;
    VecFx32 back;
    MtxFx33 pokeRotation;
    MtxFx33 wakeRotation;
    VecFx16 dir;
    FieldG3DObjSystem *objSys;
    SRTMatrix *transform;
    SurfPokemonTask *surfTask = work;
    SurfPokemonWake *wake = &surfTask->wake;

    RailActor_GetDrawOffsetByDir(surfTask->params.actor, surfTask->dir, &dir);
    forward.x = dir.x;
    forward.y = 0;
    forward.z = dir.z;
    back.x = -dir.x;
    back.y = 0;
    back.z = -dir.z;
    MAT3_RotationDir(&forward, &pokeRotation);
    MAT3_RotationDir(&back, &wakeRotation);
    objSys = FieldEffects_GetFieldMapData(surfTask->params.surfPokemon->effects);
    transform = FieldG3DObjSystem_GetObjTransform(objSys, surfTask->objIdx);
    transform->rotation = pokeRotation;
    FieldEffects_GetObjectPos(task, &transform->translation);
    if (surfTask->wake.hidden == FALSE) {
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
    FieldG3DObjSystem_DrawObj(objSys, surfTask->objIdx);
    FieldG3DObjSystem_DrawObj(objSys, wake->objIdx);
}

void *FldeffSurfPokemonSplash_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectSurfPokemonSplash *splash =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectSurfPokemonSplash), TRUE, "fldeff_namipoke.c", 975);

    splash->effects = effects;
    FldeffSurfPokemonSplash_LoadResources(splash);
    return splash;
}

void FldeffSurfPokemonSplash_Delete(FieldEffects *effects, void *data) {
    FldeffSurfPokemonSplash_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffSurfPokemonSplash_LoadResources(FieldEffectSurfPokemonSplash *splash) {
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

static void FldeffSurfPokemonSplash_FreeResources(FieldEffectSurfPokemonSplash *splash) {
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(splash->effects);

    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup0);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup1);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup2);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup3);
    FieldG3DObjSystem_FreeResGroup(objSys, splash->resGroup4);
}

FieldEffectTask *FldeffSurfPokemonSplash_StartOnEffect(FieldEffects *effects, u32 kind, FieldEffectTask *parent) {
    SplashTaskParams params;

    params.splash = FieldEffects_GetHandleData(effects, FLDEFF_SURF_POKEMON_SPLASH);
    params.kind = kind;
    params.parent = parent;
    params.endWithAnim = FALSE;
    return FieldEffects_TCBCreate(effects, &SPLASH_TASK_VTABLE, NULL, 0, &params, 1);
}

FieldEffectTask *FldeffSurfPokemonSplash_StartAtPos(FieldEffects *effects, u32 kind, const VecFx32 *pos) {
    SplashTaskParams params;

    params.splash = FieldEffects_GetHandleData(effects, FLDEFF_SURF_POKEMON_SPLASH);
    params.kind = kind;
    params.parent = NULL;
    params.endWithAnim = TRUE;
    return FieldEffects_TCBCreate(effects, &SPLASH_TASK_VTABLE, pos, 0, &params, 1);
}

BOOL FldeffSurfPokemonSplash_IsFinished(FieldEffectTask *task) {
    if (task != NULL) {
        SplashTask *splashTask = FieldEffects_GetObjectWork(task);

        return splashTask->animEnded;
    }
    return TRUE;
}

static void FldeffSurfPokemonSplash_Init(FieldEffectTask *task, void *work) {
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

static void FldeffSurfPokemonSplash_Exit(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;

    FieldG3DObjSystem_FreeObj(FieldEffects_GetFieldMapData(splashTask->params.splash->effects), splashTask->objIdx);
}

// Play the animation, once for kind 0 and over and over for the others, and follow the Pokémon
static void FldeffSurfPokemonSplash_Update(FieldEffectTask *task, void *work) {
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

static void FldeffSurfPokemonSplash_Draw(FieldEffectTask *task, void *work) {
    SplashTask *splashTask = work;
    SRTMatrix *transform = FieldG3DObjSystem_GetObjTransform(
        FieldEffects_GetFieldMapData(splashTask->params.splash->effects), splashTask->objIdx);

    FieldEffects_GetObjectPos(task, &transform->translation);
}
