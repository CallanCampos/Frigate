#include "types.h"
#include "app/pokedex_search.h"
#include "app/pokedex_search_main.h"
#include "app/pokedex_search_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The process of the Pokédex's search screen. The ROM doesn't name this file; pokedex_search.c is a guess after
// pokedex_search_main.c, the only name the overlay has

static BOOL PokedexSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexSearch_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions PokedexSearch_ProcFunctions = {
    PokedexSearch_ProcInit,
    PokedexSearch_ProcMain,
    PokedexSearch_ProcEnd,
};

static BOOL PokedexSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexSearchWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_SEARCH, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexSearchWork), HEAPID_POKEDEX_SEARCH);
    MI_CpuFill8(wk, 0, sizeof(PokedexSearchWork));
    wk->param = param;
    return TRUE;
}

static BOOL PokedexSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (PokedexSearchSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL PokedexSearch_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_SEARCH);
    return TRUE;
}
