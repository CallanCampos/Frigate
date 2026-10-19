#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_environment.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_league_center.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/game_system.h"

// The gimmick of the Pokémon League's central room once the Elite Four are beaten (zone 139, which shares zone 138's
// map): a model with three animations, a flash of light and a picture on BG 3. The file name is a guess

#define GIMMICK_WORK_ID 1
#define GIMMICK_BG 3
// The BG's palette in that archive, one for each version
#ifdef BLACK2
#define GIMMICK_BG_NCLR 4
#else
#define GIMMICK_BG_NCLR 5
#endif

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    BOOL bgLoaded;
} GimmickWork;

static void func_ov115_021eed28(GimmickWork *work);
static void func_ov115_021eedc8(GimmickWork *work);
static void func_ov115_021eedd4(GimmickWork *work);
static void func_ov115_021eee58(GimmickWork *work);
static void func_ov115_021eee64(GimmickWork *work);
static void func_ov115_021eee70(FieldExpObjSystem *system, u16 actor);

static const G3DSceneResourceSetup sResources[] = {
    { ARCID_GIMMICK, 0, 0 },
    { ARCID_GIMMICK, 1, 0 },
    { ARCID_GIMMICK, 2, 0 },
    { ARCID_GIMMICK, 3, 0 },
};

static const G3DSceneAnimationSetup sAnimations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };

static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void func_ov115_021eec80(Field *field) {
    GimmickWork *work;

    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GimmickWork));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    work->bgLoaded = FALSE;
    func_ov115_021eedd4(work);
    FieldLight_StartFlashOneWay(Field_GetLightSystem(field), GX_RGB(11, 11, 16), 1);
}

void func_ov115_021eecc8(Field *field) {
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    func_ov115_021eedc8(work);
    func_ov115_021eee58(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void func_ov115_021eece8(Field *field) {
    GimmickWork *work;

    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    if (!work->bgLoaded) {
        func_ov115_021eed28(work);
        work->bgLoaded = TRUE;
    }
    func_ov115_021eee64(work);
}

void func_ov115_021eed08(GameSystem *gsys) {
    GameData *gameData;
    GimmickWork *work;

    gameData = GSYS_GetGameData(gsys);
    work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    func_ov115_021eee70(work->expObj, 0);
}

static void func_ov115_021eed28(GimmickWork *work) {
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

        GFL_BGSysCreateBG(GIMMICK_BG, &setup, 0);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_GIMMICK, HEAPID_TAIL(heapId));
    GFL_G2DIOLoadArcNCLRDefault(arc, GIMMICK_BG_NCLR, 0, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 6, GIMMICK_BG, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, GIMMICK_BG, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(GIMMICK_BG);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
}

static void func_ov115_021eedc8(GimmickWork *work) {
    GFL_BGSysReleaseBG(GIMMICK_BG);
}

// Loads the model, at its place, with its animations stopped at their first frame
static void func_ov115_021eedd4(GimmickWork *work) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    u32 i;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, 0);
    matrix->translation.x = FX32_CONST(512);
    matrix->translation.y = FX32_CONST(768);
    matrix->translation.z = FX32_CONST(-256);
    FieldExpObj_SetActorCulling(work->expObj, 0, 0, 1);
    FieldExpObj_SetActorHidden(work->expObj, 0, 0, FALSE);
    for (i = 0; i < NELEMS(sAnimations); i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 0, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, 0, i, FALSE);
    }
}

static void func_ov115_021eee58(GimmickWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void func_ov115_021eee64(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Plays the actor's animations from their first frame
static void func_ov115_021eee70(FieldExpObjSystem *system, u16 actor) {
    FieldExpObjAnm *anm;
    s32 i;

    FieldExpObj_SetActorHidden(system, 0, actor, FALSE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(system, 0, actor, i);
        FieldExpObj_SetAnm(system, 0, actor, i, TRUE);
        FieldExpObj_SetAnmFrame(system, 0, actor, i, 0);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}
