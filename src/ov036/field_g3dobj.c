// The field's 3D objects: models with animations that are loaded in groups from archives and drawn each frame. The
// name is the ROM's string.
#include "types.h"
#include "field/field.h"
#include "field/field_g3dobj.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/aeabi.h"

#define G3DOBJ_ANM_MAX 8
#define G3DOBJ_ANM_ALL 8

// The state of a resource group's textures, none in VRAM, in VRAM, or waiting to be put there, and of an object, free,
// set up, or waiting for its group's textures before it is
enum {
    G3DOBJ_STATE_NONE,
    G3DOBJ_STATE_READY,
    G3DOBJ_STATE_PENDING,
};

// The resources of a model with its textures and animations, with a G3DOBJ_STATE_ value
typedef struct {
    G3DResource *model;
    G3DResource *texture;
    G3DResource *animations[G3DOBJ_ANM_MAX];
    u8 animationCount;
    u8 state;
    u8 pad[2];
} FieldG3DObjResGroup;

typedef struct {
    u32 state : 2;
    u32 bboxCull : 1;
    u32 hidden : 1;
    // Set when the object is drawn by FieldG3DObjSystem_DrawObj rather than with the others
    u32 manualDraw : 1;
    // Set by the object's user once a frame's animation is stepped, and cleared when the frame is drawn
    u32 redraw : 1;
    u16 resGroup;
    u16 modelIndex;
    SRTMatrix transform;
    G3DActor *actor;
    G3DAnimation *animations[G3DOBJ_ANM_MAX];
} FieldG3DObj;

struct FieldG3DObjSystem {
    u16 heapId;
    u16 ready;
    u16 resGroupCount;
    u16 objCount;
    FieldG3DObjResGroup *resGroups;
    FieldG3DObj *objs;
    FieldColorPostFX *colorPostFx;
};

void FieldG3DObjResGroup_SetupTextures(FieldG3DObjResGroup *group);
void FieldG3DObjResGroup_LoadResources(FieldG3DObjResGroup *group, u16 heapId, FieldG3DObjResRequest *request,
                                       BOOL setupTextures, FieldColorPostFX *colorPostFx);
void FieldG3DObjResGroup_Free(FieldG3DObjResGroup *group);
void FieldG3DObj_Setup(FieldG3DObj *obj, FieldG3DObjResGroup *group, u16 resGroup, u16 modelIndex, u16 heapId,
                       const VecFx32 *position, BOOL initTransform);
void FieldG3DObj_Free(FieldG3DObj *obj, FieldG3DObjResGroup *group);

FieldG3DObjSystem *FieldG3DObjSystem_Create(HeapID heapId, u16 resGroupCount, u16 objCount) {
    FieldG3DObjSystem *system = GFL_HeapAllocate(heapId, sizeof(FieldG3DObjSystem), TRUE, "field_g3dobj.c", 115);

    system->resGroups =
        GFL_HeapAllocate(heapId, resGroupCount * sizeof(FieldG3DObjResGroup), TRUE, "field_g3dobj.c", 118);
    system->objs = GFL_HeapAllocate(heapId, objCount * sizeof(FieldG3DObj), TRUE, "field_g3dobj.c", 121);
    system->heapId = heapId;
    system->resGroupCount = resGroupCount;
    system->objCount = objCount;
    system->ready = TRUE;
    return system;
}

void FieldG3DObjSystem_Free(FieldG3DObjSystem *system) {
    u32 i;
    FieldG3DObj *obj;
    FieldG3DObjResGroup *group;

    system->ready = FALSE;
    obj = system->objs;
    for (i = 0; i < system->objCount; i++, obj++) {
        if (obj->state == G3DOBJ_STATE_READY && obj->actor != NULL) {
            FieldG3DObj_Free(obj, &system->resGroups[obj->resGroup]);
        }
    }
    GFL_HeapFree(system->objs);
    group = system->resGroups;
    for (i = 0; i < system->resGroupCount; i++, group++) {
        FieldG3DObjResGroup_Free(group);
    }
    GFL_HeapFree(system->resGroups);
    GFL_HeapFree(system);
}

void FieldG3DObjSystem_SetupResources(FieldG3DObjSystem *system) {
    int i;
    int j;
    FieldG3DObj *obj;
    FieldG3DObjResGroup *group;
    FieldG3DObjResGroup *groups;

    if (system->ready == TRUE) {
        obj = system->objs;
        group = system->resGroups;
        for (i = 0; i < system->resGroupCount; i++) {
            if (group->state == G3DOBJ_STATE_PENDING) {
                FieldG3DObjResGroup_SetupTextures(group);
            }
            group++;
        }
        groups = system->resGroups;
        for (j = 0; j < system->objCount; j++) {
            if (obj->state == G3DOBJ_STATE_PENDING) {
                FieldG3DObj_Setup(obj, &groups[obj->resGroup], obj->resGroup, obj->modelIndex, system->heapId, NULL,
                                  FALSE);
            }
            obj++;
        }
    }
}

void FieldG3DObjSystem_Draw(FieldG3DObjSystem *system) {
    u32 i;
    FieldG3DObj *obj = system->objs;

    for (i = 0; i < system->objCount; i++) {
        if (obj->state == G3DOBJ_STATE_READY) {
            if (!obj->manualDraw && !obj->hidden) {
                if (obj->bboxCull == 1) {
                    GFL_G3DSysDrawObjBBoxCull(obj->actor, &obj->transform);
                } else {
                    GFL_G3DSysDrawObj(obj->actor, &obj->transform);
                }
            }
            obj->redraw = 0;
        }
        obj++;
    }
}

void FieldG3DObjSystem_SetColorPostFX(FieldG3DObjSystem *system, FieldColorPostFX *colorPostFx) {
    system->colorPostFx = colorPostFx;
}

u16 FieldG3DObjSystem_AddResGroupCore(FieldG3DObjSystem *system, FieldG3DObjResRequest *request, BOOL setupTextures,
                                      FieldColorPostFX *colorPostFx) {
    u16 i;
    FieldG3DObjResGroup *group = system->resGroups;

    for (i = 0; i < system->resGroupCount; i++, group++) {
        if (group->model == NULL) {
            FieldG3DObjResGroup_LoadResources(group, system->heapId, request, setupTextures, colorPostFx);
            return i;
        }
    }
    return 0;
}

u16 FieldG3DObjSystem_AddResGroup(FieldG3DObjSystem *system, FieldG3DObjResRequest *request, BOOL setupTextures) {
    return FieldG3DObjSystem_AddResGroupCore(system, request, setupTextures, system->colorPostFx);
}

void FieldG3DObjSystem_FreeResGroup(FieldG3DObjSystem *system, u16 resGroup) {
    FieldG3DObjResGroup_Free(&system->resGroups[resGroup]);
}

u16 FieldG3DObjSystem_AddObj(FieldG3DObjSystem *system, u16 resGroup, u16 modelIndex, const VecFx32 *position) {
    u16 i;
    FieldG3DObj *obj = system->objs;

    for (i = 0; i < system->objCount; i++, obj++) {
        if (obj->state == G3DOBJ_STATE_NONE && obj->actor == NULL) {
            FieldG3DObjResGroup *group = &system->resGroups[resGroup];

            if (group->state == G3DOBJ_STATE_PENDING) {
                obj->resGroup = resGroup;
                obj->modelIndex = modelIndex;
                obj->state = G3DOBJ_STATE_PENDING;
                obj->bboxCull = 1;
                obj->transform.scale.x = FX32_ONE;
                obj->transform.scale.y = FX32_ONE;
                obj->transform.scale.z = FX32_ONE;
                if (position != NULL) {
                    obj->transform.translation = *position;
                }
                MTX_Identity33_(&obj->transform.rotation);
            } else {
                FieldG3DObj_Setup(obj, group, resGroup, modelIndex, system->heapId, position, TRUE);
            }
            return i;
        }
    }
    return 0;
}

void FieldG3DObjSystem_FreeObj(FieldG3DObjSystem *system, u16 objIndex) {
    FieldG3DObj *obj = &system->objs[objIndex];

    system->ready = FALSE;
    if (obj->state == G3DOBJ_STATE_READY) {
        FieldG3DObj_Free(obj, &system->resGroups[obj->resGroup]);
    }
    system->ready = TRUE;
}

SRTMatrix *FieldG3DObjSystem_GetObjTransform(FieldG3DObjSystem *system, u16 objIndex) {
    return &system->objs[objIndex].transform;
}

void FieldG3DObjSystem_SetObjPosition(FieldG3DObjSystem *system, u16 objIndex, const VecFx32 *position) {
    system->objs[objIndex].transform.translation = *position;
}

void FieldG3DObjSystem_SetObjectHidden(FieldG3DObjSystem *system, u16 objIndex, BOOL value) {
    system->objs[objIndex].hidden = value;
}

BOOL FieldG3DObjSystem_StepObjAnm(FieldG3DObjSystem *system, u16 objIndex, fx32 speed) {
    BOOL playing = FALSE;
    FieldG3DObj *obj = &system->objs[objIndex];
    int i;
    int count;

    if (obj->state == G3DOBJ_STATE_READY) {
        count = system->resGroups[obj->resGroup].animationCount;
        for (i = 0; i < count; i++) {
            if (GFL_G3DActorStepAnmFrame(obj->actor, i, speed)) {
                playing = TRUE;
            }
        }
    } else if (obj->state == G3DOBJ_STATE_PENDING) {
        playing = TRUE;
    }
    return playing;
}

BOOL FieldG3DObjSystem_StepObjAnmLoop(FieldG3DObjSystem *system, u16 objIndex, fx32 speed) {
    BOOL playing = FALSE;
    FieldG3DObj *obj = &system->objs[objIndex];
    int i;
    int count;

    if (obj->state == G3DOBJ_STATE_READY) {
        count = system->resGroups[obj->resGroup].animationCount;
        for (i = 0; i < count; i++) {
            if (GFL_G3DActorStepAnmFrameLoop(obj->actor, i, speed)) {
                playing = TRUE;
            }
        }
    } else if (obj->state == G3DOBJ_STATE_PENDING) {
        playing = TRUE;
    }
    return playing;
}

void FieldG3DObjSystem_SetAnmFrame(FieldG3DObjSystem *system, u16 objIndex, int anmIndex, fx32 frame) {
    FieldG3DObj *obj = &system->objs[objIndex];
    int count;
    int i;

    if (obj->state == G3DOBJ_STATE_READY) {
        count = system->resGroups[obj->resGroup].animationCount;
        if (anmIndex == G3DOBJ_ANM_ALL) {
            for (i = 0; i < count; i++) {
                GFL_G3DActorSetAnmFrame(obj->actor, i, &frame);
            }
        } else if (anmIndex < count) {
            GFL_G3DActorSetAnmFrame(obj->actor, anmIndex, &frame);
        }
    }
}

fx32 FieldG3DObjSystem_GetAnmFrame(FieldG3DObjSystem *system, u16 objIndex, u32 anmIndex) {
    fx32 frame = 0;
    FieldG3DObj *obj = &system->objs[objIndex];

    if (obj->state == G3DOBJ_STATE_READY) {
        if (!GFL_G3DActorGetAnmFrame(obj->actor, anmIndex, &frame)) {
            frame = 0;
        }
    }
    return frame;
}

void FieldG3DObjSystem_SetObjectManualDraw(FieldG3DObjSystem *system, u16 objIndex, BOOL value) {
    system->objs[objIndex].manualDraw = value;
}

void FieldG3DObjSystem_DrawObj(FieldG3DObjSystem *system, u16 objIndex) {
    FieldG3DObj *obj = &system->objs[objIndex];

    if (obj->state == G3DOBJ_STATE_READY && !obj->hidden) {
        if (obj->bboxCull == 1) {
            GFL_G3DSysDrawObjBBoxCull(obj->actor, &obj->transform);
        } else {
            GFL_G3DSysDrawObj(obj->actor, &obj->transform);
        }
    }
}

void FieldG3DObjSystem_SetObjectRedrawFlag(FieldG3DObjSystem *system, u16 objIndex, BOOL value) {
    system->objs[objIndex].redraw = value;
}

u32 FieldG3DObjSystem_GetObjectRedrawFlag(FieldG3DObjSystem *system, u16 objIndex) {
    return system->objs[objIndex].redraw;
}

void FieldG3DObjResGroup_SetupTextures(FieldG3DObjResGroup *group) {
    group->state = G3DOBJ_STATE_NONE;
    if (group->texture != NULL) {
        GFL_G3DResUploadTexData(group->texture);
    } else {
        GFL_G3DResUploadTexData(group->model);
    }
    group->state = G3DOBJ_STATE_READY;
}

void FieldG3DObjResGroup_LoadResources(FieldG3DObjResGroup *group, u16 heapId, FieldG3DObjResRequest *request,
                                       BOOL setupTextures, FieldColorPostFX *colorPostFx) {
    int i;

    if (request->mdlArc != NULL) {
        group->model = GFL_G3DSysReadArcToolResource(request->mdlArc, request->mdlDatID);
    }
    if (request->texArc != NULL) {
        group->texture = GFL_G3DSysReadArcToolResource(request->texArc, request->texDatID);
    }
    if (colorPostFx != NULL) {
        G3DResource *res = NULL;

        if (group->model != NULL) {
            res = group->model;
        }
        if (group->texture != NULL) {
            res = group->texture;
        }
        FieldColorPostFX_Apply(colorPostFx, res);
    }
    if (group->model != NULL || group->texture != NULL) {
        if (setupTextures == TRUE) {
            FieldG3DObjResGroup_SetupTextures(group);
        } else {
            group->state = G3DOBJ_STATE_PENDING;
        }
    }
    if (request->anmCount != 0) {
        group->animationCount = request->anmCount;
        MIi_CpuClear32(0, group->animations, sizeof(group->animations));
        for (i = 0; i < group->animationCount; i++) {
            group->animations[i] = GFL_G3DSysReadArcToolResource(request->anmArc, request->anmDatIDs[i]);
        }
    }
}

void FieldG3DObjResGroup_Free(FieldG3DObjResGroup *group) {
    int i;
    u8 state = group->state;

    group->state = G3DOBJ_STATE_NONE;
    if (state == G3DOBJ_STATE_READY) {
        if (group->texture != NULL) {
            GFL_G3DResFreeTexData(group->texture);
        } else if (group->model != NULL) {
            GFL_G3DResFreeTexData(group->model);
        }
    }
    if (group->texture != NULL) {
        GFL_G3DResFree(group->texture);
    }
    if (group->model != NULL) {
        GFL_G3DResFree(group->model);
    }
    if (group->animationCount != 0) {
        for (i = 0; i < group->animationCount; i++) {
            GFL_G3DResFree(group->animations[i]);
        }
    }
    MI_CpuFill(group, 0, sizeof(FieldG3DObjResGroup));
    group->state = G3DOBJ_STATE_READY;
}

void FieldG3DObj_Setup(FieldG3DObj *obj, FieldG3DObjResGroup *group, u16 resGroup, u16 modelIndex, u16 heapId,
                       const VecFx32 *position, BOOL initTransform) {
    int i;
    G3DResource *texture = NULL;
    G3DModel *model;

    obj->resGroup = resGroup;
    obj->modelIndex = modelIndex;
    if (group->state == G3DOBJ_STATE_READY) {
        texture = group->model;
        if (group->texture != NULL) {
            texture = group->texture;
        }
    }
    model = GFL_G3DMdlCreate(group->model, modelIndex, texture);
    if (group->animationCount != 0) {
        MIi_CpuClear32(0, obj->animations, sizeof(obj->animations));
        for (i = 0; i < group->animationCount; i++) {
            obj->animations[i] = GFL_G3DAnmCreate(model, group->animations[i], 0);
        }
    }
    obj->actor = GFL_G3DActorCreate(model, obj->animations, group->animationCount);
    if (group->animationCount != 0) {
        for (i = 0; i < group->animationCount; i++) {
            GFL_G3DActorBindAnm(obj->actor, i);
        }
    }
    if (position != NULL) {
        obj->transform.translation = *position;
    }
    if (initTransform) {
        obj->bboxCull = 1;
        obj->transform.scale.x = FX32_ONE;
        obj->transform.scale.y = FX32_ONE;
        obj->transform.scale.z = FX32_ONE;
        MTX_Identity33_(&obj->transform.rotation);
    }
    obj->state = G3DOBJ_STATE_READY;
}

void FieldG3DObj_Free(FieldG3DObj *obj, FieldG3DObjResGroup *group) {
    int i;
    G3DModel *model;

    if (obj->animations != NULL) {
        for (i = 0; i < group->animationCount; i++) {
            GFL_G3DAnmFree(obj->animations[i]);
        }
    }
    if (obj->actor != NULL) {
        model = GFL_G3DActorGetMdl(obj->actor);
        GFL_G3DActorFree(obj->actor);
        GFL_G3DMdlFree(model);
    }
    MI_CpuFill(obj, 0, sizeof(FieldG3DObj));
}

void FieldG3DObjResRequest_Clear(FieldG3DObjResRequest *request) {
    MIi_CpuClear32(0, request, sizeof(FieldG3DObjResRequest));
}

void FieldG3DObjResRequest_SetModel(FieldG3DObjResRequest *request, ArcTool *arc, u16 file) {
    request->mdlArc = arc;
    request->mdlDatID = file;
}

void FieldG3DObjResRequest_SetAnmArc(FieldG3DObjResRequest *request, ArcTool *arc) {
    request->anmArc = arc;
}

void FieldG3DObjResRequest_AddAnm(FieldG3DObjResRequest *request, u16 file) {
    request->anmDatIDs[request->anmCount] = file;
    request->anmCount++;
}
