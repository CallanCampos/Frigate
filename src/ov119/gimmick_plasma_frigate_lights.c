#include "types.h"
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
// The archive of the scene's models, a/2/8/6: the same as overlay 117's, of which the scene is the first six files
#define GIMMICK_ARCID 0x11e

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

static void func_ov119_021eed58(PlasmaFrigateLightsWork *work);
static void func_ov119_021eee00(PlasmaFrigateLightsWork *work);
static void func_ov119_021eee0c(PlasmaFrigateLightsWork *work);
static void func_ov119_021eee18(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos);
static void func_ov119_021eeea0(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void func_ov119_021eeeec(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void func_ov119_021eef24(PlasmaFrigateLightsWork *work, u16 index);
static void func_ov119_021eef44(PlasmaFrigateLightsWork *work, u16 index);
static void func_ov119_021eef90(PlasmaFrigateLightsWork *work, u16 index);
static void func_ov119_021eefd8(PlasmaFrigateLightsWork *work, u16 index);
static void func_ov119_021eefec(PlasmaFrigateLightsWork *work, u16 index);
static void func_ov119_021ef034(PlasmaFrigateLightsWork *work, u16 index);

static const G3DSceneResourceSetup sResources[6] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },
    { GIMMICK_ARCID, 3, 0 }, { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 5, 0 },
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

void func_ov119_021eec80(Field *field) {
    PlasmaFrigateLightsWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(PlasmaFrigateLightsWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    func_ov119_021eed58(work);
}

void func_ov119_021eeca8(Field *field) {
    func_ov119_021eee00(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void func_ov119_021eecc0(Field *field) {
    func_ov119_021eee0c(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

void func_ov119_021eecd0(GameSystem *gsys, int index, BOOL second) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (second == FALSE) {
        func_ov119_021ef034(work, index);
        func_ov119_021eef44(work, index);
    } else {
        func_ov119_021eefec(work, index);
        func_ov119_021eef90(work, index);
    }
}

void func_ov119_021eed10(GameSystem *gsys, int index) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    func_ov119_021eef24(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_04);
}

void func_ov119_021eed34(GameSystem *gsys, int index) {
    PlasmaFrigateLightsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    func_ov119_021eefd8(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_05);
}

static void func_ov119_021eed58(PlasmaFrigateLightsWork *work) {
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
        func_ov119_021eee18(work->expObj, model, NELEMS(sSwitchAnimations), switchPositions[i]);
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
        func_ov119_021eee18(work->expObj, model, NELEMS(sLightAnimations), lightPositions[i]);
        func_ov119_021eeea0(work->expObj, model, 0, TRUE);
    }
}

static void func_ov119_021eee00(PlasmaFrigateLightsWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov119_021eee0c(PlasmaFrigateLightsWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void func_ov119_021eee18(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos) {
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
static void func_ov119_021eeea0(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void func_ov119_021eeeec(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Plays the switch's second animation once and stops its first
static void func_ov119_021eef24(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;

    func_ov119_021eeea0(work->expObj, model, 1, FALSE);
    func_ov119_021eeeec(work->expObj, model, 0);
}

// Puts the switch at the end of its first animation, with the second stopped
static void func_ov119_021eef44(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0);

    FieldExpObj_SetAnm(work->expObj, 0, model, 0, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 0, FieldExpObjAnm_GetLastFrame(anm));
    func_ov119_021eeeec(work->expObj, model, 1);
}

// Puts the switch at the end of its second animation, with the first stopped
static void func_ov119_021eef90(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, FieldExpObjAnm_GetLastFrame(anm));
    func_ov119_021eeeec(work->expObj, model, 0);
}

// Plays the second animation of the light once
static void func_ov119_021eefd8(PlasmaFrigateLightsWork *work, u16 index) {
    func_ov119_021eeea0(work->expObj, index + MODEL_LIGHT, 1, FALSE);
}

// Puts the light at the end of its second animation, with the first stopped
static void func_ov119_021eefec(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_LIGHT;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, FieldExpObjAnm_GetLastFrame(anm));
    func_ov119_021eeeec(work->expObj, model, 0);
}

// Loops the light's first animation and stops its second
static void func_ov119_021ef034(PlasmaFrigateLightsWork *work, u16 index) {
    u16 model = index + MODEL_LIGHT;

    func_ov119_021eeea0(work->expObj, model, 0, TRUE);
    func_ov119_021eeeec(work->expObj, model, 1);
}
