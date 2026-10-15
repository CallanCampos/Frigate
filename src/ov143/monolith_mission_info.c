// The Entralink monolith's top screen over the list of Funfest missions: the description of the mission that the
// list's cursor is on at its level, its time limit and what it asks for, and the player's records of it. The ROM
// gives no name for this file; monolith_mission_info.c is a guess from what it shows

#include "types.h"
#include "app/monolith/monolith_mission_info.h"
#include "app/monolith/monolith_tool.h"
#include "field/fest_mission_data.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The BGs of the text and of the background
#define MONOLITH_MISSION_INFO_BG_TEXT 0
#define MONOLITH_MISSION_INFO_BG 2

// The windows, in the order of MONOLITH_MISSION_INFO_WINDOW_*
#define MONOLITH_MISSION_INFO_WINDOW_COUNT 9

enum {
    MONOLITH_MISSION_INFO_WINDOW_TITLE,
    MONOLITH_MISSION_INFO_WINDOW_DESCRIPTION,
    MONOLITH_MISSION_INFO_WINDOW_TIME_LABEL,
    MONOLITH_MISSION_INFO_WINDOW_TARGET_LABEL,
    MONOLITH_MISSION_INFO_WINDOW_BEST_LABEL,
    MONOLITH_MISSION_INFO_WINDOW_TIME,
    MONOLITH_MISSION_INFO_WINDOW_TARGET,
    MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT_LABEL,
    MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT,
};

// No mission
#define MONOLITH_MISSION_INFO_NONE 0xff

enum {
    MONOLITH_MISSION_INFO_SEQ_MAIN,
};

typedef struct {
    BmpWin *windows[MONOLITH_MISSION_INFO_WINDOW_COUNT];
    PrintWindow prints[MONOLITH_MISSION_INFO_WINDOW_COUNT];
    FestivalText *festText;
    ArcTool *arc;
    // All the missions' data
    FestMissionData *missions;
    // The mission and level shown
    int mission;
    int level;
} MonolithMissionInfoWork;

// A window to create
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} MonolithMissionInfoWindow;

// A line of text that is drawn once, at a height; a height of -1 is drawn with the mission instead
typedef struct {
    s16 msgId;
    s16 y;
    // The message when the time limit is whole minutes
    s16 wholeMinutesMsgId;
    s16 unused;
} MonolithMissionInfoText;

static BOOL MonolithMissionInfo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithMissionInfo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MonolithMissionInfo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MonolithMissionInfo_Setup(MonolithScreenParam *screen, MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_CreateBGs(void);
static void MonolithMissionInfo_ReleaseBGs(void);
static void MonolithMissionInfo_LoadGraphics(MonolithScreenParam *screen, MonolithWork *wk);
static void MonolithMissionInfo_CreateWindows(MonolithWork *wk, MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_FreeWindows(MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_DrawAll(MonolithScreenParam *screen, MonolithWork *wk, MonolithMissionInfoWork *miw,
                                        BOOL unused);
static void MonolithMissionInfo_LoadMissions(MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_FreeMissions(MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_DrawTitle(MonolithWork *wk, MonolithMissionInfoWork *miw);
static void MonolithMissionInfo_DrawDescription(MonolithWork *wk, MonolithMissionInfoWork *miw, int mission, int level);
static void MonolithMissionInfo_DrawValue(MonolithWork *wk, MonolithMissionInfoWork *miw, int mission, int level,
                                          int window);
static void MonolithMissionInfo_DrawRecords(MonolithScreenParam *screen, MonolithMissionInfoWork *miw, int mission);
static void MonolithMissionInfo_ClearWindow(MonolithMissionInfoWork *miw, int window);
static void MonolithMissionInfo_ClearAll(MonolithMissionInfoWork *miw);

// The messages of what a mission asks for, by the kind of its target
static const s16 sMonolithMissionInfoTargetMsgs[5] = { 68, 66, 61, 31, 67 };

static const MonolithMissionInfoWindow sMonolithMissionInfoWindows[] = {
    { 0, 2, 0, 28, 3, 13 },  { 0, 2, 4, 28, 8, 13 },   { 0, 2, 15, 12, 2, 13 },
    { 0, 2, 17, 12, 2, 13 }, { 0, 2, 19, 12, 2, 13 },  { 0, 14, 15, 15, 2, 13 },
    { 0, 14, 17, 15, 2, 13 }, { 0, 14, 19, 15, 2, 13 }, { 0, 14, 21, 15, 2, 13 },
    { 0xff, 0, 0, 0, 0, 0 },
};

const GameProcFunctions MONOLITH_MISSION_INFO_PROC_FUNCTIONS = {
    MonolithMissionInfo_Init,
    MonolithMissionInfo_Main,
    MonolithMissionInfo_Exit,
};

// The setups of MONOLITH_MISSION_INFO_BG_TEXT and MONOLITH_MISSION_INFO_BG
static const BGSetup sMonolithMissionInfoBGSetups[2] = {
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x2000), GX_BG_CHARBASE(0x0c000), 0x8000,
      GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE },
    { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x1000), GX_BG_CHARBASE(0x04000), 0x8000,
      GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
};

static const MonolithMissionInfoText sMonolithMissionInfoTexts[MONOLITH_MISSION_INFO_WINDOW_COUNT] = {
    { 55, 4, 0, 0 },    { 0, -1, 0, 0 },    { 56, 0, 0, 0 },    { 57, 0, 0, 0 },    { 71, -1, 0, 0 },
    { 60, -1, 64, 0 },  { 61, -1, 0, 0 },   { 61, -1, 69, 0 },  { 61, -1, 70, 0 },
};

static BOOL MonolithMissionInfo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionInfoWork *miw;
    BOOL printing;
    int i;

    switch (*state) {
    case 0:
        miw = GFL_ProcInitSubsystem(proc, sizeof(MonolithMissionInfoWork), HEAPID_MONOLITH);
        sys_memset(miw, 0, sizeof(MonolithMissionInfoWork));
        MonolithMissionInfo_Setup(screen, miw);
        (*state)++;
        break;
    case 1:
        miw = work;
        printing = FALSE;
        for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
            PrintWindow_Flush(&miw->prints[i], screen->work->printQueue);
            if (PrintWindow_IsPrinted(&miw->prints[i]) == FALSE) {
                printing = TRUE;
            }
        }
        if (printing == FALSE) {
            GFL_BGSysSetBGEnabled(MONOLITH_MISSION_INFO_BG_TEXT, TRUE);
            GFL_BGSysSetBGEnabled(MONOLITH_MISSION_INFO_BG, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL MonolithMissionInfo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionInfoWork *miw = work;
    int i;

    if (screen->endTop == TRUE) {
        return TRUE;
    }
    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
        PrintWindow_Flush(&miw->prints[i], screen->work->printQueue);
    }
    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    if (screen->state->missionIndex != miw->mission || screen->state->missionLevel != miw->level) {
        miw->mission = screen->state->missionIndex;
        miw->level = screen->state->missionLevel;
        if (miw->mission != MONOLITH_MISSION_INFO_NONE) {
            MonolithMissionInfo_DrawAll(screen, screen->work, miw, TRUE);
        } else {
            MonolithMissionInfo_ClearAll(miw);
        }
    }
    switch (*state) {
    case MONOLITH_MISSION_INFO_SEQ_MAIN:
        return FALSE;
    }
    return FALSE;
}

static BOOL MonolithMissionInfo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MonolithScreenParam *screen = param;
    MonolithMissionInfoWork *miw = work;

    if (func_02021c0c(screen->work->printQueue) == FALSE) {
        return FALSE;
    }
    GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG3);
    MonolithMissionInfo_FreeMissions(miw);
    MonolithMissionInfo_ReleaseBGs();
    MonolithMissionInfo_FreeWindows(miw);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static void MonolithMissionInfo_Setup(MonolithScreenParam *screen, MonolithMissionInfoWork *miw) {
    miw->mission = MONOLITH_MISSION_INFO_NONE;
    miw->level = 0;
    screen->state->missionIndex = MONOLITH_MISSION_INFO_NONE;
    screen->state->missionLevel = 0;
    MonolithMissionInfo_CreateBGs();
    MonolithMissionInfo_LoadGraphics(screen, screen->work);
    MonolithMissionInfo_CreateWindows(screen->work, miw);
    MonolithMissionInfo_LoadMissions(miw);
    MonolithMissionInfo_DrawTitle(screen->work, miw);
}

static void MonolithMissionInfo_CreateBGs(void) {
    GFL_BGSysCreateBG(MONOLITH_MISSION_INFO_BG_TEXT, &sMonolithMissionInfoBGSetups[0], 0);
    GFL_BGSysCreateBG(MONOLITH_MISSION_INFO_BG, &sMonolithMissionInfoBGSetups[1], 0);
    GFL_BGSysFillScrArea(MONOLITH_MISSION_INFO_BG_TEXT, 0, 0, 0, 32, 32, 17);
    GFL_BGSysFillScrArea(MONOLITH_MISSION_INFO_BG, 0, 0, 0, 32, 32, 17);
}

static void MonolithMissionInfo_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(MONOLITH_MISSION_INFO_BG_TEXT, FALSE);
    GFL_BGSysSetBGEnabled(MONOLITH_MISSION_INFO_BG, FALSE);
    GFL_BGSysReleaseBG(MONOLITH_MISSION_INFO_BG_TEXT);
    GFL_BGSysReleaseBG(MONOLITH_MISSION_INFO_BG);
}

static void MonolithMissionInfo_LoadGraphics(MonolithScreenParam *screen, MonolithWork *wk) {
    loadBGScrToVramByFileNoReserveNegAlign(wk->arc, 15, MONOLITH_MISSION_INFO_BG, 0, 0, TRUE, HEAPID_MONOLITH);
    GFL_BGSysLoadScr(MONOLITH_MISSION_INFO_BG_TEXT);
    GFL_BGSysLoadScr(MONOLITH_MISSION_INFO_BG);
}

static void MonolithMissionInfo_CreateWindows(MonolithWork *wk, MonolithMissionInfoWork *miw) {
    const MonolithMissionInfoWindow *info = sMonolithMissionInfoWindows;
    int i;

    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++, info++) {
        if (info->bg == 0xff) {
            break;
        }
        miw->windows[i] = BmpWin_CreateDynamic(info->bg, info->x, info->y, info->width, info->height, info->palette, TRUE);
        BmpWin_FlushChar(miw->windows[i]);
        BmpWin_FlushMap(miw->windows[i]);
    }
    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
        PrintWindow_Init(&miw->prints[i], miw->windows[i]);
    }
    GFL_BGSysLoadScr(MONOLITH_MISSION_INFO_BG_TEXT);
}

static void MonolithMissionInfo_FreeWindows(MonolithMissionInfoWork *miw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
        BmpWin_Free(miw->windows[i]);
    }
}

// Draws the texts of the windows that don't depend on the mission, then those that do
static void MonolithMissionInfo_DrawAll(MonolithScreenParam *screen, MonolithWork *wk, MonolithMissionInfoWork *miw,
                                        BOOL unused) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
        if (i != 0 && sMonolithMissionInfoTexts[i].y != -1) {
            StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, sMonolithMissionInfoTexts[i].msgId);

            PrintWindow_PrintNoColor(&miw->prints[i], wk->printQueue, 0, sMonolithMissionInfoTexts[i].y, str, wk->font);
            GFL_StrBufFree(str);
        }
    }
    MonolithMissionInfo_DrawDescription(wk, miw, miw->mission, miw->level);
    MonolithMissionInfo_DrawValue(wk, miw, miw->mission, miw->level, MONOLITH_MISSION_INFO_WINDOW_TIME);
    MonolithMissionInfo_DrawValue(wk, miw, miw->mission, miw->level, MONOLITH_MISSION_INFO_WINDOW_TARGET);
    MonolithMissionInfo_DrawRecords(screen, miw, miw->mission);
}

static void MonolithMissionInfo_LoadMissions(MonolithMissionInfoWork *miw) {
    miw->festText = getTextFileForFestMissions(HEAPID_MONOLITH);
    miw->arc = GFL_ArcSysCreateFileHandle(0x121, HEAPID_MONOLITH);
    miw->missions = func_ov027_02170b50(miw->arc, HEAPID_MONOLITH);
}

static void MonolithMissionInfo_FreeMissions(MonolithMissionInfoWork *miw) {
    func_ov027_02170b00(miw->festText);
    miw->festText = NULL;
    GFL_ArcToolFree(miw->arc);
    GFL_HeapFree(miw->missions);
}

static void MonolithMissionInfo_DrawTitle(MonolithWork *wk, MonolithMissionInfoWork *miw) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, 55);

    PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_TITLE], wk->printQueue, 0, 4, str, wk->font);
    GFL_StrBufFree(str);
}

static void MonolithMissionInfo_DrawDescription(MonolithWork *wk, MonolithMissionInfoWork *miw, int mission, int level) {
    FestMission data;
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(miw->prints[MONOLITH_MISSION_INFO_WINDOW_DESCRIPTION].window), 0);
    func_ov027_02170cf8(miw->festText, mission, level, &data);
    str = GFL_StrBufCreate(256, HEAPID_MONOLITH);
    func_ov027_02170d90(miw->festText, str, &data, HEAPID_MONOLITH);
    PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_DESCRIPTION], wk->printQueue, 0, 0, str, wk->font);
    GFL_StrBufFree(str);
}

// Draws the time limit or what the mission asks for, right-aligned in a window
static void MonolithMissionInfo_DrawValue(MonolithWork *wk, MonolithMissionInfoWork *miw, int mission, int level,
                                          int window) {
    StrBuf *str;
    u16 width;
    StrBuf *format;
    PrintQueue *queue;
    u8 kind;
    u16 minutes;
    u16 seconds;

    GFL_BitmapFill(BmpWin_GetBitmap(miw->prints[window].window), 0);
    if (window == MONOLITH_MISSION_INFO_WINDOW_TIME) {
        u16 limit = miw->missions[miw->mission].params.levels[miw->level].unk0;

        seconds = limit % 60;
        minutes = limit / 60;
        WordSetNumber(wk->wordSet, 0, minutes, 2, 0, TRUE);
        WordSetNumber(wk->wordSet, 1, seconds, 2, 0, TRUE);
        str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
        if (seconds != 0) {
            format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, sMonolithMissionInfoTexts[window].msgId);
        } else {
            format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, sMonolithMissionInfoTexts[window].wholeMinutesMsgId);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
    } else if (window == MONOLITH_MISSION_INFO_WINDOW_TARGET) {
        kind = miw->missions[miw->mission].params.unk05;

        WordSetNumber(wk->wordSet, 0, miw->missions[miw->mission].params.levels[miw->level].count, 4, 0, TRUE);
        str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
        format = GFL_MsgDataLoadStrbufNew(wk->msgMonolith, sMonolithMissionInfoTargetMsgs[kind]);
        GFL_WordSetFormatStrbuf(wk->wordSet, str, format);
    }
    width = GFL_FontGetBlockWidth(str, wk->font, 0);
    queue = wk->printQueue;
    PrintWindow_PrintNoColor(&miw->prints[window], queue, 120 - width, 0, str, wk->font);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
}

// Draws the player's records of the mission, if it has been played
static void MonolithMissionInfo_DrawRecords(MonolithScreenParam *screen, MonolithMissionInfoWork *miw, int mission) {
    BOOL unplayed = FALSE;
    void *records = func_02010dec(GameData_GetSaveControl(GSYS_GetGameData(screen->param->gsys)));
    FestMission data;
    u16 best;
    u16 count;
    StrBuf *str;
    StrBuf *format;
    u32 width;
    s16 msgId;

    func_ov027_02170cf8(miw->festText, mission, 0, &data);
    best = func_02010f14(records, mission);
    if (best == 0) {
        unplayed = TRUE;
    } else {
        count = func_02010ebc(records, mission);
    }
    MonolithMissionInfo_ClearWindow(miw, MONOLITH_MISSION_INFO_WINDOW_BEST_LABEL);
    MonolithMissionInfo_ClearWindow(miw, MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT_LABEL);
    MonolithMissionInfo_ClearWindow(miw, MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT);
    if (!unplayed) {
        str = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, 71);
        PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_BEST_LABEL], screen->work->printQueue, 0, 0, str,
                                 screen->work->font);
        GFL_StrBufFree(str);
        WordSetNumber(screen->work->wordSet, 0, count, 4, 0, TRUE);
        msgId = sMonolithMissionInfoTargetMsgs[miw->missions[miw->mission].params.unk05];
        format = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, msgId);
        str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
        GFL_WordSetFormatStrbuf(screen->work->wordSet, str, format);
        GFL_StrBufFree(format);
        width = GFL_FontGetBlockWidth(str, screen->work->font, 0);
        PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT_LABEL], screen->work->printQueue,
                                 120 - width, 0, str, screen->work->font);
        GFL_StrBufFree(str);
        str = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, 69);
        PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT_LABEL], screen->work->printQueue, 0, 0,
                                 str, screen->work->font);
        GFL_StrBufFree(str);
        WordSetNumber(screen->work->wordSet, 0, best, 4, 0, TRUE);
        format = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, msgId);
        str = GFL_StrBufCreate(64, HEAPID_MONOLITH);
        GFL_WordSetFormatStrbuf(screen->work->wordSet, str, format);
        GFL_StrBufFree(format);
        width = GFL_FontGetBlockWidth(str, screen->work->font, 0);
        PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT], screen->work->printQueue, 120 - width,
                                 0, str, screen->work->font);
        GFL_StrBufFree(str);
        str = GFL_MsgDataLoadStrbufNew(screen->work->msgMonolith, 70);
        PrintWindow_PrintNoColor(&miw->prints[MONOLITH_MISSION_INFO_WINDOW_BEST_COUNT], screen->work->printQueue, 0, 0, str,
                                 screen->work->font);
        GFL_StrBufFree(str);
    }
}

static void MonolithMissionInfo_ClearWindow(MonolithMissionInfoWork *miw, int window) {
    GFL_BitmapFill(BmpWin_GetBitmap(miw->prints[window].window), 0);
    miw->prints[window].flushPending = TRUE;
}

static void MonolithMissionInfo_ClearAll(MonolithMissionInfoWork *miw) {
    int i;

    for (i = 0; i < MONOLITH_MISSION_INFO_WINDOW_COUNT; i++) {
        if (i != 0) {
            MonolithMissionInfo_ClearWindow(miw, i);
        }
    }
}
