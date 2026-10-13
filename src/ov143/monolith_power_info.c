// The Entralink monolith's top screen over the pass power list: the three pass powers that are equipped, and the
// description of the one that the list's cursor is on, or what the player needs to unlock it. The ROM gives no name
// for this file; monolith_power_info.c is a guess from what it shows

#include "types.h"
#include "app/monolith/monolith_power_info.h"
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
#include "save/high_link.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The BGs of the text and of the background
#define MONOLITH_POWER_INFO_BG_TEXT 0
#define MONOLITH_POWER_INFO_BG 2

// The title, the equipped pass powers and the description
#define MONOLITH_POWER_INFO_WINDOW_COUNT 3

enum {
    MONOLITH_POWER_INFO_SEQ_MAIN,
};

typedef struct {
    BmpWin *windows[MONOLITH_POWER_INFO_WINDOW_COUNT];
    PrintWindow prints[MONOLITH_POWER_INFO_WINDOW_COUNT];
    // The pass power whose description is shown, or HIGH_LINK_POWER_NONE
    u32 shownPower;
} MonolithPowerInfoWork;

static BOOL MonolithPowerInfo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithPowerInfo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithPowerInfo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithPowerInfo_CreateBGs(void);
static void MonolithPowerInfo_ReleaseBGs(void);
static void MonolithPowerInfo_LoadGraphics(MonolithWork *wk);
static void MonolithPowerInfo_CreateWindows(MonolithWork *wk, MonolithPowerInfoWork *piw);
static void MonolithPowerInfo_FreeWindows(MonolithPowerInfoWork *piw);
static void MonolithPowerInfo_DrawTitle(MonolithWork *wk, MonolithPowerInfoWork *piw);
static void MonolithPowerInfo_DrawEquipped(MonolithScreenParam *screen, MonolithWork *wk, MonolithPowerInfoWork *piw);
static void MonolithPowerInfo_DrawDescription(MonolithScreenParam *screen, MonolithWork *wk,
                                              MonolithPowerInfoWork *piw);
static int MonolithPowerInfo_FindEquipped(MonolithScreenParam *screen, u32 passPower);

const GameProcFunctions MONOLITH_POWER_INFO_PROC_FUNCTIONS = {
    MonolithPowerInfo_Init,
    MonolithPowerInfo_Main,
    MonolithPowerInfo_Exit,
};

static const BGSetup sMonolithPowerInfoBGSetupText = {
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
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static const BGSetup sMonolithPowerInfoBGSetup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_16,
    GX_BG_SCRBASE(0x1000),
    GX_BG_CHARBASE(0x04000),
    0x8000,
    GX_BG_EXTPLTT_01,
    3,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

static BOOL MonolithPowerInfo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerInfoWork *piw;
    BOOL printing;
    int i;

    switch (*state) {
    case 0:
        piw = GFL_ProcInitSubsystem(proc, sizeof(MonolithPowerInfoWork), HEAPID_MONOLITH);
        sys_memset(piw, 0, sizeof(MonolithPowerInfoWork));
        piw->shownPower = HIGH_LINK_POWER_NONE;
        MonolithPowerInfo_CreateBGs();
        MonolithPowerInfo_LoadGraphics(screen->work);
        MonolithPowerInfo_CreateWindows(screen->work, piw);
        MonolithPowerInfo_DrawTitle(screen->work, piw);
        MonolithPowerInfo_DrawEquipped(screen, screen->work, piw);
        (*state)++;
        break;
    case 1:
        piw = work;
        printing = FALSE;
        for (i = 0; i < MONOLITH_POWER_INFO_WINDOW_COUNT; i++) {
            PrintWindow_Flush(&piw->prints[i], screen->work->printQueue);
            if (PrintWindow_IsPrinted(&piw->prints[i]) == FALSE) {
                printing = TRUE;
            }
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_POWER_INFO_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_POWER_INFO_BG, TRUE);
            GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// PrintWindow_Flush as the library's inline most likely is: it also says whether the window's text has been sent,
// which is what this file alone uses. printsys.h's returns nothing
static inline BOOL MonolithPowerInfo_FlushWindow(PrintWindow *printWindow, PrintQueue *queue) {
    if (printWindow->flushPending) {
        if (!func_02021c1c(queue, BmpWin_GetBitmap(printWindow->window))) {
            BmpWin_FlushChar(printWindow->window);
            printWindow->flushPending = FALSE;
        }
    }
    return !printWindow->flushPending;
}

static BOOL MonolithPowerInfo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerInfoWork *piw = work;
    u8 printed[MONOLITH_POWER_INFO_WINDOW_COUNT];
    int i;

    if (screen->endTop == TRUE) {
        return TRUE;
    }
    for (i = 0; i < MONOLITH_POWER_INFO_WINDOW_COUNT; i++) {
        printed[i] = MonolithPowerInfo_FlushWindow(&piw->prints[i], screen->work->printQueue);
    }
    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    if (*state == MONOLITH_POWER_INFO_SEQ_MAIN) {
        if (screen->state->equipChanged == TRUE && printed[1] == TRUE) {
            MonolithPowerInfo_DrawEquipped(screen, screen->work, piw);
            screen->state->equipChanged = FALSE;
        }
        if (screen->state->focusPower != piw->shownPower && printed[2] == TRUE) {
            MonolithPowerInfo_DrawDescription(screen, screen->work, piw);
            piw->shownPower = screen->state->focusPower;
        }
        return FALSE;
    }
    return FALSE;
}

static BOOL MonolithPowerInfo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithPowerInfoWork *piw = work;

    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG3);
    MonolithPowerInfo_FreeWindows(piw);
    MonolithPowerInfo_ReleaseBGs();
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithPowerInfo_CreateBGs(void) {
    GFL_BGSysCreateBG(MONOLITH_POWER_INFO_BG_TEXT, &sMonolithPowerInfoBGSetupText, 0);
    GFL_BGSysCreateBG(MONOLITH_POWER_INFO_BG, &sMonolithPowerInfoBGSetup, 0);
    GFL_BGSysFillScrArea(MONOLITH_POWER_INFO_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_POWER_INFO_BG, 0, 0, 0, 32, 32, 17);
}

static void MonolithPowerInfo_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_POWER_INFO_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_POWER_INFO_BG, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_POWER_INFO_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_POWER_INFO_BG);
}

static void MonolithPowerInfo_LoadGraphics(MonolithWork *wk) {
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 14, MONOLITH_POWER_INFO_BG, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadScr(MONOLITH_POWER_INFO_BG_TEXT);
    GFL_BGSysLoadScr(MONOLITH_POWER_INFO_BG);
}

static void MonolithPowerInfo_CreateWindows(MonolithWork *wk, MonolithPowerInfoWork *piw) {
    int i;

    piw->windows[0] = BmpWin_CreateDynamic(MONOLITH_POWER_INFO_BG_TEXT, 2, 0, 28, 3, 13, TRUE);
    BmpWin_FlushChar(piw->windows[0]);
    BmpWin_FlushMap(piw->windows[0]);
    piw->windows[1] = BmpWin_CreateDynamic(MONOLITH_POWER_INFO_BG_TEXT, 2, 4, 28, 8, 13, TRUE);
    BmpWin_FlushChar(piw->windows[1]);
    BmpWin_FlushMap(piw->windows[1]);
    piw->windows[2] = BmpWin_CreateDynamic(MONOLITH_POWER_INFO_BG_TEXT, 2, 15, 28, 9, 13, TRUE);
    BmpWin_FlushChar(piw->windows[2]);
    BmpWin_FlushMap(piw->windows[2]);
    for (i = 0; i < MONOLITH_POWER_INFO_WINDOW_COUNT; i++) {
        PrintWindow_Init(&piw->prints[i], piw->windows[i]);
    }
    GFL_BGSysLoadScr(MONOLITH_POWER_INFO_BG_TEXT);
}

static void MonolithPowerInfo_FreeWindows(MonolithPowerInfoWork *piw) {
    int i;

    for (i = 0; i < MONOLITH_POWER_INFO_WINDOW_COUNT; i++) {
        BmpWin_Free(piw->windows[i]);
    }
}

static void MonolithPowerInfo_DrawTitle(MonolithWork *wk, MonolithPowerInfoWork *piw) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 39);

    PrintWindow_PrintNoColor(&piw->prints[0], wk->printQueue, 0, 4, str, wk->font);
    GFL_StrBufFree(str);
}

// Draws the names of the equipped pass powers
static void MonolithPowerInfo_DrawEquipped(MonolithScreenParam *screen, MonolithWork *wk, MonolithPowerInfoWork *piw) {
    BOOL drawn = FALSE;
    StrBuf *str;
    s16 i;

    GFL_BitmapFill(BmpWin_GetBitmap(piw->windows[1]), 0);
    for (i = 0; i < 3; i++) {
        u8 passPower = screen->state->equipped[i];

        if (passPower != HIGH_LINK_POWER_NONE) {
            PassPowerData *powers = wk->passPowerData;

            str = GFL_MsgDataLoadStrbufNew(wk->msgPowerNames, powers[passPower].name);
            PrintWindow_PrintNoColor(&piw->prints[1], wk->printQueue, 0, i * 16, str, wk->font);
            GFL_StrBufFree(str);
            drawn = TRUE;
        }
    }
    if (drawn == FALSE) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 40);
        PrintWindow_PrintNoColor(&piw->prints[1], wk->printQueue, 0, 0, str, wk->font);
        GFL_StrBufFree(str);
    }
}

// Draws the description of the pass power that the cursor is on, if it can be received or is equipped, and what is
// needed to unlock it otherwise
static void MonolithPowerInfo_DrawDescription(MonolithScreenParam *screen, MonolithWork *wk,
                                              MonolithPowerInfoWork *piw) {
    u8 passPower = screen->state->focusPower;
    BOOL show = FALSE;
    StrBuf *info;
    StrBuf *format;
    StrBuf *str;
    PassPowerLevel *levels;
    u32 msgId;
    u32 level;

    GFL_BitmapFill(BmpWin_GetBitmap(piw->windows[2]), 0);
    if (passPower != HIGH_LINK_POWER_NONE) {
        if (MonolithPowerInfo_FindEquipped(screen, passPower) != -1) {
            show = TRUE;
        }
        if (screen->state->focusPowerState == 0) {
            show = TRUE;
        }
        if (show == TRUE) {
            WordSetNumber(wk->wordSet, 0, ((PassPowerData *)wk->passPowerData)[passPower].unk02, 4, 0, TRUE);
            info = GFL_MsgDataLoadStrbufNew(wk->msgPowerInfo, ((PassPowerData *)wk->passPowerData)[passPower].info);
            format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 41);
            str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
            GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
            PrintWindow_PrintNoColor(&piw->prints[2], wk->printQueue, 0, 4, info, wk->font);
            PrintWindow_PrintNoColor(&piw->prints[2], wk->printQueue, 112, 52, str, wk->font);
            GFL_StrBufFree(info);
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
        } else {
            levels = MonolithTool_GetLevels(screen);
            if (((PassPowerData *)wk->passPowerData)[passPower].level1 > levels->level1) {
                level = ((PassPowerData *)wk->passPowerData)[passPower].level1;
                msgId = 42;
            } else {
                level = ((PassPowerData *)wk->passPowerData)[passPower].level2;
                msgId = 43;
            }
            WordSetNumber(wk->wordSet, 0, level, 3, 0, TRUE);
            WordSetNumber(wk->wordSet, 1, levels->level1, 3, 0, TRUE);
            WordSetNumber(wk->wordSet, 2, levels->level2, 3, 0, TRUE);
            format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, msgId);
            str = GFL_StrBufCreate(128, HEAPID_MONOLITH);
            GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
            PrintWindow_PrintNoColor(&piw->prints[2], wk->printQueue, 0, 4, str, wk->font);
            GFL_StrBufFree(str);
            GFL_StrBufFree(format);
        }
    } else {
        BmpWin_FlushChar(piw->windows[2]);
    }
}

// Which slot a pass power is equipped in, or -1
static int MonolithPowerInfo_FindEquipped(MonolithScreenParam *screen, u32 passPower) {
    int i;

    for (i = 0; i < 3; i++) {
        if (passPower == screen->state->equipped[i]) {
            return i;
        }
    }
    return -1;
}
