#ifndef POKEBW2_FIELD_FIELD_3DCI_H
#define POKEBW2_FIELD_FIELD_3DCI_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "gfl/tcb.h"
#include "struct_decls.h"

// Overlay 36's fld_particle.c and fld3d_ci.c: the field's particle system, and the field effects that run 3D scenes
// with its particles. Our layouts

// A screen fade of a field effect: at a frame, the brightness of the screens of flags fades from one level to
// another over frames. A frame of -1 ends a list
typedef struct {
    u32 frame;
    u16 flags;
    s16 from;
    s16 to;
    u8 frames;
} Fld3DCiFadeKey;

// A sound effect of a field effect, at a frame. A sound effect of -1 ends a list
typedef struct {
    u32 se;
    u32 frame;
} Fld3DCiSoundKey;

// fld_particle.c: the field's particle system
FieldParticle *FieldParticle_Create(HeapID heapId);
ParticleSystem *FieldParticle_CreateSystem(FieldParticle *particle);
void FieldParticle_DeleteSystem(FieldParticle *particle);
void *FieldParticle_LoadResource(FieldParticle *particle, u32 arcId, u32 fileId);
void FieldParticle_SetResource(FieldParticle *particle, void *resource, BOOL immediate, TCBManager *tcbMgr);
// Whether the particle system has no emitter left
BOOL FieldParticle_IsFinished(FieldParticle *particle);
void FieldParticle_Free(FieldParticle *particle);
void FieldParticle_DrawAndUpdateAll(void);

// The screen fades and sound effects of the field effects, which fld3d_ci.c's tables point to, numbered by their slots
// there
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_25[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_24[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_15[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_16[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_21[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_00[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_01[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_02[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_20[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_23[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_09[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_15[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_13[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_18[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_31[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_33[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_03[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_21[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_08[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_19[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_26[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_30[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_16[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_23[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_29[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_25[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_12[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_17[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_24[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_34[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_05[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_07[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_35[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_14[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_04[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_27[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_02[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_01[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_22[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_11[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_32[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_20[1];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_10[1];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_26[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_28[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_04[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_05[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_11[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_12[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_13[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_14[2];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_28[2];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_19[3];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_27[3];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_35[3];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_10[3];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_17[4];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_30[4];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_07[4];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_00[3];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_29[5];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_31[5];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_32[5];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_08[5];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_09[5];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_18[6];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_22[7];
extern const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_03[7];
extern const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_06[5];

// fld3d_ci.c: the field effects
Fld3DCi *Fld3DCi_Create(HeapID heapId, FieldParticle *particleSystem);
void Fld3DCi_Free(Fld3DCi *ci);
void Fld3DCi_Draw(Fld3DCi *ci);

#endif // POKEBW2_FIELD_FIELD_3DCI_H
