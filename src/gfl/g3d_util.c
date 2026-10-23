#include "types.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"

#define G3D_MGR_SCENE_MAX 64
#define G3D_MGR_SCENE_NONE 0xffff

// What an actor loaded a copy of, as its model's resource was already taken, and frees with it
#define G3D_ACTOR_OWN_MODEL (1 << 0)
#define G3D_ACTOR_OWN_TEXTURE (1 << 1)

typedef struct {
    u16 firstResource;
    u16 resourceCount;
    u16 firstActor;
    u16 actorCount;
} G3DScene;

struct G3DManager {
    void **resources;
    // Whether an actor has taken each resource
    u8 *resourceUsed;
    u16 resourceLimit;
    G3DActor **actors;
    u8 *actorFlags;
    u16 actorLimit;
    G3DScene scenes[G3D_MGR_SCENE_MAX];
    HeapID heapId;
};

static void GFL_G3DMgrLoadResources(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId);
static void GFL_G3DMgrProcessArcLoadReq(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId,
                                        ArcTool *arc);
static void GFL_G3DMgrFreeSceneResources(G3DManager *manager, u16 scene);
static void GFL_G3DMgrLoadActors(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId);
static void GFL_G3DMgrLoadActorsFast(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId);
static void GFL_G3DMgrFreeSceneActors(G3DManager *manager, u16 scene);

G3DManager *GFL_G3DMgrCreate(u16 resourceLimit, u16 actorLimit, HeapID heapId) {
    G3DManager *manager = GFL_HeapAllocate(heapId, sizeof(G3DManager), TRUE, "g3d_util.c", 92);
    int i;

    manager->heapId = heapId;
    manager->resources = GFL_HeapAllocate(heapId, resourceLimit * sizeof(void *), TRUE, "g3d_util.c", 97);
    manager->resourceUsed = GFL_HeapAllocate(heapId, resourceLimit, TRUE, "g3d_util.c", 98);
    manager->resourceLimit = resourceLimit;
    for (i = 0; i < resourceLimit; i++) {
        manager->resourceUsed[i] = FALSE;
    }
    manager->actors = GFL_HeapAllocate(heapId, actorLimit * sizeof(G3DActor *), TRUE, "g3d_util.c", 105);
    manager->actorFlags = GFL_HeapAllocate(heapId, actorLimit, TRUE, "g3d_util.c", 106);
    manager->actorLimit = actorLimit;
    for (i = 0; i < actorLimit; i++) {
        manager->actorFlags[i] = 0;
    }
    for (i = 0; i < G3D_MGR_SCENE_MAX; i++) {
        manager->scenes[i].firstResource = G3D_MGR_SCENE_NONE;
        manager->scenes[i].resourceCount = G3D_MGR_SCENE_NONE;
        manager->scenes[i].firstActor = G3D_MGR_SCENE_NONE;
        manager->scenes[i].actorCount = G3D_MGR_SCENE_NONE;
    }
    return manager;
}

void GFL_G3DMgrFree(G3DManager *manager) {
    GFL_HeapFree(manager->actorFlags);
    GFL_HeapFree(manager->actors);
    GFL_HeapFree(manager->resourceUsed);
    GFL_HeapFree(manager->resources);
    GFL_HeapFree(manager);
}

u16 GFL_G3DMgrNewScene(G3DManager *manager, const G3DSceneSetup *setup) {
    u16 firstResource = 0;
    u16 firstActor = 0;
    int i;

    for (i = 0; i < G3D_MGR_SCENE_MAX; i++) {
        if (manager->scenes[i].firstResource == G3D_MGR_SCENE_NONE) {
            manager->scenes[i].firstResource = firstResource;
            manager->scenes[i].resourceCount = setup->resourceCount;
            manager->scenes[i].firstActor = firstActor;
            manager->scenes[i].actorCount = setup->actorCount;
            GFL_G3DMgrLoadResources(manager, i, setup, manager->heapId);
            GFL_G3DMgrLoadActors(manager, i, setup, manager->heapId);
            return i;
        }
        firstResource = manager->scenes[i].firstResource + manager->scenes[i].resourceCount;
        firstActor = manager->scenes[i].firstActor + manager->scenes[i].actorCount;
    }
    return G3D_MGR_SCENE_NONE;
}

u16 GFL_G3DMgrNewSceneFast(G3DManager *manager, const G3DSceneSetup *setup) {
    u16 firstResource = 0;
    u16 firstActor = 0;
    int i;

    for (i = 0; i < G3D_MGR_SCENE_MAX; i++) {
        if (manager->scenes[i].firstResource == G3D_MGR_SCENE_NONE) {
            manager->scenes[i].firstResource = firstResource;
            manager->scenes[i].resourceCount = setup->resourceCount;
            manager->scenes[i].firstActor = firstActor;
            manager->scenes[i].actorCount = setup->actorCount;
            GFL_G3DMgrLoadResources(manager, i, setup, manager->heapId);
            GFL_G3DMgrLoadActorsFast(manager, i, setup, manager->heapId);
            return i;
        }
        firstResource = manager->scenes[i].firstResource + manager->scenes[i].resourceCount;
        firstActor = manager->scenes[i].firstActor + manager->scenes[i].actorCount;
    }
    return G3D_MGR_SCENE_NONE;
}

u16 GFL_G3DMgrNewSceneFastEx(G3DManager *manager, const G3DSceneSetup *setup, ArcTool *arc) {
    u16 firstResource = 0;
    u16 firstActor = 0;
    int i;

    for (i = 0; i < G3D_MGR_SCENE_MAX; i++) {
        if (manager->scenes[i].firstResource == G3D_MGR_SCENE_NONE) {
            manager->scenes[i].firstResource = firstResource;
            manager->scenes[i].resourceCount = setup->resourceCount;
            manager->scenes[i].firstActor = firstActor;
            manager->scenes[i].actorCount = setup->actorCount;
            GFL_G3DMgrProcessArcLoadReq(manager, i, setup, manager->heapId, arc);
            GFL_G3DMgrLoadActorsFast(manager, i, setup, manager->heapId);
            return i;
        }
        firstResource = manager->scenes[i].firstResource + manager->scenes[i].resourceCount;
        firstActor = manager->scenes[i].firstActor + manager->scenes[i].actorCount;
    }
    return G3D_MGR_SCENE_NONE;
}

void GFL_G3DMgrDeleteScene(G3DManager *manager, u16 scene) {
    GFL_G3DMgrFreeSceneActors(manager, scene);
    GFL_G3DMgrFreeSceneResources(manager, scene);
    manager->scenes[scene].firstResource = G3D_MGR_SCENE_NONE;
    manager->scenes[scene].resourceCount = G3D_MGR_SCENE_NONE;
    manager->scenes[scene].firstActor = G3D_MGR_SCENE_NONE;
    manager->scenes[scene].actorCount = G3D_MGR_SCENE_NONE;
}

static inline void G3DMgr_ReadResource(G3DManager *manager, const G3DSceneResourceSetup *setup, void **resource) {
    switch (setup->type) {
    case G3D_SCENE_RES_ARCSYS:
        *resource = GFL_G3DSysReadArcSysResource(setup->arcId, setup->fileId);
        break;
    case G3D_SCENE_RES_FS:
        *resource = GFL_G3DSysReadFSResource((const char *)setup->arcId, setup->fileId);
        break;
    case G3D_SCENE_RES_ARCSYS_MGRHEAP: {
        void *res = GFL_HeapAllocate(manager->heapId, GFL_G3DResGetAllocSize(), FALSE, "g3d_util.c", 342);

        GFL_G3DResSetup(res, GFL_ArcSysReadHeapNew(setup->arcId, setup->fileId, manager->heapId));
        *resource = res;
        break;
    }
    case G3D_SCENE_RES_FS_MGRHEAP: {
        void *res = GFL_HeapAllocate(manager->heapId, GFL_G3DResGetAllocSize(), FALSE, "g3d_util.c", 355);

        GFL_G3DResSetup(res, GFL_ArcSysReadHeapNewDirect((const char *)setup->arcId, setup->fileId, manager->heapId));
        *resource = res;
        break;
    }
    }
}

static void GFL_G3DMgrLoadResources(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId) {
    u16 firstResource = manager->scenes[scene].firstResource;
    u16 count = manager->scenes[scene].resourceCount;
    int i;

    for (i = 0; i < count; i++) {
        G3DMgr_ReadResource(manager, &setup->resources[i], &manager->resources[firstResource + i]);
    }
}

static void GFL_G3DMgrProcessArcLoadReq(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId,
                                        ArcTool *arc) {

    u16 count = manager->scenes[scene].resourceCount;
    u16 firstResource = manager->scenes[scene].firstResource;
    int i;

    for (i = 0; i < count; i++) {
        manager->resources[firstResource + i] = GFL_G3DSysReadArcToolResource(arc, setup->resources[i].fileId);
    }
}

static void GFL_G3DMgrFreeSceneResources(G3DManager *manager, u16 scene) {

    u16 firstResource = manager->scenes[scene].firstResource;
    u16 count = manager->scenes[scene].resourceCount;
    int i;

    for (i = 0; i < count; i++) {
        void *resource = manager->resources[firstResource + i];

        if (resource != NULL) {
            manager->resourceUsed[firstResource + i] = FALSE;
            if (GFL_G3DResCheckType(resource, G3D_RES_CHECK_TEX) == TRUE) {
                GFL_G3DResFreeTexData(resource);
            }
            GFL_G3DResFree(resource);
        }
    }
}

// The scene's resource, which the actor takes without a copy
static inline void *G3DMgr_UseResource(G3DManager *manager, u16 firstResource, u16 index) {
    manager->resourceUsed[firstResource + index] = TRUE;
    return manager->resources[firstResource + index];
}

// The scene's resource for an actor, or a copy of it if another actor has taken it, returning whether it is a copy
static inline BOOL G3DMgr_TakeResource(G3DManager *manager, u16 firstResource, u16 index, const G3DSceneSetup *setup,
                                       void **resource) {
    if (manager->resourceUsed[firstResource + index] == TRUE) {
        G3DMgr_ReadResource(manager, &setup->resources[index], resource);
        return TRUE;
    }
    *resource = G3DMgr_UseResource(manager, firstResource, index);
    return FALSE;
}

static void GFL_G3DMgrLoadActors(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId) {
    u16 firstResource = manager->scenes[scene].firstResource;
    u16 firstActor = manager->scenes[scene].firstActor;
    u16 count = manager->scenes[scene].actorCount;
    int i;

    for (i = 0; i < count; i++) {
        const G3DSceneActorSetup *actorSetup = &setup->actors[i];
        void *mdlResource;
        void *texResource;
        G3DModel *model;
        void **animations;
        int j;

        if (G3DMgr_TakeResource(manager, firstResource, actorSetup->modelResource, setup, &mdlResource) == TRUE) {
            manager->actorFlags[firstActor + i] |= G3D_ACTOR_OWN_MODEL;
        }
        if (actorSetup->texResource != actorSetup->modelResource) {
            if (GFL_G3DResCheckType(manager->resources[firstResource + actorSetup->texResource], G3D_RES_CHECK_MDL) ==
                TRUE) {
                if (G3DMgr_TakeResource(manager, firstResource, actorSetup->texResource, setup, &texResource) == TRUE) {
                    manager->actorFlags[firstActor + i] |= G3D_ACTOR_OWN_TEXTURE;
                    GFL_G3DResUploadTexData(texResource);
                }
            } else {
                texResource = manager->resources[firstResource + actorSetup->texResource];
                GFL_G3DResUploadTexData(texResource);
            }
        } else {
            texResource = mdlResource;
            if (!GFL_G3DResIsTexUploadDone(mdlResource)) {
                GFL_G3DResUploadTexData(mdlResource);
            }
        }
        model = GFL_G3DMdlCreate(mdlResource, actorSetup->modelIndex, texResource);
        animations =
            GFL_HeapAllocate(HEAPID_TAIL(heapId), actorSetup->animationCount * sizeof(void *), TRUE, "g3d_util.c", 534);
        for (j = 0; j < actorSetup->animationCount; j++) {
            const G3DSceneAnimationSetup *anmSetup = &actorSetup->animations[j];
            void *animation;

            if (anmSetup->resource == 0xff) {
                animation = NULL;
            } else {
                animation =
                    GFL_G3DAnmCreate(model, manager->resources[firstResource + anmSetup->resource], anmSetup->index);
            }
            animations[j] = animation;
        }
        manager->actors[firstActor + i] = GFL_G3DActorCreate(model, animations, actorSetup->animationCount);
        GFL_HeapFree(animations);
    }
}

static void GFL_G3DMgrLoadActorsFast(G3DManager *manager, u16 scene, const G3DSceneSetup *setup, HeapID heapId) {
    u16 firstResource = manager->scenes[scene].firstResource;
    u16 firstActor = manager->scenes[scene].firstActor;
    u16 count = manager->scenes[scene].actorCount;
    int i;

    for (i = 0; i < count; i++) {
        const G3DSceneActorSetup *actorSetup = &setup->actors[i];
        void *mdlResource;
        void *texResource;
        G3DModel *model;
        void **animations;
        int j;

        mdlResource = G3DMgr_UseResource(manager, firstResource, actorSetup->modelResource);
        if (actorSetup->texResource != actorSetup->modelResource) {
            if (GFL_G3DResCheckType(manager->resources[firstResource + actorSetup->texResource], G3D_RES_CHECK_MDL) ==
                TRUE) {
                texResource = G3DMgr_UseResource(manager, firstResource, actorSetup->texResource);
            } else {
                texResource = manager->resources[firstResource + actorSetup->texResource];
                GFL_G3DResUploadTexData(texResource);
            }
        } else {
            texResource = mdlResource;
            if (!GFL_G3DResIsTexUploadDone(mdlResource)) {
                GFL_G3DResUploadTexData(mdlResource);
            }
        }
        model = GFL_G3DMdlCreate(mdlResource, actorSetup->modelIndex, texResource);
        animations =
            GFL_HeapAllocate(HEAPID_TAIL(heapId), actorSetup->animationCount * sizeof(void *), TRUE, "g3d_util.c", 609);
        for (j = 0; j < actorSetup->animationCount; j++) {
            const G3DSceneAnimationSetup *anmSetup = &actorSetup->animations[j];
            void *animation;

            if (anmSetup->resource == 0xff) {
                animation = NULL;
            } else {
                animation =
                    GFL_G3DAnmCreate(model, manager->resources[firstResource + anmSetup->resource], anmSetup->index);
            }
            animations[j] = animation;
        }
        manager->actors[firstActor + i] = GFL_G3DActorCreate(model, animations, actorSetup->animationCount);
        GFL_HeapFree(animations);
    }
}

static void GFL_G3DMgrFreeSceneActors(G3DManager *manager, u16 scene) {

    u16 firstActor = manager->scenes[scene].firstActor;
    u16 count = manager->scenes[scene].actorCount;
    int i;

    for (i = 0; i < count; i++) {
        G3DActor *actor = manager->actors[firstActor + i];
        G3DModel *model = GFL_G3DActorGetMdl(actor);
        int anmCount = GFL_G3DActorGetAnmCount(actor);
        int j;

        for (j = 0; j < anmCount; j++) {
            GFL_G3DAnmFree(GFL_G3DActorGetAnm(actor, j));
        }
        // BUG: each test is for every flag but its own, so an actor with its own copy of only a model resource frees
        // the texture resource the model uses, and one with its own texture frees the scene's model resource
#ifdef BUGFIX
        if (manager->actorFlags[firstActor + i] & G3D_ACTOR_OWN_TEXTURE) {
#else
        if (manager->actorFlags[firstActor + i] & (u8)~G3D_ACTOR_OWN_TEXTURE) {
#endif
            void *texResource = GFL_G3DMdlGetTexResource(model);

            GFL_G3DResFreeTexData(texResource);
            GFL_G3DResFree(texResource);
        }
#ifdef BUGFIX
        if (manager->actorFlags[firstActor + i] & G3D_ACTOR_OWN_MODEL) {
#else
        if (manager->actorFlags[firstActor + i] & (u8)~G3D_ACTOR_OWN_MODEL) {
#endif
            GFL_G3DResFree(GFL_G3DMdlGetMdlResource(model));
        }
        GFL_G3DMdlFree(model);
        GFL_G3DActorFree(actor);
    }
}

u16 GFL_G3DMgrGetSceneResCount(G3DManager *manager, u16 scene) {
    return manager->scenes[scene].resourceCount;
}

u16 GFL_G3DMgrGetSceneFirstActorIdx(G3DManager *manager, u16 scene) {
    return manager->scenes[scene].firstActor;
}

u16 GFL_G3DSceneGetNodeActorCount(G3DManager *manager, u16 scene) {
    return manager->scenes[scene].actorCount;
}

G3DResource *GFL_G3DMgrGetResource(G3DManager *manager, u16 resource) {
    return manager->resources[resource];
}

G3DActor *GFL_G3DMgrGetActor(G3DManager *manager, u16 actor) {
    return manager->actors[actor];
}
