#include "types.h"
#include "app/pokedex_list.h"
#include "app/pokedex_list_main.h"
#include "app/pokedex_list_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The process of the Pokédex's list. The ROM doesn't name this file; pokedex_list.c is a guess after the search
// screen's pokedex_search.c

static BOOL PokedexList_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexList_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions PokedexList_ProcFunctions = {
    PokedexList_ProcInit,
    PokedexList_ProcMain,
    PokedexList_ProcEnd,
};

static BOOL PokedexList_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexListWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_LIST, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexListWork), HEAPID_POKEDEX_LIST);
    MI_CpuFill8(wk, 0, sizeof(PokedexListWork));
    wk->param = param;
    return TRUE;
}

static BOOL PokedexList_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (PokedexListSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PokedexList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_LIST);
    return TRUE;
}
