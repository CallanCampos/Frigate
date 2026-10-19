// The Entralink monolith's menu on the bottom screen: its three choices, receiving a pass power, a Funfest mission and
// the records, and the return button. Some choices are locked until the story reaches them. The ROM gives no name for
// this file; monolith_top.c is a guess from its assert's `mtw` ("monolith top work")

#include "types.h"
#include "app/monolith/monolith_tool.h"
#include "app/monolith/monolith_top.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/event_work.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// The menu's BG, for its message window
#define MONOLITH_TOP_BG 5

// The menu's items, and the return button's position after them
#define MONOLITH_TOP_ITEM_COUNT 3
#define MONOLITH_TOP_RETURN MONOLITH_TOP_ITEM_COUNT
#define MONOLITH_TOP_ENABLED_ROW (MONOLITH_TOP_ITEM_COUNT + 1)

// The panel whose palette marks the cursor's item
#define MONOLITH_TOP_PANEL 3

// Which items can be picked: all of them, only the Funfest missions before the player's first visit is done
// (MONOLITH_FLAG_VISITED, monolith_main.h), or only the pass powers while the story waits for the first
enum {
    MONOLITH_TOP_MODE_ALL,
    MONOLITH_TOP_MODE_MISSION_ONLY,
    MONOLITH_TOP_MODE_POWER_ONLY,
};

// The steps of the menu's main proc
enum {
    MONOLITH_TOP_SEQ_INPUT,
    MONOLITH_TOP_SEQ_PICKED,
    MONOLITH_TOP_SEQ_WAIT,
    MONOLITH_TOP_SEQ_MESSAGE,
};

typedef struct {
    MonolithTextActor texts[MONOLITH_TOP_ITEM_COUNT];
    s8 cursor;
    // The item picked, or MONOLITH_TOP_RETURN
    s8 picked;
    u8 unk32;
    // Set while the player uses the touch screen, so that the next key press only shows the cursor
    u8 touching;
    u32 mode;
    MonolithReturnButton returnButton;
    // A message window, which nothing in the ROM prints into: MONOLITH_TOP_SEQ_MESSAGE is never reached
    BmpWin *msgWindow;
    PrintStream *print_stream;
    StrBuf *msgStr;
} MonolithTopWork;

// An item: its frame's width, its position and its text
typedef struct {
    s16 width;
    s16 x;
    s16 y;
    s16 msgId;
} MonolithTopItem;

static BOOL MonolithTop_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithTop_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithTop_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithTop_CreateTexts(MonolithScreenParam *screen, MonolithTopWork *mtw);
static void MonolithTop_DeleteTexts(MonolithTopWork *mtw);
static void MonolithTop_Update(MonolithScreenParam *screen, MonolithTopWork *mtw);
static void MonolithTop_CreateReturnButton(MonolithScreenParam *screen, MonolithTopWork *mtw);
static void MonolithTop_DeleteReturnButton(MonolithTopWork *mtw);
static void MonolithTop_UpdateReturnButton(MonolithTopWork *mtw);
static void MonolithTop_CreateBG(void);
static void MonolithTop_ReleaseBG(void);
static void MonolithTop_CreateMessageWindow(MonolithTopWork *mtw, MonolithWork *wk);
static void MonolithTop_FreeMessageWindow(MonolithTopWork *mtw);
static BOOL MonolithTop_UpdateMessage(MonolithWork *wk, MonolithTopWork *mtw);
static void MonolithTop_ClearMessage(MonolithTopWork *mtw);
static BOOL MonolithTop_Input(MonolithScreenParam *screen, MonolithTopWork *mtw);
static void MonolithTop_InitCursor(MonolithScreenParam *screen, MonolithTopWork *mtw);

// The text colors of the items that can't and can be picked
static const u16 sMonolithTopTextColors[2] = { 0x820, 0x3c40 };

// The screen that each item and the return button lead to
static const s16 sMonolithTopScreens[MONOLITH_TOP_ITEM_COUNT + 1] = {
    MONOLITH_SCREEN_MISSION,
    MONOLITH_SCREEN_PASS_POWER,
    MONOLITH_SCREEN_RECORDS,
    MONOLITH_SCREEN_EXIT,
};

static const TouchRect sMonolithTopTouchRects[] = {
    { 36, 52, 56, 200 },    { 76, 92, 56, 200 },         { 116, 132, 56, 200 },
    { 172, 188, 232, 248 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// Whether each item and the return button can be picked, a row for each MONOLITH_TOP_MODE_*. The original indexes it
// as one array; a two-dimensional one computes the offsets in another order
static const s16 sMonolithTopEnabled[] = {
    TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE,
};

static const MonolithTopItem sMonolithTopItems[] = {
    { MONOLITH_TEXT_MEDIUM, 128, 44, 2 },
    { MONOLITH_TEXT_MEDIUM, 128, 84, 1 },
    { MONOLITH_TEXT_MEDIUM, 128, 124, 3 },
    { -1, 0, 0, 0 },
};

static const BGSetup sMonolithTopBGSetup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x2000),
    GX_BG_CHARBASE(0x0c000),
    0x8000,
    GX_BG_EXTPLTT_01,
    0,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

const GameProcFunctions MONOLITH_TOP_PROC_FUNCTIONS = { MonolithTop_Init, MonolithTop_Main, MonolithTop_Exit };

static BOOL MonolithTop_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithTopWork *mtw;

    GSYS_GetGameData(screen->param->gsys);
    mtw = GFL_ProcInitSubsystem(proc, sizeof(MonolithTopWork), HEAPID_MONOLITH);
    MI_CpuFill8(mtw, 0, sizeof(MonolithTopWork));
    MonolithTop_CreateBG();
    MonolithTop_CreateMessageWindow(mtw, screen->work);
    MonolithTop_InitCursor(screen, mtw);
    MonolithTop_CreateTexts(screen, mtw);
    MonolithTop_CreateReturnButton(screen, mtw);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    return TRUE;
}

static BOOL MonolithTop_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithTopWork *mtw = work;

    GSYS_GetGameCommSystem(screen->param->gsys);
    MonolithTop_Update(screen, mtw);
    MonolithTop_UpdateReturnButton(mtw);
    if (screen->exiting == TRUE) {
        return TRUE;
    }
    switch (*state) {
    case MONOLITH_TOP_SEQ_INPUT:
        if (MonolithTop_Input(screen, mtw) == TRUE) {
            (*state)++;
        }
        break;
    case MONOLITH_TOP_SEQ_PICKED:
        if (mtw->picked != MONOLITH_TOP_RETURN) {
            MonolithTool_SetTextPicked(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            MonolithTool_PressReturnButton(&mtw->returnButton);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        }
        (*state)++;
        break;
    case MONOLITH_TOP_SEQ_WAIT:
        if (mtw->picked != MONOLITH_TOP_RETURN) {
            if (MonolithTool_GetPanelMode(screen, MONOLITH_TOP_PANEL) != PANEL_MODE_FLASH) {
                screen->next = sMonolithTopScreens[mtw->picked];
                return TRUE;
            }
        } else if (MonolithTool_IsReturnButtonBlinking(&mtw->returnButton) == FALSE) {
            screen->next = sMonolithTopScreens[mtw->picked];
            return TRUE;
        }
        break;
    case MONOLITH_TOP_SEQ_MESSAGE:
        if (MonolithTop_UpdateMessage(screen->work, mtw) == TRUE &&
            (func_0203da48() || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)))) {
            MonolithTop_ClearMessage(mtw);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            *state = MONOLITH_TOP_SEQ_INPUT;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithTop_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithTopWork *mtw = work;

    if (PrintQueue_IsEmpty(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG3);
    if (mtw->print_stream != NULL) {
        PrintStream_Delete(mtw->print_stream);
    }
    if (mtw->msgStr != NULL) {
        GFL_StrBufFree(mtw->msgStr);
    }
    MonolithTop_DeleteTexts(mtw);
    MonolithTop_DeleteReturnButton(mtw);
    MonolithTop_FreeMessageWindow(mtw);
    MonolithTop_ReleaseBG();
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithTop_CreateTexts(MonolithScreenParam *screen, MonolithTopWork *mtw) {
    int i;
    const MonolithTopItem *item = sMonolithTopItems;

    MonolithTool_InitPanels(screen);
    for (i = 0; i < MONOLITH_TOP_ITEM_COUNT; i++, item++) {
        if (item->width == -1) {
            break;
        }
        MonolithTool_CreateTextCentered(
            screen->work, &mtw->texts[i], 1, item->width, item->x, item->y, item->msgId, NULL,
            sMonolithTopTextColors[sMonolithTopEnabled[mtw->mode * MONOLITH_TOP_ENABLED_ROW + i]]);
    }
    if (func_0203d554() == FALSE) {
        MonolithTool_SetTextCursor(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
        mtw->touching = FALSE;
    } else {
        mtw->touching = TRUE;
    }
}

static void MonolithTop_DeleteTexts(MonolithTopWork *mtw) {
    int i;

    for (i = 0; i < MONOLITH_TOP_ITEM_COUNT; i++) {
        MonolithTool_DeleteText(&mtw->texts[i]);
    }
}

static void MonolithTop_Update(MonolithScreenParam *screen, MonolithTopWork *mtw) {
    int i;

    MonolithTool_UpdatePanel(screen, MONOLITH_TOP_PANEL);
    for (i = 0; i < MONOLITH_TOP_ITEM_COUNT; i++) {
        MonolithTool_UpdateText(screen->work, &mtw->texts[i]);
    }
}

static void MonolithTop_CreateReturnButton(MonolithScreenParam *screen, MonolithTopWork *mtw) {
    MonolithTool_CreateReturnButton(screen->work, &mtw->returnButton);
}

static void MonolithTop_DeleteReturnButton(MonolithTopWork *mtw) {
    MonolithTool_DeleteReturnButton(&mtw->returnButton);
}

static void MonolithTop_UpdateReturnButton(MonolithTopWork *mtw) {
    MonolithTool_UpdateReturnButton(&mtw->returnButton);
}

static void MonolithTop_CreateBG(void) {
    GFL_BGSysCreateBG(MONOLITH_TOP_BG, &sMonolithTopBGSetup, 0);
    GFL_BGSysFillScrArea(MONOLITH_TOP_BG, 0, 0, 0, 32, 32, 17);
    GFL_BGSysSetBGEnabled(MONOLITH_TOP_BG, FALSE);
}

static void MonolithTop_ReleaseBG(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_TOP_BG, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_TOP_BG);
}

static void MonolithTop_CreateMessageWindow(MonolithTopWork *mtw, MonolithWork *wk) {
    LoadSysMsgBoxBGChar(MONOLITH_TOP_BG, 1, 0, HEAPID_MONOLITH);
    PaletteFade_LoadNCLREx(wk->fade, ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), HEAPID_MONOLITH,
                           PALFADE_BUFFER_SUB_BG, 0x20, 11 * 0x10, 0);
    mtw->msgWindow = BmpWin_CreateDynamic(MONOLITH_TOP_BG, 1, 19, 30, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(mtw->msgWindow), 0xff);
    BmpWin_FlushMap(mtw->msgWindow);
    BmpWin_DrawFrame(mtw->msgWindow, FALSE, 1, 11);
}

static void MonolithTop_FreeMessageWindow(MonolithTopWork *mtw) {
    BmpWin_Free(mtw->msgWindow);
}

static BOOL MonolithTop_UpdateMessage(MonolithWork *wk, MonolithTopWork *mtw) {
    return AppPrintsysCommon_Update(&wk->printsys, mtw->print_stream);
}

static void MonolithTop_ClearMessage(MonolithTopWork *mtw) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(mtw->msgWindow);

    GFL_ASSERT(mtw->print_stream != NULL);
    PrintStream_Delete(mtw->print_stream);
    mtw->print_stream = NULL;
    GFL_StrBufFree(mtw->msgStr);
    mtw->msgStr = NULL;
    GFL_BGSysSetBGEnabled(MONOLITH_TOP_BG, FALSE);
    GFL_BitmapFill(bitmap, 0xff);
    BmpWin_FlushChar(mtw->msgWindow);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
}

static BOOL MonolithTop_Input(MonolithScreenParam *screen, MonolithTopWork *mtw) {
    BOOL picked = FALSE;
    int hit = func_0203da0c(sMonolithTopTouchRects);
    u32 typed;
    int pressed;

    if (hit != TOUCH_RECT_NONE) {
        mtw->cursor = hit;
        mtw->picked = hit;
        func_0203d564(TRUE);
        mtw->touching = FALSE;
        if (sMonolithTopEnabled[mtw->mode * MONOLITH_TOP_ENABLED_ROW + hit] == TRUE) {
            picked = TRUE;
        } else {
            MonolithTool_SetTextCursor(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    } else {
        typed = GCTX_HIDGetTypedKeys();
        pressed = GCTX_HIDGetPressedKeys();
        if (pressed > 0 && mtw->touching == TRUE) {
            func_0203d564(FALSE);
            mtw->touching = FALSE;
            MonolithTool_SetTextCursor(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        } else if (pressed & PAD_BUTTON_A) {
            if (sMonolithTopEnabled[mtw->mode * MONOLITH_TOP_ENABLED_ROW + mtw->cursor] == TRUE) {
                mtw->picked = mtw->cursor;
                picked = TRUE;
            } else {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            }
        } else if (pressed & PAD_BUTTON_B) {
            mtw->picked = MONOLITH_TOP_RETURN;
            picked = TRUE;
        } else if (typed & PAD_KEY_DOWN) {
            mtw->cursor++;
            if (mtw->cursor >= MONOLITH_TOP_ITEM_COUNT) {
                mtw->cursor = 0;
            }
            MonolithTool_SetTextCursor(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        } else if (typed & PAD_KEY_UP) {
            mtw->cursor--;
            if (mtw->cursor < 0) {
                mtw->cursor = MONOLITH_TOP_ITEM_COUNT - 1;
            }
            MonolithTool_SetTextCursor(screen, mtw->texts, MONOLITH_TOP_ITEM_COUNT, mtw->cursor, MONOLITH_TOP_PANEL);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
    }
    return picked;
}

static void MonolithTop_InitCursor(MonolithScreenParam *screen, MonolithTopWork *mtw) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(screen->param->gsys));

    if (EventWork_FlagGet(eventWork, MONOLITH_FLAG_VISITED) == FALSE) {
        mtw->mode = MONOLITH_TOP_MODE_MISSION_ONLY;
    }
    if (*EventWork_GetWkPtr(eventWork, MONOLITH_SCENE_WORK) == MONOLITH_SCENE_FIRST_POWER) {
        mtw->mode = MONOLITH_TOP_MODE_POWER_ONLY;
    }
    mtw->cursor = screen->state->menuCursor;
    mtw->picked = mtw->cursor;
    screen->state->menuCursor = MONOLITH_MENU_MISSION;
}
