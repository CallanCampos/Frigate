#include "types.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_scene_area.h"
#include "field/field_task.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"

// The event camera's animations and the camera animation controller (field_camera_anime.c, after the file name that
// its allocation passes). Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the
// layouts are ours

struct FieldCameraAnimationController {
    Field *field;
    FieldCamera *camera;
    const FieldCameraAnimation *animation;
    BOOL bound;
    u32 savedTransformType;
    BOOL savedBoundaryEnable;
    void *savedBind;
    BOOL hasSavedBind;
};

static void FieldCameraCalc_EvCamera(FieldCamera *camera, void *arg);
static u16 FieldCameraCalc_LerpAngle(const u16 *from, const u16 *to, u32 frames, u32 frame);
static s32 FieldCameraCalc_LerpScalar(const s32 *from, const s32 *to, s32 frames, s32 frame);
static void FieldCameraCalc_LerpVector(const VecFx32 *from, const VecFx32 *to, u32 frames, u32 frame, VecFx32 *out);
static void FieldCamera_ClearAnimatorFunc(FieldCamera *camera);
static void FieldCamera_GetAnimationCoords_(FieldCamera *camera, FieldCameraCoords *coords);
static FieldCameraAnimationController *FieldCameraAnimationController_Create(HeapID heapId);
static void FieldCameraAnimation_Free(void *animation);
static void FieldCameraAnimationController_SetField(FieldCameraAnimationController *controller, Field *field);
static void FieldCameraAnimationController_SetupCameraBind(FieldCameraAnimationController *controller);
static void FieldCameraAnimationController_AdjustTransform(FieldCameraAnimationController *controller);
static void FieldCameraAnimationController_ResetCameraBind(FieldCameraAnimationController *controller);
static void FieldCamera_CoordsRestoreFromLookAt(FieldCamera *camera);
static void FieldCamera_CoordsRestoreZoom(FieldCamera *camera);
static void FieldCameraAnimationController_CommitDstCoords(FieldCameraAnimationController *controller);
static void FieldCameraAnimationController_SetupTasks(FieldCameraAnimationController *controller);
static FieldCamera *FieldCameraAnimationEx_GetFieldCamera(FieldCameraAnimationController *controller);
static const FieldCameraAnimation *
FieldCameraAnimationController_GetAnimation(FieldCameraAnimationController *controller);

void FieldCameraAnm_EnsureInitDone(FieldCamera *camera) {
    if (camera) {
        FieldCameraEvAnimation *anim = &camera->ev;

        if (!anim->initDone) {
            anim->savedTransformType = camera->transformType;
            FieldCamera_ChangeTransformType(camera, 0);
            FieldCamera_BackupCoordsRealTime(camera, &anim->base);
            anim->initDone = TRUE;
        }
    }
}

void FieldCameraAnm_EVCameraEnd(FieldCamera *camera) {
    if (camera) {
        FieldCameraEvAnimation *anim = &camera->ev;

        if (anim->initDone) {
            FieldCamera_ChangeTransformType(camera, anim->savedTransformType);
            anim->initDone = FALSE;
        }
    }
}

static void FieldCameraAnm_SetAnimationCore(FieldCamera *camera, const FieldEvCameraAnimationSetup *setup, u16 frames) {
    if (camera) {
        FieldCameraEvAnimation *anim = &camera->ev;

        FieldCamera_ChangeTransformType(camera, 0);
        anim->end = setup->targetCoords;
        anim->frame = 0;
        anim->frames = frames;
        anim->flags = setup->flags;
        camera->calcFunc = FieldCameraCalc_EvCamera;
        anim->calcArg = NULL;
        anim->active = TRUE;
    }
}

void FieldCameraAnm_SetAnimation(FieldCamera *camera, const FieldEvCameraAnimationSetup *setup, u16 frames) {
    if (camera) {
        FieldCameraAnm_SetAnimationCore(camera, setup, frames);
        FieldCamera_BackupCoords(camera, &camera->ev.start);
    }
}

void FieldCameraAnm_SetAnimationRealTime(FieldCamera *camera, const FieldEvCameraAnimationSetup *setup, u16 frames) {
    if (camera) {
        FieldCameraAnm_SetAnimationCore(camera, setup, frames);
        FieldCamera_BackupCoordsRealTime(camera, &camera->ev.start);
    }
}

void FieldCameraAnm_SetReturnAnimation(FieldCamera *camera, const FieldEvCameraAnimationFlags *flags, u16 frames) {
    if (camera) {
        FieldCameraEvAnimation *anim = &camera->ev;

        if (anim->initDone && frames) {
            FieldCamera_ChangeTransformType(camera, 0);
            FieldCamera_BackupCoords(camera, &anim->start);
            anim->end = anim->base;
            anim->frame = 0;
            anim->frames = frames;
            anim->flags = *flags;
            camera->calcFunc = FieldCameraCalc_EvCamera;
            anim->calcArg = NULL;
            anim->active = TRUE;
        }
    }
}

void FieldCameraAnm_SetLoadDefaultsAnimation(FieldCamera *camera, u16 frames) {
    FieldCameraDefaults defaults;

    if (camera) {
        FieldCameraEvAnimation *anim = &camera->ev;

        if (anim->initDone && frames) {
            const VecFx32 *target;

            FieldCamera_ChangeTransformType(camera, 0);
            FieldCamera_BackupCoords(camera, &anim->start);
            FieldCamera_ReadDefaults(camera, &defaults);
            anim->end.pitch = defaults.pitch;
            anim->end.yaw = defaults.yaw;
            anim->end.distance = defaults.zoom << 12;
            anim->end.fov = defaults.fov;
            if (defaults.bind) {
                anim->end.extraTranslation = defaults.target;
                target = camera->refBind;
            } else {
                anim->end.extraTranslation.x = 0;
                anim->end.extraTranslation.y = 0;
                anim->end.extraTranslation.z = 0;
                target = &defaults.target;
            }
            anim->end.targetPos = *target;
            anim->frame = 0;
            anim->frames = frames;
            anim->flags.animateExtraTranslation = TRUE;
            anim->flags.animatePitch = TRUE;
            anim->flags.animateYaw = TRUE;
            anim->flags.animateTargetDistance = TRUE;
            anim->flags.animateFOV = TRUE;
            anim->flags.animateTargetPos = TRUE;
            camera->calcFunc = FieldCameraCalc_EvCamera;
            anim->calcArg = NULL;
            anim->active = TRUE;
        }
    }
}

static void FieldCameraCalc_EvCamera(FieldCamera *camera, void *arg) {
    FieldCameraEvAnimation *anim = &camera->ev;

    if (anim->active) {
        FieldEvCameraAnimationCoords *start;
        FieldEvCameraAnimationCoords *end;

        anim->frame++;
        start = &anim->start;
        end = &anim->end;
        if (anim->flags.animatePitch) {
            camera->pitch = FieldCameraCalc_LerpAngle(&start->pitch, &end->pitch, anim->frames, anim->frame);
        }
        if (anim->flags.animateYaw) {
            camera->yaw = FieldCameraCalc_LerpAngle(&start->yaw, &end->yaw, anim->frames, anim->frame);
        }
        if (anim->flags.animateTargetPos) {
            FieldCameraCalc_LerpVector(&start->targetPos, &end->targetPos, anim->frames, anim->frame, &camera->target);
        }
        if (anim->flags.animateFOV) {
            camera->fov = FieldCameraCalc_LerpAngle(&start->fov, &end->fov, anim->frames, anim->frame);
            GFL_G3DCameraPerspectiveSetFOVSin(camera->g3dCamera, FX_SinIdx(camera->fov));
            GFL_G3DCameraPerspectiveSetFOVCos(camera->g3dCamera, FX_CosIdx(camera->fov));
        }
        if (anim->flags.animateTargetDistance) {
            camera->zoom = FieldCameraCalc_LerpScalar(&start->distance, &end->distance, anim->frames, anim->frame);
        }
        if (anim->flags.animateExtraTranslation) {
            FieldCameraCalc_LerpVector(&start->extraTranslation, &end->extraTranslation, anim->frames, anim->frame,
                                       &camera->targetOffset);
        }
        if (anim->frame >= anim->frames) {
            anim->active = FALSE;
            FieldCamera_ClearAnimatorFunc(camera);
        }
    }
}

static void FieldCameraCalc_LerpVector(const VecFx32 *from, const VecFx32 *to, u32 frames, u32 frame, VecFx32 *out) {
    if (!frames) {
        *out = *to;
    }
    out->x = FieldCameraCalc_LerpScalar(&from->x, &to->x, frames, frame);
    out->y = FieldCameraCalc_LerpScalar(&from->y, &to->y, frames, frame);
    out->z = FieldCameraCalc_LerpScalar(&from->z, &to->z, frames, frame);
}

static s32 FieldCameraCalc_LerpScalar(const s32 *from, const s32 *to, s32 frames, s32 frame) {
    s32 start = *from;
    s32 end = *to;
    s32 diff;

    if (frames == frame) {
        return end;
    }
    diff = end - start;
    return start + diff * frame / frames;
}

static u16 FieldCameraCalc_LerpAngle(const u16 *from, const u16 *to, u32 frames, u32 frame) {
    u16 start;
    u16 forward;
    u16 backward;

    if (!frames) {
        return *to;
    }
    start = *from;
    forward = *to - start;
    backward = 0x10000 - forward;
    if (forward <= backward) {
        return start + (u16)(forward * frame / frames);
    }
    return start - (u16)(backward * frame / frames);
}

static void FieldCamera_ClearAnimatorFunc(FieldCamera *camera) {
    camera->calcFunc = NULL;
    camera->ev.calcArg = NULL;
}

BOOL FieldCamera_IsAnimating(FieldCamera *camera) {
    FieldCameraEvAnimation *anim = &camera->ev;

    if (camera->calcFunc && anim->active) {
        return TRUE;
    }
    return FALSE;
}

void FieldCamera_GetAnimationCoords(FieldCamera *camera, FieldCameraCoords *coords) {
    FieldCamera_GetAnimationCoords_(camera, coords);
}

FieldCameraAnimationController *Field_CreateCameraAnimationController(Field *field) {
    FieldCameraAnimationController *controller = FieldCameraAnimationController_Create(Field_GetHeapID(field));

    FieldCameraAnimationController_SetField(controller, field);
    return controller;
}

void FieldCameraAnimationController_FreeAnimation(FieldCameraAnimationController *controller) {
    FieldCameraAnimation_Free(controller);
}

void FieldCameraAnimationController_SetAnimation(FieldCameraAnimationController *controller,
                                                 const FieldCameraAnimation *animation) {
    controller->animation = animation;
}

void FieldCameraAnimationController_PrepareCamera(FieldCameraAnimationController *controller) {
    FieldCameraAnimationController_AdjustTransform(controller);
    FieldCamera_CoordsRestoreFromLookAt(controller->camera);
    FieldCamera_CoordsRestoreZoom(controller->camera);
    FieldCameraAnimationController_SetupCameraBind(controller);
}

void FieldCameraAnimationController_CommitSrcCoords(FieldCameraAnimationController *controller) {
    FieldCamera *camera = FieldCameraAnimationEx_GetFieldCamera(controller);
    const FieldCameraCoords *src = &FieldCameraAnimationController_GetAnimation(controller)->src;

    FieldCamera_CoordsSetPitch(camera, src->pitch);
    FieldCamera_CoordsSetYaw(camera, src->yaw);
    FieldCamera_CoordsSetZoom(camera, src->distance);
    FieldCamera_CoordsSetTarget(camera, &src->target);
    FieldCamera_CoordsSetTargetOffset(camera, &src->offset);
}

void FieldCameraAnimationController_StartAnimation(FieldCameraAnimationController *controller) {
    if (FieldCameraAnimationController_GetAnimation(controller)->frames == 0) {
        FieldCameraAnimationController_CommitDstCoords(controller);
        return;
    }
    FieldCameraAnimationController_SetupTasks(controller);
}

void FieldCameraAnimationController_FinishCamera(FieldCameraAnimationController *controller) {
    if (controller->bound) {
        FieldCameraAnimationController_ResetCameraBind(controller);
    }
}

static FieldCameraAnimationController *FieldCameraAnimationController_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(FieldCameraAnimationController), FALSE, "field_camera_anime.c", 238);
}

static void FieldCameraAnimation_Free(void *animation) {
    GFL_HeapFree(animation);
}

static void FieldCameraAnimationController_SetField(FieldCameraAnimationController *controller, Field *field) {
    MI_CpuFill8(controller, 0, sizeof(FieldCameraAnimationController));
    controller->field = field;
    controller->camera = Field_GetCameraSystem(field);
    controller->animation = NULL;
    controller->bound = FALSE;
    controller->savedBoundaryEnable = FALSE;
    controller->savedBind = NULL;
    controller->hasSavedBind = FALSE;
}

static void FieldCameraAnimationController_SetupCameraBind(FieldCameraAnimationController *controller) {
    Field *field = controller->field;
    FieldCamera *camera = controller->camera;
    const FieldCameraAnimation *animation = FieldCameraAnimationController_GetAnimation(controller);

    if (Field_GetResolvedControllerTypeID(field) == 1) {
        FieldNoGridMapper_SetCameraAreaEnabled(Field_GetNoGridMapper(field), FALSE);
    }
    if (animation->unk48 && FieldCamera_GetBind(controller->camera)) {
        controller->hasSavedBind = TRUE;
        controller->savedBind = FieldCamera_GetBind(camera);
        FieldCamera_ClearBind(camera);
    }
    if (animation->unk4C) {
        controller->savedBoundaryEnable = FieldCamera_IsUseBoundaryEnable(camera);
        FieldCamera_SetUseBoundaryEnable(camera, FALSE);
    }
    SetFieldSceneAreaCameraAreaEnable(Field_GetSceneArea(field), FALSE);
    controller->bound = TRUE;
}

static void FieldCameraAnimationController_AdjustTransform(FieldCameraAnimationController *controller) {
    FieldCamera *camera = controller->camera;

    controller->savedTransformType = FieldCamera_GetTransformType(camera);
    FieldCamera_ChangeTransformType(camera, 0);
}

static void FieldCameraAnimationController_ResetCameraBind(FieldCameraAnimationController *controller) {
    Field *field = controller->field;
    FieldCamera *camera = controller->camera;
    const FieldCameraAnimation *animation = FieldCameraAnimationController_GetAnimation(controller);

    SetFieldSceneAreaCameraAreaEnable(Field_GetSceneArea(field), TRUE);
    if (animation->unk4C) {
        FieldCamera_SetUseBoundaryEnable(camera, controller->savedBoundaryEnable);
    }
    if (controller->hasSavedBind) {
        FieldCamera_SetBind(camera, controller->savedBind);
    }
    FieldCamera_ChangeTransformType(camera, controller->savedTransformType);
    if (Field_GetResolvedControllerTypeID(field) == 1) {
        FieldNoGridMapper_SetCameraAreaEnabled(Field_GetNoGridMapper(field), TRUE);
    }
}

static void FieldCamera_CoordsRestoreFromLookAt(FieldCamera *camera) {
    VecFx32 target;
    VecFx32 eye;
    VecFx32 dir;
    fx32 distance;
    u16 pitch;
    u16 yaw;

    FieldCamera_GetEye(camera, &eye);
    FieldCamera_GetTarget(camera, &target);
    VEC_Subtract(&eye, &target, &dir);
    FieldCamera_CoordsDerive(&dir, &pitch, &yaw, &distance);
    FieldCamera_CoordsSetZoom(camera, distance);
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetYaw(camera, yaw);
}

static void FieldCamera_CoordsRestoreZoom(FieldCamera *camera) {
    VecFx32 target;

    FieldCamera_GetTarget(camera, &target);
    FieldCamera_CoordsSetTarget(camera, &target);
}

static void FieldCamera_GetAnimationCoords_(FieldCamera *camera, FieldCameraCoords *coords) {
    VecFx32 target;
    VecFx32 eye;
    VecFx32 dir;

    FieldCamera_GetEye(camera, &eye);
    FieldCamera_GetTarget(camera, &target);
    VEC_Subtract(&eye, &target, &dir);
    FieldCamera_CoordsDerive(&dir, &coords->pitch, &coords->yaw, &coords->distance);
    FieldCamera_GetTarget(camera, &coords->target);
    FieldCamera_CoordsGetTargetOffset(camera, &coords->offset);
}

static void FieldCameraAnimationController_CommitDstCoords(FieldCameraAnimationController *controller) {
    FieldCamera *camera = controller->camera;
    const FieldCameraAnimation *animation = FieldCameraAnimationController_GetAnimation(controller);
    const FieldCameraCoords *dst = &animation->dst;

    FieldCamera_CoordsSetPitch(camera, dst->pitch);
    FieldCamera_CoordsSetYaw(camera, dst->yaw);
    FieldCamera_CoordsSetZoom(camera, dst->distance);
    FieldCamera_CoordsSetTarget(camera, &dst->target);
    FieldCamera_CoordsSetTargetOffset(camera, &dst->offset);
}

static void FieldCameraAnimationController_SetupTasks(FieldCameraAnimationController *controller) {
    Field *field = controller->field;
    const FieldCameraAnimation *animation = FieldCameraAnimationController_GetAnimation(controller);
    u32 frames = animation->frames;
    const FieldCameraCoords *dst = &animation->dst;
    FieldTask *pitchTask;
    FieldTask *yawTask;
    FieldTask *zoomTask;
    FieldTask *targetTask;
    FieldTask *offsetTask;
    FieldTaskManager *manager;

    pitchTask = FieldCameraMoveTaskPitch_Callback(field, frames, dst->pitch);
    yawTask = FieldCameraMoveTaskYaw_Callback(field, frames, dst->yaw);
    zoomTask = FieldCameraMoveTaskZoom_Create(field, frames, dst->distance);
    targetTask = FieldCameraMoveTaskTarget_Create(field, frames, &animation->src.target, &dst->target);
    offsetTask = FieldCameraMoveTaskTargetOffs_Create(field, frames, &dst->offset);
    manager = Field_GetTaskManager(field);
    FieldTaskManager_AddTask(manager, pitchTask, 0);
    FieldTaskManager_AddTask(manager, yawTask, 0);
    FieldTaskManager_AddTask(manager, zoomTask, 0);
    FieldTaskManager_AddTask(manager, targetTask, 0);
    FieldTaskManager_AddTask(manager, offsetTask, 0);
}

static FieldCamera *FieldCameraAnimationEx_GetFieldCamera(FieldCameraAnimationController *controller) {
    return controller->camera;
}

static const FieldCameraAnimation *
FieldCameraAnimationController_GetAnimation(FieldCameraAnimationController *controller) {
    return controller->animation;
}
