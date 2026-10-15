#include "app/comm_tvt/ctvt_game.h"
#include "types.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "app/comm_tvt/ctvt_game_balloon.h"
#include "app/comm_tvt/ctvt_game_cam.h"
#include "app/comm_tvt/ctvt_game_target.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nnsys/snd.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Xtransceiver's minigames, which the host starts from the talk mode for everyone in the call. Each member first
// takes pictures of themselves (ctvt_game_cam.c), which go on the 3D faces or balloons, then they play for 60 seconds:
// in the target game the members' faces float up the top screen and are scored by touching them, the host checking
// each touch; in the balloon game each member pumps their own balloon, which pops at the fourth stage. The sub screen
// shows everyone's score, ranked, and the results end with a menu to play again

// The steps of CtvtGame_Main
enum {
    CTVT_GAME_STATE_FADE_OUT_EXIT,
    CTVT_GAME_STATE_EXIT,
    CTVT_GAME_STATE_UNUSED,
    CTVT_GAME_STATE_FREE_CAMERA,
    CTVT_GAME_STATE_START_CAMERA,
    CTVT_GAME_STATE_CAMERA,
    CTVT_GAME_STATE_FADE_OUT_CAMERA,
    CTVT_GAME_STATE_INIT_PLAY,
    CTVT_GAME_STATE_PLAY,
    CTVT_GAME_STATE_FREE_PLAY,
};

// The steps of a game, CtvtGame_UpdatePlay's. The members wait for each other through the net's timing numbers
enum {
    CTVT_GAME_PLAY_SEND_SEED,
    CTVT_GAME_PLAY_SYNC_SEED,
    CTVT_GAME_PLAY_WAIT_SEED,
    CTVT_GAME_PLAY_INIT,
    CTVT_GAME_PLAY_FADE_IN,
    CTVT_GAME_PLAY_INTRO,
    CTVT_GAME_PLAY_WAIT_FADE_IN,
    CTVT_GAME_PLAY_WAIT_INTRO_FADE_OUT,
    CTVT_GAME_PLAY_WAIT_SYNC_START,
    CTVT_GAME_PLAY_TARGETS,
    CTVT_GAME_PLAY_TARGETS_END,
    CTVT_GAME_PLAY_BALLOON_START,
    CTVT_GAME_PLAY_BALLOONS,
    CTVT_GAME_PLAY_SYNC_END,
    CTVT_GAME_PLAY_WAIT_SYNC_END,
    CTVT_GAME_PLAY_FADE_OUT,
    CTVT_GAME_PLAY_WAIT_FADE_OUT,
    CTVT_GAME_PLAY_INIT_RESULTS,
    CTVT_GAME_PLAY_FADE_IN_RESULTS,
    CTVT_GAME_PLAY_WAIT_FADE_IN_RESULTS,
    CTVT_GAME_PLAY_RESULTS,
    CTVT_GAME_PLAY_ASK_REPLAY,
    CTVT_GAME_PLAY_FADE_OUT_MENU,
    CTVT_GAME_PLAY_WAIT_FADE_OUT_MENU,
    CTVT_GAME_PLAY_REPLAY_MENU,
    CTVT_GAME_PLAY_HOST_WAIT_JOINED,
    CTVT_GAME_PLAY_HOST_WAIT_READY,
    CTVT_GAME_PLAY_HOST_EXIT,
    CTVT_GAME_PLAY_CHILD_WAIT_JOINED,
    CTVT_GAME_PLAY_CHILD_READY,
    CTVT_GAME_PLAY_CHILD_EXIT,
    CTVT_GAME_PLAY_SYNC_FREE,
    CTVT_GAME_PLAY_WAIT_SYNC_FREE,
    CTVT_GAME_PLAY_FREE,
};

// A member's score on the sub screen, or a slot of the call that nobody is in
typedef struct {
    TCB *tcb;
    u16 heapId;
    u8 state;
    u8 rank;
    u8 pos;
    u8 netId;
    u16 score;
    BOOL absent;
    BOOL isSelf;
    BOOL shaking;
    s8 shakeFrame;
    u8 shakeWait;
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    BmpWin *nameWindow;
    BmpWin *scoreWindow;
    BOOL namePending;
    BOOL scorePending;
    StrBuf *name;
    StrBuf *scoreFormat;
    StrBuf *scoreStr;
    u8 slideWait;
    int unk48;
    BOOL sliding;
    ClActor *icon;
    ClActor *digits[3];
    ClActor *frame;
} CtvtGamePlayer;

struct CtvtGame {
    u16 heapId;
    int type;
    GameRecords *records;
    int state;
    u8 playState;
    u8 resultState;
    // Whether everyone agreed to play again
    BOOL replay;
    // The others in the call
    CtvtGameMember members[3];
    CtvtGameCam *cam;
    CtvtGameCamTask *camTask;
    TCBManager *tcbManager;
    void *tcbBuffer;
    CtvtGameTarget targets[8];
    // What the last target was launched with, which the next avoids
    u8 lastColumn;
    u8 lastAngle;
    u8 nextDepth;
    // Which of sFaceOrders the faces come up in, and where in it
    u8 faceOrder;
    u8 faceOrderPos;
    CtvtGameBalloon *balloons[4];
    u8 pumps[4];
    u8 stages[4];
    u8 wobbleCooldown;
    u8 rightPuff;
    u8 leftPuff;
    u16 pumpSeq;
    // In seconds
    u16 timeLeft;
    u32 lastTick;
    BmpWin *timeWindow;
    BmpWin *titleWindow;
    BOOL titlePending;
    BmpWin *infoWindow;
    BOOL infoPending;
    u16 introFrame;
    TCBManager *playerTcbManager;
    void *playerTcbBuffer;
    CtvtGamePlayer *players[4];
    // Whether the scores slide out and back in their new order
    BOOL sorting;
    CtvtGameShot *shots[4];
    void *paletteFile;
    NNSG2dPaletteData *palette;
    u8 paletteFrame;
    u16 resultTimer;
    BmpWin *messageWindow;
    BOOL messagePending;
    u16 exitWait;
    u16 menuFrame;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    AppTaskMenu *yesNoMenu;
    // What the others said in the menu to play again: the host sees a child quit, the children see the host quit,
    // everyone join and the game start again
    BOOL childQuit;
    BOOL hostQuit;
    BOOL allJoined;
    BOOL replayStarted;
    u8 joinedMask;
    u8 readyMask;
    u32 seed;
    // The host's frame, which every machine moves the game by
    u16 frame;
    BOOL frameReceived;
    BOOL playing;
    u16 hostFrame;
    s16 spawnWait;
    u8 spawnNow;
    int scores[4];
    BOOL g3dActive;
    u32 unk524;
    // The palettes, characters and cells of the sub screen, then the main screen
    u32 clactRes[6];
    ClActUnit *clactUnit;
    ClActor *introActors[3];
    ClActor *targetSigns[2];
    ClActor *scoreFrame;
    ClActor *hurrySign;
    ClActor *pump;
    ClActor *pumpBase;
    ClActor *pumpAir;
    ClActor *startSign;
    ClActor *rightPuffs[3];
    ClActor *leftPuffs[3];
    ClActor *resultFrame;
    ClActor *crown;
    ClActor *crownSparkles[6];
    ClActor *winBursts[2];
    ClActor *winFlashes[2];
    ClActor *winBanner;
    ClActor *winGlow;
    ClActor *rankSign;
    ClActor *tryAgainSign;
    G3DManager *g3d;
    u16 scenes[5];
    G3DCamera *camera;
    TCB *vblankTcb;
};

static BOOL CtvtGame_AreAllJoined(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_AreAllReady(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_InitPlay(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_FreePlay(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_Abort(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdatePlay(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_InitCommon(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_InitTargets(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_InitBalloons(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdateIntro(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_StartTargets(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_StartBalloons(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdateTargets(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdateBalloonStart(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdateBalloons(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_InitResults(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_UpdateResults(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_FreeCommon(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_FreeTargets(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_FreeBalloons(CommTvtWork *sys, CtvtGame *game);
static BOOL CtvtGame_CheckError(CommTvtWork *sys, CtvtGame *game);
static u32 CtvtGame_Rand(u32 max);
static void CtvtGame_InitGraphics(CtvtGame *game, HeapID heapId);
static void CtvtGame_FreeGraphics(CtvtGame *game);
static void CtvtGame_DrawTargets3D(CtvtGame *game);
static void CtvtGame_Draw3D(CtvtGame *game);
static void CtvtGame_VBlank(TCB *tcb, void *data);
static void CtvtGame_DeleteActor(ClActor *actor);
static void CtvtGame_InitBGs(CtvtGame *game, HeapID heapId);
static void CtvtGame_FreeBGs(CtvtGame *game);
static void CtvtGame_InitBG(const BGSetup *setup, u8 bg, u8 mode);
static void CtvtGame_Init3D(CtvtGame *game, HeapID heapId);
static void CtvtGame_Free3D(CtvtGame *game);
static void CtvtGame_LoadTargetResources(CtvtGame *game, HeapID heapId, ArcTool *arc);
static void CtvtGame_FreeTargetResources(CtvtGame *game);
static void CtvtGame_LoadBalloonResources(CtvtGame *game, HeapID heapId, ArcTool *arc);
static void CtvtGame_FreeBalloonResources(CtvtGame *game);
static void CtvtGame_LoadResultScenes(CtvtGame *game, HeapID heapId);
static void CtvtGame_PrintMessage(CommTvtWork *sys, CtvtGame *game, u32 msgId);
static void CtvtGame_DrawTimeBar(CtvtGame *game);
static BOOL CtvtGame_CheckQuit(CommTvtWork *sys, CtvtGame *game);
static AppTaskMenu *CtvtGame_CreateReplayMenu(CommTvtWork *sys, CtvtGame *game);
static AppTaskMenu *CtvtGame_CreateCancelMenu(CommTvtWork *sys, CtvtGame *game);
static void CtvtGame_LaunchTarget(CtvtGame *game, CtvtGameTarget *target, u16 scene);
static BOOL CtvtGame_AreTargetsGone(CtvtGame *game);
static u8 CtvtGame_NextFace(CtvtGame *game);
static BOOL CtvtGame_FindFreeTarget(CtvtGame *game, u8 *index);
static void CtvtGame_TargetTask(TCB *tcb, void *data);
static void CtvtGame_BalloonTask(TCB *tcb, void *data);
static void CtvtGame_ShotTask(TCB *tcb, void *data);
static BOOL CtvtGame_FindTouchedTarget(CtvtGame *game, u16 frame, int x, int y, u16 *index);
static void CtvtGame_SeparateTargets(CtvtGame *game);
static void CtvtGame_Separate(fx32 *x0, fx32 *y0, fx32 *x1, fx32 *y1, fx32 minDist, u8 mode);
static void CtvtGame_DrawResults3D(CtvtGame *game);
static CtvtGamePlayer *CtvtGamePlayer_Create(CommTvtWork *sys, CtvtGame *game, u8 netId, u8 pos);
static CtvtGamePlayer *CtvtGamePlayer_CreateAbsent(CommTvtWork *sys, CtvtGame *game, u8 netId);
static void CtvtGamePlayer_Delete(CtvtGamePlayer *player);
static void CtvtGame_UpdatePlayers(CtvtGame *game);
static void CtvtGamePlayer_Show(CommTvtWork *sys, CtvtGamePlayer *player);
#ifdef BUGFIX
static void CtvtGame_SetScore(CtvtGame *game, u8 netId, u16 score);
#else
static void CtvtGame_SetScore(CtvtGame *game, u8 netId, u8 score);
#endif
static void CtvtGame_StartSort(CtvtGame *game, BOOL hurry);
static void CtvtGame_Rank(CtvtGame *game);
static BOOL CtvtGamePlayer_Slide(CtvtGamePlayer *player, int dx);
static void CtvtGamePlayer_PlaceAtRank(CtvtGamePlayer *player);
static void CtvtGamePlayer_Task(TCB *tcb, void *data);
static void CtvtGamePlayer_InitResult(CommTvtWork *sys, CtvtGamePlayer *player);
static void CtvtGamePlayer_ResultTask(TCB *tcb, void *data);

static MATHRandContext32 sCtvtGameRand;

// No code reads these. The game has five bytes here, b1 1e 0c 01 00, in the section that sPumpThresholds shares,
// which no declaration tried reproduces. They sort before the 3-byte table, so they would be objects of 1 or 2 bytes
// that some code refers to without reading them; this unreferenced placeholder gets a section of its own instead.
// Splitting the targets off into ctvt_game_target.c put their bounds table in place but not these bytes, and the
// objects of equal size are not in the game's order yet
const u8 data_ov257_021b1aa8[4] = { 0xb1, 0x1e, 0x0c, 0x01 };

// How many pumps a balloon takes to reach each stage after the first
static const u8 sPumpThresholds[] = { 10, 20, 30 };

static const G3DSceneAnimationSetup sResultPopAnimations[] = { { 1, 0 } };

// The width and height of the targets that a touch hits, by their depth
static const u8 sHitSizes[3][2] = { { 15, 23 }, { 18, 28 }, { 26, 39 } };

static const G3DSceneAnimationSetup sTargetAnimations3[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sTargetAnimations0[] = { { 1, 0 }, { 2, 0 } };

// Where each member's results go on the top screen, by their place in the call
static const u8 sResultPositions[4][2] = { { 32, 176 }, { 96, 176 }, { 160, 176 }, { 224, 176 } };
// Where each member's score goes on the sub screen during the game, by their rank
static const u8 sPlayerPositions[4][2] = { { 112, 25 }, { 120, 59 }, { 128, 93 }, { 136, 127 } };

static const G3DSceneAnimationSetup sTargetAnimations9[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sTargetAnimations6[] = { { 7, 0 }, { 8, 0 } };
static const G3DSceneAnimationSetup sBalloonPopAnimations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sResultAnimations[] = { { 2, 0 }, { 3, 0 } };

static const VecFx32 sCtvtGameCameraPosition = { 0, 0, FX32_CONST(250) };
static const VecFx32 sCtvtGameCameraTarget = { 0, 0, 0 };

static const G3DSceneAnimationSetup sFaceAnimations[] = { { 3, 0 }, { 4, 0 }, { 5, 0 } };

static const G3DSceneActorSetup sResultPopActors[] = {
    { 0, 0, 0, 0, sResultPopAnimations, NELEMS(sResultPopAnimations) },
};
static const G3DSceneActorSetup sBalloonPopActors[] = {
    { 0, 0, 0, 0, sBalloonPopAnimations, NELEMS(sBalloonPopAnimations) },
};

static const G3DSceneResourceSetup sResultPopResources[] = { { 233, 38, 0 }, { 233, 49, 0 } };

static const BGSetup sCtvtGameBG1Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_23,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG2Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x1000),
    GX_BG_CHARBASE(0x0c000),
    0x8000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG3Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x2800),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG5Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x1000),
    GX_BG_CHARBASE(0x0c000),
    0x8000,
    GX_BG_EXTPLTT_23,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG4Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x0000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_23,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG6Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x2000),
    GX_BG_CHARBASE(0x10000),
    0x8000,
    GX_BG_EXTPLTT_23,
    1,
    GX_BG_AREAOVER_XLU,
    FALSE,
};
static const BGSetup sCtvtGameBG7Setup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x3000),
    GX_BG_CHARBASE(0x14000),
    0x8000,
    GX_BG_EXTPLTT_23,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const G3DSceneActorSetup sResultActors[] = {
    { 0, 0, 0, 0, sResultAnimations, NELEMS(sResultAnimations) },
    { 1, 0, 1, 0, sResultAnimations, NELEMS(sResultAnimations) },
};

// Where the pump's handle is touched: the first four while it is up, the last four while it is down. The first of
// each, its knob, makes a big pump
static const TouchRect sPumpRects[] = {
    { 168, 172, 16, 20 },   { 142, 192, 0, 47 },    { 129, 141, 0, 37 },
    { 155, 192, 48, 64 },   { 168, 172, 232, 236 }, { 142, 192, 208, 255 },
    { 155, 192, 192, 207 }, { 129, 144, 217, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const G3DSceneResourceSetup sBalloonResources0[] = { { 233, 34, 0 }, { 233, 34, 0 }, { 233, 34, 0 } };
static const G3DSceneResourceSetup sBalloonResources1[] = { { 233, 35, 0 }, { 233, 35, 0 }, { 233, 35, 0 } };
static const G3DSceneResourceSetup sBalloonResources2[] = { { 233, 36, 0 }, { 233, 36, 0 }, { 233, 36, 0 } };
static const G3DSceneResourceSetup sBalloonResources3[] = { { 233, 37, 0 }, { 233, 37, 0 }, { 233, 37, 0 } };
static const G3DSceneResourceSetup sBalloonPopResources[] = { { 233, 43, 0 }, { 233, 54, 0 }, { 233, 63, 0 } };
static const G3DSceneResourceSetup sResultResources0[] = {
    { 233, 30, 0 }, { 233, 30, 0 }, { 233, 44, 0 }, { 233, 55, 0 }
};
static const G3DSceneResourceSetup sResultResources1[] = {
    { 233, 31, 0 }, { 233, 31, 0 }, { 233, 45, 0 }, { 233, 56, 0 }
};

static const G3DSceneActorSetup sFaceActors[] = {
    { 0, 0, 0, 0, sFaceAnimations, NELEMS(sFaceAnimations) },
    { 1, 0, 1, 0, sFaceAnimations, NELEMS(sFaceAnimations) },
    { 2, 0, 2, 0, sFaceAnimations, NELEMS(sFaceAnimations) },
};

static const G3DSceneResourceSetup sResultResources2[] = {
    { 233, 32, 0 }, { 233, 32, 0 }, { 233, 46, 0 }, { 233, 57, 0 }
};

static const BGSysVRAMConfig sCtvtGameVRAMConfig = {
    GX_VRAM_BG_128_A, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_B,  GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const G3DSceneResourceSetup sResultResources3[] = {
    { 233, 33, 0 }, { 233, 33, 0 }, { 233, 47, 0 }, { 233, 58, 0 }
};

static const G3DSceneActorSetup sBalloonActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, NULL, 0 },
    { 2, 0, 2, 0, NULL, 0 },
};

static const G3DSceneActorSetup sTargetActors[] = {
    { 0, 0, 0, 0, sTargetAnimations0, NELEMS(sTargetAnimations0) },
    { 3, 0, 3, 0, sTargetAnimations3, NELEMS(sTargetAnimations3) },
    { 6, 0, 6, 0, sTargetAnimations6, NELEMS(sTargetAnimations6) },
    { 9, 0, 9, 0, sTargetAnimations9, NELEMS(sTargetAnimations9) },
};

static const G3DSceneResourceSetup sFaceResources0[] = { { 233, 30, 0 }, { 233, 30, 0 }, { 233, 30, 0 },
                                                         { 233, 44, 0 }, { 233, 55, 0 }, { 233, 48, 0 } };
static const G3DSceneResourceSetup sFaceResources1[] = { { 233, 31, 0 }, { 233, 31, 0 }, { 233, 31, 0 },
                                                         { 233, 45, 0 }, { 233, 56, 0 }, { 233, 48, 0 } };
static const G3DSceneResourceSetup sFaceResources2[] = { { 233, 32, 0 }, { 233, 32, 0 }, { 233, 32, 0 },
                                                         { 233, 46, 0 }, { 233, 57, 0 }, { 233, 48, 0 } };
static const G3DSceneResourceSetup sFaceResources3[] = { { 233, 33, 0 }, { 233, 33, 0 }, { 233, 33, 0 },
                                                         { 233, 47, 0 }, { 233, 58, 0 }, { 233, 48, 0 } };

static const G3DSceneResourceSetup sTargetResources[] = {
    { 233, 41, 0 }, { 233, 52, 0 }, { 233, 61, 0 }, { 233, 42, 0 }, { 233, 53, 0 }, { 233, 62, 0 },
    { 233, 39, 0 }, { 233, 50, 0 }, { 233, 59, 0 }, { 233, 40, 0 }, { 233, 51, 0 }, { 233, 60, 0 },
};

static const G3DSceneSetup sResultScenes[] = {
    { sResultResources0, NELEMS(sResultResources0), sResultActors, NELEMS(sResultActors) },
    { sResultResources1, NELEMS(sResultResources1), sResultActors, NELEMS(sResultActors) },
    { sResultResources2, NELEMS(sResultResources2), sResultActors, NELEMS(sResultActors) },
    { sResultResources3, NELEMS(sResultResources3), sResultActors, NELEMS(sResultActors) },
    { sResultPopResources, NELEMS(sResultPopResources), sResultPopActors, NELEMS(sResultPopActors) },
};

static const G3DSceneSetup sTargetScenes[] = {
    { sFaceResources0, NELEMS(sFaceResources0), sFaceActors, NELEMS(sFaceActors) },
    { sFaceResources1, NELEMS(sFaceResources1), sFaceActors, NELEMS(sFaceActors) },
    { sFaceResources2, NELEMS(sFaceResources2), sFaceActors, NELEMS(sFaceActors) },
    { sFaceResources3, NELEMS(sFaceResources3), sFaceActors, NELEMS(sFaceActors) },
    { sTargetResources, NELEMS(sTargetResources), sTargetActors, NELEMS(sTargetActors) },
};

static const G3DSceneSetup sBalloonScenes[] = {
    { sBalloonResources0, NELEMS(sBalloonResources0), sBalloonActors, NELEMS(sBalloonActors) },
    { sBalloonResources1, NELEMS(sBalloonResources1), sBalloonActors, NELEMS(sBalloonActors) },
    { sBalloonResources2, NELEMS(sBalloonResources2), sBalloonActors, NELEMS(sBalloonActors) },
    { sBalloonResources3, NELEMS(sBalloonResources3), sBalloonActors, NELEMS(sBalloonActors) },
    { sBalloonPopResources, NELEMS(sBalloonPopResources), sBalloonPopActors, NELEMS(sBalloonPopActors) },
};

// The orders that the members' faces come up in, every permutation of the four
static const u8 sFaceOrders[24][4] = {
    { 0, 1, 2, 3 }, { 0, 1, 3, 2 }, { 0, 2, 1, 3 }, { 0, 2, 3, 1 }, { 0, 3, 1, 2 }, { 0, 3, 2, 1 },
    { 1, 0, 2, 3 }, { 1, 0, 3, 2 }, { 1, 2, 0, 3 }, { 1, 2, 3, 0 }, { 1, 3, 0, 2 }, { 1, 3, 2, 0 },
    { 2, 0, 1, 3 }, { 2, 0, 3, 1 }, { 2, 1, 0, 3 }, { 2, 1, 3, 0 }, { 2, 3, 0, 1 }, { 2, 3, 1, 0 },
    { 3, 0, 1, 2 }, { 3, 0, 2, 1 }, { 3, 1, 0, 2 }, { 3, 1, 2, 0 }, { 3, 2, 0, 1 }, { 3, 2, 1, 0 },
};

// How high the results' puffs rise, by rank
static fx32 sShotTopYs[] = { FX32_CONST(60), FX32_CONST(30), 0, FX32_CONST(-30) };

CtvtGame *CtvtGame_Create(CommTvtWork *sys, HeapID heapId) {
    CtvtGame *game = GFL_HeapAllocate(heapId, sizeof(CtvtGame), TRUE, "ctvt_game.c", 697);

    game->heapId = heapId;
    game->records = GameData_GetRecords(CommTvt_GetParam(sys)->gameData);
    return game;
}

void CtvtGame_Delete(CommTvtWork *sys, CtvtGame *game) {
    GFL_HeapFree(game);
}

void CtvtGame_Enter(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;
    u8 selfNetId = func_02042a6c(func_02040440());
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 memberCount = func_02042a78();
    u8 slot = 0;
    u8 netId;
    u8 i;
    u8 j;
    u32 fileId;
    u32 files[4] = { 9, 12, 15, 18 };

    for (i = 0; i < 3; i++) {
        sys_memset(&game->members[i], 0, sizeof(CtvtGameMember));
    }
    for (netId = 0; netId < 4; netId++) {
        if (selfNetId == netId) {
            continue;
        }
        game->members[slot].hasCamera = CtvtComm_HasMemberCamera(sys, comm, netId);
        game->members[slot].canExchangePhotos = CtvtComm_CanMemberExchangePhotos(sys, comm, netId);
        game->members[slot].netId = netId;
        if (game->members[slot].hasCamera == TRUE && !canPlayerExchangePhotos()) {
            for (i = 0; i < 3; i++) {
                game->members[slot].pictures[i] = allocConfigDSSoftwareFeature(heapId, 0x2000, "ctvt_game.c", 773);
            }
        } else {
            fileId = files[CtvtComm_GetUnk3dc(sys, comm, netId)];
            for (i = 0; i < 3; i++) {
                game->members[slot].pictures[i] = GFL_ArcSysReadHeapNewLZ(242, i + fileId, FALSE, heapId);
            }
        }
        slot++;
    }
    {
        BOOL cameraEnabled = CommTvt_IsCameraEnabled();
        CameraSystem *cameraSystem = CtvtCamera_GetCameraSystem(sys, CommTvt_GetCamera(sys));
        u8 unk3dc = CtvtComm_GetUnk3dc(sys, comm, selfNetId);

        game->cam =
            CtvtGameCam_Create(heapId, cameraEnabled, CtvtGame_GetType(game), unk3dc, game->members, cameraSystem);
    }
    game->state = CTVT_GAME_STATE_START_CAMERA;
    game->replay = FALSE;
    func_ov257_021aae44(sys);
    func_02042ba8(TRUE, heapId);
}

void CtvtGame_Leave(CommTvtWork *sys, CtvtGame *game) {
    u8 i;
    u8 j;

    func_02021c44(CommTvt_GetPrintQueue(sys));
    CtvtGameCam_Delete(game->cam);
    for (i = 0; i < 3; i++) {
        if (game->members[i].hasCamera == TRUE && !canPlayerExchangePhotos()) {
            for (j = 0; j < 3; j++) {
                func_02042ed0(game->members[i].pictures[j]);
            }
        } else {
            for (j = 0; j < 3; j++) {
                GFL_HeapFree(game->members[i].pictures[j]);
            }
        }
    }
    if (CommTvt_GetNextMode(sys) == COMM_TVT_MODE_EXIT_ERROR) {
        CtvtGame_Abort(sys, game);
    }
}

int CtvtGame_Main(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;

    switch (game->state) {
    case CTVT_GAME_STATE_FADE_OUT_EXIT:
        CommTvt_GetCamera(sys);
        GFL_WipeSet(0, 0, 0, 0, 6, 1, heapId);
        game->state = CTVT_GAME_STATE_EXIT;
        break;
    case CTVT_GAME_STATE_EXIT:
        if (GFL_WipeIsFinished() == TRUE) {
            return COMM_TVT_MODE_EXIT;
        }
        break;
    case CTVT_GAME_STATE_START_CAMERA:
        game->camTask = CtvtGameCam_StartTask(game->cam, heapId);
        game->state = CTVT_GAME_STATE_CAMERA;
        GFL_WipeSet(0, 1, 1, 0, 6, 1, heapId);
        break;
    case CTVT_GAME_STATE_CAMERA:
        if (GFL_WipeIsFinished() == TRUE) {
            if (!CtvtGame_CheckError(sys, game)) {
                switch (CtvtGameCam_UpdateTask(game->camTask)) {
                case 0:
                    break;
                case 1:
                    game->state = CTVT_GAME_STATE_FADE_OUT_CAMERA;
                    break;
                case 2:
                    return COMM_TVT_MODE_EXIT_ERROR;
                }
            } else {
                return COMM_TVT_MODE_EXIT_ERROR;
            }
        }
        break;
    case CTVT_GAME_STATE_FADE_OUT_CAMERA:
        game->state = CTVT_GAME_STATE_FREE_CAMERA;
        GFL_WipeSet(0, 0, 0, 0, 6, 1, heapId);
        break;
    case CTVT_GAME_STATE_FREE_CAMERA:
        CtvtGameCam_EndTask(game->camTask);
        game->state = CTVT_GAME_STATE_INIT_PLAY;
        break;
    case CTVT_GAME_STATE_INIT_PLAY:
        if (GFL_WipeIsFinished() == TRUE) {
            CtvtGame_InitPlay(sys, game);
            game->state = CTVT_GAME_STATE_PLAY;
        }
        break;
    case CTVT_GAME_STATE_PLAY:
        if (!CtvtGame_CheckError(sys, game)) {
            if (CtvtGame_UpdatePlay(sys, game) == TRUE) {
                game->state = CTVT_GAME_STATE_FREE_PLAY;
            }
        } else {
            return COMM_TVT_MODE_EXIT_ERROR;
        }
        break;
    case CTVT_GAME_STATE_FREE_PLAY:
        CtvtGame_FreePlay(sys, game);
        if (game->replay == TRUE) {
            game->state = CTVT_GAME_STATE_INIT_PLAY;
            break;
        }
        return COMM_TVT_MODE_TALK;
    }
    if (game->messagePending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!func_02021c1c(queue, BmpWin_GetBitmap(game->messageWindow))) {
            BmpWin_FlushChar(game->messageWindow);
            BmpWin_FlushMap(game->messageWindow);
            GFL_BGSysLoadScr(2);
            game->messagePending = FALSE;
        }
    }
    if (game->titlePending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!func_02021c1c(queue, BmpWin_GetBitmap(game->titleWindow))) {
            BmpWin_FlushChar(game->titleWindow);
            BmpWin_FlushMap(game->titleWindow);
            GFL_BGSysLoadScr(3);
            game->titlePending = FALSE;
        }
    }
    if (game->infoPending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!func_02021c1c(queue, BmpWin_GetBitmap(game->infoWindow))) {
            BmpWin_FlushChar(game->infoWindow);
            BmpWin_FlushMap(game->infoWindow);
            GFL_BGSysLoadScr(3);
            game->infoPending = FALSE;
        }
    }
    func_0204b794();
    func_02021a3c(CommTvt_GetPrintQueue(sys));
    return COMM_TVT_MODE_GAME;
}

void CtvtGame_SetType(CtvtGame *game, int type) {
    game->type = type;
}

int CtvtGame_GetType(CtvtGame *game) {
    return game->type;
}

G3DManager *CtvtGame_GetG3DManager(CtvtGame *game) {
    return game->g3d;
}

BOOL CtvtGame_IsPlaying(CtvtGame *game) {
    return game->playing;
}

void CtvtGame_SetChildQuit(CtvtGame *game, BOOL value) {
    game->childQuit = value;
}

void CtvtGame_SetHostQuit(CtvtGame *game, BOOL value) {
    game->hostQuit = value;
}

void CtvtGame_SetAllJoined(CtvtGame *game, BOOL value) {
    game->allJoined = value;
}

void CtvtGame_SetReplayStarted(CtvtGame *game, BOOL value) {
    game->replayStarted = value;
}

void CtvtGame_SetJoined(CtvtGame *game, u8 netId) {
    game->joinedMask |= 1 << netId;
}

void CtvtGame_SetReady(CtvtGame *game, u8 netId) {
    game->readyMask |= 1 << netId;
}

static BOOL CtvtGame_AreAllJoined(CommTvtWork *sys, CtvtGame *game) {
    u8 memberCount = CommTvt_GetMemberCount(sys);
    u8 i;
    u8 count = 0;

    for (i = 1; i < 4; i++) {
        if (game->joinedMask & (1 << i)) {
            count++;
        }
    }
    if (memberCount == (u8)(count + 1)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CtvtGame_AreAllReady(CommTvtWork *sys, CtvtGame *game) {
    u8 memberCount = CommTvt_GetMemberCount(sys);
    u8 i;
    u8 count = 0;

    for (i = 1; i < 4; i++) {
        if (game->readyMask & (1 << i)) {
            count++;
        }
    }
    if (memberCount == (u8)(count + 1)) {
        return TRUE;
    }
    return FALSE;
}

void CtvtGame_LoadBalloonPictures(CtvtGame *game, u8 netId, u8 color) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 slot = 0;
    u8 i;
    void *src;
    u32 dest;

    for (i = 0; i < color; i++) {
        if (selfNetId != i) {
            slot++;
        }
    }
    for (i = 0; i < 3; i++) {
        u16 count = GFL_G3DMgrGetSceneResCount(game->g3d, game->scenes[netId]);
        void *resource = GFL_G3DMgrGetResource(game->g3d, i + count * netId);

        dest = NNS_GfdGetTexKeyAddr(GFL_G3DResGetTexVRAMHandle(resource)) + 0x2000 / 32;
        if (selfNetId == color) {
            src = game->cam->pictures[i];
        } else {
            src = game->members[slot].pictures[i];
        }
        NNS_GfdRegisterNewVramTransferTask(0, dest, src, 0x2000);
    }
}

BOOL CtvtGame_SpawnTarget(CtvtGame *game, u8 index) {
    u8 i;
    u8 rand;
    u8 scene;

    if (game->targets[index].tcb == NULL) {
        game->targets[index].tcb = GFL_TCBMgrAddTask(game->tcbManager, CtvtGame_TargetTask, &game->targets[index], 0);
        if (game->targets[index].tcb != NULL) {
            scene = CtvtGame_NextFace(game);
            for (i = 0; i < 10; i++) {
                rand = CtvtGame_Rand(4);
                if (rand != game->lastColumn) {
                    break;
                }
            }
            game->lastColumn = rand;
            for (i = 0; i < 10; i++) {
                rand = CtvtGame_Rand(3);
                if (rand != game->lastAngle) {
                    break;
                }
            }
            game->lastAngle = rand;
            game->nextDepth++;
            game->nextDepth %= 3;
            CtvtGame_LaunchTarget(game, &game->targets[index], game->scenes[scene]);
            game->targets[index].g3d = game->g3d;
            game->targets[index].actor = 0;
            for (i = 0; i < 8; i++) {
                if (i != index && game->targets[index].depth == game->targets[i].depth &&
                    game->targets[i].active == TRUE) {
                    CtvtGame_Separate(&game->targets[index].srt.translation.x, &game->targets[index].srt.translation.y,
                                      &game->targets[i].srt.translation.x, &game->targets[i].srt.translation.y,
                                      game->targets[index].srt.scale.x * 0x8c, TRUE);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

void CtvtGame_SetSeed(CtvtGame *game, u32 seed) {
    game->seed = seed;
}

void CtvtGame_SetFrame(CtvtGame *game, u16 frame) {
    game->frame = frame;
    game->frameReceived = TRUE;
}

u16 CtvtGame_GetFrame(CtvtGame *game) {
    return game->frame;
}

u16 CtvtGame_GetHostFrame(CtvtGame *game) {
    return game->hostFrame;
}

void CtvtGame_CheckTouch(CtvtComm *comm, CtvtGame *game, const CtvtGamePacket *packet, int netId) {
    u16 index;

    if (CtvtGame_FindTouchedTarget(game, packet->frame, packet->unk5[0], packet->unk5[1], &index) == TRUE) {
        CtvtGamePacket reply = { 0 };

        reply.values[netId] = index;
        reply.mask = 1 << netId;
        reply.frame = game->frame;
        CtvtComm_QueueGamePacket(comm, reply, 4);
        game->targets[index].hitPending = TRUE;
    }
}

void CtvtGame_ApplyHits(CtvtGame *game, const CtvtGamePacket *packet, u8 mask) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 i;
    u8 index;

    for (i = 0; i < 4; i++) {
        if (!(mask & (1 << i))) {
            continue;
        }
        index = packet->values[i];
        if (game->targets[index].tcb == NULL) {
            continue;
        }
        if (i == game->targets[index].scene) {
            if (game->timeLeft <= 20) {
                game->scores[i] += 4;
                game->targets[index].hitKind = 1;
            } else {
                game->scores[i] += 2;
                game->targets[index].hitKind = 0;
            }
            if (game->scores[i] > 999) {
                game->scores[i] = 999;
            }
            if (selfNetId == i) {
                GFL_SndSEPlay(SEQ_SE_LCG_05);
            }
        } else {
            if (game->timeLeft <= 20) {
                game->scores[i] -= 2;
                game->targets[index].hitKind = 3;
            } else {
                game->scores[i] -= 1;
                game->targets[index].hitKind = 2;
            }
            if (game->scores[i] < 0) {
                game->scores[i] = 0;
            }
            if (selfNetId == i) {
                GFL_SndSEPlay(SEQ_SE_LCG_02);
            }
        }
        CtvtGame_SetScore(game, i, game->scores[i]);
        if (i == game->targets[index].scene) {
            game->targets[index].actor = 1;
        } else {
            game->targets[index].actor = 2;
        }
        if (selfNetId == i) {
            game->targets[index].hitAnimating = TRUE;
        }
    }
}

void CtvtGame_SetScores(CtvtGame *game, const CtvtGameData *data) {
    u8 i;

    for (i = 0; i < 4; i++) {
        game->scores[i] = data->data[i];
    }
}

void CtvtGame_CountPump(CtvtComm *comm, CtvtGame *game, BOOL big, int netId) {
    BOOL popped = FALSE;
    u8 amount = 1;

    if (big == TRUE) {
        amount = 2;
    }
    if (game->timeLeft <= 20) {
        amount *= 2;
    }
    game->pumps[netId] += amount;
    if (game->stages[netId] < 3) {
        if (game->pumps[netId] >= sPumpThresholds[game->stages[netId]] &&
            !CtvtGameBalloon_IsBusy(game->balloons[netId])) {
            game->stages[netId]++;
            popped = TRUE;
        }
    } else if (game->pumps[netId] >= 30 && !CtvtGameBalloon_IsBusy(game->balloons[netId])) {
        game->pumps[netId] = 0;
        game->stages[netId] = 0;
        popped = TRUE;
    }
    if (popped == TRUE) {
        CtvtGamePacket packet = { 0 };

        packet.mask = 1 << netId;
        CtvtComm_QueueGamePacket(comm, packet, 7);
    }
}

void CtvtGame_PumpBalloons(CtvtGame *game, u8 mask) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 i;

    for (i = 0; i < 4; i++) {
        if (mask & (1 << i)) {
            BOOL popped = CtvtGameBalloon_Pump(game->balloons[i]);

            if (popped == TRUE) {
                game->scores[i] += 5;
                if (game->scores[i] > 999) {
                    game->scores[i] = 999;
                }
                CtvtGame_SetScore(game, i, game->scores[i]);
            }
            if (selfNetId == i) {
                if (popped == TRUE) {
                    GFL_SndSEPlay(SEQ_SE_LCG_01);
                } else {
                    GFL_SndSEPlay(SEQ_SE_LCG_09);
                }
            }
        }
    }
}

static void CtvtGame_InitPlay(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;
    ArcTool *arc;
    u8 i;

    CtvtGame_InitGraphics(game, heapId);
    arc = GFL_ArcSysCreateFileHandle(233, heapId);
    switch (CtvtGame_GetType(game)) {
    case CTVT_GAME_TYPE_TARGETS:
        CtvtGame_LoadTargetResources(game, heapId, arc);
        break;
    case CTVT_GAME_TYPE_BALLOONS:
        CtvtGame_LoadBalloonResources(game, heapId, arc);
        break;
    }
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadNCLRDefault(23, 5, 0, 0x1e0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(23, 5, 4, 0x1e0, 0x20, heapId);
    LoadSysMsgBox(2, 1, 14, 0, heapId);
    for (i = 0; i < 4; i++) {
        game->shots[i] = NULL;
    }
    for (i = 0; i < 3; i++) {
        game->introActors[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        game->targetSigns[i] = NULL;
    }
    game->scoreFrame = NULL;
    game->hurrySign = NULL;
    game->pump = NULL;
    game->pumpBase = NULL;
    game->pumpAir = NULL;
    game->startSign = NULL;
    for (i = 0; i < 3; i++) {
        game->rightPuffs[i] = NULL;
        game->leftPuffs[i] = NULL;
    }
    game->resultFrame = NULL;
    game->crown = NULL;
    for (i = 0; i < 6; i++) {
        game->crownSparkles[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        game->winBursts[i] = NULL;
        game->winFlashes[i] = NULL;
    }
    game->winBanner = NULL;
    game->winGlow = NULL;
    game->rankSign = NULL;
    game->tryAgainSign = NULL;
    GFL_BGSysSetBGPriority(0, 2);
    func_02042ba8(TRUE, heapId);
}

static void CtvtGame_FreePlay(CommTvtWork *sys, CtvtGame *game) {
    switch (CtvtGame_GetType(game)) {
    case CTVT_GAME_TYPE_TARGETS:
        CtvtGame_FreeTargetResources(game);
        break;
    case CTVT_GAME_TYPE_BALLOONS:
        CtvtGame_FreeBalloonResources(game);
        break;
    }
    CtvtGame_FreeGraphics(game);
}

static void CtvtGame_Abort(CommTvtWork *sys, CtvtGame *game) {
    int type;

    switch (game->state) {
    case CTVT_GAME_STATE_CAMERA:
    case CTVT_GAME_STATE_FADE_OUT_CAMERA:
        CtvtGameCam_EndTask(game->camTask);
        break;
    case CTVT_GAME_STATE_PLAY:
        type = CtvtGame_GetType(game);
        CtvtGame_FreeCommon(sys, game);
        switch (type) {
        case CTVT_GAME_TYPE_TARGETS:
            CtvtGame_FreeTargets(sys, game);
            break;
        case CTVT_GAME_TYPE_BALLOONS:
            CtvtGame_FreeBalloons(sys, game);
            break;
        }
        CtvtGame_FreePlay(sys, game);
        break;
    }
}

static inline void CtvtGame_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

static BOOL CtvtGame_UpdatePlay(CommTvtWork *sys, CtvtGame *game) {
    CtvtComm *comm;
    u8 selfNetId;
    int type;
    u8 i;

    CommTvt_GetCamera(sys);
    comm = CommTvt_GetComm(sys);
    selfNetId = func_02042a6c(func_02040440());
    switch (game->playState) {
    case CTVT_GAME_PLAY_SEND_SEED:
        if (selfNetId == 0) {
            CtvtGamePacket packet = { 0 };
            RTCDate date;
            RTCTime time;

            RTC_GetCachedDateTime(&date, &time);
            packet.value = date.year + date.month * 0x100 + date.day * 0x10000 + time.hour * 0x100000 +
                           (time.minute + time.second) * 0x1000000;
            CtvtComm_QueueGamePacket(comm, packet, 1);
        }
        game->playState = CTVT_GAME_PLAY_SYNC_SEED;
        break;
    case CTVT_GAME_PLAY_SYNC_SEED:
        func_02040624(func_02040440(), 22, 32);
        game->playState = CTVT_GAME_PLAY_WAIT_SEED;
        break;
    case CTVT_GAME_PLAY_WAIT_SEED:
        if (func_02040664(func_02040440(), 22, 32) == TRUE) {
            game->playState = CTVT_GAME_PLAY_INIT;
        }
        break;
    case CTVT_GAME_PLAY_INIT:
        type = CtvtGame_GetType(game);
        CtvtGame_InitCommon(sys, game);
        switch (type) {
        case CTVT_GAME_TYPE_TARGETS:
            CtvtGame_InitTargets(sys, game);
            break;
        case CTVT_GAME_TYPE_BALLOONS:
            CtvtGame_InitBalloons(sys, game);
            break;
        }
        GFL_SndBGMPlay(SEQ_ME_LCG_01, SND_CHANNEL_MASK_ALL);
        game->playState = CTVT_GAME_PLAY_FADE_IN;
        GFL_WipeSet(0, 1, 1, 0, 6, 1, game->heapId);
        break;
    case CTVT_GAME_PLAY_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            game->playState = CTVT_GAME_PLAY_INTRO;
            reg_G2S_DB_BLDCNT = 0;
        }
        break;
    case CTVT_GAME_PLAY_INTRO:
        if (CtvtGame_UpdateIntro(sys, game)) {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, game->heapId);
            func_02005d8c();
            game->playState = CTVT_GAME_PLAY_WAIT_INTRO_FADE_OUT;
        }
        break;
    case CTVT_GAME_PLAY_WAIT_INTRO_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            func_0204c124(game->scoreFrame, TRUE);
            for (i = 0; i < 4; i++) {
                CtvtGamePlayer_Show(sys, game->players[i]);
            }
            GFL_BitmapFill(BmpWin_GetBitmap(game->titleWindow), 0);
            CtvtGame_ClearWindow(game->titleWindow);
            BmpWin_ClearFrame(game->infoWindow, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(game->infoWindow), 0);
            CtvtGame_ClearWindow(game->infoWindow);
            GFL_BGSysLoadScr(2);
            GFL_BGSysSetBGEnabled(7, FALSE);
            CtvtGame_DrawTimeBar(game);
            switch (CtvtGame_GetType(game)) {
            case CTVT_GAME_TYPE_TARGETS:
                CtvtGame_StartTargets(sys, game);
                break;
            case CTVT_GAME_TYPE_BALLOONS:
                CtvtGame_StartBalloons(sys, game);
                break;
            }
            func_02040624(func_02040440(), 23, 32);
            game->playState = CTVT_GAME_PLAY_WAIT_SYNC_START;
        }
        break;
    case CTVT_GAME_PLAY_WAIT_SYNC_START:
        if (func_02040664(func_02040440(), 23, 32) == TRUE) {
            game->playState = CTVT_GAME_PLAY_WAIT_FADE_IN;
            GFL_SndBGMPlay(SEQ_BGM_LCG_01, SND_CHANNEL_MASK_ALL);
            GFL_BGSysMoveBG(4, 3, 0);
            GFL_WipeSet(0, 1, 1, 0, 6, 1, game->heapId);
        }
        break;
    case CTVT_GAME_PLAY_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 1, -8);
            game->playing = TRUE;
            switch (CtvtGame_GetType(game)) {
            case CTVT_GAME_TYPE_TARGETS:
                game->playState = CTVT_GAME_PLAY_TARGETS;
                GFL_SndSEPlay(SEQ_SE_LCG_03);
                break;
            case CTVT_GAME_TYPE_BALLOONS:
                game->playState = CTVT_GAME_PLAY_BALLOON_START;
                func_0204c124(game->startSign, TRUE);
                func_0204c520(game->startSign, TRUE);
                func_0204c56c(game->startSign);
                break;
            }
            RecordAddOne(game->records, 0x81);
            func_02038bc8(25);
        }
        break;
    case CTVT_GAME_PLAY_TARGETS:
        if (CtvtGame_UpdateTargets(sys, game)) {
            game->playing = FALSE;
            game->playState = CTVT_GAME_PLAY_TARGETS_END;
            func_02005d8c();
        }
        break;
    case CTVT_GAME_PLAY_TARGETS_END:
        game->frame++;
        CtvtGame_DrawTargets3D(game);
        if (CtvtGame_AreTargetsGone(game) == TRUE) {
            if (selfNetId == 0) {
                CtvtGameData data;

                for (i = 0; i < 4; i++) {
                    data.data[i] = game->scores[i];
                }
                CtvtComm_QueueGameData(comm, data);
            }
            game->playState = CTVT_GAME_PLAY_SYNC_END;
        }
        break;
    case CTVT_GAME_PLAY_BALLOON_START:
        if (CtvtGame_UpdateBalloonStart(sys, game)) {
            GFL_SndSEPlay(SEQ_SE_LCG_03);
            game->playState = CTVT_GAME_PLAY_BALLOONS;
        }
        break;
    case CTVT_GAME_PLAY_BALLOONS:
        if (CtvtGame_UpdateBalloons(sys, game)) {
            game->playing = FALSE;
            game->playState = CTVT_GAME_PLAY_SYNC_END;
            func_02005d8c();
        }
        break;
    case CTVT_GAME_PLAY_SYNC_END:
        func_02040624(func_02040440(), 24, 32);
        game->hostFrame = 0;
        game->playState = CTVT_GAME_PLAY_WAIT_SYNC_END;
        break;
    case CTVT_GAME_PLAY_WAIT_SYNC_END:
        if (func_02040664(func_02040440(), 24, 32) == TRUE) {
            game->playState = CTVT_GAME_PLAY_FADE_OUT;
        }
        break;
    case CTVT_GAME_PLAY_FADE_OUT:
        GFL_WipeSet(3, 0, 0, 0, 6, 1, game->heapId);
        game->playState = CTVT_GAME_PLAY_WAIT_FADE_OUT;
        break;
    case CTVT_GAME_PLAY_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            game->playState = CTVT_GAME_PLAY_INIT_RESULTS;
        }
        break;
    case CTVT_GAME_PLAY_INIT_RESULTS:
        CtvtGame_InitResults(sys, game);
        game->playState = CTVT_GAME_PLAY_FADE_IN_RESULTS;
        break;
    case CTVT_GAME_PLAY_FADE_IN_RESULTS:
        GFL_WipeSet(3, 1, 1, 0, 6, 1, game->heapId);
        game->playState = CTVT_GAME_PLAY_WAIT_FADE_IN_RESULTS;
        break;
    case CTVT_GAME_PLAY_WAIT_FADE_IN_RESULTS:
        if (GFL_WipeIsFinished() == TRUE) {
            GFL_SndBGMPlay(SEQ_ME_LCG_03, SND_CHANNEL_MASK_ALL);
            game->playState = CTVT_GAME_PLAY_RESULTS;
        }
        break;
    case CTVT_GAME_PLAY_RESULTS:
        if (CtvtGame_UpdateResults(sys, game) == TRUE) {
            RecordAdd(game->records, 0x32, game->players[func_02042a6c(func_02040440())]->score);
            game->playState = CTVT_GAME_PLAY_ASK_REPLAY;
        }
        break;
    case CTVT_GAME_PLAY_ASK_REPLAY:
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 27, -8);
        LoadSysMsgBox(2, 1, 14, 0, game->heapId);
        CtvtGame_PrintMessage(sys, game, 61);
        game->menuRes = AppTaskMenuRes_Create(2, 12, CommTvt_GetFont(sys), CommTvt_GetPrintQueue(sys), game->heapId);
        game->menu = CtvtGame_CreateReplayMenu(sys, game);
        game->replay = FALSE;
        game->exitWait = 0;
        game->menuFrame = 0;
        game->childQuit = FALSE;
        game->hostQuit = FALSE;
        game->allJoined = FALSE;
        game->replayStarted = FALSE;
        game->joinedMask = 0;
        game->readyMask = 0;
        game->playState = CTVT_GAME_PLAY_REPLAY_MENU;
        break;
    case CTVT_GAME_PLAY_FADE_OUT_MENU:
        GFL_WipeSet(0, 0, 0, 0, 6, 1, game->heapId);
        game->playState = CTVT_GAME_PLAY_WAIT_FADE_OUT_MENU;
        break;
    case CTVT_GAME_PLAY_WAIT_FADE_OUT_MENU:
        if (GFL_WipeIsFinished() == TRUE) {
            if (game->menu != NULL) {
                AppTaskMenu_Free(game->menu);
            }
            if (game->yesNoMenu != NULL) {
                AppTaskMenu_Free(game->yesNoMenu);
                game->yesNoMenu = NULL;
            }
            if (game->menuRes != NULL) {
                AppTaskMenuRes_Free(game->menuRes);
                game->menuRes = NULL;
            }
            game->messagePending = FALSE;
            BmpWin_ClearFrame(game->messageWindow, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(game->messageWindow), 0);
            CtvtGame_ClearWindow(game->messageWindow);
            GFL_BGSysLoadScr(2);
            game->playState = CTVT_GAME_PLAY_SYNC_FREE;
        }
        break;
    case CTVT_GAME_PLAY_REPLAY_MENU:
        if (!CtvtGame_CheckQuit(sys, game)) {
            game->menuFrame++;
            AppTaskMenu_Update(game->menu);
            if (AppTaskMenu_IsFlashFinished(game->menu) == TRUE) {
                if (AppTaskMenu_GetCursorPos(game->menu) == 0) {
                    CtvtGame_PrintMessage(sys, game, 45);
                    game->yesNoMenu = CtvtGame_CreateCancelMenu(sys, game);
                    if (selfNetId == 0) {
                        game->playState = CTVT_GAME_PLAY_HOST_WAIT_JOINED;
                    } else {
                        game->playState = CTVT_GAME_PLAY_CHILD_WAIT_JOINED;
                    }
                } else {
                    CtvtGame_PrintMessage(sys, game, 46);
                    game->exitWait = 150;
                    if (selfNetId == 0) {
                        game->playState = CTVT_GAME_PLAY_HOST_EXIT;
                    } else {
                        game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
                    }
                }
                AppTaskMenu_Free(game->menu);
                game->menu = NULL;
            } else if (game->menuFrame > 1800) {
                CtvtGame_PrintMessage(sys, game, 46);
                game->exitWait = 150;
                if (selfNetId == 0) {
                    game->playState = CTVT_GAME_PLAY_HOST_EXIT;
                } else {
                    game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
                }
                AppTaskMenu_Free(game->menu);
                game->menu = NULL;
            }
        } else if (selfNetId == 0) {
            game->playState = CTVT_GAME_PLAY_HOST_EXIT;
        } else {
            game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
        }
        break;
    case CTVT_GAME_PLAY_HOST_WAIT_JOINED:
        if (!CtvtGame_CheckQuit(sys, game)) {
            AppTaskMenu_Update(game->yesNoMenu);
            if (AppTaskMenu_IsFlashFinished(game->yesNoMenu) == TRUE) {
                if (AppTaskMenu_GetCursorPos(game->yesNoMenu) == 0) {
                    CtvtGame_PrintMessage(sys, game, 46);
                    game->exitWait = 150;
                    game->playState = CTVT_GAME_PLAY_HOST_EXIT;
                }
                AppTaskMenu_Free(game->yesNoMenu);
                game->yesNoMenu = NULL;
            } else if (!AppTaskMenu_IsDecided(game->yesNoMenu) && CtvtGame_AreAllJoined(sys, game)) {
                CtvtComm_QueueGameCommand(sys, CommTvt_GetComm(sys), 2);
                game->playState = CTVT_GAME_PLAY_HOST_WAIT_READY;
            }
        } else {
            game->playState = CTVT_GAME_PLAY_HOST_EXIT;
        }
        break;
    case CTVT_GAME_PLAY_HOST_WAIT_READY:
        if (!CtvtGame_CheckQuit(sys, game)) {
            if (CtvtGame_AreAllReady(sys, game)) {
                CtvtComm_QueueGameCommand(sys, CommTvt_GetComm(sys), 5);
                game->replay = TRUE;
                game->playState = CTVT_GAME_PLAY_FADE_OUT_MENU;
            }
        } else {
            game->playState = CTVT_GAME_PLAY_HOST_EXIT;
        }
        break;
    case CTVT_GAME_PLAY_HOST_EXIT:
        CommTvt_GetComm(sys);
        if (game->exitWait != 0) {
            game->exitWait--;
        }
        if (game->exitWait == 0) {
            CtvtComm_QueueGameCommand(sys, CommTvt_GetComm(sys), 3);
            game->playState = CTVT_GAME_PLAY_FADE_OUT_MENU;
            if (game->yesNoMenu != NULL) {
                AppTaskMenu_Free(game->yesNoMenu);
                game->yesNoMenu = NULL;
            }
        }
        break;
    case CTVT_GAME_PLAY_CHILD_WAIT_JOINED:
        if (!CtvtGame_CheckQuit(sys, game)) {
            AppTaskMenu_Update(game->yesNoMenu);
            if (AppTaskMenu_IsFlashFinished(game->yesNoMenu) == TRUE) {
                if (AppTaskMenu_GetCursorPos(game->yesNoMenu) == 0) {
                    CtvtGame_PrintMessage(sys, game, 46);
                    game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
                    game->exitWait = 150;
                }
                AppTaskMenu_Free(game->yesNoMenu);
                game->yesNoMenu = NULL;
            } else if (!AppTaskMenu_IsDecided(game->yesNoMenu)) {
                CtvtComm_QueueGameCommand(sys, comm, 0);
                if (game->allJoined == TRUE) {
                    game->allJoined = FALSE;
                    game->playState = CTVT_GAME_PLAY_CHILD_READY;
                }
            }
        } else {
            game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
        }
        break;
    case CTVT_GAME_PLAY_CHILD_READY:
        if (!CtvtGame_CheckQuit(sys, game)) {
            CtvtComm_QueueGameCommand(sys, comm, 4);
            if (game->replayStarted == TRUE) {
                game->replay = TRUE;
                game->replayStarted = FALSE;
                game->playState = CTVT_GAME_PLAY_FADE_OUT_MENU;
            }
        } else {
            game->playState = CTVT_GAME_PLAY_CHILD_EXIT;
        }
        break;
    case CTVT_GAME_PLAY_CHILD_EXIT:
        if (game->exitWait != 0) {
            game->exitWait--;
        }
        if (game->exitWait == 0) {
            if (!CtvtGame_CheckQuit(sys, game)) {
                CtvtComm_QueueGameCommand(sys, CommTvt_GetComm(sys), 1);
            } else {
                game->playState = CTVT_GAME_PLAY_FADE_OUT_MENU;
            }
        }
        break;
    case CTVT_GAME_PLAY_SYNC_FREE:
        func_02040624(func_02040440(), 25, 32);
        game->playState = CTVT_GAME_PLAY_WAIT_SYNC_FREE;
        break;
    case CTVT_GAME_PLAY_WAIT_SYNC_FREE:
        if (func_02040664(func_02040440(), 25, 32) == TRUE) {
            game->playState = CTVT_GAME_PLAY_FREE;
        }
        break;
    case CTVT_GAME_PLAY_FREE:
        type = CtvtGame_GetType(game);
        CtvtGame_FreeCommon(sys, game);
        switch (type) {
        case CTVT_GAME_TYPE_TARGETS:
            CtvtGame_FreeTargets(sys, game);
            break;
        case CTVT_GAME_TYPE_BALLOONS:
            CtvtGame_FreeBalloons(sys, game);
            break;
        }
        game->playState = CTVT_GAME_PLAY_SEND_SEED;
        return TRUE;
    }
    if (game->playState >= CTVT_GAME_PLAY_RESULTS && game->playState <= CTVT_GAME_PLAY_WAIT_SYNC_FREE) {
        CtvtGame_DrawResults3D(game);
    }
    return FALSE;
}

static void CtvtGame_InitCommon(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 memberCount = CommTvt_GetMemberCount(sys);
    u8 pos = 0;
    u8 i;
    ClActorSetup setup;

    MATH_InitRand32(&sCtvtGameRand, game->seed);
    setup.x = 128;
    setup.y = 168;
    setup.sequence = 0;
    setup.bgPriority = 1;
    game->scoreFrame = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4], &setup,
                                     CLACT_SURFACE_SUB, heapId);
    func_0204c124(game->scoreFrame, FALSE);
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 2;
    setup.bgPriority = 1;
    game->hurrySign = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4], &setup,
                                    CLACT_SURFACE_SUB, heapId);
    func_0204c124(game->hurrySign, FALSE);
    game->playerTcbBuffer = GFL_HeapAllocate(game->heapId, GFL_TCBMgrCalcAllocSize(4), FALSE, "ctvt_game.c", 2733);
    game->playerTcbManager = GFL_TCBMgrCreate(4, game->playerTcbBuffer);
    for (i = 0; i < 4; i++) {
        if (func_02042a80(i) == TRUE) {
            game->players[i] = CtvtGamePlayer_Create(sys, game, i, pos);
            pos++;
        } else {
            game->players[i] = CtvtGamePlayer_CreateAbsent(sys, game, i);
        }
        game->players[i]->tcb = GFL_TCBMgrAddTask(game->playerTcbManager, CtvtGamePlayer_Task, game->players[i], 1);
    }
    game->sorting = FALSE;
    game->timeWindow = BmpWin_CreateDynamic(6, 7, 20, 23, 3, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(game->timeWindow), 0);
    game->titleWindow = BmpWin_CreateDynamic(3, 8, 0, 16, 4, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(game->titleWindow), 0);
    game->infoWindow = BmpWin_CreateDynamic(2, 1, 18, 30, 6, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(game->infoWindow), 0);
    game->messageWindow = BmpWin_CreateDynamic(2, 1, 8, 30, 6, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(game->messageWindow), 0);
    for (i = 0; i < 4; i++) {
        game->scores[i] = 0;
    }
    game->lastColumn = 5;
    game->lastAngle = 4;
    game->nextDepth = 0;
    game->spawnNow = FALSE;
    game->spawnWait = 0;
    game->faceOrder = 0;
    game->faceOrderPos = 0;
    game->timeLeft = 60;
    game->lastTick = 0;
    game->seed = 0;
    game->frame = 0;
    game->hostFrame = 0;
    game->playing = FALSE;
    game->frameReceived = FALSE;
    game->introFrame = 0;
}

static void CtvtGame_InitTargets(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 memberCount = CommTvt_GetMemberCount(sys);
    u8 i;
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    StrBuf *str;
    StrBuf *format;
    StrBuf *name;
    WordSet *wordSet;
    u8 width;
    NNSSndHeapHandle sndHeap;
    ClActorSetup setup;
    ClActorSetupEx setupEx;

    for (i = 0; i < 4; i++) {
        CtvtGame_LoadBalloonPictures(game, i, i);
    }
    GFL_BGSysLoadNCLRDefault(233, 3, 0, 0, 0x60, heapId);
    GFL_BGSysLoadNCGRStatic(233, 12, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 19, 1, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCLRDefault(233, 7, 4, 0, 0xa0, heapId);
    GFL_BGSysLoadNCGRStatic(233, 16, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 23, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCGRStatic(233, 17, 7, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 24, 7, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(7, 0, 0, 32, 24, 3);
    GFL_BGSysLoadScr(7);
    GFL_BGSysLoadNCLRDefault(233, 2, 4, 0x1a0, 0x20, heapId);
    setup.x = 128;
    setup.y = 96;
    setup.priority = 0;
    setup.bgPriority = 1;
    for (i = 0; i < 3; i++) {
        setup.sequence = i + 4;
        setup.priority = i;
        game->introActors[i] = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4],
                                             &setup, CLACT_SURFACE_SUB, heapId);
        func_0204c124(game->introActors[i], TRUE);
        func_0204c520(game->introActors[i], TRUE);
    }
    sys_memset(&setupEx, 0, sizeof(ClActorSetupEx));
    setupEx.base.x = 60;
    setupEx.base.y = 50;
    setupEx.base.sequence = 13;
    setupEx.base.priority = 0;
    setupEx.base.bgPriority = 3;
    setupEx.affineCenter.x = 0;
    setupEx.affineCenter.y = 0;
    setupEx.scaleX = FX32_ONE;
    setupEx.scaleY = FX32_ONE;
    setupEx.rotation = 0;
    setupEx.affineMode = 2;
    game->targetSigns[0] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                         &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->targetSigns[0], FALSE);
    func_0204c520(game->targetSigns[0], TRUE);
    setupEx.base.x = 200;
    setupEx.base.y = 90;
    setupEx.base.sequence = 14;
    setupEx.base.bgPriority = 3;
    game->targetSigns[1] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                         &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->targetSigns[1], FALSE);
    func_0204c520(game->targetSigns[1], TRUE);
    game->tcbBuffer = GFL_HeapAllocate(game->heapId, GFL_TCBMgrCalcAllocSize(8), FALSE, "ctvt_game.c", 2945);
    game->tcbManager = GFL_TCBMgrCreate(8, game->tcbBuffer);
    for (i = 0; i < 8; i++) {
        CtvtGameTarget_Init(&game->targets[i]);
    }
    font = CommTvt_GetFont(sys);
    msgData = CommTvt_GetMsgData(sys);
    queue = CommTvt_GetPrintQueue(sys);
    str = GFL_MsgDataLoadStrbufNew(msgData, 64);
    width = GFL_FontGetBlockWidth(str, font, 0);
    func_02021c7c(queue, BmpWin_GetBitmap(game->titleWindow), 64 - width / 2, 12, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_FlushChar(game->titleWindow);
    BmpWin_FlushMap(game->titleWindow);
    game->titlePending = TRUE;
    wordSet = GFL_WordSetSystemCreateDefault(game->heapId);
    str = GFL_StrBufCreate(128, game->heapId);
    GFL_BitmapFill(BmpWin_GetBitmap(game->infoWindow), 15);
    format = GFL_MsgDataLoadStrbufNew(msgData, 59);
    name = GFL_MsgDataLoadStrbufNew(msgData, 67 + selfNetId);
    func_0202437c(wordSet, 0, name, 0, 1, 2);
    GFL_StrBufFree(name);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    GFL_StrBufFree(format);
    func_02021c7c(queue, BmpWin_GetBitmap(game->infoWindow), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(game->infoWindow);
    BmpWin_FlushMap(game->infoWindow);
    BmpWin_DrawFrame(game->infoWindow, 1, 1, 14);
    game->infoPending = TRUE;
    sndHeap = func_02005ce4();
    NNS_SndHeapSaveState(sndHeap);
    NNS_SndArcLoadGroup(1, sndHeap);
}

static void CtvtGame_InitBalloons(CommTvtWork *sys, CtvtGame *game) {
    u8 selfNetId;
    HeapID heapId = game->heapId;
    u8 memberCount;

    selfNetId = func_02042a6c(func_02040440());
    memberCount = CommTvt_GetMemberCount(sys);
    u8 i;
    u8 pos;
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    StrBuf *str;
    u8 width;
    NNSSndHeapHandle sndHeap;
    ClActorSetup setup;

    for (i = 0; i < 4; i++) {
        CtvtGame_LoadBalloonPictures(game, i, 0);
    }
    GFL_BGSysLoadNCLRDefault(233, 4, 0, 0, 0xc0, heapId);
    GFL_BGSysLoadNCGRStatic(233, 13, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 20, 1, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCLRDefault(233, 7, 4, 0, 0xa0, heapId);
    GFL_BGSysLoadNCGRStatic(233, 16, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 23, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadNCGRStatic(233, 17, 7, 0, 0, FALSE, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 24, 7, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(7, 0, 0, 32, 24, 4);
    GFL_BGSysLoadScr(7);
    game->paletteFrame = 0;
    game->paletteFile = GFL_G2DIOReadNCLR(233, 7, &game->palette, game->heapId);
    GFL_BGSysUploadStdPalette(4, (u8 *)game->palette->rawData + 0x20, 0x20, 0);
    GFL_BGSysLoadNCLRDefault(233, 2, 4, 0x1a0, 0x20, heapId);
    setup.x = 128;
    setup.y = 96;
    setup.priority = 4;
    setup.bgPriority = 1;
    for (i = 0; i < 3; i++) {
        setup.sequence = i + 7;
        setup.priority = i;
        game->introActors[i] = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4],
                                             &setup, CLACT_SURFACE_SUB, heapId);
        func_0204c124(game->introActors[i], TRUE);
        func_0204c520(game->introActors[i], TRUE);
    }
    setup.x = 128;
    setup.y = 96;
    setup.priority = 4;
    setup.bgPriority = 1;
    setup.sequence = 41;
    game->pump = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                               CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->pump, FALSE);
    setup.x = 128;
    setup.y = 96;
    setup.priority = 5;
    setup.bgPriority = 1;
    setup.sequence = 38;
    game->pumpBase = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                   CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->pumpBase, FALSE);
    setup.x = 128;
    setup.y = 96;
    setup.priority = 6;
    setup.bgPriority = 1;
    setup.sequence = 39;
    game->pumpAir = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                  CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->pumpAir, FALSE);
    func_0204c520(game->pumpAir, TRUE);
    setup.x = 65;
    setup.y = 122;
    setup.priority = 3;
    setup.bgPriority = 1;
    setup.sequence = 47;
    game->startSign = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                    CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->startSign, FALSE);
    setup.x = 234;
    setup.y = 170;
    setup.priority = 0;
    setup.bgPriority = 1;
    setup.sequence = 50;
    for (i = 0; i < 3; i++) {
        game->rightPuffs[i] = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                            &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c124(game->rightPuffs[i], FALSE);
        func_0204c520(game->rightPuffs[i], TRUE);
    }
    setup.x = 20;
    setup.y = 170;
    setup.sequence = 49;
    for (i = 0; i < 3; i++) {
        game->leftPuffs[i] = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c124(game->leftPuffs[i], FALSE);
        func_0204c520(game->leftPuffs[i], TRUE);
    }
    game->tcbBuffer = GFL_HeapAllocate(game->heapId, GFL_TCBMgrCalcAllocSize(8), FALSE, "ctvt_game.c", 3214);
    game->tcbManager = GFL_TCBMgrCreate(8, game->tcbBuffer);
    pos = 0;
    for (i = 0; i < 4; i++) {
        if (selfNetId == i) {
            game->balloons[i] = CtvtGameBalloon_Create(game, i, pos, TRUE, game->heapId);
        } else if (!func_02042a80(i)) {
            game->balloons[i] = CtvtGameBalloon_Create(game, i, pos, FALSE, game->heapId);
        } else {
            game->balloons[i] = CtvtGameBalloon_Create(game, i, pos, TRUE, game->heapId);
            pos++;
        }
        game->balloons[i]->tcb = GFL_TCBMgrAddTask(game->tcbManager, CtvtGame_BalloonTask, game->balloons[i], 1);
    }
    font = CommTvt_GetFont(sys);
    msgData = CommTvt_GetMsgData(sys);
    queue = CommTvt_GetPrintQueue(sys);
    str = GFL_MsgDataLoadStrbufNew(msgData, 64);
    width = GFL_FontGetBlockWidth(str, font, 0);
    func_02021c7c(queue, BmpWin_GetBitmap(game->titleWindow), 64 - width / 2, 12, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_FlushChar(game->titleWindow);
    BmpWin_FlushMap(game->titleWindow);
    game->titlePending = TRUE;
    GFL_BitmapFill(BmpWin_GetBitmap(game->infoWindow), 15);
    str = GFL_MsgDataLoadStrbufNew(msgData, 60);
    func_02021c7c(queue, BmpWin_GetBitmap(game->infoWindow), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_FlushChar(game->infoWindow);
    BmpWin_FlushMap(game->infoWindow);
    BmpWin_DrawFrame(game->infoWindow, 1, 1, 14);
    game->infoPending = TRUE;
    sys_memset(game->pumps, 0, sizeof(game->pumps));
    sys_memset(game->stages, 0, sizeof(game->stages));
    game->wobbleCooldown = 0;
    game->leftPuff = 0;
    game->rightPuff = 0;
    game->pumpSeq = 41;
    sndHeap = func_02005ce4();
    NNS_SndHeapSaveState(sndHeap);
    NNS_SndArcLoadGroup(2, sndHeap);
}

static BOOL CtvtGame_UpdateIntro(CommTvtWork *sys, CtvtGame *game) {
    u8 frame;

    game->introFrame++;
    if (game->introFrame > 360) {
        return TRUE;
    }
    switch (CtvtGame_GetType(game)) {
    case CTVT_GAME_TYPE_TARGETS:
        game->paletteFrame++;
        if (game->paletteFrame % 5 == 0) {
            game->paletteFrame = 0;
            GFL_BGSysMoveBG(4, 4, 1);
        }
        break;
    case CTVT_GAME_TYPE_BALLOONS:
        game->paletteFrame++;
        game->paletteFrame %= 20;
        frame = game->paletteFrame;
        if (frame % 10 == 0) {
            GFL_BGSysUploadStdPalette(4, (u8 *)game->palette->rawData + (u8)(frame / 10 + 1) * 0x20, 0x20, 0);
        }
        break;
    }
    return FALSE;
}

static void CtvtGame_StartTargets(CommTvtWork *sys, CtvtGame *game) {
    u8 i;

    for (i = 0; i < 3; i++) {
        func_0204c124(game->introActors[i], FALSE);
    }
    for (i = 0; i < 2; i++) {
        func_0204c124(game->targetSigns[i], TRUE);
        func_0204c56c(game->targetSigns[i]);
    }
    GFL_BGSysLoadNCLRDefault(233, 5, 0, 0, 0x20, game->heapId);
    GFL_BGSysLoadNCGRStatic(233, 14, 1, 0, 0, FALSE, game->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 21, 1, 0, 0, FALSE, game->heapId);
}

static void CtvtGame_StartBalloons(CommTvtWork *sys, CtvtGame *game) {
    u8 i;

    for (i = 0; i < 3; i++) {
        func_0204c124(game->introActors[i], FALSE);
    }
    func_0204c124(game->pump, TRUE);
    func_0204c520(game->pump, TRUE);
    func_0204c124(game->pumpBase, TRUE);
    func_0204c124(game->pumpAir, TRUE);
    GFL_BGSysLoadNCLRDefault(233, 6, 0, 0, 0xe0, game->heapId);
    GFL_BGSysLoadNCGRStatic(233, 15, 1, 0, 0, FALSE, game->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 22, 1, 0, 0, FALSE, game->heapId);
}

static BOOL CtvtGame_UpdateTargets(CommTvtWork *sys, CtvtGame *game) {
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 selfNetId = func_02042a6c(func_02040440());
    u32 x;
    u32 y;
    u8 index;

    if (func_0203dac8(&x, &y) == TRUE) {
        CtvtGamePacket packet = { 0 };

        packet.unk5[0] = x;
        packet.unk5[1] = y;
        packet.frame = game->frame;
        CtvtComm_QueueGamePacket(comm, packet, 3);
    }
    if (!game->frameReceived) {
        return FALSE;
    }
    game->frameReceived = FALSE;
    if (selfNetId == 0) {
        if (game->spawnWait <= 0) {
            game->spawnNow = TRUE;
        }
        if (game->spawnNow == TRUE) {
            index = 0;
            if (CtvtGame_FindFreeTarget(game, &index) == TRUE) {
                CtvtGamePacket packet = { 0 };

                packet.unk7 = index;
                CtvtComm_QueueGamePacket(comm, packet, 0);
            }
            game->spawnNow = FALSE;
            game->spawnWait = 15;
        } else {
            game->spawnWait--;
        }
    }
    if (selfNetId == 0) {
        game->hostFrame++;
    }
    if (game->timeLeft != 0) {
        if (game->frame - game->lastTick > 60) {
            game->lastTick = game->frame;
            game->timeLeft--;
            CtvtGame_DrawTimeBar(game);
        }
        if (game->timeLeft == 0) {
            GFL_SndSEPlay(SEQ_SE_LCG_04);
            return TRUE;
        }
    }
    if (game->timeLeft == 40 && !game->sorting) {
        game->sorting = TRUE;
        CtvtGame_StartSort(game, FALSE);
    }
    if (game->timeLeft == 20 && !game->sorting) {
        game->sorting = TRUE;
        CtvtGame_StartSort(game, TRUE);
    }
    if (game->timeLeft < 20 && !game->sorting) {
        func_0204c124(game->hurrySign, TRUE);
    }
    if (game->timeLeft == 20) {
        GFL_SndBGMSetParams(SND_CHANNEL_MASK_ALL, 285, 85, -1);
    }
    CtvtGame_UpdatePlayers(game);
    CtvtGame_DrawTargets3D(game);
    return FALSE;
}

static BOOL CtvtGame_UpdateBalloonStart(CommTvtWork *sys, CtvtGame *game) {
    ClActorPos pos;

    if (func_0204c4a0(game->startSign) == 47) {
        if (!func_0204c560(game->startSign)) {
            func_0204c178(game->startSign, &pos, CLACT_SURFACE_SUB);
            pos.x += 128;
            func_0204c488(game->startSign, 48);
            func_0204c140(game->startSign, &pos, CLACT_SURFACE_SUB);
            func_0204c520(game->startSign, TRUE);
            func_0204c56c(game->startSign);
            func_0204c488(game->pump, 40);
        }
    } else if (func_0204c4a0(game->startSign) == 48 && !func_0204c560(game->startSign)) {
        func_0204c124(game->startSign, FALSE);
        return TRUE;
    }
    CtvtGame_UpdatePlayers(game);
    return FALSE;
}

static BOOL CtvtGame_UpdateBalloons(CommTvtWork *sys, CtvtGame *game) {
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 selfNetId = func_02042a6c(func_02040440());
    u16 pullSeq;
    u16 pushSeq;
    u16 idleSeq;
    s32 hit;
    BOOL pumped;
    u8 i;

    if (!game->frameReceived) {
        return FALSE;
    }
    game->frameReceived = FALSE;
    if (game->timeLeft != 0 && game->frame - game->lastTick > 60) {
        game->lastTick = game->frame;
        game->timeLeft--;
        CtvtGame_DrawTimeBar(game);
    }
    if (game->timeLeft < 20) {
        pullSeq = 44;
        pushSeq = 43;
        idleSeq = 45;
        if (func_0204c4a0(game->pump) == 40) {
            game->pumpSeq = 43;
        }
        if (func_0204c4a0(game->pump) == 41) {
            game->pumpSeq = 44;
        }
    } else {
        pushSeq = 40;
        pullSeq = 41;
        idleSeq = 42;
    }
    if (CtvtGameBalloon_IsIdle(game->balloons[selfNetId]) == TRUE && game->timeLeft != 0) {
        pumped = FALSE;
        hit = func_0203da0c(sPumpRects);
        if (pullSeq == func_0204c4a0(game->pump) && hit >= 0 && hit <= 3) {
            pumped = TRUE;
        }
        if (pushSeq == func_0204c4a0(game->pump) && hit >= 4 && hit <= 7) {
            pumped = TRUE;
        }
        if (pumped == TRUE) {
            CtvtGamePacket packet = { 0 };

            if (hit == 0 || hit == 4) {
                CtvtComm_QueueGamePacket(comm, packet, 6);
                GFL_SndSEPlay(SEQ_SE_LCG_10);
            } else {
                CtvtComm_QueueGamePacket(comm, packet, 5);
                GFL_SndSEPlay(SEQ_SE_LCG_11);
            }
            if (game->wobbleCooldown == 0) {
                game->wobbleCooldown = 80;
                CtvtGameBalloon_Wobble(game->balloons[selfNetId]);
            }
            if (!func_0204c560(game->pumpAir)) {
                func_0204c56c(game->pumpAir);
                if (!GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(SEQ_SE_LCG_06))) {
                    GFL_SndSEPlay(SEQ_SE_LCG_06);
                }
            }
            if (pushSeq == func_0204c4a0(game->pump)) {
                game->pumpSeq = pullSeq;
                if (hit == 4) {
                    func_0204c124(game->rightPuffs[game->rightPuff], TRUE);
                    func_0204c56c(game->rightPuffs[game->rightPuff]);
                    func_0204c438(game->rightPuffs[game->rightPuff], 0);
                    func_0204c438(game->rightPuffs[(game->rightPuff + 1) % 3], 1);
                    func_0204c438(game->rightPuffs[(game->rightPuff + 2) % 3], 2);
                    game->rightPuff++;
                    if (game->rightPuff >= 3) {
                        game->rightPuff = 0;
                    }
                }
            } else {
                game->pumpSeq = pushSeq;
                if (hit == 0) {
                    func_0204c124(game->leftPuffs[game->leftPuff], TRUE);
                    func_0204c56c(game->leftPuffs[game->leftPuff]);
                    func_0204c438(game->leftPuffs[game->leftPuff], 0);
                    func_0204c438(game->leftPuffs[(game->leftPuff + 1) % 3], 1);
                    func_0204c438(game->leftPuffs[(game->leftPuff + 2) % 3], 2);
                    game->leftPuff++;
                    if (game->leftPuff >= 3) {
                        game->leftPuff = 0;
                    }
                }
            }
            func_0204c520(game->pump, TRUE);
            func_0204c56c(game->pump);
        }
        if (!func_0204c560(game->pump)) {
            func_0204c488(game->pump, game->pumpSeq);
            func_0204c550(game->pump);
        }
    } else {
        func_0204c488(game->pump, idleSeq);
    }
    if (game->wobbleCooldown != 0) {
        game->wobbleCooldown--;
    }
    if (selfNetId == 0) {
        game->hostFrame++;
    }
    if (game->timeLeft == 0) {
        game->frame++;
        GFL_SndSEPlay(SEQ_SE_LCG_04);
        for (i = 0; i < 4; i++) {
            game->balloons[i]->active = FALSE;
        }
        CtvtGame_Draw3D(game);
        if (selfNetId == 0) {
            CtvtGameData data;

            for (i = 0; i < 4; i++) {
                data.data[i] = game->scores[i];
            }
            CtvtComm_QueueGameData(comm, data);
        }
        return TRUE;
    }
    if (game->timeLeft == 40 && !game->sorting) {
        game->sorting = TRUE;
        CtvtGame_StartSort(game, FALSE);
    }
    if (game->timeLeft == 20 && !game->sorting) {
        game->sorting = TRUE;
        CtvtGame_StartSort(game, TRUE);
    }
    if (game->timeLeft < 20 && !game->sorting) {
        func_0204c124(game->hurrySign, TRUE);
    }
    if (game->timeLeft == 20) {
        GFL_SndBGMSetParams(SND_CHANNEL_MASK_ALL, 285, 85, -1);
    }
    CtvtGame_UpdatePlayers(game);
    CtvtGame_Draw3D(game);
    return FALSE;
}

static void CtvtGame_InitResults(CommTvtWork *sys, CtvtGame *game) {
    HeapID heapId = game->heapId;
    int type = CtvtGame_GetType(game);
    NNSSndHeapHandle sndHeap;
    u8 slot;
    BmpWin *window;
    u8 selfNetId;
    u8 netId;
    u8 i;
    u8 j;
    u8 picture;
    u32 dest;
    void *src;
    ClActorSetup setup;
    ClActorSetupEx setupEx;

    sndHeap = func_02005ce4();
    NNS_SndHeapLoadState(sndHeap, NNS_SndHeapGetCurrentLevel(sndHeap));
    func_0204c124(game->scoreFrame, FALSE);
    func_0204c124(game->hurrySign, FALSE);
    window = game->timeWindow;
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    if (type == CTVT_GAME_TYPE_TARGETS) {
        for (i = 0; i < 2; i++) {
            func_0204c124(game->targetSigns[i], FALSE);
        }
    } else if (type == CTVT_GAME_TYPE_BALLOONS) {
        func_0204c124(game->pump, FALSE);
        func_0204c124(game->pumpBase, FALSE);
        func_0204c124(game->pumpAir, FALSE);
        for (i = 0; i < 3; i++) {
            func_0204c124(game->rightPuffs[i], FALSE);
            func_0204c124(game->leftPuffs[i], FALSE);
        }
    }
    for (i = 0; i < 4; i++) {
        game->players[i]->score = game->scores[i];
    }
    for (i = 0; i < 4; i++) {
        CtvtGamePlayer_InitResult(sys, game->players[i]);
        GFL_TCBSetCallbackFunc(game->players[i]->tcb, CtvtGamePlayer_ResultTask);
        if (!game->players[i]->absent) {
            game->players[i]->state = 0;
        }
    }
    {
        u8 fileId = CtvtGame_GetType(game);

        GFL_BGSysLoadNCLRDefault(233, fileId + 8, 0, 0, 0x80, game->heapId);
    }
    GFL_BGSysLoadNCGRStatic(233, 18, 1, 0, 0, FALSE, game->heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(233, 25, 1, 0, 0, FALSE, game->heapId);
    setup.x = 128;
    setup.y = 96;
    setup.priority = 0;
    setup.bgPriority = 1;
    setup.sequence = 3;
    game->resultFrame = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4], &setup,
                                      CLACT_SURFACE_SUB, heapId);
    func_0204c124(game->resultFrame, TRUE);
    setup.priority = 1;
    setup.bgPriority = 3;
    for (i = 0; i < 2; i++) {
        if (type == CTVT_GAME_TYPE_TARGETS) {
            setup.sequence = i + 24;
        } else if (type == CTVT_GAME_TYPE_BALLOONS) {
            setup.sequence = 32;
        }
        game->winFlashes[i] = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                            &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c124(game->winFlashes[i], FALSE);
    }
    setup.priority = 2;
    setup.bgPriority = 3;
    setup.sequence = type + 26;
    game->winBanner = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                    CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->winBanner, FALSE);
    setup.priority = 0;
    setup.bgPriority = 3;
    setup.sequence = type + 28;
    game->winGlow = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                  CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->winGlow, FALSE);
    setup.priority = 1;
    setup.bgPriority = 1;
    setup.sequence = 33;
    game->rankSign = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                   CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->rankSign, FALSE);
    setup.priority = 1;
    setup.bgPriority = 1;
    setup.sequence = 37;
    game->tryAgainSign = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                       CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->tryAgainSign, FALSE);
    sys_memset(&setupEx, 0, sizeof(ClActorSetupEx));
    setupEx.base.x = 128;
    setupEx.base.y = 96;
    setupEx.base.sequence = 15;
    setupEx.base.priority = 1;
    setupEx.base.bgPriority = 1;
    setupEx.affineCenter.x = 0;
    setupEx.affineCenter.y = 0;
    setupEx.scaleX = FX32_ONE;
    setupEx.scaleY = FX32_ONE;
    setupEx.rotation = 0;
    setupEx.affineMode = 2;
    game->crown = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setupEx,
                                CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crown, FALSE);
    setupEx.base.x = 59;
    setupEx.base.y = 80;
    setupEx.base.sequence = 16;
    setupEx.base.priority = 2;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[0] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[0], FALSE);
    setupEx.base.x = 90;
    setupEx.base.y = 80;
    setupEx.base.sequence = 17;
    setupEx.base.priority = 3;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[1] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[1], FALSE);
    setupEx.base.x = 120;
    setupEx.base.y = 80;
    setupEx.base.sequence = 18;
    setupEx.base.priority = 4;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[2] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[2], FALSE);
    setupEx.base.x = 150;
    setupEx.base.y = 80;
    setupEx.base.sequence = 19;
    setupEx.base.priority = 5;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[3] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[3], FALSE);
    setupEx.base.x = 177;
    setupEx.base.y = 80;
    setupEx.base.sequence = 20;
    setupEx.base.priority = 6;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[4] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[4], FALSE);
    setupEx.base.x = 200;
    setupEx.base.y = 80;
    setupEx.base.sequence = 21;
    setupEx.base.priority = 6;
    setupEx.base.bgPriority = 1;
    game->crownSparkles[5] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
    func_0204c124(game->crownSparkles[5], FALSE);
    setupEx.base.x = 128;
    setupEx.base.y = 96;
    setupEx.base.priority = 3;
    for (i = 0; i < 2; i++) {
        if (type == CTVT_GAME_TYPE_TARGETS) {
            setupEx.base.sequence = i + 22;
        } else if (type == CTVT_GAME_TYPE_BALLOONS) {
            setupEx.base.sequence = i + 30;
        }
        game->winBursts[i] = func_0204c0a4(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                           &setupEx, CLACT_SURFACE_MAIN, heapId);
        func_0204c124(game->winBursts[i], FALSE);
    }
    {
        u8 ranks[4] = { 1, 1, 1, 1 };

        for (i = 1; i < 4; i++) {
            for (j = 0; j < i; j++) {
                if (game->players[j]->score < game->players[i]->score) {
                    ranks[j]++;
                }
                if (game->players[j]->score > game->players[i]->score) {
                    ranks[i]++;
                }
            }
        }
        for (i = 0; i < 4; i++) {
            game->players[i]->rank = ranks[i];
        }
    }
    for (i = 0; i < 4; i++) {
        if (!game->players[i]->absent) {
            game->shots[i] =
                CtvtGameShot_Create(game, game->players[i]->netId, game->players[i]->pos, game->players[i]->rank,
                                    sShotTopYs[game->players[i]->rank - 1], game->heapId);
            game->shots[i]->tcb = GFL_TCBMgrAddTask(game->tcbManager, CtvtGame_ShotTask, game->shots[i], 2);
        }
    }
    selfNetId = func_02042a6c(func_02040440());
    for (i = 0; i < 5; i++) {
        GFL_G3DMgrDeleteScene(game->g3d, game->scenes[i]);
    }
    CtvtGame_LoadResultScenes(game, heapId);
    slot = 0;
    for (netId = 0; netId < 4; netId++) {
        for (j = 0; j < 2; j++) {
            u16 count = GFL_G3DMgrGetSceneResCount(game->g3d, game->scenes[netId]);
            void *resource = GFL_G3DMgrGetResource(game->g3d, j + count * netId);

            NNSGfdTexKey key = GFL_G3DResGetTexVRAMHandle(resource);

            picture = 0;
            dest = NNS_GfdGetTexKeyAddr(key) + 0x2000 / 32;
            if (j == 0) {
                picture = 0;
            } else if (j == 1) {
                picture = 2;
            }
            if (selfNetId == netId) {
                src = game->cam->pictures[picture];
            } else {
                src = game->members[slot].pictures[picture];
            }
            NNS_GfdRegisterNewVramTransferTask(0, dest, src, 0x2000);
        }
        if (selfNetId != netId) {
            slot++;
        }
    }
    game->resultState = 0;
    game->resultTimer = 0;
}

static BOOL CtvtGame_UpdateResults(CommTvtWork *sys, CtvtGame *game) {
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 memberCount = CommTvt_GetMemberCount(sys);
    u8 i;

    switch (game->resultState) {
    case 0:
        for (i = 0; i < 4; i++) {
            if (!game->players[i]->absent && !game->shots[i]->active && game->players[i]->state == 0) {
                game->players[i]->state = 1;
            }
        }
        game->resultTimer++;
        if (game->resultTimer > 462) {
            game->resultState = 1;
        }
        break;
    case 1:
        for (i = 0; i < 4; i++) {
            game->players[i]->state = 1;
        }
        if (game->players[selfNetId]->rank == 1) {
            GFL_SndBGMPlay(SEQ_ME_LCG_02, SND_CHANNEL_MASK_ALL);
        } else {
            GFL_SndBGMPlay(SEQ_ME_LCG_04, SND_CHANNEL_MASK_ALL);
        }
        game->resultState = 2;
        break;
    case 2:
        if (game->players[selfNetId]->rank == 1) {
            func_0204c124(game->winBanner, TRUE);
            func_0204c124(game->winBursts[0], TRUE);
            func_0204c124(game->winBursts[1], TRUE);
            func_0204c520(game->winBursts[0], TRUE);
            func_0204c520(game->winBursts[1], TRUE);
            func_0204c56c(game->winBursts[0]);
            func_0204c56c(game->winBursts[1]);
            func_0204c124(game->winFlashes[0], TRUE);
            func_0204c124(game->winFlashes[1], TRUE);
            func_0204c520(game->winFlashes[0], TRUE);
            func_0204c520(game->winFlashes[1], TRUE);
            func_0204c56c(game->winFlashes[0]);
            func_0204c56c(game->winFlashes[1]);
            func_0204c124(game->winGlow, TRUE);
            func_0204c520(game->winGlow, TRUE);
            func_0204c56c(game->winGlow);
            game->resultState = 3;
        } else if (game->players[selfNetId]->rank == 2) {
            game->resultState = 4;
        } else {
            game->resultState = 5;
        }
        func_0204c488(game->rankSign, game->players[selfNetId]->rank + 32);
        func_0204c124(game->rankSign, FALSE);
        break;
    case 3:
        if (!func_0204c560(game->winFlashes[0])) {
            func_0204c124(game->winFlashes[0], FALSE);
            func_0204c124(game->winFlashes[1], FALSE);
            func_0204c124(game->crown, TRUE);
            func_0204c520(game->crown, TRUE);
            func_0204c56c(game->crown);
            game->resultState = 6;
        }
        break;
    case 4:
        func_0204c124(game->rankSign, TRUE);
        func_0204c520(game->rankSign, TRUE);
        func_0204c56c(game->rankSign);
        game->resultTimer = 0;
        game->resultState = 7;
        break;
    case 5:
        func_0204c124(game->rankSign, TRUE);
        func_0204c520(game->rankSign, TRUE);
        func_0204c56c(game->rankSign);
        func_0204c124(game->tryAgainSign, TRUE);
        func_0204c520(game->tryAgainSign, TRUE);
        func_0204c56c(game->tryAgainSign);
        game->resultTimer = 0;
        game->resultState = 7;
        break;
    case 6:
        if (!func_0204c560(game->crown)) {
            func_0204c124(game->crown, FALSE);
            for (i = 0; i < 6; i++) {
                func_0204c124(game->crownSparkles[i], TRUE);
                func_0204c520(game->crownSparkles[i], TRUE);
                func_0204c56c(game->crownSparkles[i]);
            }
            func_0204c124(game->rankSign, TRUE);
            func_0204c520(game->rankSign, TRUE);
            func_0204c56c(game->rankSign);
            game->resultTimer = 0;
            game->resultState = 7;
        }
        break;
    case 7:
        game->resultTimer++;
        if (game->resultTimer > 300) {
            game->resultState = 8;
        }
        break;
    case 8:
        return TRUE;
    }
    CtvtGame_UpdatePlayers(game);
    return FALSE;
}

static void CtvtGame_FreeCommon(CommTvtWork *sys, CtvtGame *game) {
    u8 selfNetId = func_02042a6c(func_02040440());
    NNSSndHeapHandle sndHeap = func_02005ce4();
    int level = NNS_SndHeapGetCurrentLevel(sndHeap);
    u8 i;

    if (level == 1) {
        NNS_SndHeapLoadState(sndHeap, level);
    }
    for (i = 0; i < 4; i++) {
        if (game->shots[i] != NULL) {
            CtvtGameShot_Delete(game->shots[i]);
        }
        game->shots[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        if (game->players[i] != NULL) {
            CtvtGamePlayer_Delete(game->players[i]);
            game->players[i] = NULL;
        }
    }
    func_0203a610(game->playerTcbManager);
    if (game->playerTcbManager != NULL) {
        GFL_HeapFree(game->playerTcbManager);
        game->playerTcbManager = NULL;
    }
    if (game->playerTcbBuffer != NULL) {
        GFL_HeapFree(game->playerTcbBuffer);
        game->playerTcbBuffer = NULL;
    }
    func_0203a610(game->tcbManager);
    if (game->tcbManager != NULL) {
        GFL_HeapFree(game->tcbManager);
        game->tcbManager = NULL;
    }
    if (game->tcbBuffer != NULL) {
        GFL_HeapFree(game->tcbBuffer);
        game->tcbBuffer = NULL;
    }
    for (i = 0; i < 3; i++) {
        CtvtGame_DeleteActor(game->introActors[i]);
        game->introActors[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        CtvtGame_DeleteActor(game->targetSigns[i]);
        game->targetSigns[i] = NULL;
    }
    CtvtGame_DeleteActor(game->scoreFrame);
    game->scoreFrame = NULL;
    CtvtGame_DeleteActor(game->hurrySign);
    game->hurrySign = NULL;
    CtvtGame_DeleteActor(game->pump);
    game->pump = NULL;
    CtvtGame_DeleteActor(game->pumpBase);
    game->pumpBase = NULL;
    CtvtGame_DeleteActor(game->pumpAir);
    game->pumpAir = NULL;
    CtvtGame_DeleteActor(game->startSign);
    game->startSign = NULL;
    for (i = 0; i < 3; i++) {
        CtvtGame_DeleteActor(game->rightPuffs[i]);
        game->rightPuffs[i] = NULL;
        CtvtGame_DeleteActor(game->leftPuffs[i]);
        game->leftPuffs[i] = NULL;
    }
    CtvtGame_DeleteActor(game->resultFrame);
    game->resultFrame = NULL;
    CtvtGame_DeleteActor(game->crown);
    game->crown = NULL;
    CtvtGame_DeleteActor(game->winBanner);
    game->winBanner = NULL;
    CtvtGame_DeleteActor(game->rankSign);
    game->rankSign = NULL;
    CtvtGame_DeleteActor(game->tryAgainSign);
    game->tryAgainSign = NULL;
    for (i = 0; i < 6; i++) {
        CtvtGame_DeleteActor(game->crownSparkles[i]);
        game->crownSparkles[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        CtvtGame_DeleteActor(game->winFlashes[i]);
    }
    for (i = 0; i < 2; i++) {
        CtvtGame_DeleteActor(game->winBursts[i]);
    }
    if (game->timeWindow != NULL) {
        BmpWin_Free(game->timeWindow);
        game->timeWindow = NULL;
    }
    if (game->titleWindow != NULL) {
        BmpWin_Free(game->titleWindow);
        game->titleWindow = NULL;
    }
    if (game->infoWindow != NULL) {
        BmpWin_Free(game->infoWindow);
        game->infoWindow = NULL;
    }
    if (game->messageWindow != NULL) {
        BmpWin_Free(game->messageWindow);
        game->messageWindow = NULL;
    }
    if (game->menu != NULL) {
        AppTaskMenu_Free(game->menu);
    }
    if (game->yesNoMenu != NULL) {
        AppTaskMenu_Free(game->yesNoMenu);
        game->yesNoMenu = NULL;
    }
    if (game->menuRes != NULL) {
        AppTaskMenuRes_Free(game->menuRes);
        game->menuRes = NULL;
    }
}

static void CtvtGame_FreeTargets(CommTvtWork *sys, CtvtGame *game) {
    u8 i;

    for (i = 0; i < 8; i++) {
        CtvtGameTarget_Delete(&game->targets[i]);
    }
}

static void CtvtGame_FreeBalloons(CommTvtWork *sys, CtvtGame *game) {
    u8 i;

    for (i = 0; i < 4; i++) {
        if (game->balloons[i] != NULL) {
            CtvtGameBalloon_Delete(game->balloons[i]);
        }
    }
    if (game->paletteFile != NULL) {
        GFL_HeapFree(game->paletteFile);
        game->paletteFile = NULL;
    }
}

static BOOL CtvtGame_CheckError(CommTvtWork *sys, CtvtGame *game) {
    CommTvt_GetComm(sys);
    if (CommTvt_IsErrorShown(sys) == TRUE) {
        func_020120f0(14);
        return TRUE;
    }
    if (CommTvt_GetMemberCount(sys) <= 1) {
        func_020120f0(14);
        return TRUE;
    }
    return FALSE;
}

static u32 CtvtGame_Rand(u32 max) {
    return MATH_Rand32(&sCtvtGameRand, max);
}

static void CtvtGame_InitGraphics(CtvtGame *game, HeapID heapId) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sCtvtGameVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    CtvtGame_InitBGs(game, heapId);
    CtvtGame_Init3D(game, heapId);
    {
        ClActSysSetup setup = data_02093f08;

        ClActSys_Create(&setup, &sCtvtGameVRAMConfig, heapId);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    game->clactUnit = func_0204bf1c(64, 0, heapId);
    func_0204c028(game->clactUnit);
    game->vblankTcb = GFL_VBlankTCBAdd(CtvtGame_VBlank, game, 1);
    game->g3dActive = TRUE;
}

static void CtvtGame_FreeGraphics(CtvtGame *game) {
    GFL_TCBRemove(game->vblankTcb);
    func_0204bf98(game->clactUnit);
    func_0204b758();
    CtvtGame_Free3D(game);
    CtvtGame_FreeBGs(game);
    game->g3dActive = FALSE;
}

static void CtvtGame_DrawTargets3D(CtvtGame *game) {
    u8 i;
    int j;
    u16 first;

    if (game->g3dActive) {
        GFL_G3DSysReset();
        GFL_G3DCameraFlush(game->camera);
        GFL_G3DSysMtxViewFlush();
        NNS_G3DSetLightVector(0, 0, 0, FX16_ONE);
        NNS_G3DSetLightColor(0, GX_RGB(31, 31, 31));
        NNS_G3DSetMatDifAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
        NNS_G3DSetMatSpeEmi(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
        for (i = 0; i < 4; i++) {
            first = GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, game->scenes[i]);
            for (j = 0; j < 3; j++) {
                G3DActor *actor = GFL_G3DMgrGetActor(game->g3d, first + j);

                GFL_G3DActorStepAnmFrameLoop(actor, 0, FX32_ONE);
                GFL_G3DActorStepAnmFrameLoop(actor, 1, FX32_ONE);
            }
        }
        GFL_TCBMgrUpdate(game->tcbManager);
        CtvtGame_SeparateTargets(game);
        gfxClearColor(GX_RGB(18, 29, 31), 0, GX_RGB(31, 31, 31), 63, FALSE);
        GFL_G3DSysReqSwapBuffers();
    }
}

static void CtvtGame_Draw3D(CtvtGame *game) {
    if (game->g3dActive) {
        GFL_G3DSysReset();
        GFL_G3DCameraFlush(game->camera);
        GFL_G3DSysMtxViewFlush();
        NNS_G3DSetLightVector(0, 0, 0, FX16_ONE);
        NNS_G3DSetLightColor(0, GX_RGB(31, 31, 31));
        NNS_G3DSetMatDifAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
        NNS_G3DSetMatSpeEmi(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
        GFL_TCBMgrUpdate(game->tcbManager);
        GFL_G3DSysReqSwapBuffers();
    }
}

static void CtvtGame_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    func_0204b7c8();
}

static void CtvtGame_DeleteActor(ClActor *actor) {
    if (actor != NULL) {
        func_0204c108(actor);
    }
}

static void CtvtGame_InitBGs(CtvtGame *game, HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    CtvtGame_InitBG(&sCtvtGameBG1Setup, 1, 0);
    CtvtGame_InitBG(&sCtvtGameBG2Setup, 2, 0);
    CtvtGame_InitBG(&sCtvtGameBG3Setup, 3, 0);
    CtvtGame_InitBG(&sCtvtGameBG4Setup, 4, 0);
    CtvtGame_InitBG(&sCtvtGameBG5Setup, 5, 0);
    CtvtGame_InitBG(&sCtvtGameBG6Setup, 6, 0);
    CtvtGame_InitBG(&sCtvtGameBG7Setup, 7, 0);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 1, 0x3f, 0, 16);
    GFL_BGSysSetBGPriority(0, 2);
    GFL_BGSysSetBGPriority(2, 0);
    GFL_BGSysSetBGPriority(1, 3);
    GFL_BGSysSetBGPriority(3, 0);
    GFL_BGSysSetBGPriority(4, 3);
    GFL_BGSysSetBGPriority(5, 0);
    GFL_BGSysSetBGPriority(6, 1);
    GFL_BGSysSetBGPriority(7, 2);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
}

static void CtvtGame_FreeBGs(CtvtGame *game) {
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void CtvtGame_InitBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void CtvtGame_Init3D(CtvtGame *game, HeapID heapId) {
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, heapId, NULL);
    GFL_G3DSysSetSwapBufferParams(0, 0);
    gfxClearColor(GX_RGB(31, 31, 31), 0, GX_RGB(31, 31, 31), 63, FALSE);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_AntiAlias(TRUE);
    gfxSetFog(FALSE, 0, 0, 0);
    {
        VecFx32 up = { 0, FX32_ONE, 0 };

        game->camera = GFL_G3DCameraCreate(
            G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0),
            0, FX32_ONE, FX32_CONST(1024), 0, &sCtvtGameCameraPosition, &up, &sCtvtGameCameraTarget, heapId);
    }
    GFL_G3DCameraFlush(game->camera);
}

static void CtvtGame_Free3D(CtvtGame *game) {
    GFL_G3DCameraFree(game->camera);
    GFL_G3DSysFree();
}

static void CtvtGame_LoadTargetResources(CtvtGame *game, HeapID heapId, ArcTool *arc) {
    u8 i;
    u8 scene;
    int j;
    u16 first;
    G3DActor *actor;

    game->clactRes[0] = func_0204bc48(arc, 1, CLACT_VRAM_SUB, 0, heapId);
    game->clactRes[2] = func_0204b81c(arc, 11, FALSE, CLACT_VRAM_SUB, heapId);
    game->clactRes[4] = func_0204bde0(arc, 27, 29, heapId);
    game->clactRes[1] = func_0204bc48(arc, 0, CLACT_VRAM_MAIN, 0, heapId);
    game->clactRes[3] = func_0204b81c(arc, 10, FALSE, CLACT_VRAM_MAIN, heapId);
    game->clactRes[5] = func_0204bde0(arc, 26, 28, heapId);
    game->g3d = GFL_G3DMgrCreate(36, 16, heapId);
    for (i = 0; i < 5; i++) {
        game->scenes[i] = GFL_G3DMgrNewScene(game->g3d, &sTargetScenes[i]);
    }
    for (i = 0; i < 4; i++) {
        first = GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, game->scenes[i]);
        for (j = 0; j < 3; j++) {
            actor = GFL_G3DMgrGetActor(game->g3d, first + j);
            GFL_G3DActorBindAnm(actor, 0);
            GFL_G3DActorBindAnm(actor, 1);
            GFL_G3DActorBindAnm(actor, 2);
        }
    }
    first = GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, 4);
    for (scene = 0; scene < 4; scene++) {
        actor = GFL_G3DMgrGetActor(game->g3d, first + scene);
        GFL_G3DActorBindAnm(actor, 0);
        GFL_G3DActorBindAnm(actor, 1);
    }
}

static void CtvtGame_FreeTargetResources(CtvtGame *game) {
    u8 i;

    for (i = 0; i < 2; i++) {
        func_0204bcd0(game->clactRes[i]);
    }
    for (i = 2; i < 4; i++) {
        func_0204b98c(game->clactRes[i]);
    }
    for (i = 4; i < 6; i++) {
        func_0204be64(game->clactRes[i]);
    }
    for (i = 0; i < 5; i++) {
        GFL_G3DMgrDeleteScene(game->g3d, game->scenes[i]);
    }
    GFL_G3DMgrFree(game->g3d);
}

static void CtvtGame_LoadBalloonResources(CtvtGame *game, HeapID heapId, ArcTool *arc) {
    u8 i;
    G3DActor *actor;

    game->clactRes[0] = func_0204bc48(arc, 1, CLACT_VRAM_SUB, 0, heapId);
    game->clactRes[2] = func_0204b81c(arc, 11, FALSE, CLACT_VRAM_SUB, heapId);
    game->clactRes[4] = func_0204bde0(arc, 27, 29, heapId);
    game->clactRes[1] = func_0204bc48(arc, 0, CLACT_VRAM_MAIN, 0, heapId);
    game->clactRes[3] = func_0204b81c(arc, 10, FALSE, CLACT_VRAM_MAIN, heapId);
    game->clactRes[5] = func_0204bde0(arc, 26, 28, heapId);
    game->g3d = GFL_G3DMgrCreate(36, 16, heapId);
    for (i = 0; i < 5; i++) {
        game->scenes[i] = GFL_G3DMgrNewScene(game->g3d, &sBalloonScenes[i]);
    }
    actor = GFL_G3DMgrGetActor(game->g3d, GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, 4));
    GFL_G3DActorBindAnm(actor, 0);
    GFL_G3DActorBindAnm(actor, 1);
}

static void CtvtGame_FreeBalloonResources(CtvtGame *game) {
    u8 i;

    for (i = 0; i < 2; i++) {
        func_0204bcd0(game->clactRes[i]);
    }
    for (i = 2; i < 4; i++) {
        func_0204b98c(game->clactRes[i]);
    }
    for (i = 4; i < 6; i++) {
        func_0204be64(game->clactRes[i]);
    }
    for (i = 0; i < 5; i++) {
        GFL_G3DMgrDeleteScene(game->g3d, game->scenes[i]);
    }
    GFL_G3DMgrFree(game->g3d);
}

static void CtvtGame_LoadResultScenes(CtvtGame *game, HeapID heapId) {
    u8 i;
    G3DActor *actor;

    for (i = 0; i < 5; i++) {
        game->scenes[i] = GFL_G3DMgrNewScene(game->g3d, &sResultScenes[i]);
    }
    for (i = 0; i < 4; i++) {
        actor = GFL_G3DMgrGetActor(game->g3d, GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, game->scenes[i]));
        GFL_G3DActorBindAnm(actor, 0);
        GFL_G3DActorBindAnm(actor, 1);
    }
    actor = GFL_G3DMgrGetActor(game->g3d, GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, 4));
    GFL_G3DActorBindAnm(actor, 0);
}

static void CtvtGame_PrintMessage(CommTvtWork *sys, CtvtGame *game, u32 msgId) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    StrBuf *str = GFL_StrBufCreate(128, heapId);
    StrBuf *format;
    StrBuf *name;

    func_02021c44(queue);
    GFL_BitmapFill(BmpWin_GetBitmap(game->messageWindow), 15);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    name = GFL_MsgDataLoadStrbufNew(msgData, CtvtGame_GetType(game) + 62);
    func_0202437c(wordSet, 0, name, 0, 1, 2);
    GFL_StrBufFree(name);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    GFL_StrBufFree(format);
    func_02021c7c(queue, BmpWin_GetBitmap(game->messageWindow), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(game->messageWindow);
    BmpWin_FlushMap(game->messageWindow);
    BmpWin_DrawFrame(game->messageWindow, 1, 1, 14);
    game->messagePending = TRUE;
}

static void CtvtGame_DrawTimeBar(CtvtGame *game) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(game->timeWindow);
    u8 width = game->timeLeft * 3;
    u8 color;

    if (game->timeLeft < 20) {
        color = 1;
    } else if (game->timeLeft < 40) {
        color = 2;
    } else {
        color = 3;
    }
    GFL_BitmapFillArea(bitmap, 0, 1, 177, 12, 4);
    GFL_BitmapFillArea(bitmap, 0, 1, width, 12, color);
    BmpWin_Transfer(game->timeWindow);
}

static BOOL CtvtGame_CheckQuit(CommTvtWork *sys, CtvtGame *game) {
    BOOL cancelled = FALSE;

    if (game->childQuit == TRUE && CommTvt_GetSelfIndex(sys) == 0) {
        cancelled = TRUE;
    }
    if (game->hostQuit == TRUE && CommTvt_GetSelfIndex(sys) != 0) {
        cancelled = TRUE;
    }
    if (cancelled == TRUE) {
        if (game->menu != NULL) {
            AppTaskMenu_Free(game->menu);
            game->menu = NULL;
        }
        if (game->yesNoMenu != NULL) {
            AppTaskMenu_Free(game->yesNoMenu);
            game->yesNoMenu = NULL;
        }
        CtvtGame_PrintMessage(sys, game, 46);
        game->exitWait = 90;
        game->replay = FALSE;
    }
    return cancelled;
}

static AppTaskMenu *CtvtGame_CreateReplayMenu(CommTvtWork *sys, CtvtGame *game) {
    AppTaskMenuInit init = { 0 };
    AppTaskMenuItem items[2] = { 0 };
    MsgData *msgData = CommTvt_GetMsgData(sys);
    AppTaskMenu *menu;

    items[0].str = GFL_MsgDataLoadStrbufNew(msgData, 34);
    items[1].str = GFL_MsgDataLoadStrbufNew(msgData, 35);
    items[0].color = 0x39e3;
    items[1].color = 0x39e3;
    items[0].type = 0;
    items[1].type = 0;
    init.heapId = game->heapId;
    init.itemCount = 2;
    init.items = items;
    init.x = 24;
    init.y = 15;
    init.width = 8;
    init.height = 3;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    menu = AppTaskMenu_Create(&init, game->menuRes);
    GFL_StrBufFree(items[0].str);
    GFL_StrBufFree(items[1].str);
    return menu;
}

static AppTaskMenu *CtvtGame_CreateCancelMenu(CommTvtWork *sys, CtvtGame *game) {
    AppTaskMenuInit init = { 0 };
    AppTaskMenuItem items[1] = { 0 };
    AppTaskMenu *menu;

    items[0].str = GFL_MsgDataLoadStrbufNew(CommTvt_GetMsgData(sys), 65);
    items[0].color = 0x39e3;
    items[0].type = 0;
    init.heapId = game->heapId;
    init.itemCount = 1;
    init.items = items;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 24;
    init.y = 15;
    init.width = 8;
    init.height = 3;
    menu = AppTaskMenu_Create(&init, game->menuRes);
    GFL_StrBufFree(items[0].str);
    return menu;
}

static void CtvtGame_LaunchTarget(CtvtGame *game, CtvtGameTarget *target, u16 scene) {
    fx32 x;
    fx32 speed;
    u16 angle;
    VecFx32 dir;
    MtxFx33 rotation;

    target->game = game;
    target->depth = game->nextDepth + 1;
    target->active = TRUE;
    target->alpha = 31;
    target->scene = scene;
    target->frame = game->frame;
    x = (u16)(game->lastColumn * 64 + CtvtGame_Rand(64)) * FX32_ONE;
    target->srt.translation.z = FX32_CONST(-5) * (4 - target->depth);
    target->srt.translation.x = x - FX32_CONST(128);
    target->srt.translation.y = FX32_CONST(-15);
    target->srt.scale.z = target->srt.scale.y = target->srt.scale.x = (target->depth << 9) + 0x700;
    speed = (CtvtGame_Rand(2) + 1) * 0xa00;
    dir.x = 0;
    dir.y = FX32_ONE;
    dir.z = 0;
    angle = game->lastAngle * 20 + CtvtGame_Rand(20) + 330;
    angle %= 360;
    MAT3_RotationEulerZYX(0, 0, angle * 182, &rotation);
    MTX_MultVec33(&dir, &rotation, &target->velocity);
    VEC_Normalize(&target->velocity, &target->velocity);
    if (game->timeLeft <= 20) {
        speed += FX32_CONST(2);
    }
    vecfx_mul(&target->velocity, speed, &target->velocity);
    target->velocity.y += FX32_CONST(0.75);
}

static BOOL CtvtGame_AreTargetsGone(CtvtGame *game) {
    u8 i;

    for (i = 0; i < 8; i++) {
        if (game->targets[i].active == TRUE) {
            return FALSE;
        }
    }
    return TRUE;
}

static u8 CtvtGame_NextFace(CtvtGame *game) {
    u8 scene = sFaceOrders[game->faceOrder][game->faceOrderPos];

    game->faceOrderPos++;
    if (game->faceOrderPos >= 4) {
        game->faceOrderPos = 0;
        game->faceOrder = CtvtGame_Rand(24);
    }
    return scene;
}

static BOOL CtvtGame_FindFreeTarget(CtvtGame *game, u8 *index) {
    u8 i;

    for (i = 0; i < 8; i++) {
        if (game->targets[i].tcb == NULL) {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void CtvtGame_TargetTask(TCB *tcb, void *data) {
    CtvtGameTarget *target = data;

    CtvtGameTarget_Update(target);
    if (target->active == TRUE) {
        CtvtGameTarget_Draw(target);
    }
}

static void CtvtGame_BalloonTask(TCB *tcb, void *data) {
    CtvtGameBalloon *balloon = data;

    if (balloon->active == TRUE) {
        CtvtGameBalloon_Update(balloon);
        CtvtGameBalloon_Draw(balloon);
    }
}

static void CtvtGame_ShotTask(TCB *tcb, void *data) {
    CtvtGameShot *shot = data;

    CtvtGameShot_Update(shot);
    CtvtGameShot_Draw(shot);
}

static BOOL CtvtGame_FindTouchedTarget(CtvtGame *game, u16 frame, int x, int y, u16 *index) {
    BOOL hit = FALSE;
    u8 bestDepth = 0;
    u8 i;
    BOOL inside;
    u8 width;
    u8 height;
    int screenX;
    int screenY;
    int dx;
    int dy;

    for (i = 0; i < 8; i++) {
        inside = FALSE;
        if (game->targets[i].active && game->targets[i].actor == 0 && game->targets[i].hitPending != TRUE) {
            width = sHitSizes[game->targets[i].depth - 1][0];
            height = sHitSizes[game->targets[i].depth - 1][1];
            NNS_G3DProject(&game->targets[i].srt.translation, &screenX, &screenY);
            dx = x - screenX;
            dy = (y - screenY) * ((f32)width / height);
            if (dx * dx + dy * dy <= width * height) {
                inside = TRUE;
            }
            if (inside == TRUE) {
                hit = TRUE;
                if (game->targets[i].depth == 3) {
                    *index = i;
                    return TRUE;
                }
                if (game->targets[i].depth > bestDepth) {
                    *index = i;
                    bestDepth = game->targets[i].depth;
                }
            }
        }
    }
    return hit;
}

static void CtvtGame_SeparateTargets(CtvtGame *game) {
    u8 i;
    u8 j;

    for (i = 0; i < 7; i++) {
        if (game->targets[i].active) {
            for (j = i + 1; j < 8; j++) {
                if (game->targets[i].depth == game->targets[j].depth && game->targets[j].active == TRUE) {
                    CtvtGame_Separate(&game->targets[i].srt.translation.x, &game->targets[i].srt.translation.y,
                                      &game->targets[j].srt.translation.x, &game->targets[j].srt.translation.y,
                                      game->targets[i].srt.scale.x * 0x8c, 0);
                }
            }
        }
    }
}

static void CtvtGame_Separate(fx32 *x0, fx32 *y0, fx32 *x1, fx32 *y1, fx32 minDist, u8 mode) {
    VecFx32 a;
    VecFx32 b;
    fx32 dist;
    fx32 overlap;
    fx32 *right;
    fx32 *left;

    VEC_Set(&a, *x0, *y0, 0);
    VEC_Set(&b, *x1, *y1, 0);
    dist = VEC_Distance(&a, &b);
    if (dist < minDist) {
        overlap = minDist - dist;
        if (*x0 < *x1) {
            left = x0;
            right = x1;
        } else {
            left = x1;
            right = x0;
        }
        switch (mode) {
        case 0:
            *right += FX_Div(overlap, FX32_CONST(4.5));
            *left -= FX_Div(overlap, FX32_CONST(4.5));
            break;
        case 1:
            if (right == x0) {
                *x0 += overlap;
            } else if (left == x0) {
                *x0 -= overlap;
            }
            *y0 -= overlap;
            break;
        case 2:
            if (right == x1) {
                *x1 += overlap;
            } else if (left == x1) {
                *x1 -= overlap;
            }
            break;
        }
    }
}

static void CtvtGame_DrawResults3D(CtvtGame *game) {
    int i;

    GFL_G3DSysReset();
    GFL_G3DCameraFlush(game->camera);
    GFL_G3DSysMtxViewFlush();
    for (i = 0; i < 4; i++) {
        G3DActor *actor = GFL_G3DMgrGetActor(game->g3d, GFL_G3DMgrGetSceneFirstActorIdx(game->g3d, game->scenes[i]));

        GFL_G3DActorStepAnmFrameLoop(actor, 0, FX32_ONE);
        GFL_G3DActorStepAnmFrameLoop(actor, 1, FX32_ONE);
    }
    GFL_TCBMgrUpdate(game->tcbManager);
    GFL_G3DSysReqSwapBuffers();
}

static CtvtGamePlayer *CtvtGamePlayer_Create(CommTvtWork *sys, CtvtGame *game, u8 netId, u8 pos) {
    ClActorSetup setup;
    u8 selfNetId = func_02042a6c(func_02040440());
    u8 x;
    int tileX;
    BOOL isSelf;
    u8 y;
    int tileY;
    CtvtGamePlayer *player;

    x = sPlayerPositions[pos][0];
    y = sPlayerPositions[pos][1];
    player = GFL_HeapAllocate(game->heapId, sizeof(CtvtGamePlayer), TRUE, "ctvt_game.c", 6999);

    isSelf = TRUE;
    player->heapId = game->heapId;
    player->pos = pos;
    player->rank = pos + 1;
    player->netId = netId;
    player->absent = FALSE;
    if (netId != selfNetId) {
        isSelf = FALSE;
    }
    player->isSelf = isSelf;
    player->shakeFrame = 0;
    player->shakeWait = 0;
    player->shaking = FALSE;
    setup.x = x;
    setup.y = y;
    setup.sequence = 1;
    setup.priority = 3;
    setup.bgPriority = 1;
    player->icon = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4], &setup,
                                 CLACT_SURFACE_SUB, game->heapId);
    func_0204c124(player->icon, FALSE);
    func_0204c378(player->icon, netId, 1);
    setup.x = x + 56;
    setup.y = y;
    setup.sequence = 10;
    setup.priority = 1;
    setup.bgPriority = 1;
    player->frame = func_0204c040(game->clactUnit, game->clactRes[0], game->clactRes[2], game->clactRes[4], &setup,
                                  CLACT_SURFACE_SUB, game->heapId);
    func_0204c124(player->frame, FALSE);
    tileX = x / 8;
    tileY = y / 8 - 2;
    player->nameWindow = BmpWin_CreateDynamic(5, tileX - 8, tileY, 14, 4, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(player->nameWindow), 0);
    player->scoreWindow = BmpWin_CreateDynamic(5, tileX + 4, tileY, 6, 4, 15, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(player->scoreWindow), 0);
    player->font = CommTvt_GetFont(sys);
    player->msgData = CommTvt_GetMsgData(sys);
    player->queue = CommTvt_GetPrintQueue(sys);
    player->scoreFormat = GFL_StrBufCreate(11, game->heapId);
    player->scoreStr = GFL_StrBufCreate(11, game->heapId);
    player->name = GFL_StrBufCreate(32, game->heapId);
    return player;
}

static CtvtGamePlayer *CtvtGamePlayer_CreateAbsent(CommTvtWork *sys, CtvtGame *game, u8 netId) {
    CtvtGamePlayer *player = GFL_HeapAllocate(game->heapId, sizeof(CtvtGamePlayer), TRUE, "ctvt_game.c", 7094);

    player->heapId = game->heapId;
    player->rank = 0;
    player->pos = 0;
    player->netId = netId;
    player->namePending = FALSE;
    player->scorePending = FALSE;
    player->absent = TRUE;
    return player;
}

static void CtvtGamePlayer_Delete(CtvtGamePlayer *player) {
    u8 i;

    if (!player->absent) {
        BmpWin_Free(player->nameWindow);
        BmpWin_Free(player->scoreWindow);
        GFL_StrBufFree(player->scoreStr);
        GFL_StrBufFree(player->scoreFormat);
        GFL_StrBufFree(player->name);
        CtvtGame_DeleteActor(player->icon);
        CtvtGame_DeleteActor(player->frame);
        for (i = 0; i < 3; i++) {
            CtvtGame_DeleteActor(player->digits[i]);
        }
    }
    if (player != NULL) {
        GFL_HeapFree(player);
    }
}

static void CtvtGame_UpdatePlayers(CtvtGame *game) {
    u8 i;
    u8 done = 0;
    u8 count = 0;
    BOOL stopped = TRUE;

    if (game->sorting == TRUE) {
        for (i = 0; i < 4; i++) {
            if (game->players[i]->state == 2) {
                done++;
            }
            if (!game->players[i]->absent) {
                count++;
            }
            if (game->players[i]->sliding == TRUE) {
                stopped = FALSE;
            }
        }
        if (stopped == TRUE) {
            game->sorting = FALSE;
        }
        if (done == count) {
            CtvtGame_Rank(game);
            for (i = 0; i < 4; i++) {
                if (!game->players[i]->absent) {
                    CtvtGamePlayer_PlaceAtRank(game->players[i]);
                    game->players[i]->state = 3;
                    game->players[i]->slideWait = (count - game->players[i]->rank) * 40;
                }
            }
        }
    }
    GFL_TCBMgrUpdate(game->playerTcbManager);
}

static void CtvtGamePlayer_Show(CommTvtWork *sys, CtvtGamePlayer *player) {
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    u8 y;
    WordSet *wordSet;

    if (player->absent) {
        return;
    }
    func_0204c124(player->icon, TRUE);
    func_0204c124(player->frame, TRUE);
    msgData = player->msgData;
    font = player->font;
    queue = player->queue;
    y = (player->pos + 1) * 2 + 8;
    textCopy((const u16 *)CtvtComm_GetMemberInfo(sys, CommTvt_GetComm(sys), player->netId)->playerInfo, player->name);
    func_02021c7c(queue, BmpWin_GetBitmap(player->nameWindow), 0, y, player->name, font, 0x440);
    BmpWin_FlushChar(player->nameWindow);
    BmpWin_FlushMap(player->nameWindow);
    player->namePending = TRUE;
    wordSet = GFL_WordSetSystemCreateDefault(player->heapId);
    GFL_MsgDataLoadStrbuf(msgData, 66, player->scoreFormat);
    WordSetNumber(wordSet, 0, player->score, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, player->scoreStr, player->scoreFormat);
    func_02021c7c(queue, BmpWin_GetBitmap(player->scoreWindow), 0, y, player->scoreStr, font, 0x440);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(player->scoreWindow);
    BmpWin_FlushMap(player->scoreWindow);
    player->scorePending = TRUE;
}

// BUG: the scores go up to 999 and a target can come every 16 frames of the 60-second game, at 2 or 4 points a hit,
// so a player who hits the targets with their own face can pass 255. The u8 then shows the score modulo 256 until
// the results, which take the scores as they are
#ifdef BUGFIX
static void CtvtGame_SetScore(CtvtGame *game, u8 netId, u16 score) {
#else
static void CtvtGame_SetScore(CtvtGame *game, u8 netId, u8 score) {
#endif
    CtvtGamePlayer *player = NULL;
    u8 i;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    u8 y;
    WordSet *wordSet;

    for (i = 0; i < 4; i++) {
        if (netId == game->players[i]->netId) {
            player = game->players[i];
            break;
        }
    }
    player->score = score;
    if (player->absent) {
        return;
    }
    queue = player->queue;
    font = player->font;
    msgData = player->msgData;
    y = player->rank * 2 + 8;
    GFL_BitmapFill(BmpWin_GetBitmap(player->scoreWindow), 0);
    wordSet = GFL_WordSetSystemCreateDefault(player->heapId);
    GFL_MsgDataLoadStrbuf(msgData, 66, player->scoreFormat);
    WordSetNumber(wordSet, 0, score, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, player->scoreStr, player->scoreFormat);
    func_02021c7c(queue, BmpWin_GetBitmap(player->scoreWindow), 0, y, player->scoreStr, font, 0x440);
    GFL_WordSetSystemFree(wordSet);
    player->scorePending = TRUE;
}

static void CtvtGame_StartSort(CtvtGame *game, BOOL hurry) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < 4; i++) {
        if (!game->players[i]->absent) {
            count++;
        }
    }
    for (i = 0; i < 4; i++) {
        CtvtGamePlayer *player = game->players[i];

        if (!player->absent) {
            if (game->timeLeft == 40) {
                player->slideWait = player->pos * 40;
            } else {
                player->slideWait = (count - player->rank) * 40;
            }
            player->sliding = TRUE;
            player->unk48 = 0;
            if (!hurry) {
                player->state = 0;
            } else {
                player->state = 1;
            }
        }
    }
}

static void CtvtGame_Rank(CtvtGame *game) {
    u8 ranks[4] = { 1, 2, 3, 4 };
    u8 scores[4] = { 0 };
    u8 i;
    u8 rank = 1;
    u8 j;
    u8 tmp;

    for (i = 0; i < 4; i++) {
        if (!game->players[i]->absent) {
            scores[i] = game->players[i]->score;
            ranks[i] = rank++;
        }
    }
    for (i = 0; i < 3; i++) {
        if (game->players[i]->absent != TRUE) {
            for (j = i; j < 4; j++) {
                if (scores[j] > scores[i]) {
                    tmp = scores[i];
                    scores[i] = scores[j];
                    scores[j] = tmp;
                    tmp = ranks[i];
                    ranks[i] = ranks[j];
                    ranks[j] = tmp;
                }
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (!game->players[i]->absent) {
            game->players[i]->rank = ranks[i];
        }
    }
}

static BOOL CtvtGamePlayer_Slide(CtvtGamePlayer *player, int dx) {
    ClActorPos pos;
    s16 x;

    func_0204c178(player->icon, &pos, CLACT_SURFACE_SUB);
    pos.x += (s16)(dx * 8);
    func_0204c140(player->icon, &pos, CLACT_SURFACE_SUB);
    x = pos.x;
    pos.x = x + 56;
    func_0204c140(player->frame, &pos, CLACT_SURFACE_SUB);
    BmpWin_ClearScreen(player->nameWindow);
    BmpWin_SetPosX(player->nameWindow, x / 8 - 8);
    BmpWin_FlushMap(player->nameWindow);
    BmpWin_ClearScreen(player->scoreWindow);
    BmpWin_SetPosX(player->scoreWindow, x / 8 + 4);
    BmpWin_FlushMap(player->scoreWindow);
    GFL_BGSysQueueScrLoad(5);
    if (player->state <= 1) {
        if (x < -384) {
            return TRUE;
        }
    } else if (player->state == 3) {
        if (x <= sPlayerPositions[player->rank - 1][0]) {
            return TRUE;
        }
    }
    return FALSE;
}

static void CtvtGamePlayer_PlaceAtRank(CtvtGamePlayer *player) {
    ClActorPos pos;
    s16 x;
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    u8 y;
    WordSet *wordSet;

    pos.x = 384;
    pos.y = sPlayerPositions[player->rank - 1][1];
    func_0204c140(player->icon, &pos, CLACT_SURFACE_SUB);
    x = pos.x;
    pos.x = x + 56;
    func_0204c140(player->frame, &pos, CLACT_SURFACE_SUB);
    BmpWin_SetPosX(player->nameWindow, x / 8 - 8);
    BmpWin_SetPosY(player->nameWindow, pos.y / 8 - 2);
    BmpWin_SetPosX(player->scoreWindow, x / 8 + 4);
    BmpWin_SetPosY(player->scoreWindow, pos.y / 8 - 2);
    msgData = player->msgData;
    font = player->font;
    queue = player->queue;
    y = player->rank * 2 + 8;
    GFL_BitmapFill(BmpWin_GetBitmap(player->nameWindow), 0);
    func_02021c7c(queue, BmpWin_GetBitmap(player->nameWindow), 0, y, player->name, font, 0x440);
    BmpWin_FlushChar(player->nameWindow);
    BmpWin_FlushMap(player->nameWindow);
    player->namePending = TRUE;
    wordSet = GFL_WordSetSystemCreateDefault(player->heapId);
    GFL_MsgDataLoadStrbuf(msgData, 66, player->scoreFormat);
    WordSetNumber(wordSet, 0, player->score, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, player->scoreStr, player->scoreFormat);
    GFL_BitmapFill(BmpWin_GetBitmap(player->scoreWindow), 0);
    func_02021c7c(queue, BmpWin_GetBitmap(player->scoreWindow), 0, y, player->scoreStr, font, 0x440);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(player->scoreWindow);
    BmpWin_FlushMap(player->scoreWindow);
    player->scorePending = TRUE;
    GFL_BGSysQueueScrLoad(5);
}

static void CtvtGamePlayer_Task(TCB *tcb, void *data) {
    CtvtGamePlayer *player = data;

    if (player->namePending == TRUE && !func_02021c1c(player->queue, BmpWin_GetBitmap(player->nameWindow))) {
        player->namePending = FALSE;
        BmpWin_Transfer(player->nameWindow);
    }
    if (player->scorePending == TRUE && !func_02021c1c(player->queue, BmpWin_GetBitmap(player->scoreWindow))) {
        player->scorePending = FALSE;
        BmpWin_TransferNow(player->scoreWindow);
    }
    if (player->sliding == TRUE) {
        if (player->slideWait != 0) {
            player->slideWait--;
        }
        if (player->slideWait == 0) {
            switch (player->state) {
            case 0:
                if (CtvtGamePlayer_Slide(player, -1) == TRUE) {
                    player->state = 2;
                }
                break;
            case 1:
                if (CtvtGamePlayer_Slide(player, -1) == TRUE) {
                    player->sliding = FALSE;
                }
                break;
            case 2:
                break;
            case 3:
                if (CtvtGamePlayer_Slide(player, -1) == TRUE) {
                    player->state = 0;
                    player->sliding = FALSE;
                    player->shaking = FALSE;
                }
                break;
            }
        }
    } else if (player->isSelf == TRUE) {
        if (player->shaking == FALSE) {
            player->shakeWait++;
            if (player->shakeWait >= 180) {
                player->shaking = TRUE;
                player->shakeWait = 0;
            }
        }
        if (player->shaking == TRUE) {
            if (player->shakeFrame > 14) {
                player->shakeFrame = 0;
                player->shaking = FALSE;
                return;
            }
            switch (player->shakeFrame) {
            case 0:
            case 6:
            case 8:
            case 14:
                CtvtGamePlayer_Slide(player, -1);
                break;
            case 2:
            case 4:
            case 10:
            case 12:
                CtvtGamePlayer_Slide(player, 1);
                break;
            }
            player->shakeFrame++;
        }
    }
}

static void CtvtGamePlayer_InitResult(CommTvtWork *sys, CtvtGamePlayer *player) {
    u8 i;
    CtvtGame *game = CommTvt_GetGame(sys);
    u8 x;
    u8 y;
    Font *font;
    PrintQueue *queue;
    StrBuf *name;
    u8 width;
    BmpWin *window;
    ClActorSetup setup;
    ClActorSetup digitSetup;

    if (!player->absent) {
        x = sResultPositions[player->pos][0];
        y = sResultPositions[player->pos][1];
        CtvtGame_DeleteActor(player->icon);
        CtvtGame_DeleteActor(player->frame);
        setup.x = x;
        setup.y = y;
        setup.sequence = 12;
        setup.priority = 3;
        setup.bgPriority = 1;
        player->icon = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                     CLACT_SURFACE_MAIN, game->heapId);
        func_0204c124(player->icon, TRUE);
        func_0204c378(player->icon, player->netId, 1);
        setup.x = x + 12;
        setup.y = y + 8;
        setup.sequence = 10;
        setup.priority = 1;
        setup.bgPriority = 1;
        player->frame = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5], &setup,
                                      CLACT_SURFACE_MAIN, game->heapId);
        func_0204c124(player->frame, TRUE);
        for (i = 0; i < 3; i++) {
            digitSetup.x = x + i * 9 - 16;
            digitSetup.y = y + 8;
            digitSetup.sequence = 11;
            digitSetup.priority = 0;
            digitSetup.bgPriority = 1;
            player->digits[i] = func_0204c040(game->clactUnit, game->clactRes[1], game->clactRes[3], game->clactRes[5],
                                              &digitSetup, CLACT_SURFACE_MAIN, game->heapId);
            func_0204c520(player->digits[i], TRUE);
            func_0204c504(player->digits[i], i * 3);
        }
    }
    if (!player->absent) {
        font = CommTvt_GetFont(sys);
        CommTvt_GetMsgData(sys);
        queue = CommTvt_GetPrintQueue(sys);
        name = copyTrainerNameToNewStrbuf(
            (const u16 *)CtvtComm_GetMemberInfo(sys, CommTvt_GetComm(sys), player->netId)->playerInfo, game->heapId);
        width = GFL_FontGetBlockWidth(name, font, 0);
        CtvtGame_ClearWindow(player->nameWindow);
        BmpWin_Free(player->nameWindow);
        GFL_BGSysSetBGEnabled(5, FALSE);
        player->nameWindow = BmpWin_CreateDynamic(3, player->pos * 8, 20, 14, 4, 15, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(player->nameWindow), 0);
        func_02021c7c(queue, BmpWin_GetBitmap(player->nameWindow), 32 - width / 2, 0, name, font, 0x440);
        GFL_StrBufFree(name);
        BmpWin_FlushChar(player->nameWindow);
        BmpWin_FlushMap(player->nameWindow);
        player->namePending = TRUE;
    }
}

static void CtvtGamePlayer_ResultTask(TCB *tcb, void *data) {
    CtvtGamePlayer *player = data;

    if (player->absent == TRUE) {
        return;
    }
    if (player->namePending == TRUE && !func_02021c1c(player->queue, BmpWin_GetBitmap(player->nameWindow))) {
        player->namePending = FALSE;
        BmpWin_Transfer(player->nameWindow);
    }
    if (player->scorePending == TRUE && !func_02021c1c(player->queue, BmpWin_GetBitmap(player->scoreWindow))) {
        player->scorePending = FALSE;
        BmpWin_TransferNow(player->scoreWindow);
    }
    switch (player->state) {
    case 0:
        break;
    case 1:
        func_0204c488(player->digits[0], player->score / 100);
        func_0204c488(player->digits[1], player->score % 100 / 10);
        func_0204c488(player->digits[2], player->score % 10);
        player->state = 2;
        break;
    case 2:
        break;
    }
}
