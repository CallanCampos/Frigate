#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/irc_appbar.h"
#include "app/irc_compatible.h"
#include "app/irc_sync_check.h"
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
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_3.h"

// The Sync Check of the compatibility checker: its graphics, its windows, the two 3D thumbs that follow the stylus, the
// traces of the thumbs that the two players compare, the link that sends them and the steps of the screen. The ROM has
// no string for the file, so the name is a guess. Names are ours

#define HEAP_ID 0x37
#define BG_COUNT 7
#define CLACT_COUNT 32
#define SPARKLE_COUNT 32
#define THUMB_COUNT 2
#define TRACE_POINT_COUNT 10

// The command that sends a player's traces to the other
#define NET_COMMAND_BASE 0xe00

// The frames that a thumb is traced for, and how often a point of the trace is taken
#define TRACE_FRAMES 150
#define TRACE_INTERVAL 16
#define CONNECTED_FRAMES 60
#define HINT_FRAMES 180
#define DONE_FRAMES 30

// Which of the two outlines IrcSyncCheck_ShowOutlines shows
#define OUTLINE_FIRST 1
#define OUTLINE_SECOND 2

// The steps of IrcSyncCheck_StepResult
enum {
    RESULT_EXCHANGE_IDS_FIRST,
    RESULT_PRINT_CHECKING,
    RESULT_SEND_TRACES,
    RESULT_SEND_COUNT,
    RESULT_EXCHANGE_IDS_LAST,
    RESULT_WAIT_CONNECTED,
    RESULT_WAIT_SCORE,
    RESULT_FINISH,
};

typedef struct {
    int x;
    int y;
} IrcSyncPoint;

// The points that one thumb passes through
typedef struct {
    IrcSyncPoint points[TRACE_POINT_COUNT];
    u16 index;
    u32 frames;
    u32 phase;
    BOOL done;
} IrcSyncTrace;

// What a player sends the other
typedef struct {
    IrcSyncPoint first;
    IrcSyncPoint second;
    IrcSyncTrace traceA;
    IrcSyncTrace traceB;
    int retries;
    BOOL isNew;
} IrcSyncData;

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} IrcSyncRect;

typedef struct {
    CompatibleIrcSys *sys;
    int state;
    IrcSyncData peer;
    IrcSyncData own;
    IrcSyncData send;
    BOOL received;
} IrcSyncNet;

typedef struct {
    ClActUnit *unit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *actors[CLACT_COUNT + 1];
} IrcSyncCheckClAct;

typedef struct {
    // The characters of the window frame
    u32 frameChars;
} IrcSyncCheckBG;

// A 3D model with two animations
typedef struct {
    void *resource;
    void *animationResources[2];
    G3DModel *model;
    G3DActor *actor;
    void *animations[2];
    SRTMatrix transform;
    int animation;
    BOOL animationDone;
} IrcSyncCheckModel;

typedef struct {
    G3DCamera *camera;
    IrcSyncCheckModel models[THUMB_COUNT];
} IrcSyncCheckG3d;

// The thumbs, which stay where the stylus was last put
typedef struct {
    IrcSyncCheckModel *models[THUMB_COUNT];
    int lastX;
    int lastY;
} IrcSyncCheckThumbs;

typedef struct {
    IrcSyncCheckBG bg;
    IrcSyncCheckG3d g3d;
    u32 unkC8;
    IrcSyncCheckClAct clact;
    IrcSyncCheckThumbs thumbs;
    TCB *vblankTask;
} IrcSyncCheckGraphic;

typedef struct {
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    WordSet *wordSet;
} IrcSyncCheckText;

typedef struct {
    BmpWin *window;
    PrintWindow printWindow;
    StrBuf *strbuf;
    u16 color;
} IrcSyncCheckWindow;

typedef struct {
    ClActor *actor;
    u32 value;
    u8 unk8[0x24];
} IrcSyncCheckSparkle;

typedef struct {
    IrcSyncCheckSparkle sparkles[SPARKLE_COUNT];
    int unk580;
    int palette;
    int timer;
    int interval;
} IrcSyncCheckSparkles;

typedef struct IrcSyncCheckWork IrcSyncCheckWork;
typedef void (*IrcSyncCheckStepFunc)(IrcSyncCheckWork *wk, u16 *state);

struct IrcSyncCheckWork {
    IrcSyncCheckGraphic graphic;
    IrcSyncCheckText text;
    // The message
    IrcSyncCheckWindow messageWindow;
    IrcSyncCheckSparkles sparkles;
    IrcSyncCheckStepFunc step;
    u16 stepState;
    u16 timer;
    BOOL stepDone;
    int unk734;
    // How many times a thumb was lifted too early
    int retries;
    u32 counter;
    u32 hintTimer;
    BOOL hintToggle;
    // The title
    IrcSyncCheckWindow titleWindow;
    IrcSyncNet net;
    u32 unk9F0[18];
    IrcAppBar *button;
    IrcSyncPoint first;
    IrcSyncPoint second;
    IrcSyncTrace traceA;
    IrcSyncTrace traceB;
    IrcCompatibleParam1 *param;
};

static BOOL IrcSyncCheck_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcSyncCheck_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcSyncCheck_Main(GameProc *proc, u32 *state, void *param, void *work);
static void IrcSyncCheckGraphic_Create(IrcSyncCheckGraphic *graphic, HeapID heapId);
static void IrcSyncCheckGraphic_Free(IrcSyncCheckGraphic *graphic);
static void IrcSyncCheckGraphic_Update(IrcSyncCheckGraphic *graphic);
static ClActor *IrcSyncCheckGraphic_GetActor(IrcSyncCheckGraphic *graphic, int index);
static ClActUnit *IrcSyncCheckGraphic_GetUnit(IrcSyncCheckGraphic *graphic);
static IrcSyncCheckThumbs *IrcSyncCheckGraphic_GetThumbs(IrcSyncCheckGraphic *graphic);
static void IrcSyncCheckGraphic_VBlank(TCB *tcb, void *data);
static void IrcSyncCheckBG_Init(IrcSyncCheckBG *bg, HeapID heapId);
static void IrcSyncCheckBG_Free(IrcSyncCheckBG *bg);
static void IrcSyncCheckBG_VBlank(IrcSyncCheckBG *bg);
static void IrcSyncCheckClAct_Init(IrcSyncCheckClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IrcSyncCheckClAct_Free(IrcSyncCheckClAct *clact);
static void IrcSyncCheckClAct_Update(IrcSyncCheckClAct *clact);
static void IrcSyncCheckClAct_VBlank(IrcSyncCheckClAct *clact);
static ClActor *IrcSyncCheckClAct_GetActor(IrcSyncCheckClAct *clact, int index);
static ClActUnit *IrcSyncCheckClAct_GetUnit(IrcSyncCheckClAct *clact);
static void IrcSyncCheckG3d_Init(IrcSyncCheckG3d *g3d, HeapID heapId);
static void IrcSyncCheckG3d_Free(IrcSyncCheckG3d *g3d);
static void IrcSyncCheckG3d_Begin(IrcSyncCheckG3d *g3d);
static void IrcSyncCheckG3d_End(IrcSyncCheckG3d *g3d);
static IrcSyncCheckModel *IrcSyncCheckG3d_GetModel(IrcSyncCheckG3d *g3d, u16 index);
static void IrcSyncCheck_G3DInit(void);
static void IrcSyncCheckModel_Init(IrcSyncCheckModel *model, ArcTool *arc, const u16 *fileIds, u32 polygonId,
                                   HeapID heapId);
static void IrcSyncCheckModel_Free(IrcSyncCheckModel *model);
static void IrcSyncCheckModel_Update(IrcSyncCheckModel *model);
static BOOL IrcSyncCheckModel_IsAnimationDone(IrcSyncCheckModel *model);
static void IrcSyncCheckModel_SetPosition(IrcSyncCheckModel *model, const VecFx32 *position);
static void IrcSyncCheckModel_SetAnimation(IrcSyncCheckModel *model, int animation);
static int IrcSyncCheckModel_GetAnimation(IrcSyncCheckModel *model);
static void IrcSyncCheckText_Init(IrcSyncCheckText *text, BOOL keepFont, HeapID heapId);
static void IrcSyncCheckText_Free(IrcSyncCheckText *text);
static BOOL IrcSyncCheckText_Update(IrcSyncCheckText *text);
static Font *IrcSyncCheckText_GetFont(IrcSyncCheckText *text);
static PrintQueue *IrcSyncCheckText_GetQueue(IrcSyncCheckText *text);
static MsgData *IrcSyncCheckText_GetMsgData(IrcSyncCheckText *text);
static WordSet *IrcSyncCheckText_GetWordSet(IrcSyncCheckText *text);
static void IrcSyncCheckWindow_Init(IrcSyncCheckWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                    HeapID heapId);
static void IrcSyncCheckWindow_InitWithColor(IrcSyncCheckWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                             u8 palette, u16 color, u8 fromEnd, HeapID heapId);
static void IrcSyncCheckWindow_Free(IrcSyncCheckWindow *win);
static BOOL IrcSyncCheckWindow_Flush(IrcSyncCheckWindow *win, IrcSyncCheckText *text);
static void IrcSyncCheckWindow_PrintMsg(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId, int x, s16 y);
static void IrcSyncCheckWindow_PrintMsgCentered(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId);
static void IrcSyncCheckWindow_PrintMsgPlayer(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId,
                                              CompatibleIrcProfile *other, s16 x, s16 y, HeapID heapId);
static void IrcSyncCheckWindow_Clear(IrcSyncCheckWindow *win);
static void IrcSyncCheck_PrintMsgNamed(IrcSyncCheckWindow *win, IrcSyncCheckWork *wk, u32 msgId);
static void IrcSyncCheck_SetStep(IrcSyncCheckWork *wk, IrcSyncCheckStepFunc step);
static void IrcSyncCheck_SetStepAt(IrcSyncCheckWork *wk, IrcSyncCheckStepFunc step, u16 state);
static void IrcSyncCheck_SetStepDone(IrcSyncCheckWork *wk);
static void IrcSyncCheck_StepWaitFirst(IrcSyncCheckWork *wk, u16 *state);
static void IrcSyncCheck_StepTraceA(IrcSyncCheckWork *wk, u16 *state);
static void IrcSyncCheck_StepTraceB(IrcSyncCheckWork *wk, u16 *state);
static void IrcSyncCheck_StepResult(IrcSyncCheckWork *wk, u16 *state);
static void IrcSyncCheck_StepInterrupted(IrcSyncCheckWork *wk, u16 *state);
static void IrcSyncNet_Init(IrcSyncNet *net, CompatibleIrcSys *sys);
static void IrcSyncNet_Free(IrcSyncNet *net);
static BOOL IrcSyncNet_Send(IrcSyncNet *net, const IrcSyncData *data);
static void IrcSyncNet_GetReceived(IrcSyncNet *net, IrcSyncData *dest);
static void IrcSyncNet_OnReceive(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *IrcSyncNet_GetBuffer(int netId, void *work, int size);
static void IrcSyncCheckThumbs_Init(IrcSyncCheckThumbs *thumbs, IrcSyncCheckG3d *g3d, HeapID heapId);
static void IrcSyncCheckThumbs_Free(IrcSyncCheckThumbs *thumbs);
static void IrcSyncCheckThumbs_Update(IrcSyncCheckThumbs *thumbs);
static void IrcSyncCheckThumbs_SetAnimation(IrcSyncCheckThumbs *thumbs, int index, int animation);
static void IrcSyncCheckThumbs_Place(IrcSyncCheckThumbs *thumbs, int index, int x, int y);
static BOOL IrcSyncRect_IsPressed(const IrcSyncRect *rect, IrcSyncPoint *point);
static BOOL IrcSyncRect_IsHeld(const IrcSyncRect *rect, IrcSyncPoint *point);
static u8 IrcSyncCheck_ComputeScore(IrcSyncCheckWork *wk);
static u8 IrcSyncCheck_CompareTraces(const IrcSyncPoint *ownStart, const IrcSyncPoint *own,
                                     const IrcSyncPoint *peerStart, const IrcSyncPoint *peer);
static void IrcSyncCheck_LoadScreenArea(ArcTool *arc, u32 fileId, u32 bg, u32 tileOffset, u8 srcX, u8 srcY,
                                        u8 srcWidth, u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                        BOOL compressed, HeapID heapId);
static void IrcSyncCheck_ShowOutlines(u32 outlines, HeapID heapId);
static void IrcSyncCheckSparkles_Init(IrcSyncCheckSparkles *sparkles, IrcSyncCheckGraphic *graphic, int unk580,
                                      u32 palette, int firstActor, u32 value);
static void IrcSyncCheckSparkles_Free(IrcSyncCheckSparkles *sparkles);
static void IrcSyncCheckSparkle_Init(IrcSyncCheckSparkle *sparkle, ClActor *actor, u32 palette, u32 value);
static void IrcSyncTrace_Clear(IrcSyncTrace *trace);
static BOOL IrcSyncTrace_Update(IrcSyncTrace *trace, const IrcSyncRect *rect);
static BOOL IrcSyncTrace_IsDone(const IrcSyncTrace *trace);

const GameProcFunctions IRC_SYNC_CHECK_PROC_FUNCTIONS = {
    IrcSyncCheck_Init,
    IrcSyncCheck_Main,
    IrcSyncCheck_Exit,
};

static const u16 sIrcSyncCheckModelFileIds[3] = { 52, 50, 51 };

static const NetCommand sIrcSyncCheckCommands[1] = {
    { IrcSyncNet_OnReceive, IrcSyncNet_GetBuffer },
};

static const VecFx32 sIrcSyncCheckCameraTarget = { 0, 0, 0 };
static const VecFx32 sIrcSyncCheckCameraUpVector = { 0, FX32_ONE, 0 };

// Where the player puts a thumb: the first outline and the second
static const IrcSyncRect sIrcSyncCheckRectFirst = { 0, 16, 88, 160 };
static const IrcSyncRect sIrcSyncCheckRectSecond = { 89, 16, 176, 160 };

static const BGSysLCDConfig sIrcSyncCheckLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_1, GX_BG0_AS_3D };

static const VecFx32 sIrcSyncCheckCameraPosition = { 0, 0, FX32_CONST(70) };

static const u32 sIrcSyncCheckBGs[BG_COUNT] = { 1, 2, 3, 4, 5, 6, 7 };

static const u32 sIrcSyncCheckBGModes[BG_COUNT] = { BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT,
                                                    BGMODE_TEXT, BGMODE_TEXT, BGMODE_AFFINE };

static const Light sIrcSyncCheckLights[4] = {
    { { 0, -4096, 0 }, GX_RGB(16, 16, 16) },
    { { 0, 4096, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -4096, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -4096, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sIrcSyncCheckVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSetup sIrcSyncCheckBGSetups[BG_COUNT] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x4000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL IrcSyncCheck_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcSyncCheckWork *wk;
    IrcCompatibleParam1 *syncParam = param;
    ClActUnit *unit;
    Font *font;
    PrintQueue *queue;

    GFL_HeapCreateChild(HEAPID_USER, HEAP_ID, 0x25000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcSyncCheckWork), HEAP_ID);
    sys_memset(wk, 0, sizeof(IrcSyncCheckWork));
    wk->param = syncParam;
    wk->param->unk11 = 0;
    IrcSyncNet_Init(&wk->net, wk->param->sys);
    IrcSyncCheckGraphic_Create(&wk->graphic, HEAP_ID);
    IrcSyncCheckText_Init(&wk->text, FALSE, HEAP_ID);
    if (wk->param->gsys != NULL) {
        GSYS_GetGameCommSystem(wk->param->gsys);
    }
    IrcSyncCheckWindow_Init(&wk->messageWindow, 5, 1, 18, 30, 5, 8, HEAP_ID);
    BmpWin_DrawFrame(wk->messageWindow.window, 0, (u16)wk->graphic.bg.frameChars, 6);
    IrcSyncCheckWindow_InitWithColor(&wk->titleWindow, 7, 6, 3, 20, 5, 8, 1, TRUE, HEAP_ID);
    IrcSyncCheckWindow_PrintMsgCentered(&wk->titleWindow, &wk->text, BtlMain3_Text_SyncCheck);
    GFL_BGSysScaleBGReq(7, 3, 0x1000);
    GFL_BGSysScaleBGReq(7, 6, 0x1000);
    GFL_BGSysAdjustBGOriginReq(7, 9, 0x80);
    GFL_BGSysAdjustBGOriginReq(7, 12, 0x28);
    IrcSyncTrace_Clear(&wk->traceA);
    IrcSyncTrace_Clear(&wk->traceB);
    IrcSyncCheckSparkles_Init(&wk->sparkles, &wk->graphic, 0, 1, 0, 1);
    unit = IrcSyncCheckGraphic_GetUnit(&wk->graphic);
    font = IrcSyncCheckText_GetFont(&wk->text);
    queue = IrcSyncCheckText_GetQueue(&wk->text);
    wk->button = IrcAppBar_Create(2, unit, 1, 13, 13, 2, font, queue, HEAP_ID);
    CompatibleIrcSys_SetCount(wk->param->sys, 2);
    IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepWaitFirst);
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    G2_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWnd1InsidePlane(GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                              GX_WND_PLANEMASK_OBJ,
                          FALSE);
    G2_SetWnd0Position(255, 168, 44, 192);
    G2_SetWnd1Position(0, 168, 255, 192);
    return TRUE;
}

static BOOL IrcSyncCheck_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcSyncCheckWork *wk = work;

    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    IrcAppBar_Free(wk->button);
    IrcSyncCheckSparkles_Free(&wk->sparkles);
    IrcSyncCheckWindow_Free(&wk->titleWindow);
    IrcSyncCheckWindow_Free(&wk->messageWindow);
    IrcSyncCheckGraphic_Free(&wk->graphic);
    IrcSyncCheckText_Free(&wk->text);
    IrcSyncNet_Free(&wk->net);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAP_ID);
    return TRUE;
}

static BOOL IrcSyncCheck_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcSyncCheckWork *wk = work;

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        GFL_FadeSet(3, 16, 0, 0);
        *state = 2;
        break;
    case 2:
        if (GFL_FadeIsRunning()) {
            break;
        }
        *state = 3;
        break;
    case 3:
        wk->step(wk, &wk->stepState);
        if (wk->stepDone) {
            *state = 4;
        }
        break;
    case 4:
        GFL_FadeSet(3, 0, 16, 0);
        *state = 5;
        break;
    case 5:
        if (GFL_FadeIsRunning()) {
            break;
        }
        *state = 6;
        break;
    case 6:
        return TRUE;
    }
    if (IrcSyncCheckText_Update(&wk->text)) {
        IrcSyncCheckWindow_Flush(&wk->messageWindow, &wk->text);
        IrcSyncCheckWindow_Flush(&wk->titleWindow, &wk->text);
    }
    IrcSyncCheckGraphic_Update(&wk->graphic);
    CompatibleIrcSys_Update(wk->param->sys);
    IrcAppBar_Update(wk->button);
    return FALSE;
}

static void IrcSyncCheckGraphic_Create(IrcSyncCheckGraphic *graphic, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(IrcSyncCheckGraphic));
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sIrcSyncCheckVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    IrcSyncCheckClAct_Init(&graphic->clact, &sIrcSyncCheckVRAMConfig, heapId);
    IrcSyncCheckBG_Init(&graphic->bg, heapId);
    IrcSyncCheckG3d_Init(&graphic->g3d, heapId);
    IrcSyncCheckThumbs_Init(&graphic->thumbs, &graphic->g3d, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(IrcSyncCheckGraphic_VBlank, graphic, 0);
}

static void IrcSyncCheckGraphic_Free(IrcSyncCheckGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IrcSyncCheckThumbs_Free(&graphic->thumbs);
    IrcSyncCheckG3d_Free(&graphic->g3d);
    IrcSyncCheckBG_Free(&graphic->bg);
    IrcSyncCheckClAct_Free(&graphic->clact);
}

static void IrcSyncCheckGraphic_Update(IrcSyncCheckGraphic *graphic) {
    IrcSyncCheckClAct_Update(&graphic->clact);
    IrcSyncCheckG3d_Begin(&graphic->g3d);
    IrcSyncCheckThumbs_Update(&graphic->thumbs);
    IrcSyncCheckG3d_End(&graphic->g3d);
}

static ClActor *IrcSyncCheckGraphic_GetActor(IrcSyncCheckGraphic *graphic, int index) {
    return IrcSyncCheckClAct_GetActor(&graphic->clact, index);
}

static ClActUnit *IrcSyncCheckGraphic_GetUnit(IrcSyncCheckGraphic *graphic) {
    return IrcSyncCheckClAct_GetUnit(&graphic->clact);
}

static IrcSyncCheckThumbs *IrcSyncCheckGraphic_GetThumbs(IrcSyncCheckGraphic *graphic) {
    return &graphic->thumbs;
}

static void IrcSyncCheckGraphic_VBlank(TCB *tcb, void *data) {
    IrcSyncCheckGraphic *graphic = data;

    IrcSyncCheckBG_VBlank(&graphic->bg);
    IrcSyncCheckClAct_VBlank(&graphic->clact);
}

static void IrcSyncCheckBG_Init(IrcSyncCheckBG *bg, HeapID heapId) {
    ArcTool *arc;
    int i;

    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sIrcSyncCheckLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysCreateBG(sIrcSyncCheckBGs[i], &sIrcSyncCheckBGSetups[i], sIrcSyncCheckBGModes[i]);
        GFL_BGSysClearBG(sIrcSyncCheckBGs[i]);
        GFL_BGSysSetBGEnabled(sIrcSyncCheckBGs[i], TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_MAIN_BG, 0x120, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 3, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 48, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 45, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 2, 0, 0, FALSE, heapId);
    IrcSyncCheck_ShowOutlines(OUTLINE_FIRST, heapId);
    GFL_BGSysFillChar(5, 0, 1, 0);
    bg->frameChars = GFL_BGSysLoadArcNCGRDynamic(arc, 49, 5, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 5, 12, 13, 8);
}

static void IrcSyncCheckBG_Free(IrcSyncCheckBG *bg) {
    int i;

    G2_BlendNone();
    GFL_BGSysFreeCharMemory(5, (u16)bg->frameChars, (u16)(bg->frameChars >> 16));
    GFL_BGSysFreeFilledChar(5, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sIrcSyncCheckBGs[i]);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void IrcSyncCheckBG_VBlank(IrcSyncCheckBG *bg) {
    GFL_BGSysUpdate();
}

static void IrcSyncCheckClAct_Init(IrcSyncCheckClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;
    int i;

    sys_memset(clact, 0, sizeof(IrcSyncCheckClAct));
    ClActSys_Create(&data_02093f08, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    clact->palette = func_0204bba0(arc, 12, CLACT_VRAM_SUB, 0, heapId);
    clact->chars = func_0204b81c(arc, 11, FALSE, CLACT_VRAM_SUB, heapId);
    clact->cellAnims = func_0204bde0(arc, 10, 9, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < CLACT_COUNT; i++) {
        clact->actors[i] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup,
                                         CLACT_SURFACE_SUB, heapId);
        func_0204c124(clact->actors[i], FALSE);
        func_0204c468(clact->actors[i], 3);
        func_0204c520(clact->actors[i], TRUE);
        func_0204c488(clact->actors[i], 0);
    }
}

static void IrcSyncCheckClAct_Free(IrcSyncCheckClAct *clact) {
    int i;

    for (i = 0; i < CLACT_COUNT + 1; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204bcd0(clact->palette);
    func_0204b98c(clact->chars);
    func_0204be64(clact->cellAnims);
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(IrcSyncCheckClAct));
}

static void IrcSyncCheckClAct_Update(IrcSyncCheckClAct *clact) {
    func_0204b794();
}

static void IrcSyncCheckClAct_VBlank(IrcSyncCheckClAct *clact) {
    func_0204b7c8();
}

static ClActor *IrcSyncCheckClAct_GetActor(IrcSyncCheckClAct *clact, int index) {
    return clact->actors[index];
}

static ClActUnit *IrcSyncCheckClAct_GetUnit(IrcSyncCheckClAct *clact) {
    return clact->unit;
}

static void IrcSyncCheckG3d_Init(IrcSyncCheckG3d *g3d, HeapID heapId) {
    ArcTool *arc;

    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, heapId, IrcSyncCheck_G3DInit);
    g3d->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(24), FX32_CONST(-24), FX32_CONST(-32),
                                      FX32_CONST(32), FX32_ONE, FX32_CONST(1000), 0, &sIrcSyncCheckCameraPosition,
                                      &sIrcSyncCheckCameraUpVector, &sIrcSyncCheckCameraTarget, heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    IrcSyncCheckModel_Init(&g3d->models[0], arc, sIrcSyncCheckModelFileIds, 1, heapId);
    IrcSyncCheckModel_Init(&g3d->models[1], arc, sIrcSyncCheckModelFileIds, 2, heapId);
    GFL_ArcToolFree(arc);
}

static void IrcSyncCheckG3d_Free(IrcSyncCheckG3d *g3d) {
    int i;

    for (i = 0; i < THUMB_COUNT; i++) {
        IrcSyncCheckModel_Free(&g3d->models[i]);
    }
    GFL_G3DCameraFree(g3d->camera);
    GFL_G3DSysFree();
}

static void IrcSyncCheckG3d_Begin(IrcSyncCheckG3d *g3d) {
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g3d->camera);
    GFL_G3DSysMtxViewFlush();
}

static void IrcSyncCheckG3d_End(IrcSyncCheckG3d *g3d) {
    GFL_G3DSysReqSwapBuffers();
}

static IrcSyncCheckModel *IrcSyncCheckG3d_GetModel(IrcSyncCheckG3d *g3d, u16 index) {
    return &g3d->models[index];
}

// The 3D system calls this once it is set up
static void IrcSyncCheck_G3DInit(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G2_SetBG0Priority(0);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(FALSE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    gfxSetFog(FALSE, 0, 0, 0);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sIrcSyncCheckLights); i++) {
        GFL_G3DSysLightSet(i, &sIrcSyncCheckLights[i]);
    }
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

static void IrcSyncCheckModel_Init(IrcSyncCheckModel *model, ArcTool *arc, const u16 *fileIds, u32 polygonId,
                                   HeapID heapId) {
    void *texture;
    NNSG3dRenderObj *renderObj;
    int i;

    sys_memset(model, 0, sizeof(IrcSyncCheckModel));
    texture = NULL;
    model->resource = GFL_G3DSysReadArcToolResource(arc, fileIds[0]);
    if (GFL_G3DResCheckType(model->resource, G3D_RES_CHECK_TEX)) {
        texture = model->resource;
        GFL_G3DResUploadTexData(model->resource);
    }
    if (GFL_G3DResCheckType(model->resource, G3D_RES_CHECK_MDL)) {
        model->model = GFL_G3DMdlCreate(model->resource, 0, texture);
        renderObj = GFL_G3DMdlGetEngineModel(model->model);
        for (i = 0; i < 2; i++) {
            model->animationResources[i] = GFL_G3DSysReadArcToolResource(arc, fileIds[1 + i]);
            model->animations[i] = GFL_G3DAnmCreate(model->model, model->animationResources[i], 0);
        }
        model->actor = GFL_G3DActorCreate(model->model, model->animations, 2);
        for (i = 0; i < 2; i++) {
            GFL_G3DActorBindAnm(model->actor, i);
        }
        func_020683ec(renderObj->resMdl, polygonId);
    }
    MTX_Identity33_(&model->transform.rotation);
    model->transform.scale.x = FX32_ONE;
    model->transform.scale.y = FX32_ONE;
    model->transform.scale.z = FX32_ONE;
}

static void IrcSyncCheckModel_Free(IrcSyncCheckModel *model) {
    int i;

    GFL_G3DActorFree(model->actor);
    for (i = 0; i < 2; i++) {
        if (model->animations[i] != NULL) {
            GFL_G3DAnmFree(model->animations[i]);
        }
    }
    GFL_G3DMdlFree(model->model);
    for (i = 0; i < 2; i++) {
        GFL_G3DResFree(model->animationResources[i]);
    }
    GFL_G3DResFree(model->resource);
    sys_memset(model, 0, sizeof(IrcSyncCheckModel));
}

static void IrcSyncCheckModel_Update(IrcSyncCheckModel *model) {
    int i;
    fx32 frame;
    BOOL done;

    if (model->animation != 0) {
        for (i = 0; i < 2; i++) {
            GFL_G3DActorStepAnmFrameLoop(model->actor, i, FX32_ONE);
        }
        GFL_G3DActorGetAnmFrame(model->actor, 0, &frame);
        done = FALSE;
        if (frame == 0) {
            done = TRUE;
        }
        model->animationDone = done;
        GFL_G3DSysDrawObj(model->actor, &model->transform);
    }
}

static BOOL IrcSyncCheckModel_IsAnimationDone(IrcSyncCheckModel *model) {
    return model->animationDone;
}

static void IrcSyncCheckModel_SetPosition(IrcSyncCheckModel *model, const VecFx32 *position) {
    model->transform.translation = *position;
}

static void IrcSyncCheckModel_SetAnimation(IrcSyncCheckModel *model, int animation) {
    if (model->animation != animation) {
        GFL_G3DActorResetAnmFrame(model->actor, 0);
        model->animation = animation;
    }
}

static int IrcSyncCheckModel_GetAnimation(IrcSyncCheckModel *model) {
    return model->animation;
}

static void IrcSyncCheckText_Init(IrcSyncCheckText *text, BOOL keepFont, HeapID heapId) {
    sys_memset(text, 0, sizeof(IrcSyncCheckText));
    func_020232d0();
    if (!keepFont) {
        text->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    }
    text->queue = func_02021998(heapId);
    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_3, heapId);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
}

static void IrcSyncCheckText_Free(IrcSyncCheckText *text) {
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
    func_02021a18(text->queue);
    GFL_FontFree(text->font);
    sys_memset(text, 0, sizeof(IrcSyncCheckText));
}

static BOOL IrcSyncCheckText_Update(IrcSyncCheckText *text) {
    return func_02021a3c(text->queue);
}

static Font *IrcSyncCheckText_GetFont(IrcSyncCheckText *text) {
    return text->font;
}

static PrintQueue *IrcSyncCheckText_GetQueue(IrcSyncCheckText *text) {
    return text->queue;
}

static MsgData *IrcSyncCheckText_GetMsgData(IrcSyncCheckText *text) {
    return text->msgData;
}

static WordSet *IrcSyncCheckText_GetWordSet(IrcSyncCheckText *text) {
    return text->wordSet;
}

static void IrcSyncCheckWindow_Init(IrcSyncCheckWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                    HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcSyncCheckWindow));
    win->color = 15;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    IrcSyncCheckWindow_Clear(win);
    BmpWin_TransferNow(win->window);
}

static void IrcSyncCheckWindow_InitWithColor(IrcSyncCheckWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                             u8 palette, u16 color, u8 fromEnd, HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcSyncCheckWindow));
    win->color = color;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, fromEnd);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcSyncCheckWindow_Free(IrcSyncCheckWindow *win) {
    GFL_StrBufFree(win->strbuf);
    BmpWin_Free(win->window);
    sys_memset(win, 0, sizeof(IrcSyncCheckWindow));
}

static BOOL IrcSyncCheckWindow_Flush(IrcSyncCheckWindow *win, IrcSyncCheckText *text) {
    PrintWindow_Flush(&win->printWindow, text->queue);
    return PrintWindow_IsPrinted(&win->printWindow);
}

static void IrcSyncCheckWindow_PrintMsg(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId, int x, s16 y) {
    MsgData *msgData = IrcSyncCheckText_GetMsgData(text);
    PrintQueue *queue = IrcSyncCheckText_GetQueue(text);
    Font *font = IrcSyncCheckText_GetFont(text);

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    func_02021c44(queue);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

static void IrcSyncCheckWindow_PrintMsgCentered(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId) {
    MsgData *msgData = IrcSyncCheckText_GetMsgData(text);
    PrintQueue *queue = IrcSyncCheckText_GetQueue(text);
    Font *font = IrcSyncCheckText_GetFont(text);
    u16 x;
    u16 y;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    x = BmpWin_GetSizeX(win->window) * 4;
    y = BmpWin_GetSizeY(win->window) * 4;
    x -= (u16)(GFL_FontGetBlockWidth(win->strbuf, font, 0) / 2);
    y -= (u16)(GFL_FontGetBlockHeight(win->strbuf, font) / 2);
    func_02021c44(queue);
    PrintWindow_Print(&win->printWindow, queue, x, y, win->strbuf, font, PRINT_COLOR(15, 14, 4));
}

static void IrcSyncCheckWindow_PrintMsgPlayer(IrcSyncCheckWindow *win, IrcSyncCheckText *text, u32 msgId,
                                              CompatibleIrcProfile *other, s16 x, s16 y, HeapID heapId) {
    WordSet *wordSet;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    StrBuf *format;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    wordSet = IrcSyncCheckText_GetWordSet(text);
    msgData = IrcSyncCheckText_GetMsgData(text);
    copyVarForText(wordSet, 0, &other->trainer);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    GFL_WordSetFormatStrbuf(wordSet, win->strbuf, format);
    GFL_StrBufFree(format);
    queue = IrcSyncCheckText_GetQueue(text);
    font = IrcSyncCheckText_GetFont(text);
    func_02021c44(queue);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

// Prints a message with the other player's name in it, if there is a link
static void IrcSyncCheck_PrintMsgNamed(IrcSyncCheckWindow *win, IrcSyncCheckWork *wk, u32 msgId) {
    if (wk->param->gsys != NULL) {
        IrcSyncCheckWindow_PrintMsgPlayer(win, &wk->text, msgId, wk->param->profile, 0, 0, HEAP_ID);
    } else {
        IrcSyncCheckWindow_PrintMsg(win, &wk->text, msgId, 0, 0);
    }
}

static void IrcSyncCheckWindow_Clear(IrcSyncCheckWindow *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_FlushChar(win->window);
}

static void IrcSyncCheck_SetStep(IrcSyncCheckWork *wk, IrcSyncCheckStepFunc step) {
    wk->step = step;
    wk->stepState = 0;
}

static void IrcSyncCheck_SetStepAt(IrcSyncCheckWork *wk, IrcSyncCheckStepFunc step, u16 state) {
    wk->step = step;
    wk->stepState = state;
}

static void IrcSyncCheck_SetStepDone(IrcSyncCheckWork *wk) {
    wk->stepDone = TRUE;
}

// Waits for a thumb on the first outline
static void IrcSyncCheck_StepWaitFirst(IrcSyncCheckWork *wk, u16 *state) {
    IrcSyncCheckThumbs *thumbs;
    IrcSyncPoint *first = &wk->first;
    int i;

    if (IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_SYNC_CHECK_RESULT_CANCELLED;
        IrcSyncCheck_SetStepDone(wk);
    } else if (!IrcAppBar_IsPressedOrDone(wk->button)) {
        switch (*state) {
        case 0:
            if (IrcSyncTrace_IsDone(&wk->traceA)) {
                IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_PleaseRemoveBothThumbs, 0, 0);
            } else {
                if (wk->param->gsys != NULL) {
                    IrcSyncCheckWindow_PrintMsgPlayer(&wk->messageWindow, &wk->text, BtlMain3_Text_PlaceLeftThumbOutline, wk->param->profile, 0, 0,
                                                      HEAP_ID);
                } else {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_PlaceLeftThumbOutline, 0, 0);
                }
            }
            thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
            for (i = 0; i < THUMB_COUNT; i++) {
                IrcSyncCheckThumbs_SetAnimation(thumbs, i, 0);
            }
            GFL_SndStop();
            IrcSyncCheck_ShowOutlines(OUTLINE_FIRST, HEAP_ID);
            wk->hintTimer = 0;
            *state = 1;
            break;
        case 1:
            if (IrcSyncRect_IsPressed(&sIrcSyncCheckRectFirst, first)) {
                IrcSyncCheck_ShowOutlines(0, HEAP_ID);
                wk->hintTimer = 0;
                wk->hintToggle = FALSE;
                thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
                IrcSyncCheckThumbs_SetAnimation(thumbs, 0, 1);
                IrcSyncCheckThumbs_Place(thumbs, 0, first->x, first->y);
                GFL_SndSEPlay(SEQ_SE_SYS_18);
                if (IrcSyncTrace_IsDone(&wk->traceA)) {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_KeepLeftThumbSteady, 0, 0);
                    IrcSyncCheck_ShowOutlines(OUTLINE_FIRST | OUTLINE_SECOND, HEAP_ID);
                    IrcSyncCheck_SetStepAt(wk, IrcSyncCheck_StepTraceA, 1);
                } else {
                    if (wk->param->gsys != NULL) {
                        IrcSyncCheckWindow_PrintMsgPlayer(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckingHowSynchronized, wk->param->profile, 0, 0,
                                                          HEAP_ID);
                    } else {
                        IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckingHowSynchronized, 0, 0);
                    }
                    IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepTraceA);
                }
            } else if (!IrcSyncTrace_IsDone(&wk->traceA)) {
                if (wk->hintTimer++ >= HINT_FRAMES) {
                    wk->hintTimer = 0;
                    wk->hintToggle ^= 1;
                    if (wk->hintToggle) {
                        IrcSyncCheck_PrintMsgNamed(&wk->messageWindow, wk, BtlMain3_Text_IfNothingHappensPlace);
                    } else {
                        IrcSyncCheck_PrintMsgNamed(&wk->messageWindow, wk, BtlMain3_Text_PlaceLeftThumbOutline);
                    }
                }
            }
            break;
        }
    }
    if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepInterrupted);
    }
}

// Traces the first thumb while it is held
static void IrcSyncCheck_StepTraceA(IrcSyncCheckWork *wk, u16 *state) {
    IrcSyncPoint point;
    IrcSyncCheckThumbs *thumbs;
    IrcSyncPoint *second = &wk->second;
    IrcSyncTrace *trace = &wk->traceA;

    if (IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_SYNC_CHECK_RESULT_CANCELLED;
        IrcSyncCheck_SetStepDone(wk);
    } else if (!IrcAppBar_IsPressedOrDone(wk->button)) {
        switch (*state) {
        case 0:
            if (IrcSyncRect_IsHeld(&sIrcSyncCheckRectFirst, &point)) {
                if (IrcSyncTrace_Update(trace, &sIrcSyncCheckRectFirst)) {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_KeepLeftThumbSteady, 0, 0);
                    IrcSyncCheck_ShowOutlines(OUTLINE_FIRST | OUTLINE_SECOND, HEAP_ID);
                    *state = 1;
                }
                thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
                IrcSyncCheckThumbs_SetAnimation(thumbs, 0, 1);
                IrcSyncCheckThumbs_Place(thumbs, 0, point.x, point.y);
            } else {
                if (IrcSyncTrace_IsDone(trace)) {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_KeepLeftThumbSteady, 0, 0);
                    IrcSyncCheck_ShowOutlines(OUTLINE_FIRST | OUTLINE_SECOND, HEAP_ID);
                    *state = 1;
                } else {
                    IrcSyncTrace_Clear(trace);
                }
                *state = 2;
            }
            break;
        case 1:
            if (IrcSyncRect_IsHeld(&sIrcSyncCheckRectSecond, second)) {
                IrcSyncCheck_ShowOutlines(0, HEAP_ID);
                if (wk->param->gsys != NULL) {
                    IrcSyncCheckWindow_PrintMsgPlayer(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckingHowSynchronized, wk->param->profile, 0, 0,
                                                      HEAP_ID);
                } else {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckingHowSynchronized, 0, 0);
                }
                IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepTraceB);
                thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
                IrcSyncCheckThumbs_SetAnimation(thumbs, 1, 1);
                IrcSyncCheckThumbs_Place(thumbs, 1, second->x, second->y);
                GFL_SndSEPlay(SEQ_SE_SYS_23);
            } else if (!IrcSyncRect_IsHeld(&sIrcSyncCheckRectFirst, &point)) {
                *state = 2;
            }
            break;
        case 2:
            wk->retries++;
            IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepWaitFirst);
            break;
        }
    }
    if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepInterrupted);
    }
}

// Traces the second thumb while it is held
static void IrcSyncCheck_StepTraceB(IrcSyncCheckWork *wk, u16 *state) {
    IrcSyncPoint point;
    IrcSyncCheckThumbs *thumbs;
    IrcSyncTrace *trace = &wk->traceB;

    if (IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_SYNC_CHECK_RESULT_CANCELLED;
        IrcSyncCheck_SetStepDone(wk);
    } else if (!IrcAppBar_IsPressedOrDone(wk->button)) {
        switch (*state) {
        case 0:
            if (IrcSyncRect_IsHeld(&sIrcSyncCheckRectSecond, &point)) {
                if (IrcSyncTrace_Update(trace, &sIrcSyncCheckRectSecond)) {
                    IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckCompleteCreateInfrared,
                                                0, 0);
                    GFL_SndSEPlay(SEQ_SE_SYS_21);
                    IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepResult);
                }
                thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
                IrcSyncCheckThumbs_SetAnimation(thumbs, 1, 1);
                IrcSyncCheckThumbs_Place(thumbs, 1, point.x, point.y);
            } else {
                thumbs = IrcSyncCheckGraphic_GetThumbs(&wk->graphic);
                IrcSyncCheckThumbs_SetAnimation(thumbs, 1, 0);
                IrcSyncTrace_Clear(trace);
                *state = 1;
            }
            break;
        case 1:
            wk->retries++;
            IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepWaitFirst);
            break;
        }
    }
    if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepInterrupted);
    }
}

// Sends the traces to the other player, and scores the two
static void IrcSyncCheck_StepResult(IrcSyncCheckWork *wk, u16 *state) {
    IrcSyncData data;
    IrcSyncData received;
    SaveControl *save;
    TrainerLogSave *log;

    if (*state < RESULT_WAIT_CONNECTED && IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_SYNC_CHECK_RESULT_CANCELLED;
        IrcSyncCheck_SetStepDone(wk);
        return;
    }
    switch (*state) {
    case RESULT_EXCHANGE_IDS_FIRST:
        if (!CompatibleIrcSys_ExchangeIds(wk->param->sys, 4)) {
            break;
        }
        *state = RESULT_PRINT_CHECKING;
        break;
    case RESULT_PRINT_CHECKING:
        IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_CheckingPleaseMaintainInfrared, 0, 0);
        *state = RESULT_SEND_TRACES;
        break;
    case RESULT_SEND_TRACES:
        data.first = wk->first;
        data.second = wk->second;
        data.traceA = wk->traceA;
        data.traceB = wk->traceB;
        data.retries = wk->retries;
        if (wk->param->gsys != NULL) {
            save = GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys));
            log = func_0200afbc(save);
            data.isNew = func_0200b0f0(log, getIDAsUInt(&wk->param->profile->trainer)) ? FALSE : TRUE;
        }
        if (!IrcSyncNet_Send(&wk->net, &data)) {
            break;
        }
        *state = RESULT_SEND_COUNT;
        break;
    case RESULT_SEND_COUNT:
        CompatibleIrcSys_SetCount(wk->param->sys, 3);
        if (!CompatibleIrcSys_SendCount(wk->param->sys)) {
            break;
        }
        *state = RESULT_EXCHANGE_IDS_LAST;
        break;
    case RESULT_EXCHANGE_IDS_LAST:
        if (!CompatibleIrcSys_ExchangeIds(wk->param->sys, 5)) {
            break;
        }
        IrcAppBar_SetActive(wk->button, FALSE);
        *state = RESULT_WAIT_CONNECTED;
        break;
    case RESULT_WAIT_CONNECTED:
        if (wk->timer < CONNECTED_FRAMES) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SYS_12);
        IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_YouveCreatedInfraredConnection, 0, 0);
        wk->counter = 0;
        *state = RESULT_WAIT_SCORE;
        break;
    case RESULT_WAIT_SCORE:
        if (wk->counter++ < DONE_FRAMES) {
            return;
        }
        wk->param->unk10 = IrcSyncCheck_ComputeScore(wk);
        IrcSyncNet_GetReceived(&wk->net, &received);
        wk->param->unk11 = wk->retries + received.retries;
        wk->param->unk12 = received.isNew;
        wk->param->result = IRC_SYNC_CHECK_RESULT_DONE;
        *state = RESULT_FINISH;
        return;
    case RESULT_FINISH:
        IrcSyncCheck_SetStepDone(wk);
        return;
    }
    if (*state < RESULT_SEND_COUNT) {
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
            IrcSyncCheck_SetStep(wk, IrcSyncCheck_StepInterrupted);
        }
    }
    if (*state >= RESULT_PRINT_CHECKING && *state <= RESULT_WAIT_CONNECTED && wk->timer <= CONNECTED_FRAMES) {
        wk->timer++;
    }
}

// The link broke: ends the screen
static void IrcSyncCheck_StepInterrupted(IrcSyncCheckWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) > 0) {
            *state = 1;
            return;
        }
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) < 0) {
            wk->param->unk10 = 0;
            wk->param->result = IRC_SYNC_CHECK_RESULT_INTERRUPTED;
            IrcSyncCheck_SetStepDone(wk);
            return;
        }
        break;
    case 1:
        wk->param->unk10 = 0;
        wk->param->result = IRC_SYNC_CHECK_RESULT_INTERRUPTED;
        *state = 2;
        break;
    case 2:
        IrcSyncCheckWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain3_Text_ConnectionInterruptedPleaseCreate, 0,
                                    0);
        *state = 3;
        break;
    case 3:
        if (CompatibleIrcSys_ExchangeIds(wk->param->sys, 6)) {
            *state = 4;
        }
        break;
    case 4:
        IrcSyncCheck_SetStepDone(wk);
        break;
    }
}

static void IrcSyncNet_Init(IrcSyncNet *net, CompatibleIrcSys *sys) {
    sys_memset(net, 0, sizeof(IrcSyncNet));
    net->sys = sys;
    CompatibleIrcSys_AddCommands(sys, NET_COMMAND_BASE, sIrcSyncCheckCommands, NELEMS(sIrcSyncCheckCommands), net);
}

static void IrcSyncNet_Free(IrcSyncNet *net) {
    CompatibleIrcSys_RemoveCommands(net->sys, NET_COMMAND_BASE);
    sys_memset(net, 0, sizeof(IrcSyncNet));
}

// Sends the data, and returns TRUE once the other player's has also arrived
static BOOL IrcSyncNet_Send(IrcSyncNet *net, const IrcSyncData *data) {
    switch (net->state) {
    case 0:
        net->send = *data;
        net->state = 1;
        break;
    case 1:
        if (CompatibleIrcSys_SendEx(net->sys, NET_COMMAND_BASE, sizeof(IrcSyncData), &net->send,
                                    !func_02042a6c(func_02040440()), 0, FALSE, TRUE)) {
            net->state = 2;
        }
        break;
    case 2:
        if (net->received) {
            net->received = FALSE;
            net->state = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void IrcSyncNet_GetReceived(IrcSyncNet *net, IrcSyncData *dest) {
    *dest = net->peer;
}

static void IrcSyncNet_OnReceive(int netId, int size, const void *data, void *work, NetHandle *handle) {
    IrcSyncNet *net = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            net->received = TRUE;
        }
    }
}

static void *IrcSyncNet_GetBuffer(int netId, void *work, int size) {
    IrcSyncNet *net = work;

    if (netId == func_0203ffc4()) {
        return &net->own;
    }
    return &net->peer;
}

static void IrcSyncCheckThumbs_Init(IrcSyncCheckThumbs *thumbs, IrcSyncCheckG3d *g3d, HeapID heapId) {
    int i;

    sys_memset(thumbs, 0, sizeof(IrcSyncCheckThumbs));
    for (i = 0; i < THUMB_COUNT; i++) {
        thumbs->models[i] = IrcSyncCheckG3d_GetModel(g3d, i);
        IrcSyncCheckModel_SetAnimation(thumbs->models[i], 0);
    }
}

static void IrcSyncCheckThumbs_Free(IrcSyncCheckThumbs *thumbs) {
    sys_memset(thumbs, 0, sizeof(IrcSyncCheckThumbs));
}

// Draws the thumbs, with a sound when one has finished its animation
static void IrcSyncCheckThumbs_Update(IrcSyncCheckThumbs *thumbs) {
    int i;

    for (i = 0; i < THUMB_COUNT; i++) {
        IrcSyncCheckModel_Update(thumbs->models[i]);
    }
    if (IrcSyncCheckModel_GetAnimation(thumbs->models[0]) != 0) {
        if (IrcSyncCheckModel_IsAnimationDone(thumbs->models[0])) {
            GFL_SndSEPlay(SEQ_SE_SYS_18);
        }
    }
    if (IrcSyncCheckModel_GetAnimation(thumbs->models[1]) != 0) {
        if (IrcSyncCheckModel_IsAnimationDone(thumbs->models[1])) {
            GFL_SndSEPlay(SEQ_SE_SYS_23);
        }
    }
}

static void IrcSyncCheckThumbs_SetAnimation(IrcSyncCheckThumbs *thumbs, int index, int animation) {
    IrcSyncCheckModel_SetAnimation(thumbs->models[index], animation);
}

// Puts a thumb where the stylus is. The second thumb is further from the first's last place than the stylus is
static void IrcSyncCheckThumbs_Place(IrcSyncCheckThumbs *thumbs, int index, int x, int y) {
    VecFx32 nearPos;
    VecFx32 farPos;
    VecFx32 direction;

    switch (index) {
    case 0:
        func_02067fa0(x, y, &nearPos, &farPos);
        VEC_Subtract(&farPos, &nearPos, &direction);
        VEC_Normalize(&direction, &direction);
        VEC_MultAdd(FX32_CONST(64), &direction, &nearPos, &nearPos);
        IrcSyncCheckModel_SetPosition(thumbs->models[index], &nearPos);
        thumbs->lastX = x;
        thumbs->lastY = y;
        break;
    case 1:
        func_02067fa0(thumbs->lastX + (x - thumbs->lastX) * 2, thumbs->lastY + (y - thumbs->lastY) * 2, &nearPos,
                      &farPos);
        VEC_Subtract(&farPos, &nearPos, &direction);
        VEC_Normalize(&direction, &direction);
        VEC_MultAdd(FX32_CONST(64), &direction, &nearPos, &nearPos);
        IrcSyncCheckModel_SetPosition(thumbs->models[index], &nearPos);
        break;
    }
}

// Whether the stylus was put down inside a rectangle this frame, and where
static BOOL IrcSyncRect_IsPressed(const IrcSyncRect *rect, IrcSyncPoint *point) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y) && x - rect->left < rect->right - rect->left && y - rect->top < rect->bottom - rect->top) {
        if (point != NULL) {
            point->x = x;
            point->y = y;
        }
        return TRUE;
    }
    return FALSE;
}

// Whether the stylus is on the screen inside a rectangle, and where
static BOOL IrcSyncRect_IsHeld(const IrcSyncRect *rect, IrcSyncPoint *point) {
    u32 x;
    u32 y;

    if (func_0203da84(&x, &y) && x - rect->left < rect->right - rect->left && y - rect->top < rect->bottom - rect->top) {
        if (point != NULL) {
            point->x = x;
            point->y = y;
        }
        return TRUE;
    }
    return FALSE;
}

// The score of a Sync Check: how close the first points are, and how alike the traces are
static u8 IrcSyncCheck_ComputeScore(IrcSyncCheckWork *wk) {
    IrcSyncData own;
    IrcSyncData peer;
    u32 distance;
    u32 pointScore;
    u32 traceScoreA;
    u32 traceScoreB;
    IrcSyncPoint ownFirst;
    IrcSyncPoint peerFirst;
    int dx;
    int dy;

    own.first = wk->first;
    own.second = wk->second;
    own.traceA = wk->traceA;
    own.traceB = wk->traceB;
    IrcSyncNet_GetReceived(&wk->net, &peer);
    ownFirst = own.first;
    peerFirst = peer.first;
    dy = ownFirst.y - peerFirst.y;
    dx = ownFirst.x - peerFirst.x;
    distance = dx * dx + dy * dy;
    if (distance != 0 && distance <= 64) {
        pointScore = 100;
    } else if (distance > 64 && distance <= 256) {
        pointScore = 80;
    } else if (distance > 256 && distance <= 576) {
        pointScore = 50;
    } else if (distance > 576 && distance <= 1024) {
        pointScore = 30;
    } else if (distance > 1024 && distance <= 1600) {
        pointScore = 10;
    } else {
        pointScore = 0;
    }
    traceScoreA = IrcSyncCheck_CompareTraces(own.traceA.points, own.traceA.points, peer.traceA.points,
                                             peer.traceA.points);
    traceScoreB = IrcSyncCheck_CompareTraces(own.traceB.points, own.traceB.points, peer.traceB.points,
                                             peer.traceB.points);
    return traceScoreB * 20 / 100 + (pointScore * 40 / 100 + traceScoreA * 40 / 100);
}

// How alike two traces are, by how far each of their points is from the first
static u8 IrcSyncCheck_CompareTraces(const IrcSyncPoint *ownStart, const IrcSyncPoint *own,
                                     const IrcSyncPoint *peerStart, const IrcSyncPoint *peer) {
    int i;
    u32 total = 0;

    for (i = 1; i < TRACE_POINT_COUNT; i++) {
        int ownDx = own[i].x - ownStart->x;
        int ownDy = own[i].y - ownStart->y;
        int peerDx = peer[i].x - peerStart->x;
        int peerDy = peer[i].y - peerStart->y;
        u32 ownDistance;
        u32 peerDistance;
        u16 difference;

        ownDistance = (ownDx < 0 ? -ownDx : ownDx) + (ownDy < 0 ? -ownDy : ownDy);
        peerDistance = (peerDx < 0 ? -peerDx : peerDx) + (peerDy < 0 ? -peerDy : peerDy);
        if (ownDistance > peerDistance) {
            difference = ownDistance - peerDistance;
        } else {
            difference = peerDistance - ownDistance;
        }
        if (difference == 0) {
            total += 100;
        } else if (difference == 1) {
            total += 80;
        } else if (difference == 2) {
            total += 50;
        } else if (difference >= 3 && difference <= 5) {
            total += 30;
        }
    }
    return total / 9;
}

static void IrcSyncCheck_LoadScreenArea(ArcTool *arc, u32 fileId, u32 bg, u32 tileOffset, u8 srcX, u8 srcY,
                                        u8 srcWidth, u8 srcHeight, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                        BOOL compressed, HeapID heapId) {
    NNSG2dScreenData *screen;
    void *data;
    int i;

    data = GFL_ArcToolReadHeapNewLZ(arc, fileId, compressed, HEAPID_TAIL(heapId));
    NNS_G2dGetUnpackedScreenData(data, &screen);
    if (tileOffset != 0 && GFL_BGSysGetBGColorPaletteMode(bg) == 0) {
        u16 *tiles = (u16 *)screen->rawData;

        for (i = 0; i < srcWidth * srcHeight; i++) {
            tiles[i] += tileOffset;
        }
    }
    if (GFL_BGSysIsScrHeapExists(bg)) {
        GFL_BGSysLoadScrArea(bg, x, y, width, height, screen->rawData, srcX, srcY, srcWidth, srcHeight);
        GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
        GFL_BGSysLoadScr(bg);
    }
    GFL_HeapFree(data);
}

// Shows the outlines that a mask has: the first and the second
static void IrcSyncCheck_ShowOutlines(u32 outlines, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);

    if (outlines & OUTLINE_FIRST) {
        IrcSyncCheck_LoadScreenArea(arc, 7, 2, 0, 0, 0, 16, 24, 0, 0, 16, 24, 9, FALSE, heapId);
    } else {
        GFL_BGSysFillScrArea(2, 0, 0, 0, 16, 24, 9);
    }
    if (outlines & OUTLINE_SECOND) {
        IrcSyncCheck_LoadScreenArea(arc, 8, 2, 0, 0, 0, 16, 24, 16, 0, 16, 24, 9, FALSE, heapId);
    } else {
        GFL_BGSysFillScrArea(2, 0, 16, 0, 16, 24, 9);
    }
    GFL_BGSysQueueScrLoad(2);
    GFL_ArcToolFree(arc);
}

static void IrcSyncCheckSparkles_Init(IrcSyncCheckSparkles *sparkles, IrcSyncCheckGraphic *graphic, int unk580,
                                      u32 palette, int firstActor, u32 value) {
    int i;

    sys_memset(sparkles, 0, sizeof(IrcSyncCheckSparkles));
    sparkles->unk580 = unk580;
    sparkles->palette = palette;
    sparkles->timer = 0;
    sparkles->interval = 10;
    for (i = 0; i < SPARKLE_COUNT; i++) {
        IrcSyncCheckSparkle_Init(&sparkles->sparkles[i], IrcSyncCheckGraphic_GetActor(graphic, firstActor + i),
                                 palette, value);
    }
}

static void IrcSyncCheckSparkles_Free(IrcSyncCheckSparkles *sparkles) {
    sys_memset(sparkles, 0, sizeof(IrcSyncCheckSparkles));
}

static void IrcSyncCheckSparkle_Init(IrcSyncCheckSparkle *sparkle, ClActor *actor, u32 palette, u32 value) {
    sys_memset(sparkle, 0, sizeof(IrcSyncCheckSparkle));
    sparkle->actor = actor;
    sparkle->value = value;
    func_0204c378(actor, (u8)palette, TRUE);
}

static void IrcSyncTrace_Clear(IrcSyncTrace *trace) {
    sys_memset(trace, 0, sizeof(IrcSyncTrace));
}

// Takes a point of the trace every few frames while a thumb is held, and returns TRUE once it is done
static BOOL IrcSyncTrace_Update(IrcSyncTrace *trace, const IrcSyncRect *rect) {
    if (trace->done) {
        return TRUE;
    }
    if (trace->frames++ > TRACE_FRAMES) {
        IrcSyncRect_IsHeld(rect, &trace->points[TRACE_POINT_COUNT - 1]);
        trace->done = TRUE;
        return TRUE;
    }
    if (trace->phase == 0) {
        IrcSyncRect_IsHeld(rect, &trace->points[trace->index++]);
    }
    if (trace->phase++ > TRACE_INTERVAL - 1) {
        trace->phase = 0;
    }
    return FALSE;
}

static BOOL IrcSyncTrace_IsDone(const IrcSyncTrace *trace) {
    return trace->done;
}
