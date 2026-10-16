#include "types.h"
#include "demo/ddemo_main.h"
#include "demo/ddemo_seq.h"
#include "demo/dendou_demo.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/join_avenue.h"

// The Hall of Fame entry demo's process (overlay 265). The file's name is a guess, after dendou_pc.c

static BOOL DendouDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DendouDemo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DendouDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions DENDOU_DEMO_PROC_FUNCTIONS = { DendouDemo_Init, DendouDemo_Main, DendouDemo_Exit };

static BOOL DendouDemo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    DDemoWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_DENDOU_DEMO, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(DDemoWork), HEAPID_DENDOU_DEMO);
    MI_CpuFill8(wk, 0, sizeof(DDemoWork));
    wk->param = param;
    func_02038bc8(1);
    return TRUE;
}

static BOOL DendouDemo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (DDemoSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DendouDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_DENDOU_DEMO);
    return TRUE;
}
