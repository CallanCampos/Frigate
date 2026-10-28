#include "app/wifibattlematch_subproc.h"
#include "types.h"
#include "app/livebattlematch_irc.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/wifibattlematch_net.h"
#include "battle/regulation.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_sync.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"

#define HEAPID_WIFIBATTLEMATCH_SUBPROC 0x58

typedef struct {
    u32 state;
    PokeParty *party;
    PokeListParam pokeList;
    PStatusParam status;
    // Frames counted toward the next second of the party list's time limit
    u8 frames;
    u32 lastVBlankCount;
    GameProcManager *procManager;
    WifiBattleMatchListParam param;
    HeapID heapId;
} WifiBattleMatchListWork;

typedef struct {
    // A WifiBattleMatchNet or a LiveBattleMatchIrc, by the mode
    void *link;
} WifiBattleMatchExchangeWork;

static BOOL WifiBattleMatchList_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchList_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchList_Main(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchList_SetupPokeList(WifiBattleMatchListParam *param, WifiBattleMatchListWork *wk,
                                              PokeListParam *pokeList);
static void WifiBattleMatchList_SetupStatus(WifiBattleMatchListParam *param, WifiBattleMatchListWork *wk,
                                            PStatusParam *status);
static BOOL WifiBattleMatchExchange_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchExchange_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchExchange_Main(GameProc *proc, u32 *state, void *param, void *work);
static void WifiBattleMatchExchange_InitWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                             HeapID heapId);
static BOOL WifiBattleMatchExchange_MainWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                             u32 *state);
static void WifiBattleMatchExchange_ExitWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param);
static void WifiBattleMatchExchange_InitIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                            HeapID heapId);
static BOOL WifiBattleMatchExchange_MainIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                            u32 *state);
static void WifiBattleMatchExchange_ExitIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param);

const GameProcFunctions WIFIBATTLEMATCH_LIST_PROC_FUNCTIONS = {
    WifiBattleMatchList_Init,
    WifiBattleMatchList_Main,
    WifiBattleMatchList_Exit,
};

const GameProcFunctions WIFIBATTLEMATCH_EXCHANGE_PROC_FUNCTIONS = {
    WifiBattleMatchExchange_Init,
    WifiBattleMatchExchange_Main,
    WifiBattleMatchExchange_Exit,
};

static BOOL WifiBattleMatchList_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchListParam *list = param;
    WifiBattleMatchListWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_SUBPROC, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchListWork), HEAPID_WIFIBATTLEMATCH_SUBPROC);
    MI_CpuFill8(wk, 0, sizeof(WifiBattleMatchListWork));
    wk->param = *list;
    wk->heapId = HEAPID_WIFIBATTLEMATCH_SUBPROC;
    wk->procManager = CreateGameProcManager(wk->heapId);
    wk->party = list->party;
    WifiBattleMatchList_SetupPokeList(list, wk, &wk->pokeList);
    WifiBattleMatchList_SetupStatus(list, wk, &wk->status);
    wk->frames = 0;
    wk->lastVBlankCount = OS_GetVBlankCount();
    wk->state = 0;
    list->result = 0;
    return TRUE;
}

static BOOL WifiBattleMatchList_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchListParam *list = param;
    WifiBattleMatchListWork *wk = work;
    u8 i;

    for (i = 0; i < 6; i++) {
        if (wk->pokeList.picked[i] != 0) {
            PokeParty_AddPkm(list->selected, PokeParty_GetPkm(list->party, wk->pokeList.picked[i] - 1));
        }
    }
    NetSync_Exit((NetSyncWork *)&wk->pokeList);
    GFL_OvlUnload(OVERLAY_ID(164));
    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_SUBPROC);
    return TRUE;
}

static BOOL WifiBattleMatchList_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchListParam *list = param;
    WifiBattleMatchListWork *wk = work;

    if (wk->pokeList.timerEnabled == TRUE && wk->pokeList.timeLeft != 0) {
        u32 vblankCount = OS_GetVBlankCount();
        wk->frames += (u8)(vblankCount - wk->lastVBlankCount);
        wk->lastVBlankCount = vblankCount;
        if (wk->frames > 60) {
            wk->pokeList.timeLeft--;
            wk->frames -= 60;
        }
    }

    // A failed connection ends the screens
    if (GFL_NetIsInitialized()) {
        if (GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC) {
            if (GFL_NetErrCheck()) {
                GFL_NetErrMarkShown();
                list->result = 2;
                wk->pokeList.unk73 = 1;
                wk->status.forceExit = TRUE;
            }
        } else {
            switch (DWCRapCommon_CheckError(1, 1)) {
            case DWCRAP_RESULT_OK:
            case DWCRAP_RESULT_NET_ERROR:
                break;
            case DWCRAP_RESULT_TIMEOUT:
                list->result = 3;
                wk->pokeList.unk73 = 1;
                wk->status.forceExit = TRUE;
                break;
            case DWCRAP_RESULT_DISCONNECTED:
            case DWCRAP_RESULT_FAILED:
            default:
                list->result = 1;
                wk->pokeList.unk73 = 1;
                wk->status.forceExit = TRUE;
                break;
            }
        }
    }

    switch (wk->state) {
    case 0:
        GFL_OvlLoad(OVERLAY_ID(165));
        QueueGameProc(wk->procManager, OVERLAY_NONE, &POKELIST_PROC_FUNCTIONS, &wk->pokeList);
        wk->state = 1;
        break;
    case 1:
        if (!GFL_ProcMgrUpdate(wk->procManager)) {
            wk->state = 2;
        }
        break;
    case 2:
        GFL_OvlUnload(OVERLAY_ID(165));
        if (wk->pokeList.result == 1) {
            wk->status.partyIndex = wk->pokeList.index;
            wk->state = 3;
        } else {
            return TRUE;
        }
        break;
    case 3:
        GFL_OvlLoad(OVERLAY_PSTATUS);
        QueueGameProc(wk->procManager, OVERLAY_NONE, &PSTATUS_PROC_FUNCTIONS, &wk->status);
        wk->state = 4;
        break;
    case 4:
        if (wk->pokeList.timerEnabled == TRUE && wk->pokeList.timeLeft == 0) {
            wk->status.forceExit = TRUE;
        }
        if (!GFL_ProcMgrUpdate(wk->procManager)) {
            wk->state = 5;
        }
        break;
    case 5:
        GFL_OvlUnload(OVERLAY_PSTATUS);
        if (!wk->pokeList.unk73) {
            wk->pokeList.index = wk->status.partyIndex;
            wk->state = 0;
        } else {
            return TRUE;
        }
        break;
    }

    if (!wk->pokeList.unk73) {
        NetSync_Update((NetSyncWork *)&wk->pokeList);
    }
    return FALSE;
}

static void WifiBattleMatchList_SetupPokeList(WifiBattleMatchListParam *param, WifiBattleMatchListWork *wk,
                                              PokeListParam *pokeList) {
    Regulation *regulation = param->regulation;
    u8 i;

    GameData_GetSaveControl(param->gameData);
    PokeListParam_Setup(pokeList, param->gameData, 0x1a, wk->party);
    pokeList->regulation = regulation;
    switch (regulation->battleType) {
    case 0:
        pokeList->unk48 = 0;
        pokeList->partnerCount = 1;
        break;
    case 1:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 2:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 3:
        pokeList->unk48 = 1;
        pokeList->partnerCount = 1;
        break;
    case 4:
        pokeList->unk48 = 2;
        pokeList->partnerCount = 2;
        break;
    }
    pokeList->item = 0;
    pokeList->move = 0;
    pokeList->moveSlot = 0;
    pokeList->index = 0;
    pokeList->result = 0;
    for (i = 0; i < 6; i++) {
        pokeList->picked[i] = 0;
    }
    pokeList->unk64 = 0;
    pokeList->learnIndex = 0;
    pokeList->unk68 = 0;
    pokeList->partners[1].party = param->otherParty;
    pokeList->partners[1].name = param->otherName;
    pokeList->partners[1].gender = param->otherGender;
    pokeList->showPartners = regulation->showPartners;
    if (regulation->timeLimit != 0) {
        pokeList->timerEnabled = TRUE;
        pokeList->timeLeft = regulation->timeLimit;
    } else {
        pokeList->timerEnabled = FALSE;
        pokeList->timeLeft = 0;
    }
    GFL_OvlLoad(OVERLAY_ID(164));
    NetSync_Init((NetSyncWork *)pokeList);
}

static void WifiBattleMatchList_SetupStatus(WifiBattleMatchListParam *param, WifiBattleMatchListWork *wk,
                                            PStatusParam *status) {
    SaveControl *save = GameData_GetSaveControl(param->gameData);
    PokeDexSave *pokedex;

    GetGameDataPlayerInfo(param->gameData);
    pokedex = GameData_GetPokedex(param->gameData);
    status->party = wk->party;
    status->gameData = param->gameData;
    status->trainerData = getTrainerDataBlkAddress(save);
    status->dataType = PSTATUS_DATA_PARTY;
    status->mode = PSTATUS_MODE_1;
    status->partyCount = PokeParty_GetPkmCount(wk->party);
    status->partyIndex = 0;
    status->page = PSTATUS_PAGE_INFO;
    status->slot = 0;
    status->result = 0;
    status->move = 0;
    status->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    status->fromFieldMenu = FALSE;
    status->forceExit = FALSE;
}

static BOOL WifiBattleMatchExchange_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchExchangeParam *exchange = param;
    WifiBattleMatchExchangeWork *wk;
    HeapID heapId = HEAPID_WIFIBATTLEMATCH_SUBPROC;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFIBATTLEMATCH_SUBPROC, 0x10000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiBattleMatchExchangeWork), HEAPID_WIFIBATTLEMATCH_SUBPROC);
    MI_CpuFill8(wk, 0, sizeof(WifiBattleMatchExchangeWork));
    exchange->result = 0;
    switch (exchange->mode) {
    case WIFIBATTLEMATCH_EXCHANGE_WIFI:
        WifiBattleMatchExchange_InitWifi(wk, exchange, heapId);
        break;
    case WIFIBATTLEMATCH_EXCHANGE_IRC:
        WifiBattleMatchExchange_InitIrc(wk, exchange, heapId);
        break;
    }
    return TRUE;
}

static BOOL WifiBattleMatchExchange_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchExchangeParam *exchange = param;

    switch (exchange->mode) {
    case WIFIBATTLEMATCH_EXCHANGE_WIFI:
        WifiBattleMatchExchange_ExitWifi(work, exchange);
        break;
    case WIFIBATTLEMATCH_EXCHANGE_IRC:
        WifiBattleMatchExchange_ExitIrc(work, exchange);
        break;
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFIBATTLEMATCH_SUBPROC);
    return TRUE;
}

static BOOL WifiBattleMatchExchange_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchExchangeParam *exchange = param;
    BOOL done;

    switch (exchange->mode) {
    case WIFIBATTLEMATCH_EXCHANGE_WIFI:
        done = WifiBattleMatchExchange_MainWifi(work, exchange, state);
        break;
    case WIFIBATTLEMATCH_EXCHANGE_IRC:
        done = WifiBattleMatchExchange_MainIrc(work, exchange, state);
        break;
    }
    if (done) {
        return TRUE;
    }
    return FALSE;
}

static void WifiBattleMatchExchange_InitWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                             HeapID heapId) {
}

static BOOL WifiBattleMatchExchange_MainWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                             u32 *state) {
    if (GFL_NetIsInitialized()) {
        switch (DWCRapCommon_CheckError(1, 1)) {
        case DWCRAP_RESULT_OK:
            break;
        case DWCRAP_RESULT_DISCONNECTED:
        case DWCRAP_RESULT_TIMEOUT:
            param->result = 3;
            return TRUE;
        case DWCRAP_RESULT_FAILED:
        case DWCRAP_RESULT_NET_ERROR:
        default:
            param->result = 1;
            return TRUE;
        }
    }

    switch (*state) {
    case 0:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x10, 0x24);
        (*state)++;
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x10, 0x24)) {
            (*state)++;
        }
        break;
    case 2:
        GFL_OvlLoad(OVERLAY_ID(139));
        GFL_OvlLoad(OVERLAY_ID(189));
        GFL_OvlLoad(OVERLAY_ID(260));
        wk->link = WifiBattleMatchNet_Create(param->unk14, *param->gameData, 0, HEAPID_WIFIBATTLEMATCH_SUBPROC);
        (*state)++;
        break;
    case 3:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x11, 0x24);
        (*state)++;
        break;
    case 4:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x11, 0x24)) {
            (*state)++;
        }
        break;
    case 5:
        if (WifiBattleMatchNet_SendParty(wk->link, param->party)) {
            (*state)++;
        }
        break;
    case 6:
        if (WifiBattleMatchNet_TakeParty(wk->link, param->otherParty)) {
            (*state)++;
        }
        break;
    case 7:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x12, 0x24);
        (*state)++;
        break;
    case 8:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x12, 0x24)) {
            (*state)++;
        }
        break;
    case 9:
        WifiBattleMatchNet_Delete(wk->link);
        wk->link = NULL;
        GFL_OvlUnload(OVERLAY_ID(260));
        GFL_OvlUnload(OVERLAY_ID(189));
        GFL_OvlUnload(OVERLAY_ID(139));
        (*state)++;
        break;
    case 10:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x13, 0x24);
        (*state)++;
        break;
    case 11:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x13, 0x24)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void WifiBattleMatchExchange_ExitWifi(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param) {
    if (wk->link != NULL) {
        WifiBattleMatchNet_Delete(wk->link);
        GFL_OvlUnload(OVERLAY_ID(260));
        GFL_OvlUnload(OVERLAY_ID(189));
        GFL_OvlUnload(OVERLAY_ID(139));
    }
}

static void WifiBattleMatchExchange_InitIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                            HeapID heapId) {
    GFL_OvlLoad(OVERLAY_ID(261));
    wk->link = LiveBattleMatchIrc_Create(*param->gameData, heapId);
}

static BOOL WifiBattleMatchExchange_MainIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param,
                                            u32 *state) {
    if (GFL_NetErrCheck()) {
        GFL_NetErrMarkShown();
        param->result = 2;
        return TRUE;
    }

    switch (*state) {
    case 0:
        (*state)++;
        break;
    case 1:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x11, 0x1f);
        (*state)++;
        break;
    case 2:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x11, 0x1f)) {
            (*state)++;
        }
        break;
    case 3:
        if (LiveBattleMatchIrc_SendParty(wk->link, param->party)) {
            (*state)++;
        }
        break;
    case 4:
        if (LiveBattleMatchIrc_TakeParty(wk->link, param->otherParty)) {
            (*state)++;
        }
        break;
    case 5:
        return TRUE;
    }
    LiveBattleMatchIrc_Main(wk->link);
    return FALSE;
}

static void WifiBattleMatchExchange_ExitIrc(WifiBattleMatchExchangeWork *wk, WifiBattleMatchExchangeParam *param) {
    LiveBattleMatchIrc_Delete(wk->link);
    GFL_OvlUnload(OVERLAY_ID(261));
}
