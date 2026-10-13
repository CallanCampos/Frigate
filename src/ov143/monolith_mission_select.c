// The Entralink monolith's list of Funfest missions on the bottom screen, laid out like the pass power list: a list
// scrolled with the stylus or the keys, from which a mission is picked at one of its levels, and then joined. A
// secret sequence of keys unlocks the highest level of every mission. The ROM gives no name for this file;
// monolith_mission_select.c is a guess from what it does, and its asserts call its work `mfw`

#include "types.h"
#include "app/monolith/monolith_mission_select.h"
#include "app/monolith/monolith_tool.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/event_work.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/app_menu_common.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The BGs of the message window, of the frame and of the list
#define MONOLITH_MISSION_BG_TEXT 4
#define MONOLITH_MISSION_BG_FRAME 5
#define MONOLITH_MISSION_BG_LIST 6

// The list holds every mission, with an empty row above and below
#define MONOLITH_MISSION_LIST_MAX (FEST_MISSION_COUNT + 2)

// The rows of the list on the screen, each 32 pixels tall
#define MONOLITH_MISSION_ROW_COUNT 7
#define MONOLITH_MISSION_ROW_HEIGHT 32

// No row
#define MONOLITH_MISSION_NONE 0xffff

// The color of the missions' names, and the heap that the format of a mission's text uses
#define MONOLITH_MISSION_TEXT_COLOR 0x3c40
#define MONOLITH_MISSION_TEXT_HEAP HEAPID_TAIL(HEAPID_MONOLITH)

// How the cursor was last moved: by dragging the list, by the keys or by a tap
enum {
    MONOLITH_MISSION_CURSOR_DRAG,
    MONOLITH_MISSION_CURSOR_KEYS,
    MONOLITH_MISSION_CURSOR_TAP,
};

// What the player's input changes in the level of the cursor's mission
enum {
    MONOLITH_MISSION_LEVEL_CLAMP,
    MONOLITH_MISSION_LEVEL_TOUCH,
    MONOLITH_MISSION_LEVEL_KEYS,
};

// The highest level, 0-based, which the cheat code unlocks, and the level it draws the missions at
#define MONOLITH_MISSION_LEVEL_CHEAT 3

// The missions' levels
#define MONOLITH_MISSION_LEVEL_COUNT 3

// The flags of the work: the cheat code was entered, no row has the cursor, and the player's first visit is not done
#define MONOLITH_MISSION_FLAG_CHEAT 1
#define MONOLITH_MISSION_FLAG_NO_CURSOR 2
#define MONOLITH_MISSION_FLAG_FIRST_VISIT 4

// The flags of a mission's record: it is new, and the player has seen it
#define MONOLITH_MISSION_RECORD_NEW 1
#define MONOLITH_MISSION_RECORD_SEEN 2

// The value of a mission's header.unk01 that makes it available in this version alone: 2 in Black 2, 1 in White 2.
// 0 is available in both, and 4 to 7 need a download
#ifdef BLACK2
#define MONOLITH_MISSION_UNLOCK_VERSION 2
#else
#define MONOLITH_MISSION_UNLOCK_VERSION 1
#endif

// The length of the cheat code in keys
#define MONOLITH_MISSION_CODE_LENGTH 9

// The icons, by index: the buttons of the three levels, then a marker of a new mission and one of the highest level
// cleared for each row of the list
enum {
    MONOLITH_MISSION_ICON_LEVEL,
    MONOLITH_MISSION_ICON_NEW = MONOLITH_MISSION_ICON_LEVEL + MONOLITH_MISSION_LEVEL_COUNT,
    MONOLITH_MISSION_ICON_CLEARED = MONOLITH_MISSION_ICON_NEW + MONOLITH_MISSION_ROW_COUNT,
    MONOLITH_MISSION_ICON_COUNT = MONOLITH_MISSION_ICON_CLEARED + MONOLITH_MISSION_ROW_COUNT,
};

enum {
    MONOLITH_MISSION_SEQ_START,
    MONOLITH_MISSION_SEQ_SHOW_HELP,
    MONOLITH_MISSION_SEQ_WAIT_HELP,
    MONOLITH_MISSION_SEQ_WAIT_RELEASE,
    MONOLITH_MISSION_SEQ_INPUT,
    MONOLITH_MISSION_SEQ_ASK,
    MONOLITH_MISSION_SEQ_WAIT_ASK,
    MONOLITH_MISSION_SEQ_YES_NO,
    MONOLITH_MISSION_SEQ_JOIN,
    MONOLITH_MISSION_SEQ_WAIT_JOINED,
    MONOLITH_MISSION_SEQ_WAIT_TIME,
    MONOLITH_MISSION_SEQ_WAIT_TOUCH_RELEASE,
    MONOLITH_MISSION_SEQ_RETURN,
    MONOLITH_MISSION_SEQ_WAIT_DISCONNECT,
    MONOLITH_MISSION_SEQ_BOOT_COMM,
    MONOLITH_MISSION_SEQ_END,
};

// What the save knows of a mission: the highest level cleared, and whether it is new
typedef struct {
    u8 clearedLevel;
    u8 flags;
} MonolithMissionRecord;

typedef struct {
    TCB *vblankTask;
    GameData *gameData;
    LinkFestival *festival;
    FestivalText *festText;
    // All the missions' data
    FestMissionData *missions;
    GameCommSys *comm;
    u32 barChar;
    // The frames that the message about the mission joined shows for
    u32 timer;
    // The START button
    MonolithTextActor startButton;
    ClActor *arrowUp;
    ClActor *arrowDown;
    ClActor *icons[MONOLITH_MISSION_ICON_COUNT];
    Font *font;
    // Each row of the list drawn into a bitmap, its mission and its record
    GFLBitmap *listBitmaps[MONOLITH_MISSION_LIST_MAX];
    s16 ids[MONOLITH_MISSION_LIST_MAX];
    MonolithMissionRecord records[MONOLITH_MISSION_LIST_MAX];
    u8 count;
    u8 drawnCount;
    // The message window and the title's
    BmpWin *windows[2];
    PrintWindow titlePrint;
    StrBuf *msgStr;
    PrintStream *print_stream;
    MonolithReturnButton returnButton;
    // The actors that show the rows on the screen
    BmpOamActor *rowActors[MONOLITH_MISSION_ROW_COUNT];
    GFLBitmap *rowBitmaps[MONOLITH_MISSION_ROW_COUNT];
    BmpOamActor *levelActors[MONOLITH_MISSION_LEVEL_COUNT];
    GFLBitmap *levelBitmaps[MONOLITH_MISSION_LEVEL_COUNT];
    // The list's scroll in 1/256 pixels, and how it moves
    s32 scroll;
    u32 dragFrames;
    s32 velocity;
    s32 keyScroll;
    u32 lastY;
    // Where the stylus touched the list, or MONOLITH_MISSION_NONE once it moved
    u32 tapY;
    BOOL rowsChanged;
    // How many keys of the cheat code were typed
    int codeProgress;
    // The level of the cursor's mission that is picked
    int level;
    u32 flags;
    u32 unk2E4;
    int cursor;
    int prevCursor;
    int prevCursor2;
    // The row picked to join, or MONOLITH_MISSION_NONE
    int selected;
    BOOL cursorChanged;
    // Set once the mission is joined
    BOOL joined;
    u32 cursorBy;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *app_menu_work;
} MonolithMissionWork;

// What a level button or a marker of the icons is made from
typedef struct {
    s16 x;
    s16 y;
    s16 sequence;
    s16 priority;
    s16 bgPriority;
    s16 palette;
} MonolithMissionIcon;

// The values of a level button's text in sMonolithMissionLevelButtons, which is a flat table of numbers: the code
// indexes it with i * 4 + n
enum {
    LEVEL_BUTTON_X,
    LEVEL_BUTTON_Y,
    LEVEL_BUTTON_PRIORITY,
    LEVEL_BUTTON_BG_PRIORITY,
    LEVEL_BUTTON_VALUES,
};

// By the highest level cleared: the highest level that can be picked, and how many level buttons are shown
typedef struct {
    s16 maxLevel;
    s16 buttonCount;
} MonolithMissionLevelRange;

static BOOL MonolithMission_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithMission_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithMission_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithMission_LoadFestival(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_FreeFestival(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_CreateBGs(void);
static void MonolithMission_ReleaseBGs(void);
static void MonolithMission_LoadGraphics(MonolithMissionWork *mfw, MonolithWork *wk);
static void MonolithMission_FreeBarChar(MonolithMissionWork *mfw);
static void MonolithMission_CreateWindows(MonolithMissionWork *mfw, MonolithWork *wk);
static void MonolithMission_FreeWindows(MonolithMissionWork *mfw);
static void MonolithMission_DrawTitle(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_ShowMessage(MonolithMissionWork *mfw, MonolithWork *wk, u32 msgId);
static void MonolithMission_ShowMessageExpanded(MonolithMissionWork *mfw, MonolithWork *wk, u32 msgId);
static BOOL MonolithMission_UpdateMessage(MonolithWork *wk, MonolithMissionWork *mfw);
static void MonolithMission_ClearMessage(MonolithMissionWork *mfw);
static void MonolithMission_ClearUnusedRows(MonolithMissionWork *mfw);
static void MonolithMission_CreateList(MonolithScreenParam *screen, MonolithMissionWork *mfw, MonolithWork *wk);
static void MonolithMission_FreeList(MonolithMissionWork *mfw);
static BOOL MonolithMission_DrawList(MonolithMissionWork *mfw, MonolithWork *wk, int count);
static void MonolithMission_RedrawRow(MonolithMissionWork *mfw, MonolithWork *wk, int level);
static void MonolithMission_CreateRowActors(MonolithMissionWork *mfw, MonolithWork *wk);
static void MonolithMission_DeleteRowActors(MonolithMissionWork *mfw);
static void MonolithMission_CreateStartButton(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_DeleteStartButton(MonolithMissionWork *mfw);
static void MonolithMission_UpdateStartButton(MonolithWork *wk, MonolithMissionWork *mfw);
static void MonolithMission_CreateReturnButton(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_DeleteReturnButton(MonolithMissionWork *mfw);
static void MonolithMission_UpdateReturnButton(MonolithMissionWork *mfw);
static void MonolithMission_CreateArrows(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_DeleteArrows(MonolithMissionWork *mfw);
static void MonolithMission_UpdateArrows(MonolithMissionWork *mfw);
static BOOL MonolithMission_Input(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static BOOL MonolithMission_SetCursor(MonolithMissionWork *mfw, int index, u32 by);
static void MonolithMission_SetRowPalette(MonolithMissionWork *mfw, int index, int palette);
static void MonolithMission_HighlightRow(MonolithMissionWork *mfw, int index);
static void MonolithMission_UnhighlightRow(MonolithMissionWork *mfw, int index);
static void MonolithMission_UnhighlightPrevRow(MonolithMissionWork *mfw);
static void MonolithMission_UpdateCursor(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static BOOL MonolithMission_Scroll(MonolithMissionWork *mfw, s32 delta);
static void MonolithMission_MoveBG(MonolithMissionWork *mfw);
static void MonolithMission_UpdateRows(MonolithMissionWork *mfw);
static void MonolithMission_VBlank(TCB *tcb, void *data);
static int MonolithMission_GetRowAt(MonolithMissionWork *mfw, u32 y);
static void MonolithMission_InitState(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_UpdateFocus(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_CreateIcons(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_DeleteIcons(MonolithMissionWork *mfw);
static void MonolithMission_SetIconY(MonolithMissionWork *mfw, int index, int y);
static void MonolithMission_SetIconVisible(MonolithMissionWork *mfw, int index, BOOL visible);
static void MonolithMission_SetIconSequence(MonolithMissionWork *mfw, int index, int sequence);
static void MonolithMission_UpdateNewIcons(MonolithMissionWork *mfw);
static void MonolithMission_UpdateClearedIcons(MonolithMissionWork *mfw);
static void MonolithMission_CreateLevelButtons(MonolithWork *wk, MonolithMissionWork *mfw);
static void MonolithMission_DeleteLevelButtons(MonolithMissionWork *mfw);
static void MonolithMission_DrawLevelButton(MonolithWork *wk, MonolithMissionWork *mfw, int index);
static void MonolithMission_SetLevelButtonVisible(MonolithMissionWork *mfw, int index, BOOL visible);
static void MonolithMission_UpdateLevelButtons(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static BOOL MonolithMission_IsAvailable(MonolithScreenParam *screen, MonolithMissionWork *mfw, int index);
static void MonolithMission_LoadRecord(MonolithScreenParam *screen, MonolithMissionWork *mfw, int index, int id);
static void MonolithMission_ClearNew(MonolithScreenParam *screen, MonolithMissionWork *mfw, int id);
static void MonolithMission_MoveLevelCursor(MonolithScreenParam *screen, MonolithMissionWork *mfw, int by);
static void MonolithMission_PaintCheatRows(MonolithMissionWork *mfw);
static void MonolithMission_UpdateCheatCode(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static void MonolithMission_InitIcons(MonolithMissionWork *mfw, BOOL skip);
static void MonolithMission_ShowStartMessage(MonolithScreenParam *screen, MonolithMissionWork *mfw);
static u32 MonolithMission_GetHeldKeys(MonolithMissionWork *mfw);
static u32 MonolithMission_GetTypedKeys(MonolithMissionWork *mfw);
static void MonolithMission_Dummy(MonolithScreenParam *screen, MonolithMissionWork *mfw);

// Nothing reads these
const u16 MONOLITH_MISSION_UNUSED_FLAGS[2] = { MONOLITH_FLAG_VISITED, MONOLITH_FLAG_VISITED };

static const TouchRect sMonolithMissionTouchRects[] = {
    { 172, 188, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The touch rectangles of the three levels' buttons
static const TouchRect sMonolithMissionLevelRects[] = {
    { 172, 188, 128, 156 },
    { 172, 188, 164, 192 },
    // BUG: The right edge is left of the left edge, so that the third button can't be tapped
#ifdef BUGFIX
    { 172, 188, 200, 228 },
#else
    { 172, 188, 200, 128 },
#endif
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The cheat code: SELECT, DOWN three times, UP four times and SELECT
static const u16 sMonolithMissionCode[MONOLITH_MISSION_CODE_LENGTH] = {
    PAD_BUTTON_SELECT, PAD_KEY_DOWN, PAD_KEY_DOWN, PAD_KEY_DOWN, PAD_KEY_UP,
    PAD_KEY_UP,        PAD_KEY_UP,   PAD_KEY_UP,   PAD_BUTTON_SELECT,
};

static const MonolithMissionLevelRange sMonolithMissionLevelRanges[5] = {
    { 0, 1 }, { 1, 2 }, { 2, 3 }, { 2, 3 }, { 2, 3 },
};

static const s16 sMonolithMissionLevelButtons[] = {
    130, 177, 48, 1,
    166, 177, 48, 1,
    202, 177, 48, 1,
    -1, -1, -1, -1,
};

const GameProcFunctions MONOLITH_MISSION_SELECT_PROC_FUNCTIONS = {
    MonolithMission_Init,
    MonolithMission_Main,
    MonolithMission_Exit,
};

// The setups of MONOLITH_MISSION_BG_TEXT, _FRAME and _LIST
static const BGSetup sMonolithMissionBGSetups[3] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
};

// The buttons of the levels, then the markers of the rows
static const MonolithMissionIcon sMonolithMissionIcons[MONOLITH_MISSION_ICON_COUNT + 1] = {
    { 140, 181, 16, 64, 1, 4 },
    { 176, 181, 16, 64, 1, 0 },
    { 212, 181, 16, 64, 1, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 16, 32, 15, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { 230, 32, 12, 64, 3, 0 },
    { -1, -1, -1, -1, -1, -1 },
};

static BOOL MonolithMission_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionWork *mfw = work;
    BOOL printing;

    switch (*state) {
    case 0:
        mfw = GFL_ProcInitSubsystem(proc, sizeof(MonolithMissionWork), HEAPID_MONOLITH);
        sys_memset(mfw, 0, sizeof(MonolithMissionWork));
        mfw->scroll = 8 << 8;
        mfw->selected = MONOLITH_MISSION_NONE;
        mfw->prevCursor = MONOLITH_MISSION_NONE;
        mfw->prevCursor2 = MONOLITH_MISSION_NONE;
        MonolithTool_InitPanels(screen);
        MonolithMission_LoadFestival(screen, mfw);
        MonolithMission_InitState(screen, mfw);
        MonolithMission_CreateBGs();
        MonolithMission_LoadGraphics(mfw, screen->work);
        MonolithMission_CreateWindows(mfw, screen->work);
        MonolithMission_MoveBG(mfw);
        MonolithMission_CreateReturnButton(screen, mfw);
        MonolithMission_CreateRowActors(mfw, screen->work);
        MonolithMission_CreateStartButton(screen, mfw);
        MonolithMission_CreateArrows(screen, mfw);
        MonolithMission_CreateIcons(screen, mfw);
        MonolithMission_CreateLevelButtons(screen->work, mfw);
        MonolithMission_DrawTitle(screen, mfw);
        mfw->menuRes = MonolithTool_CreateMenuRes(screen->work, MONOLITH_MISSION_BG_TEXT, HEAPID_MONOLITH);
        screen->state->focusPower = HIGH_LINK_POWER_NONE;
        mfw->vblankTask = GFL_VBlankTCBAdd(MonolithMission_VBlank, mfw, 3);
        MonolithMission_CreateList(screen, mfw, screen->work);
        MonolithMission_ClearUnusedRows(mfw);
        MonolithMission_InitIcons(mfw, FALSE);
        MonolithTool_SetTextVisible(&mfw->startButton, FALSE);
        (*state)++;
        break;
    case 1:
        if (MonolithMission_DrawList(mfw, screen->work, mfw->count) == TRUE) {
            MonolithMission_UpdateRows(mfw);
            mfw->rowsChanged = TRUE;
            if (func_0203d554() == FALSE) {
                MonolithMission_SetCursor(mfw, 1, MONOLITH_MISSION_CURSOR_DRAG);
            } else {
                MonolithMission_SetCursor(mfw, MONOLITH_MISSION_NONE, MONOLITH_MISSION_CURSOR_DRAG);
            }
            (*state)++;
        }
        break;
    case 2:
        printing = FALSE;
        PrintWindow_Flush(&mfw->titlePrint, screen->work->printQueue);
        if (PrintWindow_IsPrinted(&mfw->titlePrint) == FALSE) {
            printing = TRUE;
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_FRAME, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_LIST, TRUE);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithMission_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionWork *mfw = work;
    u32 pressed;
    int hit;
    BOOL yes;
    FestMission mission;
    u16 picked;
    s16 level;
    u8 bootState;

    GSYS_GetGameCommSystem(screen->param->gsys);
    MonolithTool_UpdatePanel(screen, 1);
    MonolithTool_UpdatePanel(screen, 3);
    MonolithMission_UpdateReturnButton(mfw);
    MonolithMission_UpdateStartButton(screen->work, mfw);
    PrintWindow_Flush(&mfw->titlePrint, screen->work->printQueue);
    if (screen->exiting == TRUE && *state == MONOLITH_MISSION_SEQ_INPUT) {
        return TRUE;
    }
    switch (*state) {
    case MONOLITH_MISSION_SEQ_START:
        *state = MONOLITH_MISSION_SEQ_SHOW_HELP;
        break;
    case MONOLITH_MISSION_SEQ_SHOW_HELP:
        MonolithMission_ShowMessage(mfw, screen->work, 45);
        (*state)++;
        break;
    case MONOLITH_MISSION_SEQ_WAIT_HELP:
        if (MonolithMission_UpdateMessage(screen->work, mfw) == TRUE &&
            (func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            MonolithMission_ClearMessage(mfw);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            if (func_0203da48() == FALSE) {
                MonolithMission_SetCursor(mfw, 1, MONOLITH_MISSION_CURSOR_DRAG);
            } else {
                MonolithMission_SetCursor(mfw, MONOLITH_MISSION_NONE, MONOLITH_MISSION_CURSOR_DRAG);
            }
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_RELEASE:
        if (!func_0203da2c() && !(MonolithMission_GetHeldKeys(mfw) & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            *state = MONOLITH_MISSION_SEQ_INPUT;
        }
        break;
    case MONOLITH_MISSION_SEQ_INPUT:
        pressed = GCTX_HIDGetPressedKeys();
        hit = func_0203da0c(sMonolithMissionTouchRects);
        if (hit == 0 || (pressed & PAD_BUTTON_B)) {
            MonolithTool_PressReturnButton(&mfw->returnButton);
            if (hit == 0) {
                func_0203d564(TRUE);
            } else {
                func_0203d564(FALSE);
            }
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *state = MONOLITH_MISSION_SEQ_RETURN;
            break;
        }
        MonolithMission_Input(screen, mfw);
        MonolithMission_UnhighlightPrevRow(mfw);
        if (MonolithMission_Scroll(mfw, mfw->velocity + mfw->keyScroll) == TRUE) {
            MonolithMission_MoveBG(mfw);
            MonolithMission_UpdateRows(mfw);
            mfw->rowsChanged = TRUE;
            MonolithMission_UpdateNewIcons(mfw);
            MonolithMission_UpdateClearedIcons(mfw);
        }
        MonolithMission_UpdateCursor(screen, mfw);
        if (mfw->cursor == MONOLITH_MISSION_NONE) {
            screen->state->focusPower = HIGH_LINK_POWER_NONE;
            MonolithTool_SetTextVisible(&mfw->startButton, FALSE);
            mfw->flags |= MONOLITH_MISSION_FLAG_NO_CURSOR;
        } else {
            screen->state->focusPower = mfw->ids[mfw->cursor];
            MonolithTool_SetTextVisible(&mfw->startButton, TRUE);
            mfw->flags &= ~MONOLITH_MISSION_FLAG_NO_CURSOR;
        }
        MonolithMission_UpdateLevelButtons(screen, mfw);
        MonolithMission_UpdateFocus(screen, mfw);
        MonolithMission_UpdateCheatCode(screen, mfw);
        MonolithMission_Dummy(screen, mfw);
        if (mfw->selected != MONOLITH_MISSION_NONE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            *state = MONOLITH_MISSION_SEQ_ASK;
        }
        break;
    case MONOLITH_MISSION_SEQ_ASK:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            MonolithMission_ShowStartMessage(screen, mfw);
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_ASK:
        if (MonolithMission_UpdateMessage(screen->work, mfw) == TRUE) {
            GFL_ASSERT(mfw->app_menu_work == NULL);
            mfw->app_menu_work = MonolithTool_CreateYesNoMenu(screen->work, mfw->menuRes, HEAPID_MONOLITH);
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_YES_NO:
        if (MonolithTool_UpdateYesNoMenu(screen->work, 4, mfw->app_menu_work, &yes) == TRUE) {
            MonolithTool_FreeYesNoMenu(mfw->app_menu_work);
            mfw->app_menu_work = NULL;
            MonolithMission_ClearMessage(mfw);
            if (yes == TRUE) {
                *state = MONOLITH_MISSION_SEQ_JOIN;
            } else {
                mfw->selected = MONOLITH_MISSION_NONE;
                MonolithTool_SetPanelPulse(screen, TRUE, 1);
                MonolithTool_SetTextCursor(screen, &mfw->startButton, 1, 0xff, 3);
                *state = MONOLITH_MISSION_SEQ_INPUT;
            }
        }
        break;
    case MONOLITH_MISSION_SEQ_JOIN:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            MonolithMission_ShowMessageExpanded(mfw, screen->work, 48);
            mfw->joined = TRUE;
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_JOINED:
        if (MonolithMission_UpdateMessage(screen->work, mfw) == TRUE) {
            mfw->timer = 60;
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_TIME:
        if (mfw->timer != 0) {
            mfw->timer--;
        }
        if (mfw->timer == 0 || func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            MonolithMission_ClearMessage(mfw);
            func_0202bf7c(GSYS_GetGameCommSystem(screen->param->gsys));
            MonolithTool_SetPanelPulse(screen, TRUE, 1);
            MonolithTool_SetTextCursor(screen, &mfw->startButton, 1, 0xff, 3);
            if (func_0203da48()) {
                *state = MONOLITH_MISSION_SEQ_WAIT_TOUCH_RELEASE;
            } else {
                *state = MONOLITH_MISSION_SEQ_RETURN;
            }
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_TOUCH_RELEASE:
        if (!func_0203da2c()) {
            *state = MONOLITH_MISSION_SEQ_RETURN;
        }
        break;
    case MONOLITH_MISSION_SEQ_RETURN:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH &&
            !MonolithTool_IsReturnButtonBlinking(&mfw->returnButton)) {
            picked = mfw->ids[mfw->selected];
            mfw->selected = MONOLITH_MISSION_NONE;
            if (mfw->joined) {
                bootState = GameCommSys_BootCheck(mfw->comm);
                level = mfw->level;
                if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
                    level = MONOLITH_MISSION_LEVEL_CHEAT;
                }
                func_ov027_02170cf8(mfw->festText, picked, level, &mission);
                func_0201472c(mfw->festival, &mission, 0);
                if (bootState == 1) {
                    *state = MONOLITH_MISSION_SEQ_END;
                } else if (bootState == 0) {
                    *state = MONOLITH_MISSION_SEQ_BOOT_COMM;
                } else {
                    GameCommSys_ExitReq(mfw->comm);
                    *state = MONOLITH_MISSION_SEQ_WAIT_DISCONNECT;
                }
            } else {
                *state = MONOLITH_MISSION_SEQ_END;
            }
        }
        break;
    case MONOLITH_MISSION_SEQ_WAIT_DISCONNECT:
        if (GameCommSys_BootCheck(mfw->comm) == 0) {
            if (GFL_NetErrCheck() == FALSE) {
                func_0202be00(mfw->comm);
            }
            (*state)++;
        }
        break;
    case MONOLITH_MISSION_SEQ_BOOT_COMM:
        GSYS_TryBootGameComm(screen->param->gsys);
        (*state)++;
        break;
    case MONOLITH_MISSION_SEQ_END:
        if (mfw->joined) {
            screen->next = MONOLITH_SCREEN_EXIT;
        } else {
            screen->next = MONOLITH_SCREEN_MENU;
            screen->state->menuCursor = MONOLITH_MENU_MISSION;
        }
        return TRUE;
    }
    MonolithMission_UpdateArrows(mfw);
    return FALSE;
}

static BOOL MonolithMission_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionWork *mfw = work;

    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG3);
    GFL_TCBRemove(mfw->vblankTask);
    if (mfw->print_stream != NULL) {
        func_020223cc(mfw->print_stream);
    }
    if (mfw->msgStr != NULL) {
        GFL_StrBufFree(mfw->msgStr);
    }
    MonolithMission_FreeList(mfw);
    if (mfw->app_menu_work != NULL) {
        MonolithTool_FreeYesNoMenu(mfw->app_menu_work);
    }
    MonolithTool_FreeMenuRes(mfw->menuRes);
    MonolithMission_DeleteStartButton(mfw);
    MonolithMission_DeleteRowActors(mfw);
    MonolithMission_DeleteReturnButton(mfw);
    MonolithMission_DeleteArrows(mfw);
    MonolithMission_DeleteIcons(mfw);
    MonolithMission_DeleteLevelButtons(mfw);
    MonolithMission_FreeBarChar(mfw);
    MonolithMission_FreeWindows(mfw);
    MonolithMission_ReleaseBGs();
    GFL_FontFree(mfw->font);
    GFL_HeapFree(mfw->missions);
    MonolithMission_FreeFestival(screen, mfw);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithMission_LoadFestival(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    mfw->gameData = GSYS_GetGameData(screen->param->gsys);
    mfw->comm = GSYS_GetGameCommSystem(screen->param->gsys);
    mfw->festival = GSYS_GetLinkFestival(screen->param->gsys);
    mfw->festText = getTextFileForFestMissions(HEAPID_MONOLITH);
}

static void MonolithMission_FreeFestival(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    func_ov027_02170b00(mfw->festText);
}

static void MonolithMission_CreateBGs(void) {
    GFL_BGSysCreateBG(MONOLITH_MISSION_BG_TEXT, &sMonolithMissionBGSetups[0], 0);
    GFL_BGSysCreateBG(MONOLITH_MISSION_BG_FRAME, &sMonolithMissionBGSetups[1], 0);
    GFL_BGSysCreateBG(MONOLITH_MISSION_BG_LIST, &sMonolithMissionBGSetups[2], 0);
    GFL_BGSysFillScrArea(MONOLITH_MISSION_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_MISSION_BG_FRAME, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_MISSION_BG_LIST, 0, 0, 0, 32, 32, 17);
}

static void MonolithMission_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_FRAME, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_MISSION_BG_LIST, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_MISSION_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_MISSION_BG_FRAME);
    GFL_BGSysReleaseBG(MONOLITH_MISSION_BG_LIST);
}

static void MonolithMission_LoadGraphics(MonolithMissionWork *mfw, MonolithWork *wk) {
    ArcTool *menuArc;

    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 7, MONOLITH_MISSION_BG_LIST, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadArcNCGRStatic(wk->arc, 16, MONOLITH_MISSION_BG_FRAME, 0, 0, TRUE, HEAPID_MONOLITH);
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 9, MONOLITH_MISSION_BG_FRAME, 0, 0, TRUE, HEAPID_MONOLITH);
    menuArc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_MONOLITH);
    mfw->barChar = GFL_BGSysAllocChar(MONOLITH_MISSION_BG_FRAME, 0x80, TRUE);
    GFL_BGSysLoadArcNCGRStatic(menuArc, 28, MONOLITH_MISSION_BG_FRAME, mfw->barChar, 0x80, FALSE, HEAPID_MONOLITH);
    AppMenuCommon_LoadBarScreen(menuArc, MONOLITH_MISSION_BG_FRAME, HEAPID_MONOLITH, mfw->barChar, 12);
    GFL_ArcToolFree(menuArc);
    GFL_BGSysLoadScr(MONOLITH_MISSION_BG_FRAME);
    GFL_BGSysLoadScr(MONOLITH_MISSION_BG_LIST);
}

static void MonolithMission_FreeBarChar(MonolithMissionWork *mfw) {
    GFL_BGSysFreeCharMemory(MONOLITH_MISSION_BG_FRAME, mfw->barChar, 0x80);
}

static void MonolithMission_CreateWindows(MonolithMissionWork *mfw, MonolithWork *wk) {
    LoadSysMsgBoxBGChar(MONOLITH_MISSION_BG_TEXT, 1, 0, HEAPID_MONOLITH);
    PaletteFade_LoadNCLREx(wk->fade, ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), HEAPID_MONOLITH,
                           PALFADE_BUFFER_SUB_BG, 0x20, 11 * 0x10, 0);
    mfw->windows[0] = BmpWin_CreateDynamic(MONOLITH_MISSION_BG_TEXT, 1, 19, 30, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(mfw->windows[0]), 0xff);
    mfw->windows[1] = BmpWin_CreateDynamic(MONOLITH_MISSION_BG_TEXT, 0, 0, 32, 3, 13, TRUE);
    BmpWin_FlushMap(mfw->windows[1]);
    GFL_BGSysLoadScr(MONOLITH_MISSION_BG_TEXT);
    PrintWindow_Init(&mfw->titlePrint, mfw->windows[1]);
}

static void MonolithMission_FreeWindows(MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < 2; i++) {
        BmpWin_Free(mfw->windows[i]);
    }
}

static void MonolithMission_DrawTitle(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, 44);
    MonolithWork *wk = screen->work;

    PrintWindow_PrintNoColor(&mfw->titlePrint, wk->printQueue, 24, 4, str, wk->font);
    GFL_StrBufFree(str);
}

static void MonolithMission_ShowMessage(MonolithMissionWork *mfw, MonolithWork *wk, u32 msgId) {
    GFL_ASSERT(mfw->print_stream == NULL);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    mfw->msgStr = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    mfw->print_stream = func_02022268(mfw->windows[0], 0, 0, mfw->msgStr, wk->font, func_02017bcc(), wk->tcbMgr, 10,
                                      HEAPID_MONOLITH, 15);
    BmpWin_FlushMap(mfw->windows[0]);
    BmpWin_DrawFrame(mfw->windows[0], TRUE, 1, 11);
}

static void MonolithMission_ShowMessageExpanded(MonolithMissionWork *mfw, MonolithWork *wk, u32 msgId) {
    StrBuf *str;

    GFL_ASSERT(mfw->print_stream == NULL);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    mfw->msgStr = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    GFL_WordSetFormatStrbuf(wk->wordSet, mfw->msgStr, str);
    GFL_StrBufFree(str);
    mfw->print_stream = func_02022268(mfw->windows[0], 0, 0, mfw->msgStr, wk->font, func_02017bcc(), wk->tcbMgr, 10,
                                      HEAPID_MONOLITH, 15);
    BmpWin_FlushMap(mfw->windows[0]);
    BmpWin_DrawFrame(mfw->windows[0], TRUE, 1, 11);
}

static BOOL MonolithMission_UpdateMessage(MonolithWork *wk, MonolithMissionWork *mfw) {
    return AppPrintsysCommon_Update(&wk->printsys, mfw->print_stream);
}

static void MonolithMission_ClearMessage(MonolithMissionWork *mfw) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(mfw->windows[0]);

    GFL_ASSERT(mfw->print_stream != NULL);
    func_020223cc(mfw->print_stream);
    mfw->print_stream = NULL;
    GFL_StrBufFree(mfw->msgStr);
    mfw->msgStr = NULL;
    BmpWin_ClearFrame(mfw->windows[0], FALSE);
    GFL_BitmapFill(bitmap, 0xff);
    BmpWin_FlushChar(mfw->windows[0]);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
}

// Clears the list's background below its last row, when the list is shorter than the screen
static void MonolithMission_ClearUnusedRows(MonolithMissionWork *mfw) {
    int y;

    if (mfw->count - 2 < MONOLITH_MISSION_ROW_COUNT) {
        y = (mfw->count - 1) * MONOLITH_MISSION_ROW_HEIGHT / 8;
        GFL_BGSysFillScrArea(MONOLITH_MISSION_BG_LIST, 0, 0, y, 32, 32 - y, 17);
        GFL_BGSysLoadScr(MONOLITH_MISSION_BG_LIST);
    }
}

// Lists the missions that can be joined, with an empty row at each end
static void MonolithMission_CreateList(MonolithScreenParam *screen, MonolithMissionWork *mfw, MonolithWork *wk) {
    int i;
    int count = 0;

    GSYS_GetGameData(screen->param->gsys);
    MonolithTool_GetLevels(screen);
    mfw->ids[count] = 0;
    mfw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
    count++;
    for (i = 0; i < FEST_MISSION_COUNT; i++) {
        if (MonolithMission_IsAvailable(screen, mfw, i) == TRUE) {
            MonolithMission_LoadRecord(screen, mfw, count, i);
            mfw->ids[count] = i;
            mfw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
            count++;
        }
    }
    mfw->ids[count] = 0;
    mfw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
    mfw->count = count + 1;
}

static void MonolithMission_FreeList(MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < mfw->count; i++) {
        GFL_BitmapFree(mfw->listBitmaps[i]);
    }
}

// Draws the names of the list into their bitmaps, at most 100 rows a frame; returns whether all are drawn
static BOOL MonolithMission_DrawList(MonolithMissionWork *mfw, MonolithWork *wk, int count) {
    int drawn = 0;
    StrBuf *str = GFL_StrBufCreate(37, HEAPID_MONOLITH);
    FestMission mission;
    int i;

    for (i = mfw->drawnCount; i < mfw->count; i++) {
        if (i >= 1 && i < mfw->count - 1) {
            func_ov027_02170cf8(mfw->festText, mfw->ids[i], 0, &mission);
            func_ov027_02170d04(mfw->festText, str, &mission, MONOLITH_MISSION_TEXT_HEAP);
            GFL_TextRendererDrawToBitmapEx(mfw->listBitmaps[i], 0, 0, str, wk->font, MONOLITH_MISSION_TEXT_COLOR);
        }
        if (++drawn >= 100) {
            break;
        }
    }
    GFL_StrBufFree(str);
    mfw->drawnCount = i;
    if (mfw->drawnCount >= count) {
        return TRUE;
    }
    return FALSE;
}

// Draws the cursor's row again, with its mission at a level
static void MonolithMission_RedrawRow(MonolithMissionWork *mfw, MonolithWork *wk, int level) {
    StrBuf *str;
    FestMission mission;

    GFL_BitmapFill(mfw->listBitmaps[mfw->cursor], 0);
    str = GFL_StrBufCreate(37, HEAPID_MONOLITH);
    func_ov027_02170cf8(mfw->festText, mfw->ids[mfw->cursor], level, &mission);
    func_ov027_02170d04(mfw->festText, str, &mission, MONOLITH_MISSION_TEXT_HEAP);
    GFL_TextRendererDrawToBitmapEx(mfw->listBitmaps[mfw->cursor], 0, 0, str, wk->font, MONOLITH_MISSION_TEXT_COLOR);
    GFL_StrBufFree(str);
    MonolithMission_UpdateRows(mfw);
    mfw->rowsChanged = TRUE;
}

static void MonolithMission_CreateRowActors(MonolithMissionWork *mfw, MonolithWork *wk) {
    BmpOamActorSetup setup = { NULL, 32, 0, wk->actorRes[1].fontPalette, 0, 10, 3, 1, CLACT_VRAM_SUB };
    int i;

    for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
        mfw->rowBitmaps[i] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
        setup.bitmap = mfw->rowBitmaps[i];
        mfw->rowActors[i] = BmpOam_ActorAdd(wk->bmpOam, &setup);
    }
}

static void MonolithMission_DeleteRowActors(MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
        BmpOam_ActorDel(mfw->rowActors[i]);
        GFL_BitmapFree(mfw->rowBitmaps[i]);
    }
}

static void MonolithMission_CreateStartButton(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    MonolithTool_CreateText(screen->work, &mfw->startButton, 1, MONOLITH_TEXT_NARROW, 64, 180, 46, NULL);
    if (func_0203d554() == TRUE) {
        MonolithTool_SetTextVisible(&mfw->startButton, FALSE);
    }
}

static void MonolithMission_DeleteStartButton(MonolithMissionWork *mfw) {
    MonolithTool_DeleteText(&mfw->startButton);
}

static void MonolithMission_UpdateStartButton(MonolithWork *wk, MonolithMissionWork *mfw) {
    MonolithTool_UpdateText(wk, &mfw->startButton);
}

static void MonolithMission_CreateReturnButton(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    MonolithTool_CreateReturnButton(screen->work, &mfw->returnButton);
}

static void MonolithMission_DeleteReturnButton(MonolithMissionWork *mfw) {
    MonolithTool_DeleteReturnButton(&mfw->returnButton);
}

static void MonolithMission_UpdateReturnButton(MonolithMissionWork *mfw) {
    MonolithTool_UpdateReturnButton(&mfw->returnButton);
}

static void MonolithMission_CreateArrows(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    mfw->arrowUp = MonolithTool_CreateActor(screen->work, 128, 28, 9);
    mfw->arrowDown = MonolithTool_CreateActor(screen->work, 128, 164, 10);
    func_0204c124(mfw->arrowUp, FALSE);
    func_0204c124(mfw->arrowDown, FALSE);
}

static void MonolithMission_DeleteArrows(MonolithMissionWork *mfw) {
    MonolithTool_DeleteActor(mfw->arrowUp);
    MonolithTool_DeleteActor(mfw->arrowDown);
}

// Shows the arrows where the list goes on
static void MonolithMission_UpdateArrows(MonolithMissionWork *mfw) {
    if (mfw->scroll <= (8 << 8)) {
        func_0204c124(mfw->arrowUp, FALSE);
    } else {
        func_0204c124(mfw->arrowUp, TRUE);
    }
    if ((mfw->scroll >> 8) >= mfw->count * MONOLITH_MISSION_ROW_HEIGHT - 200) {
        func_0204c124(mfw->arrowDown, FALSE);
    } else {
        func_0204c124(mfw->arrowDown, TRUE);
    }
    MonolithTool_UpdateActor(mfw->arrowUp);
    MonolithTool_UpdateActor(mfw->arrowDown);
}

// Moves the cursor with the keys, or scrolls the list with the stylus and picks the row tapped; returns whether a
// row was picked
static BOOL MonolithMission_Input(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    u32 x;
    u32 y;
    BOOL picked = FALSE;
    int index = MONOLITH_MISSION_NONE;
    BOOL touching;
    int row;
    int rem;
    int diff;

    mfw->keyScroll = 0;
    touching = func_0203da84(&x, &y);
    if (mfw->dragFrames == 0 && (x < 24 || x > 224)) {
        touching = FALSE;
    }
    if (MonolithMission_GetHeldKeys(mfw) &
        (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_RIGHT | PAD_KEY_LEFT | PAD_KEY_UP | PAD_KEY_DOWN)) {
        row = ((mfw->scroll >> 8) + 24) / MONOLITH_MISSION_ROW_HEIGHT;
        rem = ((mfw->scroll >> 8) + 24) % MONOLITH_MISSION_ROW_HEIGHT;
        mfw->velocity = 0;
        mfw->dragFrames = 0;
        if (mfw->cursor == MONOLITH_MISSION_NONE) {
            if (rem != 0) {
                row++;
            }
            if (row < 1) {
                row = 1;
            }
            if (row >= mfw->count - 2) {
                MonolithMission_SetCursor(mfw, mfw->count - 2, MONOLITH_MISSION_CURSOR_KEYS);
            } else {
                MonolithMission_SetCursor(mfw, row, MONOLITH_MISSION_CURSOR_KEYS);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return FALSE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            MonolithMission_SetCursor(mfw, mfw->cursor, MONOLITH_MISSION_CURSOR_KEYS);
            func_0203d564(FALSE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return TRUE;
        }
        touching = FALSE;
        if (MonolithMission_GetTypedKeys(mfw) & PAD_KEY_UP) {
            if (rem != 0) {
                row++;
            }
            if (MonolithMission_SetCursor(mfw, mfw->cursor - 1, MONOLITH_MISSION_CURSOR_KEYS) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            if (mfw->cursor < row) {
                mfw->keyScroll = -(32 << 8);
            }
            MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_CLAMP);
            return FALSE;
        }
        if (MonolithMission_GetTypedKeys(mfw) & PAD_KEY_DOWN) {
            row = ((mfw->scroll >> 8) + 168) / MONOLITH_MISSION_ROW_HEIGHT;
            if (MonolithMission_SetCursor(mfw, mfw->cursor + 1, MONOLITH_MISSION_CURSOR_KEYS) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            if (mfw->cursor >= row) {
                mfw->keyScroll = 32 << 8;
            }
            MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_CLAMP);
            return FALSE;
        }
        if (MonolithMission_GetTypedKeys(mfw) & (PAD_KEY_LEFT | PAD_KEY_RIGHT)) {
            MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_KEYS);
            return touching;
        }
    }
    if (mfw->dragFrames == 0) {
        if (touching == TRUE) {
            if (y > 24 && y < 168) {
                if (MonolithMission_GetRowAt(mfw, y) != MONOLITH_MISSION_NONE) {
                    mfw->dragFrames = 1;
                    mfw->lastY = y;
                    mfw->tapY = y;
                    MonolithMission_SetCursor(mfw, MONOLITH_MISSION_NONE, MONOLITH_MISSION_CURSOR_DRAG);
                }
            } else if (func_0203da48() == TRUE && mfw->cursor != MONOLITH_MISSION_NONE && x >= 16 && x <= 112 &&
                       y >= 172 && y <= 188) {
                picked = TRUE;
                index = mfw->cursor;
                func_0203d564(TRUE);
            } else {
                MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_TOUCH);
            }
        } else if (mfw->velocity != 0) {
            if (mfw->velocity > 0) {
                mfw->velocity -= 0xc0;
                if (mfw->velocity < 0) {
                    mfw->velocity = 0;
                }
            } else {
                mfw->velocity += 0xc0;
                if (mfw->velocity > 0) {
                    mfw->velocity = 0;
                }
            }
        }
    } else if (touching == FALSE) {
        mfw->dragFrames = 0;
        if (mfw->tapY != MONOLITH_MISSION_NONE) {
            diff = mfw->lastY - mfw->tapY;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff < 4) {
                row = MonolithMission_GetRowAt(mfw, mfw->lastY);
                if (row != MONOLITH_MISSION_NONE) {
                    picked = TRUE;
                    index = row;
                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                }
            }
        }
    } else {
        mfw->velocity = (mfw->lastY - y) << 8;
        mfw->lastY = y;
        mfw->dragFrames++;
        if (mfw->tapY != MONOLITH_MISSION_NONE) {
            diff = y - mfw->tapY;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff > 4) {
                mfw->tapY = MONOLITH_MISSION_NONE;
            }
        }
    }
    if (picked == TRUE) {
        mfw->velocity = 0;
        MonolithMission_SetCursor(mfw, index, MONOLITH_MISSION_CURSOR_TAP);
        MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_CLAMP);
    }
    return picked;
}

// Moves the cursor to a row of a mission, or hides it; returns whether it moved
static BOOL MonolithMission_SetCursor(MonolithMissionWork *mfw, int index, u32 by) {
    if (index != MONOLITH_MISSION_NONE && (index >= mfw->count - 1 || index < 1)) {
        return FALSE;
    }
    mfw->prevCursor = mfw->cursor;
    mfw->prevCursor2 = mfw->cursor;
    mfw->cursor = index;
    mfw->cursorChanged = TRUE;
    mfw->cursorBy = by;
    return TRUE;
}

// Sets the palette of a row's background, if the row is on the screen
static void MonolithMission_SetRowPalette(MonolithMissionWork *mfw, int index, int palette) {
    int scroll;
    int top;
    int rel;
    int mod32;
    int mod64;

    if (index != MONOLITH_MISSION_NONE) {
        scroll = mfw->scroll >> 8;
        top = scroll / MONOLITH_MISSION_ROW_HEIGHT;
        mod32 = scroll % 32;
        mod64 = scroll % 64;
        rel = index - top;
        if (rel >= 0 && rel < top + MONOLITH_MISSION_ROW_COUNT) {
            GFL_BGSysSetScrPaletteNo(MONOLITH_MISSION_BG_LIST, 3,
                                     (rel * MONOLITH_MISSION_ROW_HEIGHT + mod64 - mod32) % 256 / 8, 25, 4, palette);
        }
    }
}

static void MonolithMission_HighlightRow(MonolithMissionWork *mfw, int index) {
    MonolithMission_SetRowPalette(mfw, index, 3);
}

// The rows are palette 5 once the cheat code is entered
static void MonolithMission_UnhighlightRow(MonolithMissionWork *mfw, int index) {
    if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
        MonolithMission_SetRowPalette(mfw, index, 5);
    } else {
        MonolithMission_SetRowPalette(mfw, index, 0);
    }
}

static void MonolithMission_UnhighlightPrevRow(MonolithMissionWork *mfw) {
    if (mfw->cursorChanged == TRUE) {
        MonolithMission_UnhighlightRow(mfw, mfw->prevCursor);
    }
}

// Shows the cursor's row, and picks it when it is chosen a second time
static void MonolithMission_UpdateCursor(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    if (mfw->cursorChanged == TRUE) {
        MonolithMission_HighlightRow(mfw, mfw->cursor);
        if (mfw->cursor == MONOLITH_MISSION_NONE) {
            MonolithTool_SetPanelPulse(screen, FALSE, 1);
        } else if ((mfw->cursorBy == MONOLITH_MISSION_CURSOR_TAP && mfw->cursor == mfw->prevCursor2) ||
                   (mfw->cursorBy == MONOLITH_MISSION_CURSOR_KEYS && mfw->cursor == mfw->prevCursor)) {
            mfw->selected = mfw->cursor;
            if (mfw->cursorBy == MONOLITH_MISSION_CURSOR_KEYS) {
                MonolithTool_FlashPanel(screen, 1);
            } else {
                MonolithTool_SetTextPicked(screen, &mfw->startButton, 1, 0, 3);
            }
        } else {
            MonolithTool_SetPanelPulse(screen, TRUE, 1);
        }
        GFL_BGSysQueueScrLoad(MONOLITH_MISSION_BG_LIST);
        mfw->cursorChanged = FALSE;
    }
}

// Scrolls the list, between its top and its bottom; returns whether it moved a pixel
static BOOL MonolithMission_Scroll(MonolithMissionWork *mfw, s32 delta) {
    s32 old = mfw->scroll;
    int height = mfw->count * MONOLITH_MISSION_ROW_HEIGHT - 8;

    if (height <= 192) {
        return FALSE;
    }
    mfw->scroll = old + delta;
    if (mfw->scroll < (8 << 8)) {
        mfw->scroll = 8 << 8;
    } else if (mfw->scroll > (height - 192) << 8) {
        mfw->scroll = (height - 192) << 8;
    }
    if ((mfw->scroll >> 8) != (old >> 8)) {
        return TRUE;
    }
    return FALSE;
}

static void MonolithMission_MoveBG(MonolithMissionWork *mfw) {
    GFL_BGSysMoveBGReq(MONOLITH_MISSION_BG_LIST, BG_MOVE_SET_Y, (mfw->scroll >> 8) % 64);
}

// Puts the bitmaps of the rows on the screen into the row actors, and moves them and their icons with the scroll
static void MonolithMission_UpdateRows(MonolithMissionWork *mfw) {
    int scroll = mfw->scroll >> 8;
    int y = 8 - scroll % MONOLITH_MISSION_ROW_HEIGHT;
    int first = scroll / MONOLITH_MISSION_ROW_HEIGHT;
    int i;

    for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
        // The row's position is written out here and taken from y for the icons: the original adds the two anew
        // for each call, which MWCC does only when the first sum is not the one the later calls use
        BmpOam_ActorSetPos(mfw->rowActors[i], 32,
                           i * MONOLITH_MISSION_ROW_HEIGHT + (8 - scroll % MONOLITH_MISSION_ROW_HEIGHT));
        if (mfw->listBitmaps[first + i] != NULL) {
            GFL_BitmapCopy(mfw->listBitmaps[first + i], mfw->rowBitmaps[i]);
        }
        MonolithMission_SetIconY(mfw, MONOLITH_MISSION_ICON_NEW + i, i * MONOLITH_MISSION_ROW_HEIGHT + y);
        MonolithMission_SetIconY(mfw, MONOLITH_MISSION_ICON_CLEARED + i, i * MONOLITH_MISSION_ROW_HEIGHT + y);
    }
}

static void MonolithMission_VBlank(TCB *tcb, void *data) {
    MonolithMissionWork *mfw = data;
    int i;

    if (mfw->rowsChanged == TRUE) {
        for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
            BmpOam_ActorBmpTrans(mfw->rowActors[i]);
        }
        mfw->rowsChanged = FALSE;
    }
}

static int MonolithMission_GetRowAt(MonolithMissionWork *mfw, u32 y) {
    int row = ((mfw->scroll >> 8) + y) / MONOLITH_MISSION_ROW_HEIGHT;

    if (row >= mfw->count - 1) {
        row = MONOLITH_MISSION_NONE;
    }
    return row;
}

static void MonolithMission_InitState(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(screen->param->gsys));
    int i;

    mfw->flags = 0;
    mfw->level = 0;
    mfw->codeProgress = 0;
    for (i = 0; i < MONOLITH_MISSION_LIST_MAX; i++) {
        mfw->records[i].clearedLevel = 0;
        mfw->records[i].flags = 0;
    }
    mfw->font = GFL_FontCreate(23, 3, 0, FALSE, HEAPID_MONOLITH);
    mfw->missions = func_ov027_02170b8c(mfw->festText, HEAPID_MONOLITH);
    mfw->flags |= MONOLITH_MISSION_FLAG_FIRST_VISIT;
    if (EventWork_FlagGet(eventWork, MONOLITH_FLAG_VISITED) == TRUE) {
        mfw->flags &= ~MONOLITH_MISSION_FLAG_FIRST_VISIT;
    }
}

// Tells the top screen which mission and level the cursor is on
static void MonolithMission_UpdateFocus(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    if (mfw->cursor == MONOLITH_MISSION_NONE) {
        screen->state->missionIndex = 0xff;
        screen->state->missionLevel = 0;
    } else {
        screen->state->missionIndex = mfw->ids[mfw->cursor];
        if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
            screen->state->missionLevel = MONOLITH_MISSION_LEVEL_CHEAT;
        } else {
            screen->state->missionLevel = mfw->level;
        }
    }
}

static void MonolithMission_CreateIcons(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_ICON_COUNT; i++) {
        const MonolithMissionIcon *icon = &sMonolithMissionIcons[i];
        ClActorSetup setup;

        setup.x = sMonolithMissionIcons[i].x;
        setup.y = icon->y;
        setup.sequence = icon->sequence;
        setup.priority = icon->priority;
        setup.bgPriority = icon->bgPriority;
        mfw->icons[i] = func_0204c040(screen->work->clactUnit, screen->work->actorRes[1].chars, screen->work->actorRes[1].palette,
                                      screen->work->actorRes[1].cellAnims, &setup, CLACT_VRAM_SUB, HEAPID_MONOLITH);
        func_0204c520(mfw->icons[i], TRUE);
        if (icon->palette != 0) {
            func_0204c378(mfw->icons[i], (u8)icon->palette, TRUE);
        }
    }
}

static void MonolithMission_DeleteIcons(MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_ICON_COUNT; i++) {
        func_0204c108(mfw->icons[i]);
    }
}

static void MonolithMission_SetIconY(MonolithMissionWork *mfw, int index, int y) {
    ClActorPos pos;

    func_0204c178(mfw->icons[index], &pos, CLACT_VRAM_SUB);
    pos.y = y;
    pos.y += 8;
    func_0204c140(mfw->icons[index], &pos, CLACT_VRAM_SUB);
}

static void MonolithMission_SetIconVisible(MonolithMissionWork *mfw, int index, BOOL visible) {
    func_0204c124(mfw->icons[index], visible);
}

static void MonolithMission_SetIconSequence(MonolithMissionWork *mfw, int index, int sequence) {
    func_0204c488(mfw->icons[index], sequence);
    func_0204c520(mfw->icons[index], TRUE);
}

// Shows the markers of the new missions among the rows on the screen
static void MonolithMission_UpdateNewIcons(MonolithMissionWork *mfw) {
    int first = (mfw->scroll >> 8) / MONOLITH_MISSION_ROW_HEIGHT;
    int i;

    for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
        MonolithMission_SetIconVisible(mfw, MONOLITH_MISSION_ICON_NEW + i, TRUE);
        if (!(mfw->records[i + first].flags & MONOLITH_MISSION_RECORD_NEW)) {
            MonolithMission_SetIconVisible(mfw, MONOLITH_MISSION_ICON_NEW + i, FALSE);
        }
    }
}

// Shows the markers of the highest level cleared among the rows on the screen
static void MonolithMission_UpdateClearedIcons(MonolithMissionWork *mfw) {
    int first = (mfw->scroll >> 8) / MONOLITH_MISSION_ROW_HEIGHT;
    int i;

    for (i = 0; i < MONOLITH_MISSION_ROW_COUNT; i++) {
        u32 index = i + first;
        u8 cleared = mfw->records[index].clearedLevel;

        if (cleared == 0) {
            MonolithMission_SetIconVisible(mfw, MONOLITH_MISSION_ICON_CLEARED + i, FALSE);
        } else {
            if (cleared == 1) {
                MonolithMission_SetIconSequence(mfw, MONOLITH_MISSION_ICON_CLEARED + i, 12);
            } else if (cleared == 2) {
                MonolithMission_SetIconSequence(mfw, MONOLITH_MISSION_ICON_CLEARED + i, 13);
            } else if (cleared >= 3) {
                MonolithMission_SetIconSequence(mfw, MONOLITH_MISSION_ICON_CLEARED + i, 14);
            } else {
                continue;
            }
            MonolithMission_SetIconVisible(mfw, MONOLITH_MISSION_ICON_CLEARED + i, TRUE);
        }
    }
}

static void MonolithMission_CreateLevelButtons(MonolithWork *wk, MonolithMissionWork *mfw) {
    BmpOamActorSetup setup;
    int i;

    setup.bitmap = NULL;
    setup.palette = wk->actorRes[1].fontPalette;
    setup.paletteOffset = 0;
    setup.surface = 1;
    setup.vramType = CLACT_VRAM_SUB;
    for (i = 0; i < MONOLITH_MISSION_LEVEL_COUNT; i++) {
        setup.x = sMonolithMissionLevelButtons[i * LEVEL_BUTTON_VALUES + LEVEL_BUTTON_X];
        setup.y = sMonolithMissionLevelButtons[i * LEVEL_BUTTON_VALUES + LEVEL_BUTTON_Y];
        setup.priority = sMonolithMissionLevelButtons[i * LEVEL_BUTTON_VALUES + LEVEL_BUTTON_PRIORITY];
        setup.bgPriority = sMonolithMissionLevelButtons[i * LEVEL_BUTTON_VALUES + LEVEL_BUTTON_BG_PRIORITY];
        mfw->levelBitmaps[i] = GFL_BitmapCreate(6, 2, 0x20, HEAPID_MONOLITH);
        setup.bitmap = mfw->levelBitmaps[i];
        mfw->levelActors[i] = BmpOam_ActorAdd(wk->bmpOam, &setup);
        MonolithMission_DrawLevelButton(wk, mfw, i);
    }
}

static void MonolithMission_DeleteLevelButtons(MonolithMissionWork *mfw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_LEVEL_COUNT; i++) {
        BmpOam_ActorDel(mfw->levelActors[i]);
        GFL_BitmapFree(mfw->levelBitmaps[i]);
    }
}

// Draws the text of a level's button
static void MonolithMission_DrawLevelButton(MonolithWork *wk, MonolithMissionWork *mfw, int index) {
    StrBuf *str;
    StrBuf *format;

    WordSetNumber(wk->wordSet, 0, index + 1, 1, 0, TRUE);
    str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
    format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 63);
    GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
    GFL_TextRendererDrawToBitmapEx(mfw->levelBitmaps[index], 0, 0, str, mfw->font, MONOLITH_MISSION_TEXT_COLOR);
    BmpOam_ActorBmpTrans(mfw->levelActors[index]);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
}

static void MonolithMission_SetLevelButtonVisible(MonolithMissionWork *mfw, int index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(mfw->levelActors[index], visible);
    MonolithMission_SetIconVisible(mfw, index, visible);
}

// Shows the buttons of the levels that the cursor's mission can be picked at, and takes its record's new marker
static void MonolithMission_UpdateLevelButtons(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    int buttonCount;
    int i;

    if ((mfw->flags & MONOLITH_MISSION_FLAG_NO_CURSOR) || (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT)) {
        for (i = 0; i < MONOLITH_MISSION_LEVEL_COUNT; i++) {
            MonolithMission_SetLevelButtonVisible(mfw, i, FALSE);
        }
        return;
    }
    buttonCount = sMonolithMissionLevelRanges[mfw->records[mfw->cursor].clearedLevel].buttonCount;
    for (i = 0; i < MONOLITH_MISSION_LEVEL_COUNT; i++) {
        MonolithMission_SetLevelButtonVisible(mfw, i, FALSE);
    }
    for (i = 0; i < buttonCount; i++) {
        MonolithMission_SetLevelButtonVisible(mfw, i, TRUE);
    }
    if ((mfw->records[mfw->cursor].flags & MONOLITH_MISSION_RECORD_NEW) &&
        !(mfw->records[mfw->cursor].flags & MONOLITH_MISSION_RECORD_SEEN)) {
        MonolithMission_ClearNew(screen, mfw, mfw->ids[mfw->cursor]);
        mfw->records[mfw->cursor].flags |= MONOLITH_MISSION_RECORD_SEEN;
    }
}

// Whether a mission is on the list: the first before the first visit is done and the rest after, those with a
// condition when it is met
static BOOL MonolithMission_IsAvailable(MonolithScreenParam *screen, MonolithMissionWork *mfw, int index) {
    GameData *gameData = GSYS_GetGameData(screen->param->gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    BOOL available;
    PlayerInfo *playerInfo;

    if (EventWork_FlagGet(eventWork, MONOLITH_FLAG_VISITED) == TRUE) {
        if (index == 0) {
            return FALSE;
        }
    } else if (index != 0) {
        return FALSE;
    }
    available = FALSE;
    switch (mfw->missions[index].header.unk01) {
    case 0:
        available = TRUE;
        break;
    case MONOLITH_MISSION_UNLOCK_VERSION:
        available = TRUE;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        playerInfo = GetGameDataPlayerInfo(gameData);
        available = isOneShotDRObtained(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData)),
                                        mfw->missions[index].header.unk01 - 2, playerInfo);
        break;
    }
    if (!available) {
        return FALSE;
    }
    if (mfw->missions[index].params.unk10 != 0 && !EventWork_FlagGet(eventWork, mfw->missions[index].params.unk10)) {
        return FALSE;
    }
    if (mfw->missions[index].params.unk12 != 0 && !EventWork_FlagGet(eventWork, mfw->missions[index].params.unk12)) {
        return FALSE;
    }
    return TRUE;
}

// Reads a mission's record from the save into the list
static void MonolithMission_LoadRecord(MonolithScreenParam *screen, MonolithMissionWork *mfw, int index, int id) {
    void *records = func_02010dec(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys)));

    mfw->records[index].clearedLevel = func_02010f6c(records, id);
    if (func_02010fb0(records, id) == TRUE) {
        mfw->records[index].flags |= MONOLITH_MISSION_RECORD_NEW;
    } else {
        mfw->records[index].flags &= ~MONOLITH_MISSION_RECORD_NEW;
    }
}

// Takes the mission's new marker from the save
static void MonolithMission_ClearNew(MonolithScreenParam *screen, MonolithMissionWork *mfw, int id) {
    func_02010fc0(func_02010dec(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys))), id);
}

// Moves the level of the cursor's mission, by the stylus or the keys, or keeps it within the levels that can be picked
static void MonolithMission_MoveLevelCursor(MonolithScreenParam *screen, MonolithMissionWork *mfw, int by) {
    s16 prevLevel = mfw->level;
    BOOL changed = FALSE;
    s16 hit;
    int maxLevel;

    if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
        if (mfw->cursor != MONOLITH_MISSION_NONE) {
            MonolithMission_RedrawRow(mfw, screen->work, MONOLITH_MISSION_LEVEL_CHEAT);
        }
        return;
    }
    if (mfw->cursor == MONOLITH_MISSION_NONE) {
        return;
    }
    maxLevel = sMonolithMissionLevelRanges[mfw->records[mfw->cursor].clearedLevel].maxLevel;
    if (by == MONOLITH_MISSION_LEVEL_CLAMP) {
        if (mfw->level > maxLevel) {
            mfw->level = 0;
            changed = TRUE;
        }
    } else if (by == MONOLITH_MISSION_LEVEL_TOUCH) {
        hit = func_0203da0c(sMonolithMissionLevelRects);
        if (hit >= 0 && hit <= maxLevel) {
            mfw->level = hit;
            changed = TRUE;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    } else if (by == MONOLITH_MISSION_LEVEL_KEYS) {
        if (MonolithMission_GetTypedKeys(mfw) & PAD_KEY_LEFT) {
            if (mfw->level != 0) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            mfw->level--;
            if (mfw->level < 0) {
                mfw->level = 0;
            }
            changed = TRUE;
        } else if (MonolithMission_GetTypedKeys(mfw) & PAD_KEY_RIGHT) {
            if (mfw->level != maxLevel) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            mfw->level++;
            if (mfw->level >= maxLevel) {
                mfw->level = maxLevel;
            }
            changed = TRUE;
        }
    }
    if (changed == TRUE) {
        func_0204c378(mfw->icons[prevLevel], 0, TRUE);
        func_0204c378(mfw->icons[mfw->level], 4, TRUE);
    }
    MonolithMission_RedrawRow(mfw, screen->work, mfw->level);
}

// Gives every row the palette that marks the cheat code
static void MonolithMission_PaintCheatRows(MonolithMissionWork *mfw) {
    GFL_BGSysSetScrPaletteNo(MONOLITH_MISSION_BG_LIST, 0, 0, 32, 32, 5);
    GFL_BGSysQueueScrLoad(MONOLITH_MISSION_BG_LIST);
}

// Counts the keys of the cheat code, which is ignored before the first visit is done
static void MonolithMission_UpdateCheatCode(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    u32 key;

    if (!(mfw->flags & MONOLITH_MISSION_FLAG_FIRST_VISIT) && !(mfw->flags & MONOLITH_MISSION_FLAG_CHEAT)) {
        if (func_0203da48()) {
            mfw->codeProgress = 0;
            return;
        }
        if (GCTX_HIDGetPressedKeys() != 0) {
            key = sMonolithMissionCode[mfw->codeProgress];
            if (GCTX_HIDGetPressedKeys() & key) {
                mfw->codeProgress++;
                if (mfw->codeProgress >= MONOLITH_MISSION_CODE_LENGTH) {
                    mfw->flags |= MONOLITH_MISSION_FLAG_CHEAT;
                    GFL_SndSEPlay(SEQ_SE_FLD_133);
                    MonolithMission_PaintCheatRows(mfw);
                    MonolithMission_MoveLevelCursor(screen, mfw, MONOLITH_MISSION_LEVEL_CLAMP);
                }
            } else {
                mfw->codeProgress = 0;
                key = sMonolithMissionCode[mfw->codeProgress];
                if (GCTX_HIDGetPressedKeys() & key) {
                    mfw->codeProgress++;
                }
            }
        }
    }
}

static void MonolithMission_InitIcons(MonolithMissionWork *mfw, BOOL skip) {
    int i;

    if (skip == FALSE) {
        MonolithMission_UpdateNewIcons(mfw);
        MonolithMission_UpdateClearedIcons(mfw);
        for (i = 0; i < MONOLITH_MISSION_LEVEL_COUNT; i++) {
            MonolithMission_SetLevelButtonVisible(mfw, i, FALSE);
        }
    }
}

// Asks whether to join the picked mission, at its level
static void MonolithMission_ShowStartMessage(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
    int level = mfw->level;
    StrBuf *str;
    FestMission mission;

    if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
        level = MONOLITH_MISSION_LEVEL_CHEAT;
    }
    str = GFL_StrBufCreate(37, HEAPID_MONOLITH);
    func_ov027_02170cf8(mfw->festText, mfw->ids[mfw->cursor], level, &mission);
    func_ov027_02170d04(mfw->festText, str, &mission, MONOLITH_MISSION_TEXT_HEAP);
    func_0202437c(screen->work->wordSet, 0, str, 0, 1, 2);
    GFL_StrBufFree(str);
    level = mfw->level + 1;
    if (mfw->flags & MONOLITH_MISSION_FLAG_CHEAT) {
        level = 4;
    }
    WordSetNumber(screen->work->wordSet, 1, level, 1, 0, TRUE);
    MonolithMission_ShowMessageExpanded(mfw, screen->work, 47);
}

static u32 MonolithMission_GetHeldKeys(MonolithMissionWork *mfw) {
    return GCTX_HIDGetHeldKeys();
}

static u32 MonolithMission_GetTypedKeys(MonolithMissionWork *mfw) {
    return GCTX_HIDGetTypedKeys();
}

static void MonolithMission_Dummy(MonolithScreenParam *screen, MonolithMissionWork *mfw) {
}
