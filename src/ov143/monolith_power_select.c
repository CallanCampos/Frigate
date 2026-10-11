// The Entralink monolith's pass power list on the bottom screen: a list of the pass powers the Entree's levels unlock,
// scrolled with the stylus or the keys, from which one is received into the three slots of the C-Gear, giving another
// back when they are full. The ROM gives no name for this file; monolith_power_select.c is a guess from what it does,
// and its asserts call its work `mpw`

#include "types.h"
#include "app/monolith/monolith_power_select.h"
#include "app/monolith/monolith_tool.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
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
#define MONOLITH_POWER_BG_TEXT 4
#define MONOLITH_POWER_BG_FRAME 5
#define MONOLITH_POWER_BG_LIST 6

// The list holds every pass power, with an empty row above and below
#define MONOLITH_POWER_LIST_MAX 66
#define PASS_POWER_COUNT 64

// What CheckPassPowerUnlocked returns for a pass power that isn't shown
#define PASS_POWER_LOCKED 2

// The rows of the list on the screen, each 32 pixels tall
#define MONOLITH_POWER_ROW_COUNT 7
#define MONOLITH_POWER_ROW_HEIGHT 32

// No row
#define MONOLITH_POWER_NONE 0xffff

// How the cursor was last moved: by dragging the list, by the keys or by a tap
enum {
    MONOLITH_POWER_CURSOR_DRAG,
    MONOLITH_POWER_CURSOR_KEYS,
    MONOLITH_POWER_CURSOR_TAP,
};

// Set while receiving a pass power with no slot free, so that one is given back
#define MONOLITH_POWER_FLAG_FULL 1

enum {
    MONOLITH_POWER_SEQ_START,
    MONOLITH_POWER_SEQ_SHOW_HELP,
    MONOLITH_POWER_SEQ_WAIT_HELP,
    MONOLITH_POWER_SEQ_WAIT_RELEASE,
    MONOLITH_POWER_SEQ_INPUT,
    MONOLITH_POWER_SEQ_ASK,
    MONOLITH_POWER_SEQ_WAIT_ASK,
    MONOLITH_POWER_SEQ_YES_NO,
    MONOLITH_POWER_SEQ_RECEIVE,
    MONOLITH_POWER_SEQ_RECEIVED,
    MONOLITH_POWER_SEQ_WAIT_RECEIVED,
    MONOLITH_POWER_SEQ_WAIT_FADE_OUT,
    MONOLITH_POWER_SEQ_PLAY_JINGLE,
    MONOLITH_POWER_SEQ_WAIT_JINGLE,
    MONOLITH_POWER_SEQ_WAIT_FADE_IN,
    MONOLITH_POWER_SEQ_UNUSED_SAVE,
    MONOLITH_POWER_SEQ_WAIT_TOUCH_RELEASE,
    MONOLITH_POWER_SEQ_RETURN,
    MONOLITH_POWER_SEQ_FULL,
    MONOLITH_POWER_SEQ_WAIT_FULL,
    MONOLITH_POWER_SEQ_PICK_RETURNED,
    MONOLITH_POWER_SEQ_EXCHANGE,
    MONOLITH_POWER_SEQ_EXCHANGED,
};

typedef struct {
    TCB *vblankTask;
    u32 barChar;
    // The RECEIVE POWER button
    MonolithTextActor receiveButton;
    ClActor *arrowUp;
    ClActor *arrowDown;
    MsgData *msgPowerNames;
    // Each row of the list drawn into a bitmap, its pass power and whether it can't be received
    GFLBitmap *listBitmaps[MONOLITH_POWER_LIST_MAX];
    u32 ids[MONOLITH_POWER_LIST_MAX];
    u32 states[MONOLITH_POWER_LIST_MAX];
    u8 count;
    u8 drawnCount;
    // The message window and the title's
    BmpWin *windows[2];
    PrintWindow titlePrint;
    StrBuf *msgStr;
    PrintStream *print_stream;
    MonolithReturnButton returnButton;
    // The actors that show the rows on the screen
    BmpOamActor *rowActors[MONOLITH_POWER_ROW_COUNT];
    GFLBitmap *rowBitmaps[MONOLITH_POWER_ROW_COUNT];
    // The list's scroll in 1/256 pixels, and how it moves
    s32 scroll;
    u32 dragFrames;
    s32 velocity;
    s32 keyScroll;
    u32 lastY;
    // Where the stylus touched the list, or MONOLITH_POWER_NONE once it moved
    u32 tapY;
    BOOL rowsChanged;
    int cursor;
    int prevCursor;
    int prevCursor2;
    // The row picked to receive, or MONOLITH_POWER_NONE
    int selected;
    BOOL cursorChanged;
    u32 cursorBy;
    u32 flags;
    // The slot of the pass power to give back
    int returnSlot;
    u32 unk3D4;
    KeyCursor *keyCursor;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *app_menu_work;
} MonolithPowerWork;

static BOOL MonolithPower_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithPower_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithPower_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithPower_CreateBGs(void);
static void MonolithPower_ReleaseBGs(void);
static void MonolithPower_LoadGraphics(MonolithPowerWork *mpw, MonolithWork *wk);
static void MonolithPower_FreeBarChar(MonolithPowerWork *mpw);
static void MonolithPower_CreateWindows(MonolithPowerWork *mpw, MonolithWork *wk);
static void MonolithPower_FreeWindows(MonolithPowerWork *mpw);
static void MonolithPower_DrawTitle(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static void MonolithPower_ShowMessage(MonolithPowerWork *mpw, MonolithWork *wk, u32 msgId);
static void MonolithPower_ShowMessageExpanded(MonolithPowerWork *mpw, MonolithWork *wk, u32 msgId);
static BOOL MonolithPower_UpdateMessage(MonolithWork *wk, MonolithPowerWork *mpw);
static void MonolithPower_ClearMessage(MonolithPowerWork *mpw);
static void MonolithPower_ClearUnusedRows(MonolithPowerWork *mpw);
static void MonolithPower_CreateList(MonolithScreenParam *screen, MonolithPowerWork *mpw, MonolithWork *wk);
static void MonolithPower_FreeList(MonolithPowerWork *mpw);
static BOOL MonolithPower_DrawList(MonolithPowerWork *mpw, MonolithWork *wk);
static void MonolithPower_CreateRowActors(MonolithPowerWork *mpw, MonolithWork *wk);
static void MonolithPower_DeleteRowActors(MonolithPowerWork *mpw);
static void MonolithPower_CreateReceiveButton(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static void MonolithPower_DeleteReceiveButton(MonolithPowerWork *mpw);
static void MonolithPower_UpdateReceiveButton(MonolithWork *wk, MonolithPowerWork *mpw);
static void MonolithPower_CreateReturnButton(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static void MonolithPower_DeleteReturnButton(MonolithPowerWork *mpw);
static void MonolithPower_UpdateReturnButton(MonolithPowerWork *mpw);
static void MonolithPower_CreateArrows(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static void MonolithPower_DeleteArrows(MonolithPowerWork *mpw);
static void MonolithPower_UpdateArrows(MonolithPowerWork *mpw);
static BOOL MonolithPower_Input(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static BOOL MonolithPower_SetCursor(MonolithPowerWork *mpw, int index, u32 by);
static void MonolithPower_SetRowPalette(MonolithPowerWork *mpw, int index, u8 palette);
static void MonolithPower_HighlightRow(MonolithPowerWork *mpw, int index);
static void MonolithPower_UnhighlightRow(MonolithPowerWork *mpw, int index);
static void MonolithPower_UnhighlightPrevRow(MonolithPowerWork *mpw);
static void MonolithPower_UpdateCursor(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static BOOL MonolithPower_Scroll(MonolithPowerWork *mpw, s32 delta);
static void MonolithPower_MoveBG(MonolithPowerWork *mpw);
static void MonolithPower_UpdateRows(MonolithPowerWork *mpw);
static void MonolithPower_VBlank(TCB *tcb, void *data);
static int MonolithPower_GetRowAt(MonolithPowerWork *mpw, u32 y);
static void MonolithPower_LoadEquipped(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static void MonolithPower_SaveEquipped(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static int MonolithPower_WaitReceived(MonolithScreenParam *screen, MonolithPowerWork *mpw);
static int MonolithPower_FindPower(MonolithPowerWork *mpw, u32 passPower);
static void MonolithPower_RedrawRow(MonolithScreenParam *screen, MonolithPowerWork *mpw, int index);
static void MonolithPower_Equip(MonolithScreenParam *screen, MonolithPowerWork *mpw, u8 passPower, int slot);
static int MonolithPower_FindEmptySlot(MonolithScreenParam *screen);
static void MonolithPower_SetDim(MonolithPowerWork *mpw, BOOL dim);
static void MonolithPower_UpdateKeyCursor(MonolithPowerWork *mpw, BOOL printing);
static u32 MonolithPower_GetHeldKeys(MonolithPowerWork *mpw);
static u32 MonolithPower_GetTypedKeys(MonolithPowerWork *mpw);
static void MonolithPower_Dummy(MonolithScreenParam *screen, MonolithPowerWork *mpw);

// The text colors of a pass power that can be received and of one that can't, for a row drawn again and for the list
static const u16 sMonolithPowerRowColors[2] = { 0x3c40, 0x820 };
static const u16 sMonolithPowerListColors[2] = { 0x3c40, 0x820 };

static const TouchRect sMonolithPowerTouchRects[] = {
    { 172, 188, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

const GameProcFunctions MONOLITH_POWER_SELECT_PROC_FUNCTIONS = {
    MonolithPower_Init,
    MonolithPower_Main,
    MonolithPower_Exit,
};

// The setups of MONOLITH_POWER_BG_TEXT, _FRAME and _LIST. One array, which the ROM has after the proc table
static const BGSetup sMonolithPowerBGSetups[3] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x3000), GX_BG_CHARBASE(0x14000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL MonolithPower_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerWork *mpw = work;
    BOOL printing;

    switch (*state) {
    case 0:
        mpw = GFL_ProcInitSubsystem(proc, sizeof(MonolithPowerWork), HEAPID_MONOLITH);
        sys_memset(mpw, 0, sizeof(MonolithPowerWork));
        mpw->scroll = 8 << 8;
        mpw->selected = MONOLITH_POWER_NONE;
        mpw->prevCursor = MONOLITH_POWER_NONE;
        mpw->prevCursor2 = MONOLITH_POWER_NONE;
        MonolithTool_InitPanels(screen);
        mpw->msgPowerNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PASS_POWERS, HEAPID_MONOLITH);
        MonolithPower_CreateBGs();
        MonolithPower_LoadGraphics(mpw, screen->work);
        MonolithPower_CreateWindows(mpw, screen->work);
        MonolithPower_MoveBG(mpw);
        MonolithPower_CreateReturnButton(screen, mpw);
        MonolithPower_CreateRowActors(mpw, screen->work);
        MonolithPower_CreateReceiveButton(screen, mpw);
        MonolithPower_CreateArrows(screen, mpw);
        MonolithPower_DrawTitle(screen, mpw);
        mpw->menuRes = MonolithTool_CreateMenuRes(screen->work, MONOLITH_POWER_BG_TEXT, HEAPID_MONOLITH);
        screen->state->focusPower = HIGH_LINK_POWER_NONE;
        mpw->vblankTask = GFL_VBlankTCBAdd(MonolithPower_VBlank, mpw, 3);
        MonolithPower_CreateList(screen, mpw, screen->work);
        MonolithPower_ClearUnusedRows(mpw);
        MonolithPower_LoadEquipped(screen, mpw);
        (*state)++;
        break;
    case 1:
        if (MonolithPower_DrawList(mpw, screen->work) == TRUE) {
            MonolithPower_UpdateRows(mpw);
            mpw->rowsChanged = TRUE;
            if (func_0203d554() == FALSE) {
                MonolithPower_SetCursor(mpw, 1, MONOLITH_POWER_CURSOR_DRAG);
            } else {
                MonolithPower_SetCursor(mpw, MONOLITH_POWER_NONE, MONOLITH_POWER_CURSOR_DRAG);
            }
            (*state)++;
        }
        break;
    case 2:
        printing = FALSE;
        PrintWindow_Flush(&mpw->titlePrint, screen->work->printQueue);
        if (PrintWindow_IsPrinted(&mpw->titlePrint) == FALSE) {
            printing = TRUE;
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_FRAME, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_LIST, TRUE);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithPower_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerWork *mpw = work;
    u32 pressed;
    int hit;
    BOOL yes;
    u32 pos;
    GameCommSys *comm;

    GSYS_GetGameCommSystem(screen->param->gsys);
    MonolithTool_UpdatePanel(screen, 1);
    MonolithTool_UpdatePanel(screen, 3);
    MonolithPower_UpdateReturnButton(mpw);
    MonolithPower_UpdateReceiveButton(screen->work, mpw);
    PrintWindow_Flush(&mpw->titlePrint, screen->work->printQueue);
    if (screen->exiting == TRUE && *state == MONOLITH_POWER_SEQ_INPUT) {
        return TRUE;
    }
    switch (*state) {
    case MONOLITH_POWER_SEQ_START:
        *state = MONOLITH_POWER_SEQ_SHOW_HELP;
        break;
    case MONOLITH_POWER_SEQ_SHOW_HELP:
        MonolithPower_ShowMessage(mpw, screen->work, 38);
        (*state)++;
        break;
    case MONOLITH_POWER_SEQ_WAIT_HELP:
        if (MonolithPower_UpdateMessage(screen->work, mpw) == TRUE &&
            (func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            BOOL visible;

            MonolithPower_ClearMessage(mpw);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            if (func_0203da48()) {
                MonolithPower_SetCursor(mpw, MONOLITH_POWER_NONE, MONOLITH_POWER_CURSOR_DRAG);
            } else {
                MonolithPower_SetCursor(mpw, 1, MONOLITH_POWER_CURSOR_DRAG);
            }
            visible = TRUE;
            if (func_0203d554() == TRUE) {
                visible = FALSE;
            }
            MonolithTool_SetTextVisible(&mpw->receiveButton, visible);
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_RELEASE:
        if (!func_0203da2c() && !(GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            *state = MONOLITH_POWER_SEQ_INPUT;
        }
        break;
    case MONOLITH_POWER_SEQ_INPUT:
        pressed = GCTX_HIDGetPressedKeys();
        hit = func_0203da0c(sMonolithPowerTouchRects);
        if (hit == 0 || (pressed & PAD_BUTTON_B)) {
            MonolithTool_PressReturnButton(&mpw->returnButton);
            func_0203d564(hit == 0 ? TRUE : FALSE);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            *state = MONOLITH_POWER_SEQ_RETURN;
            break;
        }
        MonolithPower_Input(screen, mpw);
        MonolithPower_UnhighlightPrevRow(mpw);
        if (MonolithPower_Scroll(mpw, mpw->velocity + mpw->keyScroll) == TRUE) {
            MonolithPower_MoveBG(mpw);
            MonolithPower_UpdateRows(mpw);
            mpw->rowsChanged = TRUE;
        }
        MonolithPower_UpdateCursor(screen, mpw);
        if (mpw->cursor == MONOLITH_POWER_NONE) {
            screen->state->focusPower = HIGH_LINK_POWER_NONE;
            MonolithTool_SetTextVisible(&mpw->receiveButton, FALSE);
        } else {
            screen->state->focusPower = mpw->ids[mpw->cursor];
            screen->state->focusPowerState = mpw->states[mpw->cursor];
            if (mpw->states[mpw->cursor] == 0) {
                MonolithTool_SetTextVisible(&mpw->receiveButton, TRUE);
            } else {
                MonolithTool_SetTextVisible(&mpw->receiveButton, FALSE);
            }
        }
        MonolithPower_Dummy(screen, mpw);
        if (mpw->selected != MONOLITH_POWER_NONE) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            if (MonolithPower_FindEmptySlot(screen) == -1) {
                mpw->flags |= MONOLITH_POWER_FLAG_FULL;
                *state = MONOLITH_POWER_SEQ_FULL;
            } else {
                mpw->flags &= ~MONOLITH_POWER_FLAG_FULL;
                *state = MONOLITH_POWER_SEQ_ASK;
            }
        }
        break;
    case MONOLITH_POWER_SEQ_ASK:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            MonolithPower_ShowMessage(mpw, screen->work, 34);
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_ASK:
        if (MonolithPower_UpdateMessage(screen->work, mpw) == TRUE) {
            GFL_ASSERT(mpw->app_menu_work == NULL);
            mpw->app_menu_work = MonolithTool_CreateYesNoMenu(screen->work, mpw->menuRes, HEAPID_MONOLITH);
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_YES_NO:
        if (MonolithTool_UpdateYesNoMenu(screen->work, 4, mpw->app_menu_work, &yes) == TRUE) {
            MonolithTool_FreeYesNoMenu(mpw->app_menu_work);
            mpw->app_menu_work = NULL;
            MonolithPower_ClearMessage(mpw);
            if (yes == TRUE) {
                *state = MONOLITH_POWER_SEQ_RECEIVE;
            } else {
                mpw->selected = MONOLITH_POWER_NONE;
                MonolithTool_SetPanelPulse(screen, TRUE, 1);
                MonolithTool_SetTextCursor(screen, &mpw->receiveButton, 1, 0xff, 3);
                *state = MONOLITH_POWER_SEQ_INPUT;
            }
        }
        break;
    case MONOLITH_POWER_SEQ_RECEIVE:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            loadPassPowerToStrbuf(screen->work->wordSet, 0, mpw->ids[mpw->selected]);
            MonolithPower_ShowMessageExpanded(mpw, screen->work, 36);
            *state = MONOLITH_POWER_SEQ_RECEIVED;
        }
        break;
    case MONOLITH_POWER_SEQ_RECEIVED:
        *state = MONOLITH_POWER_SEQ_WAIT_RECEIVED;
        break;
    case MONOLITH_POWER_SEQ_WAIT_RECEIVED:
        if (MonolithPower_UpdateMessage(screen->work, mpw) == TRUE) {
            GFL_SndBGMFadeOut(6);
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_FADE_OUT:
        if (GFL_SndBGMIsFading() == FALSE) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_PLAY_JINGLE:
        GFL_SndBGMPlay(SEQ_ME_DEL_POWER, 0xffff);
        (*state)++;
        break;
    case MONOLITH_POWER_SEQ_WAIT_JINGLE:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            GFL_SndBGMFadeIn(6);
            (*state)++;
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_FADE_IN:
        if (GFL_SndBGMIsFading() == FALSE && MonolithPower_WaitReceived(screen, mpw) == 0) {
            *state = MONOLITH_POWER_SEQ_INPUT;
        }
        break;
    case MONOLITH_POWER_SEQ_UNUSED_SAVE:
        if (func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            MonolithPower_ClearMessage(mpw);
            func_0200c668(getHighLinkBlockAddress(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys))),
                          mpw->ids[mpw->selected], 0);
            comm = GSYS_GetGameCommSystem(screen->param->gsys);
            func_0202bf68(comm, func_020175cc(GSYS_GetGameData(screen->param->gsys)), screen->param->netId);
            mpw->selected = MONOLITH_POWER_NONE;
            MonolithTool_SetPanelPulse(screen, TRUE, 1);
            MonolithTool_SetTextCursor(screen, &mpw->receiveButton, 1, 0xff, 3);
            if (func_0203da48()) {
                *state = MONOLITH_POWER_SEQ_WAIT_TOUCH_RELEASE;
            } else {
                *state = MONOLITH_POWER_SEQ_RETURN;
            }
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_TOUCH_RELEASE:
        if (!func_0203da2c()) {
            *state = MONOLITH_POWER_SEQ_RETURN;
        }
        break;
    case MONOLITH_POWER_SEQ_RETURN:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH &&
            MonolithTool_IsReturnButtonBlinking(&mpw->returnButton) == FALSE) {
            screen->next = MONOLITH_SCREEN_MENU;
            screen->state->menuCursor = MONOLITH_MENU_PASS_POWER;
            return TRUE;
        }
        break;
    case MONOLITH_POWER_SEQ_FULL:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            MonolithPower_ShowMessage(mpw, screen->work, 72);
            *state = MONOLITH_POWER_SEQ_WAIT_FULL;
        }
        break;
    case MONOLITH_POWER_SEQ_WAIT_FULL:
        if (MonolithPower_UpdateMessage(screen->work, mpw) == TRUE) {
            MonolithPower_SetDim(mpw, TRUE);
            mpw->app_menu_work =
                MonolithTool_CreateReturnPowerMenu(screen, screen->work, mpw->menuRes, HEAPID_MONOLITH);
            *state = MONOLITH_POWER_SEQ_PICK_RETURNED;
        }
        break;
    case MONOLITH_POWER_SEQ_PICK_RETURNED:
        if (MonolithTool_UpdateReturnPowerMenu(screen->work, mpw->app_menu_work, &pos) == TRUE) {
            MonolithPower_SetDim(mpw, FALSE);
            MonolithTool_FreeReturnPowerMenu(mpw->app_menu_work);
            mpw->app_menu_work = NULL;
            MonolithPower_ClearMessage(mpw);
            if (pos != 3) {
                mpw->returnSlot = pos;
                *state = MONOLITH_POWER_SEQ_EXCHANGE;
            } else {
                mpw->selected = MONOLITH_POWER_NONE;
                MonolithTool_SetPanelPulse(screen, TRUE, 1);
                MonolithTool_SetTextCursor(screen, &mpw->receiveButton, 1, 0xff, 3);
                *state = MONOLITH_POWER_SEQ_INPUT;
            }
        }
        break;
    case MONOLITH_POWER_SEQ_EXCHANGE:
        if (MonolithTool_GetPanelMode(screen, 1) != PANEL_MODE_FLASH &&
            MonolithTool_GetPanelMode(screen, 3) != PANEL_MODE_FLASH) {
            loadPassPowerToStrbuf(screen->work->wordSet, 0, screen->state->equipped[mpw->returnSlot]);
            loadPassPowerToStrbuf(screen->work->wordSet, 1, mpw->ids[mpw->selected]);
            MonolithPower_ShowMessageExpanded(mpw, screen->work, 73);
            *state = MONOLITH_POWER_SEQ_RECEIVED;
        }
        break;
    case MONOLITH_POWER_SEQ_EXCHANGED:
        *state = MONOLITH_POWER_SEQ_WAIT_RECEIVED;
        break;
    }
    MonolithPower_UpdateArrows(mpw);
    return FALSE;
}

static BOOL MonolithPower_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerWork *mpw = work;

    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    MonolithPower_SaveEquipped(screen, mpw);
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG3);
    GFL_TCBRemove(mpw->vblankTask);
    if (mpw->print_stream != NULL) {
        func_020223cc(mpw->print_stream);
    }
    if (mpw->msgStr != NULL) {
        GFL_StrBufFree(mpw->msgStr);
    }
    MonolithPower_FreeList(mpw);
    if (mpw->app_menu_work != NULL) {
        MonolithTool_FreeYesNoMenu(mpw->app_menu_work);
    }
    MonolithTool_FreeMenuRes(mpw->menuRes);
    MonolithPower_DeleteReceiveButton(mpw);
    MonolithPower_DeleteRowActors(mpw);
    MonolithPower_DeleteReturnButton(mpw);
    MonolithPower_DeleteArrows(mpw);
    MonolithPower_FreeBarChar(mpw);
    MonolithPower_FreeWindows(mpw);
    MonolithPower_ReleaseBGs();
    GFL_MsgDataFree(mpw->msgPowerNames);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithPower_CreateBGs(void) {
    GFL_BGSysCreateBG(MONOLITH_POWER_BG_TEXT, &sMonolithPowerBGSetups[0], 0);
    GFL_BGSysCreateBG(MONOLITH_POWER_BG_FRAME, &sMonolithPowerBGSetups[1], 0);
    GFL_BGSysCreateBG(MONOLITH_POWER_BG_LIST, &sMonolithPowerBGSetups[2], 0);
    GFL_BGSysFillScrArea(MONOLITH_POWER_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_POWER_BG_FRAME, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_POWER_BG_LIST, 0, 0, 0, 32, 32, 17);
}

static void MonolithPower_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_FRAME, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_POWER_BG_LIST, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_POWER_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_POWER_BG_FRAME);
    GFL_BGSysReleaseBG(MONOLITH_POWER_BG_LIST);
}

static void MonolithPower_LoadGraphics(MonolithPowerWork *mpw, MonolithWork *wk) {
    ArcTool *menuArc;

    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 7, MONOLITH_POWER_BG_LIST, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadArcNCGRStatic(wk->arc, 16, MONOLITH_POWER_BG_FRAME, 0, 0, TRUE, HEAPID_MONOLITH);
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 9, MONOLITH_POWER_BG_FRAME, 0, 0, TRUE, HEAPID_MONOLITH);
    menuArc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, HEAPID_MONOLITH);
    mpw->barChar = GFL_BGSysAllocChar(MONOLITH_POWER_BG_FRAME, 0x80, TRUE);
    GFL_BGSysLoadArcNCGRStatic(menuArc, 28, MONOLITH_POWER_BG_FRAME, mpw->barChar, 0x80, FALSE, HEAPID_MONOLITH);
    AppMenuCommon_LoadBarScreen(menuArc, MONOLITH_POWER_BG_FRAME, HEAPID_MONOLITH, mpw->barChar, 12);
    GFL_ArcToolFree(menuArc);
    GFL_BGSysLoadScr(MONOLITH_POWER_BG_FRAME);
    GFL_BGSysLoadScr(MONOLITH_POWER_BG_LIST);
}

static void MonolithPower_FreeBarChar(MonolithPowerWork *mpw) {
    GFL_BGSysFreeCharMemory(MONOLITH_POWER_BG_FRAME, mpw->barChar, 0x80);
}

static void MonolithPower_CreateWindows(MonolithPowerWork *mpw, MonolithWork *wk) {
    LoadSysMsgBoxBGChar(MONOLITH_POWER_BG_TEXT, 1, 0, HEAPID_MONOLITH);
    PaletteFade_LoadNCLREx(wk->fade, ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), HEAPID_MONOLITH,
                           PALFADE_BUFFER_SUB_BG, 0x20, 11 * 0x10, 0);
    mpw->windows[0] = BmpWin_CreateDynamic(MONOLITH_POWER_BG_TEXT, 1, 19, 30, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(mpw->windows[0]), 0xff);
    mpw->windows[1] = BmpWin_CreateDynamic(MONOLITH_POWER_BG_TEXT, 0, 0, 32, 3, 13, TRUE);
    BmpWin_FlushMap(mpw->windows[1]);
    GFL_BGSysLoadScr(MONOLITH_POWER_BG_TEXT);
    PrintWindow_Init(&mpw->titlePrint, mpw->windows[1]);
}

static void MonolithPower_FreeWindows(MonolithPowerWork *mpw) {
    int i;

    for (i = 0; i < 2; i++) {
        BmpWin_Free(mpw->windows[i]);
    }
}

static void MonolithPower_DrawTitle(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, 35);
    MonolithWork *wk = screen->work;

    PrintWindow_PrintNoColor(&mpw->titlePrint, wk->printQueue, 24, 4, str, wk->font);
    GFL_StrBufFree(str);
}

static void MonolithPower_ShowMessage(MonolithPowerWork *mpw, MonolithWork *wk, u32 msgId) {
    GFL_ASSERT(mpw->print_stream == NULL);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    mpw->msgStr = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    mpw->print_stream = func_02022268(mpw->windows[0], 0, 0, mpw->msgStr, wk->font, func_02017bcc(), wk->tcbMgr, 10,
                                      HEAPID_MONOLITH, 15);
    BmpWin_FlushMap(mpw->windows[0]);
    BmpWin_DrawFrame(mpw->windows[0], TRUE, 1, 11);
}

static void MonolithPower_ShowMessageExpanded(MonolithPowerWork *mpw, MonolithWork *wk, u32 msgId) {
    StrBuf *str;

    GFL_ASSERT(mpw->print_stream == NULL);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
    mpw->msgStr = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    GFL_WordSetFormatStrbuf(wk->wordSet, mpw->msgStr, str);
    GFL_StrBufFree(str);
    mpw->print_stream = func_02022268(mpw->windows[0], 0, 0, mpw->msgStr, wk->font, func_02017bcc(), wk->tcbMgr, 10,
                                      HEAPID_MONOLITH, 15);
    BmpWin_FlushMap(mpw->windows[0]);
    BmpWin_DrawFrame(mpw->windows[0], TRUE, 1, 11);
}

static BOOL MonolithPower_UpdateMessage(MonolithWork *wk, MonolithPowerWork *mpw) {
    BOOL done = AppPrintsysCommon_Update(&wk->printsys, mpw->print_stream);

    MonolithPower_UpdateKeyCursor(mpw, TRUE);
    return done;
}

static void MonolithPower_ClearMessage(MonolithPowerWork *mpw) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(mpw->windows[0]);

    GFL_ASSERT(mpw->print_stream != NULL);
    func_020223cc(mpw->print_stream);
    mpw->print_stream = NULL;
    GFL_StrBufFree(mpw->msgStr);
    mpw->msgStr = NULL;
    BmpWin_ClearFrame(mpw->windows[0], FALSE);
    GFL_BitmapFill(bitmap, 0xff);
    BmpWin_FlushChar(mpw->windows[0]);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
}

// Clears the list's background below its last row, when the list is shorter than the screen
static void MonolithPower_ClearUnusedRows(MonolithPowerWork *mpw) {
    int y;

    if (mpw->count - 2 < MONOLITH_POWER_ROW_COUNT) {
        y = (mpw->count - 1) * MONOLITH_POWER_ROW_HEIGHT / 8;
        GFL_BGSysFillScrArea(MONOLITH_POWER_BG_LIST, 0, 0, y, 32, 32 - y, 17);
        GFL_BGSysLoadScr(MONOLITH_POWER_BG_LIST);
    }
}

static void MonolithPower_CreateList(MonolithScreenParam *screen, MonolithPowerWork *mpw, MonolithWork *wk) {
    int count = 0;
    PassPowerLevel *levels;
    int i;
    u32 unlocked;

    GSYS_GetGameData(screen->param->gsys);
    levels = MonolithTool_GetLevels(screen);
    for (i = 0; i < MONOLITH_POWER_LIST_MAX; i++) {
        mpw->ids[i] = HIGH_LINK_POWER_NONE;
    }
    mpw->ids[count] = HIGH_LINK_POWER_NONE;
    mpw->states[count] = 1;
    mpw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
    count++;
    for (i = 0; i < PASS_POWER_COUNT; i++) {
        unlocked = CheckPassPowerUnlocked(wk->passPowerData, i, levels, screen->param->powerFlags);
        if (unlocked != PASS_POWER_LOCKED) {
            mpw->ids[count] = i;
            mpw->states[count] = unlocked;
            mpw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
            count++;
        }
    }
    mpw->ids[count] = HIGH_LINK_POWER_NONE;
    mpw->states[count] = 1;
    mpw->listBitmaps[count] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
    mpw->count = count + 1;
}

static void MonolithPower_FreeList(MonolithPowerWork *mpw) {
    int i;

    for (i = 0; i < mpw->count; i++) {
        GFL_BitmapFree(mpw->listBitmaps[i]);
    }
}

// Draws the names of the list into their bitmaps, at most 100 rows a frame; returns whether all are drawn
static BOOL MonolithPower_DrawList(MonolithPowerWork *mpw, MonolithWork *wk) {
    int drawn = 0;
    StrBuf *str = GFL_StrBufCreate(32, HEAPID_MONOLITH);
    int i;

    for (i = mpw->drawnCount; i < mpw->count; i++) {
        if (i >= 1 && i < mpw->count - 1) {
            GFL_MsgDataLoadStrbuf(mpw->msgPowerNames, mpw->ids[i], str);
            GFL_TextRendererDrawToBitmapEx(mpw->listBitmaps[i], 0, 0, str, wk->font,
                                           sMonolithPowerListColors[mpw->states[i]]);
        }
        if (++drawn >= 100) {
            break;
        }
    }
    GFL_StrBufFree(str);
    mpw->drawnCount = i;
    if (mpw->drawnCount >= mpw->count) {
        return TRUE;
    }
    return FALSE;
}

static void MonolithPower_CreateRowActors(MonolithPowerWork *mpw, MonolithWork *wk) {
    BmpOamActorSetup setup = { NULL, 32, 0, wk->actorRes[1].fontPalette, 0, 10, 3, 1, CLACT_VRAM_SUB };
    int i;

    for (i = 0; i < MONOLITH_POWER_ROW_COUNT; i++) {
        mpw->rowBitmaps[i] = GFL_BitmapCreate(23, 2, 0x20, HEAPID_MONOLITH);
        setup.bitmap = mpw->rowBitmaps[i];
        mpw->rowActors[i] = BmpOam_ActorAdd(wk->bmpOam, &setup);
    }
}

static void MonolithPower_DeleteRowActors(MonolithPowerWork *mpw) {
    int i;

    for (i = 0; i < MONOLITH_POWER_ROW_COUNT; i++) {
        BmpOam_ActorDel(mpw->rowActors[i]);
        GFL_BitmapFree(mpw->rowBitmaps[i]);
    }
}

static void MonolithPower_CreateReceiveButton(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    MonolithTool_CreateText(screen->work, &mpw->receiveButton, 1, MONOLITH_TEXT_NARROW, 64, 180, 37, NULL);
    if (func_0203d554() == TRUE) {
        MonolithTool_SetTextVisible(&mpw->receiveButton, FALSE);
    }
}

static void MonolithPower_DeleteReceiveButton(MonolithPowerWork *mpw) {
    MonolithTool_DeleteText(&mpw->receiveButton);
}

static void MonolithPower_UpdateReceiveButton(MonolithWork *wk, MonolithPowerWork *mpw) {
    MonolithTool_UpdateText(wk, &mpw->receiveButton);
}

static void MonolithPower_CreateReturnButton(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    MonolithTool_CreateReturnButton(screen->work, &mpw->returnButton);
}

static void MonolithPower_DeleteReturnButton(MonolithPowerWork *mpw) {
    MonolithTool_DeleteReturnButton(&mpw->returnButton);
}

static void MonolithPower_UpdateReturnButton(MonolithPowerWork *mpw) {
    MonolithTool_UpdateReturnButton(&mpw->returnButton);
}

static void MonolithPower_CreateArrows(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    mpw->arrowUp = MonolithTool_CreateActor(screen->work, 128, 28, 9);
    mpw->arrowDown = MonolithTool_CreateActor(screen->work, 128, 164, 10);
    func_0204c124(mpw->arrowUp, FALSE);
    func_0204c124(mpw->arrowDown, FALSE);
}

static void MonolithPower_DeleteArrows(MonolithPowerWork *mpw) {
    MonolithTool_DeleteActor(mpw->arrowUp);
    MonolithTool_DeleteActor(mpw->arrowDown);
}

// Shows the arrows where the list goes on
static void MonolithPower_UpdateArrows(MonolithPowerWork *mpw) {
    if (mpw->scroll <= (8 << 8)) {
        func_0204c124(mpw->arrowUp, FALSE);
    } else {
        func_0204c124(mpw->arrowUp, TRUE);
    }
    if ((mpw->scroll >> 8) >= mpw->count * MONOLITH_POWER_ROW_HEIGHT - 200) {
        func_0204c124(mpw->arrowDown, FALSE);
    } else {
        func_0204c124(mpw->arrowDown, TRUE);
    }
    MonolithTool_UpdateActor(mpw->arrowUp);
    MonolithTool_UpdateActor(mpw->arrowDown);
}

// Moves the cursor with the keys, or scrolls the list with the stylus and picks the row tapped; returns whether a
// row was picked
static BOOL MonolithPower_Input(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    u32 x;
    u32 y;
    BOOL picked = FALSE;
    int index = MONOLITH_POWER_NONE;
    BOOL touching;
    int row;
    int rem;
    int diff;

    mpw->keyScroll = 0;
    touching = func_0203da84(&x, &y);
    if (mpw->dragFrames == 0 && (x < 24 || x > 224)) {
        touching = FALSE;
    }
    if (MonolithPower_GetHeldKeys(mpw) & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_UP | PAD_KEY_DOWN)) {
        touching = FALSE;
        row = ((mpw->scroll >> 8) + 24) / MONOLITH_POWER_ROW_HEIGHT;
        rem = ((mpw->scroll >> 8) + 24) % MONOLITH_POWER_ROW_HEIGHT;
        mpw->velocity = 0;
        mpw->dragFrames = 0;
        if (mpw->cursor == MONOLITH_POWER_NONE) {
            if (rem != 0) {
                row++;
            }
            if (row < 1) {
                row = 1;
            }
            if (row >= mpw->count - 2) {
                MonolithPower_SetCursor(mpw, mpw->count - 2, MONOLITH_POWER_CURSOR_KEYS);
            } else {
                MonolithPower_SetCursor(mpw, row, MONOLITH_POWER_CURSOR_KEYS);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return FALSE;
        }
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            MonolithPower_SetCursor(mpw, mpw->cursor, MONOLITH_POWER_CURSOR_KEYS);
            func_0203d564(touching);
            if (mpw->states[mpw->cursor] == 0) {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
            return TRUE;
        }
        if (MonolithPower_GetTypedKeys(mpw) & PAD_KEY_UP) {
            if (rem != 0) {
                row++;
            }
            if (MonolithPower_SetCursor(mpw, mpw->cursor - 1, MONOLITH_POWER_CURSOR_KEYS) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            if (mpw->cursor < row) {
                mpw->keyScroll = -(32 << 8);
            }
            return FALSE;
        }
        if (MonolithPower_GetTypedKeys(mpw) & PAD_KEY_DOWN) {
            row = ((mpw->scroll >> 8) + 168) / MONOLITH_POWER_ROW_HEIGHT;
            if (MonolithPower_SetCursor(mpw, mpw->cursor + 1, MONOLITH_POWER_CURSOR_KEYS) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_SELECT1);
            }
            if (mpw->cursor >= row) {
                mpw->keyScroll = 32 << 8;
            }
            return FALSE;
        }
    }
    if (mpw->dragFrames == 0) {
        if (touching == TRUE) {
            if (y > 24 && y < 168) {
                mpw->dragFrames = 1;
                mpw->lastY = y;
                mpw->tapY = y;
                MonolithPower_SetCursor(mpw, MONOLITH_POWER_NONE, MONOLITH_POWER_CURSOR_DRAG);
            } else if (func_0203da48() == TRUE && mpw->cursor != MONOLITH_POWER_NONE && x >= 16 && x <= 112 &&
                       y >= 172 && y <= 188) {
                picked = TRUE;
                index = mpw->cursor;
                func_0203d564(TRUE);
            }
        } else if (mpw->velocity != 0) {
            if (mpw->velocity > 0) {
                mpw->velocity -= 0xc0;
                if (mpw->velocity < 0) {
                    mpw->velocity = 0;
                }
            } else {
                mpw->velocity += 0xc0;
                if (mpw->velocity > 0) {
                    mpw->velocity = 0;
                }
            }
        }
    } else if (touching == FALSE) {
        mpw->dragFrames = 0;
        if (mpw->tapY != MONOLITH_POWER_NONE) {
            diff = mpw->lastY - mpw->tapY;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff < 4) {
                index = MonolithPower_GetRowAt(mpw, mpw->lastY);
                if (index != MONOLITH_POWER_NONE) {
                    picked = TRUE;
                    if (mpw->states[index] == 0) {
                        GFL_SndSEPlay(SEQ_SE_SELECT1);
                    } else {
                        GFL_SndSEPlay(SEQ_SE_BEEP);
                    }
                }
            }
        }
    } else {
        mpw->velocity = (mpw->lastY - y) << 8;
        mpw->lastY = y;
        mpw->dragFrames++;
        if (mpw->tapY != MONOLITH_POWER_NONE) {
            diff = y - mpw->tapY;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff > 4) {
                mpw->tapY = MONOLITH_POWER_NONE;
            }
        }
    }
    if (picked == TRUE) {
        mpw->velocity = 0;
        MonolithPower_SetCursor(mpw, index, MONOLITH_POWER_CURSOR_TAP);
    }
    return picked;
}

// Moves the cursor to a row of a pass power, or hides it; returns whether it moved
static BOOL MonolithPower_SetCursor(MonolithPowerWork *mpw, int index, u32 by) {
    if (index != MONOLITH_POWER_NONE && (index >= mpw->count - 1 || index < 1)) {
        return FALSE;
    }
    mpw->prevCursor = mpw->cursor;
    mpw->prevCursor2 = mpw->cursor;
    mpw->cursor = index;
    mpw->cursorChanged = TRUE;
    mpw->cursorBy = by;
    return TRUE;
}

// Sets the palette of a row's background, if the row is on the screen
static void MonolithPower_SetRowPalette(MonolithPowerWork *mpw, int index, u8 palette) {
    int scroll;
    int top;
    int rel;

    if (index != MONOLITH_POWER_NONE) {
        scroll = mpw->scroll >> 8;
        top = scroll / MONOLITH_POWER_ROW_HEIGHT;
        rel = index - top;
        if (rel >= 0 && rel < top + MONOLITH_POWER_ROW_COUNT) {
            GFL_BGSysSetScrPaletteNo(MONOLITH_POWER_BG_LIST, 3,
                                     (rel * MONOLITH_POWER_ROW_HEIGHT + scroll % 64 - scroll % 32) % 256 / 8, 25, 4,
                                     palette);
        }
    }
}

static void MonolithPower_HighlightRow(MonolithPowerWork *mpw, int index) {
    MonolithPower_SetRowPalette(mpw, index, 3);
}

static void MonolithPower_UnhighlightRow(MonolithPowerWork *mpw, int index) {
    MonolithPower_SetRowPalette(mpw, index, 0);
}

static void MonolithPower_UnhighlightPrevRow(MonolithPowerWork *mpw) {
    if (mpw->cursorChanged == TRUE) {
        MonolithPower_UnhighlightRow(mpw, mpw->prevCursor);
    }
}

// Shows the cursor's row, and picks it when it is chosen a second time
static void MonolithPower_UpdateCursor(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    if (mpw->cursorChanged == TRUE) {
        MonolithPower_HighlightRow(mpw, mpw->cursor);
        if (mpw->cursor == MONOLITH_POWER_NONE) {
            MonolithTool_SetPanelPulse(screen, FALSE, 1);
        } else if ((mpw->cursorBy == MONOLITH_POWER_CURSOR_TAP && mpw->cursor == mpw->prevCursor2) ||
                   (mpw->cursorBy == MONOLITH_POWER_CURSOR_KEYS && mpw->cursor == mpw->prevCursor)) {
            if (mpw->states[mpw->cursor] == 0) {
                mpw->selected = mpw->cursor;
                if (mpw->cursorBy == MONOLITH_POWER_CURSOR_KEYS) {
                    MonolithTool_FlashPanel(screen, 1);
                } else {
                    MonolithTool_SetTextPicked(screen, &mpw->receiveButton, 1, 0, 3);
                }
            } else if (mpw->cursorBy == MONOLITH_POWER_CURSOR_TAP) {
                MonolithTool_SetPanelPulse(screen, TRUE, 1);
            }
        } else {
            MonolithTool_SetPanelPulse(screen, TRUE, 1);
        }
        GFL_BGSysQueueScrLoad(MONOLITH_POWER_BG_LIST);
        mpw->cursorChanged = FALSE;
    }
}

// Scrolls the list, between its top and its bottom; returns whether it moved a pixel
static BOOL MonolithPower_Scroll(MonolithPowerWork *mpw, s32 delta) {
    s32 old = mpw->scroll;
    int height = mpw->count * MONOLITH_POWER_ROW_HEIGHT - 8;

    if (height <= 192) {
        return FALSE;
    }
    mpw->scroll = old + delta;
    if (mpw->scroll < (8 << 8)) {
        mpw->scroll = 8 << 8;
    } else if (mpw->scroll > (height - 192) << 8) {
        mpw->scroll = (height - 192) << 8;
    }
    if ((mpw->scroll >> 8) != (old >> 8)) {
        return TRUE;
    }
    return FALSE;
}

static void MonolithPower_MoveBG(MonolithPowerWork *mpw) {
    GFL_BGSysMoveBGReq(MONOLITH_POWER_BG_LIST, BG_MOVE_SET_Y, (mpw->scroll >> 8) % 64);
}

// Puts the bitmaps of the rows on the screen into the row actors, and moves them with the scroll
static void MonolithPower_UpdateRows(MonolithPowerWork *mpw) {
    int scroll = mpw->scroll >> 8;
    int y = 8 - scroll % MONOLITH_POWER_ROW_HEIGHT;
    int first = scroll / MONOLITH_POWER_ROW_HEIGHT;
    int i;

    for (i = 0; i < MONOLITH_POWER_ROW_COUNT; i++) {
        BmpOam_ActorSetPos(mpw->rowActors[i], 32, y + i * MONOLITH_POWER_ROW_HEIGHT);
        if (mpw->listBitmaps[first + i] != NULL) {
            GFL_BitmapCopy(mpw->listBitmaps[first + i], mpw->rowBitmaps[i]);
        }
    }
}

static void MonolithPower_VBlank(TCB *tcb, void *data) {
    MonolithPowerWork *mpw = data;
    int i;

    if (mpw->rowsChanged == TRUE) {
        for (i = 0; i < MONOLITH_POWER_ROW_COUNT; i++) {
            BmpOam_ActorBmpTrans(mpw->rowActors[i]);
        }
        mpw->rowsChanged = FALSE;
    }
}

static int MonolithPower_GetRowAt(MonolithPowerWork *mpw, u32 y) {
    int row = ((mpw->scroll >> 8) + y) / MONOLITH_POWER_ROW_HEIGHT;

    if (row >= mpw->count) {
        row = MONOLITH_POWER_NONE;
    }
    return row;
}

static void MonolithPower_LoadEquipped(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    HighLinkSave *highLink = getHighLinkBlockAddress(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys)));
    int i;
    int j;
    u8 passPower;

    for (i = 0; i < 4; i++) {
        screen->state->equipped[i] = HIGH_LINK_POWER_NONE;
    }
    for (i = 0; i < 3; i++) {
        screen->state->equipped[i] = func_0200c678(highLink, i);
    }
    for (j = 0; j < 3; j++) {
        passPower = screen->state->equipped[j];
        for (i = 0; i < MONOLITH_POWER_LIST_MAX; i++) {
            if (passPower == mpw->ids[i]) {
                mpw->states[i] = 1;
                break;
            }
        }
    }
    mpw->flags = 0;
    mpw->returnSlot = -1;
    mpw->keyCursor = KeyCursor_Create(15, TRUE, TRUE, HEAPID_MONOLITH);
    MonolithTool_SetTextVisible(&mpw->receiveButton, FALSE);
}

static void MonolithPower_SaveEquipped(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    HighLinkSave *highLink = getHighLinkBlockAddress(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys)));
    int i;

    for (i = 0; i < 3; i++) {
        func_0200c668(highLink, screen->state->equipped[i], i);
    }
    KeyCursor_Free(mpw->keyCursor);
}

// Waits for the message about the pass power received, then puts it in a slot; returns 0 once done, or -1
static int MonolithPower_WaitReceived(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
    int result = -1;
    int slot;

    MonolithPower_UpdateKeyCursor(mpw, FALSE);
    if (func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
        MonolithPower_ClearMessage(mpw);
        mpw->selected = MONOLITH_POWER_NONE;
        MonolithTool_SetPanelPulse(screen, TRUE, 1);
        MonolithTool_SetTextCursor(screen, &mpw->receiveButton, 1, 0xff, 3);
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        if (mpw->flags & MONOLITH_POWER_FLAG_FULL) {
            if (mpw->returnSlot != -1) {
                MonolithPower_Equip(screen, mpw, screen->state->focusPower, mpw->returnSlot);
                screen->state->equipChanged = TRUE;
                mpw->flags &= ~MONOLITH_POWER_FLAG_FULL;
            }
            result = 0;
        } else {
            slot = MonolithPower_FindEmptySlot(screen);
            if (slot != -1) {
                MonolithPower_Equip(screen, mpw, screen->state->focusPower, slot);
                screen->state->equipChanged = TRUE;
            }
            result = 0;
        }
    }
    return result;
}

static int MonolithPower_FindPower(MonolithPowerWork *mpw, u32 passPower) {
    int i;

    for (i = 0; i < MONOLITH_POWER_LIST_MAX; i++) {
        if (passPower == mpw->ids[i]) {
            return i;
        }
    }
    return -1;
}

static void MonolithPower_RedrawRow(MonolithScreenParam *screen, MonolithPowerWork *mpw, int index) {
    StrBuf *str = GFL_StrBufCreate(32, HEAPID_MONOLITH);

    GFL_MsgDataLoadStrbuf(mpw->msgPowerNames, mpw->ids[index], str);
    GFL_TextRendererDrawToBitmapEx(mpw->listBitmaps[index], 0, 0, str, screen->work->font,
                                   sMonolithPowerRowColors[mpw->states[index]]);
    GFL_StrBufFree(str);
}

// Puts a pass power in a slot, giving back the one there
static void MonolithPower_Equip(MonolithScreenParam *screen, MonolithPowerWork *mpw, u8 passPower, int slot) {
    int index;

    if (screen->state->equipped[slot] != HIGH_LINK_POWER_NONE) {
        index = MonolithPower_FindPower(mpw, screen->state->equipped[slot]);
        if (index != -1) {
            mpw->states[index] = 0;
            MonolithPower_RedrawRow(screen, mpw, index);
        }
    }
    screen->state->equipped[slot] = passPower;
    index = MonolithPower_FindPower(mpw, passPower);
    if (index != -1) {
        mpw->states[index] = 1;
        MonolithPower_RedrawRow(screen, mpw, index);
    }
    MonolithPower_UpdateRows(mpw);
    mpw->rowsChanged = TRUE;
}

static int MonolithPower_FindEmptySlot(MonolithScreenParam *screen) {
    int i;

    for (i = 0; i < 3; i++) {
        if (screen->state->equipped[i] == HIGH_LINK_POWER_NONE) {
            return i;
        }
    }
    return -1;
}

// Darkens the bottom screen behind the menu of the pass powers to give back
static void MonolithPower_SetDim(MonolithPowerWork *mpw, BOOL dim) {
    s32 brightness = 0;

    if (dim == TRUE) {
        brightness = -10;
    }
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, 0x1c, brightness);
    if (dim == FALSE) {
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3, 15, 15);
    }
}

static void MonolithPower_UpdateKeyCursor(MonolithPowerWork *mpw, BOOL printing) {
    if (printing == FALSE) {
        KeyCursor_UpdateWait(mpw->keyCursor, mpw->windows[0]);
    } else if (mpw->print_stream != NULL) {
        KeyCursor_Update(mpw->keyCursor, mpw->print_stream, mpw->windows[0]);
    }
}

static u32 MonolithPower_GetHeldKeys(MonolithPowerWork *mpw) {
    return GCTX_HIDGetHeldKeys();
}

static u32 MonolithPower_GetTypedKeys(MonolithPowerWork *mpw) {
    return GCTX_HIDGetTypedKeys();
}

static void MonolithPower_Dummy(MonolithScreenParam *screen, MonolithPowerWork *mpw) {
}
