// The Entree Forest capture (overlay 172): the process that overlay 12's scrcmd_entree_forest.c queues, which shows
// the Pokémon of the dream appearing and lets the player throw a Dream Ball at it. The name is the ROM's string, from
// GFL_HeapAllocate's assert. pdc_main.c has the scene and pdc_msg.c the messages

#include "types.h"
#include "app/pdc.h"
#include "app/pdc_main.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"

struct PdcWork {
    GameData *gameData;
    PartyPkm *pkm;
    BtlFieldStatus *status;
    PlayerInfo *playerInfo;
    BagSave *bag;
    TrainerDataSave *trainerData;
    PokeDexSave *pokedex;
    BOOL caught;
};

// The process's work
typedef struct {
    PdcMain *main;
    PdcWork *pdc;
    HeapID heapId;
} PdcProcWork;

static BOOL Pdc_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Pdc_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Pdc_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

PdcWork *PdcWork_Create(GameData *gameData, PartyPkm *pkm, BtlFieldStatus *status, PlayerInfo *playerInfo,
                             BagSave *bag, TrainerDataSave *trainerData, PokeDexSave *pokedex, HeapID heapId) {
    PdcWork *work = GFL_HeapAllocate(heapId, sizeof(PdcWork), FALSE, "pdc.c", 80);

    work->gameData = gameData;
    work->pkm = pkm;
    work->status = status;
    work->playerInfo = playerInfo;
    work->bag = bag;
    work->trainerData = trainerData;
    work->pokedex = pokedex;
    work->caught = FALSE;
    return work;
}

GameData *PdcWork_GetGameData(const PdcWork *work) {
    return work->gameData;
}

PartyPkm *PdcWork_GetPkm(const PdcWork *work) {
    return work->pkm;
}

BtlFieldStatus *PdcWork_GetFieldStatus(const PdcWork *work) {
    return work->status;
}

PlayerInfo *PdcWork_GetPlayerInfo(const PdcWork *work) {
    return work->playerInfo;
}

BagSave *PdcWork_GetBag(const PdcWork *work) {
    return work->bag;
}

TrainerDataSave *PdcWork_GetTrainerData(const PdcWork *work) {
    return work->trainerData;
}

PokeDexSave *PdcWork_GetPokedex(const PdcWork *work) {
    return work->pokedex;
}

BOOL PdcWork_IsCaught(const PdcWork *work) {
    return work->caught;
}

const GameProcFunctions PDC_PROC_FUNCTIONS = {
    Pdc_ProcInit,
    Pdc_ProcMain,
    Pdc_ProcExit,
};

static BOOL Pdc_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PdcWork *pdc = param;
    PdcProcWork *wk;

    GFL_OvlLoad(OVERLAY_ID(168));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PDC, 0xc0000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PdcProcWork), HEAPID_PDC);
    sys_memset32_fast(0, wk, sizeof(PdcProcWork));
    wk->heapId = HEAPID_PDC;
    wk->main = PdcMain_Create(pdc, wk->heapId);
    wk->pdc = pdc;
    return TRUE;
}

static BOOL Pdc_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    PdcProcWork *wk = work;

    wk->pdc->caught = PdcMain_Update(wk->main);
    if (wk->pdc->caught) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Pdc_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    PdcProcWork *wk = work;

    PdcMain_Free(wk->main);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_PDC);
    GFL_OvlUnload(OVERLAY_ID(168));
    return TRUE;
}
