#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_funfest_levels.h"
#include "field/player_state.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "save/high_link.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 0

// The archives of the scene's models
#define GIMMICK_ARCID_BODY 0xcc
#define GIMMICK_ARCID_PARTS 0xcf

// Models 0 to 4 show a player's Black level, 5 to 9 their White level, one lit each
#define MODEL_BLACK_LEVEL 0
#define MODEL_WHITE_LEVEL 5
#define MODEL_LEVEL_COUNT 10
// The models of the effects
#define MODEL_EFFECT_0 10
#define MODEL_EFFECT_1 11
#define MODEL_START_EFFECT 12
#define MODEL_LEAVE_EFFECT 13
#define MODEL_COUNT 14

// A level counts for one lit model every 6 levels, up to 4
#define LEVELS_PER_STEP 6
#define MAX_STEPS 4

// The x of the tile in front of the effect, and the z of the tiles that start and end it
#define TRIGGER_X FX32_CONST(504)
#define START_Z_MIN FX32_CONST(552)
#define START_Z_MAX FX32_CONST(568)
#define END_Z FX32_CONST(584)

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    u8 netId;
    BOOL effectOn;
} FunfestLevelsWork;

// What GameCommSys_GetWork returns while the Entralink is running
typedef struct {
    u8 unk00[0x8a];
    u16 netId : 3;
    u16 unk8A_3 : 13;
} EntralinkCommWork;

static void GimmickFunfestLevels_InitModels(FunfestLevelsWork *work);
static void GimmickFunfestLevels_Free(FunfestLevelsWork *work);
static void GimmickFunfestLevels_StepAnimations(FunfestLevelsWork *work);
static void GimmickFunfestLevels_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void GimmickFunfestLevels_InitState(FunfestLevelsWork *work, Field *field, GameData *gameData,
                                           EntralinkCommWork *commWork);
static void GimmickFunfestLevels_HideLevels(FieldExpObjSystem *expObj);
static int GetLevelSteps(u16 level);
static u8 GimmickFunfestLevels_ShowLevels(GameData *gameData, FieldExpObjSystem *expObj, EntralinkCommWork *commWork);
static EntralinkCommWork *GetCommWork(GameCommSys *comm);
static void GimmickFunfestLevels_ShowEffect0(FunfestLevelsWork *work);
static void GimmickFunfestLevels_PlayEffect1(FunfestLevelsWork *work);
static void GimmickFunfestLevels_ShowStartEffect(FunfestLevelsWork *work);
static void GimmickFunfestLevels_ShowLeaveEffect(FunfestLevelsWork *work);
static BOOL IsAtEffectStart(FunfestLevelsWork *work);
static BOOL IsAtEffectEnd(FunfestLevelsWork *work);

static const G3DSceneAnimationSetup sLeaveEffectAnimations[] = { { 11, 0 } };
static const G3DSceneAnimationSetup sEffect1Animations[] = { { 5, 0 } };
static const G3DSceneAnimationSetup sEffect0Animations[] = { { 2, 0 }, { 3, 0 } };
static const G3DSceneAnimationSetup sStartEffectAnimations[] = { { 7, 0 }, { 8, 0 }, { 9, 0 } };

static const int sAnimationCounts[MODEL_COUNT - MODEL_EFFECT_0] = { 2, 1, 3, 1 };

static const G3DSceneResourceSetup sFunfestLevelsResources[12] = {
    { GIMMICK_ARCID_BODY, 0, 0 },  { GIMMICK_ARCID_PARTS, 1, 0 },  { GIMMICK_ARCID_PARTS, 0, 0 },
    { GIMMICK_ARCID_PARTS, 2, 0 }, { GIMMICK_ARCID_PARTS, 3, 0 },  { GIMMICK_ARCID_PARTS, 4, 0 },
    { GIMMICK_ARCID_PARTS, 5, 0 }, { GIMMICK_ARCID_PARTS, 7, 0 },  { GIMMICK_ARCID_PARTS, 8, 0 },
    { GIMMICK_ARCID_PARTS, 6, 0 }, { GIMMICK_ARCID_PARTS, 10, 0 }, { GIMMICK_ARCID_PARTS, 9, 0 },
};

static const G3DSceneActorSetup sFunfestLevelsActors[MODEL_COUNT] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 0, 1, 0, 0, NULL, 0 },
    { 0, 2, 0, 0, NULL, 0 },
    { 0, 3, 0, 0, NULL, 0 },
    { 0, 4, 0, 0, NULL, 0 },
    { 0, 5, 0, 0, NULL, 0 },
    { 0, 6, 0, 0, NULL, 0 },
    { 0, 7, 0, 0, NULL, 0 },
    { 0, 8, 0, 0, NULL, 0 },
    { 0, 9, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, sEffect0Animations, NELEMS(sEffect0Animations) },
    { 4, 0, 4, 0, sEffect1Animations, NELEMS(sEffect1Animations) },
    { 6, 0, 6, 0, sStartEffectAnimations, NELEMS(sStartEffectAnimations) },
    { 10, 0, 10, 0, sLeaveEffectAnimations, NELEMS(sLeaveEffectAnimations) },
};

static const G3DSceneSetup sFunfestLevelsSceneSetup = { sFunfestLevelsResources, NELEMS(sFunfestLevelsResources), sFunfestLevelsActors, NELEMS(sFunfestLevelsActors) };

void GimmickFunfestLevels_Setup(Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    GameSystem *gsys;
    GameData *gameData;
    EntralinkCommWork *commWork;
    FunfestLevelsWork *work;

    Field_GetExpObjSystem(field);
    gsys = Field_GetGameSystem(field);
    gameData = GSYS_GetGameData(gsys);
    GameData_GetGimmickState(gameData);
    commWork = GetCommWork(GSYS_GetGameCommSystem(gsys));
    work = Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, heapId, sizeof(FunfestLevelsWork));
    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    GimmickFunfestLevels_InitModels(work);
    GimmickFunfestLevels_InitState(work, field, gameData, commWork);
}

void GimmickFunfestLevels_End(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));

    GameData_GetGimmickState(gameData);
    Field_GetExpObjSystem(field);
    GimmickFunfestLevels_Free(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

// Shows the levels of the player whose beacon changed, and starts and ends the effect as the player walks by
void GimmickFunfestLevels_Update(Field *field) {
    GameSystem *gsys = Field_GetGameSystem(field);
    GameData *gameData = GSYS_GetGameData(gsys);
    FieldExpObjSystem *expObj;
    FunfestLevelsWork *work;
    EntralinkCommWork *commWork;

    GameData_GetGimmickState(gameData);
    expObj = Field_GetExpObjSystem(field);
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    commWork = GetCommWork(GSYS_GetGameCommSystem(gsys));
    if (commWork != NULL && commWork->netId != work->netId) {
        GimmickFunfestLevels_HideLevels(expObj);
        work->netId = GimmickFunfestLevels_ShowLevels(gameData, expObj, commWork);
    }
    if (work->effectOn == FALSE) {
        if (IsAtEffectStart(work) == TRUE) {
            work->effectOn = TRUE;
            GimmickFunfestLevels_PlayEffect1(work);
            GFL_SndSEPlay(SEQ_SE_SW_FES_14);
        }
    } else if (IsAtEffectEnd(work) == TRUE) {
        work->effectOn = FALSE;
    }
    GimmickFunfestLevels_StepAnimations(work);
}

void GimmickFunfestLevels_PlayStartEffect(GameSystem *gsys) {
    FunfestLevelsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GimmickFunfestLevels_ShowStartEffect(work);
    GFL_SndSEPlay(SEQ_SE_SW_FES_15);
}

void GimmickFunfestLevels_PlayLeaveEffect(GameSystem *gsys) {
    FunfestLevelsWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GimmickFunfestLevels_ShowLeaveEffect(work);
    GFL_SndSEPlay(SEQ_SE_SW_FES_16);
}

// Places the models, hidden, with the effects' animations set and paused
static void GimmickFunfestLevels_InitModels(FunfestLevelsWork *work) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int model;
    int i;
    int count;

    FieldExpObj_AddScene(work->expObj, &sFunfestLevelsSceneSetup, 0);
    for (model = 0; model < MODEL_LEVEL_COUNT; model++) {
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model);
        matrix->translation.x = TRIGGER_X;
        matrix->translation.y = FX32_CONST(32);
        matrix->translation.z = TRIGGER_X;
        func_ov036_021b8248(work->expObj, 0, model, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
    }
    for (model = MODEL_EFFECT_0; model <= MODEL_LEAVE_EFFECT; model++) {
        count = sAnimationCounts[model - MODEL_EFFECT_0];
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model);
        matrix->translation.x = FX32_CONST(512);
        matrix->translation.y = 0;
        matrix->translation.z = FX32_CONST(512);
        func_ov036_021b8248(work->expObj, 0, model, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
        for (i = 0; i < count; i++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, i);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnm(work->expObj, 0, model, i, FALSE);
        }
    }
}

static void GimmickFunfestLevels_Free(FunfestLevelsWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickFunfestLevels_StepAnimations(FunfestLevelsWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Plays the animation of the model from its start
static void GimmickFunfestLevels_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(info, FALSE);
    FieldExpObjAnm_SetLooped(info, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

static void GimmickFunfestLevels_InitState(FunfestLevelsWork *work, Field *field, GameData *gameData,
                                           EntralinkCommWork *commWork) {
    work->netId = GimmickFunfestLevels_ShowLevels(gameData, Field_GetExpObjSystem(field), commWork);
    if (IsAtEffectStart(work) == TRUE) {
        work->effectOn = TRUE;
    } else {
        work->effectOn = FALSE;
    }
    GimmickFunfestLevels_ShowEffect0(work);
}

static void GimmickFunfestLevels_HideLevels(FieldExpObjSystem *expObj) {
    int i;

    for (i = 0; i < MODEL_LEVEL_COUNT; i++) {
        FieldExpObj_SetActorHidden(expObj, 0, i, TRUE);
    }
}

static int GetLevelSteps(u16 level) {
    int steps = level / LEVELS_PER_STEP;

    if (steps > MAX_STEPS) {
        steps = MAX_STEPS;
    }
    return steps;
}

// Lights the models of the Entralink levels of the player whose beacon is shown, in the tile of the player's
// position relative to the player's own. Returns the player's net ID
static u8 GimmickFunfestLevels_ShowLevels(GameData *gameData, FieldExpObjSystem *expObj, EntralinkCommWork *commWork) {
    u32 netId = func_020175fc(gameData);
    int offset;
    void *levels;
    int whiteSteps;
    int blackSteps;
    SRTMatrix *matrix;
    fx32 x;

    offset = netId - func_020175cc(gameData);
    if (offset < 0) {
        offset += func_020175bc(gameData);
    }
    if (netId == func_020175cc(gameData)) {
        levels = func_02017208(gameData);
    } else {
        levels = func_020171fc(gameData, netId);
    }
    whiteSteps = GetLevelSteps(func_0200c5dc(levels));
    blackSteps = GetLevelSteps(func_0200c5e0(levels));
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_WHITE_LEVEL + whiteSteps);
    x = FX32_CONST(1024) * offset + TRIGGER_X;
    matrix->translation.x = x;
    matrix->translation.y = FX32_CONST(32);
    matrix->translation.z = TRIGGER_X;
    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, MODEL_BLACK_LEVEL + blackSteps);
    matrix->translation.x = x;
    matrix->translation.y = FX32_CONST(32);
    matrix->translation.z = TRIGGER_X;
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_WHITE_LEVEL + whiteSteps, FALSE);
    FieldExpObj_SetActorHidden(expObj, 0, MODEL_BLACK_LEVEL + blackSteps, FALSE);
    return netId;
}

static EntralinkCommWork *GetCommWork(GameCommSys *comm) {
    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_UNK2) {
        return GameCommSys_GetWork(comm);
    }
    return NULL;
}

static void GimmickFunfestLevels_ShowEffect0(FunfestLevelsWork *work) {
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_EFFECT_0, FALSE);
    for (i = 0; i < 2; i++) {
        GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_EFFECT_0, i, TRUE);
    }
}

static void GimmickFunfestLevels_PlayEffect1(FunfestLevelsWork *work) {
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_EFFECT_1, FALSE);
    for (i = 0; i < NELEMS(sEffect1Animations); i++) {
        GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_EFFECT_1, i, FALSE);
    }
}

static void GimmickFunfestLevels_ShowStartEffect(FunfestLevelsWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_START_EFFECT, FALSE);
    GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_START_EFFECT, 0, FALSE);
    GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_START_EFFECT, 1, FALSE);
    GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_START_EFFECT, 2, TRUE);
}

static void GimmickFunfestLevels_ShowLeaveEffect(FunfestLevelsWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_LEAVE_EFFECT, FALSE);
    GimmickFunfestLevels_PlayAnimation(work->expObj, MODEL_LEAVE_EFFECT, 0, FALSE);
}

static BOOL IsAtEffectStart(FunfestLevelsWork *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(Field_GetPlayer(work->field), &pos);
    if (pos.x == TRIGGER_X && pos.z >= START_Z_MIN && pos.z <= START_Z_MAX) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsAtEffectEnd(FunfestLevelsWork *work) {
    VecFx32 pos;

    FieldPlayer_GetWPos(Field_GetPlayer(work->field), &pos);
    if (pos.x == TRIGGER_X && pos.z == END_Z) {
        return TRUE;
    }
    return FALSE;
}
