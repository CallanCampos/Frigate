// The ROM names this file guru2.c.
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/spin_trade.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"

// spin_trade.c, which the ROM names: the proc of the Union Room's Spin Trade. It runs the receipt screen, where the
// group gathers, the party list on which each member picks an Egg, and the main screen with the disc. Names are ours

typedef struct SpinTradeCallWork SpinTradeCallWork;

struct SpinTradeCallWork {
    int seq;
    int partyIndex;
    SpinTradeParam *param;
    SpinTradeProcWork *g2p;
    GameData *gameData;
    u32 unk14;
    PokeListParam *pokeList;
    PStatusParam *pokeStatus;
    GameProcManager *procManager;
};

enum {
    SPIN_TRADE_SEQ_START_RECEIPT,
    SPIN_TRADE_SEQ_WAIT_RECEIPT,
    SPIN_TRADE_SEQ_WAIT_POKE_LIST,
    SPIN_TRADE_SEQ_WAIT_POKE_STATUS,
    SPIN_TRADE_SEQ_WAIT_MAIN,
    SPIN_TRADE_SEQ_END,
};

// The party list's mode for picking an Egg
#define SPIN_TRADE_POKELIST_MODE 0x15

static BOOL SpinTradeProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL SpinTradeProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL SpinTradeProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static SpinTradeProcWork *SpinTradeProcWork_Create(SpinTradeParam *param, u32 heapId);
static void SpinTradeProcWork_Free(SpinTradeProcWork *g2p);
static BOOL SpinTradeProcWork_IsReceiptDone(SpinTradeProcWork *g2p);
static BOOL SpinTradeSeq_StartReceipt(SpinTradeCallWork *wk);
static PokeListParam *SpinTrade_CreatePokeListParam(SpinTradeCallWork *wk, u32 mode, u32 index);
static BOOL SpinTradeSeq_WaitReceipt(SpinTradeCallWork *wk);
static BOOL SpinTradeSeq_WaitPokeList(SpinTradeCallWork *wk);
static BOOL SpinTradeSeq_WaitPokeStatus(SpinTradeCallWork *wk);
static BOOL SpinTradeSeq_WaitMain(SpinTradeCallWork *wk);
static BOOL SpinTradeSeq_End(SpinTradeCallWork *wk);

static const GameProcFunctions sMainProcFunctions = {
    SpinTradeMain_Init,
    SpinTradeMain_Main,
    SpinTradeMain_Exit,
};

static const GameProcFunctions sReceiptProcFunctions = {
    SpinTradeReceipt_Init,
    SpinTradeReceipt_Main,
    SpinTradeReceipt_Exit,
};

static BOOL (*const sSpinTradeSeqFuncs[])(SpinTradeCallWork *wk) = {
    SpinTradeSeq_StartReceipt,   SpinTradeSeq_WaitReceipt, SpinTradeSeq_WaitPokeList,
    SpinTradeSeq_WaitPokeStatus, SpinTradeSeq_WaitMain,    SpinTradeSeq_End,
};

const GameProcFunctions SPIN_TRADE_PROC_FUNCTIONS = {
    SpinTradeProc_Init,
    SpinTradeProc_Main,
    SpinTradeProc_Exit,
};

static BOOL SpinTradeProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeParam *g2param = param;
    SpinTradeCallWork *wk;

    wk = GFL_ProcInitSubsystem(proc, sizeof(SpinTradeCallWork), HEAPID_GAMEEVENT);
    MI_CpuFill8(wk, 0, sizeof(SpinTradeCallWork));
    wk->gameData = g2param->gameData;
    wk->param = g2param;
    wk->procManager = CreateGameProcManager(HEAPID_USER);
    return TRUE;
}

static BOOL SpinTradeProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeCallWork *wk = work;

    switch (GFL_ProcMgrUpdate(wk->procManager)) {
    case 0:
        if (sSpinTradeSeqFuncs[wk->seq](wk) == TRUE) {
            return TRUE;
        }
        break;
    case 1:
        break;
    case 2:
        break;
    }
    return FALSE;
}

static BOOL SpinTradeProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeCallWork *wk = work;

    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static SpinTradeProcWork *SpinTradeProcWork_Create(SpinTradeParam *param, u32 heapId) {
    SpinTradeProcWork *g2p;

    g2p = GFL_HeapAllocate(heapId, sizeof(SpinTradeProcWork), FALSE, "guru2.c", 0xab);
    MI_CpuFill8(g2p, 0, sizeof(SpinTradeProcWork));
    g2p->param = param;
    g2p->g2c = SpinTradeComm_Create(g2p, heapId);
    return g2p;
}

static void SpinTradeProcWork_Free(SpinTradeProcWork *g2p) {
    SpinTradeComm_End(g2p->g2c);
    SpinTradeComm_Free(g2p->g2c);
    GFL_HeapFree(g2p);
}

static BOOL SpinTradeProcWork_IsReceiptDone(SpinTradeProcWork *g2p) {
    return g2p->receiptDone;
}

static BOOL SpinTradeSeq_StartReceipt(SpinTradeCallWork *wk) {
    wk->g2p = SpinTradeProcWork_Create(wk->param, HEAPID_GAMEEVENT);
    wk->seq = SPIN_TRADE_SEQ_WAIT_RECEIPT;
    QueueGameProc(wk->procManager, OVERLAY_NONE, &sReceiptProcFunctions, wk->g2p);
    return FALSE;
}

static PokeListParam *SpinTrade_CreatePokeListParam(SpinTradeCallWork *wk, u32 mode, u32 index) {
    PokeListParam *pokeList;
    int i;

    pokeList = PokeListParam_Create(wk->gameData, mode, GameData_GetParty(wk->gameData), HEAPID_GAMEEVENT);
    pokeList->index = index;
    for (i = 0; i < PokeParty_GetPkmCount(pokeList->party); i++) {
        PokeParty_GetPkm(pokeList->party, i);
    }
    return pokeList;
}

static BOOL SpinTradeSeq_WaitReceipt(SpinTradeCallWork *wk) {
    if (!SpinTradeProcWork_IsReceiptDone(wk->g2p)) {
        wk->seq = SPIN_TRADE_SEQ_END;
    } else {
        GflNet_SetAutoExit(TRUE);
        wk->pokeList = SpinTrade_CreatePokeListParam(wk, SPIN_TRADE_POKELIST_MODE, 0);
        QueueGameProc(wk->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->pokeList);
        wk->g2p->state = SPIN_TRADE_PROC_STATE_POKE_LIST;
        wk->seq = SPIN_TRADE_SEQ_WAIT_POKE_LIST;
    }
    return FALSE;
}

static BOOL SpinTradeSeq_WaitPokeList(SpinTradeCallWork *wk) {
    u32 result = wk->pokeList->result;
    u32 index = wk->pokeList->index;

    GFL_HeapFree(wk->pokeList);
    if (result == 1) {
        PokeDexSave *pokedex;
        PokeParty *party;

        GetGameDataPlayerInfo(wk->gameData);
        pokedex = GameData_GetPokedex(wk->gameData);
        wk->pokeStatus = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), FALSE, "guru2.c", 0x147);
        MI_CpuFill8(wk->pokeStatus, 0, sizeof(PStatusParam));
        party = GameData_GetParty(wk->gameData);
        wk->pokeStatus->party = party;
        wk->pokeStatus->gameData = wk->gameData;
        wk->pokeStatus->dataType = 1;
        wk->pokeStatus->partyCount = PokeParty_GetPkmCount(party);
        wk->pokeStatus->partyIndex = 0;
        wk->pokeStatus->mode = PSTATUS_MODE_1;
        wk->pokeStatus->move = 0;
        wk->pokeStatus->page = PSTATUS_PAGE_INFO;
        wk->pokeStatus->isNationalDex = PokeDex_IsNationalObtained(pokedex);
        wk->partyIndex = index;
        wk->pokeStatus->partyIndex = index;
        QueueGameProc(wk->procManager, OVERLAY_ID(207), &PSTATUS_PROC_FUNCTIONS, wk->pokeStatus);
        wk->seq = SPIN_TRADE_SEQ_WAIT_POKE_STATUS;
    } else {
        wk->g2p->partyIndex = index;
        QueueGameProc(wk->procManager, OVERLAY_NONE, &sMainProcFunctions, wk->g2p);
        wk->g2p->state = SPIN_TRADE_PROC_STATE_MAIN;
        wk->seq = SPIN_TRADE_SEQ_WAIT_MAIN;
    }
    return FALSE;
}

static BOOL SpinTradeSeq_WaitPokeStatus(SpinTradeCallWork *wk) {
    u8 index = wk->pokeStatus->partyIndex;

    GFL_HeapFree(wk->pokeStatus);
    wk->pokeList = SpinTrade_CreatePokeListParam(wk, SPIN_TRADE_POKELIST_MODE, index);
    QueueGameProc(wk->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->pokeList);
    wk->seq = SPIN_TRADE_SEQ_WAIT_POKE_LIST;
    return FALSE;
}

static BOOL SpinTradeSeq_WaitMain(SpinTradeCallWork *wk) {
    wk->seq = SPIN_TRADE_SEQ_END;
    return FALSE;
}

static BOOL SpinTradeSeq_End(SpinTradeCallWork *wk) {
    SpinTradeProcWork_Free(wk->g2p);
    return TRUE;
}
