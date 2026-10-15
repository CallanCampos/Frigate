#include "types.h"
#include "app/irc_appbar.h"
#include "app/irc_compatible.h"
#include "app/irc_ranking.h"
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
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_log.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_5.h"

// The ranking of the compatibility checker: its graphics, the list of the trainers of the trainer log with their rows
// and the scrolling of the list, the stylus and the keys that scroll it, the fling it goes on with, the labels over
// the list and the steps of the screen. Names are ours

#define HEAP_ID 0x43
#define BG_COUNT 8
#define ROW_MAX TRAINER_LOG_MAX
#define ROW_HEIGHT 24
#define LABEL_COUNT 4
#define SCREEN_ROWS 3
#define SCREEN_COLUMNS 32

// The area of the list: how far it is scrolled when its first rows are at the top, and how many rows show at once
#define LIST_TOP_OFFSET (-40)
#define LIST_BOTTOM_OFFSET 24
#define LIST_VISIBLE_HEIGHT 288

// The frames that a key is held before the scroll it makes is at full speed, and the frames it takes to get there
#define KEY_SCROLL_START_FRAMES 20
#define KEY_SCROLL_FRAMES 40
#define KEY_SCROLL_FRAMES_FLING 12

// How the label of a message is placed on its bitmap
enum {
    LABEL_ALIGN_NONE,
    LABEL_ALIGN_CENTER,
    LABEL_ALIGN_CENTER_Y,
};

// The color of text, shadow and background of the labels
#define LABEL_COLOR PRINT_COLOR(1, 2, 0)

typedef struct {
    int x;
    int y;
} IrcRankingPoint;

// A trainer of the trainer log, ranked by the value the scripts match against the fortune
typedef struct {
    StrBuf *name;
    u32 value : 7;
    u32 count : 10;
    // The palette of the row, from whether the trainer is the player, the rank and whether the trainer was met today
    u32 palette : 8;
    u32 rank : 7;
    u16 species;
    u8 form;
    u8 sex;
    u8 isEgg;
    u8 isSelf;
} IrcRankingEntry;

typedef struct {
    u32 count;
    IrcRankingEntry entries[];
} IrcRankingData;

// The screen of the characters of a row, for the main engine's BGs and for the sub engine's
typedef struct {
    u16 main[SCREEN_ROWS][SCREEN_COLUMNS];
    u16 sub[SCREEN_ROWS][SCREEN_COLUMNS];
} IrcRankingScreens;

typedef struct {
    ArcTool *arc;
    u32 mainChars;
    u32 subChars;
    BmpWin *titleWindow;
} IrcRankingBG;

typedef struct {
    ClActUnit *unit;
    u32 unk4[4];
} IrcRankingClAct;

typedef struct {
    IrcRankingBG bg;
    IrcRankingClAct clact;
    TCB *vblankTask;
} IrcRankingGraphic;

// A trainer's row: the name, rank, value and count drawn on a bitmap, and the Pokémon's icon on each screen
typedef struct {
    const IrcRankingEntry *entry;
    GFLBitmap *bitmap;
    ClActor *icons[2];
    s16 iconY[2];
    u32 iconChars;
    u32 iconCellAnims;
    const IrcRankingScreens *screens;
} IrcRankingRow;

typedef struct {
    u32 iconPalettes[2];
    IrcRankingScreens screens;
    IrcRankingRow rows[ROW_MAX];
    // The BGs of the rows on each screen: two on the main engine and two on the sub engine
    u8 bgs[4];
    IrcRankingBG *bg;
    IrcRankingData *ranking;
    u16 count;
    s16 scroll;
    s16 scrollMin;
    s16 scrollMax;
    // The row at the top
    s16 topRow;
    // How far the last scroll went
    s16 delta;
    s16 prevDirection;
    s16 flag;
    // Where the BGs are scrolled to, which wraps around
    s16 offsetA;
    s16 offsetB;
    BmpWin *windows[2];
    Font *font;
    MsgData *msgData;
    u32 unk578[4];
    WordSet *wordSet;
    TCB *vblankTask;
    // Whether the rows have been redrawn and are to be sent to VRAM
    BOOL transferPending;
    // Whether the list was scrolled and is to be settled
    BOOL scrollDirty;
    // Whether the BGs are to be moved
    BOOL bgDirty;
} IrcRankingList;

// The stylus and the Control Pad that scroll the list
typedef struct {
    IrcRankingPoint start;
    IrcRankingPoint end;
    IrcRankingPoint last;
    IrcRankingPoint delta;
    u32 moveFrames;
    u32 idleFrames;
    u32 keyFrames;
} IrcRankingTouch;

// The scroll that goes on after the stylus is lifted, and slows down
typedef struct {
    fx32 strength;
    fx32 duration;
    fx32 speed;
    BOOL pending;
    s8 direction;
} IrcRankingFling;

// Text on a bitmap that shows as OAM sprites
typedef struct {
    GFLBitmap *bitmap;
    u16 color;
    BOOL busy;
    IrcRankingPoint offset;
    int align;
    StrBuf *strbuf;
    BmpOamActor *actor;
    PrintQueue *queue;
} IrcRankingLabel;

typedef struct {
    s16 x;
    s16 y;
    u16 width;
    u16 height;
} IrcRankingLabelDef;

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
} IrcRankingBGInfo;

typedef struct IrcRankingStepMan IrcRankingStepMan;
typedef struct IrcRankingWork IrcRankingWork;
typedef void (*IrcRankingStepFunc)(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk);

struct IrcRankingStepMan {
    IrcRankingStepFunc step;
    BOOL done;
    int state;
    IrcRankingWork *work;
};

struct IrcRankingWork {
    IrcRankingGraphic graphic;
    IrcRankingStepMan stepMan;
    IrcRankingList list;
    IrcRankingTouch touch;
    IrcRankingFling fling;
    IrcAppBar *button;
    Font *font;
    PrintQueue *queue;
    MsgData *msgData;
    BmpOamSys *bmpOam;
    GameData *gameData;
    IrcRankingLabel *labels[LABEL_COUNT];
    IrcRankingData *ranking;
    // How long the list has scrolled to the player's row
    s16 autoScrollFrames;
    u32 palette;
};

static BOOL IrcRanking_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcRanking_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcRanking_Main(GameProc *proc, u32 *state, void *param, void *work);
static void IrcRankingStepMan_Init(IrcRankingStepMan *mgr, IrcRankingWork *wk, IrcRankingStepFunc step);
static void IrcRankingStepMan_Free(IrcRankingStepMan *mgr);
static void IrcRankingStepMan_Run(IrcRankingStepMan *mgr);
static BOOL IrcRankingStepMan_IsDone(IrcRankingStepMan *mgr);
static void IrcRankingStepMan_SetStep(IrcRankingStepMan *mgr, IrcRankingStepFunc step);
static void IrcRankingStepMan_SetDone(IrcRankingStepMan *mgr);
static void IrcRanking_StepFadeIn(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk);
static void IrcRanking_StepFadeOut(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk);
static void IrcRanking_StepInput(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk);
static void IrcRanking_StepScrollToSelf(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk);
static void IrcRankingBG_Init(IrcRankingBG *bg, GameData *gameData, HeapID heapId);
static void IrcRankingBG_Free(IrcRankingBG *bg);
static void IrcRankingBG_VBlank(IrcRankingBG *bg);
static u32 IrcRankingBG_GetChars(IrcRankingBG *bg, BOOL main);
static void IrcRankingClAct_Init(IrcRankingClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId);
static void IrcRankingClAct_Free(IrcRankingClAct *clact);
static void IrcRankingClAct_Update(IrcRankingClAct *clact);
static void IrcRankingClAct_VBlank(IrcRankingClAct *clact);
static ClActUnit *IrcRankingClAct_GetUnit(IrcRankingClAct *clact);
static void IrcRankingGraphic_Create(IrcRankingGraphic *graphic, GameData *gameData, HeapID heapId);
static void IrcRankingGraphic_Free(IrcRankingGraphic *graphic);
static void IrcRankingGraphic_Update(IrcRankingGraphic *graphic);
static IrcRankingBG *IrcRankingGraphic_GetBG(IrcRankingGraphic *graphic);
static ClActUnit *IrcRankingGraphic_GetUnit(IrcRankingGraphic *graphic);
static void IrcRankingGraphic_VBlank(TCB *tcb, void *data);
static void IrcRankingList_Init(IrcRankingList *list, u8 bg1, u8 bg2, u8 bg5, u8 bg6, IrcRankingBG *bg, ClActUnit *unit,
                                MsgData *msgData, IrcRankingData *ranking, HeapID heapId);
static void IrcRankingList_Free(IrcRankingList *list);
static void IrcRankingList_Settle(IrcRankingList *list);
static void IrcRankingList_Move(IrcRankingList *list, s16 delta);
static s16 IrcRankingList_GetScroll(IrcRankingList *list);
static s16 IrcRankingList_FindSelf(const IrcRankingList *list);
static s16 IrcRankingList_GetScrollMax(IrcRankingList *list);
static void IrcRankingList_Redraw(IrcRankingList *list);
static void IrcRankingList_Transfer(IrcRankingList *list);
static void IrcRankingList_VBlank(TCB *tcb, void *data);
static void IrcRankingRow_Init(IrcRankingRow *row, const IrcRankingEntry *entry, ClActUnit *unit, u32 mainPalette,
                               u32 subPalette, MsgData *msgData, Font *font, WordSet *wordSet,
                               const IrcRankingScreens *screens, s16 mainY, s16 subY, HeapID heapId);
static void IrcRankingRow_Free(IrcRankingRow *row);
static void IrcRankingRow_Draw(IrcRankingRow *row, BmpWin *window, u8 bg, u16 tileY);
static void IrcRankingRow_SetScroll(IrcRankingRow *row, s16 scroll);
static void IrcRankingScreens_Init(IrcRankingScreens *screens, IrcRankingBG *bg, HeapID heapId);
static void IrcRankingScreens_Free(IrcRankingScreens *screens);
static void IrcRankingScreens_Load(const IrcRankingScreens *screens, u8 bg, u32 tileY, u8 palette);
static IrcRankingData *IrcRankingData_Create(GameData *gameData, HeapID heapId);
static void IrcRankingData_Free(IrcRankingData *ranking);
static u32 IrcRankingData_GetCount(IrcRankingData *ranking);
static void IrcRankingTouch_Init(IrcRankingTouch *touch, HeapID heapId);
static void IrcRankingTouch_Free(IrcRankingTouch *touch);
static void IrcRankingTouch_Update(IrcRankingTouch *touch);
static BOOL IrcRankingTouch_GetMove(IrcRankingTouch *touch, IrcRankingPoint *start, IrcRankingPoint *end,
                                    IrcRankingPoint *delta);
static BOOL IrcRankingTouch_GetRelease(IrcRankingTouch *touch, IrcRankingPoint *start, IrcRankingPoint *end,
                                       VecFx32 *vec, u32 *moveFrames);
static u32 IrcRankingTouch_GetHeldKeys(IrcRankingTouch *touch, u32 *keyFrames);
static void IrcRankingFling_Init(IrcRankingFling *fling);
static void IrcRankingFling_Free(IrcRankingFling *fling);
static void IrcRankingFling_Update(IrcRankingFling *fling);
static void IrcRankingFling_Start(IrcRankingFling *fling, fx32 speed, u32 frames);
static void IrcRankingFling_Stop(IrcRankingFling *fling);
static s16 IrcRankingFling_Step(IrcRankingFling *fling);
static BOOL IrcRankingFling_IsActive(IrcRankingFling *fling);
static void IrcRanking_PrintCentered(BmpWin *window, StrBuf *format, PlayerInfo *player, Font *font);
static void IrcRanking_CopyTiles(GFLBitmap *src, GFLBitmap *dest, u32 srcX, u32 srcY, u16 destX, u16 destY, u16 width,
                                 u16 height);
static IrcRankingLabel *IrcRankingLabel_Create(const ClActorSetup *setup, u8 width, u8 height, u32 palette,
                                               u8 paletteOffset, u32 surface, BmpOamSys *oam, PrintQueue *queue,
                                               HeapID heapId);
static void IrcRankingLabel_Free(IrcRankingLabel *label);
static void IrcRankingLabel_PrintMsg(IrcRankingLabel *label, MsgData *msgData, u32 msgId, Font *font);
static void IrcRankingLabel_SetColor(IrcRankingLabel *label, u16 color);
static void IrcRankingLabel_SetPos(IrcRankingLabel *label, int x, int y, int align);
static BOOL IrcRankingLabel_Update(IrcRankingLabel *label);
static void IrcRankingLabel_GetPosition(IrcRankingLabel *label, Font *font, IrcRankingPoint *pos);

const GameProcFunctions IRC_RANKING_PROC_FUNCTIONS = {
    IrcRanking_Init,
    IrcRanking_Main,
    IrcRanking_Exit,
};

static const VecFx32 sIrcRankingUpVector = { 0, FX32_ONE, 0 };

static const BGSysLCDConfig sIrcRankingLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const ClActSysSetup sIrcRankingClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 48, 48, 48, 48, 16, 16 };

static const IrcRankingLabelDef sIrcRankingLabelDefs[LABEL_COUNT] = {
    { 8, 39, 6, 2 },
    { 64, 39, 5, 2 },
    { 112, 39, 6, 2 },
    { 168, 39, 6, 2 },
};

static const BGSysVRAMConfig sIrcRankingVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const IrcRankingBGInfo sIrcRankingBGInfos[BG_COUNT] = {
    { 0,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 1,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 2,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 3,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x04000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 5,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0800), GX_BG_CHARBASE(0x08000), 0x8000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 6,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
    { 7,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1800), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT },
};

static BOOL IrcRanking_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRankingWork *wk;
    IrcCompatibleParam4 *rankingParam = param;
    ArcTool *arc;
    ClActorSetup setup;
    int i;

    GFL_HeapCreateChild(HEAPID_USER, HEAP_ID, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcRankingWork), HEAP_ID);
    sys_memset(wk, 0, sizeof(IrcRankingWork));
    if (rankingParam->gsys != NULL) {
        wk->gameData = GSYS_GetGameData(rankingParam->gsys);
    } else {
        wk->gameData = GameData_Create(HEAPID_USER);
    }
    wk->queue = func_02021998(HEAP_ID);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAP_ID);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_5, HEAP_ID);
    wk->ranking = IrcRankingData_Create(wk->gameData, HEAP_ID);
    IrcRankingGraphic_Create(&wk->graphic, wk->gameData, HEAP_ID);
    wk->bmpOam = BmpOam_Init(HEAP_ID, IrcRankingGraphic_GetUnit(&wk->graphic));
    IrcRankingStepMan_Init(&wk->stepMan, wk, IrcRanking_StepFadeIn);
    IrcRankingTouch_Init(&wk->touch, HEAP_ID);
    IrcRankingFling_Init(&wk->fling);
    IrcRankingList_Init(&wk->list, 1, 2, 5, 6, IrcRankingGraphic_GetBG(&wk->graphic),
                        IrcRankingGraphic_GetUnit(&wk->graphic), wk->msgData, wk->ranking, HEAP_ID);
    wk->button = IrcAppBar_Create(2, IrcRankingGraphic_GetUnit(&wk->graphic), 4, 13, 0, 2, wk->font, wk->queue,
                                  HEAP_ID);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, HEAP_ID);
    wk->palette = func_0204bc48(arc, 5, CLACT_VRAM_MAIN, 0x1c0, HEAP_ID);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 8;
    setup.y = 41;
    setup.priority = 0;
    setup.bgPriority = 0;
    for (i = 0; i < LABEL_COUNT; i++) {
        setup.x = sIrcRankingLabelDefs[i].x;
        setup.y = sIrcRankingLabelDefs[i].y;
        wk->labels[i] = IrcRankingLabel_Create(&setup, sIrcRankingLabelDefs[i].width, sIrcRankingLabelDefs[i].height,
                                               wk->palette, 0, 0, wk->bmpOam, wk->queue, HEAP_ID);
        IrcRankingLabel_SetColor(wk->labels[i], LABEL_COLOR);
        IrcRankingLabel_SetPos(wk->labels[i], 0, 0, LABEL_ALIGN_CENTER);
        IrcRankingLabel_PrintMsg(wk->labels[i], wk->msgData, i + 1, wk->font);
    }
    return TRUE;
}

static BOOL IrcRanking_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRankingWork *wk = work;
    IrcCompatibleParam4 *rankingParam = param;
    int i;

    for (i = 0; i < LABEL_COUNT; i++) {
        IrcRankingLabel_Free(wk->labels[i]);
    }
    func_0204bcd0(wk->palette);
    IrcAppBar_Free(wk->button);
    IrcRankingList_Free(&wk->list);
    IrcRankingFling_Free(&wk->fling);
    IrcRankingTouch_Free(&wk->touch);
    IrcRankingStepMan_Free(&wk->stepMan);
    BmpOam_Exit(wk->bmpOam);
    IrcRankingGraphic_Free(&wk->graphic);
    IrcRankingData_Free(wk->ranking);
    GFL_MsgDataFree(wk->msgData);
    GFL_FontFree(wk->font);
    func_02021a18(wk->queue);
    if (rankingParam->gsys == NULL) {
        GameData_Free(wk->gameData);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAP_ID);
    return TRUE;
}

static BOOL IrcRanking_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcRankingWork *wk = work;
    int i;

    IrcRankingStepMan_Run(&wk->stepMan);
    IrcRankingGraphic_Update(&wk->graphic);
    func_02021a3c(wk->queue);
    for (i = 0; i < LABEL_COUNT; i++) {
        IrcRankingLabel_Update(wk->labels[i]);
    }
    if (IrcRankingStepMan_IsDone(&wk->stepMan)) {
        return TRUE;
    }
    return FALSE;
}

static void IrcRankingStepMan_Init(IrcRankingStepMan *mgr, IrcRankingWork *wk, IrcRankingStepFunc step) {
    sys_memset(mgr, 0, sizeof(IrcRankingStepMan));
    mgr->work = wk;
    IrcRankingStepMan_SetStep(mgr, step);
}

static void IrcRankingStepMan_Free(IrcRankingStepMan *mgr) {
    sys_memset(mgr, 0, sizeof(IrcRankingStepMan));
}

static void IrcRankingStepMan_Run(IrcRankingStepMan *mgr) {
    if (!mgr->done) {
        mgr->step(mgr, &mgr->state, mgr->work);
    }
}

static BOOL IrcRankingStepMan_IsDone(IrcRankingStepMan *mgr) {
    return mgr->done;
}

static void IrcRankingStepMan_SetStep(IrcRankingStepMan *mgr, IrcRankingStepFunc step) {
    mgr->step = step;
    mgr->state = 0;
}

static void IrcRankingStepMan_SetDone(IrcRankingStepMan *mgr) {
    mgr->done = TRUE;
}

static void IrcRanking_StepFadeIn(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        // BUG: the list reports -1 when the player is not in it, and this compares with 0xffff
#ifdef BUGFIX
        if (IrcRankingList_FindSelf(&wk->list) != -1) {
#else
        if (IrcRankingList_FindSelf(&wk->list) != 0xffff) {
#endif
            IrcRankingStepMan_SetStep(&wk->stepMan, IrcRanking_StepScrollToSelf);
        } else {
            IrcRankingStepMan_SetStep(mgr, IrcRanking_StepInput);
        }
        break;
    }
}

static void IrcRanking_StepFadeOut(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk) {
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 0, 16, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        IrcRankingStepMan_SetDone(mgr);
        break;
    }
}

// Scrolls the list with the stylus, the Control Pad and the fling that follows
static void IrcRanking_StepInput(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk) {
    u32 keyFrames;
    IrcRankingPoint delta;
    u32 moveFrames;
    VecFx32 release;

    if (!IrcAppBar_IsPressedOrDone(wk->button)) {
        if (IrcRankingTouch_GetHeldKeys(&wk->touch, &keyFrames) & PAD_KEY_DOWN) {
            if (FX32_CONST(keyFrames) > FX32_CONST(KEY_SCROLL_START_FRAMES)) {
                IrcRankingFling_Start(
                    &wk->fling,
                    FX_Mul(FX_Div(FX32_CONST(keyFrames) - FX32_CONST(KEY_SCROLL_START_FRAMES),
                                  FX32_CONST(KEY_SCROLL_FRAMES)),
                           FX32_CONST(60)),
                    KEY_SCROLL_FRAMES_FLING);
            } else {
                IrcRankingFling_Start(&wk->fling, FX32_CONST(2), KEY_SCROLL_FRAMES_FLING);
            }
        } else if (IrcRankingTouch_GetHeldKeys(&wk->touch, &keyFrames) & PAD_KEY_UP) {
            if (FX32_CONST(keyFrames) > FX32_CONST(KEY_SCROLL_START_FRAMES)) {
                IrcRankingFling_Start(
                    &wk->fling,
                    -FX_Mul(FX_Div(FX32_CONST(keyFrames) - FX32_CONST(KEY_SCROLL_START_FRAMES),
                                   FX32_CONST(KEY_SCROLL_FRAMES)),
                            FX32_CONST(60)),
                    KEY_SCROLL_FRAMES_FLING);
            } else {
                IrcRankingFling_Start(&wk->fling, -FX32_CONST(2), KEY_SCROLL_FRAMES_FLING);
            }
        } else if (IrcRankingTouch_GetMove(&wk->touch, NULL, NULL, &delta)) {
            IrcRankingList_Move(&wk->list, -delta.y);
            IrcRankingFling_Stop(&wk->fling);
        } else if (IrcRankingTouch_GetRelease(&wk->touch, NULL, NULL, &release, &moveFrames)) {
            IrcRankingFling_Start(&wk->fling, VEC_DotProduct(&sIrcRankingUpVector, &release), moveFrames);
        }
    }
    if (IrcRankingFling_IsActive(&wk->fling)) {
        IrcRankingList_Move(&wk->list, IrcRankingFling_Step(&wk->fling));
    }
    if (IrcAppBar_GetResult(wk->button) == 1) {
        IrcRankingStepMan_SetStep(mgr, IrcRanking_StepFadeOut);
    }
    IrcRankingList_Settle(&wk->list);
    IrcRankingTouch_Update(&wk->touch);
    IrcRankingFling_Update(&wk->fling);
    IrcAppBar_Update(wk->button);
}

// Scrolls the list to the player's row, faster the longer it takes
static void IrcRanking_StepScrollToSelf(IrcRankingStepMan *mgr, int *state, IrcRankingWork *wk) {
    s16 target = IrcRankingList_FindSelf(&wk->list) - ROW_HEIGHT * 8;
    s16 scroll;
    s16 scrollMax;
    s16 frames;

    target = MATH_MAX(target, 0);
    scroll = IrcRankingList_GetScroll(&wk->list);
    scrollMax = IrcRankingList_GetScrollMax(&wk->list);
    if (target > scroll && scroll != scrollMax) {
        wk->autoScrollFrames++;
        frames = wk->autoScrollFrames;
        if (FX32_CONST(frames) > FX32_CONST(KEY_SCROLL_START_FRAMES)) {
            IrcRankingFling_Start(&wk->fling,
                                  FX_Mul(FX_Div(FX32_CONST(frames) - FX32_CONST(KEY_SCROLL_START_FRAMES),
                                                FX32_CONST(KEY_SCROLL_FRAMES)),
                                         FX32_CONST(60)),
                                  KEY_SCROLL_FRAMES_FLING);
        } else {
            IrcRankingFling_Start(&wk->fling, FX32_CONST(2), KEY_SCROLL_FRAMES_FLING);
        }
    } else {
        IrcRankingFling_Stop(&wk->fling);
        IrcRankingStepMan_SetStep(&wk->stepMan, IrcRanking_StepInput);
    }
    if (IrcRankingFling_IsActive(&wk->fling)) {
        IrcRankingList_Move(&wk->list, IrcRankingFling_Step(&wk->fling));
    }
    if (IrcAppBar_GetResult(wk->button) == 1) {
        IrcRankingStepMan_SetStep(mgr, IrcRanking_StepFadeOut);
    }
    IrcRankingList_Settle(&wk->list);
    IrcRankingFling_Update(&wk->fling);
    IrcAppBar_Update(wk->button);
}

static void IrcRankingBG_Init(IrcRankingBG *bg, GameData *gameData, HeapID heapId) {
    MsgData *msgData;
    Font *font;
    StrBuf *format;
    int i;

    sys_memset(bg, 0, sizeof(IrcRankingBG));
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_BGSysSetLCDConfig(&sIrcRankingLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysCreateBG(sIrcRankingBGInfos[i].bg, &sIrcRankingBGInfos[i].setup, sIrcRankingBGInfos[i].mode);
        GFL_BGSysClearBG(sIrcRankingBGInfos[i].bg);
        GFL_BGSysSetBGEnabled(sIrcRankingBGInfos[i].bg, TRUE);
    }
    bg->arc = GFL_ArcSysCreateFileHandle(ARCID_IRC_RANKING, heapId);
    GFL_G2DIOLoadArcNCLRDefault(bg->arc, 0, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(bg->arc, 1, 3, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(bg->arc, 3, 0, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(bg->arc, 4, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(bg->arc, 6, 0, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(bg->arc, 0, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(bg->arc, 3, 7, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(bg->arc, 7, 7, 0, 0, FALSE, heapId);
    GFL_BGSysFillChar(1, 0, 1, 0);
    GFL_BGSysFillChar(5, 0, 1, 0);
    bg->mainChars = GFL_BGSysLoadArcNCGRDynamic(bg->arc, 2, 1, 0, FALSE, heapId);
    bg->subChars = GFL_BGSysLoadArcNCGRDynamic(bg->arc, 2, 5, 0, FALSE, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x1c0, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x1c0, 0x20, heapId);
    GFL_ArcToolFree(bg->arc);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_5, heapId);
    font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    bg->titleWindow = BmpWin_CreateDynamic(0, 3, 1, 26, 2, 14, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(bg->titleWindow), 15);
    format = GFL_MsgDataLoadStrbufNew(msgData, BtlMain5_Text_SFeelingRanking);
    IrcRanking_PrintCentered(bg->titleWindow, format, GetGameDataPlayerInfo(gameData), font);
    GFL_StrBufFree(format);
    GFL_FontFree(font);
    GFL_MsgDataFree(msgData);
    BmpWin_FlushChar(bg->titleWindow);
    BmpWin_FlushMap(bg->titleWindow);
    GFL_BGSysLoadScr(0);
}

static void IrcRankingBG_Free(IrcRankingBG *bg) {
    int i;

    BmpWin_Free(bg->titleWindow);
    GFL_BGSysFreeCharMemory(1, (u16)bg->mainChars, (u16)(bg->mainChars >> 16));
    GFL_BGSysFreeCharMemory(5, (u16)bg->subChars, (u16)(bg->subChars >> 16));
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeFilledChar(1, 1, 0);
    for (i = 0; i < BG_COUNT; i++) {
        GFL_BGSysReleaseBG(sIrcRankingBGInfos[i].bg);
    }
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    sys_memset(bg, 0, sizeof(IrcRankingBG));
}

static void IrcRankingBG_VBlank(IrcRankingBG *bg) {
    GFL_BGSysUpdate();
}

static u32 IrcRankingBG_GetChars(IrcRankingBG *bg, BOOL main) {
    if (main) {
        return bg->mainChars;
    }
    return bg->subChars;
}

static void IrcRankingClAct_Init(IrcRankingClAct *clact, const BGSysVRAMConfig *vramConfig, HeapID heapId) {
    sys_memset(clact, 0, sizeof(IrcRankingClAct));
    ClActSys_Create(&sIrcRankingClActSetup, vramConfig, heapId);
    clact->unit = func_0204bf1c(128, 0, heapId);
    func_0204c028(clact->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void IrcRankingClAct_Free(IrcRankingClAct *clact) {
    func_0204bf98(clact->unit);
    func_0204b758();
    sys_memset(clact, 0, sizeof(IrcRankingClAct));
}

static void IrcRankingClAct_Update(IrcRankingClAct *clact) {
    func_0204b794();
}

static void IrcRankingClAct_VBlank(IrcRankingClAct *clact) {
    func_0204b7c8();
}

static ClActUnit *IrcRankingClAct_GetUnit(IrcRankingClAct *clact) {
    return clact->unit;
}

static void IrcRankingGraphic_Create(IrcRankingGraphic *graphic, GameData *gameData, HeapID heapId) {
    sys_memset(graphic, 0, sizeof(IrcRankingGraphic));
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sIrcRankingVRAMConfig);
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    func_020232d0();
    IrcRankingBG_Init(&graphic->bg, gameData, heapId);
    IrcRankingClAct_Init(&graphic->clact, &sIrcRankingVRAMConfig, heapId);
    graphic->vblankTask = GFL_VBlankTCBAdd(IrcRankingGraphic_VBlank, graphic, 0);
}

static void IrcRankingGraphic_Free(IrcRankingGraphic *graphic) {
    GFL_TCBRemove(graphic->vblankTask);
    IrcRankingClAct_Free(&graphic->clact);
    IrcRankingBG_Free(&graphic->bg);
    sys_memset(graphic, 0, sizeof(IrcRankingGraphic));
}

static void IrcRankingGraphic_Update(IrcRankingGraphic *graphic) {
    IrcRankingClAct_Update(&graphic->clact);
}

static IrcRankingBG *IrcRankingGraphic_GetBG(IrcRankingGraphic *graphic) {
    return &graphic->bg;
}

static ClActUnit *IrcRankingGraphic_GetUnit(IrcRankingGraphic *graphic) {
    return IrcRankingClAct_GetUnit(&graphic->clact);
}

static void IrcRankingGraphic_VBlank(TCB *tcb, void *data) {
    IrcRankingGraphic *graphic = data;

    IrcRankingBG_VBlank(&graphic->bg);
    IrcRankingClAct_VBlank(&graphic->clact);
}

static void IrcRankingList_Init(IrcRankingList *list, u8 bg1, u8 bg2, u8 bg5, u8 bg6, IrcRankingBG *bg, ClActUnit *unit,
                                MsgData *msgData, IrcRankingData *ranking, HeapID heapId) {
    ArcTool *arc;
    u32 i;

    sys_memset(list, 0, sizeof(IrcRankingList));
    list->bg = bg;
    list->bgs[0] = bg1;
    list->bgs[1] = bg2;
    list->bgs[2] = bg5;
    list->bgs[3] = bg6;
    list->ranking = ranking;
    list->count = IrcRankingData_GetCount(ranking);
    list->font = GFL_FontCreate(ARCID_FONT, 0, 0, TRUE, heapId);
    list->msgData = msgData;
    list->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    IrcRankingScreens_Init(&list->screens, bg, heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    list->iconPalettes[0] = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 0x20, heapId);
    list->iconPalettes[1] = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0x20, heapId);
    GFL_ArcToolFree(arc);
    for (i = 0; i < IrcRankingData_GetCount(ranking); i++) {
        IrcRankingRow_Init(&list->rows[i], &ranking->entries[i], unit, list->iconPalettes[0], list->iconPalettes[1],
                           list->msgData, list->font, list->wordSet, &list->screens, i * ROW_HEIGHT + 82,
                           i * ROW_HEIGHT - 112, heapId);
    }
    list->windows[0] = BmpWin_CreateDynamic(bg2, 0, 0, 32, 32, 14, TRUE);
    list->windows[1] = BmpWin_CreateDynamic(bg6, 0, 0, 32, 32, 14, TRUE);
    list->offsetA = LIST_TOP_OFFSET;
    list->offsetB = LIST_BOTTOM_OFFSET;
    GFL_BGSysMoveBG(list->bgs[0], BG_MOVE_SET_Y, list->offsetA);
    GFL_BGSysMoveBG(list->bgs[1], BG_MOVE_SET_Y, list->offsetA);
    GFL_BGSysMoveBG(list->bgs[2], BG_MOVE_SET_Y, list->offsetB);
    GFL_BGSysMoveBG(list->bgs[3], BG_MOVE_SET_Y, list->offsetB);
    list->scroll = 0;
    list->scrollMin = 0;
    list->scrollMax = list->count * ROW_HEIGHT - LIST_VISIBLE_HEIGHT;
    list->scrollMax = MATH_MAX(0, list->scrollMax);
    IrcRankingList_Redraw(list);
    IrcRankingList_Transfer(list);
    list->vblankTask = GFL_VBlankTCBAdd(IrcRankingList_VBlank, list, 0);
}

static void IrcRankingList_Free(IrcRankingList *list) {
    int i;
    u32 row;

    GFL_TCBRemove(list->vblankTask);
    for (i = 0; i < 2; i++) {
        BmpWin_Free(list->windows[i]);
    }
    for (row = 0; row < IrcRankingData_GetCount(list->ranking); row++) {
        IrcRankingRow_Free(&list->rows[row]);
    }
    func_0204bcd0(list->iconPalettes[0]);
    func_0204bcd0(list->iconPalettes[1]);
    IrcRankingScreens_Free(&list->screens);
    GFL_WordSetSystemFree(list->wordSet);
    GFL_FontFree(list->font);
    sys_memset(list, 0, sizeof(IrcRankingList));
}

// Once the list has stopped on a row, redraws it for the rows that show, and plays a sound for a new one
static void IrcRankingList_Settle(IrcRankingList *list) {
    s16 scroll;

    if (list->scrollDirty) {
        list->scrollDirty = FALSE;
        scroll = list->scroll;
        if (scroll % ROW_HEIGHT == 0) {
            if (list->topRow != scroll / ROW_HEIGHT) {
                GFL_SndSEPlay(SEQ_SE_SYS_06);
            }
            list->topRow = list->scroll / ROW_HEIGHT;
            list->offsetA = LIST_BOTTOM_OFFSET - 64;
            list->offsetB = ROW_HEIGHT;
            IrcRankingList_Redraw(list);
            list->transferPending = TRUE;
        } else {
            list->bgDirty = TRUE;
        }
    }
}

static void IrcRankingList_Move(IrcRankingList *list, s16 delta) {
    s16 scroll = list->scroll;
    int next = scroll + delta;
    int i;

    if (list->scrollMin > next) {
        list->scroll = list->scrollMin;
        list->delta = 0;
    } else if (list->scrollMax < next) {
        list->scroll = list->scrollMax;
        list->delta = 0;
    } else {
        if (scroll / ROW_HEIGHT != next / ROW_HEIGHT && scroll % ROW_HEIGHT != 0) {
            if (delta >= 0) {
                delta = delta - next % ROW_HEIGHT;
            } else {
                delta = delta + (ROW_HEIGHT - next % ROW_HEIGHT);
            }
        }
        list->scroll += delta;
        list->delta = delta;
    }
    for (i = 0; i < list->count; i++) {
        IrcRankingRow_SetScroll(&list->rows[i], list->scroll);
    }
    list->scrollDirty = TRUE;
}

static s16 IrcRankingList_GetScroll(IrcRankingList *list) {
    return list->scroll;
}

// The scroll of the player's row, or -1
static s16 IrcRankingList_FindSelf(const IrcRankingList *list) {
    int i;
    BOOL found = FALSE;

    for (i = 0; i < list->count; i++) {
        if (list->rows[i].entry->isSelf) {
            found = TRUE;
            break;
        }
    }
    if (found) {
        return ROW_HEIGHT * i;
    }
    return -1;
}

static s16 IrcRankingList_GetScrollMax(IrcRankingList *list) {
    return list->scrollMax;
}

static void IrcRankingList_Redraw(IrcRankingList *list) {
    int i;

    GFL_BGSysFillScrArea(list->bgs[0], 0, 0, 0, 32, 24, 0);
    GFL_BGSysFillScrArea(list->bgs[2], 0, 0, 0, 32, 24, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(list->windows[0]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(list->windows[1]), 0);
    for (i = list->topRow - 1; i < list->topRow + 10; i++) {
        if (i >= 0 && i < list->count) {
            int y = (i - list->topRow) * 3 + 4;

            if (y >= 0 && y <= 24) {
                IrcRankingRow_Draw(&list->rows[i], list->windows[0], list->bgs[0], y);
            }
        }
    }
    for (i = list->topRow + 4; i < list->topRow + 17; i++) {
        if (i >= 0 && i < list->count) {
            int y = (i - (list->topRow + 4)) * 3;

            if (y >= 0 && y <= 24) {
                IrcRankingRow_Draw(&list->rows[i], list->windows[1], list->bgs[2], y);
            }
        }
    }
}

static void IrcRankingList_Transfer(IrcRankingList *list) {
    int i;

    for (i = 0; i < 2; i++) {
        BmpWin_FlushChar(list->windows[i]);
        BmpWin_FlushMap(list->windows[i]);
    }
    GFL_BGSysLoadScr(list->bgs[0]);
    GFL_BGSysLoadScr(list->bgs[2]);
    GFL_BGSysLoadScr(list->bgs[1]);
    GFL_BGSysLoadScr(list->bgs[3]);
}

static void IrcRankingList_VBlank(TCB *tcb, void *data) {
    IrcRankingList *list = data;
    s16 x;
    s16 y;
    int rem;

    if (list->transferPending) {
        IrcRankingList_Transfer(list);
        list->transferPending = FALSE;
        list->bgDirty = TRUE;
    }
    if (list->bgDirty) {
        rem = (s16)(list->scroll % ROW_HEIGHT);
        if (rem == 0) {
            x = list->offsetA + rem;
            y = list->offsetB + rem;
            list->prevDirection = list->delta;
            list->flag = 1;
        } else if ((list->delta >= 0 && list->prevDirection >= 0) || (list->delta < 0 && list->prevDirection < 0) ||
                   list->flag != 0) {
            // The list goes on the way it went, or has just stood on a row: the way it goes now decides
            list->flag = 0;
            if (list->delta >= 0) {
                x = list->offsetA + rem;
                y = list->offsetB + rem;
                list->prevDirection = 1;
            } else {
                x = list->offsetA - (ROW_HEIGHT - 1 - rem);
                y = list->offsetB - (ROW_HEIGHT - 1 - rem);
                list->prevDirection = -1;
            }
        } else {
            // It has turned around within a row: the way it went before stays
            if (list->prevDirection >= 0) {
                x = list->offsetA + rem;
                y = list->offsetB + rem;
                list->prevDirection = 1;
            } else {
                x = list->offsetA - (ROW_HEIGHT - 1 - rem);
                y = list->offsetB - (ROW_HEIGHT - 1 - rem);
                list->prevDirection = -1;
            }
        }
        GFL_BGSysMoveBG(list->bgs[0], BG_MOVE_SET_Y, x);
        GFL_BGSysMoveBG(list->bgs[1], BG_MOVE_SET_Y, x);
        GFL_BGSysMoveBG(list->bgs[2], BG_MOVE_SET_Y, y);
        GFL_BGSysMoveBG(list->bgs[3], BG_MOVE_SET_Y, y);
        list->bgDirty = FALSE;
    }
}

static void IrcRankingRow_Init(IrcRankingRow *row, const IrcRankingEntry *entry, ClActUnit *unit, u32 mainPalette,
                               u32 subPalette, MsgData *msgData, Font *font, WordSet *wordSet,
                               const IrcRankingScreens *screens, s16 mainY, s16 subY, HeapID heapId) {
    StrBuf *rankFormat;
    StrBuf *valueFormat;
    StrBuf *countFormat;
    StrBuf *text;
    u32 iconFile;
    u32 iconCells;
    u32 iconAnims;
    ArcTool *arc;
    ClActorSetup setup;
    int i;
    u32 halfWidth;
    u32 palette;

    sys_memset(row, 0, sizeof(IrcRankingRow));
    row->entry = entry;
    row->screens = screens;
    row->bitmap = GFL_BitmapCreate(32, 3, 32, heapId);
    rankFormat = GFL_StrBufCreate(32, heapId);
    valueFormat = GFL_StrBufCreate(32, heapId);
    countFormat = GFL_StrBufCreate(32, heapId);
    text = GFL_StrBufCreate(32, heapId);
    GFL_MsgDataLoadStrbuf(msgData, 5, rankFormat);
    GFL_MsgDataLoadStrbuf(msgData, 7, valueFormat);
    GFL_MsgDataLoadStrbuf(msgData, 8, countFormat);
    WordSetNumber(wordSet, 0, entry->rank, 2, 0, TRUE);
    WordSetNumber(wordSet, 2, entry->value, 3, 0, TRUE);
    WordSetNumber(wordSet, 3, entry->count, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, text, rankFormat);
    halfWidth = GFL_FontGetBlockWidth(text, font, 0) / 2;
    GFL_TextRendererDrawToBitmap(row->bitmap, 38 - halfWidth, 5, text, font);
    GFL_TextRendererDrawToBitmap(row->bitmap, 52, 5, entry->name, font);
    GFL_WordSetFormatStrbuf(wordSet, text, valueFormat);
    halfWidth = GFL_FontGetBlockWidth(text, font, 0) / 2;
    GFL_TextRendererDrawToBitmap(row->bitmap, 142 - halfWidth, 5, text, font);
    GFL_WordSetFormatStrbuf(wordSet, text, countFormat);
    halfWidth = GFL_FontGetBlockWidth(text, font, 0) / 2;
    GFL_TextRendererDrawToBitmap(row->bitmap, 194 - halfWidth, 5, text, font);
    GFL_StrBufFree(text);
    GFL_StrBufFree(countFormat);
    GFL_StrBufFree(valueFormat);
    GFL_StrBufFree(rankFormat);
    iconFile = PokeParty_GetIconIndex(entry->species, entry->form, entry->sex, entry->isEgg);
    iconCells = PokeIcon_GetCellFile_Main();
    iconAnims = getOBJTileMapping_MainEng();
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    row->iconChars = func_0204b81c(arc, iconFile, FALSE, CLACT_VRAM_BOTH, heapId);
    row->iconCellAnims = func_0204bde0(arc, iconCells, iconAnims, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.sequence = 1;
    setup.bgPriority = 1;
    for (i = 0; i < 2; i++) {
        setup.x = 232;
        if (i == 0) {
            setup.y = mainY;
            palette = mainPalette;
        } else {
            setup.y = subY;
            palette = subPalette;
        }
        row->iconY[i] = setup.y;
        row->icons[i] = func_0204c040(unit, row->iconChars, palette, row->iconCellAnims,
                                      &setup, i, heapId);
        func_0204c378(row->icons[i], PokeIcon_GetPalette(entry->species, entry->form, entry->sex, entry->isEgg), FALSE);
        if (setup.y > -16 && setup.y < 208) {
            func_0204c124(row->icons[i], TRUE);
        } else {
            func_0204c124(row->icons[i], FALSE);
        }
    }
}

static void IrcRankingRow_Free(IrcRankingRow *row) {
    int i;

    for (i = 0; i < 2; i++) {
        func_0204c108(row->icons[i]);
    }
    func_0204b98c(row->iconChars);
    func_0204be64(row->iconCellAnims);
    GFL_BitmapFree(row->bitmap);
    sys_memset(row, 0, sizeof(IrcRankingRow));
}

static void IrcRankingRow_Draw(IrcRankingRow *row, BmpWin *window, u8 bg, u16 tileY) {
    IrcRanking_CopyTiles(row->bitmap, BmpWin_GetBitmap(window), 0, 0, 0, tileY, 32, 3);
    IrcRankingScreens_Load(row->screens, bg, tileY, row->entry->palette);
}

static void IrcRankingRow_SetScroll(IrcRankingRow *row, s16 scroll) {
    ClActorPos pos;
    int offset[2];
    int i;

    offset[0] = -scroll;
    offset[1] = -scroll;
    for (i = 0; i < 2; i++) {
        func_0204c178(row->icons[i], &pos, i);
        pos.y = offset[i] + row->iconY[i];
        func_0204c140(row->icons[i], &pos, i);
        if (pos.y > -16 && pos.y < 208) {
            func_0204c124(row->icons[i], TRUE);
        } else {
            func_0204c124(row->icons[i], FALSE);
        }
    }
}

static void IrcRankingScreens_Init(IrcRankingScreens *screens, IrcRankingBG *bg, HeapID heapId) {
    NNSG2dScreenData *screen;
    void *file;
    u16 mainChars;
    u16 subChars;
    u16 *src;
    int y;
    int x;

    sys_memset(screens, 0, sizeof(IrcRankingScreens));
    mainChars = IrcRankingBG_GetChars(bg, TRUE);
    subChars = IrcRankingBG_GetChars(bg, FALSE);
    file = GFL_G2DIOReadNSCR(ARCID_IRC_RANKING, 5, FALSE, &screen, heapId);
    src = (u16 *)screen->rawData;
    for (y = 0; y < SCREEN_ROWS; y++) {
        for (x = 0; x < SCREEN_COLUMNS; x++) {
            screens->main[y][x] = src[y * SCREEN_COLUMNS + x] + mainChars;
            screens->sub[y][x] = src[y * SCREEN_COLUMNS + x] + subChars;
        }
    }
    GFL_HeapFree(file);
}

static void IrcRankingScreens_Free(IrcRankingScreens *screens) {
    sys_memset(screens, 0, sizeof(IrcRankingScreens));
}

static void IrcRankingScreens_Load(const IrcRankingScreens *screens, u8 bg, u32 tileY, u8 palette) {
    const u16 *src = screens->main[0];

    if (bg >= 4) {
        src = screens->sub[0];
    }
    GFL_BGSysLoadScrAreaAll(bg, src, 0, tileY, 32, 3);
    GFL_BGSysSetScrPaletteNo(bg, 0, tileY, 32, 3, palette);
}

// Reads the trainer log, and ranks its trainers by value
static IrcRankingData *IrcRankingData_Create(GameData *gameData, HeapID heapId) {
    TrainerLogSave *log = func_0200afbc(GameData_GetSaveControl(gameData));
    u16 count = func_0200afc8(log);
    IrcRankingData *ranking = GFL_HeapAllocate(heapId, sizeof(u32) + count * sizeof(IrcRankingEntry), FALSE,
                                               "irc_ranking.c", 2724);
    IrcRankingEntry *entry;
    u8 prevValue;
    u8 prevRank;
    u8 self;
    u32 i;

    sys_memset(ranking, 0, sizeof(u32) + count * sizeof(IrcRankingEntry));
    ranking->count = count;
    if (count != 0) {
        prevValue = 0x80;
        prevRank = 0;
        self = func_0200b04c(log);
        for (i = 0; i < ranking->count; i++) {
            u8 met;

            entry = &ranking->entries[i];
            entry->name = GFL_StrBufCreate(8, heapId);
            GFL_StrBufLoadString(entry->name, func_0200afe4(log, i));
            entry->value = func_0200afec(log, i);
            entry->count = func_0200b000(log, i);
            if (entry->value < prevValue) {
                entry->rank = i + 1;
            } else if (entry->value == prevValue) {
                entry->rank = prevRank;
            }
            met = func_0200b014(log, i);
            if (self == i) {
                u32 base = 1;

                if (met) {
                    base = 0;
                }
                entry->palette = (base + 1) * 5;
                entry->isSelf = TRUE;
            } else if (entry->rank == 1) {
                u32 base = 0;

                if (!met) {
                    base = 5;
                }
                entry->palette = base + 2;
            } else if (entry->rank == 2) {
                u32 base = 0;

                if (!met) {
                    base = 5;
                }
                entry->palette = base + 3;
            } else if (entry->rank == 3) {
                u32 base = 0;

                if (!met) {
                    base = 5;
                }
                entry->palette = base + 4;
            } else {
                u32 base = 0;

                if (!met) {
                    base = 5;
                }
                entry->palette = base + 1;
            }
            func_0200b024(log, i, &entry->species, &entry->form, &entry->sex, &entry->isEgg);
            prevValue = entry->value;
            prevRank = entry->rank;
        }
    }
    return ranking;
}

static void IrcRankingData_Free(IrcRankingData *ranking) {
    u32 i;

    if (ranking->count != 0) {
        for (i = 0; i < ranking->count; i++) {
            if (ranking->entries[i].name != NULL) {
                GFL_StrBufFree(ranking->entries[i].name);
            }
        }
    }
    GFL_HeapFree(ranking);
}

static u32 IrcRankingData_GetCount(IrcRankingData *ranking) {
    return ranking->count;
}

static void IrcRankingTouch_Init(IrcRankingTouch *touch, HeapID heapId) {
    sys_memset(touch, 0, sizeof(IrcRankingTouch));
}

static void IrcRankingTouch_Free(IrcRankingTouch *touch) {
    sys_memset(touch, 0, sizeof(IrcRankingTouch));
}

static void IrcRankingTouch_Update(IrcRankingTouch *touch) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y)) {
        touch->start.x = x;
        touch->start.y = y;
        touch->end.x = x;
        touch->end.y = y;
        touch->last.x = x;
        touch->last.y = y;
        touch->delta.x = 0;
        touch->delta.y = 0;
        touch->moveFrames = 0;
    } else if (func_0203da84(&x, &y)) {
        int diff = y - touch->start.y;

        if (MATH_ABS(diff) >= 1) {
            touch->delta.x = x - touch->last.x;
            touch->delta.y = y - touch->last.y;
            touch->last.x = x;
            touch->last.y = y;
        }
        touch->end.x = x;
        touch->end.y = y;
        touch->moveFrames++;
        touch->idleFrames = 0;
    } else {
        touch->idleFrames++;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_KEY_UP | PAD_KEY_DOWN)) {
        touch->keyFrames = 0;
        return;
    }
    if (GCTX_HIDGetHeldKeys() & (PAD_KEY_UP | PAD_KEY_DOWN)) {
        touch->keyFrames++;
    }
}

static BOOL IrcRankingTouch_GetMove(IrcRankingTouch *touch, IrcRankingPoint *start, IrcRankingPoint *end,
                                    IrcRankingPoint *delta) {
    if (func_0203da2c()) {
        if (start != NULL) {
            *start = touch->start;
        }
        if (end != NULL) {
            *end = touch->end;
        }
        if (delta != NULL) {
            *delta = touch->delta;
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL IrcRankingTouch_GetRelease(IrcRankingTouch *touch, IrcRankingPoint *start, IrcRankingPoint *end,
                                       VecFx32 *vec, u32 *moveFrames) {
    VecFx32 from;
    VecFx32 to;

    if (GCTX_HIDGetHeldKeys() == 0 && touch->idleFrames < 1) {
        touch->idleFrames = 1;
        if (start != NULL) {
            *start = touch->start;
        }
        if (end != NULL) {
            *end = touch->end;
        }
        if (vec != NULL) {
            VEC_Set(&from, FX32_CONST(touch->start.x), FX32_CONST(touch->start.y), 0);
            VEC_Set(&to, FX32_CONST(touch->end.x), FX32_CONST(touch->end.y), 0);
            VEC_Subtract(&from, &to, vec);
        }
        if (moveFrames != NULL) {
            *moveFrames = touch->moveFrames;
        }
        return TRUE;
    }
    return FALSE;
}

static u32 IrcRankingTouch_GetHeldKeys(IrcRankingTouch *touch, u32 *keyFrames) {
    if (keyFrames != NULL) {
        *keyFrames = touch->keyFrames;
    }
    return GCTX_HIDGetHeldKeys();
}

static void IrcRankingFling_Init(IrcRankingFling *fling) {
    sys_memset(fling, 0, sizeof(IrcRankingFling));
}

static void IrcRankingFling_Free(IrcRankingFling *fling) {
    sys_memset(fling, 0, sizeof(IrcRankingFling));
}

// Turns a fling that was started into its speed
static void IrcRankingFling_Update(IrcRankingFling *fling) {
    if (fling->pending) {
        fling->pending = FALSE;
        fling->speed = fling->strength + fling->duration / 2;
        fling->speed = FX_Mul(fling->speed, FX32_CONST(40));
    }
}

// Starts a fling at a speed, from a scroll that took some frames
static void IrcRankingFling_Start(IrcRankingFling *fling, fx32 speed, u32 frames) {
    fx32 limit;

    if (MATH_ABS(speed) > FX32_ONE) {
        fling->direction = speed / MATH_ABS(speed);
        speed = MATH_ABS(speed);
        speed = MATH_CLAMP(speed, FX32_ONE, FX32_CONST(60));
        if (FX32_CONST(frames) < FX32_CONST(13)) {
            limit = MATH_CLAMP(FX32_CONST(frames), 0, FX32_CONST(13));
            fling->strength = FX_Div(speed - FX32_ONE, FX32_CONST(59));
            fling->duration = FX32_ONE - FX_Div(limit, FX32_CONST(13));
            fling->pending = TRUE;
        }
    }
}

static void IrcRankingFling_Stop(IrcRankingFling *fling) {
    fling->speed = 0;
}

// Slows the fling down, and returns how far the list moves
static s16 IrcRankingFling_Step(IrcRankingFling *fling) {
    fx32 speed = fling->speed;
    fx32 step = speed * fling->direction;

    step = MATH_CLAMP(step, -0x8000, 0x8000);
    fling->speed -= FX_Mul(speed, 205);
    if (fling->speed > -FX32_ONE && fling->speed < FX32_ONE) {
        fling->speed = 0;
    }
    return FX_Whole(step);
}

static BOOL IrcRankingFling_IsActive(IrcRankingFling *fling) {
    if (fling->speed != 0) {
        return TRUE;
    }
    return FALSE;
}

// Prints a message with the player's name in it, in the middle of a window
static void IrcRanking_PrintCentered(BmpWin *window, StrBuf *format, PlayerInfo *player, Font *font) {
    WordSet *wordSet;
    StrBuf *strbuf;
    u16 x;
    u16 y;

    if (player == NULL) {
        x = BmpWin_GetSizeX(window) * 4;
        y = BmpWin_GetSizeY(window) * 4;
        x -= (u16)(GFL_FontGetBlockWidth(format, font, 0) / 2);
        y -= (u16)(GFL_FontGetBlockHeight(format, font) / 2);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, format, font);
        return;
    }
    wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(HEAP_ID));
    strbuf = GFL_StrBufCreate(128, HEAPID_TAIL(HEAP_ID));
    copyVarForText(wordSet, 0, player);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, format);
    x = BmpWin_GetSizeX(window) * 4;
    y = BmpWin_GetSizeY(window) * 4;
    x -= (u16)(GFL_FontGetBlockWidth(strbuf, font, 0) / 2);
    y -= (u16)(GFL_FontGetBlockHeight(strbuf, font) / 2);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, strbuf, font);
    GFL_StrBufFree(strbuf);
    GFL_WordSetSystemFree(wordSet);
}

// Copies a rectangle of tiles from a bitmap to another of the same width
static void IrcRanking_CopyTiles(GFLBitmap *src, GFLBitmap *dest, u32 srcX, u32 srcY, u16 destX, u16 destY, u16 width,
                                 u16 height) {
    u8 *srcPixels = GFL_BitmapGetPixelData(src);
    u8 *destPixels = GFL_BitmapGetPixelData(dest);

    sys_memcpy32(srcPixels + (srcX + width * srcY) * 32, destPixels + (destX + width * destY) * 32, width * height * 32);
}

static IrcRankingLabel *IrcRankingLabel_Create(const ClActorSetup *setup, u8 width, u8 height, u32 palette,
                                               u8 paletteOffset, u32 surface, BmpOamSys *oam, PrintQueue *queue,
                                               HeapID heapId) {
    IrcRankingLabel *label;
    BmpOamActorSetup actorSetup;

    label = GFL_HeapAllocate(heapId, sizeof(IrcRankingLabel), FALSE, "irc_ranking.c", 3402);
    sys_memset(label, 0, sizeof(IrcRankingLabel));
    label->queue = queue;
    label->strbuf = GFL_StrBufCreate(128, heapId);
    label->bitmap = GFL_BitmapCreate(width, height, 32, heapId);
    sys_memset(&actorSetup, 0, sizeof(BmpOamActorSetup));
    actorSetup.bitmap = label->bitmap;
    actorSetup.x = setup->x;
    actorSetup.y = setup->y;
    actorSetup.palette = palette;
    actorSetup.priority = setup->priority;
    actorSetup.surface = surface;
    actorSetup.vramType = surface;
    actorSetup.bgPriority = setup->bgPriority;
    actorSetup.paletteOffset = paletteOffset;
    label->actor = BmpOam_ActorAdd(oam, &actorSetup);
    return label;
}

static void IrcRankingLabel_Free(IrcRankingLabel *label) {
    BmpOam_ActorDel(label->actor);
    GFL_BitmapFree(label->bitmap);
    GFL_StrBufFree(label->strbuf);
    GFL_HeapFree(label);
}

static void IrcRankingLabel_PrintMsg(IrcRankingLabel *label, MsgData *msgData, u32 msgId, Font *font) {
    IrcRankingPoint pos;

    GFL_BitmapFill(label->bitmap, label->color & 0x1f);
    GFL_MsgDataLoadStrbuf(msgData, msgId, label->strbuf);
    IrcRankingLabel_GetPosition(label, font, &pos);
    func_02021c7c(label->queue, label->bitmap, pos.x, pos.y, label->strbuf, font, label->color);
    label->busy = TRUE;
}

static void IrcRankingLabel_SetColor(IrcRankingLabel *label, u16 color) {
    label->color = color;
}

static void IrcRankingLabel_SetPos(IrcRankingLabel *label, int x, int y, int align) {
    label->offset.x = x;
    label->offset.y = y;
    label->align = align;
}

static BOOL IrcRankingLabel_Update(IrcRankingLabel *label) {
    if (label->busy) {
        if (!func_02021c1c(label->queue, label->bitmap)) {
            BmpOam_ActorBmpTrans(label->actor);
            label->busy = FALSE;
        }
    }
    if (!label->busy) {
        return TRUE;
    }
    return FALSE;
}

static void IrcRankingLabel_GetPosition(IrcRankingLabel *label, Font *font, IrcRankingPoint *pos) {
    u32 halfWidth;
    u32 halfHeight;
    int y;

    switch (label->align) {
    case LABEL_ALIGN_NONE:
        *pos = label->offset;
        break;
    case LABEL_ALIGN_CENTER:
        halfWidth = GFL_BitmapGetWidth(label->bitmap) / 2;
        halfHeight = GFL_BitmapGetHeight(label->bitmap) / 2;
        halfWidth -= GFL_FontGetBlockWidth(label->strbuf, font, 0) / 2;
        y = halfHeight - GFL_FontGetBlockHeight(label->strbuf, font) / 2;
        pos->x = halfWidth + label->offset.x;
        pos->y = y + label->offset.y;
        break;
    case LABEL_ALIGN_CENTER_Y:
        halfHeight = GFL_BitmapGetHeight(label->bitmap) / 2;
        y = halfHeight - GFL_FontGetBlockHeight(label->strbuf, font) / 2;
        pos->x = label->offset.x;
        pos->y = y + label->offset.y;
        break;
    }
}
