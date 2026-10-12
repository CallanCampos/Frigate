#include "types.h"
#include "app/guru2.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "system/game_data.h"

// guru2.c, which the ROM names: the proc of the Union Room's Spin Trade. It runs the receipt screen, where the group
// gathers, the party list on which each member picks an Egg, and the main screen with the disc. Names are ours

typedef struct Guru2CallWork Guru2CallWork;

struct Guru2CallWork {
    int seq;
    int partyIndex;
    Guru2Param *param;
    Guru2ProcWork *g2p;
    GameData *gameData;
    u32 unk14;
    PokeListParam *pokeList;
    PStatusParam *pokeStatus;
    GameProcManager *procManager;
};

enum {
    GURU2_SEQ_START_RECEIPT,
    GURU2_SEQ_WAIT_RECEIPT,
    GURU2_SEQ_WAIT_POKE_LIST,
    GURU2_SEQ_WAIT_POKE_STATUS,
    GURU2_SEQ_WAIT_MAIN,
    GURU2_SEQ_END,
};

// The party list's mode for picking an Egg
#define GURU2_POKELIST_MODE 0x15

static BOOL Guru2Proc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Guru2Proc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Guru2Proc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static Guru2ProcWork *Guru2ProcWork_Create(Guru2Param *param, u32 heapId);
static void Guru2ProcWork_Free(Guru2ProcWork *g2p);
static BOOL Guru2ProcWork_IsReceiptDone(Guru2ProcWork *g2p);
static BOOL Guru2Seq_StartReceipt(Guru2CallWork *wk);
static PokeListParam *Guru2_CreatePokeListParam(Guru2CallWork *wk, u32 mode, u32 index);
static BOOL Guru2Seq_WaitReceipt(Guru2CallWork *wk);
static BOOL Guru2Seq_WaitPokeList(Guru2CallWork *wk);
static BOOL Guru2Seq_WaitPokeStatus(Guru2CallWork *wk);
static BOOL Guru2Seq_WaitMain(Guru2CallWork *wk);
static BOOL Guru2Seq_End(Guru2CallWork *wk);

static const GameProcFunctions sMainProcFunctions = {
    Guru2Main_Init,
    Guru2Main_Main,
    Guru2Main_Exit,
};

static const GameProcFunctions sReceiptProcFunctions = {
    Guru2Receipt_Init,
    Guru2Receipt_Main,
    Guru2Receipt_Exit,
};

static BOOL (*const sGuru2SeqFuncs[])(Guru2CallWork *wk) = {
    Guru2Seq_StartReceipt,
    Guru2Seq_WaitReceipt,
    Guru2Seq_WaitPokeList,
    Guru2Seq_WaitPokeStatus,
    Guru2Seq_WaitMain,
    Guru2Seq_End,
};

const GameProcFunctions GURU2_PROC_FUNCTIONS = {
    Guru2Proc_Init,
    Guru2Proc_Main,
    Guru2Proc_Exit,
};

static BOOL Guru2Proc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2Param *g2param = param;
    Guru2CallWork *wk;

    wk = GFL_ProcInitSubsystem(proc, sizeof(Guru2CallWork), HEAPID_GAMEEVENT);
    sys_memset(wk, 0, sizeof(Guru2CallWork));
    wk->gameData = g2param->gameData;
    wk->param = g2param;
    wk->procManager = CreateGameProcManager(HEAPID_USER);
    return TRUE;
}

static BOOL Guru2Proc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2CallWork *wk = work;

    switch (GFL_ProcMgrUpdate(wk->procManager)) {
    case 0:
        if (sGuru2SeqFuncs[wk->seq](wk) == TRUE) {
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

static BOOL Guru2Proc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2CallWork *wk = work;

    FreeGameProcManager(wk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    return TRUE;
}

static Guru2ProcWork *Guru2ProcWork_Create(Guru2Param *param, u32 heapId) {
    Guru2ProcWork *g2p;

    g2p = GFL_HeapAllocate(heapId, sizeof(Guru2ProcWork), FALSE, "guru2.c", 0xab);
    sys_memset(g2p, 0, sizeof(Guru2ProcWork));
    g2p->param = param;
    g2p->g2c = Guru2Comm_Create(g2p, heapId);
    return g2p;
}

static void Guru2ProcWork_Free(Guru2ProcWork *g2p) {
    Guru2Comm_End(g2p->g2c);
    Guru2Comm_Free(g2p->g2c);
    GFL_HeapFree(g2p);
}

static BOOL Guru2ProcWork_IsReceiptDone(Guru2ProcWork *g2p) {
    return g2p->receiptDone;
}

static BOOL Guru2Seq_StartReceipt(Guru2CallWork *wk) {
    wk->g2p = Guru2ProcWork_Create(wk->param, HEAPID_GAMEEVENT);
    wk->seq = GURU2_SEQ_WAIT_RECEIPT;
    QueueGameProc(wk->procManager, OVERLAY_NONE, &sReceiptProcFunctions, wk->g2p);
    return FALSE;
}

static PokeListParam *Guru2_CreatePokeListParam(Guru2CallWork *wk, u32 mode, u32 index) {
    PokeListParam *pokeList;
    int i;

    pokeList = PokeListParam_Create(wk->gameData, mode, GameData_GetParty(wk->gameData), HEAPID_GAMEEVENT);
    pokeList->index = index;
    for (i = 0; i < PokeParty_GetPkmCount(pokeList->party); i++) {
        PokeParty_GetPkm(pokeList->party, i);
    }
    return pokeList;
}

static BOOL Guru2Seq_WaitReceipt(Guru2CallWork *wk) {
    if (!Guru2ProcWork_IsReceiptDone(wk->g2p)) {
        wk->seq = GURU2_SEQ_END;
    } else {
        func_02042e94(TRUE);
        wk->pokeList = Guru2_CreatePokeListParam(wk, GURU2_POKELIST_MODE, 0);
        QueueGameProc(wk->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->pokeList);
        wk->g2p->state = GURU2_PROC_STATE_POKE_LIST;
        wk->seq = GURU2_SEQ_WAIT_POKE_LIST;
    }
    return FALSE;
}

static BOOL Guru2Seq_WaitPokeList(Guru2CallWork *wk) {
    u32 result = wk->pokeList->result;
    u32 index = wk->pokeList->index;

    GFL_HeapFree(wk->pokeList);
    if (result == 1) {
        PokeDexSave *pokedex;
        PokeParty *party;

        GetGameDataPlayerInfo(wk->gameData);
        pokedex = GameData_GetPokedex(wk->gameData);
        wk->pokeStatus = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), FALSE, "guru2.c", 0x147);
        sys_memset(wk->pokeStatus, 0, sizeof(PStatusParam));
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
        wk->seq = GURU2_SEQ_WAIT_POKE_STATUS;
    } else {
        wk->g2p->partyIndex = index;
        QueueGameProc(wk->procManager, OVERLAY_NONE, &sMainProcFunctions, wk->g2p);
        wk->g2p->state = GURU2_PROC_STATE_MAIN;
        wk->seq = GURU2_SEQ_WAIT_MAIN;
    }
    return FALSE;
}

static BOOL Guru2Seq_WaitPokeStatus(Guru2CallWork *wk) {
    u8 index = wk->pokeStatus->partyIndex;

    GFL_HeapFree(wk->pokeStatus);
    wk->pokeList = Guru2_CreatePokeListParam(wk, GURU2_POKELIST_MODE, index);
    QueueGameProc(wk->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, wk->pokeList);
    wk->seq = GURU2_SEQ_WAIT_POKE_LIST;
    return FALSE;
}

static BOOL Guru2Seq_WaitMain(Guru2CallWork *wk) {
    wk->seq = GURU2_SEQ_END;
    return FALSE;
}

static BOOL Guru2Seq_End(Guru2CallWork *wk) {
    Guru2ProcWork_Free(wk->g2p);
    return TRUE;
}
