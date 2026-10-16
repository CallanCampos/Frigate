#include "types.h"
#include "app/zknsearch_main.h"
#include "app/zknsearch_seq.h"
#include "app/zukansearch.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The process of the Pokédex's search screen. The ROM doesn't name this file; zukansearch.c is a guess after
// zknsearch_main.c, the only name the overlay has

static BOOL ZukanSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanSearch_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions ZukanSearch_ProcFunctions = {
    ZukanSearch_ProcInit,
    ZukanSearch_ProcMain,
    ZukanSearch_ProcEnd,
};

static BOOL ZukanSearch_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    ZknSearchWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_SEARCH, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZknSearchWork), HEAPID_ZUKAN_SEARCH);
    MI_CpuFill8(wk, 0, sizeof(ZknSearchWork));
    wk->param = param;
    return TRUE;
}

static BOOL ZukanSearch_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (ZknSearchSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ZukanSearch_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN_SEARCH);
    return TRUE;
}
