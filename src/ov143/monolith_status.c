// The Entralink monolith's top screen under the menu and the records: the Entree's total level, its White and Black
// levels with their bar, and how many pass powers can be received. The ROM gives no name for this file;
// monolith_status.c is a guess from what it shows

#include "types.h"
#include "app/monolith/monolith_status.h"
#include "app/monolith/monolith_tool.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The BGs of the text and of the background
#define MONOLITH_STATUS_BG_TEXT 0
#define MONOLITH_STATUS_BG 2

// The title, the total level, the White and Black levels and the pass powers
#define MONOLITH_STATUS_WINDOW_COUNT 4

enum {
    MONOLITH_STATUS_SEQ_MAIN,
};

typedef struct {
    BmpWin *windows[MONOLITH_STATUS_WINDOW_COUNT];
    PrintWindow prints[MONOLITH_STATUS_WINDOW_COUNT];
    u8 unk30[8];
} MonolithStatusWork;

static BOOL MonolithStatus_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithStatus_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithStatus_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithStatus_CreateBGs(MonolithStatusWork *sw);
static void MonolithStatus_ReleaseBGs(MonolithStatusWork *sw);
static void MonolithStatus_LoadScreen(MonolithScreenParam *screen, MonolithWork *wk, MonolithStatusWork *sw);
static void MonolithStatus_CreateWindows(MonolithWork *wk, MonolithStatusWork *sw);
static void MonolithStatus_FreeWindows(MonolithStatusWork *sw);
static void MonolithStatus_Draw(MonolithScreenParam *screen, MonolithWork *wk, MonolithStatusWork *sw);

const GameProcFunctions MONOLITH_STATUS_PROC_FUNCTIONS = {
    MonolithStatus_Init,
    MonolithStatus_Main,
    MonolithStatus_Exit,
};

static const BGSetup sMonolithStatusBGSetupText = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x1000),
    GX_BG_CHARBASE(0x0c000),
    0x8000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sMonolithStatusBGSetup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x2000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static BOOL MonolithStatus_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithStatusWork *sw;
    BOOL printing;
    int i;

    switch (*state) {
    case 0:
        sw = GFL_ProcInitSubsystem(proc, sizeof(MonolithStatusWork), HEAPID_MONOLITH);
        MI_CpuFill8(sw, 0, sizeof(MonolithStatusWork));
        MonolithStatus_CreateBGs(sw);
        MonolithStatus_LoadScreen(screen, screen->work, sw);
        MonolithStatus_CreateWindows(screen->work, sw);
        MonolithStatus_Draw(screen, screen->work, sw);
        (*state)++;
        break;
    case 1:
        sw = work;
        printing = FALSE;
        for (i = 0; i < MONOLITH_STATUS_WINDOW_COUNT; i++) {
            PrintWindow_Flush(&sw->prints[i], screen->work->printQueue);
            if (PrintWindow_IsPrinted(&sw->prints[i]) == FALSE) {
                printing = TRUE;
            }
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_STATUS_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_STATUS_BG, TRUE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithStatus_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithStatusWork *sw = work;
    int i;

    if (screen->exiting == TRUE) {
        return TRUE;
    }
    for (i = 0; i < MONOLITH_STATUS_WINDOW_COUNT; i++) {
        PrintWindow_Flush(&sw->prints[i], screen->work->printQueue);
    }
    switch (*state) {
    case MONOLITH_STATUS_SEQ_MAIN:
        if (screen->endTop == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithStatus_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithStatusWork *sw = work;

    if (PrintQueue_IsEmpty(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG3);
    MonolithStatus_FreeWindows(sw);
    MonolithStatus_ReleaseBGs(sw);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithStatus_CreateBGs(MonolithStatusWork *sw) {
    GFL_BGSysCreateBG(MONOLITH_STATUS_BG_TEXT, &sMonolithStatusBGSetupText, 0);
    GFL_BGSysCreateBG(MONOLITH_STATUS_BG, &sMonolithStatusBGSetup, 0);
    GFL_BGSysFillScrArea(MONOLITH_STATUS_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_STATUS_BG, 0, 0, 0, 32, 32, 17);
}

static void MonolithStatus_ReleaseBGs(MonolithStatusWork *sw) {
    GFL_BGSysSetBGEnabled(MONOLITH_STATUS_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_STATUS_BG, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_STATUS_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_STATUS_BG);
}

static void MonolithStatus_LoadScreen(MonolithScreenParam *screen, MonolithWork *wk, MonolithStatusWork *sw) {
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 12, MONOLITH_STATUS_BG, 0, 0, TRUE, HEAPID_MONOLITH);
    MonolithTool_DrawLevelBar(screen, FALSE, MONOLITH_STATUS_BG);
    GFL_BGSysLoadScr(MONOLITH_STATUS_BG);
    GFL_BGSysLoadScr(MONOLITH_STATUS_BG_TEXT);
}

static void MonolithStatus_CreateWindows(MonolithWork *wk, MonolithStatusWork *sw) {
    int i;

    sw->windows[0] = BmpWin_CreateDynamic(MONOLITH_STATUS_BG_TEXT, 2, 0, 30, 3, 13, TRUE);
    BmpWin_FlushChar(sw->windows[0]);
    BmpWin_FlushMap(sw->windows[0]);
    sw->windows[1] = BmpWin_CreateDynamic(MONOLITH_STATUS_BG_TEXT, 2, 8, 30, 2, 13, TRUE);
    BmpWin_FlushChar(sw->windows[1]);
    BmpWin_FlushMap(sw->windows[1]);
    sw->windows[2] = BmpWin_CreateDynamic(MONOLITH_STATUS_BG_TEXT, 2, 11, 30, 2, 13, TRUE);
    BmpWin_FlushChar(sw->windows[2]);
    BmpWin_FlushMap(sw->windows[2]);
    sw->windows[3] = BmpWin_CreateDynamic(MONOLITH_STATUS_BG_TEXT, 2, 14, 30, 2, 13, TRUE);
    BmpWin_FlushChar(sw->windows[3]);
    BmpWin_FlushMap(sw->windows[3]);
    for (i = 0; i < MONOLITH_STATUS_WINDOW_COUNT; i++) {
        PrintWindow_Init(&sw->prints[i], sw->windows[i]);
    }
    GFL_BGSysLoadScr(MONOLITH_STATUS_BG_TEXT);
}

static void MonolithStatus_FreeWindows(MonolithStatusWork *sw) {
    int i;

    for (i = 0; i < MONOLITH_STATUS_WINDOW_COUNT; i++) {
        BmpWin_Free(sw->windows[i]);
    }
}

static void MonolithStatus_Draw(MonolithScreenParam *screen, MonolithWork *wk, MonolithStatusWork *sw) {
    BOOL visited;
    PassPowerLevel *levels;
    StrBuf *str;
    StrBuf *expanded;

    MonolithTool_GetPlayerInfo(screen);
    visited = EventWork_FlagGet(GameData_GetEventWork(GSYS_GetGameData(screen->param->gsys)), MONOLITH_FLAG_VISITED);
    levels = MonolithTool_GetLevels(screen);
    str = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    expanded = GFL_StrBufCreate(256, HEAPID_MONOLITH);

    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 0, str);
    PrintWindow_PrintNoColor(&sw->prints[0], wk->printQueue, 0, 4, str, wk->font);

    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 7, str);
    WordSetNumber(wk->wordSet, 0, levels->level2 + levels->level1, 5, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    PrintWindow_PrintNoColor(&sw->prints[1], wk->printQueue, 0, 0, expanded, wk->font);

    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 9, str);
    WordSetNumber(wk->wordSet, 0, levels->level2, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    PrintWindow_PrintNoColor(&sw->prints[2], wk->printQueue, 0, 0, expanded, wk->font);

    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 8, str);
    WordSetNumber(wk->wordSet, 0, levels->level1, 4, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    PrintWindow_PrintNoColor(&sw->prints[2], wk->printQueue, 120, 0, expanded, wk->font);

    GFL_MsgDataLoadStrbuf(wk->msgMonolith, 10, str);
    if (visited == TRUE) {
        WordSetNumber(
            wk->wordSet, 0,
            GetUnlockedPassPowerCount(wk->passPowerData, MonolithTool_GetLevels(screen), screen->param->powerFlags), 4,
            0, TRUE);
    } else {
        WordSetNumber(wk->wordSet, 0, 0, 4, 0, TRUE);
    }
    GFL_WordSetFormatStrbuf(wk->wordSet, expanded, str);
    PrintWindow_PrintNoColor(&sw->prints[3], wk->printQueue, 0, 0, expanded, wk->font);

    GFL_StrBufFree(str);
    GFL_StrBufFree(expanded);
}
