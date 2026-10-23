#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_aspertia_city.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/season.h"

// The gimmick of zone 427 (the constant says Aspertia City), which overlay 36's gimmick table loads: four models, one
// for each season, each with two animations, and a picture on BG 3 that script plugin 14 asks for. The file name is a
// guess, since the ROM has no string for it

#define GIMMICK_WORK_ID 1
#define GIMMICK_BG 3
// The archive of the models
#define GIMMICK_MODEL_ARCID 300
// The BG's palette in that archive, one for each version
#ifdef BLACK2
#define GIMMICK_BG_NCLR 4
#else
#define GIMMICK_BG_NCLR 5
#endif

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    // The picture is wanted on the next update
    BOOL bgWanted;
    BOOL bgLoaded;
} GimmickWork;

static void AspertiaCity_LoadBG(GimmickWork *work);
static void AspertiaCity_FreeBG(GimmickWork *work);
static void AspertiaCity_LoadModels(GimmickWork *work);
static void AspertiaCity_FreeScene(GimmickWork *work);
static void AspertiaCity_StepAnimations(GimmickWork *work);
static void AspertiaCity_ShowSeasonModel(FieldExpObjSystem *system, u8 season);

static const G3DSceneAnimationSetup sSpringAnimations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sWinterAnimations[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sSummerAnimations[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sAutumnAnimations[] = { { 7, 0 }, { 8, 0 } };

static const G3DSceneResourceSetup sResources[] = {
    { GIMMICK_MODEL_ARCID, 0, 0 }, { GIMMICK_MODEL_ARCID, 1, 0 },  { GIMMICK_MODEL_ARCID, 2, 0 },
    { GIMMICK_MODEL_ARCID, 3, 0 }, { GIMMICK_MODEL_ARCID, 4, 0 },  { GIMMICK_MODEL_ARCID, 5, 0 },
    { GIMMICK_MODEL_ARCID, 6, 0 }, { GIMMICK_MODEL_ARCID, 7, 0 },  { GIMMICK_MODEL_ARCID, 8, 0 },
    { GIMMICK_MODEL_ARCID, 9, 0 }, { GIMMICK_MODEL_ARCID, 10, 0 }, { GIMMICK_MODEL_ARCID, 11, 0 },
};

static const G3DSceneActorSetup sActors[SEASON_COUNT] = {
    { 0, 0, 0, 0, sSpringAnimations, NELEMS(sSpringAnimations) },
    { 3, 0, 3, 0, sSummerAnimations, NELEMS(sSummerAnimations) },
    { 6, 0, 6, 0, sAutumnAnimations, NELEMS(sAutumnAnimations) },
    { 9, 0, 9, 0, sWinterAnimations, NELEMS(sWinterAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

// How many animations each season's model has
static const u32 sAnimationCounts[SEASON_COUNT] = { 2, 2, 2, 2 };

void AspertiaCity_Init(Field *field) {
    GimmickWork *work;

    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    work->bgWanted = FALSE;
    work->bgLoaded = FALSE;
    AspertiaCity_LoadModels(work);
}

void AspertiaCity_End(Field *field) {
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    if (work->bgLoaded == TRUE) {
        AspertiaCity_FreeBG(work);
    }
    AspertiaCity_FreeScene(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void AspertiaCity_Update(Field *field) {
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    if (work->bgWanted == TRUE) {
        AspertiaCity_LoadBG(work);
        work->bgWanted = FALSE;
        work->bgLoaded = TRUE;
    }
    AspertiaCity_StepAnimations(work);
}

void AspertiaCity_PlaySeasonModel(GameSystem *gsys) {
    GameData *gameData;
    GimmickWork *work;
    u8 season;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    season = GameData_GetSeason(gameData);
    switch (season) {
    case 0:
    case 1:
    case 2:
    case 3:
        AspertiaCity_ShowSeasonModel(work->expObj, season);
        break;
    }
}

void AspertiaCity_RequestBG(GameSystem *gsys) {
    GameData *gameData;
    GimmickWork *work;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    work->bgWanted = TRUE;
}

static void AspertiaCity_LoadBG(GimmickWork *work) {
    HeapID heapId;
    ArcTool *arc;

    heapId = Field_GetHeapID(work->field);
    GFL_BGSysReleaseBG(GIMMICK_BG);
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0x2000),
                          GX_BG_CHARBASE(0x04000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(GIMMICK_BG, &setup, BGMODE_TEXT);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_GIMMICK, HEAPID_TAIL(heapId));
    GFL_G2DIOLoadArcNCLRDefault(arc, GIMMICK_BG_NCLR, 0, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 6, GIMMICK_BG, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, GIMMICK_BG, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(GIMMICK_BG);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
}

static void AspertiaCity_FreeBG(GimmickWork *work) {
    GFL_BGSysReleaseBG(GIMMICK_BG);
}

// Loads the four models, hidden and with their animations stopped at their first frame
static void AspertiaCity_LoadModels(GimmickWork *work) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;
    int j;
    int count;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    for (i = 0; i < SEASON_COUNT; i++) {
        count = sAnimationCounts[i];
        matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, i);
        matrix->translation.x = FX32_CONST(744);
        matrix->translation.y = FX32_CONST(16);
        matrix->translation.z = FX32_CONST(12216);
        FieldExpObj_SetActorCulling(work->expObj, 0, i, 0);
        FieldExpObj_SetActorHidden(work->expObj, 0, i, TRUE);
        for (j = 0; j < count; j++) {
            anm = FieldExpObj_GetAnmInfo(work->expObj, 0, i, j);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnm(work->expObj, 0, i, j, FALSE);
        }
    }
}

static void AspertiaCity_FreeScene(GimmickWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void AspertiaCity_StepAnimations(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Shows the season's model and plays its animations from their first frame
static void AspertiaCity_ShowSeasonModel(FieldExpObjSystem *system, u8 season) {
    FieldExpObjAnm *anm;
    int count;
    int i;

    count = sAnimationCounts[season];
    FieldExpObj_SetActorHidden(system, 0, season, FALSE);
    for (i = 0; i < count; i++) {
        anm = FieldExpObj_GetAnmInfo(system, 0, season, i);
        FieldExpObj_SetAnm(system, 0, season, i, TRUE);
        FieldExpObj_SetAnmFrame(system, 0, season, i, 0);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}
