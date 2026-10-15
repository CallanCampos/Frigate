#ifndef POKEBW2_FIELD_SOUND_OBJ_H
#define POKEBW2_FIELD_SOUND_OBJ_H

#include "types.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/iss_3ds_sys.h"

// The field's sound emitters (sound_obj.c, overlay 36): a 3D sound unit of the ISS that follows an actor's matrix,
// which a motion curve can move. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// FieldSoundEmitter (struct_decls.h) is named after its functions

// An emitter for unit at matrix, which enables the unit with range and volume unless it already is
FieldSoundEmitter *FieldSoundEmitter_CreateAndSet(Field *field, SRTMatrix *matrix, ISS3DSoundUnitIndex unit, fx32 range,
                                                  s32 volume);
// An emitter without a sound, which only moves matrix along its curve
FieldSoundEmitter *FieldSoundEmitter_Create(Field *field, SRTMatrix *matrix);
void FieldSoundEmitter_Free(FieldSoundEmitter *emitter);
// Loads the curve of type from the archive's file to move the emitter along
void FieldSoundEmitter_BindMotionCurve(FieldSoundEmitter *emitter, u32 arcId, u32 fileId, u32 type);
// Steps the curve by step, looping, and moves the emitter; TRUE when the curve looped
BOOL FieldSoundEmitter_StepTransRot(FieldSoundEmitter *emitter, fx32 step);
void FieldSoundEmitter_SetAnimFrame(FieldSoundEmitter *emitter, fx32 frame);
fx32 FieldSoundEmitter_GetAnimFrame(FieldSoundEmitter *emitter);

#endif // POKEBW2_FIELD_SOUND_OBJ_H
