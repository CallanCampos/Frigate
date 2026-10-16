#include "types.h"
#include "app/zknlist_main.h"
#include "app/zknlist_seq.h"
#include "app/zukanlist.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The process of the Pokédex's list. The ROM doesn't name this file; zukanlist.c is a guess after the search screen's
// zukansearch.c

static BOOL ZukanList_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanList_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions ZukanList_ProcFunctions = {
    ZukanList_ProcInit,
    ZukanList_ProcMain,
    ZukanList_ProcEnd,
};

static BOOL ZukanList_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    ZknListWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_LIST, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZknListWork), HEAPID_ZUKAN_LIST);
    MI_CpuFill8(wk, 0, sizeof(ZknListWork));
    wk->param = param;
    return TRUE;
}

static BOOL ZukanList_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    if (ZknListSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ZukanList_ProcEnd(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN_LIST);
    return TRUE;
}
