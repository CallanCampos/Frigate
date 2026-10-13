#include "types.h"
#include "app/medalbox.h"
#include "app/medalbox_data_sys.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
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
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/blink_palanm.h"
#include "system/bmp_winframe.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/title_10.h"

// The Medal Box screen: the medal case and the medal under the cursor on the top screen, and on the touch screen
// the columns of medals, which scroll sideways, with the names of the middle column's medals beside them or, in the
// other view, five columns without names. The ROM has no name for this file; "medalbox.c" is a guess from its data
// file's, "medalbox_data_sys.c"

#define HEAPID_MEDALBOX 0x97
// The screen's graphics
#define ARCID_MEDALBOX 236
// The medals' cells
#define ARCID_MEDAL_OBJ 238

#define BG_COUNT 8
// Indexes into sMedalBoxBGs
#define BG_MAIN_BACK 0
#define BG_MAIN_MESSAGE 1
#define BG_MAIN_NUMBER 2
#define BG_MAIN_TEXT 3
#define BG_SUB_CASE 4
#define BG_SUB_CASE_BACK 5
#define BG_SUB_BACK 6
#define BG_SUB_TEXT 7

// The columns of medals the screen holds actors for, and how many of them each view shows
#define SHOWN_MAX 5
#define SHOWN_NAMES 3
#define SHOWN_ALL 5
#define COLUMN_WIDTH 0x50

// The views of the touch screen
#define VIEW_NAMES 0
#define VIEW_COLUMNS 1

#define ACTOR_MEDAL 0
#define ACTOR_SLIDER 25
#define ACTOR_SLIDER_BAR 26
#define ACTOR_CURSOR 27
#define ACTOR_FAVORITE 28
#define ACTOR_SPARKLE 29
#define ACTOR_COVER 39
#define ACTOR_RETURN 40
#define ACTOR_LEFT 41
#define ACTOR_RIGHT 42
#define ACTOR_EXIT 43
#define ACTOR_MENU 44
// The medals again as OBJ windows, which the shine of the BG shows through
#define ACTOR_MEDAL_WINDOW 45
// The medal under the cursor, on the top screen
#define ACTOR_INFO_MEDAL 70
#define ACTOR_COUNT 71

// NitroSDK's GX_OAM_MODE_OBJWND, which nitro/gx.h lacks
#define OAM_MODE_OBJWND 2

#define MEDAL_ACTOR_COUNT 25
#define SPARKLE_COUNT 10

#define RES_MEDAL_CELLS 0
#define RES_MEDAL_PALETTE 1
#define RES_MEDAL_CHARS 2
#define RES_CELLS 3
#define RES_PALETTE 4
#define RES_CHARS 5
#define RES_COMMON_CELLS 6
#define RES_COMMON_PALETTE 7
#define RES_COMMON_CHARS 8
#define RES_BAR_CHARS 9
#define RES_BAR_CELLS 10
#define RES_COUNT 11

#define WINDOW_NAMES 0
#define WINDOW_INFO 1
#define WINDOW_COUNT 2

#define MSG_UI 0
#define MSG_MEDAL_NAMES 1
#define MSG_MEDAL_HINTS 2
#define MSG_MEDAL_INFO 3
#define MSG_COUNT 4

// The slider's range, and where it starts on the screen
#define SLIDER_X 0x3c
#define SLIDER_Y 0xb4
#define SLIDER_WIDTH 0x70

// What the cursor does: hidden until a key is pressed, hidden while the columns move, or shown
#define CURSOR_TOUCH 0
#define CURSOR_HIDDEN 1
#define CURSOR_SHOWN 2

// Where the cursor goes after the columns scrolled: where it was, or to the first or last medal of its column
#define CURSOR_EDGE_NONE 0
#define CURSOR_EDGE_TOP 1
#define CURSOR_EDGE_BOTTOM 2

// What MedalBoxApp_SetFavorite did, for its message
#define FAVORITE_SET 0
#define FAVORITE_REMOVED 1
#define FAVORITE_HINT 2

// The menu's buttons are messages of the screen's bank, and this is none
#define MENU_ITEM_NONE 22

#define MEDAL_NAME_NONE 0xff

#define COLOR_NAME PRINT_COLOR(2, 3, 1)
#define COLOR_INFO PRINT_COLOR(1, 2, 0)
#define COLOR_MENU PRINT_COLOR(14, 15, 3)

enum {
    SEQ_START,
    SEQ_MAIN,
    SEQ_SCROLL,
    SEQ_SLIDER,
    SEQ_MENU,
    SEQ_SORT,
    SEQ_CHANGE_VIEW,
    SEQ_RETURN,
    SEQ_EXIT,
    SEQ_FAVORITE,
    SEQ_MANUAL,
};

// The touch screen's buttons
enum {
    BUTTON_MENU,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_RETURN,
    BUTTON_EXIT,
};

typedef struct {
    int bg;
    BGSetup setup;
    int mode;
} MedalBoxBG;

typedef struct {
    int x;
    int y;
} MedalBoxPos;

typedef struct {
    HeapID heapId;
    MedalBoxParam *param;
    TCB *vblankTask;
    ClActUnit *unit;
    ClActor *actors[ACTOR_COUNT];
    u32 resources[RES_COUNT];
    BmpWin *windows[WINDOW_COUNT];
    BmpWin *messageWindow;
    Font *font;
    MsgData *msgData[MSG_COUNT];
    u8 subSeq;
    int view;
    int seq;
    MedalBoxDataSys data;
    // The column in the middle of VIEW_NAMES, and the one to show after the view changes
    u16 column;
    u16 nextColumn;
    fx32 scrollX;
    int scrollDir;
    int scrollCount;
    u32 scrollSpeed;
    MedalBoxColumn shown[SHOWN_MAX];
    // The medal under the cursor, counted from the second shown column, and where the cursor returns to
    u8 cursor;
    u8 prevCursor;
    int cursorMode;
    u32 sliderX;
    AppTaskMenuRes *menuRes;
    AppTaskMenuItem menuItems[APP_TASKMENU_ITEM_MAX];
    AppTaskMenu *menu;
    int menuItemCount;
    fx32 coverX;
    fx32 caseY;
    fx32 backX;
    u32 wait;
    int favoriteResult;
    u32 touchX;
    int drag;
    BOOL dragging;
    BlinkPalAnm *blinkAnm;
    TCBExManager *tcbManager;
    AppPrintsysCommon printCommon;
    PrintQueue *printQueue;
    PrintStream *stream;
    KeyCursor *keyCursor;
    int cursorEdge;
    BOOL printing;
    BOOL shining;
    int shineWait;
    PrintWindow printWindows[WINDOW_COUNT];
    MedalBox *medalBox;
} MedalBoxApp;

static BOOL MedalBoxApp_ProcInit(GameProc *proc, u32 *state, void *param, void *data);
static BOOL MedalBoxApp_ProcMain(GameProc *proc, u32 *state, void *param, void *data);
static BOOL MedalBoxApp_ProcExit(GameProc *proc, u32 *state, void *param, void *data);
static void MedalBoxApp_InitData(MedalBoxApp *work);
static void MedalBoxApp_FreeData(MedalBoxApp *work);
static void MedalBoxApp_InitGraphics(MedalBoxApp *work);
static void MedalBoxApp_FreeGraphics(MedalBoxApp *work);
static void MedalBoxApp_InitBG(MedalBoxApp *work);
static int MedalBoxApp_GetRankPalette(MedalBox *box);
static void MedalBoxApp_ReloadMenuBG(MedalBoxApp *work);
static void MedalBoxApp_FreeBG(MedalBoxApp *work);
static void MedalBoxApp_InitObj(MedalBoxApp *work);
static void MedalBoxApp_FreeObj(MedalBoxApp *work);
static void MedalBoxApp_InitActors(MedalBoxApp *work);
static void MedalBoxApp_InitWindows(MedalBoxApp *work);
static void MedalBoxApp_FreeWindows(MedalBoxApp *work);
static void MedalBoxApp_VBlank(TCB *tcb, void *data);
static void MedalBoxApp_InitColumn(MedalBoxApp *work);
static void MedalBoxApp_DrawColumn(MedalBoxApp *work, u8 column, u8 shown, BOOL names);
static void MedalBoxApp_GetMedalPos(int index, ClActorPos *pos);
static void MedalBoxApp_GetFavoritePos(int slot, ClActorPos *pos);
static int MedalBoxApp_GetWindowActor(int index);
static void MedalBoxApp_UpdateBGScroll(MedalBoxApp *work);
static void MedalBoxApp_SetSeq(MedalBoxApp *work, int seq);
static void MedalBoxApp_InitPrint(MedalBoxApp *work);
static void MedalBoxApp_UpdatePrint(MedalBoxApp *work);
static void MedalBoxApp_FreePrint(MedalBoxApp *work);
static int MedalBoxApp_GetBG(int index);
static BOOL MedalBoxApp_SeqStart(MedalBoxApp *work);
static void MedalBoxApp_RestoreCursor(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqMain(MedalBoxApp *work);
static void MedalBoxApp_HandleInput(MedalBoxApp *work);
static void MedalBoxApp_StartDrag(MedalBoxApp *work);
static BOOL MedalBoxApp_UpdateDrag(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqScroll(MedalBoxApp *work);
static void MedalBoxApp_SetScrollDir(MedalBoxApp *work, int dir);
static BOOL MedalBoxApp_Scroll(MedalBoxApp *work, fx32 speed);
static void MedalBoxApp_MoveMedals(MedalBoxApp *work);
static void MedalBoxApp_MoveSparkles(MedalBoxApp *work, s16 x);
static void MedalBoxApp_DrawColumns(MedalBoxApp *work, BOOL names, BOOL slider);
static void MedalBoxApp_PrintName(MedalBoxApp *work, u32 medal, u8 slot);
static void MedalBoxApp_FlushNames(MedalBoxApp *work);
static void MedalBoxApp_DrawInfo(MedalBoxApp *work, MedalBoxEntry *medal, BOOL show);
static void MedalBoxApp_ClearNames(MedalBoxApp *work);
static u16 MedalBoxApp_GetNameColor(void);
static u16 MedalBoxApp_GetInfoColor(void);
static void MedalBoxApp_GetMedalRects(MedalBoxApp *work, TouchRect *rects);
static void MedalBoxApp_GetNameRects(MedalBoxApp *work, TouchRect *rects);
static void MedalBoxApp_GetFavoriteRects(MedalBoxApp *work, TouchRect *rects);
static void MedalBoxApp_GetCursorPos(MedalBoxApp *work, u8 cursor, ClActorPos *pos);
static BOOL MedalBoxApp_HandleKeys(MedalBoxApp *work);
static void MedalBoxApp_ScrollRight(MedalBoxApp *work);
static void MedalBoxApp_ScrollLeft(MedalBoxApp *work);
static void MedalBoxApp_SetCursor(MedalBoxApp *work, u8 cursor, BOOL keys);
static void MedalBoxApp_SetCursorMode(MedalBoxApp *work, int mode, u8 medal);
static void MedalBoxApp_UpdateSparkles(MedalBoxApp *work);
static u8 MedalBoxApp_GetCursorColumn(MedalBoxApp *work);
static u8 MedalBoxApp_GetCursorSlot(MedalBoxApp *work);
static void MedalBoxApp_SetScrollCount(MedalBoxApp *work, int count);
static void MedalBoxApp_SetCursorEdge(MedalBoxApp *work, int edge);
static void MedalBoxApp_SetScrollSpeed(MedalBoxApp *work, u32 speed);
static BOOL MedalBoxApp_SeqExit(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqReturn(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqSlider(MedalBoxApp *work);
static BOOL MedalBoxApp_UpdateSlider(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqMenu(MedalBoxApp *work);
static void MedalBoxApp_OpenMenu(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqChangeView(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqFavorite(MedalBoxApp *work);
static void MedalBoxApp_SetFavorite(MedalBoxApp *work, u16 medal, int result);
static BOOL MedalBoxApp_SeqSort(MedalBoxApp *work);
static BOOL MedalBoxApp_SeqManual(MedalBoxApp *work);
static void MedalBoxApp_ShowMessage(MedalBoxApp *work, StrBuf *str);
static void MedalBoxApp_CloseMessage(MedalBoxApp *work);

static const BGSysLCDConfig sMedalBoxLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const TouchRect sMedalBoxButtonRects[] = {
    { 0xa8, 0xc0, 0x00, 0x18 }, { 0xa8, 0xc0, 0x20, 0x38 }, { 0xa8, 0xc0, 0xb0, 0xc8 },
    { 0xa8, 0xc0, 0xe8, 0x00 }, { 0xa8, 0xc0, 0xd0, 0xe8 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// Where the first column's medals are
static const MedalBoxPos sMedalBoxMedalPos[MEDALBOX_COLUMN_SIZE] = {
    { 0x18, 0x14 }, { 0x30, 0x34 }, { 0x18, 0x54 }, { 0x30, 0x74 }, { 0x18, 0x94 },
};

// The marks beside the names, which register a favorite medal
static const MedalBoxPos sMedalBoxFavoritePos[MEDALBOX_COLUMN_SIZE] = {
    { 0xe0, 0x18 }, { 0xf0, 0x38 }, { 0xe0, 0x58 }, { 0xf0, 0x78 }, { 0xe0, 0x98 },
};

static const MedalBoxPos sMedalBoxNamePos[MEDALBOX_COLUMN_SIZE] = {
    { 0x60, 0xc }, { 0x70, 0x2c }, { 0x60, 0x4c }, { 0x70, 0x6c }, { 0x60, 0x8c },
};

// Where a column's medals have their numbers, in tiles
static const MedalBoxPos sMedalBoxNumberPos[MEDALBOX_COLUMN_SIZE] = {
    { 8, 2 }, { 2, 6 }, { 8, 10 }, { 2, 14 }, { 8, 18 },
};

static const BGSysVRAMConfig sMedalBoxVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The menu's buttons in each of the orders: the view, the orders but the current one, and the way back
static const int sMedalBoxMenuItems[6][APP_TASKMENU_ITEM_MAX] = {
    { Title10_Text_Columns, Title10_Text_MedalsOnly, Title10_Text_HintMedalsOnly, Title10_Text_DisplayByType,
      Title10_Text_DisplayByName, Title10_Text_NewestFirst, Title10_Text_Back, MENU_ITEM_NONE },
    { Title10_Text_Columns, Title10_Text_MedalsOnly, Title10_Text_HintMedalsOnly, Title10_Text_DisplayByType,
      Title10_Text_DisplayByName, Title10_Text_Revert, Title10_Text_Back, MENU_ITEM_NONE },
    { Title10_Text_Columns, Title10_Text_HintMedalsOnly, Title10_Text_DisplayByType, Title10_Text_DisplayByName,
      Title10_Text_NewestFirst, Title10_Text_Revert, Title10_Text_Back, MENU_ITEM_NONE },
    { Title10_Text_Columns, Title10_Text_MedalsOnly, Title10_Text_DisplayByType, Title10_Text_DisplayByName,
      Title10_Text_NewestFirst, Title10_Text_Revert, Title10_Text_Back, MENU_ITEM_NONE },
    { Title10_Text_Columns, Title10_Text_MedalsOnly, Title10_Text_HintMedalsOnly, Title10_Text_DisplayByType,
      Title10_Text_NewestFirst, Title10_Text_Revert, Title10_Text_Back, MENU_ITEM_NONE },
    { Title10_Text_Columns, Title10_Text_MedalsOnly, Title10_Text_HintMedalsOnly, Title10_Text_DisplayByName,
      Title10_Text_NewestFirst, Title10_Text_Revert, Title10_Text_Back, MENU_ITEM_NONE },
};

static const MedalBoxBG sMedalBoxBGs[BG_COUNT] = {
    { 0,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 1,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x8000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 2,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 3,
      { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 4,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x0000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 5,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 6,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x4000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
    { 7,
      { 0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x8000), 0x8000,
        GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 },
      BGMODE_TEXT },
};

GameProcFunctions MEDALBOX_PROC_FUNCTIONS = { MedalBoxApp_ProcInit, MedalBoxApp_ProcMain, MedalBoxApp_ProcExit };

static BOOL MedalBoxApp_ProcInit(GameProc *proc, u32 *state, void *param, void *data) {
    MedalBoxParam *boxParam = param;
    MedalBoxApp *work;
    ArcTool *arc;

    GFL_OvlLoad(OVERLAY_ID(139));
    // Nothing to show without a medal
    if (MedalBox_GetObtainedCount(SaveControl_GetMedalBox(GameData_GetSaveControl(boxParam->gameData)), TRUE) == 0) {
        return TRUE;
    }
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MEDALBOX, 0x60000);
    work = GFL_ProcInitSubsystem(proc, sizeof(MedalBoxApp), HEAPID_MEDALBOX);
    sys_memset(work, 0, sizeof(MedalBoxApp));
    work->param = boxParam;
    work->heapId = HEAPID_MEDALBOX;
    MedalBoxApp_InitData(work);
    MedalBoxApp_InitGraphics(work);
    MedalBoxApp_InitPrint(work);
    MedalBoxApp_InitActors(work);
    MedalBoxApp_InitWindows(work);
    MedalBoxApp_InitColumn(work);
    work->blinkAnm = BlinkPalAnm_Create(0x60, 0x10, BLINK_PALANM_OBJ_MAIN, work->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MEDALBOX, work->heapId);
    BlinkPalAnm_SetPalBufferArcTool(work->blinkAnm, arc, 2, 0, 0x10);
    GFL_ArcToolFree(arc);
    func_02042ba8(TRUE, work->heapId);
    MedalBoxApp_DrawInfo(work, NULL, FALSE);
    return TRUE;
}

static BOOL MedalBoxApp_ProcMain(GameProc *proc, u32 *state, void *param, void *data) {
    MedalBoxParam *boxParam = param;
    MedalBoxApp *work = data;
    BOOL result;
    int i;

    if (MedalBox_GetObtainedCount(SaveControl_GetMedalBox(GameData_GetSaveControl(boxParam->gameData)), TRUE) == 0) {
        return TRUE;
    }
    switch (work->seq) {
    case SEQ_START:
        result = MedalBoxApp_SeqStart(work);
        break;
    case SEQ_MAIN:
        result = MedalBoxApp_SeqMain(work);
        break;
    case SEQ_SCROLL:
        result = MedalBoxApp_SeqScroll(work);
        break;
    case SEQ_SLIDER:
        result = MedalBoxApp_SeqSlider(work);
        break;
    case SEQ_CHANGE_VIEW:
        result = MedalBoxApp_SeqChangeView(work);
        break;
    case SEQ_RETURN:
        result = MedalBoxApp_SeqReturn(work);
        break;
    case SEQ_EXIT:
        result = MedalBoxApp_SeqExit(work);
        break;
    case SEQ_MENU:
        result = MedalBoxApp_SeqMenu(work);
        break;
    case SEQ_SORT:
        result = MedalBoxApp_SeqSort(work);
        break;
    case SEQ_FAVORITE:
        result = MedalBoxApp_SeqFavorite(work);
        break;
    case SEQ_MANUAL:
        result = MedalBoxApp_SeqManual(work);
        break;
    default:
        result = TRUE;
        break;
    }
    MedalBoxApp_UpdateBGScroll(work);
    MedalBoxApp_UpdatePrint(work);
    work->printing = FALSE;
    func_02021a3c(work->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintWindow_Flush(&work->printWindows[i], work->printQueue);
        if (PrintWindow_IsPrinted(&work->printWindows[i]) == FALSE) {
            work->printing = TRUE;
        }
    }
    func_0204b794();
    return result;
}

static BOOL MedalBoxApp_ProcExit(GameProc *proc, u32 *state, void *param, void *data) {
    MedalBoxParam *boxParam = param;
    MedalBoxApp *work = data;

    if (MedalBox_GetObtainedCount(SaveControl_GetMedalBox(GameData_GetSaveControl(boxParam->gameData)), TRUE) == 0) {
        GFL_OvlUnload(OVERLAY_ID(139));
        return TRUE;
    }
    MedalBoxDataSys_SaveFavorite(&work->data, work->param->gameData, work->heapId);
    BlinkPalAnm_Free(work->blinkAnm);
    GFL_TCBRemove(work->vblankTask);
    MedalBoxApp_FreeWindows(work);
    MedalBoxApp_FreeObj(work);
    MedalBoxApp_FreeBG(work);
    MedalBoxApp_FreeGraphics(work);
    MedalBoxApp_FreePrint(work);
    MedalBoxApp_FreeData(work);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MEDALBOX);
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static void MedalBoxApp_InitData(MedalBoxApp *work) {
    HeapID heapId = work->heapId;

    work->medalBox = SaveControl_GetMedalBox(GameData_GetSaveControl(work->param->gameData));
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    work->msgData[MSG_UI] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_10, heapId);
    work->msgData[MSG_MEDAL_NAMES] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MEDAL_NAMES, heapId);
    work->msgData[MSG_MEDAL_HINTS] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_11, heapId);
    work->msgData[MSG_MEDAL_INFO] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_12, heapId);
    MedalBoxDataSys_Init(&work->data, work->param->gameData, work->heapId);
    MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_OLDEST);
    work->caseY = FX32_CONST(-160);
    work->cursorEdge = CURSOR_EDGE_NONE;
    work->printing = FALSE;
}

static void MedalBoxApp_FreeData(MedalBoxApp *work) {
    MedalBoxDataSys_Free(&work->data);
    GFL_MsgDataFree(work->msgData[MSG_UI]);
    GFL_MsgDataFree(work->msgData[MSG_MEDAL_INFO]);
    GFL_MsgDataFree(work->msgData[MSG_MEDAL_HINTS]);
    GFL_MsgDataFree(work->msgData[MSG_MEDAL_NAMES]);
    GFL_FontFree(work->font);
}

static void MedalBoxApp_InitGraphics(MedalBoxApp *work) {
    G2_BlendNone();
    G2S_BlendNone();
    // The medals' OBJ windows show every plane, with the shine blended
    G2_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                                GX_WND_PLANEMASK_OBJ,
                            TRUE);
    G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                          TRUE);
    GX_SetVisibleWnd(GX_WNDMASK_OW);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sMedalBoxVRAMConfig);
    GFL_BGSysSetDisplayLayout(0);
    GFL_BGSysEnableEngines();
    GFL_BGSysDisableAllA();
    func_020232d0();
    MedalBoxApp_InitBG(work);
    MedalBoxApp_InitObj(work);
    AppPrintsysCommon_Init(&work->printCommon, 6);
    work->vblankTask = GFL_VBlankTCBAdd(MedalBoxApp_VBlank, work, 0);
}

static void MedalBoxApp_FreeGraphics(MedalBoxApp *work) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

static void MedalBoxApp_InitBG(MedalBoxApp *work) {
    HeapID heapId = work->heapId;
    ArcTool *arc;
    int i;

    GFL_BGSysCreate(heapId);
    GFL_BGSysSetLCDConfig(&sMedalBoxLCDConfig);
    for (i = 0; i < BG_COUNT; i++) {
        int bg = sMedalBoxBGs[i].bg;

        GFL_BGSysCreateBG(bg, &sMedalBoxBGs[i].setup, sMedalBoxBGs[i].mode);
        GFL_BGSysSetBGEnabled(bg, TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_MEDALBOX, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, MedalBoxApp_GetBG(BG_MAIN_BACK), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, MedalBoxApp_GetBG(BG_MAIN_NUMBER), 0, 0, FALSE, heapId);
    GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_NUMBER), BG_MOVE_SET_X, COLUMN_WIDTH);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, MedalBoxApp_GetBG(BG_MAIN_BACK), 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 8, MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, MedalBoxApp_GetBG(BG_SUB_CASE), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, MedalBoxApp_GetBG(BG_SUB_CASE), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 11, MedalBoxApp_GetBG(BG_SUB_CASE_BACK), 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 9, MedalBoxApp_GetBG(BG_SUB_BACK), 0, 0, FALSE, heapId);
    // The case's color is the rank's
    GFL_G2DIOLoadArcNCLR(arc, 0, 0, (MedalBoxApp_GetRankPalette(work->medalBox) + 1) * 0x20, 0x20, 0x20, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x160, 0x20, work->heapId);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 2, 0x1d, 15, 15);
    GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 2);
    GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
    GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE_BACK), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
}

// The palette of a rank's case. The last rank has one for each version, Black 2's after White 2's
static int MedalBoxApp_GetRankPalette(MedalBox *box) {
    int rank = MedalBox_GetRank(box);
    int palette = rank;

    if (rank == MEDAL_RANK_MAX) {
        palette = rank + GAME_VERSION - VERSION_WHITE2;
    }
    return palette;
}

// Loads the BG that the menu and the messages drew over again
static void MedalBoxApp_ReloadMenuBG(MedalBoxApp *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MEDALBOX, work->heapId);

    GFL_BGSysLoadArcNCGRStatic(arc, 3, MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 8, MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0, 0, FALSE, work->heapId);
    GFL_ArcToolFree(arc);
}

static void MedalBoxApp_FreeBG(MedalBoxApp *work) {
    int i;

    for (i = 0; i < BG_COUNT; i++) {
        int bg = sMedalBoxBGs[i].bg;

        GFL_BGSysClearBG(bg);
        GFL_BGSysReleaseBG(bg);
    }
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0, 31, 0);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void MedalBoxApp_InitObj(MedalBoxApp *work) {
    HeapID heapId = work->heapId;
    ClActSysSetup setup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 32, 32, 32, 32, 16, 16 };
    ArcTool *arcs[3];
    ClActorPos pos;
    ClActorSetup actorSetup;
    ClActorPos windowPos;
    ClActorSetup windowSetup;
    u32 *resources;
    int paletteOffset;
    int i;
    int j;

    ClActSys_Create(&setup, &sMedalBoxVRAMConfig, heapId);
    work->unit = func_0204bf1c(0x80, 1, heapId);
    func_0204c028(work->unit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    resources = work->resources;
    arcs[0] = GFL_ArcSysCreateFileHandle(ARCID_MEDALBOX, heapId);
    arcs[1] = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, heapId);
    arcs[2] = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_OBJ, heapId);
    work->resources[RES_MEDAL_CELLS] = func_0204bde0(arcs[2], 2, 3, heapId);
    resources[RES_MEDAL_PALETTE] = func_0204bbb8(arcs[2], 0, 2, 0, 0, 6, heapId);
    resources[RES_MEDAL_CHARS] = func_0204b81c(arcs[2], 1, FALSE, 2, heapId);
    resources[RES_CELLS] = func_0204bde0(arcs[0], 12, 13, heapId);
    paletteOffset = 0xc0;
    resources[RES_PALETTE] = func_0204bbb8(arcs[0], 2, 0, paletteOffset, 0, 4, heapId);
    // The rank's palette for the top screen's medal case
    paletteOffset += 0x60;
    func_0204bbb8(arcs[0], 2, 0, paletteOffset, MedalBoxApp_GetRankPalette(work->medalBox) + 3, 1, heapId);
    resources[RES_CHARS] = func_0204b81c(arcs[0], 5, FALSE, 0, heapId);
    resources[RES_COMMON_CELLS] = func_0204bde0(arcs[1], 23, 26, heapId);
    resources[RES_COMMON_PALETTE] = func_0204bbb8(arcs[1], 19, 0, 0x140, 0, 4, heapId);
    resources[RES_COMMON_CHARS] = func_0204b81c(arcs[1], 20, FALSE, 0, heapId);
    resources[RES_BAR_CHARS] = func_0204b81c(arcs[1], 175, FALSE, 0, heapId);
    resources[RES_BAR_CELLS] = func_0204bde0(arcs[1], 174, 173, heapId);
    for (i = 0; i < 3; i++) {
        GFL_ArcToolFree(arcs[i]);
    }

    sys_memset(&actorSetup, 0, sizeof(ClActorSetup));
    for (i = ACTOR_MEDAL; i < ACTOR_MEDAL + MEDAL_ACTOR_COUNT; i++) {
        MedalBoxApp_GetMedalPos(i, &pos);
        actorSetup.x = pos.x;
        actorSetup.y = pos.y;
        actorSetup.bgPriority = 3;
        actorSetup.priority = 3;
        work->actors[i] = func_0204c040(work->unit, work->resources[RES_MEDAL_CHARS], work->resources[RES_MEDAL_PALETTE],
                                        work->resources[RES_MEDAL_CELLS], &actorSetup, 0, heapId);
        func_0204c488(work->actors[i], 0);
        func_0204c124(work->actors[i], FALSE);
    }
    for (i = ACTOR_SLIDER; i < ACTOR_RETURN; i++) {
        sys_memset(&actorSetup, 0, sizeof(ClActorSetup));
        actorSetup.bgPriority = 2;
        actorSetup.priority = 2;
        work->actors[i] = func_0204c040(work->unit, work->resources[RES_CHARS], work->resources[RES_PALETTE],
                                        work->resources[RES_CELLS], &actorSetup, 0, heapId);
        func_0204c488(work->actors[i], 0);
        func_0204c124(work->actors[i], TRUE);
    }
    for (i = ACTOR_RETURN; i < ACTOR_MENU; i++) {
        sys_memset(&actorSetup, 0, sizeof(ClActorSetup));
        actorSetup.bgPriority = 2;
        actorSetup.priority = 2;
        work->actors[i] =
            func_0204c040(work->unit, work->resources[RES_COMMON_CHARS], work->resources[RES_COMMON_PALETTE],
                          work->resources[RES_COMMON_CELLS], &actorSetup, 0, heapId);
        func_0204c488(work->actors[i], 0);
        func_0204c124(work->actors[i], TRUE);
    }
    sys_memset(&actorSetup, 0, sizeof(ClActorSetup));
    actorSetup.bgPriority = 2;
    actorSetup.priority = 2;
    work->actors[ACTOR_MENU] =
        func_0204c040(work->unit, work->resources[RES_BAR_CHARS], work->resources[RES_COMMON_PALETTE],
                      work->resources[RES_BAR_CELLS], &actorSetup, 0, heapId);
    func_0204c488(work->actors[ACTOR_MENU], 0);
    func_0204c124(work->actors[ACTOR_MENU], TRUE);

    sys_memset(&windowSetup, 0, sizeof(ClActorSetup));
    for (j = ACTOR_MEDAL_WINDOW; j <= ACTOR_MEDAL_WINDOW + MEDAL_ACTOR_COUNT - 1; j++) {
        MedalBoxApp_GetMedalPos(j - ACTOR_MEDAL_WINDOW, &windowPos);
        windowSetup.x = windowPos.x;
        windowSetup.y = windowPos.y;
        windowSetup.bgPriority = 3;
        windowSetup.priority = 3;
        work->actors[j] = func_0204c040(work->unit, work->resources[RES_MEDAL_CHARS], work->resources[RES_MEDAL_PALETTE],
                                        work->resources[RES_MEDAL_CELLS], &windowSetup, 0, heapId);
        func_0204c488(work->actors[j], 0);
        func_0204c124(work->actors[j], TRUE);
        func_0204c318(work->actors[j], OAM_MODE_OBJWND);
    }

    actorSetup.x = 0x2c;
    actorSetup.y = 0x46;
    actorSetup.bgPriority = 1;
    work->actors[ACTOR_INFO_MEDAL] =
        func_0204c040(work->unit, work->resources[RES_MEDAL_CHARS], work->resources[RES_MEDAL_PALETTE],
                      work->resources[RES_MEDAL_CELLS], &actorSetup, 1, heapId);
    func_0204c488(work->actors[ACTOR_INFO_MEDAL], 0);
    func_0204c124(work->actors[ACTOR_INFO_MEDAL], FALSE);
}

static void MedalBoxApp_FreeObj(MedalBoxApp *work) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (work->actors[i] != NULL) {
            func_0204c108(work->actors[i]);
        }
    }
    func_0204be64(work->resources[RES_MEDAL_CELLS]);
    func_0204b98c(work->resources[RES_MEDAL_CHARS]);
    func_0204bcd0(work->resources[RES_MEDAL_PALETTE]);
    func_0204be64(work->resources[RES_CELLS]);
    func_0204b98c(work->resources[RES_CHARS]);
    func_0204bcd0(work->resources[RES_PALETTE]);
    func_0204be64(work->resources[RES_COMMON_CELLS]);
    func_0204b98c(work->resources[RES_COMMON_CHARS]);
    func_0204bcd0(work->resources[RES_COMMON_PALETTE]);
    func_0204b98c(work->resources[RES_BAR_CHARS]);
    func_0204be64(work->resources[RES_BAR_CELLS]);
    if (work->unit != NULL) {
        func_0204bf98(work->unit);
    }
    func_0204b758();
}

static void MedalBoxApp_InitActors(MedalBoxApp *work) {
    ClActor **actors = work->actors;
    ClActorPos pos;
    int i;

    pos.x = SLIDER_X;
    pos.y = SLIDER_Y;
    func_0204c140(actors[ACTOR_SLIDER], &pos, 0);
    func_0204c488(actors[ACTOR_SLIDER], 0);
    pos.x = 0x74;
    pos.y = SLIDER_Y;
    func_0204c140(actors[ACTOR_SLIDER_BAR], &pos, 0);
    func_0204c488(actors[ACTOR_SLIDER_BAR], 1);
    func_0204c140(actors[ACTOR_CURSOR], &pos, 0);
    func_0204c488(actors[ACTOR_CURSOR], 2);
    func_0204c468(actors[ACTOR_CURSOR], 3);
    MedalBoxApp_GetFavoritePos(0, &pos);
    func_0204c140(actors[ACTOR_FAVORITE], &pos, 0);
    func_0204c488(actors[ACTOR_FAVORITE], 3);
    pos.x = 0;
    pos.y = 0;
    for (i = 0; i < SPARKLE_COUNT; i++) {
        func_0204c140(actors[ACTOR_SPARKLE + i], &pos, 0);
        func_0204c488(actors[ACTOR_SPARKLE + i], 4);
        func_0204c124(work->actors[ACTOR_SPARKLE + i], FALSE);
        func_0204c468(actors[ACTOR_SPARKLE + i], 3);
        func_0204c520(actors[ACTOR_SPARKLE + i], TRUE);
        func_0204c438(actors[ACTOR_SPARKLE + i], 1);
    }
    pos.x = 0x80;
    pos.y = 0x60;
    func_0204c438(actors[ACTOR_COVER], 0);
    func_0204c140(actors[ACTOR_COVER], &pos, 0);
    func_0204c488(actors[ACTOR_COVER], 5);
    pos.x = 0;
    pos.y = 0xa8;
    func_0204c140(actors[ACTOR_MENU], &pos, 0);
    func_0204c488(actors[ACTOR_MENU], 0);
    pos.x = 0xd0;
    pos.y = 0xa8;
    func_0204c140(actors[ACTOR_EXIT], &pos, 0);
    func_0204c488(actors[ACTOR_EXIT], 0);
    pos.x = 0xe8;
    pos.y = 0xa8;
    func_0204c140(actors[ACTOR_RETURN], &pos, 0);
    func_0204c488(actors[ACTOR_RETURN], 1);
    pos.x = 0xb0;
    pos.y = 0xa8;
    func_0204c140(actors[ACTOR_RIGHT], &pos, 0);
    func_0204c488(actors[ACTOR_RIGHT], 5);
    pos.x = 0x20;
    pos.y = 0xa8;
    func_0204c140(actors[ACTOR_LEFT], &pos, 0);
    func_0204c488(actors[ACTOR_LEFT], 4);
}

static void MedalBoxApp_InitWindows(MedalBoxApp *work) {
    BmpWin_InitAllocator(work->heapId);
    work->windows[WINDOW_NAMES] = BmpWin_CreateDynamic(MedalBoxApp_GetBG(BG_MAIN_TEXT), 0, 0, 32, 24, 15, 1);
    BmpWin_FlushMap(work->windows[WINDOW_NAMES]);
    work->windows[WINDOW_INFO] = BmpWin_CreateDynamic(MedalBoxApp_GetBG(BG_SUB_TEXT), 0, 0, 32, 24, 15, 1);
    BmpWin_FlushMap(work->windows[WINDOW_INFO]);
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_TEXT));
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_SUB_TEXT));
    PrintWindow_Init(&work->printWindows[WINDOW_NAMES], work->windows[WINDOW_NAMES]);
    PrintWindow_Init(&work->printWindows[WINDOW_INFO], work->windows[WINDOW_INFO]);
}

static void MedalBoxApp_FreeWindows(MedalBoxApp *work) {
    BmpWin_Free(work->windows[WINDOW_NAMES]);
    BmpWin_Free(work->windows[WINDOW_INFO]);
}

static void MedalBoxApp_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
}

// Starts at the column of the medal the cursor was on the last time
static void MedalBoxApp_InitColumn(MedalBoxApp *work) {
    u8 medal = func_02017b58(work->param->gameData);
    int index = MedalBoxDataSys_FindMedal(&work->data, medal);

    if (medal != 0xff) {
        work->column = index / MEDALBOX_COLUMN_SIZE;
    } else {
        work->column = 0;
    }
    MedalBoxApp_DrawColumns(work, TRUE, TRUE);
}

// Shows a column of the box as one of the shown columns: its medals and their numbers and, for the middle column
// of VIEW_NAMES, their names and the mark of the favorite medal
static void MedalBoxApp_DrawColumn(MedalBoxApp *work, u8 column, u8 shown, BOOL names) {
    BOOL favorite = FALSE;
    MedalBoxColumn *medals = &work->shown[shown];
    int i;

    MedalBoxDataSys_GetColumn(&work->data, medals, column);
    for (i = 0; i < MEDALBOX_COLUMN_SIZE; i++) {
        u8 actor = i + shown * MEDALBOX_COLUMN_SIZE;

        if (i < medals->count) {
            int number;
            int divisor;
            int window;
            int j;

            switch (medals->medals[i]->status) {
            case MEDAL_STATUS_OBTAINED:
                func_0204c488(work->actors[ACTOR_MEDAL + actor], medals->medals[i]->sequence);
                break;
            case MEDAL_STATUS_DISCOVERED:
                // A hint medal is the cell's last sequence
                func_0204c488(work->actors[ACTOR_MEDAL + actor],
                              func_0204c4a8(work->actors[ACTOR_MEDAL + actor]) - 1);
                break;
            }
            if (shown == 1 && work->view == VIEW_NAMES) {
                if (names == TRUE) {
                    MedalBoxEntry *medal = medals->medals[i];

                    if (medal->status == MEDAL_STATUS_OBTAINED) {
                        MedalBoxApp_PrintName(work, medal->id, actor % MEDALBOX_COLUMN_SIZE);
                    } else {
                        MedalBoxApp_PrintName(work, MEDAL_NAME_NONE, actor % MEDALBOX_COLUMN_SIZE);
                    }
                }
                if (work->data.favoriteMedal == medals->medals[i]->id && work->view == VIEW_NAMES) {
                    ClActorPos pos;

                    MedalBoxApp_GetFavoritePos(actor % MEDALBOX_COLUMN_SIZE, &pos);
                    func_0204c140(work->actors[ACTOR_FAVORITE], &pos, 0);
                    func_0204c124(work->actors[ACTOR_FAVORITE], TRUE);
                    favorite = TRUE;
                }
            }
            // The medal's place in the order, in three digits, highlighted for the favorite medal
            number = actor % MEDALBOX_COLUMN_SIZE + column * MEDALBOX_COLUMN_SIZE + 1;
            divisor = 100;
            for (j = 0; j < 3; j++) {
                int digit = number / divisor;
                int x = sMedalBoxNumberPos[i].x + shown * 10 - (2 - j);
                int y = sMedalBoxNumberPos[i].y;
                u8 palette = 8;

                if (work->data.favoriteMedal != medals->medals[i]->id) {
                    palette = 0;
                }
                if (digit == 0) {
                    GFL_BGSysFillScrArea(MedalBoxApp_GetBG(BG_MAIN_NUMBER), 10, x, y, 1, 1,
                                         palette);
                } else {
                    GFL_BGSysFillScrArea(MedalBoxApp_GetBG(BG_MAIN_NUMBER), digit, x, y, 1, 1,
                                         palette);
                }
                number -= digit * divisor;
                divisor /= 10;
            }
            window = MedalBoxApp_GetWindowActor(actor);
            func_0204c124(work->actors[ACTOR_MEDAL + actor], TRUE);
            func_0204c124(work->actors[window], TRUE);
        } else {
            int window = MedalBoxApp_GetWindowActor(actor);
            int j;

            func_0204c124(work->actors[ACTOR_MEDAL + actor], FALSE);
            func_0204c124(work->actors[window], FALSE);
            for (j = 0; j < 3; j++) {
                GFL_BGSysFillScrArea(MedalBoxApp_GetBG(BG_MAIN_NUMBER), 10,
                                     sMedalBoxNumberPos[i].x + shown * 10 - (2 - j), sMedalBoxNumberPos[i].y, 1, 1, 0);
            }
        }
    }
    if (!favorite && work->view == VIEW_NAMES && shown == 1) {
        func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
    }
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_NUMBER));
}

// Where a medal's actor goes, with the second shown column at the screen's left edge
static void MedalBoxApp_GetMedalPos(int index, ClActorPos *pos) {
    pos->x = sMedalBoxMedalPos[index % MEDALBOX_COLUMN_SIZE].x + (index / MEDALBOX_COLUMN_SIZE - 1) * COLUMN_WIDTH;
    pos->y = sMedalBoxMedalPos[index % MEDALBOX_COLUMN_SIZE].y;
}

static void MedalBoxApp_GetFavoritePos(int slot, ClActorPos *pos) {
    pos->x = sMedalBoxFavoritePos[slot].x;
    pos->y = sMedalBoxFavoritePos[slot].y;
}

static int MedalBoxApp_GetWindowActor(int index) {
    return ACTOR_MEDAL_WINDOW + index;
}

// Lets a shine run over the touch screen every now and then
static void MedalBoxApp_UpdateBGScroll(MedalBoxApp *work) {
    switch (work->seq) {
    case SEQ_START:
    case SEQ_MAIN:
    case SEQ_SCROLL:
    case SEQ_SLIDER:
    case SEQ_SORT:
    case SEQ_CHANGE_VIEW:
    case SEQ_RETURN:
    case SEQ_EXIT:
        if (work->shining == TRUE) {
            if (GFL_BGSysGetBGOffsetX(MedalBoxApp_GetBG(BG_MAIN_MESSAGE)) <= -0x200) {
                work->shineWait = GFL_RandomMTRange(40) + 100;
                work->shining = FALSE;
                GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), BG_MOVE_SET_X, 0);
            } else {
                GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), BG_MOVE_LEFT, 10);
            }
        } else {
            work->shineWait--;
            if (work->shineWait <= 0) {
                work->shining = TRUE;
            }
        }
        break;
    }
}

static void MedalBoxApp_SetSeq(MedalBoxApp *work, int seq) {
    work->seq = seq;
    work->subSeq = 0;
    work->dragging = FALSE;
}

static void MedalBoxApp_InitPrint(MedalBoxApp *work) {
    work->stream = NULL;
    work->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 1, 0);
    work->printQueue = func_02021998(work->heapId);
    work->keyCursor = KeyCursor_Create(15, TRUE, TRUE, work->heapId);
}

// Runs the message, which a button or a touch ends once it is printed
static void MedalBoxApp_UpdatePrint(MedalBoxApp *work) {
    GFL_TCBExMgrUpdate(work->tcbManager);
    if (work->stream != NULL) {
        KeyCursor_Update(work->keyCursor, work->stream, work->messageWindow);
        if (AppPrintsysCommon_Update(&work->printCommon, work->stream) == TRUE) {
            if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48() == TRUE) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                func_020223cc(work->stream);
                work->stream = NULL;
            } else {
                KeyCursor_Draw(work->keyCursor, BmpWin_GetBitmap(work->messageWindow), 15);
                BmpWin_FlushChar(work->messageWindow);
            }
        }
    }
}

static void MedalBoxApp_FreePrint(MedalBoxApp *work) {
    KeyCursor_Free(work->keyCursor);
    func_02021c44(work->printQueue);
    func_02021a18(work->printQueue);
    work->printQueue = NULL;
    GFL_TCBExMgrFree(work->tcbManager);
}

static int MedalBoxApp_GetBG(int index) {
    return sMedalBoxBGs[index].bg;
}

// Fades in, then lets the cover slide off the medals while the case comes down on the top screen
static BOOL MedalBoxApp_SeqStart(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0: {
        ClActorPos pos;

        GFL_FadeSet(3, 16, 0, 0);
        pos.x = 0x2c;
        pos.y = 0x46 - (work->caseY >> FX32_SHIFT);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE_BACK), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_TEXT), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        func_0204c140(work->actors[ACTOR_INFO_MEDAL], &pos, 1);
        work->subSeq++;
        break;
    }
    case 1:
        if (!GFL_FadeIsRunning()) {
            GFL_SndSEPlay(SEQ_SE_MD_01);
            work->subSeq++;
        }
        break;
    case 2: {
        ClActorPos coverPos;
        ClActorPos pos;
        BOOL coverDone = FALSE;
        BOOL caseDone = FALSE;

        work->coverX -= FX32_CONST(4);
        coverPos.x = (work->coverX >> FX32_SHIFT) + 0x80;
        coverPos.y = 0x60;
        if (coverPos.x < -0x50) {
            coverDone = TRUE;
            work->coverX = FX32_CONST(-0x50);
            coverPos.x = (work->coverX >> FX32_SHIFT) + 0x80;
            coverPos.y = 0x60;
        }
        work->caseY += FX32_CONST(5);
        if (work->caseY > 0) {
            work->caseY = 0;
            caseDone = TRUE;
        }
        func_0204c140(work->actors[ACTOR_COVER], &coverPos, 0);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_CASE_BACK), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_SUB_TEXT), BG_MOVE_SET_Y, work->caseY >> FX32_SHIFT);
        pos.x = 0x2c;
        pos.y = 0x46 - (work->caseY >> FX32_SHIFT);
        func_0204c140(work->actors[ACTOR_INFO_MEDAL], &pos, 1);
        if (coverDone && caseDone) {
            // The first time, a manual falls out of the case
            if (func_0200fb28(work->medalBox) == TRUE) {
                MedalBoxApp_RestoreCursor(work);
                MedalBoxApp_SetSeq(work, SEQ_MAIN);
            } else {
                MedalBoxApp_SetSeq(work, SEQ_MANUAL);
            }
        }
        break;
    }
    }
    return FALSE;
}

// Puts the cursor on the medal it was on the last time, if the keys are in use
static void MedalBoxApp_RestoreCursor(MedalBoxApp *work) {
    if (!func_0203d554()) {
        u8 medal = func_02017b58(work->param->gameData);
        int index = MedalBoxDataSys_FindMedal(&work->data, medal);

        if (medal != 0xff) {
            MedalBoxApp_SetCursor(work, index % MEDALBOX_COLUMN_SIZE, TRUE);
        } else {
            MedalBoxApp_SetCursor(work, 0, TRUE);
        }
    }
}

static BOOL MedalBoxApp_SeqMain(MedalBoxApp *work) {
    if (!work->printing) {
        MedalBoxApp_HandleInput(work);
    }
    MedalBoxApp_UpdateSparkles(work);
    BlinkPalAnm_Main(work->blinkAnm);
    return FALSE;
}

static void MedalBoxApp_HandleInput(MedalBoxApp *work) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    u32 held = GCTX_HIDGetHeldKeys();

    MedalBoxApp_StartDrag(work);
    MedalBoxApp_UpdateDrag(work);
    // A drag of the medals scrolls them by a column
    if (work->drag < -20) {
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
        MedalBoxApp_SetScrollCount(work, 1);
        MedalBoxApp_SetScrollDir(work, -1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        work->drag = 0;
        func_0203d564(TRUE);
        return;
    }
    if (work->drag > 20) {
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
        MedalBoxApp_SetScrollCount(work, 1);
        MedalBoxApp_SetScrollDir(work, 1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        work->drag = 0;
        func_0203d564(TRUE);
        return;
    }
    if (work->view == VIEW_NAMES) {
        TouchRect rects[MEDALBOX_COLUMN_SIZE + 1];
        int hit;

        MedalBoxApp_GetFavoriteRects(work, rects);
        hit = func_0203da0c(rects);
        if (hit != -1) {
            u8 column = MedalBoxApp_GetCursorColumn(work);

            if (work->shown[column].count > hit) {
                MedalBoxEntry *medal = work->shown[column].medals[hit];

                if (medal->status == MEDAL_STATUS_OBTAINED) {
                    if (medal->id != work->data.favoriteMedal) {
                        MedalBoxApp_SetFavorite(work, medal->id, FAVORITE_SET);
                    } else {
                        MedalBoxApp_SetFavorite(work, medal->id, FAVORITE_REMOVED);
                    }
                    MedalBoxApp_SetCursor(work, hit, FALSE);
                    MedalBoxApp_SetSeq(work, SEQ_FAVORITE);
                    func_0203d564(TRUE);
                    return;
                }
                MedalBoxApp_SetCursor(work, hit, FALSE);
                MedalBoxApp_SetFavorite(work, 0, FAVORITE_HINT);
                MedalBoxApp_SetSeq(work, SEQ_FAVORITE);
                func_0203d564(TRUE);
                return;
            }
        }
    }
    if (pressed & PAD_BUTTON_X) {
        MedalBoxApp_SetSeq(work, SEQ_EXIT);
        func_0203d564(FALSE);
        return;
    }
    if (pressed & PAD_BUTTON_B) {
        MedalBoxApp_SetSeq(work, SEQ_RETURN);
        func_0203d564(FALSE);
        return;
    }
    switch (func_0203da0c(sMedalBoxButtonRects)) {
    case BUTTON_MENU:
        MedalBoxApp_SetSeq(work, SEQ_MENU);
        func_0203d564(TRUE);
        return;
    case BUTTON_LEFT:
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
        MedalBoxApp_SetScrollCount(work, 1);
        MedalBoxApp_SetScrollDir(work, -1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        func_0203d564(TRUE);
        return;
    case BUTTON_RIGHT:
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
        MedalBoxApp_SetScrollCount(work, 1);
        MedalBoxApp_SetScrollDir(work, 1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        func_0203d564(TRUE);
        return;
    case BUTTON_RETURN:
        MedalBoxApp_SetSeq(work, SEQ_RETURN);
        func_0203d564(TRUE);
        return;
    case BUTTON_EXIT:
        MedalBoxApp_SetSeq(work, SEQ_EXIT);
        func_0203d564(TRUE);
        return;
    default: {
        TouchRect sliderRects[] = { { 0xa8, 0xc9, 0x3c, 0xac }, { TOUCH_RECT_END, 0, 0, 0 } };

        if (func_0203d9c8(sliderRects) == 0) {
            u32 x;
            u32 y;

            if (func_0203da84(&x, &y)) {
                if (x < SLIDER_X) {
                    work->sliderX = 0;
                } else if (x - SLIDER_X > SLIDER_WIDTH) {
                    work->sliderX = SLIDER_WIDTH;
                } else {
                    work->sliderX = x - SLIDER_X;
                }
                MedalBoxApp_SetSeq(work, SEQ_SLIDER);
                func_0203d564(TRUE);
                return;
            }
        } else {
            if (pressed & PAD_BUTTON_START) {
                MedalBoxApp_SetSeq(work, SEQ_MENU);
                func_0203d564(FALSE);
                return;
            }
            if (MedalBoxApp_HandleKeys(work)) {
                return;
            }
        }
        break;
    }
    }
}

// A touch starts a drag, on the medals only while the names are beside them
static void MedalBoxApp_StartDrag(MedalBoxApp *work) {
    u32 x;
    u32 y;

    if (func_0203dac8(&x, &y) && !work->dragging) {
        if (work->view == VIEW_NAMES) {
            if (x < COLUMN_WIDTH) {
                work->dragging = TRUE;
                work->touchX = x;
            }
        } else {
            work->dragging = TRUE;
            work->touchX = x;
        }
    }
    if (!func_0203da2c()) {
        work->dragging = FALSE;
    }
}

static BOOL MedalBoxApp_UpdateDrag(MedalBoxApp *work) {
    u32 x;
    u32 y;

    if (work->dragging && func_0203da84(&x, &y)) {
        work->drag += work->touchX - x;
        work->touchX = x;
        if (work->view == VIEW_NAMES && x > COLUMN_WIDTH) {
            work->dragging = FALSE;
        }
    }
    return FALSE;
}

// Scrolls the columns by scrollCount columns
static BOOL MedalBoxApp_SeqScroll(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0:
        MedalBoxApp_ClearNames(work);
        MedalBoxApp_SetCursorMode(work, CURSOR_HIDDEN, 0xff);
        if (work->scrollDir == 1) {
            func_0204c488(work->actors[ACTOR_RIGHT], 13);
            func_0204c520(work->actors[ACTOR_RIGHT], TRUE);
        } else {
            func_0204c488(work->actors[ACTOR_LEFT], 12);
            func_0204c520(work->actors[ACTOR_LEFT], TRUE);
        }
        GFL_SndSEPlay(SEQ_SE_MD_02);
        work->subSeq++;
        // fallthrough
    case 1:
        if (MedalBoxApp_Scroll(work, work->scrollSpeed * work->scrollDir)) {
            work->scrollCount--;
            if (work->scrollCount <= 0) {
                work->subSeq++;
            } else {
                GFL_SndSEPlay(SEQ_SE_MD_02);
            }
        }
        break;
    case 2: {
        u8 top = work->cursor / MEDALBOX_COLUMN_SIZE * MEDALBOX_COLUMN_SIZE;
        ClActorPos pos;

        if (!func_0203d554()) {
            switch (work->cursorEdge) {
            case CURSOR_EDGE_NONE:
                MedalBoxApp_SetCursor(work, work->cursor, TRUE);
                break;
            case CURSOR_EDGE_TOP:
                MedalBoxApp_SetCursor(work, top, TRUE);
                break;
            case CURSOR_EDGE_BOTTOM:
                MedalBoxApp_SetCursor(work, top + MEDALBOX_COLUMN_SIZE - 1, TRUE);
                break;
            }
        } else {
            work->prevCursor = work->cursor;
            MedalBoxApp_SetCursorMode(work, CURSOR_TOUCH, 0xff);
        }
        work->cursorEdge = CURSOR_EDGE_NONE;
        work->scrollCount = 1;
        work->sliderX = FX_Mul(FX_Div(work->column << FX32_SHIFT, (work->data.columnCount - 1) << FX32_SHIFT),
                               FX32_CONST(SLIDER_WIDTH)) >>
                        FX32_SHIFT;
        pos.x = work->sliderX + SLIDER_X;
        pos.y = SLIDER_Y;
        func_0204c140(work->actors[ACTOR_SLIDER], &pos, 0);
        MedalBoxApp_FlushNames(work);
        MedalBoxApp_SetSeq(work, SEQ_MAIN);
        break;
    }
    }
    return FALSE;
}

static void MedalBoxApp_SetScrollDir(MedalBoxApp *work, int dir) {
    work->scrollDir = dir;
}

// Moves the columns, and returns whether they went a whole column, which then is the next one
static BOOL MedalBoxApp_Scroll(MedalBoxApp *work, fx32 speed) {
    BOOL done = FALSE;

    work->scrollX += speed;
    if (MATH_ABS(work->scrollX >> FX32_SHIFT) >= COLUMN_WIDTH) {
        if (work->column == 0 && work->scrollDir < 0) {
            work->column = work->data.columnCount - 1;
        } else {
            work->column += work->scrollDir;
            work->column %= (u32)work->data.columnCount;
        }
        work->scrollX = 0;
        done = TRUE;
        MedalBoxApp_DrawColumns(work, TRUE, FALSE);
    }
    MedalBoxApp_MoveMedals(work);
    MedalBoxApp_MoveSparkles(work, speed >> FX32_SHIFT);
    return done;
}

static void MedalBoxApp_MoveMedals(MedalBoxApp *work) {
    int x = work->scrollX >> FX32_SHIFT;
    s16 actorX;
    int bgX;
    ClActorPos pos;
    int i;

    bgX = x + COLUMN_WIDTH;
    actorX = x;
    for (i = 0; i < MEDAL_ACTOR_COUNT; i++) {
        MedalBoxApp_GetMedalPos(i, &pos);
        pos.x -= actorX;
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_NUMBER), BG_MOVE_SET_X, bgX);
        func_0204c140(work->actors[ACTOR_MEDAL + i], &pos, 0);
        func_0204c140(work->actors[MedalBoxApp_GetWindowActor(i)], &pos, 0);
    }
}

static void MedalBoxApp_MoveSparkles(MedalBoxApp *work, s16 x) {
    int i;

    for (i = 0; i < SPARKLE_COUNT; i++) {
        if (func_0204c560(work->actors[ACTOR_SPARKLE + i]) == TRUE) {
            ClActorPos pos;

            func_0204c178(work->actors[ACTOR_SPARKLE + i], &pos, 0);
            pos.x -= x;
            func_0204c140(work->actors[ACTOR_SPARKLE + i], &pos, 0);
        }
    }
}

// Shows the columns around the current one, and puts the slider where it is
static void MedalBoxApp_DrawColumns(MedalBoxApp *work, BOOL names, BOOL slider) {
    int count;
    int i;

    if (work->view == VIEW_NAMES) {
        count = SHOWN_NAMES;
    } else if (work->view == VIEW_COLUMNS) {
        count = SHOWN_ALL;
    }
    if (names) {
        MedalBoxApp_ClearNames(work);
    }
    func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
    for (i = 0; i < count; i++) {
        int column = work->column + i - 1;

        if (column < 0) {
            MedalBoxApp_DrawColumn(work, work->data.columnCount - 1, i, names);
        } else if (column >= work->data.columnCount) {
            MedalBoxApp_DrawColumn(work, column % work->data.columnCount, i, names);
        } else {
            MedalBoxApp_DrawColumn(work, column, i, names);
        }
    }
    if (names) {
        MedalBoxApp_FlushNames(work);
    }
    if (slider == TRUE) {
        ClActorPos pos;

        work->sliderX = FX_Mul(FX_Div(work->column << FX32_SHIFT, (work->data.columnCount - 1) << FX32_SHIFT),
                               FX32_CONST(SLIDER_WIDTH)) >>
                        FX32_SHIFT;
        pos.x = work->sliderX + SLIDER_X;
        pos.y = SLIDER_Y;
        func_0204c140(work->actors[ACTOR_SLIDER], &pos, 0);
    }
}

static void MedalBoxApp_PrintName(MedalBoxApp *work, u32 medal, u8 slot) {
    u16 color = MedalBoxApp_GetNameColor();

    if (work->view == VIEW_NAMES) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_MEDAL_NAMES], medal);

        PrintWindow_Print(&work->printWindows[WINDOW_NAMES], work->printQueue, sMedalBoxNamePos[slot].x,
                          sMedalBoxNamePos[slot].y, str, work->font, color);
        GFL_StrBufFree(str);
    }
}

static void MedalBoxApp_FlushNames(MedalBoxApp *work) {
    BmpWin_FlushChar(work->windows[WINDOW_NAMES]);
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_TEXT));
}

// Draws the top screen: a medal with its name, date and description, or the goal of the rank without one
static void MedalBoxApp_DrawInfo(MedalBoxApp *work, MedalBoxEntry *medal, BOOL show) {
    u16 color;
    u32 obtained;
    u32 total;
    int rank;
    WordSet *wordSet;
    ClActor *actor;
    StrBuf *str;
    StrBuf *buf;
    u8 goal;

    color = MedalBoxApp_GetInfoColor();
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    rank = MedalBox_GetRank(work->medalBox);
    actor = work->actors[ACTOR_INFO_MEDAL];
    if (show == TRUE) {
        if (medal->status == MEDAL_STATUS_OBTAINED) {
            func_0204c488(actor, medal->sequence);
        } else {
            func_0204c488(actor, func_0204c4a8(actor) - 1);
        }
        func_0204c124(actor, TRUE);
    } else {
        func_0204c124(actor, FALSE);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_INFO]), color & 0x1f);
    if (show == FALSE && rank != MEDAL_RANK_MAX) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_MedalCollectionGoal);

        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 8, 8, str, work->font, color);
        GFL_StrBufFree(str);
    }
    if (show == TRUE) {
        if (medal->status == MEDAL_STATUS_OBTAINED) {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_MedalsCollected);
        } else {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_HintMedals);
        }
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 8, 8, str, work->font, color);
        GFL_StrBufFree(str);
    }
    obtained = MedalBox_GetObtainedCount(work->medalBox, FALSE);
    total = MedalBox_GetObtainedCount(work->medalBox, TRUE);
    goal = MedalBox_GetNextRankRequirement(work->medalBox) + 1;
    if (show == TRUE) {
        buf = GFL_StrBufCreate(20, work->heapId);
        str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_Empty_2);
        if (medal->status == MEDAL_STATUS_OBTAINED) {
            WordSetNumber(wordSet, 0, obtained, 3, 1, TRUE);
            WordSetNumber(wordSet, 1, total, 3, 1, TRUE);
        } else {
            WordSetNumber(wordSet, 0, total - obtained, 3, 1, TRUE);
            WordSetNumber(wordSet, 1, total, 3, 1, TRUE);
        }
        GFL_WordSetFormatStrbuf(wordSet, buf, str);
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 200, 8, buf, work->font, color);
        GFL_StrBufFree(str);
        GFL_StrBufFree(buf);
    }
    if (show == FALSE && rank != MEDAL_RANK_MAX) {
        StrBuf *buf = GFL_StrBufCreate(20, work->heapId);
        StrBuf *format = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_Empty);

        WordSetNumber(wordSet, 0, goal, 3, 1, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, buf, format);
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 200, 8, buf, work->font, color);
        GFL_StrBufFree(format);
        GFL_StrBufFree(buf);
    }
    if (show == TRUE) {
        u32 width;

        if (medal->status == MEDAL_STATUS_OBTAINED) {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_MEDAL_NAMES], medal->id);
        } else {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_MEDAL_NAMES], MEDAL_NAME_NONE);
        }
        width = GFL_FontGetBlockWidth(str, work->font, 0);
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 232 - width, 48, str, work->font,
                          color);
        GFL_StrBufFree(str);
    }
    if (show == TRUE) {
        StrBuf *buf = GFL_StrBufCreate(30, work->heapId);
        StrBuf *format;

        WordSetNumber(wordSet, 0, medal->year, 2, 2, TRUE);
        WordSetNumber(wordSet, 1, medal->month, 3, 0, TRUE);
        WordSetNumber(wordSet, 2, medal->day, 3, 0, TRUE);
        if (medal->status == MEDAL_STATUS_OBTAINED) {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_DateObtained);
        } else {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_DateReceived);
        }
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 232, 80, str, work->font, color);
        GFL_StrBufFree(str);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_20);
        GFL_WordSetFormatStrbuf(wordSet, buf, format);
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 232, 96, buf, work->font, color);
        GFL_StrBufFree(format);
        GFL_StrBufFree(buf);
    }
    if (show == TRUE) {
        if (medal->status == MEDAL_STATUS_OBTAINED) {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_MEDAL_HINTS], medal->id);
        } else {
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_MEDAL_INFO], medal->id);
        }
        PrintWindow_Print(&work->printWindows[WINDOW_INFO], work->printQueue, 20, 128, str, work->font, color);
        GFL_StrBufFree(str);
    }
    if (show == FALSE && rank == MEDAL_RANK_MAX) {
        BmpWin_FlushChar(work->windows[WINDOW_INFO]);
        GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_SUB_TEXT));
    }
    GFL_WordSetSystemFree(wordSet);
    // The medal has been looked at now
    if (show == TRUE) {
        MedalBox_SetMedalNotSpecial(work->medalBox, medal->id);
    }
}

static void MedalBoxApp_ClearNames(MedalBoxApp *work) {
    u16 color = MedalBoxApp_GetNameColor();

    GFL_BitmapFill(BmpWin_GetBitmap(work->windows[WINDOW_NAMES]), 0);
    BmpWin_FlushChar(work->windows[WINDOW_NAMES]);
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_TEXT));
}

static u16 MedalBoxApp_GetNameColor(void) {
    return COLOR_NAME;
}

static u16 MedalBoxApp_GetInfoColor(void) {
    return COLOR_INFO;
}

// The medals of the three columns on the screen
static void MedalBoxApp_GetMedalRects(MedalBoxApp *work, TouchRect *rects) {
    ClActorPos pos;
    u8 i;

    for (i = 0; i < 3 * MEDALBOX_COLUMN_SIZE; i++) {
        MedalBoxApp_GetMedalPos(i + MEDALBOX_COLUMN_SIZE, &pos);
        rects[i].top = pos.y - 16;
        rects[i].bottom = pos.y + 16;
        rects[i].left = pos.x - 24;
        rects[i].right = pos.x + 24;
    }
    rects[i].top = TOUCH_RECT_END;
    rects[i].bottom = 0;
    rects[i].left = 0;
    rects[i].right = 0;
}

static void MedalBoxApp_GetNameRects(MedalBoxApp *work, TouchRect *rects) {
    int i;

    for (i = 0; i < MEDALBOX_COLUMN_SIZE; i++) {
        u8 x = sMedalBoxNamePos[i].x;
        u8 y = sMedalBoxNamePos[i].y;

        rects[i].top = y;
        rects[i].bottom = y + 24;
        rects[i].left = x;
        rects[i].right = x + 120;
    }
    rects[i].top = TOUCH_RECT_END;
    rects[i].bottom = 0;
    rects[i].left = 0;
    rects[i].right = 0;
}

static void MedalBoxApp_GetFavoriteRects(MedalBoxApp *work, TouchRect *rects) {
    ClActorPos pos;
    u8 i;

    for (i = 0; i < MEDALBOX_COLUMN_SIZE; i++) {
        MedalBoxApp_GetFavoritePos(i, &pos);
        rects[i].top = pos.y - 16;
        rects[i].bottom = pos.y + 16;
        rects[i].left = pos.x - 16;
        rects[i].right = pos.x + 16;
    }
    rects[i].top = TOUCH_RECT_END;
    rects[i].bottom = 0;
    rects[i].left = 0;
    rects[i].right = 0;
}

static void MedalBoxApp_GetCursorPos(MedalBoxApp *work, u8 cursor, ClActorPos *pos) {
    MedalBoxApp_GetMedalPos(cursor + MEDALBOX_COLUMN_SIZE, pos);
    pos->y -= 4;
}

// Moves the cursor with the keys or a touch of a medal or a name. Returns whether anything was done
static BOOL MedalBoxApp_HandleKeys(MedalBoxApp *work) {
    u32 pressed = GCTX_HIDGetPressedKeys();
    u32 typed = GCTX_HIDGetTypedKeys();
    u32 held = GCTX_HIDGetHeldKeys();
    u8 column = MedalBoxApp_GetCursorColumn(work);
    u8 slot = MedalBoxApp_GetCursorSlot(work);
    u8 cursor;
    int hit;

    if (work->view == VIEW_NAMES) {
        TouchRect rects[MEDALBOX_COLUMN_SIZE + 1];

        MedalBoxApp_GetNameRects(work, rects);
        hit = func_0203da0c(rects);
        if (hit != -1) {
            func_0203d564(TRUE);
            MedalBoxApp_SetCursor(work, hit, TRUE);
            return TRUE;
        }
    }
    {
        TouchRect rects[3 * MEDALBOX_COLUMN_SIZE + 1];

        MedalBoxApp_GetMedalRects(work, rects);
        hit = func_0203da0c(rects);
        if (hit != -1) {
            func_0203d564(TRUE);
            MedalBoxApp_SetCursor(work, hit, TRUE);
            return TRUE;
        }
    }
    cursor = work->cursor;
    // The first key brings the cursor back
    if (work->cursorMode == CURSOR_TOUCH) {
        if ((pressed & PAD_KEY_UP) || (pressed & PAD_KEY_DOWN) || (pressed & PAD_KEY_LEFT) ||
            (pressed & PAD_KEY_RIGHT) || (pressed & PAD_BUTTON_L) || (pressed & PAD_BUTTON_R) ||
            (pressed & PAD_BUTTON_A)) {
            func_0203d564(FALSE);
            MedalBoxApp_SetCursor(work, work->prevCursor, TRUE);
            work->prevCursor = 0;
            return TRUE;
        }
    } else if (typed & PAD_KEY_UP) {
        if (cursor == 0) {
            MedalBoxApp_SetCursorEdge(work, CURSOR_EDGE_BOTTOM);
            MedalBoxApp_ScrollLeft(work);
            return TRUE;
        }
        func_0203d564(FALSE);
        MedalBoxApp_SetCursor(work, cursor - 1, TRUE);
        return TRUE;
    } else if (typed & PAD_KEY_DOWN) {
        if (work->view == VIEW_NAMES && cursor == MEDALBOX_COLUMN_SIZE - 1) {
            MedalBoxApp_SetCursorEdge(work, CURSOR_EDGE_TOP);
            MedalBoxApp_ScrollRight(work);
            return TRUE;
        }
        if (work->view == VIEW_COLUMNS && cursor == 3 * MEDALBOX_COLUMN_SIZE - 1) {
            MedalBoxApp_SetCursorEdge(work, CURSOR_EDGE_TOP);
            MedalBoxApp_ScrollRight(work);
            return TRUE;
        }
        func_0203d564(FALSE);
        MedalBoxApp_SetCursor(work, cursor + 1, TRUE);
        return TRUE;
    } else if (typed & PAD_KEY_RIGHT) {
        column = MedalBoxApp_GetCursorColumn(work);
        if ((work->view == VIEW_NAMES && column == 1) || (work->view == VIEW_COLUMNS && column == 3)) {
            MedalBoxApp_ScrollRight(work);
            return TRUE;
        }
        MedalBoxApp_SetCursor(work, cursor + MEDALBOX_COLUMN_SIZE, TRUE);
        return TRUE;
    } else if (typed & PAD_KEY_LEFT) {
        if (MedalBoxApp_GetCursorColumn(work) == 1) {
            MedalBoxApp_ScrollLeft(work);
            return TRUE;
        }
        MedalBoxApp_SetCursor(work, cursor - MEDALBOX_COLUMN_SIZE, TRUE);
        return TRUE;
    } else if (typed & PAD_BUTTON_R) {
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(20));
        MedalBoxApp_SetScrollCount(work, 3);
        MedalBoxApp_SetScrollDir(work, 1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        func_0203d564(FALSE);
        return TRUE;
    } else if (typed & PAD_BUTTON_L) {
        MedalBoxApp_SetScrollSpeed(work, FX32_CONST(20));
        MedalBoxApp_SetScrollCount(work, 3);
        MedalBoxApp_SetScrollDir(work, -1);
        MedalBoxApp_SetSeq(work, SEQ_SCROLL);
        func_0203d564(FALSE);
        return TRUE;
    } else if (pressed & PAD_BUTTON_A) {
        column = MedalBoxApp_GetCursorColumn(work);
        slot = MedalBoxApp_GetCursorSlot(work);
        if (work->shown[column].count > slot) {
            MedalBoxEntry *medal = work->shown[column].medals[slot];

            if (medal->status == MEDAL_STATUS_OBTAINED) {
                if (medal->id != work->data.favoriteMedal) {
                    MedalBoxApp_SetFavorite(work, medal->id, FAVORITE_SET);
                } else {
                    MedalBoxApp_SetFavorite(work, medal->id, FAVORITE_REMOVED);
                }
                MedalBoxApp_SetSeq(work, SEQ_FAVORITE);
                func_0203d564(FALSE);
                return TRUE;
            }
            MedalBoxApp_SetFavorite(work, 0, FAVORITE_HINT);
            MedalBoxApp_SetSeq(work, SEQ_FAVORITE);
            func_0203d564(TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

static void MedalBoxApp_ScrollRight(MedalBoxApp *work) {
    MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
    MedalBoxApp_SetScrollCount(work, 1);
    MedalBoxApp_SetScrollDir(work, 1);
    MedalBoxApp_SetSeq(work, SEQ_SCROLL);
    func_0203d564(FALSE);
}

static void MedalBoxApp_ScrollLeft(MedalBoxApp *work) {
    MedalBoxApp_SetScrollSpeed(work, FX32_CONST(7));
    MedalBoxApp_SetScrollCount(work, 1);
    MedalBoxApp_SetScrollDir(work, -1);
    MedalBoxApp_SetSeq(work, SEQ_SCROLL);
    func_0203d564(FALSE);
}

// Puts the cursor on a medal and shows it on the top screen. With the names shown it stays in the middle column
static void MedalBoxApp_SetCursor(MedalBoxApp *work, u8 cursor, BOOL keys) {
    u8 prev = work->cursor;
    u8 column;
    u8 slot;
    MedalBoxEntry *medal;
    ClActorPos pos;

    if (prev == cursor && work->cursorMode == CURSOR_SHOWN) {
        return;
    }
    work->cursor = cursor;
    column = MedalBoxApp_GetCursorColumn(work);
    slot = MedalBoxApp_GetCursorSlot(work);
    if (work->view == VIEW_NAMES && column != 1) {
        work->cursor = prev;
        return;
    }
    medal = work->shown[column].medals[slot];
    if (medal != NULL) {
        MedalBoxApp_GetCursorPos(work, cursor, &pos);
        func_0204c140(work->actors[ACTOR_CURSOR], &pos, 0);
        MedalBoxApp_DrawInfo(work, medal, TRUE);
        MedalBoxApp_SetCursorMode(work, CURSOR_SHOWN, medal->id);
    } else {
        MedalBoxApp_GetCursorPos(work, cursor, &pos);
        func_0204c140(work->actors[ACTOR_CURSOR], &pos, 0);
        MedalBoxApp_SetCursorMode(work, CURSOR_SHOWN, 0xff);
        MedalBoxApp_DrawInfo(work, medal, FALSE);
    }
}

// Shows or hides the cursor, and remembers its medal for the next time
static void MedalBoxApp_SetCursorMode(MedalBoxApp *work, int mode, u8 medal) {
    work->cursorMode = mode;
    func_02017b4c(work->param->gameData, medal);
    switch (work->cursorMode) {
    case CURSOR_TOUCH:
        MedalBoxApp_DrawInfo(work, NULL, FALSE);
        func_0204c124(work->actors[ACTOR_CURSOR], FALSE);
        break;
    case CURSOR_HIDDEN:
        func_0204c124(work->actors[ACTOR_CURSOR], FALSE);
        break;
    case CURSOR_SHOWN:
        GFL_SndSEPlay(SEQ_SE_MD_03);
        func_0204c124(work->actors[ACTOR_CURSOR], TRUE);
        break;
    }
}

// Lets the medals that were not looked at yet sparkle
static void MedalBoxApp_UpdateSparkles(MedalBoxApp *work) {
    ClActor **actors = work->actors;
    ClActorPos pos;
    int i;

    for (i = 0; i < SPARKLE_COUNT; i++) {
        if (GFL_RandomMTRange(10) == 0 && !func_0204c560(actors[ACTOR_SPARKLE + i])) {
            u8 shown = GFL_RandomMTRange(4) + 1;
            MedalBoxColumn *medals = &work->shown[shown];
            u8 slot = GFL_RandomMTRange(MEDALBOX_COLUMN_SIZE);

            if (medals->count > slot) {
                if (!medals->medals[slot]->special) {
                    continue;
                }
                MedalBoxApp_GetMedalPos(slot + shown * MEDALBOX_COLUMN_SIZE, &pos);
                pos.x -= (s16)(work->scrollX + (GFL_RandomMTRange(26) - 13));
                pos.y -= (s16)(work->scrollX + (GFL_RandomMTRange(26) - 13));
                func_0204c488(actors[ACTOR_SPARKLE + i], 4);
                func_0204c140(actors[ACTOR_SPARKLE + i], &pos, 0);
                func_0204c124(work->actors[ACTOR_SPARKLE + i], TRUE);
                func_0204c520(actors[ACTOR_SPARKLE + i], TRUE);
            } else {
                func_0204c488(actors[ACTOR_SPARKLE + i], 4);
                func_0204c140(actors[ACTOR_SPARKLE + i], &pos, 0);
                func_0204c124(work->actors[ACTOR_SPARKLE + i], FALSE);
                func_0204c520(actors[ACTOR_SPARKLE + i], TRUE);
            }
        }
    }
}

// The shown column the cursor is in, and its medal there
static u8 MedalBoxApp_GetCursorColumn(MedalBoxApp *work) {
    return work->cursor / MEDALBOX_COLUMN_SIZE + 1;
}

static u8 MedalBoxApp_GetCursorSlot(MedalBoxApp *work) {
    return work->cursor % MEDALBOX_COLUMN_SIZE;
}

static void MedalBoxApp_SetScrollCount(MedalBoxApp *work, int count) {
    work->scrollCount = count;
}

static void MedalBoxApp_SetCursorEdge(MedalBoxApp *work, int edge) {
    work->cursorEdge = edge;
}

static void MedalBoxApp_SetScrollSpeed(MedalBoxApp *work, u32 speed) {
    work->scrollSpeed = speed;
}

static BOOL MedalBoxApp_SeqExit(MedalBoxApp *work) {
    ClActor *button = work->actors[ACTOR_EXIT];

    switch (work->subSeq) {
    case 0:
        func_0204c488(button, 8);
        func_0204c520(button, TRUE);
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        work->subSeq++;
        break;
    case 1:
        if (!func_0204c560(button)) {
            work->subSeq++;
        }
        break;
    case 2:
        GFL_FadeSet(3, 0, 16, 0);
        work->subSeq++;
        break;
    case 3:
        if (!GFL_FadeIsRunning()) {
            work->param->result = MEDALBOX_RESULT_EXIT;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MedalBoxApp_SeqReturn(MedalBoxApp *work) {
    ClActor *button = work->actors[ACTOR_RETURN];

    switch (work->subSeq) {
    case 0:
        func_0204c488(button, 9);
        func_0204c520(button, TRUE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        work->subSeq++;
        break;
    case 1:
        if (!func_0204c560(button)) {
            work->subSeq++;
        }
        break;
    case 2:
        GFL_FadeSet(3, 0, 16, 0);
        work->subSeq++;
        break;
    case 3:
        if (!GFL_FadeIsRunning()) {
            work->param->result = MEDALBOX_RESULT_RETURN;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The slider is held: the columns run after it, and go on to its column once it is let go
static BOOL MedalBoxApp_SeqSlider(MedalBoxApp *work) {
    u32 x;
    u32 y;

    switch (work->subSeq) {
    case 0:
        MedalBoxApp_ClearNames(work);
        MedalBoxApp_SetCursorMode(work, CURSOR_HIDDEN, 0xff);
        work->scrollSpeed = FX32_CONST(9);
        work->subSeq++;
        // fallthrough
    case 1:
        if (func_0203da84(&x, &y)) {
            if (x < SLIDER_X) {
                work->sliderX = 0;
            } else if (x - SLIDER_X > SLIDER_WIDTH) {
                work->sliderX = SLIDER_WIDTH;
            } else {
                work->sliderX = x - SLIDER_X;
            }
            if (work->column == FX_Mul((work->data.columnCount - 1) << FX32_SHIFT,
                                       FX_Div(work->sliderX << FX32_SHIFT, FX32_CONST(SLIDER_WIDTH))) >>
                                    FX32_SHIFT) {
                return FALSE;
            }
            MedalBoxApp_UpdateSlider(work);
        } else {
            work->subSeq++;
        }
        break;
    case 2:
        if (MedalBoxApp_UpdateSlider(work) == TRUE) {
            work->prevCursor = work->cursor;
            MedalBoxApp_SetCursorMode(work, CURSOR_TOUCH, 0xff);
            MedalBoxApp_SetSeq(work, SEQ_MAIN);
        }
        break;
    }
    return FALSE;
}

// Scrolls toward the slider's column, faster while it is far, and returns whether the columns are there
static BOOL MedalBoxApp_UpdateSlider(MedalBoxApp *work) {
    ClActorPos pos;
    u32 distance;
    fx32 target;

    target = FX_Mul((work->data.columnCount - 1) << FX32_SHIFT,
                    FX_Div(work->sliderX << FX32_SHIFT, FX32_CONST(SLIDER_WIDTH)));
    pos.x = work->sliderX + SLIDER_X;
    pos.y = SLIDER_Y;
    func_0204c140(work->actors[ACTOR_SLIDER], &pos, 0);
    if ((target >> FX32_SHIFT) < work->column) {
        MedalBoxApp_SetScrollDir(work, -1);
    } else if ((target >> FX32_SHIFT) > work->column) {
        MedalBoxApp_SetScrollDir(work, 1);
    } else {
        work->scrollX = 0;
        MedalBoxApp_MoveMedals(work);
        MedalBoxApp_MoveSparkles(work, work->scrollX >> FX32_SHIFT);
        MedalBoxApp_DrawColumns(work, TRUE, FALSE);
        return TRUE;
    }
    distance = MATH_ABS((target >> FX32_SHIFT) - work->column);
    if (distance > 5) {
        work->scrollSpeed += FX32_CONST(2);
        if (work->scrollSpeed > FX32_CONST(32)) {
            work->scrollSpeed = FX32_CONST(32);
        }
    } else {
        work->scrollSpeed -= 0x1500;
        if (work->scrollSpeed < FX32_CONST(9)) {
            work->scrollSpeed = FX32_CONST(9);
        }
    }
    if (MedalBoxApp_Scroll(work, work->scrollSpeed * work->scrollDir)) {
        GFL_SndSEPlay(SEQ_SE_MD_02);
        // At full speed, columns are skipped
        if (work->scrollSpeed == FX32_CONST(32)) {
            u16 skip = distance / 3;

            if ((target >> FX32_SHIFT) < work->column) {
                work->column -= skip;
            } else if ((target >> FX32_SHIFT) > work->column) {
                work->column += skip;
            } else {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// The menu of the view and the orders
static BOOL MedalBoxApp_SeqMenu(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0: {
        u8 column = MedalBoxApp_GetCursorColumn(work);

        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), FALSE);
        func_0204c488(work->actors[ACTOR_MENU], 1);
        func_0204c520(work->actors[ACTOR_MENU], TRUE);
        // A change of the view keeps the cursor's column
        if (work->shown[column].count != 0 && work->cursorMode == CURSOR_SHOWN) {
            work->nextColumn = work->shown[column].column;
        } else {
            work->nextColumn = 0;
        }
        work->prevCursor = work->cursor;
        MedalBoxApp_SetCursorMode(work, CURSOR_TOUCH, 0xff);
        work->subSeq++;
        break;
    }
    case 1:
        if (!func_0204c560(work->actors[ACTOR_MENU])) {
            work->subSeq++;
        }
        break;
    case 2:
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), BG_MOVE_SET_X, 0);
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0x1d, -8);
        GFL_BGSysClearBG(MedalBoxApp_GetBG(BG_MAIN_MESSAGE));
        MedalBoxApp_OpenMenu(work);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), TRUE);
        work->subSeq++;
        break;
    case 3: {
        u8 pos;

        AppTaskMenu_Update(work->menu);
        if (!AppTaskMenu_IsFlashFinished(work->menu)) {
            break;
        }
        pos = AppTaskMenu_GetCursorPos(work->menu);
        AppTaskMenu_Free(work->menu);
        GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 0);
        AppTaskMenuRes_Free(work->menuRes);
        BmpWin_Free(work->messageWindow);
        MedalBoxApp_ReloadMenuBG(work);
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, 0, 0);
        work->menuItemCount = 0;
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 2);
        work->subSeq++;
        switch (sMedalBoxMenuItems[work->data.sort][pos]) {
        case Title10_Text_Browser:
        case Title10_Text_Columns:
            MedalBoxApp_SetSeq(work, SEQ_CHANGE_VIEW);
            break;
        case Title10_Text_MedalsOnly:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_MEDALS_ONLY);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_HintMedalsOnly:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_HINTS_ONLY);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_DisplayByType:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_TYPE);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_NewestFirst:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_NEWEST);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_DisplayByName:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_NAME);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_Revert:
            MedalBoxDataSys_Sort(&work->data, MEDALBOX_SORT_OLDEST);
            MedalBoxApp_SetSeq(work, SEQ_SORT);
            break;
        case Title10_Text_Back:
        default:
            if (!func_0203d554()) {
                MedalBoxApp_SetCursor(work, work->cursor, TRUE);
            } else {
                work->prevCursor = work->cursor;
            }
            MedalBoxApp_SetSeq(work, SEQ_MAIN);
            break;
        }
        break;
    }
    }
    return FALSE;
}

static void MedalBoxApp_OpenMenu(MedalBoxApp *work) {
    int sort = work->data.sort;
    u16 color = MedalBoxApp_GetInfoColor();
    AppTaskMenuInit init;
    StrBuf *str;
    int i;

    work->menuItemCount = 0;
    for (i = 0; i < APP_TASKMENU_ITEM_MAX; i++) {
        if (sMedalBoxMenuItems[sort][i] != MENU_ITEM_NONE) {
            // The first button changes the view
            if (i == 0) {
                if (work->view == VIEW_NAMES) {
                    work->menuItems[work->menuItemCount].str =
                        GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_Browser);
                } else {
                    work->menuItems[work->menuItemCount].str =
                        GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_Columns);
                }
            } else {
                work->menuItems[work->menuItemCount].str =
                    GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], sMedalBoxMenuItems[sort][i]);
            }
            work->menuItems[work->menuItemCount].color = COLOR_MENU;
            work->menuItems[work->menuItemCount].type = 0;
            work->menuItemCount++;
        }
    }
    (work->menuItems + work->menuItemCount - 1)->type = APP_TASKMENU_ITEM_RETURN;
    init.heapId = work->heapId;
    init.itemCount = work->menuItemCount;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 13;
    init.height = 3;
    work->menuRes = AppTaskMenuRes_Create(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 13, work->font, work->printQueue,
                                          work->heapId);
    work->menu = AppTaskMenu_Create(&init, work->menuRes);
    work->messageWindow = BmpWin_CreateDynamic(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 1, 19, 17, 4, 11, 1);
    BmpWin_FlushMap(work->messageWindow);
    LoadSysMsgBox(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 1, 12, 0, work->heapId);
    BmpWin_DrawFrame(work->messageWindow, 1, 1, 12);
    str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_HowShouldMedalsDisplayed);
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    BmpWin_FlushChar(work->messageWindow);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(work->messageWindow), 0, 0, str, work->font);
    BmpWin_FlushChar(work->messageWindow);
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_MESSAGE));
    GFL_StrBufFree(str);
    for (i = 0; i < work->menuItemCount; i++) {
        GFL_StrBufFree(work->menuItems[i].str);
    }
}

// Changes the view behind the cover: with five columns the touch screen's back moves aside
static BOOL MedalBoxApp_SeqChangeView(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_MD_04);
        MedalBoxApp_ClearNames(work);
        func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
        work->subSeq++;
        // fallthrough
    case 1: {
        ClActorPos pos;
        BOOL backDone = TRUE;
        BOOL coverDone = FALSE;

        work->coverX += FX32_CONST(4);
        pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
        pos.y = 0x60;
        if (pos.x > 0x80) {
            work->coverX = 0;
            pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
            pos.y = 0x60;
            coverDone = TRUE;
        }
        func_0204c140(work->actors[ACTOR_COVER], &pos, 0);
        if (work->view == VIEW_COLUMNS) {
            backDone = FALSE;
            work->backX += 0x8ccc;
            if ((work->backX >> FX32_SHIFT) > 0) {
                work->backX = 0;
                backDone = TRUE;
            }
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_BACK), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_TEXT), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
        }
        if (coverDone && backDone) {
            work->subSeq++;
        }
        break;
    }
    case 2:
        if (work->view == VIEW_NAMES) {
            work->view = VIEW_COLUMNS;
        } else {
            work->view = VIEW_NAMES;
        }
        work->column = work->nextColumn;
        MedalBoxApp_DrawColumns(work, FALSE, TRUE);
        MedalBoxApp_ClearNames(work);
        func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
        work->wait = 0;
        work->subSeq++;
        break;
    case 3:
        work->wait++;
        if (work->wait > 6) {
            work->subSeq++;
        }
        break;
    case 4: {
        ClActorPos pos;
        BOOL coverDone = FALSE;
        BOOL backDone = TRUE;

        work->coverX -= FX32_CONST(4);
        pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
        pos.y = 0x60;
        if (pos.x < -0x50) {
            coverDone = TRUE;
            work->coverX = FX32_CONST(-0x50);
            pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
            pos.y = 0x60;
        }
        func_0204c140(work->actors[ACTOR_COVER], &pos, 0);
        if (work->view == VIEW_COLUMNS) {
            backDone = FALSE;
            work->backX -= 0x8ccc;
            if ((work->backX >> FX32_SHIFT) < -0xb8) {
                work->backX = FX32_CONST(-0xb8);
                backDone = TRUE;
            }
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_BACK), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_TEXT), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
        }
        if (work->view == VIEW_NAMES) {
            work->cursor %= MEDALBOX_COLUMN_SIZE;
        }
        if (coverDone && backDone) {
            MedalBoxApp_DrawColumns(work, TRUE, TRUE);
            if (!func_0203d554()) {
                MedalBoxApp_SetCursor(work, work->cursor, TRUE);
            } else {
                work->prevCursor = work->cursor;
            }
            MedalBoxApp_SetSeq(work, SEQ_MAIN);
        }
        break;
    }
    }
    return FALSE;
}

// The message after a favorite medal was registered or removed
static BOOL MedalBoxApp_SeqFavorite(MedalBoxApp *work) {
    StrBuf *str;

    switch (work->subSeq) {
    case 0:
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), FALSE);
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), BG_MOVE_SET_X, 0);
        work->subSeq++;
        break;
    case 1:
        if (work->favoriteResult != FAVORITE_HINT) {
            GFL_SndSEPlay(SEQ_SE_SAVE);
        } else {
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
        switch (work->favoriteResult) {
        case FAVORITE_SET:
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_RegisteringFavoriteMedalWill);
            break;
        case FAVORITE_REMOVED:
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_Removed);
            break;
        case FAVORITE_HINT:
            str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_HintMedalCannotRegistered);
            break;
        }
        MedalBoxApp_ShowMessage(work, str);
        GFL_StrBufFree(str);
        MedalBoxApp_DrawColumns(work, FALSE, TRUE);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), TRUE);
        work->subSeq++;
        break;
    case 2: {
        int i;

        for (i = 0; i < 4; i++) {
            BlinkPalAnm_Main(work->blinkAnm);
        }
        if (work->stream == NULL) {
            GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), FALSE);
            work->subSeq++;
        }
        break;
    }
    case 3:
        MedalBoxApp_CloseMessage(work);
        MedalBoxApp_ReloadMenuBG(work);
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 2);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), TRUE);
        MedalBoxApp_SetSeq(work, SEQ_MAIN);
        work->subSeq++;
        break;
    }
    return FALSE;
}

// Registers a medal as the favorite one, which the game's beacon sends, or removes it
static void MedalBoxApp_SetFavorite(MedalBoxApp *work, u16 medal, int result) {
    work->favoriteResult = result;
    if (result == FAVORITE_SET) {
        work->data.favoriteMedal = medal;
        func_0202d194(medal);
    } else if (result == FAVORITE_REMOVED) {
        work->data.favoriteMedal = 0xff;
        func_0202d194(0xff);
    }
}

// Shows the medals in their new order from the first column, behind the cover
static BOOL MedalBoxApp_SeqSort(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_MD_04);
        MedalBoxApp_ClearNames(work);
        func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
        work->subSeq++;
        // fallthrough
    case 1: {
        ClActorPos pos;
        BOOL backDone = TRUE;
        BOOL coverDone = FALSE;

        work->coverX += FX32_CONST(4);
        pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
        pos.y = 0x60;
        if (pos.x > 0x80) {
            work->coverX = 0;
            pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
            pos.y = 0x60;
            coverDone = TRUE;
        }
        func_0204c140(work->actors[ACTOR_COVER], &pos, 0);
        if (work->view == VIEW_COLUMNS) {
            backDone = FALSE;
            work->backX += 0x8ccc;
            if ((work->backX >> FX32_SHIFT) > 0) {
                work->backX = 0;
                backDone = TRUE;
            }
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_BACK), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_TEXT), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
        }
        if (coverDone && backDone) {
            work->subSeq++;
        }
        break;
    }
    case 2:
        work->column = 0;
        MedalBoxApp_DrawColumns(work, FALSE, TRUE);
        MedalBoxApp_ClearNames(work);
        func_0204c124(work->actors[ACTOR_FAVORITE], FALSE);
        work->wait = 0;
        work->subSeq++;
        break;
    case 3:
        work->wait++;
        if (work->wait > 6) {
            work->subSeq++;
        }
        break;
    case 4: {
        ClActorPos pos;
        BOOL coverDone = FALSE;
        BOOL backDone = TRUE;

        work->coverX -= FX32_CONST(4);
        pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
        pos.y = 0x60;
        if (pos.x < -0x50) {
            coverDone = TRUE;
            work->coverX = FX32_CONST(-0x50);
            pos.x = (work->coverX >> FX32_SHIFT) + 0x80;
            pos.y = 0x60;
        }
        func_0204c140(work->actors[ACTOR_COVER], &pos, 0);
        if (work->view == VIEW_COLUMNS) {
            backDone = FALSE;
            work->backX -= 0x8ccc;
            if ((work->backX >> FX32_SHIFT) < -0xb8) {
                work->backX = FX32_CONST(-0xb8);
                backDone = TRUE;
            }
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_BACK), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
            GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_TEXT), BG_MOVE_SET_X, work->backX >> FX32_SHIFT);
        }
        if (coverDone && backDone) {
            if (!func_0203d554()) {
                MedalBoxApp_SetCursor(work, 0, TRUE);
            } else {
                work->prevCursor = 0;
            }
            MedalBoxApp_DrawColumns(work, TRUE, TRUE);
            MedalBoxApp_SetSeq(work, SEQ_MAIN);
        }
        break;
    }
    }
    return FALSE;
}

// The message of the manual, the first time the box is opened
static BOOL MedalBoxApp_SeqManual(MedalBoxApp *work) {
    switch (work->subSeq) {
    case 0:
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), FALSE);
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 0);
        GFL_BGSysMoveBGReq(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), BG_MOVE_SET_X, 0);
        work->subSeq++;
        break;
    case 1: {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData[MSG_UI], Title10_Text_ManualWedgedInsideMedal);

        MedalBoxApp_ShowMessage(work, str);
        GFL_StrBufFree(str);
        MedalBoxApp_DrawColumns(work, FALSE, TRUE);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), TRUE);
        work->subSeq++;
        break;
    }
    case 2:
        BlinkPalAnm_Main(work->blinkAnm);
        if (work->stream == NULL) {
            GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), FALSE);
            work->subSeq++;
        }
        break;
    case 3:
        MedalBoxApp_CloseMessage(work);
        MedalBoxApp_ReloadMenuBG(work);
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        GFL_BGSysSetBGPriority(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 2);
        GFL_BGSysSetBGEnabled(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), TRUE);
        func_0200fb34(work->medalBox);
        MedalBoxApp_RestoreCursor(work);
        MedalBoxApp_SetSeq(work, SEQ_MAIN);
        work->subSeq++;
        break;
    }
    return FALSE;
}

static void MedalBoxApp_ShowMessage(MedalBoxApp *work, StrBuf *str) {
    work->messageWindow = BmpWin_CreateDynamic(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 2, 18, 28, 4, 11, 1);
    BmpWin_FlushMap(work->messageWindow);
    LoadSysMsgBox(MedalBoxApp_GetBG(BG_MAIN_MESSAGE), 1, 12, 0, work->heapId);
    BmpWin_DrawFrame(work->messageWindow, 1, 1, 12);
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    BmpWin_FlushChar(work->messageWindow);
    work->stream = func_02022268(work->messageWindow, 0, 0, str, work->font, func_02017bcc(), work->tcbManager, 0,
                                 work->heapId, 15);
    BmpWin_FlushChar(work->messageWindow);
    GFL_BGSysQueueScrLoad(MedalBoxApp_GetBG(BG_MAIN_MESSAGE));
}

static void MedalBoxApp_CloseMessage(MedalBoxApp *work) {
    BmpWin_Free(work->messageWindow);
}
