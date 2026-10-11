#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/irc_appbar.h"
#include "app/irc_compatible.h"
#include "app/irc_rhythm_check.h"
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
#include "nitro/math.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_7.h"

// The Rhythm Check of the compatibility checker: its graphics, its windows, the countdown, the sparkles of the taps,
// the flash of the background, the taps that the two players compare, the link that sends them and the steps of the
// screen. The ROM has no string for the file, so the name is a guess. Names are ours

#define HEAP_ID 0x38
#define BG_COUNT 7
#define CLACT_COUNT 35
#define PARTICLE_COUNT 30
#define COUNTDOWN_ACTOR 31

// The command that sends a player's taps to the other
#define NET_COMMAND_BASE 0xf00

// The taps of a player: ten are kept, and the taps after them are only counted
#define TAP_CAPACITY 15
#define TAP_COUNT 10
#define EXTRA_TAP_MAX 99

// The area of the touch screen that counts as a tap
#define TOUCH_MAX_X 256
#define TOUCH_MAX_Y 168

#define COUNTDOWN_FRAME_COUNT 60
#define COUNTDOWN_COUNT 3
#define CONNECTED_FRAMES 60
#define SCORE_FRAMES 30

// How well a tap kept the rhythm of the two before it, which picks the color of its sparkles
enum {
    TAP_GRADE_EARLY_FAR,
    TAP_GRADE_EARLY,
    TAP_GRADE_UNUSED,
    TAP_GRADE_ON_BEAT,
    TAP_GRADE_UNUSED_4,
    TAP_GRADE_LATE,
    TAP_GRADE_LATE_FAR,
};

// What the sparkles of the system do by themselves
enum {
    PARTICLES_RISE,
    PARTICLES_TAPS_ONLY,
    PARTICLES_CONVERGE,
};

// The steps of IrcRhythmCheck_StepStart
enum {
    START_BEGIN,
    START_HIDE_BG,
    START_UNUSED,
    START_COUNTDOWN,
};

// The steps of IrcRhythmCheck_StepResult
enum {
    RESULT_EXCHANGE_IDS_FIRST,
    RESULT_PRINT_CHECKING,
    RESULT_SEND_TAPS,
    RESULT_SEND_COUNT,
    RESULT_EXCHANGE_IDS_LAST,
    RESULT_WAIT_CONNECTED,
    RESULT_WAIT_SCORE,
    RESULT_FINISH,
};

typedef struct {
    int x;
    int y;
} IrcRhythmPoint;

static inline void IrcRhythmPoint_Set(IrcRhythmPoint *point, int x, int y) {
    point->x = x;
    point->y = y;
}

// A tap: when it was, and how long after the one before
typedef struct {
    int time;
    int interval;
} IrcRhythmTap;

// What a player sends the other
typedef struct {
    IrcRhythmTap taps[TAP_CAPACITY];
    u16 count;
    u16 extraTaps;
    u64 start;
} IrcRhythmData;

typedef struct {
    CompatibleIrcSys *sys;
    int state;
    IrcRhythmData peer;
    IrcRhythmData own;
    IrcRhythmData send;
    BOOL received;
} IrcRhythmNet;

typedef struct {
    ClActUnit *unit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 subPalette;
    u32 subChars;
    u32 subCellAnims;
    u32 countdownPalette;
    u32 countdownChars;
    u32 countdownCellAnims;
    ClActor *actors[CLACT_COUNT];
} IrcRhythmClAct;

typedef struct {
    // The characters of the window frame
    u32 frameChars;
} IrcRhythmBG;

typedef struct {
    IrcRhythmBG bg;
    IrcRhythmClAct clact;
    TCB *vblankTask;
} IrcRhythmGraphic;

typedef struct {
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    WordSet *wordSet;
} IrcRhythmText;

typedef struct {
    BmpWin *window;
    PrintWindow printWindow;
    StrBuf *strbuf;
    u16 color;
} IrcRhythmWindow;

// A sparkle that flies from a point to another
typedef struct {
    ClActor *actor;
    u32 surface;
    BOOL active;
    VecFx32 start;
    VecFx32 end;
    u32 frame;
    u32 duration;
} IrcRhythmParticle;

typedef struct {
    IrcRhythmParticle particles[PARTICLE_COUNT];
    int mode;
    u32 timer;
    u32 interval;
} IrcRhythmParticles;

// The 3, 2, 1 before the taps
typedef struct {
    ClActor *actor;
    u32 unk4;
    u32 unk8;
    u32 frame;
    u32 count;
    BOOL active;
    BOOL done;
} IrcRhythmCountdown;

// The color of palette 5 of BG 0 that swings between two colors while the taps last
typedef struct {
    u16 color;
    u16 angle;
    BOOL enabled;
    u32 unk8;
    BOOL active;
} IrcRhythmFlash;

typedef struct IrcRhythmCheckWork IrcRhythmCheckWork;
typedef void (*IrcRhythmStepFunc)(IrcRhythmCheckWork *wk, u16 *state);

struct IrcRhythmCheckWork {
    IrcRhythmGraphic graphic;
    IrcRhythmText text;
    // The message
    IrcRhythmWindow messageWindow;
    IrcRhythmNet net;
    IrcRhythmParticles particles;
    IrcAppBar *button;
    IrcRhythmCountdown countdown;
    IrcRhythmFlash flash;
    // The title
    IrcRhythmWindow titleWindow;
    IrcRhythmWindow hintWindow;
    IrcRhythmStepFunc step;
    u16 stepState;
    u16 timer;
    BOOL stepDone;
    u32 unk810;
    IrcRhythmData data;
    u32 unk898;
    // Whether the taps are done and are being scored
    BOOL scoring;
    u16 resultState;
    u16 resultTimer;
    TCB *vblankTask;
    BOOL clearBlend;
    // Whether a tap was made this frame
    BOOL tapped;
    IrcCompatibleParam2 *param;
};

static BOOL IrcRhythmCheck_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcRhythmCheck_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcRhythmCheck_Main(GameProc *proc, u32 *state, void *param, void *work);
static void IrcRhythmGraphic_Create(IrcRhythmGraphic *graphic, HeapID heapId);
static void IrcRhythmGraphic_Free(IrcRhythmGraphic *graphic);
static void IrcRhythmGraphic_Update(IrcRhythmGraphic *graphic);
static ClActor *IrcRhythmGraphic_GetActor(IrcRhythmGraphic *graphic, int index);
static ClActUnit *IrcRhythmGraphic_GetUnit(IrcRhythmGraphic *graphic);
static void IrcRhythmGraphic_VBlank(TCB *tcb, void *data);
static void IrcRhythmBG_Init(IrcRhythmBG *bg, HeapID heapId);
static void IrcRhythmBG_Free(IrcRhythmBG *bg);
static void IrcRhythmBG_VBlank(IrcRhythmBG *bg);
static void IrcRhythmClAct_Init(IrcRhythmClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IrcRhythmClAct_Free(IrcRhythmClAct *clact);
static void IrcRhythmClAct_Update(IrcRhythmClAct *clact);
static void IrcRhythmClAct_VBlank(IrcRhythmClAct *clact);
static ClActor *IrcRhythmClAct_GetActor(IrcRhythmClAct *clact, int index);
static ClActUnit *IrcRhythmClAct_GetUnit(IrcRhythmClAct *clact);
static void IrcRhythmText_Init(IrcRhythmText *text, int fontType, HeapID heapId);
static void IrcRhythmText_Free(IrcRhythmText *text);
static BOOL IrcRhythmText_Update(IrcRhythmText *text);
static Font *IrcRhythmText_GetFont(IrcRhythmText *text);
static PrintQueue *IrcRhythmText_GetQueue(IrcRhythmText *text);
static MsgData *IrcRhythmText_GetMsgData(IrcRhythmText *text);
static WordSet *IrcRhythmText_GetWordSet(IrcRhythmText *text);
static void IrcRhythmWindow_Init(IrcRhythmWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                 HeapID heapId);
static void IrcRhythmWindow_InitWithColor(IrcRhythmWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                          u16 color, u8 fromEnd, HeapID heapId);
static void IrcRhythmWindow_Free(IrcRhythmWindow *win);
static BOOL IrcRhythmWindow_Flush(IrcRhythmWindow *win, IrcRhythmText *text);
static void IrcRhythmWindow_PrintMsg(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId, int x, s16 y);
static void IrcRhythmWindow_PrintMsgCentered(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId);
static void IrcRhythmWindow_PrintMsgCenteredColor(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId, u16 color);
static void IrcRhythmWindow_PrintMsgPlayer(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId,
                                           CompatibleIrcProfile *other, s16 x, s16 y, HeapID heapId);
static void IrcRhythmWindow_Clear(IrcRhythmWindow *win);
static void IrcRhythmCountdown_Init(IrcRhythmCountdown *countdown, IrcRhythmGraphic *graphic, HeapID heapId);
static void IrcRhythmCountdown_Free(IrcRhythmCountdown *countdown);
static void IrcRhythmCountdown_Update(IrcRhythmCountdown *countdown);
static void IrcRhythmCountdown_Start(IrcRhythmCountdown *countdown);
static BOOL IrcRhythmCountdown_IsDone(IrcRhythmCountdown *countdown);
static void IrcRhythmCheck_SetStep(IrcRhythmCheckWork *wk, IrcRhythmStepFunc step);
static void IrcRhythmCheck_SetStepDone(IrcRhythmCheckWork *wk);
static void IrcRhythmCheck_StepStart(IrcRhythmCheckWork *wk, u16 *state);
static void IrcRhythmCheck_StepCountdown(IrcRhythmCheckWork *wk, u16 *state);
static void IrcRhythmCheck_StepTap(IrcRhythmCheckWork *wk, u16 *state);
static void IrcRhythmCheck_StepResult(IrcRhythmCheckWork *wk, u16 *state);
static void IrcRhythmCheck_StepInterrupted(IrcRhythmCheckWork *wk, u16 *state);
static void IrcRhythmData_Clear(IrcRhythmData *data);
static void IrcRhythmData_Start(IrcRhythmData *data);
static BOOL IrcRhythmData_IsFull(IrcRhythmData *data);
static void IrcRhythmData_AddTap(IrcRhythmData *data);
static int IrcRhythmData_GetTapGrade(IrcRhythmData *data);
static void IrcRhythmNet_Init(IrcRhythmNet *net, CompatibleIrcSys *sys);
static void IrcRhythmNet_Free(IrcRhythmNet *net);
static BOOL IrcRhythmNet_Send(IrcRhythmNet *net, const IrcRhythmData *data);
static void IrcRhythmNet_GetReceived(IrcRhythmNet *net, IrcRhythmData *dest);
static void IrcRhythmNet_OnReceive(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *IrcRhythmNet_GetBuffer(int netId, void *work, int size);
static BOOL IrcRhythm_GetTouch(IrcRhythmPoint *point);
static u8 IrcRhythmCheck_ComputeScore(IrcRhythmCheckWork *wk);
static void IrcRhythm_BlendColor(NNS_GFD_DST_TYPE type, u16 *dest, int angle, u32 palette, u8 index, u16 from, u16 to);
static void IrcRhythmParticles_Init(IrcRhythmParticles *particles, IrcRhythmGraphic *graphic, int mode, int firstActor,
                                    u32 surface);
static void IrcRhythmParticles_Free(IrcRhythmParticles *particles);
static void IrcRhythmParticles_Update(IrcRhythmParticles *particles);
static void IrcRhythmParticles_Burst(IrcRhythmParticles *particles, const IrcRhythmPoint *origin, int palette);
static void IrcRhythmParticle_Init(IrcRhythmParticle *particle, ClActor *actor, u32 surface);
static void IrcRhythmParticle_Update(IrcRhythmParticle *particle);
static void IrcRhythmParticle_Start(IrcRhythmParticle *particle, const IrcRhythmPoint *from, const IrcRhythmPoint *to,
                                    int palette, u32 duration);
static BOOL IrcRhythmParticle_IsActive(IrcRhythmParticle *particle);
static void IrcRhythmFlash_Init(IrcRhythmFlash *flash, HeapID heapId);
static void IrcRhythmFlash_Free(IrcRhythmFlash *flash);
static void IrcRhythmFlash_Update(IrcRhythmFlash *flash);
static void IrcRhythmFlash_Start(IrcRhythmFlash *flash);
static void IrcRhythmCheck_VBlank(TCB *tcb, void *data);

// Nothing in the overlay or in another refers to it, and it is the first of the overlay's read-only data
const u32 IRC_RHYTHM_CHECK_UNUSED_400 = 0x400;

static const NetCommand sIrcRhythmCommands[1] = {
    { IrcRhythmNet_OnReceive, IrcRhythmNet_GetBuffer },
};

static const BGSysLCDConfig sIrcRhythmLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_1, GX_BG0_AS_2D };

static const u32 sIrcRhythmBGModes[BG_COUNT] = { BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT,
                                                 BGMODE_TEXT, BGMODE_TEXT, BGMODE_AFFINE };

static const u32 sIrcRhythmBGs[BG_COUNT] = { 0, 1, 2, 4, 5, 6, 7 };

static const BGSysVRAMConfig sIrcRhythmVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

const GameProcFunctions IRC_RHYTHM_CHECK_PROC_FUNCTIONS = {
    IrcRhythmCheck_Init,
    IrcRhythmCheck_Main,
    IrcRhythmCheck_Exit,
};

static const BGSetup sIrcRhythmBGSetups[BG_COUNT] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x18000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x4000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL IrcRhythmCheck_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRhythmCheckWork *wk;
    IrcCompatibleParam2 *rhythmParam = param;
    ClActUnit *unit;
    Font *font;
    PrintQueue *queue;

    GFL_HeapCreateChild(HEAPID_USER, HEAP_ID, 0x20000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcRhythmCheckWork), HEAP_ID);
    sys_memset(wk, 0, sizeof(IrcRhythmCheckWork));
    wk->param = rhythmParam;
    IrcRhythmGraphic_Create(&wk->graphic, HEAP_ID);
    IrcRhythmText_Init(&wk->text, 0, HEAP_ID);
    if (wk->param->gsys != NULL) {
        GSYS_GetGameCommSystem(wk->param->gsys);
    }
    IrcRhythmWindow_Init(&wk->messageWindow, 5, 1, 18, 30, 5, 8, HEAP_ID);
    BmpWin_DrawFrame(wk->messageWindow.window, 0, (u16)wk->graphic.bg.frameChars, 6);
    IrcRhythmFlash_Init(&wk->flash, HEAP_ID);
    if (wk->param->gsys != NULL) {
        IrcRhythmWindow_PrintMsgPlayer(&wk->messageWindow, &wk->text, BtlMain7_Text_TapTouchScreen10,
                                       wk->param->profile, 0, 0, HEAP_ID);
    } else {
        IrcRhythmWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain7_Text_TapTouchScreen10, 0, 0);
    }
    IrcRhythmCountdown_Init(&wk->countdown, &wk->graphic, HEAP_ID);
    IrcRhythmWindow_InitWithColor(&wk->titleWindow, 7, 6, 3, 20, 5, 8, 1, TRUE, HEAP_ID);
    IrcRhythmWindow_PrintMsgCentered(&wk->titleWindow, &wk->text, BtlMain7_Text_RhythmCheck);
    GFL_BGSysScaleBGReq(7, 3, 0x1000);
    GFL_BGSysScaleBGReq(7, 6, 0x1000);
    GFL_BGSysAdjustBGOriginReq(7, 9, 0x80);
    GFL_BGSysAdjustBGOriginReq(7, 12, 0x28);
    IrcRhythmWindow_InitWithColor(&wk->hintWindow, 2, 8, 10, 16, 2, 13, 0, FALSE, HEAP_ID);
    IrcRhythmNet_Init(&wk->net, wk->param->sys);
    IrcRhythmData_Clear(&wk->data);
    IrcRhythmParticles_Init(&wk->particles, &wk->graphic, PARTICLES_TAPS_ONLY, 0, 0);
    unit = IrcRhythmGraphic_GetUnit(&wk->graphic);
    font = IrcRhythmText_GetFont(&wk->text);
    queue = IrcRhythmText_GetQueue(&wk->text);
    wk->button = IrcAppBar_Create(2, unit, 0, 14, 13, 2, font, queue, HEAP_ID);
    CompatibleIrcSys_SetCount(wk->param->sys, 1);
    gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0x22, -8);
    wk->vblankTask = GFL_VBlankTCBAdd(IrcRhythmCheck_VBlank, wk, 0);
    IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepStart);
    return TRUE;
}

static BOOL IrcRhythmCheck_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRhythmCheckWork *wk = work;

    GFL_TCBRemove(wk->vblankTask);
    IrcRhythmParticles_Free(&wk->particles);
    G2_BlendNone();
    IrcAppBar_Free(wk->button);
    IrcRhythmNet_Free(&wk->net);
    IrcRhythmCountdown_Free(&wk->countdown);
    IrcRhythmFlash_Free(&wk->flash);
    IrcRhythmWindow_Free(&wk->hintWindow);
    IrcRhythmWindow_Free(&wk->titleWindow);
    IrcRhythmWindow_Free(&wk->messageWindow);
    IrcRhythmGraphic_Free(&wk->graphic);
    IrcRhythmText_Free(&wk->text);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAP_ID);
    return TRUE;
}

static BOOL IrcRhythmCheck_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRhythmCheckWork *wk = work;

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
        IrcRhythmFlash_Update(&wk->flash);
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
    if (IrcRhythmText_Update(&wk->text)) {
        IrcRhythmWindow_Flush(&wk->messageWindow, &wk->text);
        IrcRhythmWindow_Flush(&wk->titleWindow, &wk->text);
        IrcRhythmWindow_Flush(&wk->hintWindow, &wk->text);
    }
    IrcRhythmGraphic_Update(&wk->graphic);
    IrcRhythmParticles_Update(&wk->particles);
    if (!wk->tapped) {
        IrcAppBar_Update(wk->button);
    }
    CompatibleIrcSys_Update(wk->param->sys);
    return FALSE;
}

static void IrcRhythmGraphic_Create(IrcRhythmGraphic *graphic, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(IrcRhythmGraphic));
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sIrcRhythmVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    IrcRhythmClAct_Init(&graphic->clact, &sIrcRhythmVRAMConfig, heapId);
    IrcRhythmBG_Init(&graphic->bg, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(IrcRhythmGraphic_VBlank, graphic, 0);
}

static void IrcRhythmGraphic_Free(IrcRhythmGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IrcRhythmBG_Free(&graphic->bg);
    IrcRhythmClAct_Free(&graphic->clact);
}

static void IrcRhythmGraphic_Update(IrcRhythmGraphic *graphic) {
    IrcRhythmClAct_Update(&graphic->clact);
}

static ClActor *IrcRhythmGraphic_GetActor(IrcRhythmGraphic *graphic, int index) {
    return IrcRhythmClAct_GetActor(&graphic->clact, index);
}

static ClActUnit *IrcRhythmGraphic_GetUnit(IrcRhythmGraphic *graphic) {
    return IrcRhythmClAct_GetUnit(&graphic->clact);
}

static void IrcRhythmGraphic_VBlank(TCB *tcb, void *data) {
    IrcRhythmGraphic *graphic = data;

    IrcRhythmBG_VBlank(&graphic->bg);
    IrcRhythmClAct_VBlank(&graphic->clact);
}

static void IrcRhythmBG_Init(IrcRhythmBG *bg, HeapID heapId) {
    ArcTool *arc;
    int i;

    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sIrcRhythmLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysCreateBG(sIrcRhythmBGs[i], &sIrcRhythmBGSetups[i], sIrcRhythmBGModes[i]);
        GFL_BGSysClearBG(sIrcRhythmBGs[i]);
        GFL_BGSysSetBGEnabled(sIrcRhythmBGs[i], TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 36, 1, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 48, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 37, 1, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 47, 4, 0, 0, FALSE, heapId);
    GFL_BGSysFillChar(5, 0, 1, 0);
    bg->frameChars = GFL_BGSysLoadArcNCGRDynamic(arc, 49, 5, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x1a0, 0x20, heapId);
    GFL_BGSysFillChar(2, 0, 1, 0);
}

static void IrcRhythmBG_Free(IrcRhythmBG *bg) {
    int i;

    GFL_BGSysFreeFilledChar(2, 1, 0);
    GFL_BGSysFreeCharMemory(5, (u16)bg->frameChars, (u16)(bg->frameChars >> 16));
    GFL_BGSysFreeFilledChar(5, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sIrcRhythmBGs[i]);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void IrcRhythmBG_VBlank(IrcRhythmBG *bg) {
    GFL_BGSysUpdate();
}

static void IrcRhythmClAct_Init(IrcRhythmClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;
    int i;

    sys_memset(clact, 0, sizeof(IrcRhythmClAct));
    ClActSys_Create(&data_02093f08, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    clact->palette = func_0204bba0(arc, 35, CLACT_VRAM_MAIN, 0, heapId);
    clact->chars = func_0204b81c(arc, 34, FALSE, CLACT_VRAM_MAIN, heapId);
    clact->cellAnims = func_0204bde0(arc, 33, 32, heapId);
    clact->countdownPalette = func_0204bbb8(arc, 12, CLACT_VRAM_MAIN, 0xe0, 12, 1, heapId);
    clact->countdownChars = func_0204b81c(arc, 11, FALSE, CLACT_VRAM_MAIN, heapId);
    clact->countdownCellAnims = func_0204bde0(arc, 10, 9, heapId);
    clact->subPalette = func_0204bba0(arc, 35, CLACT_VRAM_SUB, 0, heapId);
    clact->subChars = func_0204b81c(arc, 34, FALSE, CLACT_VRAM_SUB, heapId);
    clact->subCellAnims = func_0204bde0(arc, 33, 32, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    for (i = 0; i < PARTICLE_COUNT; i++) {
        clact->actors[i] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup,
                                         CLACT_SURFACE_MAIN, heapId);
        func_0204c124(clact->actors[i], FALSE);
        func_0204c468(clact->actors[i], 3);
        func_0204c520(clact->actors[i], TRUE);
        func_0204c488(clact->actors[i], 0);
    }
    setup.x = 128;
    setup.y = 96;
    for (i = COUNTDOWN_ACTOR; i < COUNTDOWN_ACTOR + 3; i++) {
        setup.sequence = i - 30;
        clact->actors[i] = func_0204c040(clact->unit, clact->countdownChars, clact->countdownPalette,
                                         clact->countdownCellAnims, &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c124(clact->actors[i], FALSE);
        func_0204c378(clact->actors[i], 0, TRUE);
    }
}

static void IrcRhythmClAct_Free(IrcRhythmClAct *clact) {
    int i;

    for (i = 0; i < CLACT_COUNT; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204bcd0(clact->palette);
    func_0204b98c(clact->chars);
    func_0204be64(clact->cellAnims);
    func_0204bcd0(clact->subPalette);
    func_0204b98c(clact->subChars);
    func_0204be64(clact->subCellAnims);
    func_0204bcd0(clact->countdownPalette);
    func_0204b98c(clact->countdownChars);
    func_0204be64(clact->countdownCellAnims);
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(IrcRhythmClAct));
}

static void IrcRhythmClAct_Update(IrcRhythmClAct *clact) {
    func_0204b794();
}

static void IrcRhythmClAct_VBlank(IrcRhythmClAct *clact) {
    func_0204b7c8();
}

static ClActor *IrcRhythmClAct_GetActor(IrcRhythmClAct *clact, int index) {
    return clact->actors[index];
}

static ClActUnit *IrcRhythmClAct_GetUnit(IrcRhythmClAct *clact) {
    return clact->unit;
}

static void IrcRhythmText_Init(IrcRhythmText *text, int fontType, HeapID heapId) {
    sys_memset(text, 0, sizeof(IrcRhythmText));
    func_020232d0();
    switch (fontType) {
    case 0:
        text->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
        break;
    case 1:
        text->font = GFL_FontCreate(ARCID_FONT, 1, 0, FALSE, heapId);
        break;
    }
    text->queue = func_02021998(heapId);
    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_7, heapId);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x1c0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x1c0, 0x20, heapId);
}

static void IrcRhythmText_Free(IrcRhythmText *text) {
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
    func_02021a18(text->queue);
    GFL_FontFree(text->font);
    sys_memset(text, 0, sizeof(IrcRhythmText));
}

static BOOL IrcRhythmText_Update(IrcRhythmText *text) {
    return func_02021a3c(text->queue);
}

static Font *IrcRhythmText_GetFont(IrcRhythmText *text) {
    return text->font;
}

static PrintQueue *IrcRhythmText_GetQueue(IrcRhythmText *text) {
    return text->queue;
}

static MsgData *IrcRhythmText_GetMsgData(IrcRhythmText *text) {
    return text->msgData;
}

static WordSet *IrcRhythmText_GetWordSet(IrcRhythmText *text) {
    return text->wordSet;
}

static void IrcRhythmWindow_Init(IrcRhythmWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                 HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcRhythmWindow));
    win->color = 15;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    IrcRhythmWindow_Clear(win);
    BmpWin_TransferNow(win->window);
}

static void IrcRhythmWindow_InitWithColor(IrcRhythmWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                          u16 color, u8 fromEnd, HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcRhythmWindow));
    win->color = color;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, fromEnd);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcRhythmWindow_Free(IrcRhythmWindow *win) {
    GFL_StrBufFree(win->strbuf);
    BmpWin_Free(win->window);
    sys_memset(win, 0, sizeof(IrcRhythmWindow));
}

static BOOL IrcRhythmWindow_Flush(IrcRhythmWindow *win, IrcRhythmText *text) {
    PrintWindow_Flush(&win->printWindow, text->queue);
    return PrintWindow_IsPrinted(&win->printWindow);
}

static void IrcRhythmWindow_PrintMsg(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId, int x, s16 y) {
    MsgData *msgData = IrcRhythmText_GetMsgData(text);
    PrintQueue *queue = IrcRhythmText_GetQueue(text);
    Font *font = IrcRhythmText_GetFont(text);

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

static void IrcRhythmWindow_PrintMsgCentered(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId) {
    IrcRhythmWindow_PrintMsgCenteredColor(win, text, msgId, PRINT_COLOR(15, 14, 4));
}

static void IrcRhythmWindow_PrintMsgCenteredColor(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId, u16 color) {
    MsgData *msgData = IrcRhythmText_GetMsgData(text);
    PrintQueue *queue = IrcRhythmText_GetQueue(text);
    Font *font = IrcRhythmText_GetFont(text);
    u16 x;
    u16 y;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    x = BmpWin_GetSizeX(win->window) * 4;
    y = BmpWin_GetSizeY(win->window) * 4;
    x -= (u16)(GFL_FontGetBlockWidth(win->strbuf, font, 0) / 2);
    y -= (u16)(GFL_FontGetBlockHeight(win->strbuf, font) / 2);
    PrintWindow_Print(&win->printWindow, queue, x, y, win->strbuf, font, color);
}

static void IrcRhythmWindow_PrintMsgPlayer(IrcRhythmWindow *win, IrcRhythmText *text, u32 msgId,
                                           CompatibleIrcProfile *other, s16 x, s16 y, HeapID heapId) {
    WordSet *wordSet;
    PrintQueue *queue;
    Font *font;
    MsgData *msgData;
    StrBuf *format;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    wordSet = IrcRhythmText_GetWordSet(text);
    msgData = IrcRhythmText_GetMsgData(text);
    copyVarForText(wordSet, 0, &other->trainer);
    format = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    GFL_WordSetFormatStrbuf(wordSet, win->strbuf, format);
    GFL_StrBufFree(format);
    queue = IrcRhythmText_GetQueue(text);
    font = IrcRhythmText_GetFont(text);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

static void IrcRhythmWindow_Clear(IrcRhythmWindow *win) {
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_FlushChar(win->window);
}

static void IrcRhythmCountdown_Init(IrcRhythmCountdown *countdown, IrcRhythmGraphic *graphic, HeapID heapId) {
    int i;

    sys_memset(countdown, 0, sizeof(IrcRhythmCountdown));
    for (i = 0; i < COUNTDOWN_COUNT; i++) {
        countdown->actor = IrcRhythmGraphic_GetActor(graphic, COUNTDOWN_ACTOR + i);
    }
}

static void IrcRhythmCountdown_Free(IrcRhythmCountdown *countdown) {
    sys_memset(countdown, 0, sizeof(IrcRhythmCountdown));
}

static void IrcRhythmCountdown_Update(IrcRhythmCountdown *countdown) {
    if (countdown->active) {
        if (countdown->frame == 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_43);
            func_0204c124(countdown->actor, TRUE);
            func_0204c488(countdown->actor, 4 - countdown->count);
        }
        if (countdown->frame++ >= COUNTDOWN_FRAME_COUNT) {
            countdown->frame = 0;
            countdown->count++;
        }
        if (countdown->count == COUNTDOWN_COUNT) {
            GFL_SndSEPlay(SEQ_SE_SYS_44);
            countdown->done = TRUE;
            countdown->active = FALSE;
            func_0204c124(countdown->actor, FALSE);
        }
    }
}

static void IrcRhythmCountdown_Start(IrcRhythmCountdown *countdown) {
    countdown->frame = 0;
    countdown->count = 0;
    countdown->active = TRUE;
}

static BOOL IrcRhythmCountdown_IsDone(IrcRhythmCountdown *countdown) {
    return countdown->done;
}

static void IrcRhythmCheck_SetStep(IrcRhythmCheckWork *wk, IrcRhythmStepFunc step) {
    wk->step = step;
    wk->stepState = 0;
}

static void IrcRhythmCheck_SetStepDone(IrcRhythmCheckWork *wk) {
    wk->stepDone = TRUE;
}

// Waits for the screen to appear, and starts the countdown
static void IrcRhythmCheck_StepStart(IrcRhythmCheckWork *wk, u16 *state) {
    switch (*state) {
    case START_BEGIN:
        wk->timer = 0;
        *state = START_HIDE_BG;
        break;
    case START_HIDE_BG:
        GFL_BGSysSetBGEnabled(2, FALSE);
        *state = START_COUNTDOWN;
        break;
    case START_COUNTDOWN:
        IrcRhythmCountdown_Start(&wk->countdown);
        IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepCountdown);
        break;
    }
    if (IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_RHYTHM_CHECK_RESULT_CANCELLED;
        IrcRhythmCheck_SetStepDone(wk);
    }
    if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepInterrupted);
    }
}

static void IrcRhythmCheck_StepCountdown(IrcRhythmCheckWork *wk, u16 *state) {
    IrcRhythmCountdown_Update(&wk->countdown);
    if (IrcRhythmCountdown_IsDone(&wk->countdown)) {
        wk->clearBlend = TRUE;
        IrcRhythmData_Start(&wk->data);
        IrcRhythmFlash_Start(&wk->flash);
        IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepTap);
    }
    if (IrcAppBar_GetResult(wk->button) == 1) {
        wk->param->result = IRC_RHYTHM_CHECK_RESULT_CANCELLED;
        IrcRhythmCheck_SetStepDone(wk);
    }
    if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepInterrupted);
    }
}

// Takes the taps, and scores them once there are ten
static void IrcRhythmCheck_StepTap(IrcRhythmCheckWork *wk, u16 *state) {
    IrcRhythmPoint touch;
    IrcRhythmData *data = &wk->data;

    wk->tapped = FALSE;
    if (!IrcAppBar_IsPressedOrDone(wk->button)) {
        if (IrcRhythm_GetTouch(&touch)) {
            int grade = IrcRhythmData_GetTapGrade(data);

            IrcRhythmParticles_Burst(&wk->particles, &touch, grade);
            if (wk->resultState == 0) {
                IrcRhythmData_AddTap(data);
            }
            wk->tapped = TRUE;
        }
    }
    if (IrcRhythmData_IsFull(data)) {
        if (!wk->scoring) {
            IrcRhythmWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain7_Text_CheckCompleteCreateInfrared, 0, 0);
            GFL_SndSEPlay(SEQ_SE_SYS_21);
            wk->scoring = TRUE;
        }
        if (wk->scoring) {
            IrcRhythmCheck_StepResult(wk, &wk->resultState);
        }
    }
    if (!wk->scoring) {
        if (IrcAppBar_GetResult(wk->button) == 1) {
            wk->param->result = IRC_RHYTHM_CHECK_RESULT_CANCELLED;
            IrcRhythmCheck_SetStepDone(wk);
        }
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
            IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepInterrupted);
        }
    }
}

// Sends the taps to the other player, and scores the two
static void IrcRhythmCheck_StepResult(IrcRhythmCheckWork *wk, u16 *state) {
    IrcRhythmData received;
    int diff;

    if (*state < RESULT_WAIT_CONNECTED) {
        if (IrcAppBar_GetResult(wk->button) == 1) {
            CompatibleIrcSys_ResetState(wk->param->sys);
            wk->param->result = IRC_RHYTHM_CHECK_RESULT_CANCELLED;
            IrcRhythmCheck_SetStepDone(wk);
            return;
        }
    }
    switch (*state) {
    case RESULT_EXCHANGE_IDS_FIRST:
        if (CompatibleIrcSys_ExchangeIds(wk->param->sys, 2)) {
            *state = RESULT_PRINT_CHECKING;
        }
        break;
    case RESULT_PRINT_CHECKING:
        IrcRhythmWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain7_Text_CheckingPleaseMaintainInfrared, 0, 0);
        *state = RESULT_SEND_TAPS;
        break;
    case RESULT_SEND_TAPS:
        if (IrcRhythmNet_Send(&wk->net, &wk->data)) {
            *state = RESULT_SEND_COUNT;
        }
        break;
    case RESULT_SEND_COUNT:
        CompatibleIrcSys_SetCount(wk->param->sys, 2);
        if (CompatibleIrcSys_SendCount(wk->param->sys)) {
            *state = RESULT_EXCHANGE_IDS_LAST;
        }
        break;
    case RESULT_EXCHANGE_IDS_LAST:
        if (CompatibleIrcSys_ExchangeIds(wk->param->sys, 3)) {
            IrcAppBar_SetActive(wk->button, FALSE);
            *state = RESULT_WAIT_CONNECTED;
        }
        break;
    case RESULT_WAIT_CONNECTED:
        if (wk->timer < CONNECTED_FRAMES) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SYS_12);
        IrcRhythmWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain7_Text_YouveCreatedInfraredConnection, 0, 0);
        *state = RESULT_WAIT_SCORE;
        break;
    case RESULT_WAIT_SCORE:
        if (wk->resultTimer++ < SCORE_FRAMES) {
            return;
        }
        wk->param->unk10 = IrcRhythmCheck_ComputeScore(wk);
        IrcRhythmNet_GetReceived(&wk->net, &received);
        diff = wk->data.extraTaps - received.extraTaps;
        if (diff < 0) {
            diff = -diff;
        }
        wk->param->unk14 = diff;
        wk->param->result = IRC_RHYTHM_CHECK_RESULT_DONE;
        *state = RESULT_FINISH;
        return;
    case RESULT_FINISH:
        IrcRhythmCheck_SetStepDone(wk);
        return;
    }
    if (*state >= RESULT_PRINT_CHECKING && *state <= RESULT_WAIT_CONNECTED && wk->timer <= CONNECTED_FRAMES) {
        wk->timer++;
    }
    if (*state < RESULT_SEND_COUNT) {
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
            IrcRhythmCheck_SetStep(wk, IrcRhythmCheck_StepInterrupted);
        }
    }
}

// The link broke: ends the screen
static void IrcRhythmCheck_StepInterrupted(IrcRhythmCheckWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) > 0) {
            wk->param->unk10 = 0;
            wk->param->result = IRC_RHYTHM_CHECK_RESULT_CANCELLED;
            IrcRhythmCheck_SetStepDone(wk);
            return;
        }
        if (CompatibleIrcSys_GetPendingCount(wk->param->sys) < 0) {
            *state = 1;
            return;
        }
        break;
    case 1:
        wk->param->unk10 = 0;
        wk->param->result = IRC_RHYTHM_CHECK_RESULT_INTERRUPTED;
        *state = 2;
        break;
    case 2:
        IrcRhythmWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain7_Text_ConnectionInterruptedPleaseRealign, 0,
                                 0);
        *state = 3;
        break;
    case 3:
        if (CompatibleIrcSys_ExchangeIds(wk->param->sys, 6)) {
            *state = 4;
        }
        break;
    case 4:
        IrcRhythmCheck_SetStepDone(wk);
        break;
    }
}

static void IrcRhythmData_Clear(IrcRhythmData *data) {
    sys_memset(data, 0, sizeof(IrcRhythmData));
}

// Starts the clock that the taps are timed from
static void IrcRhythmData_Start(IrcRhythmData *data) {
    // BUG: the clock is only started if it already was
#ifdef BUGFIX
    if (!clock_is_inited()) {
#else
    if (clock_is_inited()) {
#endif
        clock_init();
    }
    data->start = OS_TicksToMilliSeconds(clock());
}

static BOOL IrcRhythmData_IsFull(IrcRhythmData *data) {
    if (data->count >= TAP_COUNT) {
        return TRUE;
    }
    return FALSE;
}

static void IrcRhythmData_AddTap(IrcRhythmData *data) {
    if (data->count < TAP_COUNT) {
        data->taps[data->count].time = OS_TicksToMilliSeconds(clock()) - data->start;
        if (data->count == 0) {
            data->taps[data->count].interval = 0;
        } else {
            data->taps[data->count].interval = data->taps[data->count].time - data->taps[data->count - 1].time;
        }
        data->count++;
    } else if (data->extraTaps < EXTRA_TAP_MAX) {
        data->extraTaps++;
    }
}

// Compares the last two intervals, plays a sound for how they differ and returns the tap's grade
static int IrcRhythmData_GetTapGrade(IrcRhythmData *data) {
    u32 sound;
    int grade;
    int diff;

    if (data->count < 2) {
        sound = SEQ_SE_SYS_13;
        grade = TAP_GRADE_ON_BEAT;
    } else {
        diff = data->taps[data->count - 1].interval - data->taps[data->count - 2].interval;
        if (MATH_ABS(diff) >= 0 && MATH_ABS(diff) < 50) {
            sound = SEQ_SE_SYS_13;
            grade = TAP_GRADE_ON_BEAT;
        } else if (diff < 0) {
            if (MATH_ABS(diff) < 200) {
                sound = SEQ_SE_SYS_16;
                grade = TAP_GRADE_EARLY;
            } else {
                sound = SEQ_SE_SYS_17;
                grade = TAP_GRADE_EARLY_FAR;
            }
        } else {
            if (MATH_ABS(diff) < 200) {
                sound = SEQ_SE_SYS_14;
                grade = TAP_GRADE_LATE;
            } else {
                sound = SEQ_SE_SYS_15;
                grade = TAP_GRADE_LATE_FAR;
            }
        }
    }
    GFL_SndSEPlay(sound);
    return grade;
}

static void IrcRhythmNet_Init(IrcRhythmNet *net, CompatibleIrcSys *sys) {
    sys_memset(net, 0, sizeof(IrcRhythmNet));
    net->sys = sys;
    CompatibleIrcSys_AddCommands(sys, NET_COMMAND_BASE, sIrcRhythmCommands, 1, net);
}

static void IrcRhythmNet_Free(IrcRhythmNet *net) {
    CompatibleIrcSys_RemoveCommands(net->sys, NET_COMMAND_BASE);
    sys_memset(net, 0, sizeof(IrcRhythmNet));
}

// Sends the data, and returns TRUE once the other player's has also arrived
static BOOL IrcRhythmNet_Send(IrcRhythmNet *net, const IrcRhythmData *data) {
    switch (net->state) {
    case 0:
        net->send = *data;
        net->state = 1;
        break;
    case 1:
        if (CompatibleIrcSys_SendEx(net->sys, NET_COMMAND_BASE, sizeof(IrcRhythmData), &net->send,
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

static void IrcRhythmNet_GetReceived(IrcRhythmNet *net, IrcRhythmData *dest) {
    *dest = net->peer;
}

static void IrcRhythmNet_OnReceive(int netId, int size, const void *data, void *work, NetHandle *handle) {
    IrcRhythmNet *net = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            net->received = TRUE;
        }
    }
}

static void *IrcRhythmNet_GetBuffer(int netId, void *work, int size) {
    IrcRhythmNet *net = work;

    if (netId == func_0203ffc4()) {
        return &net->own;
    }
    return &net->peer;
}

// Reads a touch inside the area of the screen that counts
static BOOL IrcRhythm_GetTouch(IrcRhythmPoint *point) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y) && x <= TOUCH_MAX_X && y <= TOUCH_MAX_Y) {
        point->x = x;
        point->y = y;
        return TRUE;
    }
    return FALSE;
}

// Scores how close the two players' rhythms were, from how long all the taps took and how each interval differed
static u8 IrcRhythmCheck_ComputeScore(IrcRhythmCheckWork *wk) {
    IrcRhythmData own = wk->data;
    IrcRhythmData peer;
    // BUG: nothing sets this if the two durations differ by 30000 ms or more
#ifdef BUGFIX
    u32 base = 0;
#else
    u32 base;
#endif
    u32 sum;
    u32 diff;
    int i;

    IrcRhythmNet_GetReceived(&wk->net, &peer);
    diff = (own.taps[own.count - 1].time - own.taps[0].time) - (peer.taps[peer.count - 1].time - peer.taps[0].time);
    if ((int)diff < 0) {
        diff = -diff;
    }
    if (diff == 0) {
        base = 100;
    } else if (diff < 500) {
        base = 90;
    } else if (diff < 1000) {
        base = 80;
    } else if (diff < 3000) {
        base = 70;
    } else if (diff < 5000) {
        base = 60;
    } else if (diff < 10000) {
        base = 50;
    } else if (diff < 15000) {
        base = 40;
    } else if (diff < 20000) {
        base = 30;
    } else if (diff < 25000) {
        base = 20;
    } else if (diff < 30000) {
        base = 10;
    }
    sum = 0;
    for (i = 1; i < TAP_COUNT; i++) {
        diff = own.taps[i].interval - peer.taps[i].interval;
        if ((int)diff < 0) {
            diff = -diff;
        }
        if (diff == 0) {
            sum += 100;
        } else if (diff < 50) {
            sum += 90;
        } else if (diff < 100) {
            sum += 80;
        } else if (diff < 200) {
            sum += 70;
        } else if (diff < 300) {
            sum += 60;
        } else if (diff < 400) {
            sum += 50;
        } else if (diff < 500) {
            sum += 40;
        } else if (diff < 600) {
            sum += 30;
        } else if (diff < 1000) {
            sum += 20;
        } else if (diff < 15000) {
            sum += 10;
        }
    }
    return (base + sum / 9) / 2;
}

// Sets a color of a palette to a blend of two colors, by how far the angle has gone around a circle
static void IrcRhythm_BlendColor(NNS_GFD_DST_TYPE type, u16 *dest, int angle, u32 palette, u8 index, u16 from, u16 to) {
    u8 fromR = from & GX_RGB_R_MASK;
    u8 fromG = (from & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 fromB = (from & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    u8 toR = to & GX_RGB_R_MASK;
    u8 toG = (to & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
    u8 toB = (to & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
    s16 weight = (FX_CosIdx(angle) + FX32_ONE) / 2;
    int stepB = (toB - fromB) * weight >> FX32_SHIFT;
    int stepR = (toR - fromR) * weight >> FX32_SHIFT;
    int stepG = (toG - fromG) * weight >> FX32_SHIFT;

    *dest = GX_RGB((u8)(fromR + stepR), (u8)(fromG + stepG), (u8)(fromB + stepB));
    NNS_GfdRegisterNewVramTransferTask(type, palette * 0x20 + index * 2, dest, 2);
}

static void IrcRhythmParticles_Init(IrcRhythmParticles *particles, IrcRhythmGraphic *graphic, int mode, int firstActor,
                                    u32 surface) {
    int i;

    sys_memset(particles, 0, sizeof(IrcRhythmParticles));
    particles->mode = mode;
    particles->timer = 0;
    particles->interval = 10;
    for (i = 0; i < PARTICLE_COUNT; i++) {
        IrcRhythmParticle_Init(&particles->particles[i], IrcRhythmGraphic_GetActor(graphic, firstActor + i), surface);
    }
}

static void IrcRhythmParticles_Free(IrcRhythmParticles *particles) {
    sys_memset(particles, 0, sizeof(IrcRhythmParticles));
}

static void IrcRhythmParticles_Update(IrcRhythmParticles *particles) {
    IrcRhythmPoint b;
    IrcRhythmPoint a;
    u32 duration;
    u8 side;
    int palette;
    u16 angle;
    int i;
    int x;

    switch (particles->mode) {
    case PARTICLES_RISE:
        if (particles->timer++ > particles->interval) {
            particles->timer = 0;
            for (i = 0; i < PARTICLE_COUNT; i++) {
                if (!IrcRhythmParticle_IsActive(&particles->particles[i])) {
                    side = GFL_RandomLC(2);
                    x = GFL_RandomLC(256);
                    IrcRhythmPoint_Set(&b, x, -36);
                    IrcRhythmPoint_Set(&a, x, 228);
                    duration = GFL_RandomLC(50) + 110;
                    palette = 2;
                    if (GFL_RandomLC(2)) {
                        palette = 5;
                    }
                    if (side) {
                        IrcRhythmParticle_Start(&particles->particles[i], &a, &b, palette, duration);
                    } else {
                        IrcRhythmParticle_Start(&particles->particles[i], &b, &a, palette, duration);
                    }
                    break;
                }
            }
        }
        break;
    case PARTICLES_TAPS_ONLY:
        break;
    case PARTICLES_CONVERGE:
        if (particles->timer++ > particles->interval) {
            particles->timer = 0;
            for (i = 0; i < PARTICLE_COUNT; i++) {
                if (!IrcRhythmParticle_IsActive(&particles->particles[i])) {
                    angle = GFL_RandomLC(0xffff);
                    IrcRhythmPoint_Set(&b, FX_Whole(FX_SinIdx(angle) << 8), FX_Whole(FX_CosIdx(angle) << 8));
                    IrcRhythmPoint_Set(&a, 128, 96);
                    duration = GFL_RandomLC(50) + 110;
                    palette = 2;
                    if (GFL_RandomLC(2)) {
                        palette = 5;
                    }
                    IrcRhythmParticle_Start(&particles->particles[i], &b, &a, palette, duration);
                    break;
                }
            }
        }
        break;
    }
    for (i = 0; i < PARTICLE_COUNT; i++) {
        IrcRhythmParticle_Update(&particles->particles[i]);
    }
}

// Sends sparkles out from a tap, in the color of its grade
static void IrcRhythmParticles_Burst(IrcRhythmParticles *particles, const IrcRhythmPoint *origin, int palette) {
    VecFx32 center;
    VecFx32 ring;
    VecFx32 dir;
    IrcRhythmPoint target;
    u16 angle;
    int burst;
    int i;

    center.x = origin->x << FX32_SHIFT;
    center.y = origin->y << FX32_SHIFT;
    center.z = 0;
    for (burst = 0; burst < 12; burst++) {
        for (i = 0; i < PARTICLE_COUNT; i++) {
            if (!IrcRhythmParticle_IsActive(&particles->particles[i])) {
                angle = GFL_RandomLC(0xffff);
                target.x = FX_Whole(FX_SinIdx(angle) << 8);
                target.y = FX_Whole(FX_CosIdx(angle) << 8);
                ring.x = target.x << FX32_SHIFT;
                ring.y = target.y << FX32_SHIFT;
                ring.z = 0;
                VEC_Subtract(&ring, &center, &dir);
                vecfx_normalize(&dir, &dir);
                target.x = FX_Whole(dir.x << 8);
                target.y = FX_Whole(dir.y << 8);
                target.x += origin->x;
                target.y += origin->y;
                IrcRhythmParticle_Start(&particles->particles[i], origin, &target, palette, GFL_RandomLC(5) + 40);
                break;
            }
        }
    }
}

static void IrcRhythmParticle_Init(IrcRhythmParticle *particle, ClActor *actor, u32 surface) {
    sys_memset(particle, 0, sizeof(IrcRhythmParticle));
    particle->actor = actor;
    particle->surface = surface;
}

static void IrcRhythmParticle_Update(IrcRhythmParticle *particle) {
    VecFx32 dir;
    VecFx32 current;
    ClActorPos pos;
    fx32 length;

    if (particle->active) {
        VEC_Subtract(&particle->end, &particle->start, &dir);
        length = VEC_Mag(&dir);
        vecfx_normalize(&dir, &dir);
        vecfx_muladd(length * particle->frame / particle->duration, &dir, &particle->start, &current);
        pos.x = current.x >> FX32_SHIFT;
        pos.y = current.y >> FX32_SHIFT;
        if (particle->frame++ > particle->duration) {
            // BUG: the end is in fixed point, and the position is not
#ifdef BUGFIX
            pos.x = particle->end.x >> FX32_SHIFT;
            pos.y = particle->end.y >> FX32_SHIFT;
#else
            pos.x = particle->end.x;
            pos.y = particle->end.y;
#endif
            func_0204c124(particle->actor, FALSE);
            particle->active = FALSE;
        }
        func_0204c140(particle->actor, &pos, particle->surface);
    }
}

static void IrcRhythmParticle_Start(IrcRhythmParticle *particle, const IrcRhythmPoint *from, const IrcRhythmPoint *to,
                                    int palette, u32 duration) {
    particle->active = TRUE;
    VEC_Set(&particle->start, FX32_CONST(from->x), FX32_CONST(from->y), 0);
    VEC_Set(&particle->end, FX32_CONST(to->x), FX32_CONST(to->y), 0);
    particle->frame = 0;
    particle->duration = duration;
    func_0204c124(particle->actor, TRUE);
    func_0204c378(particle->actor, (u8)palette, TRUE);
}

static BOOL IrcRhythmParticle_IsActive(IrcRhythmParticle *particle) {
    return particle->active;
}

static void IrcRhythmFlash_Init(IrcRhythmFlash *flash, HeapID heapId) {
    sys_memset(flash, 0, sizeof(IrcRhythmFlash));
    flash->enabled = TRUE;
}

static void IrcRhythmFlash_Free(IrcRhythmFlash *flash) {
    sys_memset(flash, 0, sizeof(IrcRhythmFlash));
}

static void IrcRhythmFlash_Update(IrcRhythmFlash *flash) {
    if (flash->active && flash->enabled) {
        u16 angle = flash->angle;

        if (angle + 0x400 >= 0x10000) {
            flash->angle = angle - 0xfc00;
        } else {
            flash->angle = angle + 0x400;
        }
        IrcRhythm_BlendColor(NNS_GFD_DST_2D_BG_PLTT_MAIN, &flash->color, flash->angle, 5, 3, 0x6adf, 0x55fe);
    }
}

static void IrcRhythmFlash_Start(IrcRhythmFlash *flash) {
    flash->active = TRUE;
}

// Puts the blend back to none once the countdown is done
static void IrcRhythmCheck_VBlank(TCB *tcb, void *data) {
    IrcRhythmCheckWork *wk = data;

    if (wk->clearBlend) {
        wk->clearBlend = FALSE;
        G2_BlendNone();
    }
}
