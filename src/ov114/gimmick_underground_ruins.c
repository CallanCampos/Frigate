#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_underground_ruins.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "system/game_system.h"

// The gimmick of the Underground Ruins (zone 565): one animated model and four others, of which the scripts show one.
// The file's name is a guess, since the overlay holds no string

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
} GimmickWork;

static void UndergroundRuins_LoadScene(GimmickWork *work);
static void UndergroundRuins_FreeScene(GimmickWork *work);
static void UndergroundRuins_StepAnimations(GimmickWork *work);

static const G3DSceneAnimationSetup sAnimations[] = { { 1, 0 } };

static const G3DSceneResourceSetup sResources[] = {
    { 0x12d, 0, 0 }, { 0x12d, 1, 0 }, { 0x12d, 2, 0 }, { 0x12d, 3, 0 }, { 0x12d, 4, 0 }, { 0x12d, 5, 0 },
};

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
    { 2, 0, 2, 0, NULL, 0 },
    { 3, 0, 3, 0, NULL, 0 },
    { 4, 0, 4, 0, NULL, 0 },
    { 5, 0, 5, 0, NULL, 0 },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void UndergroundRuins_Init(Field *field) {
    GimmickWork *work;

    Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, 1);
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    UndergroundRuins_LoadScene(work);
}

void UndergroundRuins_End(Field *field) {
    UndergroundRuins_FreeScene(Field_GetGimmickWorkBlock(field, 1));
    Field_DeleteGimmickWorkBlock(field, 1);
}

void UndergroundRuins_Update(Field *field) {
    UndergroundRuins_StepAnimations(Field_GetGimmickWorkBlock(field, 1));
}

void UndergroundRuins_ShowModel(GameSystem *gsys, u8 a1) {
    GameData *gameData;
    GimmickWork *work;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 2, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 3, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 4, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, a1 + 1, FALSE);
}

void UndergroundRuins_SetAnmEnd(GameSystem *gsys, u16 a1) {
    GameData *gameData;
    GimmickWork *work;
    FieldExpObjAnm *anm;
    fx32 lastFrame;
    int i;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), 1);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 0, i);
        FieldExpObj_SetAnm(work->expObj, 0, 0, i, TRUE);
        lastFrame = FieldExpObjAnm_GetLastFrame(anm);
        if (a1 == 1) {
            FieldExpObj_SetAnmFrame(work->expObj, 0, 0, i, lastFrame);
        } else {
            FieldExpObj_SetAnmFrame(work->expObj, 0, 0, i, 0);
        }
    }
}

void UndergroundRuins_PlayAnm(GameSystem *gsys, u16 a1) {
    GameData *gameData;
    GimmickWork *work;
    int i;
    FieldExpObjAnm *anm;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), 1);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 0, i);
        FieldExpObj_SetAnm(work->expObj, 0, 0, i, TRUE);
        if (a1 == 1) {
            FieldExpObjAnm_SetFrameStep(anm, FX32_ONE);
        } else {
            FieldExpObjAnm_SetFrameStep(anm, -FX32_ONE);
        }
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void UndergroundRuins_LoadScene(GimmickWork *work) {
    SRTMatrix *matrix;
    int i;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, 0);
    matrix->translation.x = FX32_CONST(208);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(32);
    FieldExpObj_SetActorCulling(work->expObj, 0, 0, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, 0, FALSE);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 0, i);

        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, 0, i, FALSE);
    }
    for (i = 1; i <= 4; i++) {
        u16 actor = i;

        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, actor);
        matrix->translation.x = FX32_CONST(224);
        matrix->translation.y = 0;
        matrix->translation.z = FX32_CONST(112);
        FieldExpObj_SetActorCulling(work->expObj, 0, actor, 1);
        if (actor == 4) {
            FieldExpObj_SetActorHidden(work->expObj, 0, actor, FALSE);
        } else {
            FieldExpObj_SetActorHidden(work->expObj, 0, actor, TRUE);
        }
    }
}

static void UndergroundRuins_FreeScene(GimmickWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void UndergroundRuins_StepAnimations(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}
