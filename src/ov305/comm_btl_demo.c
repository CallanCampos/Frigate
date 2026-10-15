#include "types.h"
#include "app/comm_btl_demo.h"
#include "app/comm_btl_demo_graphic.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/particle.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g3d.h"
#include "save/player_info.h"
#include "save/records.h"
#include "system/gf_font.h"
#include "pml/poke_party.h"
#include "system/printsys.h"

// The screen before and after a link battle. The ROM has no name for this file; "comm_btl_demo.c" is a guess from
// "comm_btl_demo_graphic.c", the name of the file after it, as wifi_bsubway.c is to wifi_bsubway_graphic.c.
//
// Before the battle the trainers' names slide in around a "VS" with their parties as rows of Poké Balls. After it the
// balls show what became of each Pokémon, then each side's number of Pokémon left and "WIN", "LOSE" or "DRAW"

#define TRAINER_MAX 4
#define BALL_MAX 6
// The Poké Balls of a trainer in a battle of four
#define BALL_MAX_MULTI 3
#define SIDE_COUNT 2
#define PARTICLE_WORK_SIZE 0x4800

// The scenes of the screen, steps of sCommBtlDemoSteps
enum {
    SCENE_START,
    SCENE_RESULT,
    SCENE_DRAW,
    SCENE_COUNT,
};

// The 3D scenes, each a model with two animations
enum {
    G3D_SCENE_SINGLE_START,
    G3D_SCENE_SINGLE_END,
    G3D_SCENE_MULTI_START,
    G3D_SCENE_MULTI_END,
    G3D_SCENE_VS,
    G3D_SCENE_DRAW,
    G3D_SCENE_COUNT,
};

// The files of the screen's archive
enum {
    FILE_BG_CHARS,
    FILE_BG_PALETTE,
    FILE_BG_SCREEN,
    FILE_SUB_BG_CHARS,
    FILE_SUB_BG_PALETTE,
    FILE_SUB_BG_SCREEN,
    FILE_OBJ_ANIMS,
    FILE_OBJ_CELLS,
    FILE_OBJ_CHARS,
    FILE_OBJ_PALETTE,
    FILE_PARTICLES_VS,
    FILE_PARTICLES_VS_COMPETITION,
    FILE_PARTICLES_RESULT,
    FILE_PARTICLES_BALL,
    FILE_BG_COMPETITION_CHARS,
    FILE_BG_COMPETITION_PALETTE,
    FILE_BG_COMPETITION_SCREEN,
    FILE_SINGLE_START_ANIM1,
    FILE_SINGLE_START_ANIM2,
    FILE_SINGLE_START_MODEL,
    FILE_SINGLE_END_ANIM1,
    FILE_SINGLE_END_ANIM2,
    FILE_SINGLE_END_MODEL,
    FILE_MULTI_START_ANIM1,
    FILE_MULTI_START_ANIM2,
    FILE_MULTI_START_MODEL,
    FILE_MULTI_END_ANIM1,
    FILE_MULTI_END_ANIM2,
    FILE_MULTI_END_MODEL,
    FILE_DRAW_ANIM1,
    FILE_DRAW_ANIM2,
    FILE_DRAW_MODEL,
    FILE_VS_ANIM2,
    FILE_VS_ANIM1,
    FILE_VS_MODEL,
};

// The sequences of the OBJ: the numbers 0 to 6 of the Pokémon left, the labels of the result and the Poké Balls
enum {
    ANIM_COUNT_0,
    ANIM_WIN = 7,
    ANIM_LOSE,
    ANIM_DRAW,
    ANIM_BALL_HEALTHY,
    ANIM_BALL_SICK,
    ANIM_BALL_FAINTED,
    ANIM_BALL_NONE,
};

// The labels of the result: one on each side, and the one for a draw in the middle
enum {
    LABEL_SIDE0,
    LABEL_SIDE1,
    LABEL_MIDDLE,
    LABEL_COUNT,
};

// The sounds of sCommBtlDemoSE
enum {
    SE_BALL,
    SE_VS,
    SE_RESULT,
    SE_VS_END,
    SE_DRAW,
    SE_COUNT,
};

typedef struct {
    u32 unk0;
} CommBtlDemoBack;

// The screen's OBJ
typedef struct {
    CommBtlDemoGraphic *graphic;
    u16 heapId;
    UIObjRes res;
} CommBtlDemoObj;

// The 3D scene and the particles in front of it
typedef struct {
    CommBtlDemoGraphic *graphic;
    u16 heapId;
    G3DManager *manager;
    u16 scene;
    u8 animEnded;
    u8 sceneLoaded;
    ParticleSystem *particles;
    u8 particleWork[PARTICLE_WORK_SIZE];
    u8 emitterCount;
} CommBtlDemoG3D;

// A trainer's row of Poké Balls, which appear one by one from a frame that depends on the trainer
typedef struct {
    PokeParty *party;
    CommBtlDemoG3D *g3d;
    u8 type;
    u8 index;
    u8 pkmCount;
    u8 ballCount;
    ClActor *actors[BALL_MAX];
    u32 timer;
    u8 started;
    const u8 *states;
} CommBtlDemoBalls;

typedef struct {
    BattlePlayer *player;
    CommBtlDemoObj *obj;
    CommBtlDemoG3D *g3d;
    Font *font;
    u8 type;
    u8 index;
    u32 gender;
    StrBuf *name;
    BmpWin *nameWindow;
    CommBtlDemoBalls balls;
    u32 timer;
} CommBtlDemoTrainer;

// The numbers of Pokémon left on each side and the labels of the result
typedef struct {
    u32 unk0;
    CommBtlDemoObj *obj;
    CommBtlDemoG3D *g3d;
    u16 counts[SIDE_COUNT];
    ClActor *countActors[SIDE_COUNT];
    ClActor *labels[LABEL_COUNT];
    u16 timer;
    u8 type;
} CommBtlDemoResult;

typedef struct {
    u16 heapId;
    CommBtlDemoParam *param;
    CommBtlDemoBack back;
    CommBtlDemoObj obj;
    CommBtlDemoG3D g3d;
    CommBtlDemoTrainer trainers[TRAINER_MAX];
    CommBtlDemoResult result;
    CommBtlDemoGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    UIScene *scene;
    u8 type;
    u32 battleResult;
    int timer;
    int scroll;
} CommBtlDemoWork;

typedef struct {
    u32 palette;
    u32 chars;
    u32 screen;
} CommBtlDemoBackFiles;

static BOOL CommBtlDemoProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommBtlDemoProc_End(GameProc *proc, u32 *state, void *param, void *work);
static BOOL CommBtlDemoProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static void CommBtlDemo_LoadBG(CommBtlDemoBack *back, HeapID heapId, BOOL competition);
static void CommBtlDemo_EndScene(UIScene *scene, CommBtlDemoWork *wk);
static BOOL CommBtlDemoStart_Begin(UIScene *scene, void *param);
static void CommBtlDemo_BindSinglePalettes(CommBtlDemoWork *wk);
static void CommBtlDemo_BindMultiPalettes(CommBtlDemoWork *wk);
static u32 CommBtlDemo_GetSE(u32 se, BOOL competition);
static BOOL CommBtlDemoStart_Main(UIScene *scene, void *param);
static BOOL CommBtlDemoStart_End(UIScene *scene, void *param);
static BOOL CommBtlDemoResult_Begin(UIScene *scene, void *param);
static BOOL CommBtlDemoResult_Main(UIScene *scene, void *param);
static BOOL CommBtlDemoResult_End(UIScene *scene, void *param);
static BOOL CommBtlDemoDraw_Main(UIScene *scene, void *param);
static BOOL CommBtlDemoDraw_End(UIScene *scene, void *param);
static u16 CommBtlDemo_GetFirstScene(CommBtlDemoParam *param);
static BOOL CommBtlDemo_IsSingle(u8 type);
static BOOL CommBtlDemo_IsStart(u8 type);
static u32 CommBtlDemo_GetBallAnim(PartyPkm *pkm, u32 state);
static void CommBtlDemoBalls_Init(CommBtlDemoBalls *balls, PokeParty *party, u8 type, u8 index, CommBtlDemoObj *obj,
                                  CommBtlDemoG3D *g3d, const u8 *states);
static void CommBtlDemoBalls_Free(CommBtlDemoBalls *balls);
static void CommBtlDemoBalls_Step(CommBtlDemoBalls *balls, u32 start);
static void CommBtlDemoBalls_Update(CommBtlDemoBalls *balls);
static void CommBtlDemoBalls_Start(CommBtlDemoBalls *balls);
static void CommBtlDemoBalls_SetState(CommBtlDemoBalls *balls, PokeParty *party, const u8 *states, int index);
static BmpWin *CommBtlDemo_CreateNameWindow(u8 type, u8 index);
static void CommBtlDemoTrainer_Init(CommBtlDemoTrainer *trainer, u8 type, u8 index, BattlePlayer *player,
                                    CommBtlDemoObj *obj, CommBtlDemoG3D *g3d, Font *font);
static void CommBtlDemoTrainer_UpdateName(CommBtlDemoTrainer *trainer);
static void CommBtlDemoTrainer_Free(CommBtlDemoTrainer *trainer);
static void CommBtlDemoTrainer_Update(CommBtlDemoTrainer *trainer);
static void CommBtlDemoTrainer_PrintName(CommBtlDemoTrainer *trainer, Font *font);
static void CommBtlDemo_InitTrainers(CommBtlDemoWork *wk);
static void CommBtlDemo_FreeTrainers(CommBtlDemoWork *wk);
static void CommBtlDemo_UpdateTrainers(CommBtlDemoWork *wk);
static void CommBtlDemo_StartBalls(CommBtlDemoWork *wk);
static void CommBtlDemo_AddRecord(GameRecords *records, u32 id, u32 result);
static void CommBtlDemo_AddRecords(CommBtlDemoWork *wk);
static void CommBtlDemoResult_Init(CommBtlDemoResult *result, CommBtlDemoObj *obj, CommBtlDemoG3D *g3d, u16 count0,
                                   u16 count1, u8 type);
static void CommBtlDemoResult_Free(CommBtlDemoResult *result);
static void CommBtlDemoResult_Update(CommBtlDemoResult *result);
static void CommBtlDemoResult_ShowLabel(CommBtlDemoResult *result, u32 label, u32 kind);
static void CommBtlDemoObj_Init(CommBtlDemoObj *obj, CommBtlDemoGraphic *graphic, HeapID heapId);
static void CommBtlDemoObj_Free(CommBtlDemoObj *obj);
static ClActor *CommBtlDemoObj_CreateActor(CommBtlDemoObj *obj, s16 x, s16 y, u16 anim);
static ClActor *CommBtlDemoObj_CreateCount(CommBtlDemoObj *obj, u32 side, u8 count);
static void CommBtlDemoG3D_Init(CommBtlDemoG3D *g3d, CommBtlDemoGraphic *graphic, HeapID heapId);
static void CommBtlDemoG3D_Free(CommBtlDemoG3D *g3d);
static void CommBtlDemoG3D_Draw(CommBtlDemoG3D *g3d);
static void CommBtlDemoG3D_LoadParticles(CommBtlDemoG3D *g3d, u32 file);
static void CommBtlDemoG3D_ClearParticles(CommBtlDemoG3D *g3d);
static void CommBtlDemoG3D_FreeParticles(CommBtlDemoG3D *g3d);
static void CommBtlDemoG3D_AddEmitter(CommBtlDemoG3D *g3d, u16 emitter, const VecFx32 *pos);
static void CommBtlDemoG3D_AddEmitters(CommBtlDemoG3D *g3d, const VecFx32 *pos);
static BOOL CommBtlDemoG3D_BindPalette(G3DActor *actor, u32 plttToMatListIdx, u32 plttNameIdx);
static void CommBtlDemoG3D_LoadScene(CommBtlDemoG3D *g3d, u32 scene);
static void CommBtlDemoG3D_DeleteScene(CommBtlDemoG3D *g3d);
static void CommBtlDemoG3D_FreeScene(CommBtlDemoG3D *g3d);
static BOOL CommBtlDemoG3D_UpdateAnim(CommBtlDemoG3D *g3d);
static void CommBtlDemo_ResetStates(CommBtlDemoParam *param);
static u32 CommBtlDemo_CountLeft(BattlePlayer *player);

// The 3D scenes: the model and its two animations, and the actor that plays them
static const G3DSceneResourceSetup sCommBtlDemoSingleStartResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_START_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_START_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_START_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoSingleStartAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoSingleStartActor = {
    0, 0, 0, 0, sCommBtlDemoSingleStartAnims, NELEMS(sCommBtlDemoSingleStartAnims)
};

static const G3DSceneResourceSetup sCommBtlDemoSingleEndResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_END_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_END_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_SINGLE_END_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoSingleEndAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoSingleEndActor = {
    0, 0, 0, 0, sCommBtlDemoSingleEndAnims, NELEMS(sCommBtlDemoSingleEndAnims)
};

static const G3DSceneResourceSetup sCommBtlDemoMultiStartResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_START_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_START_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_START_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoMultiStartAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoMultiStartActor = {
    0, 0, 0, 0, sCommBtlDemoMultiStartAnims, NELEMS(sCommBtlDemoMultiStartAnims)
};

static const G3DSceneResourceSetup sCommBtlDemoMultiEndResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_END_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_END_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_MULTI_END_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoMultiEndAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoMultiEndActor = {
    0, 0, 0, 0, sCommBtlDemoMultiEndAnims, NELEMS(sCommBtlDemoMultiEndAnims)
};

static const G3DSceneResourceSetup sCommBtlDemoVsResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_VS_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_VS_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_VS_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoVsAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoVsActor = { 0, 0, 0, 0, sCommBtlDemoVsAnims, NELEMS(sCommBtlDemoVsAnims) };

static const G3DSceneResourceSetup sCommBtlDemoDrawResources[] = {
    { ARCID_COMM_BTL_DEMO, FILE_DRAW_MODEL, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_DRAW_ANIM1, 0, 0 },
    { ARCID_COMM_BTL_DEMO, FILE_DRAW_ANIM2, 0, 0 },
};

static const G3DSceneAnimationSetup sCommBtlDemoDrawAnims[] = { { 1, 0 }, { 2, 0 } };

static const G3DSceneActorSetup sCommBtlDemoDrawActor = {
    0, 0, 0, 0, sCommBtlDemoDrawAnims, NELEMS(sCommBtlDemoDrawAnims)
};

static const G3DSceneSetup sCommBtlDemoScenes[G3D_SCENE_COUNT] = {
    { sCommBtlDemoSingleStartResources, NELEMS(sCommBtlDemoSingleStartResources), &sCommBtlDemoSingleStartActor, 1 },
    { sCommBtlDemoSingleEndResources, NELEMS(sCommBtlDemoSingleEndResources), &sCommBtlDemoSingleEndActor, 1 },
    { sCommBtlDemoMultiStartResources, NELEMS(sCommBtlDemoMultiStartResources), &sCommBtlDemoMultiStartActor, 1 },
    { sCommBtlDemoMultiEndResources, NELEMS(sCommBtlDemoMultiEndResources), &sCommBtlDemoMultiEndActor, 1 },
    { sCommBtlDemoVsResources, NELEMS(sCommBtlDemoVsResources), &sCommBtlDemoVsActor, 1 },
    { sCommBtlDemoDrawResources, NELEMS(sCommBtlDemoDrawResources), &sCommBtlDemoDrawActor, 1 },
};

// The sounds, and those of the Wi-Fi competition
static const u32 sCommBtlDemoSE[SE_COUNT][2] = {
    { SEQ_SE_SYS_65, SEQ_SE_SYS_65 },   { SEQ_SE_SYS_66, SEQ_SE_WCS_02 },     { SEQ_SE_SYS_67, SEQ_SE_WCS_03 },
    { SEQ_SE_TDEMO_009, SEQ_SE_WCS_04 }, { SEQ_SE_W235_03, SEQ_SE_W235_03 },
};

static const UISceneStep sCommBtlDemoSteps[SCENE_COUNT] = {
    { CommBtlDemoStart_Begin, NULL, CommBtlDemoStart_Main, NULL, CommBtlDemoStart_End },
    { CommBtlDemoResult_Begin, NULL, CommBtlDemoResult_Main, NULL, CommBtlDemoResult_End },
    { NULL, NULL, CommBtlDemoDraw_Main, NULL, CommBtlDemoDraw_End },
};

const GameProcFunctions CommBtlDemoProc = { CommBtlDemoProc_Init, CommBtlDemoProc_Main, CommBtlDemoProc_End };

// The background of the main screen: the usual one and the Wi-Fi competition's
static const CommBtlDemoBackFiles sCommBtlDemoBackFiles[] = {
    { FILE_BG_PALETTE, FILE_BG_CHARS, FILE_BG_SCREEN },
    { FILE_BG_COMPETITION_PALETTE, FILE_BG_COMPETITION_CHARS, FILE_BG_COMPETITION_SCREEN },
};

static BOOL CommBtlDemoProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    CommBtlDemoParam *prm = param;
    CommBtlDemoWork *wk;

    if (*state == 1 && GFL_FadeIsRunning() == TRUE) {
        return TRUE;
    }
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_COMM_BTL_DEMO, 0x80000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(CommBtlDemoWork), HEAPID_COMM_BTL_DEMO);
    sys_memset(wk, 0, sizeof(CommBtlDemoWork));
    wk->heapId = HEAPID_COMM_BTL_DEMO;
    wk->param = prm;
    wk->type = prm->battle.unk44;
    wk->battleResult = wk->param->battle.result;
    CommBtlDemo_ResetStates(prm);
    wk->graphic = CommBtlDemoGraphic_Create(GX_DISP_SELECT_SUB_MAIN, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_INTRO_BTL_SERVER_FLOW, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->scene = UIScene_Create(wk->heapId, sCommBtlDemoSteps, 3, CommBtlDemo_GetFirstScene(prm), wk);
    CommBtlDemo_LoadBG(&wk->back, wk->heapId, wk->param->competition);
    CommBtlDemoG3D_Init(&wk->g3d, wk->graphic, wk->heapId);
    CommBtlDemoObj_Init(&wk->obj, wk->graphic, wk->heapId);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    func_02042ba8(FALSE, wk->heapId);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
    *state = 1;
    return FALSE;
}

static BOOL CommBtlDemoProc_End(GameProc *proc, u32 *state, void *param, void *work) {
    CommBtlDemoParam *prm = param;
    CommBtlDemoWork *wk = work;
    HeapID heapId;

    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_X, 0);
    if (prm->battle.unk44 == COMM_BTL_DEMO_TYPE_SINGLE_END || prm->battle.unk44 == COMM_BTL_DEMO_TYPE_MULTI_END) {
        CommBtlDemo_AddRecords(wk);
    }
    UIScene_Free(wk->scene);
    GFL_MsgDataFree(wk->msgData);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    CommBtlDemoObj_Free(&wk->obj);
    CommBtlDemoG3D_Free(&wk->g3d);
    CommBtlDemoGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL CommBtlDemoProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    CommBtlDemoWork *wk = work;

    CommBtlDemoG3D_Draw(&wk->g3d);
    if (UIScene_Main(wk->scene)) {
        return TRUE;
    }
    func_02021a3c(wk->printQueue);
    CommBtlDemoGraphic_Update(wk->graphic);
    wk->scroll++;
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_X, wk->scroll / 2);
    return FALSE;
}

static void CommBtlDemo_LoadBG(CommBtlDemoBack *back, HeapID heapId, BOOL competition) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_COMM_BTL_DEMO, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, sCommBtlDemoBackFiles[competition].palette, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, sCommBtlDemoBackFiles[competition].chars, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, sCommBtlDemoBackFiles[competition].screen, 3, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, FILE_SUB_BG_PALETTE, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, FILE_SUB_BG_CHARS, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, FILE_SUB_BG_SCREEN, 6, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static void CommBtlDemo_EndScene(UIScene *scene, CommBtlDemoWork *wk) {
    CommBtlDemo_FreeTrainers(wk);
    CommBtlDemoResult_Free(&wk->result);
    UIScene_SetNext(scene, UI_SCENE_END);
}

static BOOL CommBtlDemoStart_Begin(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;

    CommBtlDemo_InitTrainers(wk);
    return TRUE;
}

// Gives the trainers of the scene's model the palette of their sex
static void CommBtlDemo_BindSinglePalettes(CommBtlDemoWork *wk) {
    CommBtlDemoG3D *g3d = &wk->g3d;
    G3DActor *actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    int i;

    for (i = 0; i < 2; i++) {
        CommBtlDemoG3D_BindPalette(actor, i + 1, wk->trainers[i].gender == 0 ? 1 : 2);
    }
}

static void CommBtlDemo_BindMultiPalettes(CommBtlDemoWork *wk) {
    CommBtlDemoG3D *g3d = &wk->g3d;
    G3DActor *actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    int i;

    for (i = 0; i < 4; i++) {
        CommBtlDemoG3D_BindPalette(actor, i + 1, wk->trainers[i].gender == 0 ? 1 : 3);
    }
}

static u32 CommBtlDemo_GetSE(u32 se, BOOL competition) {
    return sCommBtlDemoSE[se][competition];
}

static BOOL CommBtlDemoStart_Main(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;
    u8 counter = UIScene_GetCounter(scene);

    CommBtlDemo_UpdateTrainers(wk);
    switch (counter) {
    case 0:
        CommBtlDemoG3D_LoadParticles(&wk->g3d, FILE_PARTICLES_BALL);
        if (CommBtlDemo_IsSingle(wk->type)) {
            CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_SINGLE_START);
            CommBtlDemo_BindSinglePalettes(wk);
        } else {
            CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_MULTI_START);
            CommBtlDemo_BindMultiPalettes(wk);
        }
        CommBtlDemo_StartBalls(wk);
        UIScene_IncCounter(scene);
        break;
    case 1:
        if (!CommBtlDemoG3D_UpdateAnim(&wk->g3d)) {
            CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_VS);
            if (wk->param->competition) {
                CommBtlDemoG3D_LoadParticles(&wk->g3d, FILE_PARTICLES_VS_COMPETITION);
            } else {
                CommBtlDemoG3D_LoadParticles(&wk->g3d, FILE_PARTICLES_VS);
            }
            wk->timer = 0;
            UIScene_IncCounter(scene);
        }
        break;
    case 2:
        CommBtlDemoG3D_UpdateAnim(&wk->g3d);
        if (wk->timer == 15) {
            GFL_SndSEPlay(CommBtlDemo_GetSE(SE_VS, wk->param->competition));
            {
                VecFx32 pos = { 0, 0, -100 };

                CommBtlDemoG3D_AddEmitter(&wk->g3d, 0, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                CommBtlDemoG3D_AddEmitter(&wk->g3d, 1, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                CommBtlDemoG3D_AddEmitter(&wk->g3d, 2, &pos);
            }
        } else if (wk->timer == 25) {
            VecFx32 pos = { 0, 0, -100 };

            CommBtlDemoG3D_AddEmitter(&wk->g3d, 3, &pos);
        } else if (wk->timer == 160) {
            GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 0, 16, -3);
        } else if (wk->timer == 150) {
            GFL_SndSEPlay(CommBtlDemo_GetSE(SE_VS_END, wk->param->competition));
        } else if (wk->timer == 168) {
            return TRUE;
        }
        wk->timer++;
        break;
    }
    return FALSE;
}

static BOOL CommBtlDemoStart_End(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;

    CommBtlDemo_FreeTrainers(wk);
    UIScene_SetNext(scene, UI_SCENE_END);
    return TRUE;
}

static BOOL CommBtlDemoResult_Begin(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;
    u16 count0;
    u16 count1;

    CommBtlDemo_InitTrainers(wk);
    if (CommBtlDemo_IsSingle(wk->type)) {
        count0 = CommBtlDemo_CountLeft(&wk->param->battle.players[0]);
        count1 = CommBtlDemo_CountLeft(&wk->param->battle.players[1]);
    } else {
        count0 = CommBtlDemo_CountLeft(&wk->param->battle.players[0]);
        count0 += CommBtlDemo_CountLeft(&wk->param->battle.players[1]);
        count1 = CommBtlDemo_CountLeft(&wk->param->battle.players[2]);
        count1 += CommBtlDemo_CountLeft(&wk->param->battle.players[3]);
    }
    CommBtlDemoResult_Init(&wk->result, &wk->obj, &wk->g3d, count0, count1, wk->type);
    return TRUE;
}

static BOOL CommBtlDemoResult_Main(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;
    u8 counter = UIScene_GetCounter(scene);

    CommBtlDemo_UpdateTrainers(wk);
    CommBtlDemoResult_Update(&wk->result);
    switch (counter) {
    case 0:
        if (CommBtlDemo_IsSingle(wk->type)) {
            CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_SINGLE_END);
        } else {
            CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_MULTI_END);
        }
        CommBtlDemoG3D_LoadParticles(&wk->g3d, FILE_PARTICLES_BALL);
        CommBtlDemo_StartBalls(wk);
        UIScene_IncCounter(scene);
        break;
    case 1:
        if (!CommBtlDemoG3D_UpdateAnim(&wk->g3d)) {
            if (wk->battleResult == COMM_BTL_DEMO_RESULT_DRAW) {
                return TRUE;
            }
            wk->timer = 0;
            CommBtlDemoG3D_LoadParticles(&wk->g3d, FILE_PARTICLES_RESULT);
            UIScene_IncCounter(scene);
        }
        break;
    case 2:
        if (wk->timer == 15) {
            u32 side = wk->battleResult == COMM_BTL_DEMO_RESULT_WIN ? 0 : 1;

            GFL_SndSEPlay(CommBtlDemo_GetSE(SE_RESULT, wk->param->competition));
            CommBtlDemoResult_ShowLabel(&wk->result, side, 0);
        } else if (wk->timer == 24) {
            func_0204c124(wk->result.countActors[wk->battleResult != COMM_BTL_DEMO_RESULT_WIN ? 1 : 0], FALSE);
        } else if (wk->timer == 45) {
            CommBtlDemoResult_ShowLabel(&wk->result, wk->battleResult == COMM_BTL_DEMO_RESULT_LOSE ? 0 : 1, 1);
        } else if (wk->timer == 54) {
            func_0204c124(wk->result.countActors[wk->battleResult != COMM_BTL_DEMO_RESULT_LOSE ? 1 : 0], FALSE);
        } else if (wk->timer == 120) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, -3);
        } else if (wk->timer == 128) {
            return TRUE;
        }
        // The particles around the winner's label
        if (wk->timer % 180 == 0) {
            fx32 x;
            fx32 y;

            if (wk->battleResult == COMM_BTL_DEMO_RESULT_LOSE) {
                x = -0x2800;
                y = 0x2000;
            } else {
                x = 0x2800;
                y = -0x1800;
            }
            {
                VecFx32 pos = { 0, 0, -300 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 0, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -300 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 1, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 2, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 3, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 4, &pos);
            }
            {
                VecFx32 pos = { 0, 0, -100 };

                pos.x = x;
                pos.y = y;
                CommBtlDemoG3D_AddEmitter(&wk->g3d, 5, &pos);
            }
        }
        wk->timer++;
        break;
    }
    return FALSE;
}

static BOOL CommBtlDemoResult_End(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;

    if (wk->battleResult == COMM_BTL_DEMO_RESULT_DRAW) {
        GFL_SndSEPlay(CommBtlDemo_GetSE(SE_DRAW, wk->param->competition));
        UIScene_SetNext(scene, SCENE_DRAW);
        return TRUE;
    }
    CommBtlDemo_EndScene(scene, wk);
    return TRUE;
}

static BOOL CommBtlDemoDraw_Main(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;
    u8 counter = UIScene_GetCounter(scene);

    CommBtlDemo_UpdateTrainers(wk);
    CommBtlDemoResult_Update(&wk->result);
    switch (counter) {
    case 0:
        CommBtlDemoG3D_LoadScene(&wk->g3d, G3D_SCENE_DRAW);
        wk->timer = 0;
        UIScene_IncCounter(scene);
        break;
    case 1:
        if (wk->timer++ == 30) {
            CommBtlDemoResult_ShowLabel(&wk->result, LABEL_MIDDLE, 2);
        }
        if (!CommBtlDemoG3D_UpdateAnim(&wk->g3d)) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, -3);
            return TRUE;
        }
        break;
    case 2:
        break;
    }
    return FALSE;
}

static BOOL CommBtlDemoDraw_End(UIScene *scene, void *param) {
    CommBtlDemoWork *wk = param;

    if (GFL_FadeIsRunning() == TRUE) {
        return FALSE;
    }
    CommBtlDemo_EndScene(scene, wk);
    return TRUE;
}

static u16 CommBtlDemo_GetFirstScene(CommBtlDemoParam *param) {
    switch (param->battle.unk44) {
    case COMM_BTL_DEMO_TYPE_SINGLE_START:
    case COMM_BTL_DEMO_TYPE_MULTI_START:
        return SCENE_START;
    case COMM_BTL_DEMO_TYPE_SINGLE_END:
    case COMM_BTL_DEMO_TYPE_MULTI_END:
        return SCENE_RESULT;
    default:
        return SCENE_START;
    }
}

// Whether the battle is one of two players
static BOOL CommBtlDemo_IsSingle(u8 type) {
    switch (type) {
    case COMM_BTL_DEMO_TYPE_SINGLE_START:
        return TRUE;
    case COMM_BTL_DEMO_TYPE_SINGLE_END:
        return TRUE;
    case COMM_BTL_DEMO_TYPE_MULTI_START:
        return FALSE;
    case COMM_BTL_DEMO_TYPE_MULTI_END:
        return FALSE;
    default:
        return TRUE;
    }
}

// Whether the screen is the one before the battle
static BOOL CommBtlDemo_IsStart(u8 type) {
    switch (type) {
    case COMM_BTL_DEMO_TYPE_SINGLE_START:
        return TRUE;
    case COMM_BTL_DEMO_TYPE_SINGLE_END:
        return FALSE;
    case COMM_BTL_DEMO_TYPE_MULTI_START:
        return TRUE;
    case COMM_BTL_DEMO_TYPE_MULTI_END:
        return FALSE;
    default:
        return TRUE;
    }
}

static u32 CommBtlDemo_GetBallAnim(PartyPkm *pkm, u32 state) {
    if (pkm == NULL) {
        return ANIM_BALL_NONE;
    }
    switch (state) {
    case COMM_BTL_DEMO_POKE_NONE:
        return ANIM_BALL_NONE;
    case COMM_BTL_DEMO_POKE_HEALTHY:
        return ANIM_BALL_HEALTHY;
    case COMM_BTL_DEMO_POKE_SICK:
        return ANIM_BALL_SICK;
    case COMM_BTL_DEMO_POKE_FAINTED:
        return ANIM_BALL_FAINTED;
    }
    return ANIM_BALL_HEALTHY;
}

static void CommBtlDemoBalls_Init(CommBtlDemoBalls *balls, PokeParty *party, u8 type, u8 index, CommBtlDemoObj *obj,
                                  CommBtlDemoG3D *g3d, const u8 *states) {
    int i;

    balls->type = type;
    balls->index = index;
    balls->party = party;
    balls->pkmCount = PokeParty_GetPkmCount(party);
    balls->ballCount = CommBtlDemo_IsSingle(type) ? BALL_MAX : BALL_MAX_MULTI;
    balls->g3d = g3d;
    balls->timer = 0;
    balls->states = states;
    for (i = 0; i < balls->ballCount; i++) {
        s16 x;
        s16 y;
        PartyPkm *pkm;
        u32 anim;

        if (CommBtlDemo_IsSingle(type)) {
            x = i * 16 + 136;
            y = index == 0 ? 168 : 56;
            // The actor's position is a byte, so this is 120 - i * 16
            if (index == 0) {
                x *= -1;
            }
        } else {
            switch (index) {
            case 0:
                x = i * -16 + 80;
                y = 136;
                break;
            case 1:
                x = i * -16 + 144;
                y = 176;
                break;
            case 2:
                x = i * 16 + 128;
                y = 48;
                break;
            case 3:
                x = i * 16 + 192;
                y = 88;
                break;
            }
        }
        pkm = NULL;
        if (i < balls->pkmCount) {
            pkm = PokeParty_GetPkm(party, i);
        }
        anim = CommBtlDemo_GetBallAnim(pkm, states[i]);
        // Before the battle, nothing shows what state the Pokémon are in
        if (!CommBtlDemo_IsStart(type) && anim != ANIM_BALL_NONE) {
            anim = ANIM_BALL_HEALTHY;
        }
        balls->actors[i] = CommBtlDemoObj_CreateActor(obj, x, y, anim);
        if (CommBtlDemo_IsStart(type)) {
            func_0204c124(balls->actors[i], FALSE);
        } else {
            func_0204c124(balls->actors[i], TRUE);
        }
    }
}

static void CommBtlDemoBalls_Free(CommBtlDemoBalls *balls) {
    int i;

    for (i = 0; i < balls->ballCount; i++) {
        func_0204c108(balls->actors[i]);
    }
}

// Shows the next ball before the battle, or the state of the next Pokémon after it, with particles where the row of
// balls is, start frames after the row began
static void CommBtlDemoBalls_Step(CommBtlDemoBalls *balls, u32 start) {
    int frame = balls->timer - start;
    int i;

    if (frame < balls->ballCount) {
        fx32 x;
        fx32 y;

        if (frame == 0 && balls->index == 0) {
            GFL_SndSEPlay(CommBtlDemo_GetSE(SE_BALL, FALSE));
        }
        if (CommBtlDemo_IsSingle(balls->type)) {
            if (balls->index == 0) {
                x = -0x400 - frame * 0x800;
                y = -0x2500;
            } else if (balls->index == 1) {
                x = frame * 0x800 + 0x300;
                y = 0x1300;
            }
        } else {
            if (balls->index == 0) {
                x = -0x1800 - frame * 0x800;
                y = -0x1500;
            } else if (balls->index == 1) {
                x = 0x800 - frame * 0x800;
                y = -0x2800;
            } else if (balls->index == 2) {
                x = frame * 0x800 - 0x100;
                y = 0x1800;
            } else if (balls->index == 3) {
                x = frame * 0x800 + 0x1f00;
                y = 0x300;
            }
        }
        {
            VecFx32 pos = { 0, 0, -100 };

            pos.x = x;
            pos.y = y;
            CommBtlDemoG3D_AddEmitters(balls->g3d, &pos);
        }
        if (CommBtlDemo_IsStart(balls->type)) {
            func_0204c124(balls->actors[frame], TRUE);
        } else {
            CommBtlDemoBalls_SetState(balls, balls->party, balls->states, balls->ballCount - 1 - frame);
        }
    } else {
        for (i = 0; i < balls->ballCount; i++) {
            func_0204c468(balls->actors[i], 1);
        }
        balls->started = FALSE;
    }
}

// The values the screen was tuned with. The order of the ROM's data puts all of them here, after the last function with
// a local initializer and before the one that reads the frames. First the camera of the particles
static VecFx32 sCommBtlDemoParticleCameraPosition = { 0, 0, FX32_CONST(128) };
static VecFx32 sCommBtlDemoParticleCameraUp = { 0, FX32_ONE, 0 };
static VecFx32 sCommBtlDemoParticleCameraTarget = { 0, 0, 0 };

// How many frames the 3D animations advance each frame
static int sCommBtlDemoAnimSpeed = 1;

// The frames a trainer's first Poké Ball appears at. Their reads are folded into the code, and the ROM still has all
// six, which it would not if they were static
const u32 CommBtlDemoSingleStartDelay = 12;
const u32 CommBtlDemoMultiStartDelay1 = 15;
const u32 CommBtlDemoMultiStartDelay2 = 19;
const u32 CommBtlDemoSingleEndDelay = 8;
const u32 CommBtlDemoMultiEndDelay1 = 11;
const u32 CommBtlDemoMultiEndDelay2 = 15;

static void CommBtlDemoBalls_Update(CommBtlDemoBalls *balls) {
    u32 start;

    if (!balls->started) {
        return;
    }
    // BUG: start is not set when the screen was called with a type of 0
    switch (balls->type) {
#ifdef BUGFIX
    default:
#endif
    case COMM_BTL_DEMO_TYPE_SINGLE_START:
        start = CommBtlDemoSingleStartDelay;
        break;
    case COMM_BTL_DEMO_TYPE_MULTI_START:
        if (balls->index == 0 || balls->index == 2) {
            start = CommBtlDemoMultiStartDelay1;
        } else if (balls->index == 1 || balls->index == 3) {
            start = CommBtlDemoMultiStartDelay2;
        }
        break;
    case COMM_BTL_DEMO_TYPE_SINGLE_END:
        start = CommBtlDemoSingleEndDelay;
        break;
    case COMM_BTL_DEMO_TYPE_MULTI_END:
        if (balls->index == 0 || balls->index == 2) {
            start = CommBtlDemoMultiEndDelay1;
        } else if (balls->index == 1 || balls->index == 3) {
            start = CommBtlDemoMultiEndDelay2;
        }
        break;
    }
    if (balls->timer >= start) {
        CommBtlDemoBalls_Step(balls, start);
    }
    balls->timer++;
}

static void CommBtlDemoBalls_Start(CommBtlDemoBalls *balls) {
    balls->started = TRUE;
}

static void CommBtlDemoBalls_SetState(CommBtlDemoBalls *balls, PokeParty *party, const u8 *states, int index) {
    PartyPkm *pkm = NULL;

    if (index < PokeParty_GetPkmCount(party)) {
        pkm = PokeParty_GetPkm(balls->party, index);
    }
    func_0204c4b8(balls->actors[index], CommBtlDemo_GetBallAnim(pkm, states[index]));
}

static BmpWin *CommBtlDemo_CreateNameWindow(u8 type, u8 index) {
    int x;
    int y;

    if (CommBtlDemo_IsSingle(type)) {
        if (index == 0) {
            x = 4;
            y = 17;
        } else {
            x = 16;
            y = 3;
        }
    } else {
        switch (index) {
        case 0:
            x = 5;
            y = 13;
            break;
        case 1:
            x = 13;
            y = 19;
            break;
        case 2:
            x = 15;
            y = 1;
            break;
        case 3:
            x = 23;
            y = 7;
            break;
        }
    }
    return BmpWin_CreateDynamic(1, x, y, 10, 3, 1, TRUE);
}

static void CommBtlDemoTrainer_Init(CommBtlDemoTrainer *trainer, u8 type, u8 index, BattlePlayer *player,
                                    CommBtlDemoObj *obj, CommBtlDemoG3D *g3d, Font *font) {
    trainer->player = player;
    trainer->obj = obj;
    trainer->g3d = g3d;
    trainer->font = font;
    trainer->type = type;
    trainer->index = index;
    trainer->timer = 0;
    CommBtlDemoBalls_Init(&trainer->balls, player->party, type, index, obj, g3d, player->order);
    trainer->name = GFL_StrBufCreate(16, HEAPID_COMM_BTL_DEMO);
    textCopy(trainer->player->info->name, trainer->name);
    trainer->gender = getTrainerGender(trainer->player->info);
    trainer->nameWindow = CommBtlDemo_CreateNameWindow(type, index);
    if (!CommBtlDemo_IsStart(trainer->type)) {
        CommBtlDemoTrainer_PrintName(trainer, trainer->font);
    }
}

// Before the battle, the name appears when the balls have
static void CommBtlDemoTrainer_UpdateName(CommBtlDemoTrainer *trainer) {
    if (trainer->timer == 45) {
        CommBtlDemoTrainer_PrintName(trainer, trainer->font);
    }
}

static void CommBtlDemoTrainer_Free(CommBtlDemoTrainer *trainer) {
    GFL_StrBufFree(trainer->name);
    CommBtlDemoBalls_Free(&trainer->balls);
    BmpWin_Free(trainer->nameWindow);
}

static void CommBtlDemoTrainer_Update(CommBtlDemoTrainer *trainer) {
    CommBtlDemoBalls_Update(&trainer->balls);
    if (CommBtlDemo_IsStart(trainer->type)) {
        CommBtlDemoTrainer_UpdateName(trainer);
    }
    trainer->timer++;
}

static void CommBtlDemoTrainer_PrintName(CommBtlDemoTrainer *trainer, Font *font) {
    GFL_BitmapFill(BmpWin_GetBitmap(trainer->nameWindow), 0);
    if (trainer->gender == 0) {
        GFL_TextRndUpdateColorIndexLUT(5, 6, 0);
    } else {
        GFL_TextRndUpdateColorIndexLUT(3, 4, 0);
    }
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(trainer->nameWindow), 1, 1, trainer->name, font);
    BmpWin_Transfer(trainer->nameWindow);
}

static void CommBtlDemo_InitTrainers(CommBtlDemoWork *wk) {
    int count = CommBtlDemo_IsSingle(wk->type) ? 2 : 4;
    int i;

    for (i = 0; i < count; i++) {
        CommBtlDemoTrainer_Init(&wk->trainers[i], wk->type, i, &wk->param->battle.players[i], &wk->obj, &wk->g3d,
                                wk->font);
    }
}

static void CommBtlDemo_FreeTrainers(CommBtlDemoWork *wk) {
    int count = CommBtlDemo_IsSingle(wk->type) ? 2 : 4;
    int i;

    for (i = 0; i < count; i++) {
        CommBtlDemoTrainer_Free(&wk->trainers[i]);
    }
}

static void CommBtlDemo_UpdateTrainers(CommBtlDemoWork *wk) {
    int count = CommBtlDemo_IsSingle(wk->type) ? 2 : 4;
    int i;

    for (i = 0; i < count; i++) {
        CommBtlDemoTrainer_Update(&wk->trainers[i]);
    }
}

static void CommBtlDemo_StartBalls(CommBtlDemoWork *wk) {
    int count = CommBtlDemo_IsSingle(wk->type) ? 2 : 4;
    int i;

    for (i = 0; i < count; i++) {
        CommBtlDemoBalls_Start(&wk->trainers[i].balls);
    }
}

// Counts a battle of a kind, which is one of records 13, 17 and 37, in that record and in the record of its result
static void CommBtlDemo_AddRecord(GameRecords *records, u32 id, u32 result) {
    switch (id) {
    case 13:
        if (result == COMM_BTL_DEMO_RESULT_WIN) {
            RecordAddOne(records, 14);
        } else if (result == COMM_BTL_DEMO_RESULT_LOSE) {
            RecordAddOne(records, 15);
        }
        break;
    case 37:
        if (result == COMM_BTL_DEMO_RESULT_WIN) {
            RecordAddOne(records, 38);
        } else if (result == COMM_BTL_DEMO_RESULT_LOSE) {
            RecordAddOne(records, 39);
        }
        break;
    case 17:
        if (result == COMM_BTL_DEMO_RESULT_WIN) {
            RecordAddOne(records, 18);
        } else if (result == COMM_BTL_DEMO_RESULT_LOSE) {
            RecordAddOne(records, 19);
        } else if (result == COMM_BTL_DEMO_RESULT_DRAW) {
            RecordAddOne(records, 20);
        }
        break;
    }
    RecordAddOne(records, id);
}

// Counts the battle in the records of the kind of connection it was fought over
static void CommBtlDemo_AddRecords(CommBtlDemoWork *wk) {
    if (wk->param->records == NULL) {
        return;
    }
    switch (func_02042d34()) {
    case 12:
    case 31:
    case 37:
        CommBtlDemo_AddRecord(wk->param->records, 37, wk->battleResult);
        break;
    case 10:
        CommBtlDemo_AddRecord(wk->param->records, 17, wk->battleResult);
        break;
    case 20:
    case 21:
    case 23:
    case 24:
    case 25:
    case 26:
        CommBtlDemo_AddRecord(wk->param->records, 13, wk->battleResult);
        break;
    }
}

static void CommBtlDemoResult_Init(CommBtlDemoResult *result, CommBtlDemoObj *obj, CommBtlDemoG3D *g3d, u16 count0,
                                   u16 count1, u8 type) {
    int i;

    result->obj = obj;
    result->g3d = g3d;
    result->type = type;
    result->counts[0] = count0;
    result->counts[1] = count1;
    result->countActors[0] = CommBtlDemoObj_CreateCount(obj, 0, result->counts[0]);
    result->countActors[1] = CommBtlDemoObj_CreateCount(obj, 1, result->counts[1]);
    for (i = 0; i < LABEL_COUNT; i++) {
        s16 x;
        s16 y;

        switch (i) {
        case LABEL_SIDE0:
            x = 206;
            y = 152;
            break;
        case LABEL_SIDE1:
            x = 50;
            y = 40;
            break;
        case LABEL_MIDDLE:
            x = 128;
            y = 96;
            break;
        }
        result->labels[i] = CommBtlDemoObj_CreateActor(obj, x, y, ANIM_WIN);
        func_0204c124(result->labels[i], FALSE);
    }
}

static void CommBtlDemoResult_Free(CommBtlDemoResult *result) {
    int i;

    for (i = 0; i < LABEL_COUNT; i++) {
        func_0204c108(result->labels[i]);
    }
    for (i = 0; i < SIDE_COUNT; i++) {
        func_0204c108(result->countActors[i]);
    }
}

static void CommBtlDemoResult_Update(CommBtlDemoResult *result) {
    CommBtlDemo_IsSingle(result->type);
    if (result->timer == 30) {
        func_0204c124(result->countActors[0], TRUE);
        func_0204c520(result->countActors[0], TRUE);
        func_0204c124(result->countActors[1], TRUE);
        func_0204c520(result->countActors[1], TRUE);
    }
    result->timer++;
}

// Shows a label as that of the winner (0), of the loser (1) or of a draw (2)
static void CommBtlDemoResult_ShowLabel(CommBtlDemoResult *result, u32 label, u32 kind) {
    u32 anim;

    switch (kind) {
    case 0:
        anim = ANIM_WIN;
        break;
    case 1:
        anim = ANIM_LOSE;
        break;
    case 2:
        anim = ANIM_DRAW;
        break;
    }
    func_0204c124(result->labels[label], TRUE);
    func_0204c520(result->labels[label], TRUE);
    func_0204c4b8(result->labels[label], anim);
}

static void CommBtlDemoObj_Init(CommBtlDemoObj *obj, CommBtlDemoGraphic *graphic, HeapID heapId) {
    UIObjResSetup setup;

    obj->graphic = graphic;
    obj->heapId = heapId;
    setup.vramType = 0;
    setup.flags = 0;
    setup.arcId = ARCID_COMM_BTL_DEMO;
    setup.paletteFile = FILE_OBJ_PALETTE;
    setup.charFile = FILE_OBJ_CHARS;
    setup.cellFile = FILE_OBJ_CELLS;
    setup.animFile = FILE_OBJ_ANIMS;
    setup.paletteOffset = 0;
    setup.paletteStart = 0;
    setup.paletteCount = 4;
    UIObjRes_Load(&obj->res, &setup, CommBtlDemoGraphic_GetClActUnit(graphic), heapId);
}

static void CommBtlDemoObj_Free(CommBtlDemoObj *obj) {
    UIObjRes_Free(&obj->res);
}

static ClActor *CommBtlDemoObj_CreateActor(CommBtlDemoObj *obj, s16 x, s16 y, u16 anim) {
    return UIObjRes_CreateActor(&obj->res, CommBtlDemoGraphic_GetClActUnit(obj->graphic), x, y, anim, obj->heapId);
}

// The number of Pokémon a side has left, hidden until the balls have shown what became of them
static ClActor *CommBtlDemoObj_CreateCount(CommBtlDemoObj *obj, u32 side, u8 count) {
    s16 x;
    s16 y;
    ClActor *actor;

    if (side == 0) {
        x = 206;
        y = 152;
    } else {
        x = 50;
        y = 40;
    }
    actor = CommBtlDemoObj_CreateActor(obj, x, y, ANIM_COUNT_0 + count);
    func_0204c124(actor, FALSE);
    func_0204c438(actor, 1);
    return actor;
}

static void CommBtlDemoG3D_Init(CommBtlDemoG3D *g3d, CommBtlDemoGraphic *graphic, HeapID heapId) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG0, 0x1f, 0, 0);
    g3d->graphic = graphic;
    g3d->heapId = heapId;
    g3d->manager = GFL_G3DMgrCreate(16, 20, heapId);
    func_0204f918(heapId);
}

static void CommBtlDemoG3D_Free(CommBtlDemoG3D *g3d) {
    CommBtlDemoG3D_FreeScene(g3d);
    CommBtlDemoG3D_FreeParticles(g3d);
    GFL_G3DMgrFree(g3d->manager);
}

static void CommBtlDemoG3D_Draw(CommBtlDemoG3D *g3d) {
    CommBtlDemoGraphic_Begin3D(g3d->graphic);
    if (g3d->particles != NULL) {
        func_0204f954();
    }
    if (g3d->sceneLoaded) {
        SRTMatrix matrix;

        matrix.translation.x = 0;
        matrix.translation.y = 0;
        matrix.translation.z = 0;
        matrix.scale.x = FX32_ONE;
        matrix.scale.y = FX32_ONE;
        matrix.scale.z = FX32_ONE;
        MTX_Identity33_(&matrix.rotation);
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(g3d->manager, g3d->scene), &matrix);
    }
    CommBtlDemoGraphic_End3D(g3d->graphic);
}

// Replaces the particles with those of a file
static void CommBtlDemoG3D_LoadParticles(CommBtlDemoG3D *g3d, u32 file) {
    G3DCameraProjection projection;
    void *resource;

    if (g3d->particles != NULL) {
        CommBtlDemoG3D_ClearParticles(g3d);
    }
    g3d->particles = func_0204f968(g3d->particleWork, PARTICLE_WORK_SIZE, TRUE, g3d->heapId);
    projection.type = G3DCAM_PROJECTION_ORTHO;
    projection.param1 = FX32_CONST(3);
    projection.param2 = -FX32_CONST(3);
    projection.param3 = -FX32_CONST(4);
    projection.param4 = FX32_CONST(4);
    projection.near = FX32_ONE;
    projection.far = FX32_CONST(1024);
    projection.ndcRangeOverride = 0;
    func_02050178(g3d->particles);
    func_020500cc(g3d->particles, &projection, FX32_CONST(2), &sCommBtlDemoParticleCameraPosition,
                  &sCommBtlDemoParticleCameraUp, &sCommBtlDemoParticleCameraTarget, g3d->heapId);
    resource = func_0204fdf8(ARCID_COMM_BTL_DEMO, file, g3d->heapId);
    g3d->emitterCount = func_020503f0(resource);
    func_0204fe04(g3d->particles, resource, FALSE, GFL_VBlankGetTCBMgr());
}

static void CommBtlDemoG3D_ClearParticles(CommBtlDemoG3D *g3d) {
    func_020500b0(g3d->particles);
    func_0204fa84(g3d->particles);
    g3d->particles = NULL;
}

static void CommBtlDemoG3D_FreeParticles(CommBtlDemoG3D *g3d) {
    if (g3d->particles != NULL) {
        func_0204fa84(g3d->particles);
    }
    func_0204fb4c();
}

static void CommBtlDemoG3D_AddEmitter(CommBtlDemoG3D *g3d, u16 emitter, const VecFx32 *pos) {
    func_0205006c(g3d->particles, emitter, pos);
}

// Adds every emitter of the particle file
static void CommBtlDemoG3D_AddEmitters(CommBtlDemoG3D *g3d, const VecFx32 *pos) {
    int i;

    for (i = 0; i < g3d->emitterCount; i++) {
        CommBtlDemoG3D_AddEmitter(g3d, i, pos);
    }
}

// Binds one of the texture's palettes to the materials of an actor's model that use another
static BOOL CommBtlDemoG3D_BindPalette(G3DActor *actor, u32 plttToMatListIdx, u32 plttNameIdx) {
    G3DModel *model = GFL_G3DActorGetMdl(actor);
    void *texResource = GFL_G3DMdlGetTexResource(model);
    NNSG3dResMdl *resMdl = GFL_G3DMdlGetEngineModel(model)->resMdl;
    NNSG3dResTex *tex = GFL_G3DResGetTexData(texResource);

    NNS_G3dReleaseMdlPltt(resMdl);
    return NNS_G3dForceBindMdlPltt(resMdl, tex, plttToMatListIdx, plttNameIdx) & 1;
}

static void CommBtlDemoG3D_LoadScene(CommBtlDemoG3D *g3d, u32 scene) {
    G3DActor *actor;
    int i;
    int count;

    if (g3d->sceneLoaded) {
        CommBtlDemoG3D_DeleteScene(g3d);
    }
    g3d->scene = GFL_G3DMgrNewScene(g3d->manager, &sCommBtlDemoScenes[scene]);
    g3d->sceneLoaded = TRUE;
    actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    count = GFL_G3DActorGetAnmCount(actor);
    for (i = 0; i < count; i++) {
        GFL_G3DActorBindAnm(actor, i);
    }
}

static void CommBtlDemoG3D_DeleteScene(CommBtlDemoG3D *g3d) {
    GFL_G3DMgrDeleteScene(g3d->manager, g3d->scene);
    g3d->sceneLoaded = FALSE;
    g3d->animEnded = FALSE;
}

static void CommBtlDemoG3D_FreeScene(CommBtlDemoG3D *g3d) {
    if (g3d->sceneLoaded) {
        CommBtlDemoG3D_DeleteScene(g3d);
    }
}

// Steps the scene's animations, and returns FALSE once they have ended
static BOOL CommBtlDemoG3D_UpdateAnim(CommBtlDemoG3D *g3d) {
    BOOL playing = TRUE;
    G3DActor *actor;
    int count;
    int i;

    if (g3d->animEnded) {
        return FALSE;
    }
    actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    count = GFL_G3DActorGetAnmCount(actor);
    for (i = 0; i < count; i++) {
        playing = GFL_G3DActorStepAnmFrameLoop(actor, i, sCommBtlDemoAnimSpeed * FX32_ONE);
        if (!playing) {
            g3d->animEnded = TRUE;
        }
    }
    return playing;
}

// Before the battle, every Pokémon is healthy
static void CommBtlDemo_ResetStates(CommBtlDemoParam *param) {
    int i;
    int j;

    if (param->battle.unk44 == COMM_BTL_DEMO_TYPE_SINGLE_START) {
        for (i = 0; i < 2; i++) {
            for (j = 0; j < PokeParty_GetPkmCount(param->battle.players[i].party); j++) {
                param->battle.players[i].order[j] = COMM_BTL_DEMO_POKE_HEALTHY;
            }
        }
    } else if (param->battle.unk44 == COMM_BTL_DEMO_TYPE_MULTI_START) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < PokeParty_GetPkmCount(param->battle.players[i].party); j++) {
                param->battle.players[i].order[j] = COMM_BTL_DEMO_POKE_HEALTHY;
            }
        }
    }
}

// How many of a player's Pokémon have not fainted
static u32 CommBtlDemo_CountLeft(BattlePlayer *player) {
    PokeParty *party = player->party;
    int i;
    u32 count = 0;
    const u8 *states = player->order;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        if (states[i] == COMM_BTL_DEMO_POKE_HEALTHY || states[i] == COMM_BTL_DEMO_POKE_SICK) {
            count++;
        }
    }
    return count;
}
