// The Battle Recorder's proc (br_main.c, the name its allocation passes). It holds the data that outlives the core
// proc and switches between the core proc of overlay 271 and the battle system, which plays a battle video

#include "types.h"
#include "app/battle_recorder.h"
#include "app/battle_recorder/br_core.h"
#include "battle/battle_proc.h"
#include "battle/battle_rec_tool.h"
#include "battle/btl_setup.h"
#include "constants/sound.h"
#include "field/bsubway_scr.h"
#include "field/field_sound.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The procs that br_main.c switches between, which index sBrMainProcTable
enum {
    BR_MAIN_PROCID_CORE,
    BR_MAIN_PROCID_BATTLE,
};

// The steps of the Battle Recorder's proc
enum {
    BR_MAIN_STATE_START,
    BR_MAIN_STATE_RUN,
    BR_MAIN_STATE_END,
};

// The steps of running a proc: pick the next one, start it, wait for it and free its parameter
enum {
    BR_MAIN_SEQ_SWITCH,
    BR_MAIN_SEQ_START,
    BR_MAIN_SEQ_WAIT,
    BR_MAIN_SEQ_FREE,
    BR_MAIN_SEQ_END,
};

// Makes a proc's parameter. preID is the proc that ran before it
typedef void *(*BrMainCreateParamFunc)(HeapID heapId, void *work, u32 preID);
// Reads the results from a proc's parameter once it has ended and frees it
typedef void (*BrMainFreeParamFunc)(void *param, void *work);

typedef struct {
    s32 overlayId;
    const GameProcFunctions *procFuncs;
    BrMainCreateParamFunc createParam;
    BrMainFreeParamFunc freeParam;
} BrMainProcData;

// Runs one proc at a time, the next one being the one that the last freeParam asked for
typedef struct {
    GameProcManager *procMgr;
    u32 seq;
    void *p_proc_param;
    u32 unk0c;
    u16 heapId;
    void *work;
    const BrMainProcData *tbl;
    u32 preProcID;
    u32 nextProcID;
    u32 procID;
} BrMainProcSys;

typedef struct {
    BrMainProcSys procSys;
    BrData data;
    BattleRecorderParam *param;
} BrMainWork;

static BOOL BrMain_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMain_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrMain_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrMainProcSys_Init(BrMainProcSys *p_wk, const BrMainProcData *tbl, void *work, HeapID heapId);
static BOOL BrMainProcSys_Main(BrMainProcSys *p_wk);
static void BrMainProcSys_Exit(BrMainProcSys *p_wk);
static void BrMainProcSys_SetNext(BrMainProcSys *p_wk, u32 procID);
static void *BrMain_CoreCreateParam(HeapID heapId, void *work, u32 preID);
static void BrMain_CoreFreeParam(void *param, void *work);
static void *BrMain_BattleCreateParam(HeapID heapId, void *work, u32 preID);
static void BrMain_BattleFreeParam(void *param, void *work);
static void BrMain_CheckBgm(u16 *bgm, u16 *winBgm);

const GameProcFunctions BR_MAIN_PROC_FUNCTIONS = {
    BrMain_ProcInit,
    BrMain_ProcMain,
    BrMain_ProcExit,
};

static const BrMainProcData sBrMainProcTable[] = {
    [BR_MAIN_PROCID_CORE] = { OVERLAY_ID(271), &BR_CORE_PROC_FUNCTIONS, BrMain_CoreCreateParam, BrMain_CoreFreeParam },
    [BR_MAIN_PROCID_BATTLE] = { OVERLAY_ID(167), &BATTLE_PROC_FUNCTIONS, BrMain_BattleCreateParam,
                                BrMain_BattleFreeParam },
};

static BOOL BrMain_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMainWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE_RECORDER_SYS, 0x12000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BrMainWork), HEAPID_BATTLE_RECORDER_SYS);
    MI_CpuFill8(wk, 0, sizeof(BrMainWork));
    wk->param = param;
    BattleRec_Init(HEAPID_BATTLE_RECORDER_SYS);
    BrMainProcSys_Init(&wk->procSys, sBrMainProcTable, wk, HEAPID_BATTLE_RECORDER_SYS);
    BrMainProcSys_SetNext(&wk->procSys, BR_MAIN_PROCID_CORE);
    return TRUE;
}

static BOOL BrMain_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrMainWork *wk = work;
    int i;

    if (wk->data.recordInfo.isInit) {
        for (i = 0; i < BR_RECORD_NUM; i++) {
            if (wk->data.recordInfo.name[i] != NULL) {
                GFL_StrBufFree(wk->data.recordInfo.name[i]);
            }
        }
    }
    BrMainProcSys_Exit(&wk->procSys);
    freeVSPlayerBlkClearPtr();
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_RECORDER_SYS);
    return TRUE;
}

static BOOL BrMain_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrMainWork *wk = work;

    switch (*state) {
    case BR_MAIN_STATE_START:
        *state = BR_MAIN_STATE_RUN;
        break;
    case BR_MAIN_STATE_RUN:
        if (BrMainProcSys_Main(&wk->procSys)) {
            *state = BR_MAIN_STATE_END;
        }
        break;
    case BR_MAIN_STATE_END:
        return TRUE;
    }
    return FALSE;
}

static void BrMainProcSys_Init(BrMainProcSys *p_wk, const BrMainProcData *tbl, void *work, HeapID heapId) {
    MI_CpuFill8(p_wk, 0, sizeof(BrMainProcSys));
    p_wk->procMgr = CreateGameProcManager(heapId);
    p_wk->work = work;
    p_wk->tbl = tbl;
    p_wk->heapId = heapId;
}

static BOOL BrMainProcSys_Main(BrMainProcSys *p_wk) {
    switch (p_wk->seq) {
    case BR_MAIN_SEQ_SWITCH:
        p_wk->preProcID = p_wk->procID;
        p_wk->procID = p_wk->nextProcID;
        p_wk->seq = BR_MAIN_SEQ_START;
        break;
    case BR_MAIN_SEQ_START:
        if (p_wk->tbl[p_wk->procID].createParam != NULL) {
            p_wk->p_proc_param = p_wk->tbl[p_wk->procID].createParam(p_wk->heapId, p_wk->work, p_wk->preProcID);
        } else {
            p_wk->p_proc_param = NULL;
        }
        QueueGameProc(p_wk->procMgr, p_wk->tbl[p_wk->procID].overlayId, p_wk->tbl[p_wk->procID].procFuncs,
                      p_wk->p_proc_param);
        p_wk->seq = BR_MAIN_SEQ_WAIT;
        break;
    case BR_MAIN_SEQ_WAIT:
        if (!GFL_ProcMgrUpdate(p_wk->procMgr)) {
            p_wk->seq = BR_MAIN_SEQ_FREE;
        }
        break;
    case BR_MAIN_SEQ_FREE:
        if (p_wk->tbl[p_wk->procID].freeParam != NULL) {
            p_wk->tbl[p_wk->procID].freeParam(p_wk->p_proc_param, p_wk->work);
            p_wk->p_proc_param = NULL;
        }
        if (p_wk->procID != p_wk->nextProcID) {
            p_wk->seq = BR_MAIN_SEQ_SWITCH;
        } else {
            p_wk->seq = BR_MAIN_SEQ_END;
        }
        break;
    case BR_MAIN_SEQ_END:
        return TRUE;
    }
    return FALSE;
}

static void BrMainProcSys_Exit(BrMainProcSys *p_wk) {
    GFL_ASSERT(p_wk->p_proc_param == NULL);
    FreeGameProcManager(p_wk->procMgr);
    MI_CpuFill8(p_wk, 0, sizeof(BrMainProcSys));
}

static void BrMainProcSys_SetNext(BrMainProcSys *p_wk, u32 procID) {
    p_wk->nextProcID = procID;
}

static void *BrMain_CoreCreateParam(HeapID heapId, void *work, u32 preID) {
    BrMainWork *wk = work;
    BrCoreParam *param = GFL_HeapAllocate(heapId, sizeof(BrCoreParam), FALSE, "br_main.c", 487);

    MI_CpuFill8(param, 0, sizeof(BrCoreParam));
    param->mainParam = wk->param;
    param->data = &wk->data;
    if (preID == BR_MAIN_PROCID_BATTLE) {
        param->mode = BR_CORE_MODE_RETURN;
    } else {
        param->mode = BR_CORE_MODE_INIT;
    }
    return param;
}

static void BrMain_CoreFreeParam(void *param, void *work) {
    BrCoreParam *coreParam = param;

    switch (coreParam->result) {
    case BR_CORE_RESULT_EXIT:
        break;
    case BR_CORE_RESULT_PLAY_VIDEO:
        BrMainProcSys_SetNext(work, BR_MAIN_PROCID_BATTLE);
        break;
    }
    GFL_HeapFree(coreParam);
}

static void *BrMain_BattleCreateParam(HeapID heapId, void *work, u32 preID) {
    BrMainWork *wk = work;
    BtlSetup *setup = BtlSetup_Create(heapId);

    BtlSetup_InitForRecord(setup, wk->param->gameData, heapId);
    BattleRec_LoadToolOverlay();
    BattleRecTool_LoadSetup(setup, 0, heapId);
    BattleRec_UnloadToolOverlay();
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    BrMain_CheckBgm(&setup->fieldSituation.bgm, &setup->fieldSituation.winBgm);
    GFL_SndBGMPlay(setup->fieldSituation.bgm, 0xffff);
    return setup;
}

static void BrMain_BattleFreeParam(void *param, void *work) {
    BtlSetup *setup = param;
    BrMainWork *wk = work;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(wk->param->gameData);

    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    GFL_SndBGMFadeIn(30);
    if (wk->param->mode == BR_MODE_BROWSE) {
        FieldSnd_SetPlayerVolumeFade(fieldSound, 64, 0);
    }
    wk->data.recReadComplete = setup->recReadComplete;
    GFL_HeapFree(setup->playerInfo[0]);
    BtlSetup_FreeForRecord(setup);
    BtlSetup_Free(setup);
    BrMainProcSys_SetNext(work, BR_MAIN_PROCID_CORE);
}

// A battle video plays with the music of a link battle: any other battle music becomes the Wi-Fi trainer battle's,
// and any other victory music the second one
static void BrMain_CheckBgm(u16 *bgm, u16 *winBgm) {
    u16 win = *winBgm;

    switch (*bgm) {
    case SEQ_BGM_VS_SUBWAY_TRAINER:
    case SEQ_BGM_VS_CHAMP:
    case SEQ_BGM_VS_WCS:
    case SEQ_BGM_VS_TRAINER_WIFI:
        break;
    case SEQ_BGM_VS_TRAINER_M:
    case SEQ_BGM_VS_TRAINER_S:
    default:
        *bgm = SEQ_BGM_VS_TRAINER_WIFI;
        break;
    }
    if (win != SEQ_BGM_WIN2 && win != SEQ_BGM_WIN5) {
        *winBgm = SEQ_BGM_WIN2;
    }
}
