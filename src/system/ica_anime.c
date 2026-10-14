#include "types.h"
#include "gfl/arc.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

// Curves: animations of a translation, a rotation and a scale that are read from a file of keyframes, all at once or
// a few frames at a time, and that can drive a camera. The name is the ROM's string. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except GFL_G3DCurveCreateToLoadBufferEntire and the static
// ones

// The file: the frame count, a byte each for whether it has a scale, a rotation and a translation, a pad byte, then
// for each frame the three floats of each value it has, in that order
#define CURVE_FILE_FRAME_COUNT 0
#define CURVE_FILE_HAS_SCALE 4
#define CURVE_FILE_HAS_ROTATION 5
#define CURVE_FILE_HAS_TRANSLATION 6
#define CURVE_FILE_FRAMES 8

struct G3DCurve {
    u16 heapId;
    ArcTool *arc;
    u32 fileId;
    u32 frameCount;
    fx32 frame;
    // The frames that are loaded: the first one and how many
    int loadedFrame;
    int loadedCount;
    VecFx32 *translations;
    VecFx32 *rotations;
    VecFx32 *scales;
    BOOL hasTranslation;
    BOOL hasRotation;
    BOOL hasScale;
    // Whether the curve allocated its frame buffers
    BOOL ownsBuffers;
};

static void GFL_G3DCurveReset(G3DCurve *curve);
static void GFL_G3DCurveLoadHeader(G3DCurve *curve, u32 arcId, u32 fileId, HeapID heapId);
static void GFL_G3DCurveAllocPreload(G3DCurve *curve, int count);
static BOOL GFL_G3DCurveSetLoadBuffer(G3DCurve *curve, int count, void *buffer, int bufferSize);
static void GFL_G3DCurvePreloadFrames(G3DCurve *curve, fx32 frame);

G3DCurve *GFL_G3DCurveCreateToLoadBufferEntire(HeapID heapId, u32 arcId, u16 fileId, void *buffer, u32 bufferSize) {
    G3DCurve *curve = GFL_HeapAllocate(heapId, sizeof(G3DCurve), FALSE, "ica_anime.c", 84);

    GFL_G3DCurveReset(curve);
    GFL_G3DCurveLoadHeader(curve, arcId, fileId, heapId);
    if (!GFL_G3DCurveSetLoadBuffer(curve, curve->frameCount, buffer, bufferSize)) {
        return NULL;
    }
    GFL_G3DCurvePreloadFrames(curve, 0);
    return curve;
}

G3DCurve *GFL_G3DCurveCreateToLoadBuffer(HeapID heapId, u32 arcId, u32 fileId, u32 preloadCount, void *buffer,
                                         u32 bufferSize) {
    G3DCurve *curve = GFL_HeapAllocate(heapId, sizeof(G3DCurve), FALSE, "ica_anime.c", 116);

    GFL_G3DCurveReset(curve);
    GFL_G3DCurveLoadHeader(curve, arcId, fileId, heapId);
    if (!GFL_G3DCurveSetLoadBuffer(curve, preloadCount, buffer, bufferSize)) {
        return NULL;
    }
    GFL_G3DCurvePreloadFrames(curve, 0);
    return curve;
}

G3DCurve *GFL_G3DCurveLoadFileAll(HeapID heapId, u32 arcId, u32 fileId) {
    G3DCurve *curve = GFL_HeapAllocate(heapId, sizeof(G3DCurve), FALSE, "ica_anime.c", 144);

    GFL_G3DCurveReset(curve);
    GFL_G3DCurveLoadHeader(curve, arcId, fileId, heapId);
    GFL_G3DCurveAllocPreload(curve, curve->frameCount);
    GFL_G3DCurvePreloadFrames(curve, 0);
    return curve;
}

G3DCurve *GFL_G3DCurveLoadFileStream(HeapID heapId, u32 arcId, u32 fileId, u32 preloadCount) {
    G3DCurve *curve = GFL_HeapAllocate(heapId, sizeof(G3DCurve), FALSE, "ica_anime.c", 174);

    GFL_G3DCurveReset(curve);
    GFL_G3DCurveLoadHeader(curve, arcId, fileId, heapId);
    GFL_G3DCurveAllocPreload(curve, preloadCount);
    GFL_G3DCurvePreloadFrames(curve, 0);
    return curve;
}

void GFL_G3DCurveFree(G3DCurve *curve) {
    GFL_ArcToolFree(curve->arc);
    if (curve->ownsBuffers) {
        if (curve->translations != NULL) {
            GFL_HeapFree(curve->translations);
        }
        if (curve->rotations != NULL) {
            GFL_HeapFree(curve->rotations);
        }
        if (curve->scales != NULL) {
            GFL_HeapFree(curve->scales);
        }
    }
    GFL_HeapFree(curve);
}

BOOL GFL_G3DCurveFrameStepLoop(G3DCurve *curve, fx32 step) {
    BOOL looped = FALSE;
    int frame;

    curve->frame += step;
    frame = curve->frame >> FX32_SHIFT;
    if (curve->frameCount <= frame) {
        curve->frame = 0;
        frame = 0;
        looped = TRUE;
    }
    if (frame < curve->loadedFrame || curve->loadedFrame + curve->loadedCount <= frame) {
        GFL_G3DCurvePreloadFrames(curve, curve->frame);
    }
    return looped;
}

BOOL GFL_G3DCurveFrameStep(G3DCurve *curve, fx32 step) {
    BOOL ended = FALSE;
    int frame;

    curve->frame += step;
    if ((curve->frame >> FX32_SHIFT) >= curve->frameCount) {
        curve->frame = (curve->frameCount - 1) << FX32_SHIFT;
        ended = TRUE;
    }
    frame = curve->frame >> FX32_SHIFT;
    if (frame < curve->loadedFrame || curve->loadedFrame + curve->loadedCount <= frame) {
        GFL_G3DCurvePreloadFrames(curve, curve->frame);
    }
    return ended;
}

void GFL_G3DCurveFrameSet(G3DCurve *curve, fx32 frame) {
    int index = frame >> FX32_SHIFT;

    curve->frame = frame;
    if (index < curve->loadedFrame || curve->loadedFrame + curve->loadedCount <= index) {
        GFL_G3DCurvePreloadFrames(curve, frame);
    }
}

u32 GFL_G3DCurveGetFrameCount(G3DCurve *curve) {
    return curve->frameCount;
}

fx32 GFL_G3DCurveGetNowFrame(G3DCurve *curve) {
    return curve->frame;
}

BOOL GFL_G3DCurveGetNowTranslation(G3DCurve *curve, VecFx32 *translation) {
    if (curve->hasTranslation) {
        *translation = curve->translations[(curve->frame >> FX32_SHIFT) - curve->loadedFrame];
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DCurveGetNowRotation(G3DCurve *curve, VecFx32 *rotation) {
    if (curve->hasRotation) {
        *rotation = curve->rotations[(curve->frame >> FX32_SHIFT) - curve->loadedFrame];
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DCurveGetNowTranslationLoop(const G3DCurve *curve, VecFx32 *translation, u32 frame) {
    frame %= curve->frameCount;
    if (curve->loadedCount == curve->frameCount) {
        translation->x = curve->translations[frame].x;
        translation->y = curve->translations[frame].y;
        translation->z = curve->translations[frame].z;
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DCurveGetNowRotationLoop(const G3DCurve *curve, VecFx32 *rotation, u32 frame) {
    if (!curve->hasRotation) {
        return FALSE;
    }
    frame %= curve->frameCount;
    if (curve->loadedCount == curve->frameCount) {
        rotation->x = curve->rotations[frame].x;
        rotation->y = curve->rotations[frame].y;
        rotation->z = curve->rotations[frame].z;
        return TRUE;
    }
    return FALSE;
}

BOOL GFL_G3DCurveGetNowScaleLoop(const G3DCurve *curve, VecFx32 *scale, u32 frame) {
    if (!curve->hasScale) {
        return FALSE;
    }
    frame %= curve->frameCount;
    if (curve->loadedCount == curve->frameCount) {
        scale->x = curve->scales[frame].x;
        scale->y = curve->scales[frame].y;
        scale->z = curve->scales[frame].z;
        return TRUE;
    }
    return FALSE;
}

static void GFL_G3DCurveReset(G3DCurve *curve) {
    curve->heapId = 0;
    curve->arc = NULL;
    curve->fileId = 0;
    curve->frameCount = 0;
    curve->frame = 0;
    curve->loadedFrame = 0;
    curve->loadedCount = 0;
    curve->translations = NULL;
    curve->rotations = NULL;
    curve->scales = NULL;
    curve->hasTranslation = FALSE;
    curve->hasRotation = FALSE;
    curve->hasScale = FALSE;
    curve->ownsBuffers = FALSE;
}

static void GFL_G3DCurveLoadHeader(G3DCurve *curve, u32 arcId, u32 fileId, HeapID heapId) {
    u8 hasScale;
    u8 hasRotation;
    u8 hasTranslation;

    curve->heapId = heapId;
    curve->arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
    curve->fileId = fileId;
    GFL_ArcToolReadRange(curve->arc, fileId, CURVE_FILE_FRAME_COUNT, sizeof(u32), &curve->frameCount);
    GFL_ArcToolReadRange(curve->arc, curve->fileId, CURVE_FILE_HAS_SCALE, sizeof(u8), &hasScale);
    GFL_ArcToolReadRange(curve->arc, curve->fileId, CURVE_FILE_HAS_ROTATION, sizeof(u8), &hasRotation);
    GFL_ArcToolReadRange(curve->arc, curve->fileId, CURVE_FILE_HAS_TRANSLATION, sizeof(u8), &hasTranslation);
    curve->hasScale = hasScale ? TRUE : FALSE;
    curve->hasRotation = hasRotation ? TRUE : FALSE;
    curve->hasTranslation = hasTranslation ? TRUE : FALSE;
}

// Allocates the buffers of count frames
static void GFL_G3DCurveAllocPreload(G3DCurve *curve, int count) {
    curve->loadedCount = count;
    if (curve->hasTranslation) {
        curve->translations = GFL_HeapAllocate(curve->heapId, count * sizeof(VecFx32), FALSE, "ica_anime.c", 619);
    }
    if (curve->hasRotation) {
        curve->rotations = GFL_HeapAllocate(curve->heapId, count * sizeof(VecFx32), FALSE, "ica_anime.c", 623);
    }
    if (curve->hasScale) {
        curve->scales = GFL_HeapAllocate(curve->heapId, count * sizeof(VecFx32), FALSE, "ica_anime.c", 627);
    }
    curve->ownsBuffers = TRUE;
}

// Splits the caller's buffer into the buffers of count frames. Returns FALSE when it is too small
static BOOL GFL_G3DCurveSetLoadBuffer(G3DCurve *curve, int count, void *buffer, int bufferSize) {
    int total = 0;
    int size = count * sizeof(VecFx32);
    int index = 0;

    if (curve->hasTranslation) {
        total += size;
    }
    if (curve->hasRotation) {
        total += size;
    }
    if (curve->hasScale) {
        total += size;
    }
    if (bufferSize < total) {
        return FALSE;
    }
    if (curve->hasTranslation) {
        curve->translations = buffer;
        index++;
    }
    if (curve->hasRotation) {
        curve->rotations = (VecFx32 *)((u8 *)buffer + size * index);
        index++;
    }
    if (curve->hasScale) {
        curve->scales = (VecFx32 *)((u8 *)buffer + size * index);
    }
    curve->loadedCount = count;
    return TRUE;
}

// Reads the frames from the one given into the buffers, as fixed-point numbers
static void GFL_G3DCurvePreloadFrames(G3DCurve *curve, fx32 frame) {
    int first = frame >> FX32_SHIFT;
    u32 index = first % curve->frameCount;
    u32 offset = CURVE_FILE_FRAMES;
    u32 size = 0;
    int i = 0;
    f32 *values;
    int value;

    if (curve->hasScale) {
        offset += index * sizeof(VecFx32);
    }
    if (curve->hasRotation) {
        offset += index * sizeof(VecFx32);
    }
    if (curve->hasTranslation) {
        offset += index * sizeof(VecFx32);
    }
    if (curve->hasScale) {
        size += curve->loadedCount * sizeof(VecFx32);
    }
    if (curve->hasRotation) {
        size += curve->loadedCount * sizeof(VecFx32);
    }
    if (curve->hasTranslation) {
        size += curve->loadedCount * sizeof(VecFx32);
    }
    values = GFL_HeapAllocate(HEAPID_TAIL(curve->heapId), size, FALSE, "ica_anime.c", 716);
    GFL_ArcToolReadRange(curve->arc, curve->fileId, offset, size, values);
    value = 0;
    for (; i < curve->loadedCount; i++) {
        if (curve->hasScale) {
            curve->scales[i].x = FX32_CONST(values[value]);
            curve->scales[i].y = FX32_CONST(values[value + 1]);
            curve->scales[i].z = FX32_CONST(values[value + 2]);
            value += 3;
        }
        if (curve->hasRotation) {
            curve->rotations[i].x = FX32_CONST(values[value]);
            curve->rotations[i].y = FX32_CONST(values[value + 1]);
            curve->rotations[i].z = FX32_CONST(values[value + 2]);
            value += 3;
        }
        if (curve->hasTranslation) {
            curve->translations[i].x = FX32_CONST(values[value]);
            curve->translations[i].y = FX32_CONST(values[value + 1]);
            curve->translations[i].z = FX32_CONST(values[value + 2]);
            value += 3;
        }
    }
    GFL_HeapFree(values);
    curve->loadedFrame = first;
}

void GFL_G3DCurveApplyCamera(G3DCamera *camera, G3DCurve *curve) {
    MtxFx33 matrix;
    VecFx32 rotation;
    VecFx32 forward = { 0, 0, -FX32_ONE };
    VecFx32 up = { 0, FX32_ONE, 0 };
    VecFx32 position;
    VecFx32 target;
    VecFx32 direction;
    VecFx32 upVector;
    f32 x;
    f32 y;
    f32 z;
    u16 angleX;
    u16 angleY;
    u16 angleZ;

    // The rotation is in degrees
    GFL_G3DCurveGetNowRotation(curve, &rotation);
    x = FX_FX32_TO_F32(rotation.x);
    y = FX_FX32_TO_F32(rotation.y);
    z = FX_FX32_TO_F32(rotation.z);
    while (x < 0.0f) {
        x += 360.0f;
    }
    while (y < 0.0f) {
        y += 360.0f;
    }
    while (z < 0.0f) {
        z += 360.0f;
    }
    angleX = 65535.0f * (x / 360.0f);
    angleY = 65535.0f * (y / 360.0f);
    angleZ = 65535.0f * (z / 360.0f);
    MAT3_RotationEulerZYX(angleX, angleY, angleZ, &matrix);
    MTX_MultVec33(&forward, &matrix, &direction);
    MTX_MultVec33(&up, &matrix, &upVector);
    GFL_G3DCurveGetNowTranslation(curve, &position);
    VEC_Add(&position, &direction, &target);
    GFL_G3DCameraSetLookatPos(camera, &position);
    GFL_G3DCameraSetLookatTarget(camera, &target);
    GFL_G3DCameraSetLookatUpVector(camera, &upVector);
}

void GFL_G3DCurveApplyCameraPosTranslation(G3DCamera *camera, G3DCurve *curve) {
    VecFx32 translation;

    GFL_G3DCurveGetNowTranslation(curve, &translation);
    GFL_G3DCameraSetLookatPos(camera, &translation);
}

void GFL_G3DCurveApplyCameraTgtTranslation(G3DCamera *camera, G3DCurve *curve) {
    VecFx32 translation;

    GFL_G3DCurveGetNowTranslation(curve, &translation);
    GFL_G3DCameraSetLookatTarget(camera, &translation);
}
