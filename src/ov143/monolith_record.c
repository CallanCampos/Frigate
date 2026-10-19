// The Entralink monolith's mission records on the bottom screen: the Funfest missions hosted, joined and completed,
// the most participants and the best score, with the return button. The ROM gives no name for this file;
// monolith_record.c is a guess from its title, "MISSION RECORDS"

#include "types.h"
#include "app/monolith/monolith_record.h"
#include "app/monolith/monolith_tool.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The BGs of the records' text and of their background
#define MONOLITH_RECORD_BG_TEXT 4
#define MONOLITH_RECORD_BG 6

// The title and the five records
#define MONOLITH_RECORD_WINDOW_COUNT 6

// Where the values end, from the left of their windows
#define MONOLITH_RECORD_VALUE_RIGHT 224

// How MonolithRecord_GetAlignOffset places a string
enum {
    MONOLITH_ALIGN_LEFT,
    MONOLITH_ALIGN_RIGHT,
    MONOLITH_ALIGN_CENTER,
};

enum {
    MONOLITH_RECORD_SEQ_INPUT,
    MONOLITH_RECORD_SEQ_RETURN,
};

typedef struct {
    BmpWin *windows[MONOLITH_RECORD_WINDOW_COUNT];
    PrintWindow prints[MONOLITH_RECORD_WINDOW_COUNT];
    MonolithReturnButton returnButton;
} MonolithRecordWork;

static BOOL MonolithRecord_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithRecord_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithRecord_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithRecord_CreateBGs(void);
static void MonolithRecord_ReleaseBGs(void);
static void MonolithRecord_LoadScreen(MonolithWork *wk);
static void MonolithRecord_CreateWindows(MonolithWork *wk, MonolithRecordWork *rw);
static void MonolithRecord_FreeWindows(MonolithRecordWork *rw);
static void MonolithRecord_Draw(MonolithScreenParam *screen, MonolithWork *wk, MonolithRecordWork *rw);
static void MonolithRecord_CreateReturnButton(MonolithScreenParam *screen, MonolithRecordWork *rw);
static void MonolithRecord_DeleteReturnButton(MonolithRecordWork *rw);
static void MonolithRecord_UpdateReturnButton(MonolithRecordWork *rw);
static u16 MonolithRecord_GetAlignOffset(StrBuf *str, Font *font, int align);

static const TouchRect sMonolithRecordTouchRects[] = {
    { 172, 188, 232, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The messages of the title, the records' labels and their values, which nothing reads
const u16 MonolithRecord_UnusedMsgIds[] = { 4, 12, 13, 11, 14, 15, 17, 18, 16, 19, 20, 0 };

// Reconstructed, not known from the ROM: MWCC puts the table with the touch rectangles in .rodata, as the original has
// it, only when some code refers to it, even code it never emits. Nothing calls this
static inline const u16 *MonolithRecord_GetUnusedMsgIds(void) {
    return MonolithRecord_UnusedMsgIds;
}

const GameProcFunctions MONOLITH_RECORD_PROC_FUNCTIONS = {
    MonolithRecord_Init,
    MonolithRecord_Main,
    MonolithRecord_Exit,
};

// The setups of MONOLITH_RECORD_BG_TEXT and MONOLITH_RECORD_BG. One array: two setups would be laid out ahead of the
// proc table, where the ROM has them after it
static const BGSetup sMonolithRecordBGSetups[2] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
};

static BOOL MonolithRecord_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithRecordWork *rw;
    BOOL printing;
    int i;

    switch (*state) {
    case 0:
        rw = GFL_ProcInitSubsystem(proc, sizeof(MonolithRecordWork), HEAPID_MONOLITH);
        MI_CpuFill8(rw, 0, sizeof(MonolithRecordWork));
        MonolithRecord_CreateBGs();
        MonolithRecord_LoadScreen(screen->work);
        MonolithRecord_CreateWindows(screen->work, rw);
        MonolithRecord_CreateReturnButton(screen, rw);
        MonolithRecord_Draw(screen, screen->work, rw);
        (*state)++;
        break;
    case 1:
        rw = work;
        printing = FALSE;
        for (i = 0; i < MONOLITH_RECORD_WINDOW_COUNT; i++) {
            PrintWindow_Flush(&rw->prints[i], screen->work->printQueue);
            if (PrintWindow_IsPrinted(&rw->prints[i]) == FALSE) {
                printing = TRUE;
            }
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_RECORD_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_RECORD_BG, TRUE);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithRecord_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithRecordWork *rw = work;
    int i;
    u32 pressed;

    if (screen->exiting == TRUE) {
        return TRUE;
    }
    for (i = 0; i < MONOLITH_RECORD_WINDOW_COUNT; i++) {
        PrintWindow_Flush(&rw->prints[i], screen->work->printQueue);
    }
    MonolithRecord_UpdateReturnButton(rw);
    switch (*state) {
    case MONOLITH_RECORD_SEQ_INPUT:
        pressed = GCTX_HIDGetPressedKeys();
        if (func_0203da0c(sMonolithRecordTouchRects) != TOUCH_RECT_NONE || (pressed & PAD_BUTTON_B)) {
            MonolithTool_PressReturnButton(&rw->returnButton);
            if (pressed & PAD_BUTTON_B) {
                func_0203d564(FALSE);
            } else {
                func_0203d564(TRUE);
            }
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            (*state)++;
        }
        break;
    case MONOLITH_RECORD_SEQ_RETURN:
        if (MonolithTool_IsReturnButtonBlinking(&rw->returnButton) == FALSE) {
            screen->next = MONOLITH_SCREEN_MENU;
            screen->state->menuCursor = MONOLITH_MENU_RECORDS;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithRecord_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithRecordWork *rw = work;

    if (PrintQueue_IsEmpty(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG3);
    MonolithRecord_DeleteReturnButton(rw);
    MonolithRecord_FreeWindows(rw);
    MonolithRecord_ReleaseBGs();
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithRecord_CreateBGs(void) {
    GFL_BGSysCreateBG(MONOLITH_RECORD_BG_TEXT, &sMonolithRecordBGSetups[0], 0);
    GFL_BGSysCreateBG(MONOLITH_RECORD_BG, &sMonolithRecordBGSetups[1], 0);
    GFL_BGSysFillScrArea(MONOLITH_RECORD_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_RECORD_BG, 0, 0, 0, 32, 32, 17);
}

static void MonolithRecord_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_RECORD_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_RECORD_BG, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_RECORD_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_RECORD_BG);
}

static void MonolithRecord_LoadScreen(MonolithWork *wk) {
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 5, MONOLITH_RECORD_BG, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadScr(MONOLITH_RECORD_BG);
    GFL_BGSysLoadScr(MONOLITH_RECORD_BG_TEXT);
}

static void MonolithRecord_CreateWindows(MonolithWork *wk, MonolithRecordWork *rw) {
    int i;

    rw->windows[0] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 0, 30, 3, 13, TRUE);
    BmpWin_FlushChar(rw->windows[0]);
    BmpWin_FlushMap(rw->windows[0]);
    rw->windows[1] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 5, 30, 2, 13, TRUE);
    BmpWin_FlushChar(rw->windows[1]);
    BmpWin_FlushMap(rw->windows[1]);
    rw->windows[2] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 7, 30, 2, 13, TRUE);
    BmpWin_FlushChar(rw->windows[2]);
    BmpWin_FlushMap(rw->windows[2]);
    rw->windows[3] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 9, 30, 4, 13, TRUE);
    BmpWin_FlushChar(rw->windows[3]);
    BmpWin_FlushMap(rw->windows[3]);
    rw->windows[4] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 14, 30, 2, 13, TRUE);
    BmpWin_FlushChar(rw->windows[4]);
    BmpWin_FlushMap(rw->windows[4]);
    rw->windows[5] = BmpWin_CreateDynamic(MONOLITH_RECORD_BG_TEXT, 2, 16, 30, 2, 13, TRUE);
    BmpWin_FlushChar(rw->windows[5]);
    BmpWin_FlushMap(rw->windows[5]);
    for (i = 0; i < MONOLITH_RECORD_WINDOW_COUNT; i++) {
        PrintWindow_Init(&rw->prints[i], rw->windows[i]);
    }
    GFL_BGSysLoadScr(MONOLITH_RECORD_BG_TEXT);
}

static void MonolithRecord_FreeWindows(MonolithRecordWork *rw) {
    int i;

    for (i = 0; i < MONOLITH_RECORD_WINDOW_COUNT; i++) {
        BmpWin_Free(rw->windows[i]);
    }
}

static void MonolithRecord_Draw(MonolithScreenParam *screen, MonolithWork *wk, MonolithRecordWork *rw) {
    void *records;
    StrBuf *str;
    StrBuf *expanded;
    u32 value;
    u16 width;

    MonolithTool_GetPlayerInfo(screen);
    records = FestRecords_GetFromSave(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys)));
    MonolithTool_GetLevels(screen);
    str = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    expanded = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 4, str);
    PrintWindow_PrintNoColor(&rw->prints[0], wk->printQueue, 0, 4, str, wk->font);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 12, str);
    PrintWindow_PrintNoColor(&rw->prints[1], wk->printQueue, 0, 0, str, wk->font);
    value = FestRecords_GetHosted(records);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 17, str);
    WordSetNumber(wk->wordSet, 0, value, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    width = MonolithRecord_GetAlignOffset(expanded, wk->font, MONOLITH_ALIGN_RIGHT);
    PrintWindow_PrintNoColor(&rw->prints[1], wk->printQueue, MONOLITH_RECORD_VALUE_RIGHT - width, 0, expanded,
                             wk->font);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 13, str);
    PrintWindow_PrintNoColor(&rw->prints[2], wk->printQueue, 0, 0, str, wk->font);
    value = FestRecords_GetJoined(records);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 18, str);
    WordSetNumber(wk->wordSet, 0, value, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    width = MonolithRecord_GetAlignOffset(expanded, wk->font, MONOLITH_ALIGN_RIGHT);
    PrintWindow_PrintNoColor(&rw->prints[2], wk->printQueue, MONOLITH_RECORD_VALUE_RIGHT - width, 0, expanded,
                             wk->font);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 11, str);
    PrintWindow_PrintNoColor(&rw->prints[3], wk->printQueue, 0, 0, str, wk->font);
    value = FestRecords_GetCompleted(records);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 16, str);
    WordSetNumber(wk->wordSet, 0, value, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    width = MonolithRecord_GetAlignOffset(expanded, wk->font, MONOLITH_ALIGN_RIGHT);
    PrintWindow_PrintNoColor(&rw->prints[3], wk->printQueue, MONOLITH_RECORD_VALUE_RIGHT - width, 0, expanded,
                             wk->font);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 14, str);
    PrintWindow_PrintNoColor(&rw->prints[4], wk->printQueue, 0, 0, str, wk->font);
    value = FestRecords_GetMostParticipants(records);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 19, str);
    WordSetNumber(wk->wordSet, 0, value, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    width = MonolithRecord_GetAlignOffset(expanded, wk->font, MONOLITH_ALIGN_RIGHT);
    PrintWindow_PrintNoColor(&rw->prints[4], wk->printQueue, MONOLITH_RECORD_VALUE_RIGHT - width, 0, expanded,
                             wk->font);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 15, str);
    PrintWindow_PrintNoColor(&rw->prints[5], wk->printQueue, 0, 0, str, wk->font);
    value = func_02010e94(records);
    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 20, str);
    WordSetNumber(wk->wordSet, 0, value, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    width = MonolithRecord_GetAlignOffset(expanded, wk->font, MONOLITH_ALIGN_RIGHT);
    PrintWindow_PrintNoColor(&rw->prints[5], wk->printQueue, MONOLITH_RECORD_VALUE_RIGHT - width, 0, expanded,
                             wk->font);
    GFL_StrBufFree(str);
    GFL_StrBufFree(expanded);
}

static void MonolithRecord_CreateReturnButton(MonolithScreenParam *screen, MonolithRecordWork *rw) {
    MonolithTool_CreateReturnButton(screen->work, &rw->returnButton);
}

static void MonolithRecord_DeleteReturnButton(MonolithRecordWork *rw) {
    MonolithTool_DeleteReturnButton(&rw->returnButton);
}

static void MonolithRecord_UpdateReturnButton(MonolithRecordWork *rw) {
    MonolithTool_UpdateReturnButton(&rw->returnButton);
}

// How far from the left a string's start goes for an alignment, or how far from the right
static u16 MonolithRecord_GetAlignOffset(StrBuf *str, Font *font, int align) {
    if (align == MONOLITH_ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(str, font, 0);
    }
    if (align == MONOLITH_ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(str, font, 0) / 2;
    }
    return 0;
}
