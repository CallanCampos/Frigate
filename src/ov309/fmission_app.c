#include "types.h"
#include "app/fmission_app.h"
#include "app/festival.h"
#include "app/fmission_app_gra.h"
#include "app/fmission_log_accessor_fapp.h"
#include "app/ui/print_msg.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/hw.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/fest_mission.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/title_4.h"

// The Funfest mission screen: the mission, the player's score and the ranking while it runs, and its result, rewards
// and records when it is over. The ROM has no name for this file; "fmission_app.c" is a guess from its graphic file's,
// "fmission_app_gra.c"

// The flags of the work
#define FLAG_REDRAW 0x1
#define FLAG_DRAWN 0x2
// The player is the mission's only one, so there is no ranking and no log
#define FLAG_ALONE 0x4
#define FLAG_CLEARED 0x8
#define FLAG_BONUS 0x10
#define FLAG_ITEM2 0x20
#define FLAG_TIMED 0x40
// The bag has room for the reward
#define FLAG_ITEM_ROOM 0x80
#define FLAG_NO_SCROLL 0x100
#define FLAG_TIMELINE_DONE 0x200
#define FLAG_LEVEL_UP 0x400
#define FLAG_ITEM2_ROOM 0x800
#define FLAG_NEW_COUNT 0x1000
#define FLAG_NEW_BEST 0x4000
// The parts of the result that the timeline's steps show, from step 1
#define FLAG_STEP_SHIFT 15
#define FLAG_STEP_TITLE 0x10000
#define FLAG_STEP_SCORE 0x20000
#define FLAG_STEP_GOAL 0x40000
#define FLAG_STEP_TOTAL 0x80000
#define FLAG_STEP_RANKING 0x100000
#define FLAG_STEPS 0x1f0000
#define FLAG_JINGLE 0x200000
#define FLAG_KEY_WAIT 0x400000
// The result's messages that were shown or skipped
#define FLAG_MESSAGE_RESULT 0x1000000
#define FLAG_MESSAGE_ITEM 0x2000000
#define FLAG_MESSAGE_BONUS 0x4000000
#define FLAG_MESSAGE_ITEM2 0x8000000
#define FLAG_MESSAGE_LEVEL 0x10000000
#define FLAG_MESSAGES_DONE 0x20000000

enum {
    SEQ_START = 0,
    SEQ_MAIN = 100,
    SEQ_JINGLE = 105,
    SEQ_KEY_WAIT = 110,
    SEQ_MESSAGE = 200,
    SEQ_END = 10000,
};

// What FMissionApp_PrintText prints
enum {
    TEXT_RESULT,
    TEXT_MISSION,
    TEXT_GOAL,
    TEXT_PLAYERS,
    TEXT_RANK,
    TEXT_SCORE,
    TEXT_BEST_SCORE,
    TEXT_BEST_NAME,
    TEXT_UNIT,
    TEXT_ITEM,
    TEXT_BONUS,
    TEXT_ITEM2,
};

enum {
    JINGLE_ITEM,
    JINGLE_CLEARED,
    JINGLE_FAILED,
};

// The messages of a kind of score: the unit after a number, its heading and a line of the log
typedef struct {
    s16 unk0;
    s16 unit;
    s16 log;
    s16 padding;
} FMissionAppUnitText;

typedef struct {
    s16 message;
    s16 window;
} FMissionAppLabel;

typedef void (*FMissionAppFunc)(FMissionApp *work);

static BOOL FMissionApp_Init(GameProc *proc, u32 *state, void *param, void *data);
static BOOL FMissionApp_Exit(GameProc *proc, u32 *state, void *param, void *data);
static BOOL FMissionApp_Main(GameProc *proc, u32 *state, void *param, void *data);
static void FMissionApp_InitWork(FMissionApp *work);
static void FMissionApp_Update(FMissionApp *work);
static void FMissionApp_UpdateMode(FMissionApp *work);
static void FMissionApp_FreeWork(FMissionApp *work);
static void FMissionApp_UpdatePrint(FMissionApp *work);
static void FMissionApp_ScrollLog(FMissionApp *work);
static void FMissionApp_InitMission(FMissionApp *work);
static void FMissionApp_UpdateMission(FMissionApp *work);
static void FMissionApp_UpdateMissionMain(FMissionApp *work);
static void FMissionApp_DrawMission(FMissionApp *work);
static void FMissionApp_DrawRanking(FMissionApp *work);
static void FMissionApp_CheckMission(FMissionApp *work);
static void FMissionApp_ClearRanking(FMissionApp *work);
static void FMissionApp_UpdateLog(FMissionApp *work);
static void FMissionApp_InitResult(FMissionApp *work);
static void FMissionApp_UpdateResult(FMissionApp *work);
static void FMissionApp_UpdateResultMain(FMissionApp *work);
static void FMissionApp_DrawResult(FMissionApp *work);
static void FMissionApp_DrawResultRanking(FMissionApp *work);
static void FMissionApp_PrintText(FMissionApp *work, int text, int window, int x, int y, u32 align);
static void FMissionApp_DrawTotal(FMissionApp *work, BOOL result);
static void FMissionApp_PrintLog(FMissionApp *work, int window);
static void FMissionApp_ReadLog(FMissionApp *work);
static void FMissionApp_AddLogEntry(FMissionApp *work, int index);
static void FMissionApp_MoveLogEntry(FMissionApp *work, int from, int to);
static void FMissionApp_PrintLabels(FMissionApp *work, u32 unused);
static void FMissionApp_UpdateTimeline(FMissionApp *work);
static BOOL FMissionApp_NextMessage(FMissionApp *work);
static void FMissionApp_PlayJingle(FMissionApp *work, int jingle);
static BOOL FMissionApp_IsJinglePlaying(FMissionApp *work);
static void FMissionApp_Dummy(FMissionApp *work);

static const u16 sFMissionAppJingles[] = { SEQ_ME_ITEM, SEQ_ME_MISSION_CLEAR, SEQ_ME_MISSION_FAILED };
// The frames at which the result's timeline goes a step further
static const s16 sFMissionAppTimeline[] = { 0, 60, 120, 180, 240, 241, -1 };
static const s16 sFMissionAppRankingWindows[] = { 8, 9, 10, 11, 13, 14, 15, -1 };

static const FMissionAppLabel sFMissionAppLabels[] = {
    { Title4_Text_TimeLeft, 1 },   { Title4_Text_NumberPlayers, 2 }, { Title4_Text_Objective, 3 },
    { Title4_Text_TotalScore, 4 }, { -1, -1 },
};

static const FMissionAppUnitText sFMissionAppUnitTexts[] = {
    { Title4_Text_Empty_78, Title4_Text_Empty_74, Title4_Text_SScore_5 },
    { Title4_Text_Empty_75, Title4_Text_Empty_66, Title4_Text_SScore_2 },
    { Title4_Text_Empty_65, Title4_Text_Empty_69, Title4_Text_SScore },
    { Title4_Text_Empty_76, Title4_Text_Empty_72, Title4_Text_SScore_3 },
    { Title4_Text_Empty_77, Title4_Text_Empty_73, Title4_Text_SScore_4 },
};

const GameProcFunctions FMISSION_APP_PROC_FUNCTIONS = { FMissionApp_Init, FMissionApp_Main, FMissionApp_Exit };

static FMissionAppFunc sFMissionAppUpdateFuncs[FMISSION_APP_MODE_COUNT] = { FMissionApp_UpdateMission, FMissionApp_UpdateResult };
static FMissionAppFunc sFMissionAppInitFuncs[FMISSION_APP_MODE_COUNT] = { FMissionApp_InitMission, FMissionApp_InitResult };

static BOOL FMissionApp_Init(GameProc *proc, u32 *state, void *param, void *data) {
    FestivalEventParam *eventParam = param;
    FMissionApp *work;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_FMISSION_APP, 0x30000);
    work = GFL_ProcInitSubsystem(proc, sizeof(FMissionApp), HEAPID_FMISSION_APP);
    sys_memset(work, 0, sizeof(FMissionApp));
    work->heapId = HEAPID_FMISSION_APP;
    work->param = eventParam;
    work->wifiList = GameData_GetWifiList(eventParam->gameData);
    work->graphic = FMissionAppGra_Create(0, work->heapId);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    work->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_4, work->heapId);
    work->msgData[1] = NULL;
    work->printQueue = func_02021998(work->heapId);
    work->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 32, 32);
    work->stream = NULL;
    work->message = NULL;
    AppPrintsysCommon_Init(&work->printCommon, APP_PRINTSYS_COMMON_KEYS | APP_PRINTSYS_COMMON_TOUCH);
    work->keyCursor = KeyCursor_Create(15, TRUE, TRUE, work->heapId);
    work->missionText = getTextFileForFestMissions(work->heapId);
    work->missionArc = GFL_ArcSysCreateFileHandle(ARCID_FMISSION_DATA, work->heapId);
    work->missions = func_ov027_02170b50(work->missionArc, work->heapId);
    work->best = LinkFestivalBest_Create(work->heapId);
    work->log = FMissionLogFApp_Create(work->heapId);
    LinkFestival_GetRanking(work->param->festival, &work->ranking);
    FMissionApp_InitWork(work);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 4, 8, 15, 15);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 4, 8, 15, 15);
    if (work->mode == FMISSION_APP_MODE_MISSION) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, work->heapId);
    } else {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_WHITE, 6, 1, work->heapId);
    }
    return TRUE;
}

static BOOL FMissionApp_Exit(GameProc *proc, u32 *state, void *param, void *data) {
    FMissionApp *work = data;
    HeapID heapId;
    int i;

    FMissionApp_FreeWork(work);
    GFL_HeapFree(work->positions);
    for (i = 0; i < 2; i++) {
        if (work->msgData[i] != NULL) {
            GFL_MsgDataFree(work->msgData[i]);
            work->msgData[i] = NULL;
        }
    }
    GFL_WordSetSystemFree(work->wordSet);
    func_ov027_02170b00(work->missionText);
    work->missionText = NULL;
    GFL_ArcToolFree(work->missionArc);
    GFL_HeapFree(work->missions);
    LinkFestivalBest_Free(work->best);
    FMissionLogFApp_Free(work->log);
    KeyCursor_Free(work->keyCursor);
    func_02021a18(work->printQueue);
    GFL_TCBExMgrFree(work->tcbManager);
    GFL_FontFree(work->font);
    FMissionAppGra_Free(work->graphic);
    heapId = work->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL FMissionApp_Main(GameProc *proc, u32 *state, void *param, void *data) {
    FMissionApp *work = data;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
        }
        break;
    case 1:
        if (work->seq == SEQ_END && func_02021c0c(work->printQueue) == TRUE) {
            (*state)++;
        }
        break;
    case 2:
        if (work->mode == FMISSION_APP_MODE_MISSION) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, work->heapId);
        } else {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, 6, 1, work->heapId);
        }
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    FMissionApp_Update(work);
    GFL_TCBExMgrUpdate(work->tcbManager);
    FMissionApp_UpdatePrint(work);
    FMissionAppGra_Update(work->graphic);
    FMissionAppGra_Begin3D(work->graphic);
    FMissionAppGra_End3D(work->graphic);
    func_02012be4(work->wifiList);
    return FALSE;
}

static void FMissionApp_InitWork(FMissionApp *work) {
    int i;

    work->seq = SEQ_START;
    work->mode = work->param->mode;
    work->flags = 0;
    work->scroll = 0;
    work->timer = 0;
    work->step = 0;
    work->lastSeconds = -1;
    work->lastCount = -1;
    work->lastTotal = -1;
    work->lastScore = -1;
    work->shownName = NULL;
    work->shownScore = 0;
    work->shownTrainerId = 0;
    work->logCount = 0;
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        work->logEntries[i].name = NULL;
        work->logEntries[i].score = 0;
        work->logEntries[i].trainerId = 0;
    }
    for (i = 0; i < FMISSION_APP_WINDOW_COUNT; i++) {
        work->windows[i].window = NULL;
    }
    for (i = 0; i < FMISSION_APP_UPPER_ACTOR_COUNT; i++) {
        work->upperActors[i] = NULL;
    }
    for (i = 0; i < FMISSION_APP_LOWER_ACTOR_COUNT; i++) {
        work->lowerActors[i] = NULL;
    }
    work->recordActors[0] = NULL;
    work->positions = GFL_ArcSysReadHeapNew(ARCID_FMISSION_APP, 14, work->heapId);
    FMissionAppDraw_Init(work, work->mode);
    sFMissionAppInitFuncs[work->mode](work);
    work->flags |= FLAG_REDRAW;
}

static void FMissionApp_Update(FMissionApp *work) {
    FMissionApp_UpdateMode(work);
}

static void FMissionApp_UpdateMode(FMissionApp *work) {
    if (work->mode >= 0 && work->mode < FMISSION_APP_MODE_COUNT) {
        sFMissionAppUpdateFuncs[work->mode](work);
    }
}

static void FMissionApp_FreeWork(FMissionApp *work) {
    int i;

    FMissionAppDraw_Free(work);
    if (work->shownName != NULL) {
        GFL_StrBufFree(work->shownName);
        work->shownName = NULL;
    }
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        if (work->logEntries[i].name != NULL) {
            GFL_StrBufFree(work->logEntries[i].name);
            work->logEntries[i].name = NULL;
        }
    }
}

static void FMissionApp_UpdatePrint(FMissionApp *work) {
    int i;

    func_02021a3c(work->printQueue);
    for (i = 0; i < FMISSION_APP_WINDOW_COUNT; i++) {
        if (work->windows[i].window != NULL) {
            PrintWindow_Flush(&work->windows[i], work->printQueue);
        }
    }
}

// Scrolls the sub screen's BG1, which is two screens wide and holds a line of the log on each
static void FMissionApp_ScrollLog(FMissionApp *work) {
    if (!(work->flags & FLAG_NO_SCROLL)) {
        GFL_BGSysMoveBG(5, BG_MOVE_SET_X, work->scroll >> 8);
        work->scroll += 0x80;
        if (work->scroll >= 512 << 8) {
            work->scroll -= 512 << 8;
        }
    }
}

static void FMissionApp_InitMission(FMissionApp *work) {
    if (work->ranking.count <= 1) {
        work->flags |= FLAG_ALONE;
    }
}

static void FMissionApp_UpdateMission(FMissionApp *work) {
    switch (work->seq) {
    case SEQ_START:
        work->seq = SEQ_MAIN;
        // fallthrough
    case SEQ_MAIN:
        FMissionApp_UpdateMissionMain(work);
        break;
    case SEQ_MESSAGE:
        if (!FMissionAppDraw_IsActorAnimActive(work, FMISSION_APP_ACTOR_LOWER, 0)) {
            work->seq = SEQ_END;
        }
        break;
    case SEQ_END:
        break;
    }
    FMissionApp_UpdateLog(work);
    FMissionApp_CheckMission(work);
    FMissionApp_ScrollLog(work);
    FMissionApp_Dummy(work);
}

static void FMissionApp_UpdateMissionMain(FMissionApp *work) {
    BOOL exit = FALSE;

    if (FMissionAppDraw_CheckTouch(work) == 0) {
        exit = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        exit = TRUE;
    }
    if (exit == TRUE) {
        work->seq = SEQ_MESSAGE;
        FMissionAppDraw_SetActorAnim(work, FMISSION_APP_ACTOR_LOWER, 0, 9);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
    if (work->flags & FLAG_REDRAW) {
        FMissionApp_DrawMission(work);
        FMissionApp_DrawRanking(work);
        work->flags |= FLAG_DRAWN;
        work->flags &= ~FLAG_REDRAW;
    }
}

static void FMissionApp_DrawMission(FMissionApp *work) {
    if (!(work->flags & FLAG_DRAWN)) {
        FMissionApp_PrintText(work, TEXT_MISSION, 0, 0, 4, PRINT_ALIGN_LEFT);
        FMissionAppDraw_SetLevelIcon(work, 2);
        FMissionApp_PrintLabels(work, 0);
        FMissionApp_PrintText(work, TEXT_GOAL, 6, 48, 0, PRINT_ALIGN_RIGHT);
        FMissionApp_PrintText(work, TEXT_UNIT, 7, 48, 0, PRINT_ALIGN_RIGHT);
    }
    FMissionAppDraw_ClearWindow(work, 5);
    FMissionApp_PrintText(work, TEXT_PLAYERS, 5, 48, 0, PRINT_ALIGN_RIGHT);
    FMissionApp_DrawTotal(work, FALSE);
}

static void FMissionApp_DrawRanking(FMissionApp *work) {
    StrBuf *strbuf;

    if (!(work->flags & FLAG_DRAWN)) {
        FMissionApp_PrintText(work, TEXT_RESULT, 12, 0, 0, PRINT_ALIGN_LEFT);
    }
    FMissionApp_ClearRanking(work);
    if (!(work->flags & FLAG_ALONE)) {
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Score);
        FMissionAppDraw_Print(work, 8, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_SCORE, 13, 64, 0, PRINT_ALIGN_RIGHT);
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Rank);
        FMissionAppDraw_Print(work, 9, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_RANK, 14, 152, 0, PRINT_ALIGN_RIGHT);
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_TopScore);
        FMissionAppDraw_Print(work, 10, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_BEST_NAME, 11, 0, 0, PRINT_ALIGN_LEFT);
        FMissionApp_PrintText(work, TEXT_BEST_SCORE, 15, 64, 0, PRINT_ALIGN_RIGHT);
    }
}

// Asks for a redraw when the mission's players, total, the player's score or the best score changed, and shows the
// time left
static void FMissionApp_CheckMission(FMissionApp *work) {
    int count;
    s16 total;
    s16 score;

    if (work->seq == SEQ_END) {
        return;
    }
    LinkFestival_GetRanking(work->param->festival, &work->ranking);
    if (work->lastSeconds != 0) {
        u16 seconds = func_020145dc(work->param->festival);

        if (work->lastSeconds != seconds) {
            s16 i;

            FMissionAppDraw_SetTime(work, FMISSION_APP_ACTOR_UPPER, 3, (s16)(seconds / 60), (s16)(seconds % 60));
            if (seconds < 10) {
                for (i = 0; i < 5; i++) {
                    FMissionAppDraw_SetActorPalette(work, FMISSION_APP_ACTOR_UPPER, i + 3, 1);
                }
                GFL_SndSEPlay(SEQ_SE_DECIDE5);
            }
            work->lastSeconds = seconds;
        }
    }
    count = work->ranking.count;
    if (work->lastCount != count) {
        if (count <= 1) {
            work->flags |= FLAG_ALONE;
            work->flags |= FLAG_NO_SCROLL;
            work->scroll = 0;
            FMissionAppDraw_ClearWindow(work, 16);
            FMissionAppDraw_ClearWindow(work, 17);
        } else {
            work->flags &= ~FLAG_ALONE;
            work->flags &= ~FLAG_NO_SCROLL;
        }
        work->flags |= FLAG_REDRAW;
        work->lastCount = count;
    }
    total = work->ranking.total;
    if (work->lastTotal != total) {
        work->flags |= FLAG_REDRAW;
        work->lastTotal = total;
    }
    score = work->ranking.score;
    if (work->lastScore != score) {
        work->flags |= FLAG_REDRAW;
        work->lastScore = score;
    }
    if (func_02014a4c(work->param->festival) == TRUE) {
        work->flags |= FLAG_REDRAW;
    }
}

static void FMissionApp_ClearRanking(FMissionApp *work) {
    int i;

    for (i = 0; i < 32; i++) {
        if (sFMissionAppRankingWindows[i] == -1) {
            break;
        }
        FMissionAppDraw_ClearWindow(work, sFMissionAppRankingWindows[i]);
    }
}

// Reads the log, and prints its next line into the half of the scrolling BG that has just left the screen
static void FMissionApp_UpdateLog(FMissionApp *work) {
    u32 halves = 0;

    if (work->seq == SEQ_END) {
        return;
    }
    FMissionApp_ReadLog(work);
    if (work->flags & FLAG_ALONE) {
        return;
    }
    if (work->scroll != 0 && work->scroll != 256 << 8) {
        return;
    }
    if (work->scroll == 0) {
        halves |= 1;
    }
    if (work->scroll == 256 << 8) {
        halves |= 2;
    }
    if (halves & 2) {
        FMissionAppDraw_ClearWindow(work, 16);
        FMissionApp_PrintLog(work, 16);
    }
    if (halves & 1) {
        FMissionAppDraw_ClearWindow(work, 17);
        FMissionApp_PrintLog(work, 17);
    }
}

static void FMissionApp_InitResult(FMissionApp *work) {
    func_02010dec(GameData_GetSaveControl(work->param->gameData));
    FMissionAppDraw_SetActorVisible(work, FMISSION_APP_ACTOR_LOWER, 0, FALSE);
    if (work->ranking.count <= 1) {
        work->flags |= FLAG_ALONE;
    }
    FMissionLogFApp_EndMission(work->param->festival, &work->result);
    if (work->result.cleared == TRUE) {
        work->flags |= FLAG_CLEARED;
        if (work->result.itemCount != 0) {
            if (FMissionAppDraw_GetItemRoom(work, work->result.item) > 0) {
                work->flags |= FLAG_ITEM_ROOM;
            }
        }
    }
    if (work->result.bonusCount != 0) {
        work->flags |= FLAG_BONUS;
    }
    if (work->result.item2 != 0) {
        work->flags |= FLAG_ITEM2;
        if (work->result.item2Count != 0) {
            if (FMissionAppDraw_GetItemRoom(work, work->result.item2) > 0) {
                work->flags |= FLAG_ITEM2_ROOM;
            }
        }
    }
    if (work->param->missionConfig->info.unk4_31 == TRUE) {
        work->flags |= FLAG_TIMED;
    }
    if (work->result.newCount == TRUE) {
        work->flags |= FLAG_NEW_COUNT;
    }
    if (work->result.levels != 0) {
        work->flags |= FLAG_LEVEL_UP;
    }
    if (work->result.newBest == TRUE) {
        work->flags |= FLAG_NEW_BEST;
    }
}

static void FMissionApp_UpdateResult(FMissionApp *work) {
    switch (work->seq) {
    case SEQ_START:
        work->seq = SEQ_MAIN;
        // fallthrough
    case SEQ_MAIN:
        FMissionApp_UpdateResultMain(work);
        break;
    case SEQ_JINGLE:
        if (!(work->flags & FLAG_JINGLE)) {
            break;
        }
        if (FMissionApp_IsJinglePlaying(work)) {
            break;
        }
        work->flags &= ~FLAG_JINGLE;
        work->flags |= FLAG_KEY_WAIT;
        work->seq = SEQ_KEY_WAIT;
        break;
    case SEQ_KEY_WAIT:
        if (work->flags & FLAG_MESSAGES_DONE) {
            work->seq = SEQ_MAIN;
            work->flags |= FLAG_KEY_WAIT;
            break;
        }
        if (work->flags & FLAG_KEY_WAIT) {
            FMissionAppDraw_SetKeyCursor(work, TRUE);
        }
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            work->flags &= ~FLAG_KEY_WAIT;
            FMissionApp_NextMessage(work);
            work->seq = SEQ_MESSAGE;
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
        }
        break;
    case SEQ_MESSAGE:
        if (FMissionAppDraw_UpdateMessage(work)) {
            break;
        }
        if (work->flags & FLAG_MESSAGES_DONE) {
            if (work->flags & FLAG_JINGLE) {
                if (FMissionApp_IsJinglePlaying(work)) {
                    break;
                }
                work->flags &= ~FLAG_JINGLE;
                work->flags |= FLAG_KEY_WAIT;
                work->seq = SEQ_MAIN;
            } else {
                work->flags |= FLAG_KEY_WAIT;
                work->seq = SEQ_MAIN;
            }
            break;
        }
        if (work->flags & FLAG_JINGLE) {
            if (FMissionApp_IsJinglePlaying(work)) {
                break;
            }
            work->flags &= ~FLAG_JINGLE;
            work->flags |= FLAG_KEY_WAIT;
            break;
        }
        if (work->flags & FLAG_KEY_WAIT) {
            FMissionAppDraw_SetKeyCursor(work, TRUE);
        }
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            work->flags &= ~FLAG_KEY_WAIT;
            FMissionApp_NextMessage(work);
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
        }
        break;
    case SEQ_END:
        break;
    }
    FMissionApp_UpdateTimeline(work);
    FMissionApp_Dummy(work);
}

static void FMissionApp_UpdateResultMain(FMissionApp *work) {
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) && (work->flags & FLAG_MESSAGES_DONE)) {
        work->seq = SEQ_END;
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
    }
    if (work->flags & FLAG_REDRAW) {
        FMissionApp_DrawResult(work);
        FMissionApp_DrawResultRanking(work);
        work->flags &= ~FLAG_REDRAW;
        work->flags &= ~FLAG_STEPS;
    }
    if ((work->flags & FLAG_MESSAGES_DONE) && (work->flags & FLAG_KEY_WAIT)) {
        FMissionAppDraw_SetKeyCursor(work, TRUE);
    }
}

// Draws the parts of the result that the timeline has reached: the mission, the players, the goal and the total
static void FMissionApp_DrawResult(FMissionApp *work) {
    BOOL record = FALSE;
    StrBuf *strbuf;

    if (work->flags & FLAG_STEP_TITLE) {
        FMissionApp_PrintText(work, TEXT_MISSION, 0, 0, 4, PRINT_ALIGN_LEFT);
        FMissionAppDraw_SetLevelIcon(work, 1);
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_NumberPlayers);
        FMissionAppDraw_Print(work, 1, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionAppDraw_SetActorVisible(work, FMISSION_APP_ACTOR_UPPER, 0, TRUE);
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Objective);
        FMissionAppDraw_Print(work, 2, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_TotalScore);
        FMissionAppDraw_Print(work, 3, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        if (work->flags & FLAG_TIMED) {
            strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_TimeLeft_2);
            FMissionAppDraw_Print(work, 4, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
            GFL_StrBufFree(strbuf);
        }
    }
    if (work->flags & FLAG_STEP_SCORE) {
        FMissionApp_PrintText(work, TEXT_PLAYERS, 5, 48, 0, PRINT_ALIGN_RIGHT);
        GFL_SndSEPlay(SEQ_SE_SW_FES_06);
    }
    if (work->flags & FLAG_STEP_GOAL) {
        FMissionApp_PrintText(work, TEXT_GOAL, 6, 48, 0, PRINT_ALIGN_RIGHT);
        GFL_SndSEPlay(SEQ_SE_SW_FES_06);
    }
    if (work->flags & FLAG_STEP_TOTAL) {
        FMissionApp_DrawTotal(work, TRUE);
        strbuf = GFL_MsgDataLoadStrbufNew(
            work->msgData[0],
            sFMissionAppUnitTexts[work->missions[work->param->missionConfig->info.index].params.unk05].unit);
        FMissionAppDraw_Print(work, 7, 48, 0, strbuf, PRINT_ALIGN_RIGHT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        if (work->flags & FLAG_TIMED) {
            u16 seconds = work->result.seconds;

            FMissionAppDraw_SetTime(work, FMISSION_APP_ACTOR_UPPER, 2, seconds / 60, seconds % 60);
        }
        if (work->flags & FLAG_NEW_COUNT) {
            record = TRUE;
            FMissionAppDraw_SetActorVisible(work, FMISSION_APP_ACTOR_UPPER, 11, TRUE);
        }
        if (!record) {
            GFL_SndSEPlay(SEQ_SE_SW_FES_06);
        } else {
            GFL_SndSEPlay(SEQ_SE_SW_FES_07);
        }
    }
}

// Draws the last part of the result: the player's score and rank, the best score and the rewards
static void FMissionApp_DrawResultRanking(FMissionApp *work) {
    StrBuf *strbuf;

    if (!(work->flags & FLAG_STEP_RANKING)) {
        return;
    }
    strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Score);
    FMissionAppDraw_Print(work, 8, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
    GFL_StrBufFree(strbuf);
    FMissionApp_PrintText(work, TEXT_SCORE, 15, 64, 0, PRINT_ALIGN_RIGHT);
    if (work->flags & FLAG_NEW_BEST) {
        FMissionAppDraw_SetActorVisible(work, FMISSION_APP_ACTOR_RECORD, 0, TRUE);
    }
    strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Rank);
    FMissionAppDraw_Print(work, 9, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
    GFL_StrBufFree(strbuf);
    FMissionApp_PrintText(work, TEXT_RANK, 16, 152, 0, PRINT_ALIGN_RIGHT);
    if (!(work->flags & FLAG_ALONE)) {
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_TopScore);
        FMissionAppDraw_Print(work, 10, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_BEST_NAME, 11, 0, 0, PRINT_ALIGN_LEFT);
        FMissionApp_PrintText(work, TEXT_BEST_SCORE, 17, 64, 0, PRINT_ALIGN_RIGHT);
    }
    if (work->flags & FLAG_CLEARED) {
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Reward);
        FMissionAppDraw_Print(work, 12, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_ITEM, 18, 128, 0, PRINT_ALIGN_RIGHT);
    }
    if (work->flags & FLAG_BONUS) {
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_BonusScore);
        FMissionAppDraw_Print(work, 13, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_BONUS, 19, 48, 0, PRINT_ALIGN_RIGHT);
    }
    if (work->flags & FLAG_ITEM2) {
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_DoubleScore);
        FMissionAppDraw_Print(work, 14, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
        FMissionApp_PrintText(work, TEXT_ITEM2, 20, 128, 0, PRINT_ALIGN_RIGHT);
    }
}

// Prints one of the screen's texts into a window
static void FMissionApp_PrintText(FMissionApp *work, int text, int window, int x, int y, u32 align) {
    // Nothing reads this pair of messages
    static const s16 unused[] = { Title4_Text_Empty_70, Title4_Text_Empty_79 };
    StrBuf *strbuf;
    StrBuf *format;
    u32 index = work->param->missionConfig->info.index;
    u32 level = work->param->missionConfig->info.level;

    if (text == TEXT_RESULT) {
        strbuf = GFL_StrBufCreate(256, work->heapId);
        func_ov027_02170d90(work->missionText, strbuf, (const FestMission *)work->param->missionConfig, work->heapId);
    } else if (text == TEXT_MISSION) {
        strbuf = GFL_StrBufCreate(128, work->heapId);
        func_ov027_02170d04(work->missionText, strbuf, (const FestMission *)work->param->missionConfig, work->heapId);
    } else if (text == TEXT_GOAL) {
        u32 unit = work->missions[index].params.unk05;

        WordSetNumber(work->wordSet, 0, work->missions[index].params.levels[level].count, 4, 0, TRUE);
        strbuf = GFL_StrBufCreate(64, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppUnitTexts[unit].unit);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_PLAYERS) {
        WordSetNumber(work->wordSet, 0, work->ranking.count, 3, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Empty_66);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_RANK) {
        u32 rank = work->ranking.rank;

        WordSetNumber(work->wordSet, 0, work->ranking.count, 3, 0, TRUE);
        WordSetNumber(work->wordSet, 1, rank, 3, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Of);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_SCORE) {
        u32 unit = work->missions[index].params.unk05;

        WordSetNumber(work->wordSet, 0, work->ranking.score, 4, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppUnitTexts[unit].unit);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_BEST_SCORE) {
        u32 unit;

        func_02014a78(work->param->festival, work->best);
        if (work->best->name == NULL) {
            return;
        }
        unit = work->missions[index].params.unk05;
        WordSetNumber(work->wordSet, 0, work->best->score, 4, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppUnitTexts[unit].unit);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_BEST_NAME) {
        func_02014a78(work->param->festival, work->best);
        if (work->best->name == NULL) {
            return;
        }
        FMissionAppDraw_Print(work, window, x, y, work->best->name, align, FMISSION_APP_COLOR_TEXT);
        return;
    } else if (text == TEXT_UNIT) {
        strbuf =
            GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppUnitTexts[work->missions[index].params.unk05].unit);
    } else if (text == TEXT_ITEM) {
        u32 count = work->result.itemCount;
        BOOL plural = TRUE;

        if (count == 1) {
            plural = FALSE;
        }
        loadItemText(work->wordSet, 0, work->result.item, plural, FALSE);
        WordSetNumber(work->wordSet, 1, count, 2, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Empty_70);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_BONUS) {
        WordSetNumber(work->wordSet, 0, work->result.bonusCount, 2, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Empty_71);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    } else if (text == TEXT_ITEM2) {
        u32 count = work->result.item2Count;
        BOOL plural = TRUE;

        if (count == 1) {
            plural = FALSE;
        }
        loadItemText(work->wordSet, 0, work->result.item2, plural, FALSE);
        WordSetNumber(work->wordSet, 1, count, 2, 0, TRUE);
        strbuf = GFL_StrBufCreate(128, work->heapId);
        format = GFL_MsgDataLoadStrbufNew(work->msgData[0], Title4_Text_Empty_70);
        GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
        GFL_StrBufFree(format);
    }
    FMissionAppDraw_Print(work, window, x, y, strbuf, align, FMISSION_APP_COLOR_TEXT);
    GFL_StrBufFree(strbuf);
}

// Shows the total with digit actors, left of where its unit's text starts
static void FMissionApp_DrawTotal(FMissionApp *work, BOOL result) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(
        work->msgData[0],
        sFMissionAppUnitTexts[work->missions[work->param->missionConfig->info.index].params.unk05].unit);
    int offset = 24 - (u16)GFL_FontGetBlockWidth(strbuf, work->font, 0);
    int x;
    int y;

    GFL_StrBufFree(strbuf);
    if (!result) {
        x = FMissionAppDraw_GetPos(work, 8);
        y = FMissionAppDraw_GetPos(work, 9);
        FMissionAppDraw_SetNumber(work, FMISSION_APP_ACTOR_UPPER, 8, work->ranking.total, 4, FALSE, x + offset, y);
    } else {
        x = FMissionAppDraw_GetPos(work, 14);
        y = FMissionAppDraw_GetPos(work, 15);
        FMissionAppDraw_SetNumber(work, FMISSION_APP_ACTOR_UPPER, 7, work->result.total, 4, FALSE, x + offset, y);
    }
}

// Prints the log's line after the one that was shown last
static void FMissionApp_PrintLog(FMissionApp *work, int window) {
    u32 unit = work->missions[work->param->missionConfig->info.index].params.unk05;
    int i;
    int shown;
    int next;
    StrBuf *format;
    StrBuf *strbuf;

    if (work->logCount == 0) {
        return;
    }
    if (work->shownName == NULL) {
        work->shownName = GFL_StrBufClone(work->logEntries[0].name, work->heapId);
        work->shownScore = work->logEntries[0].score;
        work->shownTrainerId = work->logEntries[0].trainerId;
    } else {
        shown = -1;
        for (i = 0; i < work->logCount; i++) {
            if (work->shownTrainerId == work->logEntries[i].trainerId) {
                shown = i;
                break;
            }
        }
        next = shown + 1;
        if (next >= work->logCount) {
            next = 0;
        }
        GFL_StrBufFree(work->shownName);
        work->shownName = GFL_StrBufClone(work->logEntries[next].name, work->heapId);
        work->shownScore = work->logEntries[next].score;
        work->shownTrainerId = work->logEntries[next].trainerId;
    }
    format = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppUnitTexts[unit].log);
    func_0202437c(work->wordSet, 0, work->shownName, 0, 1, 2);
    WordSetNumber(work->wordSet, 1, work->shownScore, 4, 0, TRUE);
    strbuf = GFL_StrBufCreate(64, work->heapId);
    GFL_WordSetFormatStrbuf(work->wordSet, strbuf, format);
    FMissionAppDraw_Print(work, window, 0, 0, strbuf, PRINT_ALIGN_LEFT, FMISSION_APP_COLOR_TEXT);
    GFL_StrBufFree(format);
    GFL_StrBufFree(strbuf);
}

// Takes the log's last lines from the festival, the oldest first so that the newest ends up in front
static void FMissionApp_ReadLog(FMissionApp *work) {
    int i;
    int count;

    FMissionLogFApp_Read(func_020146fc(work->param->festival), work->log);
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        FMissionApp_AddLogEntry(work, FMISSION_LOG_FAPP_COUNT - 1 - i);
    }
    count = 0;
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        if (work->logEntries[i].trainerId != 0) {
            count++;
        }
    }
    work->logCount = count;
}

// Puts a line of the festival's log in front of the screen's lines, where each player has one line
// Whether a line of the festival's log has no player
static inline BOOL FMissionApp_IsLogEntryEmpty(FMissionApp *work, u32 index) {
    return work->log->entries[index].trainerId == 0;
}

static void FMissionApp_AddLogEntry(FMissionApp *work, int index) {
    int found;
    int i;

    if (FMissionApp_IsLogEntryEmpty(work, index)) {
        return;
    }
    found = -1;
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        if (work->log->entries[index].trainerId == work->logEntries[i].trainerId
            && GFL_StrBufCmp(work->log->entries[index].name, work->logEntries[i].name) == TRUE) {
            found = i;
            break;
        }
    }
    if (found == 0) {
        work->logEntries[found].trainerId = work->log->entries[index].trainerId;
        work->logEntries[found].score = work->log->entries[index].score;
        if (work->logEntries[found].name != NULL) {
            GFL_StrBufFree(work->logEntries[found].name);
        }
        work->logEntries[found].name = GFL_StrBufClone(work->log->entries[index].name, work->heapId);
    } else if (found == 1) {
        FMissionApp_MoveLogEntry(work, 0, 1);
        work->logEntries[0].trainerId = work->log->entries[index].trainerId;
        work->logEntries[0].score = work->log->entries[index].score;
        if (work->logEntries[0].name != NULL) {
            GFL_StrBufFree(work->logEntries[0].name);
        }
        work->logEntries[0].name = GFL_StrBufClone(work->log->entries[index].name, work->heapId);
    } else if (found == 2 || found == -1) {
        if (work->logEntries[1].trainerId != 0) {
            FMissionApp_MoveLogEntry(work, 1, 2);
        }
        if (work->logEntries[0].trainerId != 0) {
            FMissionApp_MoveLogEntry(work, 0, 1);
        }
        work->logEntries[0].trainerId = work->log->entries[index].trainerId;
        work->logEntries[0].score = work->log->entries[index].score;
        if (work->logEntries[0].name != NULL) {
            GFL_StrBufFree(work->logEntries[0].name);
        }
        work->logEntries[0].name = GFL_StrBufClone(work->log->entries[index].name, work->heapId);
    }
}

// Copies one of the screen's lines over another, or clears it when from is -1
static void FMissionApp_MoveLogEntry(FMissionApp *work, int from, int to) {
    if (from == -1) {
        work->logEntries[to].trainerId = 0;
        work->logEntries[to].score = 0;
        if (work->logEntries[to].name != NULL) {
            GFL_StrBufFree(work->logEntries[to].name);
        }
        return;
    }
    work->logEntries[to].trainerId = work->logEntries[from].trainerId;
    work->logEntries[to].score = work->logEntries[from].score;
    if (work->logEntries[to].name != NULL) {
        GFL_StrBufFree(work->logEntries[to].name);
    }
    work->logEntries[to].name = GFL_StrBufClone(work->logEntries[from].name, work->heapId);
}

static void FMissionApp_PrintLabels(FMissionApp *work, u32 unused) {
    // Nothing reads this table of the screen's messages
    static const s16 messages[] = {
        Title4_Text_TimeLeft,   Title4_Text_NumberPlayers, Title4_Text_Objective,  Title4_Text_TotalScore,
        Title4_Text_Reward,     Title4_Text_Score,         Title4_Text_Rank,       Title4_Text_TopScore,
        Title4_Text_Empty_68,   Title4_Text_TimeLeft_2,    Title4_Text_BonusScore, Title4_Text_DoubleScore,
        Title4_Text_Empty_66,   Title4_Text_Empty_71,      Title4_Text_Empty_70,   Title4_Text_Of,
    };
    int i;

    for (i = 0; i < 32; i++) {
        StrBuf *strbuf;

        if (sFMissionAppLabels[i].message == -1) {
            break;
        }
        strbuf = GFL_MsgDataLoadStrbufNew(work->msgData[0], sFMissionAppLabels[i].message);
        FMissionAppDraw_Print(work, sFMissionAppLabels[i].window, 0, 0, strbuf, PRINT_ALIGN_LEFT,
                              FMISSION_APP_COLOR_TEXT);
        GFL_StrBufFree(strbuf);
    }
}

// Counts the result's frames: each step asks for the next part of the result, and the end starts its messages
static void FMissionApp_UpdateTimeline(FMissionApp *work) {
    if (work->seq == SEQ_END) {
        return;
    }
    if (work->flags & FLAG_TIMELINE_DONE) {
        return;
    }
    work->timer++;
    if (work->timer < sFMissionAppTimeline[work->step]) {
        return;
    }
    work->step++;
    if (sFMissionAppTimeline[work->step] == -1) {
        work->flags |= FLAG_TIMELINE_DONE;
        FMissionApp_NextMessage(work);
        work->seq = SEQ_JINGLE;
        return;
    }
    work->flags |= 1 << (work->step + FLAG_STEP_SHIFT);
    work->flags |= FLAG_REDRAW;
}

// Shows the next of the result's messages that applies; FALSE when there is none left
static BOOL FMissionApp_NextMessage(FMissionApp *work) {
    int message;

    if (!(work->flags & FLAG_MESSAGE_RESULT)) {
        if (work->flags & FLAG_CLEARED) {
            message = FMISSION_APP_MESSAGE_CLEARED;
            FMissionApp_PlayJingle(work, JINGLE_CLEARED);
        } else {
            message = FMISSION_APP_MESSAGE_FAILED;
            FMissionApp_PlayJingle(work, JINGLE_FAILED);
            work->flags |= FLAG_MESSAGES_DONE;
        }
        FMissionAppDraw_ShowMessage(work, FMISSION_APP_WINDOW_MESSAGE, message, FALSE);
        work->flags |= FLAG_MESSAGE_RESULT;
        return TRUE;
    }
    if (!(work->flags & FLAG_MESSAGE_ITEM)) {
        if (work->flags & FLAG_ITEM_ROOM) {
            message = FMISSION_APP_MESSAGE_ITEM;
            FMissionApp_PlayJingle(work, JINGLE_ITEM);
        } else {
            message = FMISSION_APP_MESSAGE_ITEM_NO_ROOM;
            work->flags |= FLAG_KEY_WAIT;
        }
        if (!(work->flags & FLAG_ITEM2) && !(work->flags & FLAG_LEVEL_UP)) {
            work->flags |= FLAG_MESSAGES_DONE;
        }
        FMissionAppDraw_ShowMessage(work, FMISSION_APP_WINDOW_MESSAGE, message, TRUE);
        work->flags |= FLAG_MESSAGE_ITEM;
        return TRUE;
    }
    if (!(work->flags & FLAG_MESSAGE_BONUS)) {
        if (work->flags & FLAG_ITEM2) {
            FMissionAppDraw_ShowMessage(work, FMISSION_APP_WINDOW_MESSAGE, FMISSION_APP_MESSAGE_BONUS, TRUE);
            work->flags |= FLAG_MESSAGE_BONUS;
            work->flags |= FLAG_KEY_WAIT;
            return TRUE;
        }
        work->flags |= FLAG_MESSAGE_BONUS;
    }
    if (!(work->flags & FLAG_MESSAGE_ITEM2)) {
        if (work->flags & FLAG_ITEM2) {
            if (work->flags & FLAG_ITEM2_ROOM) {
                message = FMISSION_APP_MESSAGE_ITEM2;
                FMissionApp_PlayJingle(work, JINGLE_ITEM);
            } else {
                message = FMISSION_APP_MESSAGE_ITEM2_NO_ROOM;
                work->flags |= FLAG_KEY_WAIT;
            }
            if (!(work->flags & FLAG_LEVEL_UP)) {
                work->flags |= FLAG_MESSAGES_DONE;
            }
            FMissionAppDraw_ShowMessage(work, FMISSION_APP_WINDOW_MESSAGE, message, TRUE);
            work->flags |= FLAG_MESSAGE_ITEM2;
            return TRUE;
        }
        work->flags |= FLAG_MESSAGE_ITEM2;
    }
    if (!(work->flags & FLAG_MESSAGE_LEVEL)) {
        if (work->flags & FLAG_LEVEL_UP) {
            if (work->flags & FLAG_ALONE) {
                message = FMISSION_APP_MESSAGE_LEVEL_ALONE;
            } else {
                message = FMISSION_APP_MESSAGE_LEVEL;
            }
            FMissionAppDraw_ShowMessage(work, FMISSION_APP_WINDOW_MESSAGE, message, TRUE);
            work->flags |= FLAG_MESSAGE_LEVEL;
            work->flags |= FLAG_MESSAGES_DONE;
            return TRUE;
        }
        work->flags |= FLAG_MESSAGE_LEVEL;
        work->flags |= FLAG_MESSAGES_DONE;
    }
    return FALSE;
}

// Plays a jingle over the music, which FMissionApp_IsJinglePlaying brings back
static void FMissionApp_PlayJingle(FMissionApp *work, int jingle) {
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    GFL_SndBGMPlay(sFMissionAppJingles[jingle], 0xffff);
    work->flags |= FLAG_JINGLE;
}

static BOOL FMissionApp_IsJinglePlaying(FMissionApp *work) {
    BOOL playing = TRUE;

    if (!GFL_SndBGMIsPlaying()) {
        GFL_SndBGMPop();
        playing = FALSE;
        GFL_SndBGMSetPaused(FALSE);
    }
    return playing;
}

static void FMissionApp_Dummy(FMissionApp *work) {
}
