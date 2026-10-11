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
    int unk24;
    int unk28;
    u8 unk2C;
    u8 unk2D;
    u8 unk2E;
    u8 unk2F;
    int unk30;
    BOOL firstVisit;
    BOOL unk38;
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
static int IrcCompatible_CalcCompatibility(u32 a, u32 b, u32 c, u32 d, CompatibleIrcProfile *mine,
                                           CompatibleIrcProfile *other, BOOL isNew, u32 date, HeapID heapId);
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
    sys_memset(wk, 0, sizeof(IrcCompatibleWork));
    wk->param = param;
    wk->profile = GFL_HeapAllocate(HEAP_ID, sizeof(CompatibleIrcProfile), FALSE, "irc_compatible.c", 281);
    sys_memset(wk->profile, 0, sizeof(CompatibleIrcProfile));
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
    case 0:
        if (!GFL_SndBGMIsFading()) {
            (*state)++;
        }
        break;
    case 1:
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
    case 0:
        *state = 1;
        break;
    case 1:
        IrcCompatible_Finish(wk);
        break;
    }
}

static void IrcCompatible_StepMenu(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        if (IrcCompatible_RunSubProc(&wk->subProc, 0, HEAP_ID, wk)) {
            *state = 1;
        }
        break;
    case 1:
        switch (wk->mode) {
        case 0:
            IrcCompatible_SetStep(wk, IrcCompatible_StepFlowA);
            break;
        case 1:
            IrcCompatible_SetStep(wk, IrcCompatible_StepFlowB);
            break;
        case 2:
            IrcCompatible_SetStep(wk, IrcCompatible_StepEnd);
            break;
        }
        break;
    }
}

static void IrcCompatible_StepFlowA(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        wk->unk2E = 0;
        wk->unk2C = 0;
        *state = 1;
        break;
    case 1:
        if (IrcCompatible_RunSubProc(&wk->subProc, 2, HEAP_ID, wk)) {
            *state = 2;
        }
        break;
    case 2:
        if (wk->unk28 == 0) {
            *state = 3;
        } else if (wk->unk28 == 2) {
            wk->unk2E = 0;
            wk->unk2C = 0;
            *state = 5;
        } else {
            *state = 6;
        }
        break;
    case 3:
        if (IrcCompatible_RunSubProc(&wk->subProc, 1, HEAP_ID, wk)) {
            *state = 4;
        }
        break;
    case 4:
        if (wk->unk24 == 0) {
            *state = 5;
        } else if (wk->unk24 == 2) {
            wk->unk2E = 0;
            wk->unk2C = 0;
            *state = 5;
        } else {
            *state = 6;
        }
        break;
    case 5:
        if (IrcCompatible_RunSubProc(&wk->subProc, 3, HEAP_ID, wk)) {
            *state = 6;
        }
        break;
    case 6:
        IrcCompatible_SetStep(wk, IrcCompatible_StepMenu);
        break;
    }
}

static void IrcCompatible_StepFlowB(IrcCompatibleWork *wk, u16 *state) {
    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        if (IrcCompatible_RunSubProc(&wk->subProc, 4, HEAP_ID, wk)) {
            *state = 2;
        }
        break;
    case 2:
        IrcCompatible_SetStep(wk, IrcCompatible_StepMenu);
        break;
    }
}

static void IrcCompatible_InitSubProc(IrcCompatibleSubProc *subProc, HeapID heapId) {
    sys_memset(subProc, 0, sizeof(IrcCompatibleSubProc));
    subProc->manager = CreateGameProcManager(heapId);
}

static BOOL IrcCompatible_UpdateSubProc(IrcCompatibleSubProc *subProc) {
    return GFL_ProcMgrUpdate(subProc->manager);
}

static void IrcCompatible_FreeSubProc(IrcCompatibleSubProc *subProc) {
    FreeGameProcManager(subProc->manager);
    sys_memset(subProc, 0, sizeof(IrcCompatibleSubProc));
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

    sys_memset(param, 0, sizeof(IrcCompatibleParam0));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    if (wk->firstVisit == TRUE) {
        param->visit = 0;
        wk->firstVisit = FALSE;
    } else {
        if (wk->unk38) {
            wk->unk38 = FALSE;
            param->visit = 2;
        } else {
            param->visit = 1;
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

    sys_memset(param, 0, sizeof(IrcCompatibleParam1));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    return param;
}

static void IrcCompatible_FreeParam1(void *param, IrcCompatibleWork *wk) {
    IrcCompatibleParam1 *p = param;

    wk->unk24 = p->result;
    wk->unk2E = p->unk10;
    wk->unk2F = p->unk11;
    wk->unk30 = p->unk12;
    GFL_HeapFree(p);
}

static void *IrcCompatible_CreateParam2(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam2 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam2), FALSE, "irc_compatible.c", 996);

    sys_memset(param, 0, sizeof(IrcCompatibleParam2));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    return param;
}

static void IrcCompatible_FreeParam2(void *param, IrcCompatibleWork *wk) {
    IrcCompatibleParam2 *p = param;

    wk->unk28 = p->result;
    wk->unk2C = p->unk10;
    wk->unk2D = p->unk14;
    GFL_HeapFree(p);
}

static void *IrcCompatible_CreateParam3(HeapID heapId, IrcCompatibleWork *wk) {
    BOOL isNew = FALSE;
    IrcCompatibleParam3 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam3), FALSE, "irc_compatible.c", 1048);

    sys_memset(param, 0, sizeof(IrcCompatibleParam3));
    param->gsys = wk->param->gsys;
    param->sys = wk->sys;
    param->profile = wk->profile;
    if (wk->unk2C != 0 && wk->unk2E != 0) {
        CompatibleIrcProfile mine;
        TrainerLogSave *log;
        BOOL isNewTrainer;
        PlayerInfo *trainer = &wk->profile->trainer;
        u32 date;

        GSYS_GetGameData(wk->param->gsys);
        log = func_0200afbc(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys)));
        if (!func_0200b0f0(log, getIDAsUInt(trainer))) {
            isNew = TRUE;
        }
        isNewTrainer = FALSE;
        if (wk->unk30 != 0 && isNew) {
            isNewTrainer = TRUE;
        }
        CompatibleIrcSys_BuildProfile(wk->param->gsys, &mine);
        if (func_02042bc4()) {
            date = mine.date;
        } else {
            date = wk->profile->date;
        }
        param->result = IrcCompatible_CalcCompatibility(wk->unk2C, wk->unk2E, wk->unk2D, wk->unk2F, &mine, wk->profile,
                                                        isNewTrainer, date, HEAP_ID);
    }
    return param;
}

static void IrcCompatible_FreeParam3(void *param, IrcCompatibleWork *wk) {
    GFL_HeapFree(param);
}

static void *IrcCompatible_CreateParam4(HeapID heapId, IrcCompatibleWork *wk) {
    IrcCompatibleParam4 *param = GFL_HeapAllocate(heapId, sizeof(IrcCompatibleParam4), FALSE, "irc_compatible.c", 1179);

    sys_memset(param, 0, sizeof(IrcCompatibleParam4));
    param->gsys = wk->param->gsys;
    return param;
}

static void IrcCompatible_FreeParam4(void *param, IrcCompatibleWork *wk) {
    GFL_HeapFree(param);
    wk->unk38 = TRUE;
}

static int IrcCompatible_CalcCompatibility(u32 a, u32 b, u32 c, u32 d, CompatibleIrcProfile *mine,
                                           CompatibleIrcProfile *other, BOOL isNew, u32 date, HeapID heapId) {
    u32 birthday0 = IrcCompatible_BirthdayScore(mine, date);
    u32 birthday1 = IrcCompatible_BirthdayScore(other, date);
    int score = (birthday0 + birthday1) / 2 * 10 / 100;
    u32 nameMatch;
    int ratio;

    score += 50 * a / 100 + 40 * b / 100;
    nameMatch = IrcCompatible_NameMatch(GetPlayerName(&mine->trainer), GetPlayerName(&other->trainer));
    ratio = FX32_CONST(nameMatch) / 150;
    if (isNew) {
        ratio = FX32_CONST(0.8);
    } else if (ratio > FX32_ONE) {
        ratio = FX32_ONE;
    } else if (ratio < FX32_ONE / 2) {
        ratio = FX32_ONE / 2;
    }
    score = score + (((100 - score) * ratio) >> FX32_SHIFT) - (d + c);
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
    return func_0200b11c(profile->birthMonth, profile->birthDay, &rtcDate);
}
