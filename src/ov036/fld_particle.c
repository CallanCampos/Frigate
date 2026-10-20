#include "types.h"
#include "field/field_3dci.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"

// The field's particle system (fld_particle.c, after the file name that its allocations pass), which the field effects
// of fld3d_ci.c emit their particles from. The layout is ours

struct FieldParticle {
    u32 unk0;
    u16 heapId;
    void *work;
    ParticleSystem *system;
};

// The sound effects and the screen fades of fld3d_ci.c's field effects, by the frame they start at, which its tables
// of each effect's keys point to. MWCC places data of the same size in a scrambled order, so these are defined in the
// order that lays them out by address
const Fld3DCiSoundKey data_ov036_021d17c4[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d17a4[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d17bc[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d179c[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d178c[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d1794[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d1784[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d17b4[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d17ac[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d17cc[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1870[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d17f8[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18dc[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1894[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1954[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d17d4[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1804[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d187c[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1858[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1828[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18d0[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1864[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d17e0[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1810[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d181c[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d184c[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1948[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1834[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1840[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18c4[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1918[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d190c[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18e8[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18f4[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1900[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1924[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1930[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d193c[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18b8[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18ac[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d18a0[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d17ec[1] = { { -1 } };
const Fld3DCiFadeKey data_ov036_021d1888[1] = { { -1 } };
const Fld3DCiSoundKey data_ov036_021d1960[2] = { { 0x84f, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1970[2] = { { 0x8fe, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1980[2] = { { 0x672, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1990[2] = { { 0x7eb, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19a0[2] = { { 0x672, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19b0[2] = { { 0x672, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19c0[2] = { { 0x7eb, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19d0[2] = { { 0x7c5, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a10[3] = { { 0x8d2, 1 }, { 0x8d3, 30 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a28[3] = { { 0x8df, 1 }, { 0x8e0, 31 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a40[3] = { { 0x7d5, 1 }, { 0x794, 38 }, { -1 } };
const Fld3DCiFadeKey data_ov036_021d19e0[2] = { { 50, 12, 0, 16, 5 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19f8[3] = { { 0x7b6, 16 }, { 0x7b7, 32 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a58[4] = { { 0x7cd, 2 }, { 0x796, 12 }, { 0x7b7, 21 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a78[4] = { { 0x8ec, 6 }, { 0x8ed, 27 }, { 0x8ee, 28 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a98[4] = { { 0x57a, 10 }, { 0x7de, 38 }, { 0x786, 91 }, { -1 } };
const Fld3DCiFadeKey data_ov036_021d1ab8[3] = { { 1, 3, 0, 16, 10 }, { 10, 3, 16, 0, 20 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b54[5] = { { 0x5f2, 1 }, { 0x7cd, 22 }, { 0x7de, 46 }, { 0x766, 117 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b7c[5] = { { 0x7cd, 9 }, { 0x7cd, 15 }, { 0x789, 30 }, { 0x5f2, 49 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1adc[5] = { { 0x8e8, 1 }, { 0x8e9, 10 }, { 0x8ea, 25 }, { 0x8eb, 26 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b04[5] = { { 0x8f5, 1 }, { 0x8f6, 2 }, { 0x8f7, 70 }, { 0x8f8, 71 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b2c[5] = { { 0x8f9, 1 }, { 0x8fa, 2 }, { 0x8fb, 70 }, { 0x8fc, 71 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1ba4[6] = { { 0x58f, 10 }, { 0x58f, 27 }, { 0x58f, 41 },
                                                 { 0x5a9, 52 }, { 0x794, 75 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1bd4[7] = { { 0x789, 1 },   { 0x7d9, 3 },   { 0x5e5, 159 }, { 0x7da, 208 },
                                                 { 0x794, 272 }, { 0x57a, 316 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1c0c[7] = { { 0x789, 1 },   { 0x7d9, 3 },   { 0x5e5, 159 }, { 0x7da, 208 },
                                                 { 0x794, 272 }, { 0x57a, 316 }, { -1 } };
const Fld3DCiFadeKey data_ov036_021d1c44[5] = {
    { 90, 4, 0, 16, 1 }, { 93, 4, 16, 0, 5 }, { 165, 4, 0, 16, 5 }, { 170, 4, 16, 0, 5 }, { -1 }
};

FieldParticle *func_ov036_021bb590(HeapID heapId) {
    FieldParticle *particle = GFL_HeapAllocate(heapId, sizeof(FieldParticle), TRUE, "fld_particle.c", 46);

    particle->heapId = heapId;
    Particle_Init(heapId);
    particle->work = GFL_HeapAllocate(heapId, 0x4800, TRUE, "fld_particle.c", 52);
    return particle;
}

ParticleSystem *func_ov036_021bb5d0(FieldParticle *particle) {
    G3DCamera *camera;
    fx32 zNear;
    fx32 zFar;

    particle->system = Particle_CreateSystem(particle->work, 0x4800, TRUE, particle->heapId);
    camera = Particle_GetCamera(particle->system);
    zNear = FX32_ONE;
    zFar = FX32_CONST(900);
    GFL_G3DCameraSetProjectionZNear(camera, &zNear);
    GFL_G3DCameraSetProjectionZFar(camera, &zFar);
    return particle->system;
}

void func_ov036_021bb60c(FieldParticle *particle) {
    if (particle->system != NULL) {
        Particle_DeleteSystem(particle->system);
        particle->system = NULL;
    }
}

void *FieldParticle_LoadResource(FieldParticle *particle, u32 arcId, u32 fileId) {
    return Particle_ReadResource(arcId, fileId, particle->heapId);
}

void func_ov036_021bb630(FieldParticle *particle, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    Particle_SetResource(particle->system, resource, immediate, tcbMgr);
}

BOOL func_ov036_021bb63c(FieldParticle *particle) {
    if (particle->system == NULL) {
        return TRUE;
    }
    if (Particle_GetEmitterCount(particle->system) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void func_ov036_021bb658(FieldParticle *particle) {
    func_ov036_021bb60c(particle);
    Particle_Free();
    GFL_HeapFree(particle->work);
    GFL_HeapFree(particle);
}

void func_ov036_021bb674(void) {
    Particle_DrawAndUpdateAll();
}
