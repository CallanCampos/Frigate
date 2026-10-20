// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
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
const Fld3DCiSoundKey data_ov036_021d1960[2] = { { SEQ_SE_SW_FES_13, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1970[2] = { { SEQ_SE_SW_GIANTHOLE_10, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1980[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1990[2] = { { SEQ_SE_FLD_172, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19a0[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19b0[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19c0[2] = { { SEQ_SE_FLD_172, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19d0[2] = { { SEQ_SE_FLD_157, 1 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a10[3] = { { SEQ_SE_SW_MEDALGET, 1 }, { SEQ_SE_SW_MEDALGET_02, 30 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a28[3] = { { SEQ_SE_SW_KERTIO_FORM_04, 1 },
                                                 { SEQ_SE_SW_KERTIO_FORM_05, 31 },
                                                 { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a40[3] = { { SEQ_SE_FLD_166, 1 }, { SEQ_SE_SYS_89, 38 }, { -1 } };
const Fld3DCiFadeKey data_ov036_021d19e0[2] = { { 50, 12, 0, 16, 5 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d19f8[3] = { { SEQ_SE_FLD_152, 16 }, { SEQ_SE_SDEMO_03, 32 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1a58[4] = {
    { SEQ_SE_FLD_164, 2 }, { SEQ_SE_NDEMO_01, 12 }, { SEQ_SE_SDEMO_03, 21 }, { -1 }
};
const Fld3DCiSoundKey data_ov036_021d1a78[4] = {
    { SEQ_SE_SW_CI_PKWOOD_01, 6 }, { SEQ_SE_SW_CI_PKWOOD_02, 27 }, { SEQ_SE_SW_CI_PKWOOD_03, 28 }, { -1 }
};
const Fld3DCiSoundKey data_ov036_021d1a98[4] = {
    { SEQ_SE_ROTATION_B, 10 }, { SEQ_SE_SHDEMO_04, 38 }, { SEQ_SE_TDEMO_003, 91 }, { -1 }
};
const Fld3DCiFadeKey data_ov036_021d1ab8[3] = { { 1, 3, 0, 16, 10 }, { 10, 3, 16, 0, 20 }, { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b54[5] = {
    { SEQ_SE_W009_01, 1 }, { SEQ_SE_FLD_164, 22 }, { SEQ_SE_SHDEMO_04, 46 }, { SEQ_SE_FLD_127, 117 }, { -1 }
};
const Fld3DCiSoundKey data_ov036_021d1b7c[5] = {
    { SEQ_SE_FLD_164, 9 }, { SEQ_SE_FLD_164, 15 }, { SEQ_SE_TDEMO_006, 30 }, { SEQ_SE_W009_01, 49 }, { -1 }
};
const Fld3DCiSoundKey data_ov036_021d1adc[5] = { { SEQ_SE_SW_CI_WBT_01, 1 },
                                                 { SEQ_SE_SW_CI_WBT_02, 10 },
                                                 { SEQ_SE_SW_CI_WBT_03, 25 },
                                                 { SEQ_SE_SW_CI_WBT_04, 26 },
                                                 { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b04[5] = { { SEQ_SE_SW_GIANTHOLE_07_1, 1 },
                                                 { SEQ_SE_SW_GIANTHOLE_07_2, 2 },
                                                 { SEQ_SE_SW_GIANTHOLE_07_3, 70 },
                                                 { SEQ_SE_SW_GIANTHOLE_07_4, 71 },
                                                 { -1 } };
const Fld3DCiSoundKey data_ov036_021d1b2c[5] = { { SEQ_SE_SW_GIANTHOLE_08_1, 1 },
                                                 { SEQ_SE_SW_GIANTHOLE_08_2, 2 },
                                                 { SEQ_SE_SW_GIANTHOLE_08_3, 70 },
                                                 { SEQ_SE_SW_GIANTHOLE_08_4, 71 },
                                                 { -1 } };
const Fld3DCiSoundKey data_ov036_021d1ba4[6] = { { SEQ_SE_W010_01, 10 }, { SEQ_SE_W010_01, 27 }, { SEQ_SE_W010_01, 41 },
                                                 { SEQ_SE_W043_01, 52 }, { SEQ_SE_SYS_89, 75 },  { -1 } };
const Fld3DCiSoundKey data_ov036_021d1bd4[7] = { { SEQ_SE_TDEMO_006, 1 },
                                                 { SEQ_SE_BGDEMO_01, 3 },
                                                 { SEQ_SE_W319_01, 159 },
                                                 { SEQ_SE_BGDEMO_02, 208 },
                                                 { SEQ_SE_SYS_89, 272 },
                                                 { SEQ_SE_ROTATION_B, 316 },
                                                 { -1 } };
const Fld3DCiSoundKey data_ov036_021d1c0c[7] = { { SEQ_SE_TDEMO_006, 1 },
                                                 { SEQ_SE_BGDEMO_01, 3 },
                                                 { SEQ_SE_W319_01, 159 },
                                                 { SEQ_SE_BGDEMO_02, 208 },
                                                 { SEQ_SE_SYS_89, 272 },
                                                 { SEQ_SE_ROTATION_B, 316 },
                                                 { -1 } };
const Fld3DCiFadeKey data_ov036_021d1c44[5] = {
    { 90, 4, 0, 16, 1 }, { 93, 4, 16, 0, 5 }, { 165, 4, 0, 16, 5 }, { 170, 4, 16, 0, 5 }, { -1 }
};

FieldParticle *FieldParticle_Create(HeapID heapId) {
    FieldParticle *particle = GFL_HeapAllocate(heapId, sizeof(FieldParticle), TRUE, "fld_particle.c", 46);

    particle->heapId = heapId;
    Particle_Init(heapId);
    particle->work = GFL_HeapAllocate(heapId, 0x4800, TRUE, "fld_particle.c", 52);
    return particle;
}

ParticleSystem *FieldParticle_CreateSystem(FieldParticle *particle) {
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

void FieldParticle_DeleteSystem(FieldParticle *particle) {
    if (particle->system != NULL) {
        Particle_DeleteSystem(particle->system);
        particle->system = NULL;
    }
}

void *FieldParticle_LoadResource(FieldParticle *particle, u32 arcId, u32 fileId) {
    return Particle_ReadResource(arcId, fileId, particle->heapId);
}

void FieldParticle_SetResource(FieldParticle *particle, void *resource, BOOL immediate, TCBManager *tcbMgr) {
    Particle_SetResource(particle->system, resource, immediate, tcbMgr);
}

BOOL FieldParticle_IsFinished(FieldParticle *particle) {
    if (particle->system == NULL) {
        return TRUE;
    }
    if (Particle_GetEmitterCount(particle->system) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void FieldParticle_Free(FieldParticle *particle) {
    FieldParticle_DeleteSystem(particle);
    Particle_Free();
    GFL_HeapFree(particle->work);
    GFL_HeapFree(particle);
}

void FieldParticle_DrawAndUpdateAll(void) {
    Particle_DrawAndUpdateAll();
}
