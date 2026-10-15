#include "field/sound_obj.h"
#include "types.h"
#include "field/field.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "system/game_system.h"
#include "system/iss_3ds_sys.h"
#include "system/iss_sys.h"

// The field's sound emitters: a 3D sound unit that follows a matrix, which a motion curve can move. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

struct FieldSoundEmitter {
    HeapID heapId;
    Field *field;
    // NULL for an emitter without a sound
    ISS3DSoundSys *soundSys;
    ISS3DSoundUnitIndex unit;
    G3DCurve *curve;
    SRTMatrix *matrix;
};

static void FieldSoundEmitter_ApplyCurveLocation(FieldSoundEmitter *emitter);
static void FieldSoundEmitter_ApplyCurveRotation(FieldSoundEmitter *emitter);

FieldSoundEmitter *FieldSoundEmitter_CreateAndSet(Field *field, SRTMatrix *matrix, ISS3DSoundUnitIndex unit, fx32 range,
                                                  s32 volume) {
    HeapID heapId = Field_GetHeapID(field);
    ISS3DSoundSys *soundSys = ISS_Get3DSoundSys(GameSystem_GetISS(Field_GetGameSystem(field)));
    FieldSoundEmitter *emitter = GFL_HeapAllocate(heapId, sizeof(FieldSoundEmitter), FALSE, "sound_obj.c", 60);

    emitter->heapId = heapId;
    emitter->field = field;
    emitter->soundSys = soundSys;
    emitter->unit = unit;
    emitter->curve = NULL;
    emitter->matrix = matrix;
    if (!ISS3DSoundSys_IsUnitEnabled(soundSys, unit)) {
        ISS3DSoundSys_EnableUnit(soundSys, unit, range, volume);
    }
    return emitter;
}

FieldSoundEmitter *FieldSoundEmitter_Create(Field *field, SRTMatrix *matrix) {
    HeapID heapId = Field_GetHeapID(field);
    FieldSoundEmitter *emitter = GFL_HeapAllocate(heapId, sizeof(FieldSoundEmitter), FALSE, "sound_obj.c", 97);

    emitter->heapId = heapId;
    emitter->field = field;
    emitter->soundSys = NULL;
    emitter->curve = NULL;
    emitter->matrix = matrix;
    return emitter;
}

void FieldSoundEmitter_Free(FieldSoundEmitter *emitter) {
    if (emitter->curve != NULL) {
        GFL_G3DCurveFree(emitter->curve);
    }
    GFL_HeapFree(emitter);
}

void FieldSoundEmitter_BindMotionCurve(FieldSoundEmitter *emitter, u32 arcId, u32 fileId, u32 type) {
    if (emitter->curve != NULL) {
        GFL_G3DCurveFree(emitter->curve);
    }
    emitter->curve = GFL_G3DCurveLoadFileStream(emitter->heapId, arcId, fileId, type);
}

BOOL FieldSoundEmitter_StepTransRot(FieldSoundEmitter *emitter, fx32 step) {
    BOOL looped;

    if (emitter->curve == NULL) {
        return FALSE;
    }
    looped = GFL_G3DCurveFrameStepLoop(emitter->curve, step);
    FieldSoundEmitter_ApplyCurveLocation(emitter);
    FieldSoundEmitter_ApplyCurveRotation(emitter);
    return looped;
}

void FieldSoundEmitter_SetAnimFrame(FieldSoundEmitter *emitter, fx32 frame) {
    if (emitter->curve != NULL) {
        GFL_G3DCurveFrameSet(emitter->curve, frame);
        FieldSoundEmitter_ApplyCurveLocation(emitter);
    }
}

fx32 FieldSoundEmitter_GetAnimFrame(FieldSoundEmitter *emitter) {
    if (emitter->curve == NULL) {
        return 0;
    }
    return GFL_G3DCurveGetNowFrame(emitter->curve);
}

static void FieldSoundEmitter_ApplyCurveLocation(FieldSoundEmitter *emitter) {
    VecFx32 pos;

    if (emitter->curve != NULL && GFL_G3DCurveGetNowTranslation(emitter->curve, &pos) == TRUE) {
        VEC_Set(&emitter->matrix->translation, pos.x, pos.y, pos.z);
        if (emitter->soundSys != NULL) {
            ISS3DSoundSys_SetUnitLocation(emitter->soundSys, emitter->unit, &pos);
        }
    }
}

// The curve's rotation is in fixed-point degrees
static void FieldSoundEmitter_ApplyCurveRotation(FieldSoundEmitter *emitter) {
    VecFx32 rot;
    f32 x;
    f32 y;
    f32 z;
    u16 rotX;
    u16 rotY;
    u16 rotZ;

    if (emitter->curve != NULL && GFL_G3DCurveGetNowRotation(emitter->curve, &rot) == TRUE) {
        x = rot.x / 4096.0f;
        y = rot.y / 4096.0f;
        z = rot.z / 4096.0f;
        while (x < 0.0f) {
            x += 360.0f;
        }
        while (y < 0.0f) {
            y += 360.0f;
        }
        while (z < 0.0f) {
            z += 360.0f;
        }
        rotX = x / 360.0f * 65535.0f;
        rotY = y / 360.0f * 65535.0f;
        rotZ = z / 360.0f * 65535.0f;
        MTX_Identity33_(&emitter->matrix->rotation);
        MAT3_RotationEulerZYX(rotX, rotY, rotZ, &emitter->matrix->rotation);
    }
}
