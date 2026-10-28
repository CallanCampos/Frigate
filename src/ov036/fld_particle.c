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
// of each effect's keys point to and which are numbered by their slots there. MWCC places data of the same size in a
// scrambled order, so these are defined in the order that lays them out by address
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_20[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_21[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_02[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_16[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_24[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_15[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_25[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_01[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_00[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_23[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_23[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_18[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_35[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_12[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_10[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_09[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_31[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_29[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_30[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_21[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_07[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_16[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_15[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_33[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_03[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_26[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_20[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_08[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_19[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_05[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_01[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_02[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_14[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_04[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_27[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_22[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_11[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_32[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_34[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_24[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_17[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_13[1] = { { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_25[1] = { { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_26[2] = { { SEQ_SE_SW_FES_13, 1 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_28[2] = { { SEQ_SE_SW_GIANTHOLE_10, 1 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_04[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_05[2] = { { SEQ_SE_FLD_172, 1 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_11[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_12[2] = { { SEQ_SE_FLD_01, 24 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_13[2] = { { SEQ_SE_FLD_172, 1 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_14[2] = { { SEQ_SE_FLD_157, 1 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_27[3] = { { SEQ_SE_SW_MEDALGET, 1 }, { SEQ_SE_SW_MEDALGET_02, 30 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_35[3] = { { SEQ_SE_SW_KERTIO_FORM_04, 1 },
                                                   { SEQ_SE_SW_KERTIO_FORM_05, 31 },
                                                   { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_10[3] = { { SEQ_SE_FLD_166, 1 }, { SEQ_SE_SYS_89, 38 }, { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_28[2] = { { 50, 12, 0, 16, 5 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_19[3] = { { SEQ_SE_FLD_152, 16 }, { SEQ_SE_SDEMO_03, 32 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_17[4] = {
    { SEQ_SE_FLD_164, 2 }, { SEQ_SE_NDEMO_01, 12 }, { SEQ_SE_SDEMO_03, 21 }, { -1 }
};
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_30[4] = {
    { SEQ_SE_SW_CI_PKWOOD_01, 6 }, { SEQ_SE_SW_CI_PKWOOD_02, 27 }, { SEQ_SE_SW_CI_PKWOOD_03, 28 }, { -1 }
};
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_07[4] = {
    { SEQ_SE_ROTATION_B, 10 }, { SEQ_SE_SHDEMO_04, 38 }, { SEQ_SE_TDEMO_003, 91 }, { -1 }
};
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_00[3] = { { 1, 3, 0, 16, 10 }, { 10, 3, 16, 0, 20 }, { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_08[5] = {
    { SEQ_SE_W009_01, 1 }, { SEQ_SE_FLD_164, 22 }, { SEQ_SE_SHDEMO_04, 46 }, { SEQ_SE_FLD_127, 117 }, { -1 }
};
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_09[5] = {
    { SEQ_SE_FLD_164, 9 }, { SEQ_SE_FLD_164, 15 }, { SEQ_SE_TDEMO_006, 30 }, { SEQ_SE_W009_01, 49 }, { -1 }
};
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_29[5] = { { SEQ_SE_SW_CI_WBT_01, 1 },
                                                   { SEQ_SE_SW_CI_WBT_02, 10 },
                                                   { SEQ_SE_SW_CI_WBT_03, 25 },
                                                   { SEQ_SE_SW_CI_WBT_04, 26 },
                                                   { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_31[5] = { { SEQ_SE_SW_GIANTHOLE_07_1, 1 },
                                                   { SEQ_SE_SW_GIANTHOLE_07_2, 2 },
                                                   { SEQ_SE_SW_GIANTHOLE_07_3, 70 },
                                                   { SEQ_SE_SW_GIANTHOLE_07_4, 71 },
                                                   { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_32[5] = { { SEQ_SE_SW_GIANTHOLE_08_1, 1 },
                                                   { SEQ_SE_SW_GIANTHOLE_08_2, 2 },
                                                   { SEQ_SE_SW_GIANTHOLE_08_3, 70 },
                                                   { SEQ_SE_SW_GIANTHOLE_08_4, 71 },
                                                   { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_18[6] = { { SEQ_SE_W010_01, 10 }, { SEQ_SE_W010_01, 27 },
                                                   { SEQ_SE_W010_01, 41 }, { SEQ_SE_W043_01, 52 },
                                                   { SEQ_SE_SYS_89, 75 },  { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_22[7] = { { SEQ_SE_TDEMO_006, 1 },
                                                   { SEQ_SE_BGDEMO_01, 3 },
                                                   { SEQ_SE_W319_01, 159 },
                                                   { SEQ_SE_BGDEMO_02, 208 },
                                                   { SEQ_SE_SYS_89, 272 },
                                                   { SEQ_SE_ROTATION_B, 316 },
                                                   { -1 } };
const Fld3DCiSoundKey FLD3DCI_SOUND_KEYS_03[7] = { { SEQ_SE_TDEMO_006, 1 },
                                                   { SEQ_SE_BGDEMO_01, 3 },
                                                   { SEQ_SE_W319_01, 159 },
                                                   { SEQ_SE_BGDEMO_02, 208 },
                                                   { SEQ_SE_SYS_89, 272 },
                                                   { SEQ_SE_ROTATION_B, 316 },
                                                   { -1 } };
const Fld3DCiFadeKey FLD3DCI_FADE_KEYS_06[5] = {
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
