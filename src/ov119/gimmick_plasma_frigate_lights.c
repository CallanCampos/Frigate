#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_plasma_frigate_lights.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1

#define SWITCH_COUNT 4

// The models of the scene, by the first of each group
enum {
    MODEL_SWITCH = 0,
    MODEL_LIGHT = 4,
};

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
} PlasmaFrigateLightsWork;

static void PlasmaFrigateLights_LoadScene(PlasmaFrigateLightsWork *work);
static void PlasmaFrigateLights_FreeScene(PlasmaFrigateLightsWork *work);
static void PlasmaFrigateLights_StepAnimations(PlasmaFrigateLightsWork *work);
static void PlasmaFrigateLights_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos);
static void PlasmaFrigateLights_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void PlasmaFrigateLights_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void PlasmaFrigateLights_PlaySwitchSecond(PlasmaFrigateLightsWork *work, u16 index);
static void PlasmaFrigateLights_ShowSwitchFirstEnd(PlasmaFrigateLightsWork *work, u16 index);
static void PlasmaFrigateLights_ShowSwitchSecondEnd(PlasmaFrigateLightsWork *work, u16 index);
static void PlasmaFrigateLights_PlayLightSecond(PlasmaFrigateLightsWork *work, u16 index);
static void PlasmaFrigateLights_ShowLightSecondEnd(PlasmaFrigateLightsWork *work, u16 index);
static void PlasmaFrigateLights_LoopLightFirst(PlasmaFrigateLightsWork *work, u16 index);

static const G3DSceneResourceSetup sResources[6] = {
    { ARCID_GIMMICK_PLASMA_FRIGATE, 0, 0 }, { ARCID_GIMMICK_PLASMA_FRIGATE, 1, 0 },
    { ARCID_GIMMICK_PLASMA_FRIGATE, 2, 0 }, { ARCID_GIMMICK_PLASMA_FRIGATE, 3, 0 },
    { ARCID_GIMMICK_PLASMA_FRIGATE, 4, 0 }, { ARCID_GIMMICK_PLASMA_FRIGATE, 5, 0 },
};

static const G3DSceneAnimationSetup sSwitchAnimations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sLightAnimations[] = { { 4, 0 }, { 5, 0 } };

static const G3DSceneActorSetup sActors[8] = {
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 3, 0, 3, 0, sLightAnimations, NELEMS(sLightAnimations) },
    { 3, 0, 3, 0, sLightAnimations, NELEMS(sLightAnimations) },
    { 3, 0, 3, 0, sLightAnimations, NELEMS(sLightAnimations) },
    { 3, 0, 3, 0, sLightAnimations, NELEMS(sLightAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void PlasmaFrigateLights_Init(Field *field) {
    PlasmaFrigateLightsWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(PlasmaFrigateLightsWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    PlasmaFrigateLights_LoadScene(work);
}

void PlasmaFrigateLights_End(Field *field) {
    PlasmaFrigateLights_FreeScene(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void PlasmaFrigateLights_Update(Field *field) {
    PlasmaFrigateLights_StepAnimations(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

void PlasmaFrigateLights_ShowSwitchState(GameSystem *gsys, int index, BOOL second) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (second == FALSE) {
        PlasmaFrigateLights_LoopLightFirst(work, index);
        PlasmaFrigateLights_ShowSwitchFirstEnd(work, index);
    } else {
        PlasmaFrigateLights_ShowLightSecondEnd(work, index);
        PlasmaFrigateLights_ShowSwitchSecondEnd(work, index);
    }
}

void PlasmaFrigateLights_PlaySwitchAnim(GameSystem *gsys, int index) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    PlasmaFrigateLights_PlaySwitchSecond(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_04);
}

void PlasmaFrigateLights_PlayLightAnim(GameSystem *gsys, int index) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    PlasmaFrigateLights_PlayLightSecond(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_05);
}

static void PlasmaFrigateLights_LoadScene(PlasmaFrigateLightsWork *work) {
    int i;
    int model;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    VecFx32 switchPositions[4] = {
        { FX32_CONST(104), FX32_CONST(0), FX32_CONST(120) },
        { FX32_CONST(104), FX32_CONST(0), FX32_CONST(200) },
        { FX32_CONST(264), FX32_CONST(0), FX32_CONST(120) },
        { FX32_CONST(264), FX32_CONST(0), FX32_CONST(200) },
    };
    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_SWITCH + i;
        PlasmaFrigateLights_InitModel(work->expObj, model, NELEMS(sSwitchAnimations), switchPositions[i]);
#ifdef WHITE2
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
#endif
    }
    VecFx32 lightPositions[4] = {
        { FX32_CONST(184), FX32_CONST(0), FX32_CONST(112) },
        { FX32_CONST(184), FX32_CONST(0), FX32_CONST(96) },
        { FX32_CONST(184), FX32_CONST(0), FX32_CONST(80) },
        { FX32_CONST(184), FX32_CONST(0), FX32_CONST(64) },
    };
    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_LIGHT + i;
        PlasmaFrigateLights_InitModel(work->expObj, model, NELEMS(sLightAnimations), lightPositions[i]);
        PlasmaFrigateLights_PlayAnimation(work->expObj, model, 0, TRUE);
    }
}

static void PlasmaFrigateLights_FreeScene(PlasmaFrigateLightsWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void PlasmaFrigateLights_StepAnimations(PlasmaFrigateLightsWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void PlasmaFrigateLights_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;
    fx32 z = pos.z;
    fx32 y = pos.y;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = pos.x;
    matrix->translation.y = y;
    matrix->translation.z = z;
    FieldExpObj_SetActorCulling(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Plays the animation from its first frame
static void PlasmaFrigateLights_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void PlasmaFrigateLights_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Plays the switch's second animation once and stops its first
static void PlasmaFrigateLights_PlaySwitchSecond(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;

    PlasmaFrigateLights_PlayAnimation(work->expObj, model, 1, FALSE);
    PlasmaFrigateLights_StopAnimation(work->expObj, model, 0);
}

// Puts the switch at the end of its first animation, with the second stopped
static void PlasmaFrigateLights_ShowSwitchFirstEnd(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0);

    FieldExpObj_SetAnm(work->expObj, 0, model, 0, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 0, FieldExpObjAnm_GetLastFrame(anm));
    PlasmaFrigateLights_StopAnimation(work->expObj, model, 1);
}

// Puts the switch at the end of its second animation, with the first stopped
static void PlasmaFrigateLights_ShowSwitchSecondEnd(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, FieldExpObjAnm_GetLastFrame(anm));
    PlasmaFrigateLights_StopAnimation(work->expObj, model, 0);
}

// Plays the second animation of the light once
static void PlasmaFrigateLights_PlayLightSecond(PlasmaFrigateLightsWork *work, u16 index) {
    PlasmaFrigateLights_PlayAnimation(work->expObj, index + MODEL_LIGHT, 1, FALSE);
}

// Puts the light at the end of its second animation, with the first stopped
static void PlasmaFrigateLights_ShowLightSecondEnd(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_LIGHT;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, FieldExpObjAnm_GetLastFrame(anm));
    PlasmaFrigateLights_StopAnimation(work->expObj, model, 0);
}

// Loops the light's first animation and stops its second
static void PlasmaFrigateLights_LoopLightFirst(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_LIGHT;

    PlasmaFrigateLights_PlayAnimation(work->expObj, model, 0, TRUE);
    PlasmaFrigateLights_StopAnimation(work->expObj, model, 1);
}
