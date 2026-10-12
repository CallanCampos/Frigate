#include "types.h"
#include "app/dendou_pc.h"
#include "app/dpc_main.h"
#include "app/dpc_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The Hall of Fame PC's proc. The ROM doesn't name this file; the name is a guess, after the screen's subject

static BOOL DendouPCProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DendouPCProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DendouPCProc_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions DENDOU_PC_PROC_FUNCTIONS = { DendouPCProc_Init, DendouPCProc_Main, DendouPCProc_Exit };

static BOOL DendouPCProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    DendouPCWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_DENDOU_PC, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(DendouPCWork), HEAPID_DENDOU_PC);
    sys_memset(wk, 0, sizeof(DendouPCWork));
    wk->param = param;
    return TRUE;
}

static BOOL DendouPCProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (DpcSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DendouPCProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_DENDOU_PC);
    return TRUE;
}
