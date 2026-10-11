#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/gimmick_marine_tube.h"
#include "gfl/g3d.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 50
// The archive of the scene's models
#define GIMMICK_ARCID 0x127

#define MODEL_COUNT 13
#define FRAME_COUNT 20

// What the save keeps of the tube: the frame of each animation, and which of the tube's stages is shown
typedef struct {
    // A model's frames start at its own index, so they overlap those of the next model when it has two animations
    fx32 frames[FRAME_COUNT];
    u32 stage;
} MarineTubeState;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    MarineTubeState *state;
} MarineTubeWork;

static void func_ov111_021eedac(MarineTubeWork *work);
static void func_ov111_021eed08(MarineTubeWork *work);
static void func_ov111_021eef70(MarineTubeWork *work);
static void func_ov111_021eef7c(MarineTubeWork *work);
static void func_ov111_021eef88(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos);
static void func_ov111_021ef010(FieldExpObjSystem *expObj, u16 model);
static void func_ov111_021ef074(MarineTubeWork *work);
static void func_ov111_021ef0c8(MarineTubeWork *work);

static const G3DSceneResourceSetup sResources[29] = {
    { GIMMICK_ARCID, 0, 0 },  { GIMMICK_ARCID, 1, 0 },  { GIMMICK_ARCID, 2, 0 },  { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 },  { GIMMICK_ARCID, 5, 0 },  { GIMMICK_ARCID, 6, 0 },  { GIMMICK_ARCID, 7, 0 },
    { GIMMICK_ARCID, 8, 0 },  { GIMMICK_ARCID, 9, 0 },  { GIMMICK_ARCID, 10, 0 }, { GIMMICK_ARCID, 11, 0 },
    { GIMMICK_ARCID, 12, 0 }, { GIMMICK_ARCID, 13, 0 }, { GIMMICK_ARCID, 14, 0 }, { GIMMICK_ARCID, 15, 0 },
    { GIMMICK_ARCID, 16, 0 }, { GIMMICK_ARCID, 17, 0 }, { GIMMICK_ARCID, 18, 0 }, { GIMMICK_ARCID, 19, 0 },
    { GIMMICK_ARCID, 20, 0 }, { GIMMICK_ARCID, 21, 0 }, { GIMMICK_ARCID, 22, 0 }, { GIMMICK_ARCID, 23, 0 },
    { GIMMICK_ARCID, 24, 0 }, { GIMMICK_ARCID, 25, 0 }, { GIMMICK_ARCID, 26, 0 }, { GIMMICK_ARCID, 27, 0 },
    { GIMMICK_ARCID, 28, 0 },
};

static const G3DSceneAnimationSetup sModel0Animations[] = { { 1, 0 } };
static const G3DSceneAnimationSetup sModel1Animations[] = { { 3, 0 } };
static const G3DSceneAnimationSetup sModel2Animations[] = { { 5, 0 } };
static const G3DSceneAnimationSetup sModel3Animations[] = { { 7, 0 } };
static const G3DSceneAnimationSetup sModel4Animations[] = { { 9, 0 }, { 10, 0 } };
static const G3DSceneAnimationSetup sModel5Animations[] = { { 12, 0 } };
static const G3DSceneAnimationSetup sModel6Animations[] = { { 14, 0 } };
static const G3DSceneAnimationSetup sModel7Animations[] = { { 16, 0 }, { 17, 0 } };
static const G3DSceneAnimationSetup sModel8Animations[] = { { 19, 0 }, { 20, 0 } };
static const G3DSceneAnimationSetup sModel9Animations[] = { { 22, 0 } };
static const G3DSceneAnimationSetup sModel10Animations[] = { { 24, 0 } };
static const G3DSceneAnimationSetup sModel11Animations[] = { { 26, 0 } };
static const G3DSceneAnimationSetup sModel12Animations[] = { { 28, 0 } };

static const G3DSceneActorSetup sActors[MODEL_COUNT] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 0, 0, 2, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 4, 0, 4, 0, sModel2Animations, NELEMS(sModel2Animations) },
    { 6, 0, 6, 0, sModel3Animations, NELEMS(sModel3Animations) },
    { 8, 0, 8, 0, sModel4Animations, NELEMS(sModel4Animations) },
    { 11, 0, 11, 0, sModel5Animations, NELEMS(sModel5Animations) },
    { 13, 0, 13, 0, sModel6Animations, NELEMS(sModel6Animations) },
    { 15, 0, 15, 0, sModel7Animations, NELEMS(sModel7Animations) },
    { 18, 0, 18, 0, sModel8Animations, NELEMS(sModel8Animations) },
    { 21, 0, 21, 0, sModel9Animations, NELEMS(sModel9Animations) },
    { 23, 0, 23, 0, sModel10Animations, NELEMS(sModel10Animations) },
    { 25, 0, 25, 0, sModel11Animations, NELEMS(sModel11Animations) },
    { 27, 0, 27, 0, sModel12Animations, NELEMS(sModel12Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// How many animations each model has
static const int sAnimationCounts[MODEL_COUNT] = { 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1, 1 };

void func_ov111_021eec80(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    MarineTubeWork *work;

    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(MarineTubeWork));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    func_ov111_021eedac(work);
    func_ov111_021eed08(work);
    func_ov111_021ef0c8(work);
}

void func_ov111_021eecd8(Field *field) {
    MarineTubeWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    func_ov111_021ef074(work);
    func_ov111_021eef70(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void func_ov111_021eecf8(Field *field) {
    func_ov111_021eef7c(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

// Shows the models of the saved stage, which loop
static void func_ov111_021eed08(MarineTubeWork *work) {
    switch (work->state->stage) {
    case 0:
        func_ov111_021ef010(work->expObj, 0);
        func_ov111_021ef010(work->expObj, 1);
        func_ov111_021ef010(work->expObj, 2);
        func_ov111_021ef010(work->expObj, 3);
        func_ov111_021ef010(work->expObj, 5);
        func_ov111_021ef010(work->expObj, 6);
        func_ov111_021ef010(work->expObj, 7);
        func_ov111_021ef010(work->expObj, 8);
        break;
    case 1:
        func_ov111_021ef010(work->expObj, 0);
        func_ov111_021ef010(work->expObj, 1);
        func_ov111_021ef010(work->expObj, 2);
        func_ov111_021ef010(work->expObj, 3);
        func_ov111_021ef010(work->expObj, 9);
        func_ov111_021ef010(work->expObj, 10);
        func_ov111_021ef010(work->expObj, 11);
        func_ov111_021ef010(work->expObj, 12);
        break;
    case 2:
        func_ov111_021ef010(work->expObj, 4);
        break;
    }
}

// Places the models, hidden
static void func_ov111_021eedac(MarineTubeWork *work) {
    VecFx32 positions[7] = {
        { FX32_CONST(256), FX32_CONST(0), FX32_CONST(256) },  { FX32_CONST(256), FX32_CONST(0), FX32_CONST(768) },
        { FX32_CONST(256), FX32_CONST(0), FX32_CONST(1280) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(1792) },
        { FX32_CONST(256), FX32_CONST(0), FX32_CONST(2304) }, { FX32_CONST(256), FX32_CONST(0), FX32_CONST(2816) },
        { FX32_CONST(256), FX32_CONST(0), FX32_CONST(3328) },
    };
    VecFx32 posA;
    VecFx32 posB;
    VecFx32 posC;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    posA = positions[3];
    func_ov111_021eef88(work->expObj, 0, 1, posA);
    posB = positions[1];
    func_ov111_021eef88(work->expObj, 1, 1, posB);
    func_ov111_021eef88(work->expObj, 2, 1, posA);
    func_ov111_021eef88(work->expObj, 3, 1, posB);
    func_ov111_021eef88(work->expObj, 4, 2, posA);
    posC = positions[4];
    func_ov111_021eef88(work->expObj, 5, 1, posC);
    func_ov111_021eef88(work->expObj, 6, 1, posB);
    func_ov111_021eef88(work->expObj, 7, 2, posC);
    func_ov111_021eef88(work->expObj, 8, 2, posB);
    func_ov111_021eef88(work->expObj, 9, 1, posA);
    func_ov111_021eef88(work->expObj, 10, 1, posB);
    func_ov111_021eef88(work->expObj, 11, 1, posA);
    func_ov111_021eef88(work->expObj, 12, 1, posB);
}

static void func_ov111_021eef70(MarineTubeWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov111_021eef7c(MarineTubeWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set looping and paused
static void func_ov111_021eef88(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;
    fx32 z = pos.z;
    fx32 y = pos.y;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = pos.x;
    matrix->translation.y = y;
    matrix->translation.z = z;
    func_ov036_021b8248(expObj, 0, model, 0);
    FieldExpObj_SetActorHidden(expObj, 0, model, TRUE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Shows the model with its animations playing in a loop
static void func_ov111_021ef010(FieldExpObjSystem *expObj, u16 model) {
    int animCount = sAnimationCounts[model];
    FieldExpObjAnm *anm;
    int i;

    FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObj_SetAnm(expObj, 0, model, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

// Keeps the frame of each animation in the save
static void func_ov111_021ef074(MarineTubeWork *work) {
    FieldExpObjSystem *expObj = work->expObj;
    MarineTubeState *state = work->state;
    int i;
    int j;
    int animCount;

    for (i = 0; i < MODEL_COUNT; i++) {
        animCount = sAnimationCounts[i];
        for (j = 0; j < animCount; j++) {
            state->frames[i + j] = func_ov036_021b8520(expObj, 0, i, (u16)j);
        }
    }
}

// Sets each animation to the frame that the save kept
static void func_ov111_021ef0c8(MarineTubeWork *work) {
    FieldExpObjSystem *expObj = work->expObj;
    MarineTubeState *state = work->state;
    int i;
    int j;
    int animCount;

    for (i = 0; i < MODEL_COUNT; i++) {
        animCount = sAnimationCounts[i];
        for (j = 0; j < animCount; j++) {
            FieldExpObj_SetAnmFrame(expObj, 0, i, j, state->frames[i + j]);
        }
    }
}
