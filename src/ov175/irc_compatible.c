#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/irc_compat_menu.h"
#include "app/irc_compatible.h"
#include "app/irc_ranking.h"
#include "app/irc_rhythm_check.h"
#include "app/irc_sync_check.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "nitro/rtc.h"
#include "save/player_info.h"
#include "save/trainer_log.h"
#include "system/game_data.h"
#include "system/game_system.h"

// irc_compatible.c, a ROM name: the compatibility checker's proc. It runs one of five screens at a time, each in its
// own process of a manager that it owns, and picks the next one by what the last gave back. The names are descriptive
// guesses

#define HEAP_ID 0x3a
#define HEAP_ID_TAIL HEAPID_TAIL(HEAP_ID)
#define SUB_PROC_COUNT 5

// The screens, as indexes of the table of sub processes
enum {
    SCREEN_MENU,
    SCREEN_SYNC_CHECK,
    SCREEN_RHYTHM_CHECK,
    SCREEN_RESULT,
    SCREEN_RANKING,
};

// The steps of the proc's main function, and of the steps that run the screens
enum {
    MAIN_WAIT_BGM,
    MAIN_RUN,
};

enum {
    END_DELAY,
    END_FINISH,
};

enum {
    MENU_RUN_SCREEN,
    MENU_NEXT,
};

enum {
    FLOW_A_INIT,
    FLOW_A_RHYTHM_CHECK,
    FLOW_A_AFTER_RHYTHM_CHECK,
    FLOW_A_SYNC_CHECK,
    FLOW_A_AFTER_SYNC_CHECK,
    FLOW_A_RESULT,
    FLOW_A_BACK_TO_MENU,
};

enum {
    FLOW_B_INIT,
    FLOW_B_RANKING,
    FLOW_B_BACK_TO_MENU,
};

// The steps of the sub process runner
enum {
    SUB_PROC_CREATE_PARAM,
    SUB_PROC_QUEUE,
    SUB_PROC_FREE_PARAM,
    SUB_PROC_DONE,
};

typedef struct {
    GameProcManager *manager;
    int state;
    void *param;
} IrcCompatibleSubProc;

typedef struct IrcCompatibleWork IrcCompatibleWork;
typedef void (*IrcCompatibleStep)(IrcCompatibleWork *wk, u16 *state);

struct IrcCompatibleWork {
    CompatibleIrcSys *sys;
    IrcCompatibleSubProc subProc;
    IrcCompatibleParam *param;
    IrcCompatibleStep step;
    u16 stepState;
    BOOL done;
    int mode;
    int syncResult;
    int rhythmResult;
    u8 rhythmScore;
    u8 rhythmTapDiff;
    u8 syncScore;
    u8 syncRetries;
    int partnerIsNew;
    BOOL firstVisit;
    BOOL rankingVisited;
    CompatibleIrcProfile *profile;
};

typedef struct {
    u32 overlayId;
    const GameProcFunctions *procFunctions;
    void *(*createParam)(HeapID heapId, IrcCompatibleWork *wk);
    void (*freeParam)(void *param, IrcCompatibleWork *wk);
} IrcCompatibleSubProcDef;

static BOOL IrcCompatible_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatible_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcCompatible_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void IrcCompatible_SetStep(IrcCompatibleWork *wk, IrcCompatibleStep step);
static void IrcCompatible_Finish(IrcCompatibleWork *wk);
static void IrcCompatible_StepStart(IrcCompatibleWork *wk, u16 *state);
static void IrcCompatible_StepEnd(IrcCompatibleWork *wk, u16 *state);
static void IrcCompatible_StepMenu(IrcCompatibleWork *wk, u16 *state);
static void IrcCompatible_StepFlowA(IrcCompatibleWork *wk, u16 *state);
static void IrcCompatible_StepFlowB(IrcCompatibleWork *wk, u16 *state);
static void IrcCompatible_InitSubProc(IrcCompatibleSubProc *subProc, HeapID heapId);
static BOOL IrcCompatible_UpdateSubProc(IrcCompatibleSubProc *subProc);
static void IrcCompatible_FreeSubProc(IrcCompatibleSubProc *subProc);
static BOOL IrcCompatible_RunSubProc(IrcCompatibleSubProc *subProc, int index, HeapID heapId, IrcCompatibleWork *wk);
static void *IrcCompatible_CreateParam0(HeapID heapId, IrcCompatibleWork *wk);
static void IrcCompatible_FreeParam0(void *param, IrcCompatibleWork *wk);
static void *IrcCompatible_CreateParam1(HeapID heapId, IrcCompatibleWork *wk);
static void IrcCompatible_FreeParam1(void *param, IrcCompatibleWork *wk);
static void *IrcCompatible_CreateParam2(HeapID heapId, IrcCompatibleWork *wk);
static void IrcCompatible_FreeParam2(void *param, IrcCompatibleWork *wk);
static void *IrcCompatible_CreateParam3(HeapID heapId, IrcCompatibleWork *wk);
static void IrcCompatible_FreeParam3(void *param, IrcCompatibleWork *wk);
static void *IrcCompatible_CreateParam4(HeapID heapId, IrcCompatibleWork *wk);
static void IrcCompatible_FreeParam4(void *param, IrcCompatibleWork *wk);
static int IrcCompatible_CalcCompatibility(u32 rhythmScore, u32 syncScore, u32 tapDiff, u32 syncRetries,
                                           CompatibleIrcProfile *mine, CompatibleIrcProfile *other, BOOL isNew,
                                           u32 date, HeapID heapId);
static u32 IrcCompatible_NameMatch(const u16 *name0, const u16 *name1);
static u32 IrcCompatible_HighestBit(u32 value, u32 count);
static u8 IrcCompatible_BirthdayScore(const CompatibleIrcProfile *profile, u32 date);

const GameProcFunctions IRC_COMPATIBLE_PROC_FUNCTIONS = {
    IrcCompatible_Init,
    IrcCompatible_Main,
    IrcCompatible_Exit,
};

static const IrcCompatibleSubProcDef sIrcCompatibleSubProcs[SUB_PROC_COUNT] = {
    { OVERLAY_ID(176), &IRC_COMPAT_MENU_PROC_FUNCTIONS, IrcCompatible_CreateParam0, IrcCompatible_FreeParam0 },
    { OVERLAY_ID(177), &IRC_SYNC_CHECK_PROC_FUNCTIONS, IrcCompatible_CreateParam1, IrcCompatible_FreeParam1 },
    { OVERLAY_ID(178), &IRC_RHYTHM_CHECK_PROC_FUNCTIONS, IrcCompatible_CreateParam2, IrcCompatible_FreeParam2 },
    { OVERLAY_ID(179), &IRC_COMPAT_RESULT_PROC_FUNCTIONS, IrcCompatible_CreateParam3, IrcCompatible_FreeParam3 },
    { OVERLAY_ID(180), &IRC_RANKING_PROC_FUNCTIONS, IrcCompatible_CreateParam4, IrcCompatible_FreeParam4 },
};

static BOOL IrcCompatible_Init(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatibleWork *wk;
    GameData *gameData;

    GFL_HeapCreateChild(HEAPID_USER, HEAP_ID, 0x8000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(IrcCompatibleWork), HEAP_ID);
    gameData = NULL;
    MI_CpuFill8(wk, 0, sizeof(IrcCompatibleWork));
    wk->param = param;
    wk->profile = GFL_HeapAllocate(HEAP_ID, sizeof(CompatibleIrcProfile), FALSE, "irc_compatible.c", 281);
    MI_CpuFill8(wk->profile, 0, sizeof(CompatibleIrcProfile));
    IrcCompatible_InitSubProc(&wk->subProc, HEAP_ID);
    if (wk->param->gsys != NULL) {
        gameData = GSYS_GetGameData(wk->param->gsys);
    }
    wk->sys = CompatibleIrcSys_Create(-1, gameData, HEAP_ID, FALSE);
    wk->firstVisit = TRUE;
    IrcCompatible_SetStep(wk, IrcCompatible_StepStart);
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    return TRUE;
}

static BOOL IrcCompatible_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatibleWork *wk = work;

    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    CompatibleIrcSys_Free(wk->sys);
    IrcCompatible_FreeSubProc(&wk->subProc);
    GFL_HeapFree(wk->profile);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAP_ID);
    return TRUE;
}

static BOOL IrcCompatible_Main(GameProc *proc, u32 *state, void *param, void *work) {
    IrcCompatibleWork *wk = work;

    switch (*state) {
    case MAIN_WAIT_BGM:
        if (!GFL_SndBGMIsFading()) {
            (*state)++;
        }
        break;
    case MAIN_RUN:
        if (!IrcCompatible_UpdateSubProc(&wk->subProc)) {
            wk->step(wk, &wk->stepState);
        }
        if (wk->done) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void IrcCompatible_SetStep(IrcCompatibleWork *wk, IrcCompatibleStep step) {
    wk->step = step;
    wk->stepState = 0;
}

static void IrcCompatible_Finish(IrcCompatibleWork *wk) {
    wk->done = TRUE;
}

static void IrcCompatible_StepStart(IrcCompatibleWork *wk, u16 *state) {
    if (*state == 0) {
        IrcCompatible_SetStep(wk, IrcCompatible_StepMenu);
    }
}

static void IrcCompatible_StepEnd(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case END_DELAY:
        *state = END_FINISH;
        break;
    case END_FINISH:
        IrcCompatible_Finish(wk);
        break;
    }
}

static void IrcCompatible_StepMenu(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case MENU_RUN_SCREEN:
        if (IrcCompatible_RunSubProc(&wk->subProc, SCREEN_MENU, HEAP_ID, wk)) {
            *state = MENU_NEXT;
        }
        break;
    case MENU_NEXT:
        switch (wk->mode) {
        case IRC_COMPAT_MENU_RESULT_CHECK:
            IrcCompatible_SetStep(wk, IrcCompatible_StepFlowA);
            break;
        case IRC_COMPAT_MENU_RESULT_RANKING:
            IrcCompatible_SetStep(wk, IrcCompatible_StepFlowB);
            break;
        case IRC_COMPAT_MENU_RESULT_QUIT:
            IrcCompatible_SetStep(wk, IrcCompatible_StepEnd);
            break;
        }
        break;
    }
}

static void IrcCompatible_StepFlowA(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case FLOW_A_INIT:
        wk->syncScore = 0;
        wk->rhythmScore = 0;
        *state = FLOW_A_RHYTHM_CHECK;
        break;
    case FLOW_A_RHYTHM_CHECK:
        if (IrcCompatible_RunSubProc(&wk->subProc, SCREEN_RHYTHM_CHECK, HEAP_ID, wk)) {
            *state = FLOW_A_AFTER_RHYTHM_CHECK;
        }
        break;
    case FLOW_A_AFTER_RHYTHM_CHECK:
        if (wk->rhythmResult == IRC_RHYTHM_CHECK_RESULT_DONE) {
            *state = FLOW_A_SYNC_CHECK;
        } else if (wk->rhythmResult == IRC_RHYTHM_CHECK_RESULT_INTERRUPTED) {
            wk->syncScore = 0;
            wk->rhythmScore = 0;
            *state = FLOW_A_RESULT;
        } else {
            *state = FLOW_A_BACK_TO_MENU;
        }
        break;
    case FLOW_A_SYNC_CHECK:
        if (IrcCompatible_RunSubProc(&wk->subProc, SCREEN_SYNC_CHECK, HEAP_ID, wk)) {
            *state = FLOW_A_AFTER_SYNC_CHECK;
        }
        break;
    case FLOW_A_AFTER_SYNC_CHECK:
        if (wk->syncResult == IRC_SYNC_CHECK_RESULT_DONE) {
            *state = FLOW_A_RESULT;
        } else if (wk->syncResult == IRC_SYNC_CHECK_RESULT_INTERRUPTED) {
            wk->syncScore = 0;
            wk->rhythmScore = 0;
            *state = FLOW_A_RESULT;
        } else {
            *state = FLOW_A_BACK_TO_MENU;
        }
        break;
    case FLOW_A_RESULT:
        if (IrcCompatible_RunSubProc(&wk->subProc, SCREEN_RESULT, HEAP_ID, wk)) {
            *state = FLOW_A_BACK_TO_MENU;
        }
        break;
    case FLOW_A_BACK_TO_MENU:
        IrcCompatible_SetStep(wk, IrcCompatible_StepMenu);
        break;
    }
}

static void IrcCompatible_StepFlowB(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case FLOW_B_INIT:
        *state = FLOW_B_RANKING;
        break;
    case FLOW_B_RANKING:
        if (IrcCompatible_RunSubProc(&wk->subProc, SCREEN_RANKING, HEAP_ID, wk)) {
            *state = FLOW_B_BACK_TO_MENU;
        }
        break;
    case FLOW_B_BACK_TO_MENU:
        IrcCompatible_SetStep(wk, IrcCompatible_StepMenu);
        break;
    }
}

static void IrcCompatible_InitSubProc(IrcCompatibleSubProc *subProc, HeapID heapId) {
    MI_CpuFill8(subProc, 0, sizeof(IrcCompatibleSubProc));
    subProc->manager = CreateGameProcManager(heapId);
}

static BOOL IrcCompatible_UpdateSubProc(IrcCompatibleSubProc *subProc) {
    return GFL_ProcMgrUpdate(subProc->manager);
}

static void IrcCompatible_FreeSubProc(IrcCompatibleSubProc *subProc) {
    FreeGameProcManager(subProc->manager);
    MI_CpuFill8(subProc, 0, sizeof(IrcCompatibleSubProc));
}

// Runs a screen, and returns TRUE once it has ended and given its result back
static BOOL IrcCompatible_RunSubProc(IrcCompatibleSubProc *subProc, int index, HeapID heapId, IrcCompatibleWork *wk) {
    switch (subProc->state) {
    case SUB_PROC_CREATE_PARAM:
        if (sIrcCompatibleSubProcs[index].createParam != NULL) {
            subProc->param = sIrcCompatibleSubProcs[index].createParam(heapId, wk);
        } else {
            subProc->param = NULL;
        }
        subProc->state = SUB_PROC_QUEUE;
        break;
    case SUB_PROC_QUEUE:
        QueueGameProc(subProc->manager, sIrcCompatibleSubProcs[index].overlayId,
                      sIrcCompatibleSubProcs[index].procFunctions, subProc->param);
        subProc->state = SUB_PROC_FREE_PARAM;
        break;
    case SUB_PROC_FREE_PARAM:
        if (sIrcCompatibleSubProcs[index].freeParam != NULL) {
            sIrcCompatibleSubProcs[index].freeParam(subProc->param, wk);
        }
        subProc->state = SUB_PROC_DONE;
        break;
    case SUB_PROC_DONE:
        subProc->state = SUB_PROC_CREATE_PARAM;
        return TRUE;
    }
    return FALSE;
}

static void *IrcCompatible_CreateParam0(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam0 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam0), FALSE, "irc_compatible.c", 878);

    MI_CpuFill8(param, 0, sizeof(IrcCompatibleParam0));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    if (wk->firstVisit == TRUE) {
        param->visit = IRC_COMPAT_VISIT_FIRST;
        wk->firstVisit = FALSE;
    } else {
        if (wk->rankingVisited) {
            wk->rankingVisited = FALSE;
            param->visit = IRC_COMPAT_VISIT_AFTER_RESULT;
        } else {
            param->visit = IRC_COMPAT_VISIT_AGAIN;
        }
    }
    return param;
}

static void IrcCompatible_FreeParam0(void *param, IrcCompatibleWork *wk) {
    IrcCompatibleParam0 *p = param;

    wk->mode = p->result;
    GFL_HeapFree(p);
}

static void *IrcCompatible_CreateParam1(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam1 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam1), FALSE, "irc_compatible.c", 943);

    MI_CpuFill8(param, 0, sizeof(IrcCompatibleParam1));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    return param;
}

static void IrcCompatible_FreeParam1(void *param, IrcCompatibleWork *wk) {
    IrcCompatibleParam1 *p = param;

    wk->syncResult = p->result;
    wk->syncScore = p->score;
    wk->syncRetries = p->retries;
    wk->partnerIsNew = p->isNew;
    GFL_HeapFree(p);
}

static void *IrcCompatible_CreateParam2(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam2 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam2), FALSE, "irc_compatible.c", 996);

    MI_CpuFill8(param, 0, sizeof(IrcCompatibleParam2));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    return param;
}

static void IrcCompatible_FreeParam2(void *param, IrcCompatibleWork *wk) {
    IrcCompatibleParam2 *p = param;

    wk->rhythmResult = p->result;
    wk->rhythmScore = p->score;
    wk->rhythmTapDiff = p->extraTapDiff;
    GFL_HeapFree(p);
}

static void *IrcCompatible_CreateParam3(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam3 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam3), FALSE, "irc_compatible.c", 1048);

    MI_CpuFill8(param, 0, sizeof(IrcCompatibleParam3));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    if (wk->rhythmScore != 0 && wk->syncScore != 0) {
        CompatibleIrcProfile mine;
        TrainerLogSave *log;
        BOOL isNewTrainer;
        BOOL isNew = FALSE;
        PlayerInfo *trainer = &wk->profile->trainer;
        u32 date;

        GSYS_GetGameData(wk->param->gsys);
        log = TrainerLogSave_Get(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));
        if (!TrainerLogSave_HasId(log, getIDAsUInt(trainer))) {
            isNew = TRUE;
        }
        isNewTrainer = FALSE;
        if (wk->partnerIsNew != 0 && isNew) {
            isNewTrainer = TRUE;
        }
        CompatibleIrcSys_BuildProfile(wk->param->gsys, &mine);
        if (GFL_NetIsParent()) {
            date = mine.date;
        } else {
            date = wk->profile->date;
        }
        param->result =
            IrcCompatible_CalcCompatibility(wk->rhythmScore, wk->syncScore, wk->rhythmTapDiff, wk->syncRetries, &mine,
                                            wk->profile, isNewTrainer, date, HEAP_ID);
    }
    return param;
}

static void IrcCompatible_FreeParam3(void *param, IrcCompatibleWork *wk) {
    GFL_HeapFree(param);
}

static void *IrcCompatible_CreateParam4(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam4 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam4), FALSE, "irc_compatible.c", 1179);

    MI_CpuFill8(param, 0, sizeof(IrcCompatibleParam4));
    param->gsys = wk->param->gsys;
    return param;
}

static void IrcCompatible_FreeParam4(void *param, IrcCompatibleWork *wk) {
    GFL_HeapFree(param);
    wk->rankingVisited = TRUE;
}

static int IrcCompatible_CalcCompatibility(u32 rhythmScore, u32 syncScore, u32 tapDiff, u32 syncRetries,
                                           CompatibleIrcProfile *mine, CompatibleIrcProfile *other, BOOL isNew,
                                           u32 date, HeapID heapId) {
    u32 birthday0 = IrcCompatible_BirthdayScore(mine, date);
    u32 birthday1 = IrcCompatible_BirthdayScore(other, date);
    int score = (birthday0 + birthday1) / 2 * 10 / 100;
    u32 nameMatch;
    int ratio;

    score += 50 * rhythmScore / 100 + 40 * syncScore / 100;
    nameMatch = IrcCompatible_NameMatch(GetPlayerName(&mine->trainer), GetPlayerName(&other->trainer));
    ratio = FX32_CONST(nameMatch) / 150;
    if (isNew) {
        ratio = FX32_CONST(0.8);
    } else if (ratio > FX32_ONE) {
        ratio = FX32_ONE;
    } else if (ratio < FX32_ONE / 2) {
        ratio = FX32_ONE / 2;
    }
    score = score + (((100 - score) * ratio) >> FX32_SHIFT) - (syncRetries + tapDiff);
    if (score > 100) {
        return 100;
    }
    if (score < 0) {
        return 0;
    }
    return score;
}

// How much of the two names' characters agree in a bit, over the bits that the characters have
static u32 IrcCompatible_NameMatch(const u16 *name0, const u16 *name1) {
    u32 char0 = *name0;
    u32 char1 = *name1;
    u32 bit0;
    u32 bit1;
    u32 matches;
    u32 total;

    if (char0 >= 0x30a1 && char0 <= 0x30f4) {
        char0 -= 0x60;
    }
    if (char1 >= 0x30a1 && char1 <= 0x30f4) {
        char1 -= 0x60;
    }
    bit0 = IrcCompatible_HighestBit(char0, 8);
    bit1 = IrcCompatible_HighestBit(char1, 8);
    total = 0;
    matches = 0;
    while (TRUE) {
        if (bit0 == 0) {
            name0++;
            char0 = *name0;
            if (char0 == GFL_StrBufGetTerminator()) {
                break;
            }
            if (char0 >= 0x30a1 && char0 <= 0x30f4) {
                char0 -= 0x60;
            }
            bit0 = IrcCompatible_HighestBit(char0, 8);
        }
        if (bit1 == 0) {
            name1++;
            char1 = *name1;
            if (char1 == GFL_StrBufGetTerminator()) {
                break;
            }
            if (char1 >= 0x30a1 && char1 <= 0x30f4) {
                char1 -= 0x60;
            }
            bit1 = IrcCompatible_HighestBit(char1, 8);
        }
        if (bit0 != 0 && bit1 != 0) {
            if (((char0 >> bit0) & 1) == ((char1 >> bit1) & 1)) {
                matches++;
            }
            total++;
            bit0--;
            bit1--;
        }
    }
    return matches * 100 / total;
}

// The highest set bit of a value below the given bit count, or 0
static u32 IrcCompatible_HighestBit(u32 value, u32 count) {
    u32 bit = count - 1;

    while (bit != 0) {
        if ((value >> bit) & 1) {
            break;
        }
        bit--;
    }
    return bit;
}

static u8 IrcCompatible_BirthdayScore(const CompatibleIrcProfile *profile, u32 date) {
    RTCDate rtcDate;

    rtcDate.year = (u8)(date >> 24);
    rtcDate.month = (u8)(date >> 16);
    rtcDate.day = (u8)(date >> 8);
    rtcDate.week = (u8)date;
    return TrainerLog_GetBirthdayValue(profile->birthMonth, profile->birthDay, &rtcDate);
}
