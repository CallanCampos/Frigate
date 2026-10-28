// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "field/field_camera.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/math.h"

// The field's camera (field_camera.c, after the file name that its allocations pass): where its target and eye are,
// how it follows what it is bound to, the zone's boundaries it keeps to, and the delay of its following. The layouts
// are in field/field_camera.h, but for the delay's, which is ours

// The last targets and eyes, in a ring, that the camera puts back some frames later
struct FieldCameraDelay {
    int count;
    int head;
    int tail;
    int maxDelay;
    BOOL filled;
    BOOL delayX;
    BOOL delayY;
    BOOL delayZ;
    VecFx32 *targets;
    VecFx32 *eyes;
    BOOL active;
    BOOL finishing;
};

static void FieldCamera_CoordsLoadDefaults(FieldCamera *camera, const VecFx32 *target);
static void FieldCamera_SetZNear(FieldCamera *camera, fx32 zNear);
static void FieldCamera_SetZFar(FieldCamera *camera, fx32 zFar);
static void FieldCameraDelay_Create(int count, int maxDelay, int flags, u32 heapId, FieldCamera *camera);
static void FieldCameraDelay_Update(FieldCameraDelay *delay, const VecFx32 *target, const VecFx32 *eye,
                                    VecFx32 *outTarget, VecFx32 *outEye);
static void FieldCameraDelay_Disable(FieldCameraDelay *delay);
static void FieldCamera_FreeDelayManager(FieldCamera *camera);
static void FieldCamera_ApplyDelay(FieldCamera *camera);
static void FieldCamera_CallTransformCalcFunc(FieldCamera *camera, u16 heldKeys);
static void FieldCameraApply_SetInitTgtBase(FieldCamera *camera);
static BOOL FieldCameraApply_SetInitEyeBase(FieldCamera *camera);
static void FieldCameraApply_MakeEyeByTarget(FieldCamera *camera);
static void FieldCameraApply_MakeTargetByEye(FieldCamera *camera);
static void FieldCameraBoundary_ApplyTarget(FieldCameraBoundary *bound, VecFx32 *vec);
static void FieldCameraBoundary_ApplyEye(FieldCameraBoundary *bound, VecFx32 *vec);
static void FieldCameraBoundary_LoadFile(FieldCamera *camera, const u32 *file);
static void FieldCamera_UpdateViewAngles(FieldCamera *camera, const VecFx32 *target, const VecFx32 *eye);
static void FieldCamera_RefineDerivedAngles(const VecFx32 *dir, u16 yaw, u16 pitch, u16 *outYaw, u16 *outPitch);
static void FieldCamera_CoordsDerive_(const VecFx32 *vec, u16 *pitch, u16 *yaw, fx32 *zoom);
static void FieldCamera_CoordsSetFromDir(FieldCamera *camera, const VecFx32 *vec);
static BOOL FieldCameraBoundary_RestrictRect(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out);
static BOOL FieldCameraBoundary_RestrictCircle(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out);
// The side of a rectangle that a point inside it is pushed out through
enum {
    RECT_SIDE_MIN_X,
    RECT_SIDE_MAX_X,
    RECT_SIDE_MIN_Z,
    RECT_SIDE_MAX_Z,
};

static BOOL FieldCameraBoundary_RepealRect(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out);
static void FieldCameraApply_TransRot(const VecFx32 *base, VecFx32 *out, u16 yaw, u16 pitch, fx32 zoom);
static void FieldCameraApply_SetAllInit(FieldCamera *camera);
static void FieldCameraApply_FinishLookAtMatrix(FieldCamera *camera);
static void FieldCameraTransform_TARGET_REL(FieldCamera *camera, u16 heldKeys);
static void FieldCameraTransform_CAMERA_REL(FieldCamera *camera, u16 heldKeys);
static void FieldCameraTransform_FIXED_ALL(FieldCamera *camera, u16 heldKeys);
static void FieldCameraTransform_TARGET_REL_Init(FieldCamera *camera);
static void FieldCameraTransform_CAMERA_REL_Init(FieldCamera *camera);
static void FieldCameraTransform_FIXED_ALL_Init(FieldCamera *camera);
static void FieldCameraTransform_TARGET_REL_ConvertTo(FieldCamera *camera);
static void FieldCameraTransform_CAMERA_REL_ConvertTo(FieldCamera *camera);
static void FieldCameraTransform_FIXED_ALL_ConvertTo(FieldCamera *camera);

static void (*const FIELD_CAMERA_TRANSFORM_FUNCS[])(FieldCamera *camera, u16 heldKeys) = {
    FieldCameraTransform_TARGET_REL,
    FieldCameraTransform_CAMERA_REL,
    FieldCameraTransform_FIXED_ALL,
};

static void (*FIELD_CAMERA_TRANSFORM_INIT_FUNCS[])(FieldCamera *camera) = {
    FieldCameraTransform_TARGET_REL_Init,
    FieldCameraTransform_CAMERA_REL_Init,
    FieldCameraTransform_FIXED_ALL_Init,
};
static void (*FIELD_CAMERA_TRANSFORM_CONVERT_FUNCS[])(FieldCamera *camera) = {
    FieldCameraTransform_TARGET_REL_ConvertTo,
    FieldCameraTransform_CAMERA_REL_ConvertTo,
    FieldCameraTransform_FIXED_ALL_ConvertTo,
};
static BOOL (*MM_CAMBOUND_FUNCS[])(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out) = {
    NULL,
    FieldCameraBoundary_RestrictRect,
    FieldCameraBoundary_RestrictCircle,
    FieldCameraBoundary_RepealRect,
};

FieldCamera *FieldCamera_Create(u32 cameraIndex, u32 transformType, G3DCamera *g3dCamera, const VecFx32 *target,
                                HeapID heapId) {
    FieldCamera *camera = GFL_HeapAllocate(heapId, sizeof(FieldCamera), TRUE, "field_camera.c", 128);
    camera->defaultsIndex = cameraIndex;
    camera->transformType = transformType;
    camera->g3dCamera = g3dCamera;
    camera->eyeBind = NULL;
    camera->heapId = heapId;
    camera->eye.x = 0;
    camera->eye.y = 0;
    camera->eye.z = 0;
    camera->target.x = 0;
    camera->target.y = 0;
    camera->target.z = 0;
    camera->targetBase.x = 0;
    camera->targetBase.y = 0;
    camera->targetBase.z = 0;
    camera->eyeOffset.x = 0;
    camera->eyeOffset.y = 0;
    camera->eyeOffset.z = 0;
    camera->yaw = 0;
    camera->pitch = 0;
    camera->zoom = 120;
    camera->useBoundary = TRUE;
    if (GFL_G3DCameraGetProjectionType(g3dCamera) == G3DCAM_PROJECTION_PERSPECTIVE) {
        fx32 fovCos;
        GFL_G3DCameraPerspectiveGetFOVCos(g3dCamera, &fovCos);
        camera->fov = FX_AcosIdx(fovCos);
    }
    camera->defaultsArc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_CAMERA_DEFAULT, camera->heapId);
    camera->boundaryArc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_CAMERA_MAP_BOUNDARY, camera->heapId);
    camera->boundaryId = 0xffff;
    FieldCamera_CoordsLoadDefaults(camera, target);
    FieldCameraDelay_Create(7, 6, 2, heapId, camera);
    camera->calcFunc = NULL;
    return camera;
}

void FieldCamera_Free(FieldCamera *camera) {
    GFL_ArcToolFree(camera->defaultsArc);
    GFL_ArcToolFree(camera->boundaryArc);
    FieldCamera_FreeDelayManager(camera);
    GFL_HeapFree(camera);
}

void FieldCamera_CalcTransform(FieldCamera *camera, u16 heldKeys) {
    if (camera->calcFunc != NULL) {
        camera->calcFunc(camera, camera->ev.calcArg);
    }
    FieldCamera_CallTransformCalcFunc(camera, heldKeys);
}

void FieldCamera_SetTransformType(FieldCamera *camera, u32 type) {
    camera->transformType = type;
    FIELD_CAMERA_TRANSFORM_INIT_FUNCS[type](camera);
}

u32 FieldCamera_GetTransformType(FieldCamera *camera) {
    return camera->transformType;
}

void FieldCamera_ChangeTransformType(FieldCamera *camera, u32 type) {
    if (camera->transformType != type) {
        FIELD_CAMERA_TRANSFORM_CONVERT_FUNCS[type](camera);
        FieldCamera_SetTransformType(camera, type);
    }
}

void FieldCamera_LoadDefaults(FieldCamera *camera) {
    FieldCamera_SetTransformType(camera, 0);
    FieldCamera_CoordsLoadDefaults(camera, camera->refBind);
    camera->useBoundary = TRUE;
}

void FieldCamera_SetDefaultsIndex(FieldCamera *camera, u32 index) {
    camera->defaultsIndex = index;
    FieldCamera_LoadDefaults(camera);
}

u32 FieldCamera_GetDefaultsIndex(FieldCamera *camera) {
    return camera->defaultsIndex;
}

static void FieldCamera_CoordsLoadDefaults(FieldCamera *camera, const VecFx32 *target) {
    FieldCameraDefaults defaults;
    FieldCamera_ReadDefaults(camera, &defaults);
    camera->zoom = defaults.zoom << 12;
    camera->pitch = defaults.pitch;
    camera->yaw = defaults.yaw;
    camera->fov = defaults.fov;
    FieldCamera_SetZFar(camera, defaults.zFar);
    FieldCamera_SetZNear(camera, defaults.zNear);
    switch (defaults.swapMode) {
    case GX_BUFFERMODE_Z:
        GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
        break;
    case GX_BUFFERMODE_W:
        GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_W);
        break;
    }
    if (GFL_G3DCameraGetProjectionType(camera->g3dCamera) == G3DCAM_PROJECTION_PERSPECTIVE) {
        GFL_G3DCameraPerspectiveSetFOVSin(camera->g3dCamera, FX_SinIdx(camera->fov));
        GFL_G3DCameraPerspectiveSetFOVCos(camera->g3dCamera, FX_CosIdx(camera->fov));
    }
    if (defaults.bind) {
        camera->refBind = target;
        camera->bind = target;
        camera->target = *target;
        camera->targetOffset = defaults.target;
    } else {
        camera->refBind = NULL;
        camera->bind = NULL;
        camera->target = defaults.target;
        camera->targetOffset.x = 0;
        camera->targetOffset.y = 0;
        camera->targetOffset.z = 0;
    }
}

void FieldCamera_ReadDefaults(FieldCamera *camera, FieldCameraDefaults *defaults) {
    GFL_ArcToolGetDataLength(camera->defaultsArc, 0);
    GFL_ArcToolReadRange(camera->defaultsArc, 0, camera->defaultsIndex * sizeof(FieldCameraDefaults),
                         sizeof(FieldCameraDefaults), defaults);
}

static void FieldCameraApply_SetInitTgtBase(FieldCamera *camera) {
    if (camera->bind != NULL) {
        camera->target = *camera->bind;
    }
}

static BOOL FieldCameraApply_SetInitEyeBase(FieldCamera *camera) {
    if (camera->eyeBind != NULL) {
        camera->eye = *camera->eyeBind;
        return TRUE;
    }
    return FALSE;
}

static void FieldCameraApply_TransRot(const VecFx32 *base, VecFx32 *out, u16 yaw, u16 pitch, fx32 zoom) {
    VecFx32 dir;
    fx16 sinYaw = FX_SinIdx(yaw);
    fx16 cosYaw = FX_CosIdx(yaw);
    fx32 sinPitch = FX_SinIdx(pitch);
    fx16 cosPitch = FX_CosIdx(pitch);
    if (cosPitch < 0) {
        cosPitch = -cosPitch;
    }
    if (cosPitch < 0x200) {
        cosPitch = 0x200;
    }
    dir.x = FX_Mul(sinYaw, cosPitch);
    dir.y = sinPitch;
    dir.z = FX_Mul(cosYaw, cosPitch);
    VEC_Normalize(&dir, &dir);
    VEC_MultAdd(zoom, &dir, base, &dir);
    *out = dir;
}

static void FieldCameraApply_MakeEyeByTarget(FieldCamera *camera) {
    int i;
    camera->targetActual = camera->target;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyTarget(&camera->bounds[i], &camera->targetActual);
        }
    }
    FieldCameraApply_TransRot(&camera->targetActual, &camera->eye, camera->yaw, camera->pitch, camera->zoom);
    camera->eyeActual = camera->eye;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyEye(&camera->bounds[i], &camera->eyeActual);
        }
    }
}

static void FieldCameraApply_MakeTargetByEye(FieldCamera *camera) {
    int i;
    camera->eyeActual = camera->eye;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyEye(&camera->bounds[i], &camera->eyeActual);
        }
    }
    FieldCameraApply_TransRot(&camera->eyeActual, &camera->target, camera->yaw, camera->pitch, camera->zoom);
    camera->targetActual = camera->target;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyTarget(&camera->bounds[i], &camera->targetActual);
        }
    }
}

static void FieldCameraApply_SetAllInit(FieldCamera *camera) {
    int i;
    camera->eyeActual = camera->eye;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyEye(&camera->bounds[i], &camera->eyeActual);
        }
    }
    camera->targetActual = camera->target;
    if (camera->useBoundary) {
        for (i = 0; i < camera->boundCount; i++) {
            FieldCameraBoundary_ApplyTarget(&camera->bounds[i], &camera->targetActual);
        }
    }
}

static void FieldCameraApply_FinishLookAtMatrix(FieldCamera *camera) {
    VecFx32 target;
    VecFx32 eye;
    VEC_Add(&camera->targetActual, &camera->targetOffset, &target);
    VEC_Add(&camera->eyeActual, &camera->eyeOffset, &eye);
    GFL_G3DCameraSetLookatTarget(camera->g3dCamera, &target);
    GFL_G3DCameraSetLookatPos(camera->g3dCamera, &eye);
    FieldCamera_ApplyDelay(camera);
    FieldCamera_UpdateViewAngles(camera, &target, &camera->eyeActual);
}

static void FieldCamera_CallTransformCalcFunc(FieldCamera *camera, u16 heldKeys) {
    FIELD_CAMERA_TRANSFORM_FUNCS[camera->transformType](camera, heldKeys);
}

static void FieldCameraTransform_TARGET_REL(FieldCamera *camera, u16 heldKeys) {
    FieldCameraApply_SetInitTgtBase(camera);
    FieldCameraApply_MakeEyeByTarget(camera);
    FieldCameraApply_FinishLookAtMatrix(camera);
}

static void FieldCameraTransform_CAMERA_REL(FieldCamera *camera, u16 heldKeys) {
    FieldCameraApply_SetInitEyeBase(camera);
    FieldCameraApply_MakeTargetByEye(camera);
    FieldCameraApply_FinishLookAtMatrix(camera);
}

static void FieldCameraTransform_FIXED_ALL(FieldCamera *camera, u16 heldKeys) {
    FieldCameraApply_SetInitTgtBase(camera);
    FieldCameraApply_SetInitEyeBase(camera);
    FieldCameraApply_SetAllInit(camera);
    FieldCameraApply_FinishLookAtMatrix(camera);
}

static void FieldCameraTransform_TARGET_REL_Init(FieldCamera *camera) {
    camera->eyeBind = NULL;
}

static void FieldCameraTransform_CAMERA_REL_Init(FieldCamera *camera) {
    camera->bind = NULL;
}

static void FieldCameraTransform_FIXED_ALL_Init(FieldCamera *camera) {
    camera->pitch = 0;
    camera->yaw = 0;
    camera->zoom = 0;
}

static void FieldCamera_CoordsDerive_(const VecFx32 *vec, u16 *pitch, u16 *yaw, fx32 *zoom) {
    VecFx32 norm;
    VecFx32 flat;
    u16 angles[2];
    fx32 mag = VEC_Mag(vec);
    VEC_Normalize(vec, &norm);
    flat.x = norm.x;
    flat.y = 0;
    flat.z = norm.z;
    angles[1] = FX_Atan2Idx(norm.y, VEC_Mag(&flat));
    angles[0] = FX_Atan2Idx(norm.x, norm.z);
    FieldCamera_RefineDerivedAngles(&norm, angles[0], angles[1], &angles[0], &angles[1]);
    *pitch = angles[1];
    *yaw = angles[0];
    *zoom = mag;
}

static void FieldCamera_CoordsSetFromDir(FieldCamera *camera, const VecFx32 *vec) {
    fx32 zoom;
    u16 angles[2];
    FieldCamera_CoordsDerive_(vec, &angles[1], &angles[0], &zoom);
    camera->pitch = angles[1];
    camera->yaw = angles[0];
    camera->zoom = zoom;
}

static void FieldCameraTransform_TARGET_REL_ConvertTo(FieldCamera *camera) {
    VecFx32 dir;
    if (camera->transformType == 1) {
        camera->yaw += 0x8000;
        camera->pitch += 0x8000;
    } else {
        VEC_Subtract(&camera->eye, &camera->targetActual, &dir);
        FieldCamera_CoordsSetFromDir(camera, &dir);
    }
}

static void FieldCameraTransform_CAMERA_REL_ConvertTo(FieldCamera *camera) {
    VecFx32 dir;
    if (camera->transformType == 0) {
        camera->yaw += 0x8000;
        camera->pitch += 0x8000;
    } else {
        VEC_Subtract(&camera->target, &camera->eyeActual, &dir);
        FieldCamera_CoordsSetFromDir(camera, &dir);
    }
}

static void FieldCameraTransform_FIXED_ALL_ConvertTo(FieldCamera *camera) {
    if (camera->transformType == 0) {
        FieldCameraApply_SetInitTgtBase(camera);
        FieldCameraApply_MakeEyeByTarget(camera);
    } else if (camera->transformType == 1) {
        FieldCameraApply_SetInitEyeBase(camera);
        FieldCameraApply_MakeTargetByEye(camera);
    }
}

static void FieldCamera_UpdateViewAngles(FieldCamera *camera, const VecFx32 *target, const VecFx32 *eye) {
    VecFx32 dir;
    VEC_Subtract(eye, target, &dir);
    VEC_Normalize(&dir, &dir);
    camera->angleYaw = FX_Atan2Idx(dir.x, dir.z);
    {
        VecFx32 up = { 0, FX32_ONE, 0 };
        VecFx32 flat;
        fx32 upDot;
        fx32 flatDot;
        u16 pitch;
        upDot = VEC_DotProduct(&up, &dir);
        flat = dir;
        pitch = 0;
        flat.y = 0;
        VEC_Normalize(&flat, &flat);
        flatDot = VEC_DotProduct(&flat, &dir);
        if (flatDot != 0) {
            pitch = FX_Atan2Idx(upDot, flatDot);
        }
        camera->anglePitch = pitch;
    }
}

// Moves a yaw and pitch closer to dir by a fraction of a degree: how far dir lies from the direction they give,
// against how far one degree moves that direction, in the quadrant of each angle
static void FieldCamera_RefineDerivedAngles(const VecFx32 *dir, u16 yaw, u16 pitch, u16 *outYaw, u16 *outPitch) {
    VecFx16 base = { 0, 0, 0 };
    VecFx16 minus = { 0, 0, 0 };
    VecFx16 plus = { 0, 0, 0 };
    fx16 sinYaw;
    fx16 cosYaw;
    fx16 cosPitch;
    fx16 sinPitchMinus;
    fx16 sinPitchPlus;
    *outYaw = yaw;
    *outPitch = pitch;
    sinYaw = FX_SinIdx(yaw);
    cosYaw = FX_CosIdx(yaw);
    cosPitch = FX_CosIdx(pitch);
    base.x = FX_Mul(sinYaw, cosPitch);
    base.y = FX_SinIdx(pitch);
    base.z = FX_Mul(cosYaw, cosPitch);
    minus.x = FX_Mul(FX_SinIdx(yaw - DEG_TO_IDX(1)), cosPitch);
    minus.z = FX_Mul(FX_CosIdx(yaw - DEG_TO_IDX(1)), cosPitch);
    sinPitchMinus = FX_SinIdx(pitch - DEG_TO_IDX(1));
    plus.x = FX_Mul(FX_SinIdx(yaw + DEG_TO_IDX(1)), cosPitch);
    plus.z = FX_Mul(FX_CosIdx(yaw + DEG_TO_IDX(1)), cosPitch);
    sinPitchPlus = FX_SinIdx(pitch + DEG_TO_IDX(1));
    if (yaw < 0x4000) {
        if (base.x < dir->x && base.z > dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw - (u16)((dir->x - base.x) * DEG_TO_IDX(1) / (minus.x - base.x));
            } else {
                yaw = yaw - (u16)((base.z - dir->z) * DEG_TO_IDX(1) / (base.z - minus.z));
            }
        } else if (base.x > dir->x && base.z < dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw + (u16)((base.x - dir->x) * DEG_TO_IDX(1) / (base.x - plus.x));
            } else {
                yaw = yaw + (u16)((dir->z - base.z) * DEG_TO_IDX(1) / (plus.z - base.z));
            }
        }
    } else if (yaw < 0x8000) {
        if (base.x > dir->x && base.z > dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw - (u16)((base.x - dir->x) * DEG_TO_IDX(1) / (base.x - minus.x));
            } else {
                yaw = yaw - (u16)((base.z - dir->z) * DEG_TO_IDX(1) / (base.z - minus.z));
            }
        } else if (base.x < dir->x && base.z < dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw + (u16)((dir->x - base.x) * DEG_TO_IDX(1) / (plus.x - base.x));
            } else {
                yaw = yaw + (u16)((dir->z - base.z) * DEG_TO_IDX(1) / (plus.z - base.z));
            }
        }
    } else if (yaw < 0xc000) {
        if (base.x > dir->x && base.z < dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw - (u16)((base.x - dir->x) * DEG_TO_IDX(1) / (base.x - minus.x));
            } else {
                yaw = yaw - (u16)((dir->z - base.z) * DEG_TO_IDX(1) / (minus.z - base.z));
            }
        } else if (base.x < dir->x && base.z > dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw + (u16)((dir->x - base.x) * DEG_TO_IDX(1) / (plus.x - base.x));
            } else {
                yaw = yaw + (u16)((base.z - dir->z) * DEG_TO_IDX(1) / (base.z - plus.z));
            }
        }
    } else {
        if (base.x < dir->x && base.z < dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw - (u16)((dir->x - base.x) * DEG_TO_IDX(1) / (minus.x - base.x));
            } else {
                yaw = yaw - (u16)((dir->z - base.z) * DEG_TO_IDX(1) / (minus.z - base.z));
            }
        } else if (base.x > dir->x && base.z > dir->z) {
            if (MATH_ABS(base.x - dir->x) >= MATH_ABS(base.z - dir->z)) {
                yaw = yaw + (u16)((base.x - dir->x) * DEG_TO_IDX(1) / (base.x - plus.x));
            } else {
                yaw = yaw + (u16)((base.z - dir->z) * DEG_TO_IDX(1) / (base.z - plus.z));
            }
        }
    }
    if (pitch < 0x4000) {
        if (base.y > dir->y) {
            pitch = pitch - (u16)((base.y - dir->y) * DEG_TO_IDX(1) / (base.y - sinPitchMinus));
        } else if (base.y < dir->y) {
            pitch = pitch + (u16)((dir->y - base.y) * DEG_TO_IDX(1) / (sinPitchPlus - base.y));
        }
    } else if (pitch >= 0xc000) {
        if (base.y > dir->y) {
            pitch = pitch - (u16)((base.y - dir->y) * DEG_TO_IDX(1) / (base.y - sinPitchMinus));
        } else if (base.y < dir->y) {
            pitch = pitch + (u16)((dir->y - base.y) * DEG_TO_IDX(1) / (sinPitchPlus - base.y));
        }
    }
    *outYaw = yaw;
    *outPitch = pitch;
}

G3DCamera *FieldCamera_GetG3DCamera(FieldCamera *camera) {
    return camera->g3dCamera;
}

void FieldCamera_SetBind(FieldCamera *camera, const VecFx32 *bind) {
    camera->bind = bind;
    camera->targetBase = *bind;
    camera->target = *bind;
}

void FieldCamera_ClearBind(FieldCamera *camera) {
    camera->bind = NULL;
}

const VecFx32 *FieldCamera_GetBind(FieldCamera *camera) {
    return camera->bind;
}

void FieldCamera_ResetBind(FieldCamera *camera) {
    camera->bind = camera->refBind;
}

void FieldCamera_SetRefBind(FieldCamera *camera, const VecFx32 *bind) {
    camera->refBind = bind;
}

const VecFx32 *FieldCamera_GetRefBind(FieldCamera *camera) {
    return camera->refBind;
}

static void FieldCamera_SetZNear(FieldCamera *camera, fx32 zNear) {
    GFL_G3DCameraSetProjectionZNear(camera->g3dCamera, &zNear);
}

static void FieldCamera_SetZFar(FieldCamera *camera, fx32 zFar) {
    GFL_G3DCameraSetProjectionZFar(camera->g3dCamera, &zFar);
}

void FieldCamera_CoordsSetTarget(FieldCamera *camera, const VecFx32 *target) {
    camera->target = *target;
    camera->targetBase = *target;
}

void FieldCamera_CoordsGetTarget(FieldCamera *camera, VecFx32 *target) {
    *target = camera->target;
}

void FieldCamera_CoordsGetTargetOffset(FieldCamera *camera, VecFx32 *offset) {
    *offset = camera->targetOffset;
}

void FieldCamera_CoordsSetTargetOffset(FieldCamera *camera, const VecFx32 *offset) {
    camera->targetOffset = *offset;
}

void FieldCamera_CoordsGetEye(FieldCamera *camera, VecFx32 *eye) {
    *eye = camera->eye;
}

void FieldCamera_CoordsSetEye(FieldCamera *camera, const VecFx32 *eye) {
    camera->eye = *eye;
}

void FieldCamera_CoordsGetEyeOffset(FieldCamera *camera, VecFx32 *offset) {
    *offset = camera->eyeOffset;
}

void FieldCamera_CoordsSetEyeOffset(FieldCamera *camera, const VecFx32 *offset) {
    camera->eyeOffset = *offset;
}

u16 FieldCamera_CoordsGetPitch(FieldCamera *camera) {
    return camera->pitch;
}

void FieldCamera_CoordsSetPitch(FieldCamera *camera, u16 pitch) {
    camera->pitch = pitch;
}

u16 *FieldCamera_GetViewPitchPtr(FieldCamera *camera) {
    return &camera->anglePitch;
}

u16 FieldCamera_CoordsGetYaw(FieldCamera *camera) {
    return camera->yaw;
}

void FieldCamera_CoordsSetYaw(FieldCamera *camera, u16 yaw) {
    camera->yaw = yaw;
}

u16 *FieldCamera_GetViewYawPtr(FieldCamera *camera) {
    return &camera->angleYaw;
}

fx32 FieldCamera_CoordsGetZoom(FieldCamera *camera) {
    return camera->zoom;
}

void FieldCamera_CoordsSetZoom(FieldCamera *camera, fx32 zoom) {
    camera->zoom = zoom;
}

void FieldCamera_CoordsDerive(const VecFx32 *vec, u16 *pitch, u16 *yaw, fx32 *zoom) {
    FieldCamera_CoordsDerive_(vec, pitch, yaw, zoom);
}

void FieldCamera_SetFOV(FieldCamera *camera, u16 fov) {
    camera->fov = fov;
    GFL_G3DCameraPerspectiveSetFOVSin(camera->g3dCamera, FX_SinIdx(camera->fov));
    GFL_G3DCameraPerspectiveSetFOVCos(camera->g3dCamera, FX_CosIdx(camera->fov));
}

static void FieldCameraDelay_Update(FieldCameraDelay *delay, const VecFx32 *target, const VecFx32 *eye,
                                    VecFx32 *outTarget, VecFx32 *outEye) {
    int head;
    int tail;
    if (delay == NULL) {
        *outTarget = *target;
        *outEye = *eye;
        return;
    }
    if (!delay->active) {
        *outTarget = *target;
        *outEye = *eye;
        return;
    }
    head = delay->head;
    tail = delay->tail;
    if (!delay->finishing) {
        delay->targets[tail] = *target;
        delay->eyes[tail] = *eye;
        tail = (tail + 1) % delay->count;
    }
    if (delay->filled) {
        *outTarget = delay->targets[head];
        *outEye = delay->eyes[head];
    } else {
        *outTarget = delay->targets[delay->maxDelay];
        *outEye = delay->eyes[delay->maxDelay];
        if (head == delay->maxDelay) {
            delay->filled = TRUE;
        }
    }
    if (delay->finishing) {
        if (head != tail) {
            head = (head + 1) % delay->count;
            if (head == tail) {
                FieldCameraDelay_Disable(delay);
            }
        }
    } else {
        head = (head + 1) % delay->count;
    }
    delay->head = head;
    delay->tail = tail;
    if (!delay->delayX) {
        outTarget->x = target->x;
        outEye->x = eye->x;
    }
    if (!delay->delayY) {
        outTarget->y = target->y;
        outEye->y = eye->y;
    }
    if (!delay->delayZ) {
        outTarget->z = target->z;
        outEye->z = eye->z;
    }
}

static void FieldCameraDelay_Create(int count, int maxDelay, int flags, u32 heapId, FieldCamera *camera) {
    int i;
    FieldCameraDelay *delay;
    if (camera->bind != NULL) {
        delay = GFL_HeapAllocate(heapId, sizeof(FieldCameraDelay), FALSE, "field_camera.c", 1728);
        delay->targets = GFL_HeapAllocate(heapId, sizeof(VecFx32) * count, FALSE, "field_camera.c", 1729);
        delay->eyes = GFL_HeapAllocate(heapId, sizeof(VecFx32) * count, FALSE, "field_camera.c", 1730);
        for (i = 0; i < count; i++) {
            VecFx32 *targets = delay->targets;
            VecFx32 *eyes;
            targets[i].x = 0;
            targets[i].y = 0;
            targets[i].z = 0;
            eyes = delay->eyes;
            eyes[i].x = 0;
            eyes[i].y = 0;
            eyes[i].z = 0;
        }
        delay->count = count;
        delay->maxDelay = maxDelay;
        delay->filled = FALSE;
        delay->head = 0;
        delay->tail = maxDelay;
        delay->delayX = FALSE;
        delay->delayY = FALSE;
        delay->delayZ = FALSE;
        if (flags & 1) {
            delay->delayX = TRUE;
        }
        if (flags & 2) {
            delay->delayY = TRUE;
        }
        if (flags & 4) {
            delay->delayZ = TRUE;
        }
        delay->active = TRUE;
        delay->finishing = FALSE;
        camera->delay = delay;
    }
}

static void FieldCamera_FreeDelayManager(FieldCamera *camera) {
    if (camera->delay != NULL) {
        GFL_HeapFree(camera->delay->targets);
        GFL_HeapFree(camera->delay->eyes);
        GFL_HeapFree(camera->delay);
        camera->delay = NULL;
    }
}

static void FieldCamera_ApplyDelay(FieldCamera *camera) {
    if (camera->bind != NULL) {
        VecFx32 eye;
        VecFx32 target;
        VecFx32 outEye;
        VecFx32 outTarget;
        GFL_G3DCameraGetLookatTarget(camera->g3dCamera, &target);
        GFL_G3DCameraGetLookatPos(camera->g3dCamera, &eye);
        FieldCameraDelay_Update(camera->delay, &target, &eye, &outTarget, &outEye);
        GFL_G3DCameraSetLookatTarget(camera->g3dCamera, &outTarget);
        GFL_G3DCameraSetLookatPos(camera->g3dCamera, &outEye);
    }
}

static void FieldCameraDelay_Disable(FieldCameraDelay *delay) {
    delay->finishing = FALSE;
    delay->active = FALSE;
    delay->filled = FALSE;
}

void FieldCamera_EnableDelay(FieldCamera *camera) {
    FieldCameraDelay *delay = camera->delay;
    int i;
    if (delay != NULL) {
        for (i = 0; i < delay->count; i++) {
            VecFx32 *targets = delay->targets;
            VecFx32 *eyes;
            targets[i].x = 0;
            targets[i].y = 0;
            targets[i].z = 0;
            eyes = delay->eyes;
            eyes[i].x = 0;
            eyes[i].y = 0;
            eyes[i].z = 0;
        }
        delay->head = 0;
        delay->tail = delay->maxDelay;
        delay->filled = FALSE;
        delay->active = TRUE;
        delay->finishing = FALSE;
    }
}

void FieldCamera_FinishDelay(FieldCamera *camera) {
    FieldCameraDelay *delay = camera->delay;
    if (delay != NULL) {
        if (!delay->filled && delay->tail == delay->maxDelay && delay->head == 0) {
            FieldCameraDelay_Disable(delay);
            return;
        }
        delay->finishing = TRUE;
    }
}

void FieldCamera_DisableDelay(FieldCamera *camera) {
    FieldCameraDelay *delay = camera->delay;
    if (delay != NULL) {
        delay->finishing = FALSE;
        delay->active = FALSE;
        delay->filled = FALSE;
    }
}

BOOL FieldCamera_IsDelayActive(FieldCamera *camera) {
    FieldCameraDelay *delay = camera->delay;
    if (delay == NULL) {
        return FALSE;
    }
    return delay->active;
}

BOOL FieldCamera_SupportsDelay(FieldCamera *camera) {
    if (camera->bind == NULL) {
        return FALSE;
    }
    if (camera->delay != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void FieldCameraBoundary_LoadFile(FieldCamera *camera, const u32 *file) {
    MI_CpuCopy8(file + 1, camera->bounds, file[0] * sizeof(FieldCameraBoundary));
    camera->boundCount = file[0];
}

void FieldCameraBoundary_LoadDummy(FieldCamera *camera) {
    MI_CpuFill8(camera->bounds, 0, sizeof(camera->bounds));
    camera->boundCount = 0;
}

void FieldCameraBoundary_ChangeID(FieldCamera *camera, u16 boundaryId, HeapID heapId) {
    u32 *file;
    if (camera->boundaryId != boundaryId) {
        file = GFL_ArcToolReadHeapNewLZ(camera->boundaryArc, boundaryId, FALSE, HEAPID_TAIL(heapId));
        FieldCameraBoundary_LoadFile(camera, file);
        GFL_HeapFree(file);
        camera->boundaryId = boundaryId;
    }
}

void FieldCamera_GetTarget(FieldCamera *camera, VecFx32 *target) {
    *target = camera->targetActual;
}

void FieldCamera_GetEye(FieldCamera *camera, VecFx32 *eye) {
    *eye = camera->eyeActual;
}

void FieldCamera_SetUseBoundaryEnable(FieldCamera *camera, BOOL enable) {
    camera->useBoundary = enable;
}

BOOL FieldCamera_IsUseBoundaryEnable(FieldCamera *camera) {
    return camera->useBoundary;
}

static void FieldCameraBoundary_ApplyTarget(FieldCameraBoundary *bound, VecFx32 *vec) {
    if (bound->head.applyTo == 0) {
        if (MM_CAMBOUND_FUNCS[bound->head.shape] != NULL) {
            MM_CAMBOUND_FUNCS[bound->head.shape](bound, vec, vec);
        }
    }
}

static void FieldCameraBoundary_ApplyEye(FieldCameraBoundary *bound, VecFx32 *vec) {
    if (bound->head.applyTo == 1) {
        if (MM_CAMBOUND_FUNCS[bound->head.shape] != NULL) {
            MM_CAMBOUND_FUNCS[bound->head.shape](bound, vec, vec);
        }
    }
}

static BOOL FieldCameraBoundary_RestrictRect(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out) {
    const FieldCameraRect *rect = &bound->rect;
    VecFx32 pos = *in;
    BOOL moved = FALSE;
    out->x = pos.x;
    out->z = pos.z;
    out->y = pos.y;
    if (FX32_CONST(rect->minX) > pos.x) {
        moved = TRUE;
        out->x = FX32_CONST(rect->minX);
    } else if ((rect->maxX << 12) < pos.x) {
        moved = TRUE;
        out->x = FX32_CONST(rect->maxX);
    }
    if ((rect->minZ << 12) > pos.z) {
        moved = TRUE;
        out->z = FX32_CONST(rect->minZ);
    } else if ((rect->maxZ << 12) < pos.z) {
        moved = TRUE;
        out->z = FX32_CONST(rect->maxZ);
    }
    return moved;
}

static BOOL FieldCameraBoundary_RestrictCircle(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out) {
    const FieldCameraCircle *circle = &bound->circle;
    VecFx32 axis = { 0, 0, FX32_ONE };
    VecFx32 pos = *in;
    VecFx32 orig = *in;
    VecFx32 center;
    VecFx32 offset;
    VecFx32 cross;
    fx32 dist;
    u16 angle;
    BOOL moved = FALSE;
    out->x = in->x;
    out->y = in->y;
    out->z = in->z;
    center.x = circle->centerX;
    center.y = 0;
    center.z = circle->centerZ;
    pos.y = 0;
    VEC_Subtract(&pos, &center, &offset);
    dist = VEC_Mag(&offset);
    if (FX_Modf(dist, &dist) > 0) {
        dist += FX32_ONE;
    }
    VEC_Normalize(&offset, &offset);
    VEC_CrossProduct(&axis, &offset, &cross);
    angle = FX_AcosIdx(VEC_DotProduct(&axis, &offset));
    if (cross.y < 0) {
        angle = 0x10000 - angle;
    }
    if (dist > circle->radius) {
        dist = circle->radius;
        moved = TRUE;
    }
    if (circle->angleA != circle->angleB) {
        if (circle->angleA > circle->angleB) {
            if (circle->angleB < angle && circle->angleA >= angle) {
                if (MATH_ABS(circle->angleB - angle) > MATH_ABS(circle->angleA - angle)) {
                    angle = circle->angleA;
                    moved = TRUE;
                } else {
                    angle = circle->angleB;
                    moved = TRUE;
                }
            }
        } else if (circle->angleA > angle) {
            angle = circle->angleA;
            moved = TRUE;
        } else if (circle->angleB < angle) {
            angle = circle->angleB;
            moved = TRUE;
        }
    }
    if (moved) {
        out->x = center.x + FX_Mul(FX_SinIdx(angle), dist);
        out->z = center.z + FX_Mul(FX_CosIdx(angle), dist);
        out->y = orig.y;
    }
    return moved;
}

static BOOL FieldCameraBoundary_RepealRect(const FieldCameraBoundary *bound, const VecFx32 *in, VecFx32 *out) {
    const FieldCameraRect *rect = &bound->rect;
    VecFx32 pos = *in;
    BOOL moved = FALSE;
    fx32 best;
    int side;
    s32 minX;
    s32 maxX;
    s32 minZ;
    s32 maxZ;
    out->x = pos.x;
    out->z = pos.z;
    out->y = pos.y;
    minX = rect->minX;
    if (FX32_CONST(minX) < pos.x) {
        maxX = rect->maxX;
        if (FX32_CONST(maxX) > pos.x) {
            minZ = rect->minZ;
            if (FX32_CONST(minZ) < pos.z) {
                maxZ = rect->maxZ;
                if (FX32_CONST(maxZ) > pos.z) {
                    moved = TRUE;
                    best = out->x - FX32_CONST(minX);
                    side = RECT_SIDE_MIN_X;
                    if (best > FX32_CONST(maxX) - out->x) {
                        best = FX32_CONST(maxX) - out->x;
                        side = RECT_SIDE_MAX_X;
                    }
                    if (best > out->z - FX32_CONST(minZ)) {
                        best = out->z - FX32_CONST(minZ);
                        side = RECT_SIDE_MIN_Z;
                    }
                    if (best > FX32_CONST(maxZ) - out->z) {
                        best = FX32_CONST(maxZ) - out->z;
                        side = RECT_SIDE_MAX_Z;
                    }
                    switch (side) {
                    case RECT_SIDE_MIN_X:
                        out->x = FX32_CONST(minX);
                        break;
                    case RECT_SIDE_MAX_X:
                        out->x = FX32_CONST(maxX);
                        break;
                    case RECT_SIDE_MIN_Z:
                        out->z = FX32_CONST(minZ);
                        break;
                    case RECT_SIDE_MAX_Z:
                        out->z = FX32_CONST(maxZ);
                        break;
                    }
                }
            }
        }
    }
    return moved;
}

void FieldCamera_BackupCoords(FieldCamera *camera, FieldEvCameraAnimationCoords *coords) {
    coords->cameraPos = camera->eye;
    coords->targetPos = camera->target;
    coords->pitch = camera->pitch;
    coords->yaw = camera->yaw;
    coords->distance = camera->zoom;
    coords->fov = camera->fov;
    coords->extraTranslation = camera->targetOffset;
}

void FieldCamera_BackupCoordsRealTime(FieldCamera *camera, FieldEvCameraAnimationCoords *coords) {
    coords->cameraPos = camera->eye;
    FieldCamera_GetTarget(camera, &coords->targetPos);
    coords->pitch = camera->pitch;
    coords->yaw = camera->yaw;
    coords->distance = camera->zoom;
    coords->fov = camera->fov;
    coords->extraTranslation = camera->targetOffset;
}

void FieldCamera_EVCameraInit(FieldCamera *camera) {
    FieldCameraEvAnimation *ev;
    if (camera != NULL) {
        ev = &camera->ev;
        if (!ev->initDone) {
            ev->savedTransformType = camera->transformType;
            FieldCamera_ChangeTransformType(camera, 0);
            FieldCamera_BackupCoords(camera, &ev->base);
            ev->initDone = TRUE;
        }
    }
}
