#ifndef POKEBW2_FIELD_FIELD_SCENE_AREA_H
#define POKEBW2_FIELD_FIELD_SCENE_AREA_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"
// Overlay 36: the camera areas of a zone, and the loader of their data. Names and layouts from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

typedef struct CameraArea CameraArea;

typedef BOOL (*CameraAreaCollCheck)(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);
typedef void (*CameraAreaCalcFunc)(FieldSceneArea *area, const CameraArea *cameraArea, const VecFx32 *pos);

// A rectangle of the map with two camera settings, which the camera blends between across it
typedef struct {
    u16 gridX;
    u16 gridZ;
    u16 gridW;
    u16 unk06;
    u16 gridH;
    u16 unk0A;
    u16 pitch1;
    u16 yaw1;
    fx32 tz1;
    u16 pitch2;
    u16 yaw2;
    fx32 tz2;
    BOOL forbidDelayManager;
    VecFx32 targetOffset1;
    VecFx32 targetOffset2;
    u16 fov1;
    u16 fov2;
} CameraAreaRect;

// A ring of the map around a point, which the camera turns around
typedef struct {
    u32 angleStart;
    u32 angleEnd;
    fx32 radiusStart;
    fx32 radiusEnd;
    fx32 x;
    fx32 y;
    fx32 z;
    s32 pitch;
    int tzDist;
    VecFx32 camTarget;
    VecFx32 camPos;
} CameraAreaCircle;

struct CameraArea {
    union {
        CameraAreaRect rect;
        CameraAreaCircle circle;
    };
    int unk3C;
    // The functions that check whether the position is in the area, and run while it stays, enters and leaves
    u16 collCheckFunc;
    u16 stayFunc;
    u16 enterFunc;
    u16 exitFunc;
};

typedef struct {
    CameraAreaCollCheck *collCheckFuncs;
    CameraAreaCalcFunc *calcFuncs;
    u16 collCheckFuncCount;
    u16 calcFuncCount;
} FieldDynCameraFunctions;

struct FieldSceneArea {
    FieldCamera *camera;
    Field *field;
    CameraArea *cameraData;
    const FieldDynCameraFunctions *cameraFunctions;
    u32 cameraCount;
    int lastCamIdx;
    BOOL isCameraAreaEnable;
    u32 currentCamDataFlags;
};

FieldSceneArea *FieldSceneArea_Create(u32 heapId, FieldCamera *camera, Field *field);
void FreeFieldSceneArea(FieldSceneArea *area);
void BuildSceneArea(FieldSceneArea *area, CameraArea *cameraData, u32 count, const FieldDynCameraFunctions *funcs);
void ResetSceneArea(FieldSceneArea *area);
// The camera area the position is in, or -1
int FieldCameraArea_Update(FieldSceneArea *area, const VecFx32 *pos);
void SetFieldSceneAreaCameraAreaEnable(FieldSceneArea *area, BOOL enable);
FieldCamera *GetFieldSceneAreaCamera(FieldSceneArea *area);
Field *GetFieldSceneAreaField(FieldSceneArea *area);
// The loader of the camera areas: each zone's areas come from one file of one of the two archives
typedef struct {
    CameraArea *cameraData;
    u32 cameraCount;
    ArcTool *camArcTools[2];
} FieldSceneAreaLoader;

FieldSceneAreaLoader *FieldSceneAreaLoader_Create(HeapID heapId);
void FreeFieldSceneAreaLoader(FieldSceneAreaLoader *loader);
void LoadCameraDataToSceneAreaLoader(FieldSceneAreaLoader *loader, u32 index, u32 cameraId, HeapID heapId);
void ResetSceneAreaLoader(FieldSceneAreaLoader *loader);
CameraArea *GetFldSceneAreaLoaderCameraData(FieldSceneAreaLoader *loader);
u32 GetFldSceneAreaLoaderCamCount(FieldSceneAreaLoader *loader);
const FieldDynCameraFunctions *GetFldSceneAreaLoaderCamFuncsStaticOffs(FieldSceneAreaLoader *loader);

#endif // POKEBW2_FIELD_FIELD_SCENE_AREA_H
