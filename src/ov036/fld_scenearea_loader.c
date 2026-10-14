// The loader of the camera areas' data, and the functions that check and move the camera for each kind of area. The
// name is the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_scene_area.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

static BOOL FieldCameraArea_CIRCLE_CheckColl(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);
static void FieldCameraAreaCalc_CIRCLE_AimPlayerCentre(FieldSceneArea *area, const CameraArea *cameraArea,
                                                       const VecFx32 *pos);
static void FieldCameraAreaCalc_CIRCLE_FixedLookAt(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);
static BOOL FieldCameraAreaCalc_RECT_CheckColl(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_PitchYawTZ(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_PitchYawTZ_BeginDisableDelay(FieldSceneArea *area, const CameraArea *cameraArea,
                                                                  const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_PitchYawTZ_ResetEnableDelay(FieldSceneArea *area, const CameraArea *cameraArea,
                                                                 const VecFx32 *pos);
static void CalcFieldDynCameraLerpFactors(const CameraArea *cameraArea, const VecFx32 *pos, fx32 *length, fx32 *progress);
static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_STAY(FieldSceneArea *area, const CameraArea *cameraArea,
                                                              const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_BeginDisableDelay(FieldSceneArea *area,
                                                                           const CameraArea *cameraArea,
                                                                           const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_ResetEnableDelay(FieldSceneArea *area,
                                                                          const CameraArea *cameraArea,
                                                                          const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_LookatTargetOffs(FieldSceneArea *area, const CameraArea *cameraArea,
                                                      const VecFx32 *pos);
static void FieldCameraAreaCalc_RECT_FOV(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);

// The camera areas of the field maps, and of the other maps
static const u32 FIELD_CAMERA_ARCIDS[2] = { 79, 156 };

static CameraAreaCollCheck CAM_COLL_CHECK_FUNCS[2] = {
    FieldCameraArea_CIRCLE_CheckColl,
    FieldCameraAreaCalc_RECT_CheckColl,
};

static CameraAreaCalcFunc CAM_CALC_FUNCS[8] = {
    FieldCameraAreaCalc_CIRCLE_AimPlayerCentre,
    FieldCameraAreaCalc_CIRCLE_FixedLookAt,
    FieldCameraAreaCalc_RECT_PitchYawTZ,
    FieldCameraAreaCalc_RECT_PitchYawTZ_BeginDisableDelay,
    FieldCameraAreaCalc_RECT_PitchYawTZ_ResetEnableDelay,
    FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_STAY,
    FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_BeginDisableDelay,
    FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_ResetEnableDelay,
};

static const FieldDynCameraFunctions FIELD_CAMERA_AREA_FUNCS = {
    CAM_COLL_CHECK_FUNCS,
    CAM_CALC_FUNCS,
    2,
    8,
};

FieldSceneAreaLoader *FieldSceneAreaLoader_Create(HeapID heapId) {
    FieldSceneAreaLoader *loader = GFL_HeapAllocate(heapId, sizeof(FieldSceneAreaLoader), TRUE,
                                                    "fld_scenearea_loader.c", 129);
    int i;

    for (i = 0; i < 2; i++) {
        loader->camArcTools[i] = GFL_ArcSysCreateFileHandle(FIELD_CAMERA_ARCIDS[i], heapId);
    }
    return loader;
}

void FreeFieldSceneAreaLoader(FieldSceneAreaLoader *loader) {
    int i;

    ResetSceneAreaLoader(loader);
    for (i = 0; i < 2; i++) {
        GFL_ArcToolFree(loader->camArcTools[i]);
    }
    GFL_HeapFree(loader);
}

void LoadCameraDataToSceneAreaLoader(FieldSceneAreaLoader *loader, u32 index, u32 cameraId, HeapID heapId) {
    u32 size;

    loader->cameraData = GFL_ArcToolReadHeapNewLZGetLen(loader->camArcTools[index], cameraId, FALSE, heapId, &size);
    loader->cameraCount = size / sizeof(CameraArea);
}

void ResetSceneAreaLoader(FieldSceneAreaLoader *loader) {
    if (loader->cameraData != NULL) {
        GFL_HeapFree(loader->cameraData);
        loader->cameraData = NULL;
        loader->cameraCount = 0;
    }
}

CameraArea *GetFldSceneAreaLoaderCameraData(FieldSceneAreaLoader *loader) {
    return loader->cameraData;
}

u32 GetFldSceneAreaLoaderCamCount(FieldSceneAreaLoader *loader) {
    return loader->cameraCount;
}

const FieldDynCameraFunctions *GetFldSceneAreaLoaderCamFuncsStaticOffs(FieldSceneAreaLoader *loader) {
    return &FIELD_CAMERA_AREA_FUNCS;
}

// Whether the position is in the ring, between the two radii and the two angles around the centre
static BOOL FieldCameraArea_CIRCLE_CheckColl(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos) {
    VecFx32 dir;
    VecFx32 center;
    fx32 dist;
    u32 angle;

    VEC_Set(&center, cameraArea->circle.x, cameraArea->circle.y, cameraArea->circle.z);
    VEC_Subtract(pos, &center, &dir);
    dir.y = 0;
    dist = VEC_Mag(&dir);
    VEC_Normalize(&dir, &dir);
    angle = FX_Atan2Idx(dir.x, dir.z);
    if (cameraArea->circle.radiusStart <= dist && cameraArea->circle.radiusEnd > dist) {
        if (cameraArea->circle.angleEnd < cameraArea->circle.angleStart) {
            // The ring passes angle 0
            if (cameraArea->circle.angleEnd >= angle || (angle < 0x10000 && angle >= cameraArea->circle.angleStart)) {
                return TRUE;
            }
        } else if (cameraArea->circle.angleStart <= angle && cameraArea->circle.angleEnd > angle) {
            return TRUE;
        }
    }
    return FALSE;
}

// The camera looks at the centre from behind the player
static void FieldCameraAreaCalc_CIRCLE_AimPlayerCentre(FieldSceneArea *area, const CameraArea *cameraArea,
                                                       const VecFx32 *pos) {
    VecFx32 dir;
    VecFx32 target;
    VecFx32 normal;
    VecFx32 eye;
    fx32 horizontal;
    fx32 centerY;
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    FieldCamera_SetTransformType(camera, 2);
    centerY = cameraArea->circle.y;
    VEC_Set(&target, cameraArea->circle.x, centerY, cameraArea->circle.z);
    dir = *pos;
    target.y = 0;
    dir.y = 0;
    VEC_Subtract(&dir, &target, &dir);
    VEC_Normalize(&dir, &normal);
    target.y = centerY;
    eye.y = FX_Mul(FX_SinIdx(cameraArea->circle.pitch), cameraArea->circle.tzDist);
    horizontal = FX_Mul(FX_CosIdx(cameraArea->circle.pitch), cameraArea->circle.tzDist);
    eye.x = FX_Mul(normal.x, horizontal);
    eye.z = FX_Mul(normal.z, horizontal);
    eye.x += target.x;
    eye.y += target.y;
    eye.z += target.z;
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_CoordsSetEye(camera, &eye);
}

static void FieldCameraAreaCalc_CIRCLE_FixedLookAt(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos) {
    VecFx32 target;
    VecFx32 eye;
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    FieldCamera_SetTransformType(camera, 2);
    VEC_Set(&target, cameraArea->circle.camTarget.x, cameraArea->circle.camTarget.y, cameraArea->circle.camTarget.z);
    VEC_Set(&eye, cameraArea->circle.camPos.x, cameraArea->circle.camPos.y, cameraArea->circle.camPos.z);
    FieldCamera_CoordsSetTarget(camera, &target);
    FieldCamera_CoordsSetEye(camera, &eye);
}

// Whether the position's tile is in the rectangle; never with the controller type 1
static BOOL FieldCameraAreaCalc_RECT_CheckColl(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos) {
    VecFx32 tilePos;
    u16 gridX;
    u16 gridZ;
    u16 width;
    u16 height;

    if (Field_GetResolvedControllerTypeID(GetFieldSceneAreaField(area)) == 1) {
        return FALSE;
    }
    tilePos = *pos;
    tilePos.x -= FX32_CONST(8);
    tilePos.z -= FX32_CONST(8);
    gridX = FX_Whole(tilePos.x) / 16;
    gridZ = FX_Whole(tilePos.z) / 16;
    if (cameraArea->rect.gridW == 1) {
        width = cameraArea->rect.gridH;
        height = cameraArea->rect.unk06;
    } else {
        width = cameraArea->rect.gridW;
        height = cameraArea->rect.gridH;
    }
    if (gridX >= cameraArea->rect.gridX && gridX < cameraArea->rect.gridX + width && gridZ >= cameraArea->rect.gridZ &&
        gridZ < cameraArea->rect.gridZ + height) {
        return TRUE;
    }
    return FALSE;
}

// The pitch, yaw and distance blend from the first setting to the second across the rectangle
static void FieldCameraAreaCalc_RECT_PitchYawTZ(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos) {
    fx32 length;
    fx32 progress;
    fx32 pitch;
    fx32 yaw;
    fx32 tz;
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    FieldCamera_SetTransformType(camera, 0);
    CalcFieldDynCameraLerpFactors(cameraArea, pos, &length, &progress);
    progress = FX_Div(progress, length);
    pitch = cameraArea->rect.pitch2 - cameraArea->rect.pitch1;
    pitch = pitch * progress / FX32_ONE;
    pitch += cameraArea->rect.pitch1;
    yaw = cameraArea->rect.yaw2 - cameraArea->rect.yaw1;
    yaw = yaw * progress / FX32_ONE;
    yaw += cameraArea->rect.yaw1;
    tz = FX_Mul(cameraArea->rect.tz2 - cameraArea->rect.tz1, progress);
    tz += cameraArea->rect.tz1;
    FieldCamera_CoordsSetPitch(camera, pitch);
    FieldCamera_CoordsSetYaw(camera, yaw);
    FieldCamera_CoordsSetZoom(camera, tz);
}

static void FieldCameraAreaCalc_RECT_PitchYawTZ_BeginDisableDelay(FieldSceneArea *area, const CameraArea *cameraArea,
                                                                  const VecFx32 *pos) {
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    if (cameraArea->rect.forbidDelayManager) {
        FieldCamera_DisableDelay(camera);
    }
    FieldCameraAreaCalc_RECT_PitchYawTZ(area, cameraArea, pos);
}

static void FieldCameraAreaCalc_RECT_PitchYawTZ_ResetEnableDelay(FieldSceneArea *area, const CameraArea *cameraArea,
                                                                 const VecFx32 *pos) {
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    FieldCameraAreaCalc_RECT_PitchYawTZ(area, cameraArea, pos);
    if (cameraArea->rect.forbidDelayManager) {
        FieldCamera_EnableDelay(camera);
    }
}

// The length of the rectangle along the blend, and how far into it the position is
static void CalcFieldDynCameraLerpFactors(const CameraArea *cameraArea, const VecFx32 *pos, fx32 *length, fx32 *progress) {
    VecFx32 tilePos = *pos;

    tilePos.x -= FX32_CONST(8);
    tilePos.z -= FX32_CONST(8);
    *length = cameraArea->rect.gridH * FX32_CONST(16);
    if (cameraArea->rect.gridW == 1) {
        *progress = tilePos.x - cameraArea->rect.gridX * FX32_CONST(16);
    } else {
        *progress = tilePos.z - cameraArea->rect.gridZ * FX32_CONST(16);
    }
}

static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_STAY(FieldSceneArea *area, const CameraArea *cameraArea,
                                                              const VecFx32 *pos) {
    FieldCameraAreaCalc_RECT_PitchYawTZ(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_LookatTargetOffs(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_FOV(area, cameraArea, pos);
}

static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_BeginDisableDelay(FieldSceneArea *area,
                                                                           const CameraArea *cameraArea,
                                                                           const VecFx32 *pos) {
    FieldCameraAreaCalc_RECT_PitchYawTZ_BeginDisableDelay(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_LookatTargetOffs(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_FOV(area, cameraArea, pos);
}

static void FieldCameraAreaCalc_RECT_PitchYawTZLookatFOV_ResetEnableDelay(FieldSceneArea *area,
                                                                          const CameraArea *cameraArea,
                                                                          const VecFx32 *pos) {
    FieldCameraAreaCalc_RECT_PitchYawTZ_ResetEnableDelay(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_LookatTargetOffs(area, cameraArea, pos);
    FieldCameraAreaCalc_RECT_FOV(area, cameraArea, pos);
}

static void FieldCameraAreaCalc_RECT_LookatTargetOffs(FieldSceneArea *area, const CameraArea *cameraArea,
                                                      const VecFx32 *pos) {
    fx32 length;
    fx32 progress;
    VecFx32 offset;
    fx32 diffY;
    fx32 diffZ;
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    CalcFieldDynCameraLerpFactors(cameraArea, pos, &length, &progress);
    diffY = cameraArea->rect.targetOffset2.y - cameraArea->rect.targetOffset1.y;
    diffZ = cameraArea->rect.targetOffset2.z - cameraArea->rect.targetOffset1.z;
    offset.x = cameraArea->rect.targetOffset1.x +
               FX_Div(FX_Mul(cameraArea->rect.targetOffset2.x - cameraArea->rect.targetOffset1.x, progress), length);
    offset.y = cameraArea->rect.targetOffset1.y + FX_Div(FX_Mul(diffY, progress), length);
    offset.z = cameraArea->rect.targetOffset1.z + FX_Div(FX_Mul(diffZ, progress), length);
    FieldCamera_CoordsSetTargetOffset(camera, &offset);
}

static void FieldCameraAreaCalc_RECT_FOV(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos) {
    fx32 length;
    fx32 progress;
    fx32 diff;
    FieldCamera *camera = GetFieldSceneAreaCamera(area);

    CalcFieldDynCameraLerpFactors(cameraArea, pos, &length, &progress);
    progress = FX_Div(progress, length);
    diff = cameraArea->rect.fov2 - cameraArea->rect.fov1;
    FieldCamera_SetFOV(camera, cameraArea->rect.fov1 + diff * progress / FX32_ONE);
}
