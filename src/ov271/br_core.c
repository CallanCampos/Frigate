// The Battle Recorder's core proc, which sets up the systems its screens share and runs the screens. The name is a
// guess: the ROM gives none, and br_main.c is overlay 272's proc, which runs this one

#include "types.h"
#include "app/battle_recorder/br_core.h"
#include "field/field_sound.h"
#include "gfl/fade.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/battle_rec.h"
#include "save/join_avenue.h"
#include "save/musical_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_data.h"

typedef struct {
    BrGraphic *graphic;
    BrRes *res;
    BrProcSys *procSys;
    BrFade *fade;
    BrSidebar *sidebar;
    BrNet *net;
    BrCoreParam *param;
} BrCoreWork;

static BOOL BrCore_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrCore_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BrCore_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static void BrCore_StartBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_StartAfter(void *param, void *work);
static void BrCore_MenuBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_MenuAfter(void *param, void *work);
static void BrCore_RecordBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_RecordAfter(void *param, void *work);
static void BrCore_BtlSubwayBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BtlSubwayAfter(void *param, void *work);
static void BrCore_RndMatchBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_RndMatchAfter(void *param, void *work);
static void BrCore_BvRankBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BvRankAfter(void *param, void *work);
static void BrCore_BvSearchBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BvSearchAfter(void *param, void *work);
static void BrCore_CodeInBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_CodeInAfter(void *param, void *work);
static void BrCore_BvSendBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BvSendAfter(void *param, void *work);
static void BrCore_BvDeleteBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BvDeleteAfter(void *param, void *work);
static void BrCore_BvSaveBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_BvSaveAfter(void *param, void *work);
static void BrCore_MusicalLookBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_MusicalLookAfter(void *param, void *work);
static void BrCore_MusicalSendBefore(void *param, void *work, const void *preParam, u32 preID);
static void BrCore_MusicalSendAfter(void *param, void *work);
static void BrCore_LoadRecordInfo(BrRecordInfo *info, BOOL keepLoaded, GameData *gameData, HeapID heapId);

const GameProcFunctions BR_CORE_PROC_FUNCTIONS = {
    BrCore_ProcInit,
    BrCore_ProcMain,
    BrCore_ProcExit,
};

static const BrProcData sBrProcTable[BR_PROCID_MAX] = {
    [BR_PROCID_START] = { &BR_START_PROC_FUNCTIONS, sizeof(BrStartProcParam), OVERLAY_ID(268), BrCore_StartBefore,
                          BrCore_StartAfter },
    [BR_PROCID_MENU] = { &BR_MENU_PROC_FUNCTIONS, sizeof(BrMenuProcParam), OVERLAY_ID(268), BrCore_MenuBefore,
                         BrCore_MenuAfter },
    [BR_PROCID_RECORD] = { &BR_RECORD_PROC_FUNCTIONS, sizeof(BrRecordProcParam), OVERLAY_ID(268), BrCore_RecordBefore,
                           BrCore_RecordAfter },
    [BR_PROCID_BTLSUBWAY] = { &BR_BTLSUBWAY_PROC_FUNCTIONS, sizeof(BrBtlSubwayProcParam), OVERLAY_ID(268),
                              BrCore_BtlSubwayBefore, BrCore_BtlSubwayAfter },
    [BR_PROCID_RNDMATCH] = { &BR_RNDMATCH_PROC_FUNCTIONS, sizeof(BrRndMatchProcParam), OVERLAY_ID(268), BrCore_RndMatchBefore,
                             BrCore_RndMatchAfter },
    [BR_PROCID_BV_RANK] = { &BR_BVRANK_PROC_FUNCTIONS, sizeof(BrBvRankProcParam), OVERLAY_ID(268), BrCore_BvRankBefore,
                            BrCore_BvRankAfter },
    [BR_PROCID_BV_SEARCH] = { &BR_BVSEARCH_PROC_FUNCTIONS, sizeof(BrBvSearchProcParam), OVERLAY_ID(268), BrCore_BvSearchBefore,
                              BrCore_BvSearchAfter },
    [BR_PROCID_CODEIN] = { &BR_CODEIN_PROC_FUNCTIONS, sizeof(BrCodeInProcParam), OVERLAY_ID(268), BrCore_CodeInBefore,
                           BrCore_CodeInAfter },
    [BR_PROCID_BV_SEND] = { &BR_BVSEND_PROC_FUNCTIONS, sizeof(BrBvSendProcParam), OVERLAY_ID(268), BrCore_BvSendBefore,
                            BrCore_BvSendAfter },
    [BR_PROCID_BV_DELETE] = { &BR_BVDELETE_PROC_FUNCTIONS, sizeof(BrBvDeleteProcParam), OVERLAY_ID(268), BrCore_BvDeleteBefore,
                              BrCore_BvDeleteAfter },
    [BR_PROCID_BV_SAVE] = { &BR_BVSAVE_PROC_FUNCTIONS, sizeof(BrBvSaveProcParam), OVERLAY_ID(268), BrCore_BvSaveBefore,
                            BrCore_BvSaveAfter },
    [BR_PROCID_MUSICAL_LOOK] = { &BR_MUSICAL_LOOK_PROC_FUNCTIONS, sizeof(BrMusicalLookProcParam), OVERLAY_ID(270),
                                 BrCore_MusicalLookBefore, BrCore_MusicalLookAfter },
    [BR_PROCID_MUSICAL_SEND] = { &BR_MUSICAL_SEND_PROC_FUNCTIONS, sizeof(BrMusicalSendProcParam), OVERLAY_ID(269),
                                 BrCore_MusicalSendBefore, BrCore_MusicalSendAfter },
};

static BOOL BrCore_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BrCoreParam *coreParam = param;
    BrCoreWork *wk;
    u8 color;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE_RECORDER, 0x90000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BrCoreWork), HEAPID_BATTLE_RECORDER);
    sys_memset(wk, 0, sizeof(BrCoreWork));
    wk->param = coreParam;
    coreParam->mainParam->result = 0;

    wk->graphic = BrGraphic_Init(wk->param->mainParam->mode == BR_MODE_GLOBAL_MUSICAL, 1, HEAPID_BATTLE_RECORDER);
    color = func_0200cb30(getTrainerCardDataBlkAddress(wk->param->mainParam->gameData));
    wk->res = BrRes_Init(color, wk->param->mainParam->mode == BR_MODE_BROWSE, HEAPID_BATTLE_RECORDER);
    BrRes_LoadBG(wk->res, 0, HEAPID_BATTLE_RECORDER);
    BrRes_LoadBG(wk->res, 1, HEAPID_BATTLE_RECORDER);
    wk->procSys = BrProcSys_Init(BR_PROCID_START, sBrProcTable, BR_PROCID_MAX, wk, &wk->param->data->procRecovery,
                                 HEAPID_BATTLE_RECORDER);
    wk->fade = BrFade_Init(HEAPID_BATTLE_RECORDER);
    BrFade_LoadPltt(wk->fade);
    BrFade_SetColor(wk->fade, BrRes_GetFadeColor(wk->res));
    if (wk->param->mode == BR_CORE_MODE_INIT) {
        BrFade_FillColor(wk->fade, BR_FADE_DISPLAY_BOTH);
    }
    wk->sidebar = BrSidebar_Init(BrGraphic_GetClunit(wk->graphic), wk->fade, wk->res, HEAPID_BATTLE_RECORDER);
    BrCore_LoadRecordInfo(&wk->param->data->recordInfo, TRUE, wk->param->mainParam->gameData, HEAPID_BATTLE_RECORDER);

    if (wk->param->mainParam->mode != BR_MODE_BROWSE) {
        wk->net =
            BrNet_Init(wk->param->mainParam->gameData, wk->param->mainParam->unk8, HEAPID_BATTLE_RECORDER);
    } else {
        GFL_OvlLoad(OVERLAY_ID(201));
    }
    if (func_02042788()) {
        func_02042ba8(FALSE, HEAPID_BATTLE_RECORDER);
    }

    if (wk->param->mainParam->mode == BR_MODE_BROWSE) {
        if (wk->param->mode != BR_CORE_MODE_RETURN) {
            FieldSnd_SetPlayerVolumeFade(GameData_GetFieldSoundSystem(wk->param->mainParam->gameData), 64, 0);
        }
    } else {
        GFL_SndBGMPlay(0x484, 0xffff);
    }

    if (wk->param->mode == BR_CORE_MODE_RETURN) {
        BrSidebar_SetEndPos(wk->sidebar);
        BrSidebar_StartBound(wk->sidebar);
        BrFade_SetAlpha(wk->fade, BR_FADE_DISPLAY_BOTH, 0);
    }
    if (wk->param->mainParam->mode == BR_MODE_GLOBAL_BV && wk->param->mode == BR_CORE_MODE_INIT) {
        func_02038bc8(0x13);
    }
    return TRUE;
}

static BOOL BrCore_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BrCoreWork *wk = work;

    if (wk->param->mainParam->mode == BR_MODE_BROWSE) {
        FieldSnd_SetPlayerVolumeFade(GameData_GetFieldSoundSystem(wk->param->mainParam->gameData), 127, 0);
    }

    if (wk->net != NULL) {
        if (BrNet_IsFailed(wk->net)) {
            wk->param->mainParam->result = 2;
        }
        BrNet_Exit(wk->net);
        wk->net = NULL;
    } else {
        GFL_OvlUnload(OVERLAY_ID(201));
    }

    BrSidebar_Exit(wk->sidebar, wk->res);
    BrFade_Exit(wk->fade);
    BrProcSys_Exit(wk->procSys);
    if (wk->param->mainParam->mode == BR_MODE_BROWSE) {
        TrainerCardSave *trainerCard = getTrainerCardDataBlkAddress(wk->param->mainParam->gameData);
        func_0200cb3c(trainerCard, BrRes_GetColor(wk->res));
    }
    BrRes_UnloadBG(wk->res, 0);
    BrRes_UnloadBG(wk->res, 1);
    BrRes_Exit(wk->res);
    BrGraphic_Exit(wk->graphic);

    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_RECORDER);
    return TRUE;
}

static BOOL BrCore_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BrCoreWork *wk = work;

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = 2;
        break;
    case 2:
        if (!GFL_FadeIsRunning()) {
            *state = 3;
        }
        break;
    case 3:
        BrFade_Main(wk->fade);
        BrSidebar_Main(wk->sidebar);
        BrProcSys_Main(wk->procSys);
        BrGraphic_Main(wk->graphic);
        if (wk->net != NULL) {
            BrNet_Main(wk->net);
        }
        if (BrProcSys_IsEnd(wk->procSys)) {
            *state = 4;
        }
        break;
    case 4:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        *state = 5;
        break;
    case 5:
        if (!GFL_FadeIsRunning()) {
            *state = 6;
        }
        break;
    case 6:
        return TRUE;
    }
    return FALSE;
}

static void BrCore_StartBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrStartProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->sidebar = wk->sidebar;
    if (preID == BR_PROCID_MENU) {
        p->mode = 1;
    } else if (wk->param->mode == BR_CORE_MODE_INIT) {
        p->mode = 0;
    } else {
        p->mode = 2;
    }
}

static void BrCore_StartAfter(void *param, void *work) {
}

static void BrCore_MenuBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrMenuProcParam *p = param;
    BrCoreWork *wk = work;

    p->fadeType = 2;
    p->result = &wk->param->mainParam->result;
    p->btnRecovery = &wk->param->data->btnRecovery;

    if (wk->param->mode == BR_CORE_MODE_RETURN) {
        switch (wk->param->mainParam->mode) {
        case BR_MODE_BROWSE:
            p->menuID = 1;
            break;
        case BR_MODE_GLOBAL_BV:
            p->menuID = 7;
            break;
        case BR_MODE_GLOBAL_MUSICAL:
            p->menuID = 10;
            break;
        }
    } else {
        switch (preID) {
        case BR_PROCID_RECORD: {
            const BrRecordProcParam *recordParam = preParam;

            switch (recordParam->mode) {
            case BR_RECORD_MODE_MINE:
                p->menuID = 1;
                break;
            case BR_RECORD_MODE_OTHER1:
            case BR_RECORD_MODE_OTHER2:
            case BR_RECORD_MODE_OTHER3:
                p->menuID = 2;
                break;
            case BR_RECORD_MODE_BROWSE:
                GFL_ASSERT(0);
                p->menuID = 7;
                break;
            case BR_RECORD_MODE_CODEIN:
                GFL_ASSERT(0);
                p->menuID = 7;
                break;
            }
            break;
        }
        case BR_PROCID_BV_RANK:
            p->menuID = 7;
            break;
        case BR_PROCID_BV_SEARCH:
        case BR_PROCID_CODEIN:
            p->menuID = 6;
            break;
        case BR_PROCID_MUSICAL_SEND: {
            const BrMusicalSendProcParam *musicalParam = preParam;

            if (!musicalParam->isSend) {
                p->fadeType = 4;
            }
            p->menuID = 10;
            break;
        }
        default:
            switch (wk->param->mainParam->mode) {
            case BR_MODE_BROWSE:
                p->menuID = 0;
                break;
            case BR_MODE_GLOBAL_BV:
                p->menuID = 5;
                break;
            case BR_MODE_GLOBAL_MUSICAL:
                p->menuID = 10;
                break;
            }
        }
    }

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->recordInfo = &wk->param->data->recordInfo;
}

static void BrCore_MenuAfter(void *param, void *work) {
    BrMenuProcParam *p = param;
    BrBtnRecovery *recovery = p->btnRecovery;
    u16 last = recovery->stack_num - 1;

    if (recovery->stack[last].menuID == 8) {
        recovery->stack_num = recovery->stack_num - 1;
    }
}

static void BrCore_RecordBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrRecordProcParam *p = param;
    BrCoreWork *wk = work;
    u32 result;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->net = wk->net;
    p->gameData = wk->param->mainParam->gameData;
    p->unk2C = &wk->param->data->unk_1720;
    p->isRecovery = FALSE;
    p->recordInfo = &wk->param->data->recordInfo;
    p->unk3C = &wk->param->data->unk_17bc;

    switch (preID) {
    case BR_PROCID_MENU: {
        const BrMenuProcParam *menuParam = preParam;
        GameData *gameData;

        p->mode = menuParam->nextMode;
        p->outline = NULL;
        wk->param->data->recordMode = menuParam->nextMode;
        gameData = wk->param->mainParam->gameData;
        func_0200bc9c(GameData_GetSaveControl(gameData), HEAPID_BATTLE_RECORDER_SYS, &result, p->mode);
        if (result == 1) {
            if (p->mode == BR_RECORD_MODE_MINE) {
                GdsProfile_SetFromGameData(getVSPlayerAllocation(), gameData);
            }
        } else {
            GFL_ASSERT(0);
        }
        break;
    }
    case BR_PROCID_BV_RANK:
        p->mode = BR_RECORD_MODE_BROWSE;
        p->outline = &wk->param->data->outline;
        wk->param->data->recordMode = BR_RECORD_MODE_BROWSE;
        break;
    case BR_PROCID_CODEIN: {
        const BrCodeInProcParam *codeInParam = preParam;

        p->mode = BR_RECORD_MODE_CODEIN;
        p->videoNumber = codeInParam->videoNumber;
        p->outline = NULL;
        wk->param->data->recordMode = BR_RECORD_MODE_CODEIN;
        break;
    }
    case BR_PROCID_BV_SAVE: {
        const BrBvSaveProcParam *saveParam = preParam;

        p->mode = wk->param->data->recordMode;
        p->outline = &wk->param->data->outline;
        p->videoNumber = saveParam->videoNumber;
        p->isRecovery = TRUE;
        break;
    }
    case BR_PROCID_START:
        break;
    case BR_PROCID_RECOVERY:
        p->mode = wk->param->data->recordMode;
        p->outline = &wk->param->data->outline;
        p->isRecovery = TRUE;
        break;
    }
}

static void BrCore_RecordAfter(void *param, void *work) {
    BrRecordProcParam *p = param;
    BrCoreWork *wk = work;

    if (p->result == 1) {
        wk->param->result = 1;
    }
}

static void BrCore_BtlSubwayBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBtlSubwayProcParam *p = param;
    BrCoreWork *wk = work;
    SaveControl *save = GameData_GetSaveControl(wk->param->mainParam->gameData);

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    p->gameData = wk->param->mainParam->gameData;
}

static void BrCore_BtlSubwayAfter(void *param, void *work) {
}

static void BrCore_RndMatchBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrRndMatchProcParam *p = param;
    BrCoreWork *wk = work;
    SaveControl *save = GameData_GetSaveControl(wk->param->mainParam->gameData);

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->gameData = wk->param->mainParam->gameData;
    p->record = func_0200f2d4(getRecordBlkAddress(save));
}

static void BrCore_RndMatchAfter(void *param, void *work) {
}

static void BrCore_BvRankBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBvRankProcParam *p = param;
    BrCoreWork *wk = work;

    p->p_outline = &wk->param->data->outline;
    p->listPos = &wk->param->data->bvRankListPos;
    if (preID == BR_PROCID_MENU) {
        const BrMenuProcParam *menuParam = preParam;

        p->mode = menuParam->nextMode;
        wk->param->data->bvRankMode = menuParam->nextMode;
        sys_memset(p->listPos, 0, sizeof(BrListPos));
    } else if (preID == BR_PROCID_BV_SEARCH) {
        const BrBvSearchProcParam *searchParam = preParam;

        p->mode = 3;
        p->search = searchParam->search;
        wk->param->data->bvRankMode = 3;
        sys_memset(p->listPos, 0, sizeof(BrListPos));
    } else if (preID == BR_PROCID_RECORD) {
        p->mode = wk->param->data->bvRankMode;
        p->isReturn = TRUE;
    }
    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->net = wk->net;
}

static void BrCore_BvRankAfter(void *param, void *work) {
}

static void BrCore_BvSearchBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBvSearchProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->gameData = wk->param->mainParam->gameData;
}

static void BrCore_BvSearchAfter(void *param, void *work) {
}

static void BrCore_CodeInBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrCodeInProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
}

static void BrCore_CodeInAfter(void *param, void *work) {
}

static void BrCore_BvSendBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBvSendProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->net = wk->net;
    p->gameData = wk->param->mainParam->gameData;
}

static void BrCore_BvSendAfter(void *param, void *work) {
}

static void BrCore_BvDeleteBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBvDeleteProcParam *p = param;
    BrCoreWork *wk = work;

    if (preID == BR_PROCID_MENU) {
        const BrMenuProcParam *menuParam = preParam;

        p->mode = menuParam->nextMode;
        p->res = wk->res;
        p->fade = wk->fade;
        p->procSys = wk->procSys;
        p->unit = BrGraphic_GetClunit(wk->graphic);
        p->gameData = wk->param->mainParam->gameData;
        p->recordInfo = &wk->param->data->recordInfo;
    } else {
        GFL_ASSERT(0);
    }
}

static void BrCore_BvDeleteAfter(void *param, void *work) {
    BrBvDeleteProcParam *p = param;
    BrCoreWork *wk = work;

    if (p->isDelete) {
        BrCore_LoadRecordInfo(&wk->param->data->recordInfo, FALSE, wk->param->mainParam->gameData,
                              HEAPID_BATTLE_RECORDER);
    }
}

static void BrCore_BvSaveBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrBvSaveProcParam *p = param;
    BrCoreWork *wk = work;
    const BrRecordProcParam *recordParam = preParam;

    GFL_ASSERT(preID == BR_PROCID_RECORD);
    p->res = wk->res;
    p->fade = wk->fade;
    p->net = wk->net;
    p->procSys = wk->procSys;
    p->unit = BrGraphic_GetClunit(wk->graphic);
    p->videoNumber = recordParam->videoNumber;
    p->gameData = wk->param->mainParam->gameData;
    p->recordInfo = &wk->param->data->recordInfo;
    p->unk24 = recordParam->unk4;
}

static void BrCore_BvSaveAfter(void *param, void *work) {
    BrBvSaveProcParam *p = param;
    BrCoreWork *wk = work;
    u32 result;

    if (p->isSave) {
        BrCore_LoadRecordInfo(&wk->param->data->recordInfo, FALSE, wk->param->mainParam->gameData,
                              HEAPID_BATTLE_RECORDER);
        func_0200bc9c(GameData_GetSaveControl(wk->param->mainParam->gameData), HEAPID_TAIL(HEAPID_BATTLE_RECORDER),
                      &result, p->saveSlot);
    }
}

static void BrCore_MusicalLookBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrMusicalLookProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->graphic = wk->graphic;
    p->net = wk->net;
    p->sidebar = wk->sidebar;
    p->gameData = wk->param->mainParam->gameData;
}

static void BrCore_MusicalLookAfter(void *param, void *work) {
}

static void BrCore_MusicalSendBefore(void *param, void *work, const void *preParam, u32 preID) {
    BrMusicalSendProcParam *p = param;
    BrCoreWork *wk = work;

    p->res = wk->res;
    p->fade = wk->fade;
    p->procSys = wk->procSys;
    p->graphic = wk->graphic;
    p->net = wk->net;
    p->sidebar = wk->sidebar;
    p->gameData = wk->param->mainParam->gameData;
}

static void BrCore_MusicalSendAfter(void *param, void *work) {
}

// Loads the names and numbers of the saved videos, unless keepLoaded is set and they are loaded already
static void BrCore_LoadRecordInfo(BrRecordInfo *info, BOOL keepLoaded, GameData *gameData, HeapID heapId) {
    BOOL load = TRUE;
    SaveControl *save;
    int i;
    u32 result;

    if (keepLoaded && info->isInit) {
        load = FALSE;
    }
    if (load) {
        save = GameData_GetSaveControl(gameData);
        if (info->isInit) {
            for (i = 0; i < BR_RECORD_NUM; i++) {
                if (info->name[i] != NULL) {
                    GFL_StrBufFree(info->name[i]);
                    info->name[i] = NULL;
                }
            }
        }
        sys_memset(info, 0, sizeof(BrRecordInfo));

        for (i = 0; i < BR_RECORD_NUM; i++) {
            func_0200bc9c(save, HEAPID_TAIL(heapId), &result, i);
            if (result == 1) {
                GdsProfile *profile = getVSPlayerAllocation();

                if (i == 0) {
                    GdsProfile_SetFromGameData(profile, gameData);
                }
                info->isValid[i] = TRUE;
                info->name[i] = GdsProfile_CreateNameStrBuf(profile, HEAPID_BATTLE_RECORDER_SYS);
                info->sex[i] = GdsProfile_GetSex(profile);
                info->videoNumber[i] = func_0200c124(func_0200c0c0(), 4, 0);
            }
        }
        info->hasMusicalShot = func_0200ad4c(getAddressOfMusicalDataInfo(GameData_GetSaveControl(gameData)));
        info->isInit = TRUE;
    }
}
