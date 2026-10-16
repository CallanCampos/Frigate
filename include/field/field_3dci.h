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
FieldParticle *func_ov036_021bb590(HeapID heapId);
ParticleSystem *func_ov036_021bb5d0(FieldParticle *particle);
void func_ov036_021bb60c(FieldParticle *particle);
void *FieldParticle_LoadResource(FieldParticle *particle, u32 arcId, u32 fileId);
void func_ov036_021bb630(FieldParticle *particle, void *resource, BOOL immediate, TCBManager *tcbMgr);
// Whether the particle system has no emitter left
BOOL func_ov036_021bb63c(FieldParticle *particle);
void func_ov036_021bb658(FieldParticle *particle);
void func_ov036_021bb674(void);

// The screen fades and sound effects of the field effects, which fld3d_ci.c's tables point to
extern const Fld3DCiSoundKey data_ov036_021d1784[1];
extern const Fld3DCiSoundKey data_ov036_021d178c[1];
extern const Fld3DCiSoundKey data_ov036_021d1794[1];
extern const Fld3DCiSoundKey data_ov036_021d179c[1];
extern const Fld3DCiSoundKey data_ov036_021d17a4[1];
extern const Fld3DCiSoundKey data_ov036_021d17ac[1];
extern const Fld3DCiSoundKey data_ov036_021d17b4[1];
extern const Fld3DCiSoundKey data_ov036_021d17bc[1];
extern const Fld3DCiSoundKey data_ov036_021d17c4[1];
extern const Fld3DCiSoundKey data_ov036_021d17cc[1];
extern const Fld3DCiFadeKey data_ov036_021d17d4[1];
extern const Fld3DCiFadeKey data_ov036_021d17e0[1];
extern const Fld3DCiFadeKey data_ov036_021d17ec[1];
extern const Fld3DCiFadeKey data_ov036_021d17f8[1];
extern const Fld3DCiFadeKey data_ov036_021d1804[1];
extern const Fld3DCiFadeKey data_ov036_021d1810[1];
extern const Fld3DCiFadeKey data_ov036_021d181c[1];
extern const Fld3DCiFadeKey data_ov036_021d1828[1];
extern const Fld3DCiFadeKey data_ov036_021d1834[1];
extern const Fld3DCiFadeKey data_ov036_021d1840[1];
extern const Fld3DCiFadeKey data_ov036_021d184c[1];
extern const Fld3DCiFadeKey data_ov036_021d1858[1];
extern const Fld3DCiFadeKey data_ov036_021d1864[1];
extern const Fld3DCiFadeKey data_ov036_021d1870[1];
extern const Fld3DCiFadeKey data_ov036_021d187c[1];
extern const Fld3DCiFadeKey data_ov036_021d1888[1];
extern const Fld3DCiFadeKey data_ov036_021d1894[1];
extern const Fld3DCiFadeKey data_ov036_021d18a0[1];
extern const Fld3DCiFadeKey data_ov036_021d18ac[1];
extern const Fld3DCiFadeKey data_ov036_021d18b8[1];
extern const Fld3DCiFadeKey data_ov036_021d18c4[1];
extern const Fld3DCiFadeKey data_ov036_021d18d0[1];
extern const Fld3DCiFadeKey data_ov036_021d18dc[1];
extern const Fld3DCiFadeKey data_ov036_021d18e8[1];
extern const Fld3DCiFadeKey data_ov036_021d18f4[1];
extern const Fld3DCiFadeKey data_ov036_021d1900[1];
extern const Fld3DCiFadeKey data_ov036_021d190c[1];
extern const Fld3DCiFadeKey data_ov036_021d1918[1];
extern const Fld3DCiFadeKey data_ov036_021d1924[1];
extern const Fld3DCiFadeKey data_ov036_021d1930[1];
extern const Fld3DCiFadeKey data_ov036_021d193c[1];
extern const Fld3DCiFadeKey data_ov036_021d1948[1];
extern const Fld3DCiFadeKey data_ov036_021d1954[1];
extern const Fld3DCiSoundKey data_ov036_021d1960[2];
extern const Fld3DCiSoundKey data_ov036_021d1970[2];
extern const Fld3DCiSoundKey data_ov036_021d1980[2];
extern const Fld3DCiSoundKey data_ov036_021d1990[2];
extern const Fld3DCiSoundKey data_ov036_021d19a0[2];
extern const Fld3DCiSoundKey data_ov036_021d19b0[2];
extern const Fld3DCiSoundKey data_ov036_021d19c0[2];
extern const Fld3DCiSoundKey data_ov036_021d19d0[2];
extern const Fld3DCiFadeKey data_ov036_021d19e0[2];
extern const Fld3DCiSoundKey data_ov036_021d19f8[3];
extern const Fld3DCiSoundKey data_ov036_021d1a10[3];
extern const Fld3DCiSoundKey data_ov036_021d1a28[3];
extern const Fld3DCiSoundKey data_ov036_021d1a40[3];
extern const Fld3DCiSoundKey data_ov036_021d1a58[4];
extern const Fld3DCiSoundKey data_ov036_021d1a78[4];
extern const Fld3DCiSoundKey data_ov036_021d1a98[4];
extern const Fld3DCiFadeKey data_ov036_021d1ab8[3];
extern const Fld3DCiSoundKey data_ov036_021d1adc[5];
extern const Fld3DCiSoundKey data_ov036_021d1b04[5];
extern const Fld3DCiSoundKey data_ov036_021d1b2c[5];
extern const Fld3DCiSoundKey data_ov036_021d1b54[5];
extern const Fld3DCiSoundKey data_ov036_021d1b7c[5];
extern const Fld3DCiSoundKey data_ov036_021d1ba4[6];
extern const Fld3DCiSoundKey data_ov036_021d1bd4[7];
extern const Fld3DCiSoundKey data_ov036_021d1c0c[7];
extern const Fld3DCiFadeKey data_ov036_021d1c44[5];

void Fld3DCi_Draw(void *ci);
void *Fld3DCi_Create(HeapID heapId, void *particleSystem);
void Fld3DCi_Free(void *ci);

#endif // POKEBW2_FIELD_FIELD_3DCI_H
