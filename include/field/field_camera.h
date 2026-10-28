#ifndef POKEBW2_FIELD_FIELD_CAMERA_H
#define POKEBW2_FIELD_FIELD_CAMERA_H

#include "types.h"
#include "field/field_task.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// An event camera animation's target and what it animates. Names and layouts from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
typedef struct {
    VecFx32 cameraPos;
    VecFx32 targetPos;
    VecFx32 extraTranslation;
    u16 pitch;
    u16 yaw;
    fx32 distance;
    u16 fov;
    u16 padFov;
} FieldEvCameraAnimationCoords;

typedef struct {
    BOOL animateExtraTranslation;
    BOOL animatePitch;
    BOOL animateYaw;
    BOOL animateTargetDistance;
    BOOL animateFOV;
    BOOL animateTargetPos;
} FieldEvCameraAnimationFlags;

typedef struct {
    FieldEvCameraAnimationCoords targetCoords;
    FieldEvCameraAnimationFlags flags;
} FieldEvCameraAnimationSetup;

// field_camera.c's camera. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the
// layouts are ours, from field_camera.c and field_camera_anime.c

// Where the camera's target or eye is kept inside a rectangle or a circle, or out of a rectangle: a boundary of the
// zone's file in archive 0x6d
typedef struct {
    u32 shape;
    u32 applyTo;
    s32 minX;
    s32 maxX;
    s32 minZ;
    s32 maxZ;
} FieldCameraRect;

typedef struct {
    u32 shape;
    u32 applyTo;
    s32 centerX;
    s32 centerZ;
    s32 radius;
    u16 angleA;
    u16 angleB;
} FieldCameraCircle;

typedef union {
    struct {
        u32 shape;
        u32 applyTo;
    } head;
    FieldCameraRect rect;
    FieldCameraCircle circle;
} FieldCameraBoundary;

// A camera setup of archive 0x3b. With bind, the camera follows what it is bound to, at target from it; without, it
// looks at target
typedef struct {
    s32 zoom;
    u32 pitch;
    u32 yaw;
    u32 unkC;
    u8 unk10;
    // The GXBufferMode of the swap
    u8 swapMode;
    u16 fov;
    fx32 zNear;
    fx32 zFar;
    BOOL bind;
    VecFx32 target;
} FieldCameraDefaults;

// The event camera: the coordinates it started from, and the animation that runs (field_camera_anime.c)
typedef struct {
    u32 savedTransformType;
    FieldEvCameraAnimationCoords base;
    FieldEvCameraAnimationCoords start;
    FieldEvCameraAnimationCoords end;
    u16 frames;
    u16 frame;
    BOOL initDone;
    BOOL active;
    FieldEvCameraAnimationFlags flags;
    // What the camera's calcFunc is called with
    void *calcArg;
} FieldCameraEvAnimation;

struct FieldCamera {
    u16 heapId;
    ArcTool *defaultsArc;
    ArcTool *boundaryArc;
    G3DCamera *g3dCamera;
    u32 defaultsIndex;
    u32 transformType;
    const VecFx32 *refBind;
    const VecFx32 *bind;
    const VecFx32 *eyeBind;
    VecFx32 eye;
    VecFx32 eyeOffset;
    VecFx32 eyeActual;
    VecFx32 target;
    VecFx32 targetActual;
    VecFx32 targetOffset;
    VecFx32 targetBase;
    u16 pitch;
    u16 yaw;
    fx32 zoom;
    u16 fov;
    u16 angleYaw;
    u16 anglePitch;
    FieldCameraBoundary bounds[4];
    u8 boundCount;
    u8 useBoundary;
    u16 boundaryId;
    FieldCameraDelay *delay;
    FieldCameraEvAnimation ev;
    // Called before each transform with ev.calcArg, such as the event camera's animation
    void (*calcFunc)(FieldCamera *camera, void *arg);
};

u32 FieldCamera_GetTransformType(FieldCamera *camera);
void FieldCamera_ReadDefaults(FieldCamera *camera, FieldCameraDefaults *defaults);
// The pitch, yaw and distance of a direction
void FieldCamera_CoordsDerive(const VecFx32 *vec, u16 *pitch, u16 *yaw, fx32 *zoom);
// The target and the eye as the camera last put them, after its boundaries
void FieldCamera_GetTarget(FieldCamera *camera, VecFx32 *target);
void FieldCamera_GetEye(FieldCamera *camera, VecFx32 *eye);
// The camera's coordinates, with the target as set or as the camera last put it
void FieldCamera_BackupCoords(FieldCamera *camera, FieldEvCameraAnimationCoords *coords);
void FieldCamera_BackupCoordsRealTime(FieldCamera *camera, FieldEvCameraAnimationCoords *coords);

// Where a camera animation of the camera animation controller starts or ends
typedef struct {
    u16 pitch;
    u16 yaw;
    fx32 distance;
    VecFx32 target;
    VecFx32 offset;
} FieldCameraCoords;

typedef struct {
    u32 frames;
    FieldCameraCoords src;
    FieldCameraCoords dst;
    // Whether the camera lets go of what it is bound to, and leaves its boundary, while the animation plays
    BOOL releaseBind;
    BOOL disableBoundary;
} FieldCameraAnimation;

typedef struct FieldCameraAnimationController FieldCameraAnimationController;

void FieldCamera_GetAnimationCoords(FieldCamera *camera, FieldCameraCoords *coords);
FieldCameraAnimationController *Field_CreateCameraAnimationController(Field *field);
void FieldCameraAnimationController_FreeAnimation(FieldCameraAnimationController *controller);
void FieldCameraAnimationController_SetAnimation(FieldCameraAnimationController *controller,
                                                 const FieldCameraAnimation *animation);
void FieldCameraAnimationController_PrepareCamera(FieldCameraAnimationController *controller);
void FieldCameraAnimationController_StartAnimation(FieldCameraAnimationController *controller);
// Overlay 36's field_camera_anime.c: releases the camera the animation is bound to, and puts the
// camera at the animation's start
void FieldCameraAnimationController_FinishCamera(FieldCameraAnimationController *controller);
void FieldCameraAnimationController_CommitSrcCoords(FieldCameraAnimationController *controller);
void FieldCamera_CalcTransform(FieldCamera *camera, u16 heldKeys);
G3DCamera *FieldCamera_GetG3DCamera(FieldCamera *camera);
void FieldCamera_CoordsGetEyeOffset(FieldCamera *camera, VecFx32 *offset);
void FieldCamera_CoordsGetEye(FieldCamera *camera, VecFx32 *eye);
void FieldCamera_CoordsGetTarget(FieldCamera *camera, VecFx32 *target);
void FieldCamera_CoordsGetTargetOffset(FieldCamera *camera, VecFx32 *offset);
void FieldCamera_CoordsSetEyeOffset(FieldCamera *camera, const VecFx32 *offset);
void FieldCamera_CoordsSetTargetOffset(FieldCamera *camera, const VecFx32 *offset);
u16 FieldCamera_CoordsGetPitch(FieldCamera *camera);
u16 FieldCamera_CoordsGetYaw(FieldCamera *camera);
fx32 FieldCamera_CoordsGetZoom(FieldCamera *camera);
void FieldCamera_CoordsSetTarget(FieldCamera *camera, const VecFx32 *target);
void FieldCamera_CoordsSetEye(FieldCamera *camera, const VecFx32 *eye);
void FieldCamera_CoordsSetPitch(FieldCamera *camera, u16 pitch);
void FieldCamera_CoordsSetZoom(FieldCamera *camera, fx32 zoom);
void FieldCamera_SetFOV(FieldCamera *camera, u16 fov);
void FieldCamera_CoordsSetYaw(FieldCamera *camera, u16 yaw);
// Whether the camera keeps inside the zone's boundary
BOOL FieldCamera_IsUseBoundaryEnable(FieldCamera *camera);
void FieldCamera_SetUseBoundaryEnable(FieldCamera *camera, BOOL enable);
// Stop the camera following its target, and follow it again
void FieldCamera_ClearBind(FieldCamera *camera);
void FieldCamera_ResetBind(FieldCamera *camera);
void FieldCamera_ChangeTransformType(FieldCamera *camera, u32 type);
void FieldCamera_SetTransformType(FieldCamera *camera, u32 type);
void FieldCamera_LoadDefaults(FieldCamera *camera);
void FieldCamera_DisableDelay(FieldCamera *camera);
void FieldCamera_SetDefaultsIndex(FieldCamera *camera, u32 index);
u32 FieldCamera_GetDefaultsIndex(FieldCamera *camera);
void FieldCamera_EnableDelay(FieldCamera *camera);
// Whether the camera has a delay it can finish
BOOL FieldCamera_SupportsDelay(FieldCamera *camera);
void FieldCamera_FinishDelay(FieldCamera *camera);
BOOL FieldCamera_IsDelayActive(FieldCamera *camera);
// What the camera follows
const VecFx32 *FieldCamera_GetBind(FieldCamera *camera);
void FieldCamera_SetBind(FieldCamera *camera, const VecFx32 *bind);
// What the camera follows on a rail map
void FieldCamera_SetRefBind(FieldCamera *camera, const VecFx32 *bind);
const VecFx32 *FieldCamera_GetRefBind(FieldCamera *camera);
// The event camera's animations: start, animate to a target or back over frames, and end
void FieldCamera_EVCameraInit(FieldCamera *camera);
void FieldCameraAnm_EnsureInitDone(FieldCamera *camera);
void FieldCameraAnm_SetAnimation(FieldCamera *camera, const FieldEvCameraAnimationSetup *setup, u16 frames);
void FieldCameraAnm_SetAnimationRealTime(FieldCamera *camera, const FieldEvCameraAnimationSetup *setup, u16 frames);
void FieldCameraAnm_SetReturnAnimation(FieldCamera *camera, const FieldEvCameraAnimationFlags *flags, u16 frames);
void FieldCameraAnm_SetLoadDefaultsAnimation(FieldCamera *camera, u16 frames);
BOOL FieldCamera_IsAnimating(FieldCamera *camera);
void FieldCameraAnm_EVCameraEnd(FieldCamera *camera);
// How the camera shakes: the size of the shake along the screen's axes, how many swings are left of totalSteps, and
// how many frames each swing lasts. From the swing decayStart on, the size weakens by decayX and decayY every
// decayInterval frames. Our layout, from the task's code
typedef struct {
    s16 amplitudeX;
    s16 amplitudeY;
    u8 steps;
    u8 pad5;
    s16 decayX;
    s16 decayY;
    u16 decayStart;
    // The frames since the size last weakened
    u16 decayTimer;
    u16 decayInterval;
    u8 totalSteps;
    u8 pad11[3];
    u32 period;
    // The frame of the swing
    u32 frame;
    s16 delay;
    // A CAMERA_SHAKE_ value
    u8 mode;
    u8 pad1F;
} FieldCameraShakeSetup;
// What the shake moves: the camera's target and its eye offset, the target only, or the eye offset only
enum {
    CAMERA_SHAKE_BOTH,
    CAMERA_SHAKE_TARGET,
    CAMERA_SHAKE_EYE,
};

// Shakes the camera; the task ends when the shake does
FieldTask *FieldCameraShakeTask_Create(Field *field, const FieldCameraShakeSetup *setup);
// The event of Scrcmd_Camera_EvCameraShake and the League's lift, which runs the task with no delay
GameEvent *EventEvCameraShake_Create(GameSystem *gsys, FieldCameraShakeSetup *setup);
FieldCamera *FieldCamera_Create(u32 cameraIndex, u32 transformType, G3DCamera *g3dCamera, const VecFx32 *target,
                                HeapID heapId);
void FieldCamera_Free(FieldCamera *camera);
// The camera's pitch
u16 *FieldCamera_GetViewPitchPtr(FieldCamera *camera);
u16 *FieldCamera_GetViewYawPtr(FieldCamera *camera);
void FieldCameraBoundary_LoadDummy(FieldCamera *camera);
void FieldCameraBoundary_ChangeID(FieldCamera *camera, u16 boundaryId, HeapID heapId);

#endif // POKEBW2_FIELD_FIELD_CAMERA_H
