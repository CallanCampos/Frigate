// The field's expansion objects: up to four scenes of 3D actors that zones such as gyms add, each actor with a
// matrix, a flag that hides it and its animations' playback. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. The functions' names are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// FieldExpObjAnm_SetFrameStep, FieldExpObj_SetActorCulling, FieldExpObj_IsActorHidden, FieldExpObj_SetSceneFrameStep,
// FieldExpObjAnm_IsPaused, FieldExpObj_GetAnmFrame and FieldExpObjAnm_GetLastFrame, which are ours, guessed from what
// they do, as is gEnterBuildingLCDConfig. The code was first written by mariano54 on their parallel-decomp branch.
#include "types.h"
#include "field/field_exp_obj.h"
#include "field/fld_exp_obj.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g3d.h"
#include "struct_decls.h"

#define FIELD_EXP_OBJ_SCENE_MAX 4

// The playback of one of an actor's animations
struct FieldExpObjAnm {
    u8 paused;
    u8 looped;
    // Whether the animation is bound to the actor
    u8 active;
    fx32 frameStep;
    // What the last step returned: FALSE once an animation that doesn't loop has reached its end
    BOOL playing;
    // The actor's animation, from GFL_G3DActorGetAnm
    G3DAnimation *anm;
};

typedef struct {
    int count;
    FieldExpObjAnm *anms;
} FieldExpObjAnmSet;

typedef struct {
    // Whether FieldExpObj_Draw culls the actor by its bounding box
    BOOL cullByBBox;
    BOOL hidden;
    SRTMatrix matrix;
} FieldExpObjActor;

typedef struct {
    BOOL active;
    FieldExpObjActor *actors;
    FieldExpObjAnmSet *anmSets;
    // The scene's index in the G3D manager
    u16 scene;
    u16 actorCount;
} FieldExpObjScene;

struct FieldExpObjSystem {
    u32 heapId;
    G3DManager *g3d;
    FieldExpObjScene scenes[FIELD_EXP_OBJ_SCENE_MAX];
};

static void FieldExpObj_AllocAnmSet(FieldExpObjAnmSet *set, int count, u32 heapId);

// The LCD setup of field_transition.c's building entrance effect, which the ROM has in this file's .rodata
const BGSysLCDConfig gEnterBuildingLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_3D };

FieldExpObjSystem *FieldExpObj_Create(u32 resourceLimit, u32 actorLimit, u32 heapId) {
    FieldExpObjSystem *system = GFL_HeapAllocate(heapId, sizeof(FieldExpObjSystem), TRUE, "fld_exp_obj.c", 76);

    system->g3d = GFL_G3DMgrCreate(resourceLimit, actorLimit, heapId);
    system->heapId = heapId;
    return system;
}

void FieldExpObj_Free(FieldExpObjSystem *system) {
    u16 i;

    for (i = 0; i < FIELD_EXP_OBJ_SCENE_MAX; i++) {
        if (system->scenes[i].active) {
            FieldExpObj_FreeScene(system, i);
        }
    }
    GFL_G3DMgrFree(system->g3d);
    GFL_HeapFree(system);
}

void FieldExpObj_AddScene(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene) {
    u16 sceneId = GFL_G3DMgrNewSceneFast(system->g3d, setup);
    FieldExpObjActor *actors;
    u16 actorCount;
    u16 i;
    int j;
    FieldExpObjAnmSet *set;
    G3DActor *g3dActor;

    actorCount = setup->actorCount;
    system->scenes[scene].actorCount = actorCount;
    system->scenes[scene].active = TRUE;
    system->scenes[scene].scene = sceneId;
    actors = GFL_HeapAllocate(system->heapId, actorCount * sizeof(FieldExpObjActor), TRUE, "fld_exp_obj.c", 139);
    system->scenes[scene].actors = actors;
    system->scenes[scene].anmSets =
        GFL_HeapAllocate(system->heapId, actorCount * sizeof(FieldExpObjAnmSet), TRUE, "fld_exp_obj.c", 142);
    for (i = 0; i < actorCount; i++) {
        FieldExpObj_AllocAnmSet(&system->scenes[scene].anmSets[i], setup->actors[i].animationCount, system->heapId);
        actors[i].matrix.scale.x = FX32_ONE;
        actors[i].matrix.scale.y = FX32_ONE;
        actors[i].matrix.scale.z = FX32_ONE;
        MTX_Identity33_(&actors[i].matrix.rotation);
        actors[i].matrix.translation.x = 0;
        actors[i].matrix.translation.y = 0;
        actors[i].matrix.translation.z = 0;
        actors[i].cullByBBox = FALSE;
        actors[i].hidden = FALSE;
        g3dActor = FieldExpObj_GetActor(system, scene, i);
        set = &system->scenes[scene].anmSets[i];
        for (j = 0; j < set->count; j++) {
            set->anms[j].anm = GFL_G3DActorGetAnm(g3dActor, j);
        }
    }
}

void FieldExpObj_FreeScene(FieldExpObjSystem *system, u16 scene) {
    u16 i;

    // BUG: The scene's flag is read before the scene is checked against the size of the array
#ifdef BUGFIX
    if (scene < FIELD_EXP_OBJ_SCENE_MAX && system->scenes[scene].active) {
#else
    if (system->scenes[scene].active && scene < FIELD_EXP_OBJ_SCENE_MAX) {
#endif
        for (i = 0; i < system->scenes[scene].actorCount; i++) {
            GFL_HeapFree(system->scenes[scene].anmSets[i].anms);
        }
        GFL_HeapFree(system->scenes[scene].anmSets);
        GFL_HeapFree(system->scenes[scene].actors);
        GFL_G3DMgrDeleteScene(system->g3d, system->scenes[scene].scene);
        system->scenes[scene].active = FALSE;
    }
}

SRTMatrix *FieldExpObj_GetActorMatrixPtr(FieldExpObjSystem *system, u16 scene, u16 actor) {
    // BUG: The scene's flag is read before the scene is checked against the size of the array
#ifdef BUGFIX
    if (scene >= FIELD_EXP_OBJ_SCENE_MAX) {
        return NULL;
    }
    if (!system->scenes[scene].active) {
        return NULL;
    }
#else
    if (!system->scenes[scene].active) {
        return NULL;
    }
    if (scene >= FIELD_EXP_OBJ_SCENE_MAX) {
        return NULL;
    }
#endif
    return &system->scenes[scene].actors[actor].matrix;
}

void FieldExpObj_SetActorCulling(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL cullByBBox) {
    system->scenes[scene].actors[actor].cullByBBox = cullByBBox;
}

void FieldExpObj_SetActorHidden(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL hidden) {
    system->scenes[scene].actors[actor].hidden = hidden;
}

BOOL FieldExpObj_IsActorHidden(FieldExpObjSystem *system, u16 scene, u16 actor) {
    return system->scenes[scene].actors[actor].hidden;
}

G3DActor *FieldExpObj_GetActor(FieldExpObjSystem *system, u16 scene, u16 actor) {
    u16 first;

    // BUG: The scene's flag is read before the scene is checked against the size of the array
#ifdef BUGFIX
    if (scene >= FIELD_EXP_OBJ_SCENE_MAX) {
        return NULL;
    }
    if (!system->scenes[scene].active) {
        return NULL;
    }
#else
    if (!system->scenes[scene].active) {
        return NULL;
    }
    if (scene >= FIELD_EXP_OBJ_SCENE_MAX) {
        return NULL;
    }
#endif
    first = GFL_G3DMgrGetSceneFirstActorIdx(system->g3d, system->scenes[scene].scene);
    return GFL_G3DMgrGetActor(system->g3d, first + actor);
}

void FieldExpObj_SetSceneFrameStep(FieldExpObjSystem *system, u16 scene, fx32 frameStep) {
    if (system->scenes[scene].active == TRUE) {
        u8 i;
        int actorCount = GFL_G3DSceneGetNodeActorCount(system->g3d, system->scenes[scene].scene);

        for (i = 0; i < actorCount; i++) {
            u8 j;
            int anmCount;

            // The actor is fetched and not used
            FieldExpObj_GetActor(system, scene, i);
            anmCount = system->scenes[scene].anmSets[i].count;
            for (j = 0; j < anmCount; j++) {
                if (system->scenes[scene].anmSets[i].anms[j].active) {
                    FieldExpObjAnm_SetFrameStep(&system->scenes[scene].anmSets[i].anms[j], frameStep);
                }
            }
        }
    }
}

void FieldExpObj_Draw(FieldExpObjSystem *system) {
    u8 i;

    for (i = 0; i < FIELD_EXP_OBJ_SCENE_MAX; i++) {
        if (system->scenes[i].active == TRUE) {
            u8 j;
            int firstActor = GFL_G3DMgrGetSceneFirstActorIdx(system->g3d, system->scenes[i].scene);
            int actorCount = GFL_G3DSceneGetNodeActorCount(system->g3d, system->scenes[i].scene);

            for (j = 0; j < actorCount; j++) {
                if (!system->scenes[i].actors[j].hidden) {
                    G3DActor *g3dActor = GFL_G3DMgrGetActor(system->g3d, firstActor + j);

                    if (system->scenes[i].actors[j].cullByBBox) {
                        GFL_G3DSysDrawObjBBoxCull(g3dActor, &system->scenes[i].actors[j].matrix);
                    } else {
                        GFL_G3DSysDrawObj(g3dActor, &system->scenes[i].actors[j].matrix);
                    }
                }
            }
        }
    }
}

void FieldExpObj_StepAllAnimations(FieldExpObjSystem *system) {
    u8 i, j, k;
    u16 actorCount;
    int anmCount;
    G3DActor *g3dActor;
    FieldExpObjAnm *anm;

    for (i = 0; i < FIELD_EXP_OBJ_SCENE_MAX; i++) {
        if (system->scenes[i].active == TRUE) {
            actorCount = GFL_G3DSceneGetNodeActorCount(system->g3d, system->scenes[i].scene);
            for (j = 0; j < actorCount; j++) {
                g3dActor = FieldExpObj_GetActor(system, i, j);
                anmCount = system->scenes[i].anmSets[j].count;
                for (k = 0; k < anmCount; k++) {
                    anm = &system->scenes[i].anmSets[j].anms[k];
                    if (anm->active && !anm->paused) {
                        if (anm->looped) {
                            anm->playing = GFL_G3DActorStepAnmFrameLoop(g3dActor, k, anm->frameStep);
                        } else {
                            anm->playing = GFL_G3DActorStepAnmFrame(g3dActor, k, anm->frameStep);
                        }
                    }
                }
            }
        }
    }
}

static void FieldExpObj_AllocAnmSet(FieldExpObjAnmSet *set, int count, u32 heapId) {
    int i;
    FieldExpObjAnm *anms = GFL_HeapAllocate(heapId, count * sizeof(FieldExpObjAnm), TRUE, "fld_exp_obj.c", 463);

    set->anms = anms;
    set->count = count;
    for (i = 0; i < count; i++) {
        anms[i].paused = FALSE;
        anms[i].looped = TRUE;
        anms[i].active = FALSE;
        anms[i].frameStep = FX32_ONE;
    }
}

FieldExpObjAnm *FieldExpObj_GetAnmInfo(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm) {
    FieldExpObjAnm *anms;

    if (!system->scenes[scene].active) {
        return NULL;
    }
    if (actor >= system->scenes[scene].actorCount) {
        return NULL;
    }
    anms = system->scenes[scene].anmSets[actor].anms;
    if (anm >= system->scenes[scene].anmSets[actor].count) {
        return NULL;
    }
    return &anms[anm];
}

void FieldExpObjAnm_SetFrameStep(FieldExpObjAnm *anm, fx32 step) {
    anm->frameStep = step;
}

void FieldExpObjAnm_SetPaused(FieldExpObjAnm *anm, u8 paused) {
    anm->paused = paused;
}

u8 FieldExpObjAnm_IsPaused(FieldExpObjAnm *anm) {
    return anm->paused;
}

void FieldExpObjAnm_SetLooped(FieldExpObjAnm *anm, u8 looped) {
    anm->looped = looped;
}

BOOL FieldExpObjAnm_IsPlaybackFinished(FieldExpObjAnm *anm) {
    return !anm->playing;
}

void FieldExpObj_SetAnmFrame(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, fx32 frame) {
    // The frame is copied into a local, whose address is passed
    fx32 value = frame;

    GFL_G3DActorSetAnmFrame(FieldExpObj_GetActor(system, scene, actor), anm, &value);
}

fx32 FieldExpObj_GetAnmFrame(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm) {
    fx32 frame;

    GFL_G3DActorGetAnmFrame(FieldExpObj_GetActor(system, scene, actor), anm, &frame);
    return frame;
}

void FieldExpObj_SetAnm(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, BOOL bind) {
    G3DActor *g3dActor = FieldExpObj_GetActor(system, scene, actor);
    FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(system, scene, actor, anm);

    if (info != NULL) {
        if (bind) {
            if (info->active) {
                return;
            }
            GFL_G3DActorBindAnm(g3dActor, anm);
        } else {
            if (!info->active) {
                return;
            }
            GFL_G3DActorUnbindAnm(g3dActor, anm);
        }
        info->active = bind;
    }
}

fx32 FieldExpObjAnm_GetLastFrame(FieldExpObjAnm *anm) {
    return NNS_G3dAnmObjGetNumFrame(GFL_G3DAnmGetRenderObj(anm->anm)) - FX32_ONE;
}

void LoadFieldExpandObjData(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(setup->resources->arcId, HEAPID_TAIL((HeapID)system->heapId));
    u16 sceneId = GFL_G3DMgrNewSceneFastEx(system->g3d, setup, arc);
    FieldExpObjActor *actors;
    u16 actorCount;
    u16 i;
    int j;
    FieldExpObjAnmSet *set;
    G3DActor *g3dActor;

    GFL_ArcToolFree(arc);
    actorCount = setup->actorCount;
    system->scenes[scene].actorCount = actorCount;
    system->scenes[scene].active = TRUE;
    system->scenes[scene].scene = sceneId;
    actors = GFL_HeapAllocate(system->heapId, actorCount * sizeof(FieldExpObjActor), TRUE, "fld_exp_obj.c", 632);
    system->scenes[scene].actors = actors;
    system->scenes[scene].anmSets =
        GFL_HeapAllocate(system->heapId, actorCount * sizeof(FieldExpObjAnmSet), TRUE, "fld_exp_obj.c", 635);
    for (i = 0; i < actorCount; i++) {
        FieldExpObj_AllocAnmSet(&system->scenes[scene].anmSets[i], setup->actors[i].animationCount, system->heapId);
        actors[i].matrix.scale.x = FX32_ONE;
        actors[i].matrix.scale.y = FX32_ONE;
        actors[i].matrix.scale.z = FX32_ONE;
        MTX_Identity33_(&actors[i].matrix.rotation);
        actors[i].matrix.translation.x = 0;
        actors[i].matrix.translation.y = 0;
        actors[i].matrix.translation.z = 0;
        actors[i].cullByBBox = FALSE;
        actors[i].hidden = FALSE;
        g3dActor = FieldExpObj_GetActor(system, scene, i);
        set = &system->scenes[scene].anmSets[i];
        for (j = 0; j < set->count; j++) {
            set->anms[j].anm = GFL_G3DActorGetAnm(g3dActor, j);
        }
    }
}
