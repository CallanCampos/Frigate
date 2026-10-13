#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/irc_appbar.h"
#include "app/irc_compat_menu.h"
#include "app/irc_compatible.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/os.h"
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
#include "text/system/btl_main_4.h"

// The first screen of the compatibility checker: its graphics, its windows, the two buttons, the hearts that float up
// the background and the steps of the screen, which also make the link with the other player. The ROM has no string for
// the file, so the name is a guess. Names are ours

#define HEAP_ID 0x3b
#define BG_COUNT 8
#define CLACT_COUNT 32
#define HEART_COUNT 30
#define BUTTON_COUNT_MAX 4

// The actor of the sprite behind the hearts
#define ACTOR_BACKGROUND (CLACT_COUNT - 1)

// The frames that the title scrolls in for, and the frames the connection is shown for
#define SCROLL_FRAMES 120
#define SCROLL_DISTANCE 0x58
#define CONNECTED_FRAMES 60
#define FINISH_FRAMES 30

// The steps of IrcCompatMenu_StepConnect
enum {
    CONNECT_RESET,
    CONNECT_START_NET,
    CONNECT_START_NEGOTIATION,
    CONNECT_PRINT_CHECKING,
    CONNECT_SYNC,
    CONNECT_EXCHANGE_PROFILES,
    CONNECT_EXCHANGE_IDS,
    CONNECT_UNUSED,
    CONNECT_SEND_SELECTION,
    CONNECT_POLL_SELECTION,
    CONNECT_SEND_PING,
    CONNECT_POLL_PING,
    CONNECT_SEND_COUNT,
    CONNECT_EXCHANGE_IDS_LAST,
    CONNECT_WAIT_CONNECTED,
    CONNECT_WAIT_FINISH,
};

typedef struct {
    ClActUnit *unit;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *actors[CLACT_COUNT];
} IrcCompatMenuClAct;

typedef struct {
    IrcCompatMenuClAct clact;
    TCB *vblankTask;
} IrcCompatMenuGraphic;

typedef struct {
    // The characters of the window frame
    u32 frameChars;
    TCB *vblankTask;
} IrcCompatMenuBG;

typedef struct {
    Font *font;
    MsgData *msgData;
    PrintQueue *queue;
    WordSet *wordSet;
} IrcCompatMenuText;

typedef struct {
    BmpWin *window;
    PrintWindow printWindow;
    StrBuf *strbuf;
    u16 color;
} IrcCompatMenuWindow;

// A button: the message on it, and its place in tiles
typedef struct {
    u16 msgId;
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    u16 palette;
} IrcCompatMenuButtonDef;

typedef struct {
    ButtonMan *manager;
    BmpWin *windows[BUTTON_COUNT_MAX];
    TouchRect rects[BUTTON_COUNT_MAX + 1];
    const IrcCompatMenuButtonDef *defs;
    u16 count;
    u16 bg;
    // The button that was touched
    u16 touched;
    // Whether the touched button has flashed
    u16 pressed;
    u16 state;
    u16 frame;
    BOOL enabled;
} IrcCompatMenuButtons;

// A value that goes from one number to another in steps, in fixed point
typedef struct {
    int current;
    int start;
    int end;
    int step;
    int frame;
    int duration;
} IrcCompatMenuEase;

// How the title scrolls in
typedef struct {
    IrcCompatMenuEase ease;
    BOOL active;
} IrcCompatMenuScroll;

// A heart sprite that floats up from below the screen and starts again once it is out of sight
typedef struct {
    int x;
    int y;
    int startX;
    int startY;
    u32 frame;
    u32 duration;
    ClActor *actor;
} IrcCompatMenuHeart;

// A heart for each player the trainer has logged
typedef struct {
    IrcCompatMenuHeart hearts[HEART_COUNT];
    u32 timer;
    u16 activeCount;
    u16 count;
} IrcCompatMenuHearts;

typedef struct IrcCompatMenuWork IrcCompatMenuWork;
typedef void (*IrcCompatMenuStepFunc)(IrcCompatMenuWork *wk, u16 *state);

struct IrcCompatMenuWork {
    IrcCompatMenuGraphic graphic;
    IrcCompatMenuBG bg;
    IrcCompatMenuText text;
    // The message of the step
    IrcCompatMenuWindow messageWindow;
    IrcCompatMenuButtons buttons;
    IrcCompatMenuScroll scroll;
    IrcCompatMenuHearts hearts;
    IrcAppBar *button;
    // The title
    IrcCompatMenuWindow titleWindow;
    IrcCompatMenuStepFunc step;
    u16 stepState;
    u16 timer;
    BOOL stepDone;
    IrcCompatibleParam0 *param;
    u32 startTime;
    // The button that was chosen
    int choice;
    // How long the choice took
    int choiceTime;
    u32 counter;
    BOOL choiceSent;
};

static BOOL IrcCompatMenu_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatMenu_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatMenu_Main(GameProc *proc, u32 *state, void *param, void *work);
static void IrcCompatMenuGraphic_Create(IrcCompatMenuGraphic *graphic, HeapID heapId);
static void IrcCompatMenuGraphic_Free(IrcCompatMenuGraphic *graphic);
static void IrcCompatMenuGraphic_Update(IrcCompatMenuGraphic *graphic);
static ClActor *IrcCompatMenuGraphic_GetActor(IrcCompatMenuGraphic *graphic, int index);
static ClActUnit *IrcCompatMenuGraphic_GetUnit(IrcCompatMenuGraphic *graphic);
static void IrcCompatMenuGraphic_VBlank(TCB *tcb, void *data);
static void IrcCompatMenuBG_Init(IrcCompatMenuBG *bg, HeapID heapId);
static void IrcCompatMenuBG_Free(IrcCompatMenuBG *bg);
static void IrcCompatMenuBG_VBlank(TCB *tcb, void *data);
static void IrcCompatMenuClAct_Init(IrcCompatMenuClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IrcCompatMenuClAct_Free(IrcCompatMenuClAct *clact);
static void IrcCompatMenuClAct_Update(IrcCompatMenuClAct *clact);
static void IrcCompatMenuClAct_VBlank(IrcCompatMenuClAct *clact);
static ClActor *IrcCompatMenuClAct_GetActor(IrcCompatMenuClAct *clact, int index);
static ClActUnit *IrcCompatMenuClAct_GetUnit(IrcCompatMenuClAct *clact);
static void IrcCompatMenuText_Init(IrcCompatMenuText *text, HeapID heapId);
static void IrcCompatMenuText_Free(IrcCompatMenuText *text);
static BOOL IrcCompatMenuText_Update(IrcCompatMenuText *text);
static Font *IrcCompatMenuText_GetFont(IrcCompatMenuText *text);
static PrintQueue *IrcCompatMenuText_GetQueue(IrcCompatMenuText *text);
static MsgData *IrcCompatMenuText_GetMsgData(IrcCompatMenuText *text);
static void IrcCompatMenuWindow_Init(IrcCompatMenuWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                     HeapID heapId);
static void IrcCompatMenuWindow_InitWithColor(IrcCompatMenuWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                              u8 palette, u16 color, u8 fromEnd, HeapID heapId);
static void IrcCompatMenuWindow_Free(IrcCompatMenuWindow *win);
static BOOL IrcCompatMenuWindow_Flush(IrcCompatMenuWindow *win, IrcCompatMenuText *text);
static void IrcCompatMenuWindow_PrintMsg(IrcCompatMenuWindow *win, IrcCompatMenuText *text, u32 msgId, int x, s16 y);
static void IrcCompatMenuWindow_PrintMsgCentered(IrcCompatMenuWindow *win, IrcCompatMenuText *text, u32 msgId);
static void IrcCompatMenu_SetStep(IrcCompatMenuWork *wk, IrcCompatMenuStepFunc step);
static void IrcCompatMenu_SetStepDone(IrcCompatMenuWork *wk);
static void IrcCompatMenu_StepConnect(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_StepMenu(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_StepFinish(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_StepReturn(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_StepCancel(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_StepPeerStarted(IrcCompatMenuWork *wk, u16 *state);
static void IrcCompatMenu_UpdateAll(IrcCompatMenuWork *wk);
static void IrcCompatMenuButtons_Init(IrcCompatMenuButtons *buttons, u8 bg, const IrcCompatMenuButtonDef *defs,
                                      u16 count, IrcCompatMenuText *text, HeapID heapId);
static void IrcCompatMenuButtons_Free(IrcCompatMenuButtons *buttons);
static void IrcCompatMenuButtons_Update(IrcCompatMenuButtons *buttons);
static void IrcCompatMenuButtons_SetEnabled(IrcCompatMenuButtons *buttons, BOOL enabled);
static BOOL IrcCompatMenuButtons_IsTouched(IrcCompatMenuButtons *buttons);
static BOOL IrcCompatMenuButtons_GetResult(IrcCompatMenuButtons *buttons, int *result);
static void IrcCompatMenuButtons_Reset(IrcCompatMenuButtons *buttons);
static void IrcCompatMenuButtons_Callback(u32 button, u32 event, void *work);
static void IrcCompatMenuScroll_Init(IrcCompatMenuScroll *scroll, HeapID heapId);
static void IrcCompatMenuScroll_Free(IrcCompatMenuScroll *scroll);
static void IrcCompatMenuScroll_Update(IrcCompatMenuScroll *scroll);
static void IrcCompatMenuScroll_Start(IrcCompatMenuScroll *scroll);
static void IrcCompatMenuHeart_Init(IrcCompatMenuHeart *heart, ClActor *actor, BOOL isSpecial, HeapID heapId);
static void IrcCompatMenuHeart_Free(IrcCompatMenuHeart *heart);
static void IrcCompatMenuHeart_Update(IrcCompatMenuHeart *heart);
static void IrcCompatMenuHeart_Reset(IrcCompatMenuHeart *heart);
static void IrcCompatMenuHearts_Init(IrcCompatMenuHearts *hearts, IrcCompatMenuGraphic *graphic, TrainerLogSave *log,
                                     HeapID heapId);
static void IrcCompatMenuHearts_Free(IrcCompatMenuHearts *hearts);
static void IrcCompatMenuHearts_Update(IrcCompatMenuHearts *hearts);
static void IrcCompatMenuEase_Start(IrcCompatMenuEase *ease, int from, int to, int frames);
static BOOL IrcCompatMenuEase_Update(IrcCompatMenuEase *ease);

const GameProcFunctions IRC_COMPAT_MENU_PROC_FUNCTIONS = {
    IrcCompatMenu_Init,
    IrcCompatMenu_Main,
    IrcCompatMenu_Exit,
};

static const BGSysLCDConfig sIrcCompatMenuLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_1, GX_BG0_AS_2D };

static const IrcCompatMenuButtonDef sIrcCompatMenuButtonDefs[2] = {
    { BtlMain4_Text_BeginCheck, 3, 2, 26, 5, 11 },
    { BtlMain4_Text_Ranking, 3, 14, 26, 5, 4 },
};

static const u32 sIrcCompatMenuBGModes[BG_COUNT] = { BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT,
                                                     BGMODE_TEXT, BGMODE_TEXT, BGMODE_TEXT, BGMODE_AFFINE };

static const u32 sIrcCompatMenuBGs[BG_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static const BGSysVRAMConfig sIrcCompatMenuVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSetup sIrcCompatMenuBGSetups[BG_COUNT] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x10000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x08000), 0x8000,
      GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x1000, 0, BGRES_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x4000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL IrcCompatMenu_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatMenuWork *wk;
    IrcCompatibleParam0 *menuParam = param;
    SaveControl *save;
    TrainerLogSave *log;
    ClActUnit *unit;
    Font *font;
    PrintQueue *queue;

    GFL_HeapCreateChild(HEAPID_USER, HEAP_ID, 0x40000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcCompatMenuWork), HEAP_ID);
    sys_memset(wk, 0, sizeof(IrcCompatMenuWork));
    wk->param = menuParam;
    IrcCompatMenuText_Init(&wk->text, HEAP_ID);
    IrcCompatMenuGraphic_Create(&wk->graphic, HEAP_ID);
    IrcCompatMenuBG_Init(&wk->bg, HEAP_ID);
    if (wk->param->gsys != NULL) {
        GSYS_GetGameCommSystem(wk->param->gsys);
    }
    IrcCompatMenuButtons_Init(&wk->buttons, 2, sIrcCompatMenuButtonDefs, 2, &wk->text, HEAP_ID);
    IrcCompatMenuWindow_Init(&wk->messageWindow, 5, 1, 18, 30, 5, 8, HEAP_ID);
    BmpWin_DrawFrame(wk->messageWindow.window, 0, (u16)wk->bg.frameChars, 6);
    IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_TapMenuTouchScreen, 0, 0);
    IrcCompatMenuWindow_InitWithColor(&wk->titleWindow, 7, 6, 3, 20, 5, 8, 1, TRUE, HEAP_ID);
    IrcCompatMenuWindow_PrintMsgCentered(&wk->titleWindow, &wk->text, BtlMain4_Text_FeelingCheck);
    GFL_BGSysScaleBGReq(7, 3, 0x1000);
    GFL_BGSysScaleBGReq(7, 6, 0x1000);
    GFL_BGSysAdjustBGOriginReq(7, 9, 0x80);
    GFL_BGSysAdjustBGOriginReq(7, 12, 0x28);
    IrcCompatMenuScroll_Init(&wk->scroll, HEAP_ID);
    if (wk->param->gsys != NULL) {
        save = GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys));
    } else {
        save = SaveControl_GetInstance();
    }
    log = func_0200afbc(save);
    IrcCompatMenuHearts_Init(&wk->hearts, &wk->graphic, log, HEAP_ID);
    unit = IrcCompatMenuGraphic_GetUnit(&wk->graphic);
    font = IrcCompatMenuText_GetFont(&wk->text);
    queue = IrcCompatMenuText_GetQueue(&wk->text);
    wk->button = IrcAppBar_Create(2, unit, 0, 14, 0, 2, font, queue, HEAP_ID);
    switch (wk->param->visit) {
    case IRC_COMPAT_VISIT_FIRST:
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepMenu);
        break;
    case IRC_COMPAT_VISIT_AGAIN:
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepReturn);
        break;
    case IRC_COMPAT_VISIT_AFTER_RESULT:
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepMenu);
        break;
    }
    func_02005d8c();
    CompatibleIrcSys_SetCount(wk->param->sys, 0);
    return TRUE;
}

static BOOL IrcCompatMenu_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatMenuWork *wk = work;

    IrcAppBar_Free(wk->button);
    IrcCompatMenuHearts_Free(&wk->hearts);
    IrcCompatMenuScroll_Free(&wk->scroll);
    IrcCompatMenuButtons_Free(&wk->buttons);
    IrcCompatMenuWindow_Free(&wk->titleWindow);
    IrcCompatMenuWindow_Free(&wk->messageWindow);
    IrcCompatMenuBG_Free(&wk->bg);
    IrcCompatMenuGraphic_Free(&wk->graphic);
    IrcCompatMenuText_Free(&wk->text);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAP_ID);
    return TRUE;
}

static BOOL IrcCompatMenu_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatMenuWork *wk = work;

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
        func_0204c520(IrcCompatMenuGraphic_GetActor(&wk->graphic, ACTOR_BACKGROUND), TRUE);
        IrcCompatMenuScroll_Start(&wk->scroll);
        GFL_SndBGMPlay(SEQ_ME_AISHOU_KIDOU, SND_CHANNEL_MASK_ALL);
        *state = 3;
        break;
    case 3:
        IrcCompatMenuHearts_Update(&wk->hearts);
        IrcCompatMenuScroll_Update(&wk->scroll);
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
    IrcCompatMenu_UpdateAll(wk);
    return FALSE;
}

static void IrcCompatMenuGraphic_Create(IrcCompatMenuGraphic *graphic, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(IrcCompatMenuGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysSetVRAMBanks(&sIrcCompatMenuVRAMConfig);
    IrcCompatMenuClAct_Init(&graphic->clact, &sIrcCompatMenuVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(IrcCompatMenuGraphic_VBlank, graphic, 0);
}

static void IrcCompatMenuGraphic_Free(IrcCompatMenuGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IrcCompatMenuClAct_Free(&graphic->clact);
}

static void IrcCompatMenuGraphic_Update(IrcCompatMenuGraphic *graphic) {
    IrcCompatMenuClAct_Update(&graphic->clact);
}

static ClActor *IrcCompatMenuGraphic_GetActor(IrcCompatMenuGraphic *graphic, int index) {
    return IrcCompatMenuClAct_GetActor(&graphic->clact, index);
}

static ClActUnit *IrcCompatMenuGraphic_GetUnit(IrcCompatMenuGraphic *graphic) {
    return IrcCompatMenuClAct_GetUnit(&graphic->clact);
}

static void IrcCompatMenuGraphic_VBlank(TCB *tcb, void *data) {
    IrcCompatMenuGraphic *graphic = data;

    IrcCompatMenuClAct_VBlank(&graphic->clact);
}

static void IrcCompatMenuBG_Init(IrcCompatMenuBG *bg, HeapID heapId) {
    ArcTool *arc;
    int i;

    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sIrcCompatMenuLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysCreateBG(sIrcCompatMenuBGs[i], &sIrcCompatMenuBGSetups[i], sIrcCompatMenuBGModes[i]);
        GFL_BGSysClearBG(sIrcCompatMenuBGs[i]);
        GFL_BGSysSetBGEnabled(sIrcCompatMenuBGs[i], TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_COMPAT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 3, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 48, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 42, 2, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 44, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 43, 2, 0, 0, FALSE, heapId);
    GFL_BGSysFillChar(5, 0, 1, 0);
    bg->frameChars = GFL_BGSysLoadArcNCGRDynamic(arc, 49, 5, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    bg->vblankTask = GFL_VBlankTCBAdd(IrcCompatMenuBG_VBlank, bg, 0);
}

static void IrcCompatMenuBG_Free(IrcCompatMenuBG *bg) {
    int i;

    GFL_TCBRemove(bg->vblankTask);
    GFL_BGSysFreeCharMemory(5, (u16)bg->frameChars, (u16)(bg->frameChars >> 16));
    GFL_BGSysFreeFilledChar(5, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sIrcCompatMenuBGs[i]);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void IrcCompatMenuBG_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
}

static void IrcCompatMenuClAct_Init(IrcCompatMenuClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;
    int i;

    sys_memset(clact, 0, sizeof(IrcCompatMenuClAct));
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
    for (i = 0; i < HEART_COUNT; i++) {
        clact->actors[i] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims, &setup,
                                         CLACT_SURFACE_SUB, heapId);
        func_0204c124(clact->actors[i], FALSE);
        func_0204c468(clact->actors[i], 3);
        func_0204c520(clact->actors[i], TRUE);
    }
    setup.x = 129;
    setup.y = 104;
    setup.sequence = 15;
    clact->actors[ACTOR_BACKGROUND] = func_0204c040(clact->unit, clact->chars, clact->palette, clact->cellAnims,
                                                    &setup, CLACT_SURFACE_SUB, heapId);
    func_0204c124(clact->actors[ACTOR_BACKGROUND], TRUE);
    func_0204c468(clact->actors[ACTOR_BACKGROUND], 2);
    func_0204c520(clact->actors[ACTOR_BACKGROUND], FALSE);
}

static void IrcCompatMenuClAct_Free(IrcCompatMenuClAct *clact) {
    int i;

    for (i = 0; i < CLACT_COUNT; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204bcd0(clact->palette);
    func_0204b98c(clact->chars);
    func_0204be64(clact->cellAnims);
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(IrcCompatMenuClAct));
}

static void IrcCompatMenuClAct_Update(IrcCompatMenuClAct *clact) {
    func_0204b794();
}

static void IrcCompatMenuClAct_VBlank(IrcCompatMenuClAct *clact) {
    func_0204b7c8();
}

static ClActor *IrcCompatMenuClAct_GetActor(IrcCompatMenuClAct *clact, int index) {
    return clact->actors[index];
}

static ClActUnit *IrcCompatMenuClAct_GetUnit(IrcCompatMenuClAct *clact) {
    return clact->unit;
}

static void IrcCompatMenuText_Init(IrcCompatMenuText *text, HeapID heapId) {
    sys_memset(text, 0, sizeof(IrcCompatMenuText));
    func_020232d0();
    text->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    text->queue = func_02021998(heapId);
    text->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_4, heapId);
    text->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0x20, heapId);
}

static void IrcCompatMenuText_Free(IrcCompatMenuText *text) {
    GFL_WordSetSystemFree(text->wordSet);
    GFL_MsgDataFree(text->msgData);
    func_02021a18(text->queue);
    GFL_FontFree(text->font);
    sys_memset(text, 0, sizeof(IrcCompatMenuText));
}

static BOOL IrcCompatMenuText_Update(IrcCompatMenuText *text) {
    return func_02021a3c(text->queue);
}

static Font *IrcCompatMenuText_GetFont(IrcCompatMenuText *text) {
    return text->font;
}

static PrintQueue *IrcCompatMenuText_GetQueue(IrcCompatMenuText *text) {
    return text->queue;
}

static MsgData *IrcCompatMenuText_GetMsgData(IrcCompatMenuText *text) {
    return text->msgData;
}

static void IrcCompatMenuWindow_Init(IrcCompatMenuWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette,
                                     HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcCompatMenuWindow));
    win->color = 15;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcCompatMenuWindow_InitWithColor(IrcCompatMenuWindow *win, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                              u8 palette, u16 color, u8 fromEnd, HeapID heapId) {
    sys_memset(win, 0, sizeof(IrcCompatMenuWindow));
    win->color = color;
    win->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, fromEnd);
    win->strbuf = GFL_StrBufCreate(255, heapId);
    PrintWindow_Init(&win->printWindow, win->window);
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_TransferNow(win->window);
}

static void IrcCompatMenuWindow_Free(IrcCompatMenuWindow *win) {
    GFL_StrBufFree(win->strbuf);
    BmpWin_Free(win->window);
    sys_memset(win, 0, sizeof(IrcCompatMenuWindow));
}

static BOOL IrcCompatMenuWindow_Flush(IrcCompatMenuWindow *win, IrcCompatMenuText *text) {
    PrintWindow_Flush(&win->printWindow, text->queue);
    return PrintWindow_IsPrinted(&win->printWindow);
}

static void IrcCompatMenuWindow_PrintMsg(IrcCompatMenuWindow *win, IrcCompatMenuText *text, u32 msgId, int x, s16 y) {
    MsgData *msgData = IrcCompatMenuText_GetMsgData(text);
    PrintQueue *queue = IrcCompatMenuText_GetQueue(text);
    Font *font = IrcCompatMenuText_GetFont(text);

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    PrintWindow_PrintNoColor(&win->printWindow, queue, x, y, win->strbuf, font);
}

static void IrcCompatMenuWindow_PrintMsgCentered(IrcCompatMenuWindow *win, IrcCompatMenuText *text, u32 msgId) {
    MsgData *msgData = IrcCompatMenuText_GetMsgData(text);
    PrintQueue *queue = IrcCompatMenuText_GetQueue(text);
    Font *font = IrcCompatMenuText_GetFont(text);
    u16 x;
    u16 y;

    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    GFL_MsgDataLoadStrbuf(msgData, msgId, win->strbuf);
    x = BmpWin_GetSizeX(win->window) * 4;
    y = BmpWin_GetSizeY(win->window) * 4;
    x -= (u16)(GFL_FontGetBlockWidth(win->strbuf, font, 0) / 2);
    y -= (u16)(GFL_FontGetBlockHeight(win->strbuf, font) / 2);
    func_02021c44(queue);
    PrintWindow_Print(&win->printWindow, queue, x, y, win->strbuf, font, PRINT_COLOR(15, 14, 1));
}

static void IrcCompatMenu_SetStep(IrcCompatMenuWork *wk, IrcCompatMenuStepFunc step) {
    wk->step = step;
    wk->stepState = 0;
}

static void IrcCompatMenu_SetStepDone(IrcCompatMenuWork *wk) {
    wk->stepDone = TRUE;
}

// Makes the link with the other player, exchanges the trainers and the choice and shows that it is made
static void IrcCompatMenu_StepConnect(IrcCompatMenuWork *wk, u16 *state) {
    RivalDataSave *rivalData;

    if (*state < CONNECT_WAIT_CONNECTED && IrcAppBar_GetResult(wk->button) == 1) {
        CompatibleIrcSys_ResetState(wk->param->sys);
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepReturn);
        IrcAppBar_Reset(wk->button);
        IrcAppBar_SetLabel(wk->button, IRC_APPBAR_LABEL_CANCEL);
        GFL_BGSysLoadScr(0);
        IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_TapMenuTouchScreen, 0, 0);
        IrcCompatMenuButtons_Reset(&wk->buttons);
        return;
    }
    switch (*state) {
    case CONNECT_RESET:
        CompatibleIrcSys_ResetState(wk->param->sys);
        *state = CONNECT_START_NET;
        break;
    case CONNECT_START_NET:
        if (!CompatibleIrcSys_StartNet(wk->param->sys)) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SYS_21);
        IrcAppBar_SetActive(wk->button, TRUE);
        *state = CONNECT_START_NEGOTIATION;
        break;
    case CONNECT_START_NEGOTIATION:
        if (!CompatibleIrcSys_StartNegotiation(wk->param->sys)) {
            break;
        }
        *state = CONNECT_PRINT_CHECKING;
        break;
    case CONNECT_PRINT_CHECKING:
        IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_CheckingPleaseMaintainInfrared, 0, 0);
        *state = CONNECT_SYNC;
        break;
    case CONNECT_SYNC:
        if (!CompatibleIrcSys_Sync(wk->param->sys)) {
            break;
        }
        *state = CONNECT_EXCHANGE_PROFILES;
        break;
    case CONNECT_EXCHANGE_PROFILES:
        if (!CompatibleIrcSys_ExchangeProfiles(wk->param->sys, wk->param->gsys)) {
            break;
        }
        CompatibleIrcSys_GetPeerProfile(wk->param->sys, wk->param->profile);
        if (wk->param->gsys != NULL) {
            rivalData = getHollow_RivalData(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));
            Misc_AddMetTrainer(rivalData, getIDAsUInt(&wk->param->profile->trainer));
        }
        *state = CONNECT_EXCHANGE_IDS;
        break;
    case CONNECT_EXCHANGE_IDS:
        if (!CompatibleIrcSys_ExchangeIds(wk->param->sys, 0)) {
            break;
        }
        *state = CONNECT_SEND_SELECTION;
        break;
    case CONNECT_UNUSED:
        break;
    case CONNECT_SEND_SELECTION:
        wk->choiceSent = TRUE;
        if (!CompatibleIrcSys_SendSelection(wk->param->sys, wk->choiceTime, wk->choice)) {
            break;
        }
        *state = CONNECT_POLL_SELECTION;
        break;
    case CONNECT_POLL_SELECTION:
        if (!CompatibleIrcSys_PollSelection(wk->param->sys)) {
            break;
        }
        *state = CONNECT_SEND_PING;
        break;
    case CONNECT_SEND_PING:
        if (!CompatibleIrcSys_SendPing(wk->param->sys)) {
            break;
        }
        *state = CONNECT_POLL_PING;
        break;
    case CONNECT_POLL_PING:
        if (!CompatibleIrcSys_PollPing(wk->param->sys)) {
            break;
        }
        *state = CONNECT_SEND_COUNT;
        break;
    case CONNECT_SEND_COUNT:
        CompatibleIrcSys_SetCount(wk->param->sys, 1);
        if (!CompatibleIrcSys_SendCount(wk->param->sys)) {
            break;
        }
        *state = CONNECT_EXCHANGE_IDS_LAST;
        break;
    case CONNECT_EXCHANGE_IDS_LAST:
        if (!CompatibleIrcSys_ExchangeIds(wk->param->sys, 1)) {
            break;
        }
        IrcAppBar_SetActive(wk->button, FALSE);
        *state = CONNECT_WAIT_CONNECTED;
        break;
    case CONNECT_WAIT_CONNECTED:
        if (wk->timer < CONNECTED_FRAMES) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_SYS_12);
        IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_YouveCreatedInfraredConnection, 0, 0);
        *state = CONNECT_WAIT_FINISH;
        break;
    case CONNECT_WAIT_FINISH:
        if (wk->counter++ < FINISH_FRAMES) {
            return;
        }
        wk->param->result = IRC_COMPAT_MENU_RESULT_CHECK;
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepFinish);
        return;
    }
    if (*state >= CONNECT_PRINT_CHECKING && *state <= CONNECT_WAIT_CONNECTED && wk->timer <= CONNECTED_FRAMES) {
        wk->timer++;
    }
    if (*state < CONNECT_SEND_COUNT && CompatibleIrcSys_GetPendingCount(wk->param->sys) != 0) {
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepPeerStarted);
    }
}

// Lets the player choose a button, and waits for the other player's choice
static void IrcCompatMenu_StepMenu(IrcCompatMenuWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        wk->choice = 2;
        wk->choiceSent = FALSE;
        wk->startTime = OS_TicksToMilliSeconds((u32)clock());
        *state = 1;
        break;
    case 1:
        IrcCompatMenuButtons_Update(&wk->buttons);
        if (IrcCompatMenuButtons_IsTouched(&wk->buttons)) {
            IrcAppBar_SetActive(wk->button, FALSE);
        }
        if (IrcCompatMenuButtons_GetResult(&wk->buttons, &wk->choice)) {
            switch (wk->choice) {
            case 0:
                *state = 2;
                break;
            case 1:
                wk->param->result = IRC_COMPAT_MENU_RESULT_RANKING;
                IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepFinish);
                break;
            }
        }
        wk->choiceTime = OS_TicksToMilliSeconds((u32)clock()) - wk->startTime;
        if (IrcAppBar_IsPressed(wk->button)) {
            IrcCompatMenuButtons_SetEnabled(&wk->buttons, FALSE);
        }
        if (IrcAppBar_GetResult(wk->button) == 1) {
            CompatibleIrcSys_ResetState(wk->param->sys);
            IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepCancel);
            return;
        }
        break;
    case 2:
        IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_CreateInfraredConnectionStart, 0, 0);
        IrcAppBar_SetLabel(wk->button, IRC_APPBAR_LABEL_BACK);
        IrcAppBar_SetActive(wk->button, FALSE);
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepConnect);
        break;
    }
}

// Waits a moment, and then ends the screen
static void IrcCompatMenu_StepFinish(IrcCompatMenuWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        if (wk->param->result == IRC_COMPAT_MENU_RESULT_RANKING) {
            if (wk->timer++ > 10) {
                wk->timer = 0;
                *state = 1;
            }
        } else {
            *state = 1;
        }
        break;
    case 1:
        IrcCompatMenu_SetStepDone(wk);
        break;
    }
}

// Closes the link that the screen found open, and goes to the menu
static void IrcCompatMenu_StepReturn(IrcCompatMenuWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        IrcCompatMenuWindow_PrintMsg(&wk->messageWindow, &wk->text, BtlMain4_Text_TapMenuTouchScreen, 0, 0);
        CompatibleIrcSys_ResetCounts(wk->param->sys);
        CompatibleIrcSys_ResetState(wk->param->sys);
        *state = 1;
        break;
    case 1:
        if (CompatibleIrcSys_EndNet(wk->param->sys)) {
            *state = 2;
        }
        break;
    case 2:
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepMenu);
        break;
    }
}

// Closes the link when the player cancelled, and ends the screen
static void IrcCompatMenu_StepCancel(IrcCompatMenuWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        *state = 1;
        CompatibleIrcSys_ResetCounts(wk->param->sys);
        CompatibleIrcSys_ResetState(wk->param->sys);
        break;
    case 1:
        if (CompatibleIrcSys_EndNet(wk->param->sys)) {
            *state = 2;
        }
        break;
    case 2:
        wk->param->result = IRC_COMPAT_MENU_RESULT_QUIT;
        IrcCompatMenu_SetStepDone(wk);
        break;
    }
}

// The other player started first: ends the screen if the link is made, and goes back to the menu if not
static void IrcCompatMenu_StepPeerStarted(IrcCompatMenuWork *wk, u16 *state) {
    CompatibleIrcSys_GetPendingCount(wk->param->sys);
    if (CompatibleIrcSys_IsConnected(wk->param->sys)) {
        CompatibleIrcSys_SetCount(wk->param->sys, 1);
        wk->param->result = IRC_COMPAT_MENU_RESULT_CHECK;
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepFinish);
    } else {
        IrcCompatMenu_SetStep(wk, IrcCompatMenu_StepMenu);
    }
}

static void IrcCompatMenu_UpdateAll(IrcCompatMenuWork *wk) {
    IrcCompatMenuText_Update(&wk->text);
    IrcCompatMenuWindow_Flush(&wk->messageWindow, &wk->text);
    IrcCompatMenuWindow_Flush(&wk->titleWindow, &wk->text);
    IrcAppBar_Update(wk->button);
    IrcCompatMenuGraphic_Update(&wk->graphic);
}

static void IrcCompatMenuButtons_Init(IrcCompatMenuButtons *buttons, u8 bg, const IrcCompatMenuButtonDef *defs,
                                      u16 count, IrcCompatMenuText *text, HeapID heapId) {
    int i;

    sys_memset(buttons, 0, sizeof(IrcCompatMenuButtons));
    buttons->count = count;
    buttons->defs = defs;
    buttons->bg = bg;
    buttons->enabled = TRUE;
    for (i = 0; i < buttons->count; i++) {
        const IrcCompatMenuButtonDef *def = &defs[i];

        buttons->rects[i].top = def->y * 8 + 1;
        buttons->rects[i].bottom = (def->y + def->height) * 8 - 1;
        buttons->rects[i].left = def->x * 8 + 1;
        buttons->rects[i].right = (def->x + def->width) * 8 - 1;
    }
    buttons->rects[i].top = TOUCH_RECT_END;
    buttons->manager = GFL_BMN_Create(buttons->rects, IrcCompatMenuButtons_Callback, buttons, heapId);
    for (i = 0; i < buttons->count; i++) {
        const IrcCompatMenuButtonDef *def = &defs[i];
        StrBuf *label;
        u32 width;
        u32 height;
        GFLBitmap *bitmap;

        buttons->windows[i] = BmpWin_CreateDynamic(bg, def->x + 4, def->y + 1, def->width - 8, def->height - 2,
                                                   def->palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(buttons->windows[i]), 4);
        BmpWin_TransferNow(buttons->windows[i]);
        label = GFL_MsgDataLoadStrbufNew(IrcCompatMenuText_GetMsgData(text), def->msgId);
        GFL_TextRndUpdateColorIndexLUT(15, 14, 4);
        width = GFL_FontGetBlockWidth(label, IrcCompatMenuText_GetFont(text), 0);
        height = GFL_FontGetBlockHeight(label, IrcCompatMenuText_GetFont(text));
        bitmap = BmpWin_GetBitmap(buttons->windows[i]);
        GFL_TextRendererDrawToBitmap(bitmap, (def->width - 8) * 4 - width / 2, (def->height - 2) * 4 - height / 2,
                                     label, IrcCompatMenuText_GetFont(text));
        GFL_StrBufFree(label);
        BmpWin_FlushChar(buttons->windows[i]);
    }
    func_020232d8();
}

static void IrcCompatMenuButtons_Free(IrcCompatMenuButtons *buttons) {
    int i;

    for (i = 0; i < buttons->count; i++) {
        BmpWin_Free(buttons->windows[i]);
    }
    GFL_BMN_Delete(buttons->manager);
    sys_memset(buttons, 0, sizeof(IrcCompatMenuButtons));
}

static inline u8 IrcCompatMenuButtons_GetBG(const IrcCompatMenuButtons *buttons, u32 index) {
    return BmpWin_GetBGIndex(buttons->windows[index]);
}

// Runs the buttons: a touched button flashes for a few frames, and then counts as pressed
static void IrcCompatMenuButtons_Update(IrcCompatMenuButtons *buttons) {
    switch (buttons->state) {
    case 0:
        buttons->pressed = 0;
        if (GFL_BMN_Main(buttons->manager) && buttons->enabled) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            buttons->frame = 0;
            buttons->state = 1;
        }
        break;
    case 1: {
        u8 flash = (buttons->frame / 4) % 2;
        u8 bg = IrcCompatMenuButtons_GetBG(buttons, buttons->touched);
        u8 x = buttons->defs[buttons->touched].x;
        u8 y = buttons->defs[buttons->touched].y;
        u8 width = buttons->defs[buttons->touched].width;
        u8 height = buttons->defs[buttons->touched].height;
        u8 normalPalette;
        u8 flashPalette;

        switch (buttons->touched) {
        case 0:
            flashPalette = 12;
            normalPalette = 11;
            break;
        case 1:
            flashPalette = 9;
            normalPalette = 4;
            break;
        }
        if (flash == 0) {
            GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, normalPalette);
            GFL_BGSysLoadScr(bg);
        } else {
            GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, flashPalette);
            GFL_BGSysLoadScr(bg);
        }
        buttons->frame++;
        if (buttons->frame >= 16) {
            buttons->pressed = 1;
            buttons->state = 0;
        }
        break;
    }
    }
}

static void IrcCompatMenuButtons_SetEnabled(IrcCompatMenuButtons *buttons, BOOL enabled) {
    buttons->enabled = enabled;
}

static BOOL IrcCompatMenuButtons_IsTouched(IrcCompatMenuButtons *buttons) {
    if (buttons->state != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IrcCompatMenuButtons_GetResult(IrcCompatMenuButtons *buttons, int *result) {
    if (buttons->pressed != 0) {
        if (result != NULL) {
            *result = buttons->touched;
        }
        return TRUE;
    }
    return FALSE;
}

static void IrcCompatMenuButtons_Reset(IrcCompatMenuButtons *buttons) {
    int i;

    buttons->state = 0;
    buttons->pressed = 0;
    for (i = 0; i < buttons->count; i++) {
        u8 bg = BmpWin_GetBGIndex(buttons->windows[i]);
        const IrcCompatMenuButtonDef *def = &buttons->defs[i];
        u8 x = def->x;
        u8 y = def->y;
        u8 width = def->width;
        u8 height = def->height;
        u8 palette;

        switch (i) {
        case 0:
            palette = 11;
            break;
        case 1:
            palette = 4;
            break;
        }
        GFL_BGSysSetScrPaletteNo(bg, x, y, width, height, palette);
        GFL_BGSysLoadScr(bg);
    }
}

static void IrcCompatMenuButtons_Callback(u32 button, u32 event, void *work) {
    IrcCompatMenuButtons *buttons = work;

    if (event == BMN_EVENT_TOUCH) {
        buttons->touched = button;
    }
}

static void IrcCompatMenuScroll_Init(IrcCompatMenuScroll *scroll, HeapID heapId) {
    sys_memset(scroll, 0, sizeof(IrcCompatMenuScroll));
    GFL_BGSysMoveBGReq(4, BG_MOVE_SET_Y, SCROLL_DISTANCE);
    GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, SCROLL_DISTANCE);
}

static void IrcCompatMenuScroll_Free(IrcCompatMenuScroll *scroll) {
    sys_memset(scroll, 0, sizeof(IrcCompatMenuScroll));
}

static void IrcCompatMenuScroll_Update(IrcCompatMenuScroll *scroll) {
    if (scroll->active) {
        if (IrcCompatMenuEase_Update(&scroll->ease)) {
            scroll->active = FALSE;
        }
        GFL_BGSysMoveBGReq(4, BG_MOVE_SET_Y, SCROLL_DISTANCE - scroll->ease.current);
        GFL_BGSysMoveBGReq(7, BG_MOVE_SET_Y, SCROLL_DISTANCE - scroll->ease.current);
    }
}

static void IrcCompatMenuScroll_Start(IrcCompatMenuScroll *scroll) {
    scroll->active = TRUE;
    IrcCompatMenuEase_Start(&scroll->ease, 0, SCROLL_DISTANCE, SCROLL_FRAMES);
}

static void IrcCompatMenuHeart_Init(IrcCompatMenuHeart *heart, ClActor *actor, BOOL isSpecial, HeapID heapId) {
    ClActorPos pos;
    u16 sequence;

    sys_memset(heart, 0, sizeof(IrcCompatMenuHeart));
    heart->actor = actor;
    if (isSpecial) {
        sequence = 13;
    } else {
        sequence = 12;
    }
    func_0204c488(heart->actor, sequence);
    IrcCompatMenuHeart_Reset(heart);
    pos.x = heart->startX;
    pos.y = heart->startY;
    func_0204c140(heart->actor, &pos, TRUE);
    func_0204c124(heart->actor, TRUE);
    func_0204c4d4(heart->actor, GFL_RandomLCAlt(1 << 13));
}

static void IrcCompatMenuHeart_Free(IrcCompatMenuHeart *heart) {
    sys_memset(heart, 0, sizeof(IrcCompatMenuHeart));
}

static void IrcCompatMenuHeart_Update(IrcCompatMenuHeart *heart) {
    ClActorPos pos;
    int diff = heart->x - heart->startX;
    s8 sign = diff / (diff < 0 ? -diff : diff);

    diff = diff < 0 ? -diff : diff;
    pos.x = heart->startX + sign * (diff * heart->frame / heart->duration);
    diff = heart->y - heart->startY;
    sign = diff / (diff < 0 ? -diff : diff);
    diff = diff < 0 ? -diff : diff;
    pos.y = heart->startY + sign * (diff * heart->frame / heart->duration);
    func_0204c140(heart->actor, &pos, TRUE);
    if (heart->frame++ > heart->duration) {
        IrcCompatMenuHeart_Reset(heart);
    }
}

static void IrcCompatMenuHeart_Reset(IrcCompatMenuHeart *heart) {
    heart->startX = GFL_RandomLCAlt(256);
    heart->startY = 208;
    heart->x = heart->startX + GFL_RandomLCAlt(184) - 92;
    heart->y = 184 - 200;
    heart->frame = 0;
    heart->duration = GFL_RandomLCAlt(184 + 176) + 240;
}

static void IrcCompatMenuHearts_Init(IrcCompatMenuHearts *hearts, IrcCompatMenuGraphic *graphic, TrainerLogSave *log,
                                     HeapID heapId) {
    int i;

    sys_memset(hearts, 0, sizeof(IrcCompatMenuHearts));
    if (log == NULL) {
        hearts->count = HEART_COUNT;
    } else {
        hearts->count = func_0200afc8(log);
    }
    for (i = 0; i < hearts->count; i++) {
        ClActor *actor = IrcCompatMenuGraphic_GetActor(graphic, i);
        BOOL isSpecial;

        if (log == NULL) {
            isSpecial = GFL_RandomLCAlt(2);
        } else {
            isSpecial = func_0200b014(log, i) == 1 ? TRUE : FALSE;
        }
        IrcCompatMenuHeart_Init(&hearts->hearts[i], actor, isSpecial, heapId);
    }
}

static void IrcCompatMenuHearts_Free(IrcCompatMenuHearts *hearts) {
    int i;

    for (i = 0; i < hearts->count; i++) {
        IrcCompatMenuHeart_Free(&hearts->hearts[i]);
    }
    sys_memset(hearts, 0, sizeof(IrcCompatMenuHearts));
}

// Sets one more heart going every 30 frames
static void IrcCompatMenuHearts_Update(IrcCompatMenuHearts *hearts) {
    int i;

    for (i = 0; i < hearts->activeCount; i++) {
        IrcCompatMenuHeart_Update(&hearts->hearts[i]);
    }
    if (hearts->timer++ > 30) {
        hearts->timer = 0;
        if (hearts->activeCount < hearts->count) {
            hearts->activeCount++;
        }
    }
}

static void IrcCompatMenuEase_Start(IrcCompatMenuEase *ease, int from, int to, int frames) {
    int diff;

    ease->current = from;
    ease->start = from;
    ease->end = to;
    ease->duration = frames;
    if (frames != 0) {
        diff = to - from;
        ease->step = FX32_CONST(diff) / frames;
        ease->frame = 0;
    } else {
        ease->frame = frames - 2;
    }
}

static BOOL IrcCompatMenuEase_Update(IrcCompatMenuEase *ease) {
    if (ease->frame < ease->duration - 1) {
        ease->frame++;
        ease->current = ease->start + FX_Whole(ease->step * ease->frame);
        return FALSE;
    }
    ease->current = ease->end;
    return TRUE;
}
