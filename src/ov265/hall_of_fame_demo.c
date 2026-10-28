#include "types.h"
#include "demo/hall_of_fame_demo.h"
#include "demo/hall_of_fame_demo_main.h"
#include "demo/hall_of_fame_demo_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/join_avenue.h"

// The Hall of Fame entry demo's process (overlay 265). The file's name is a guess, after hall_of_fame_pc.c

static BOOL HallOfFameDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL HallOfFameDemo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL HallOfFameDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions HALL_OF_FAME_DEMO_PROC_FUNCTIONS = { HallOfFameDemo_Init, HallOfFameDemo_Main,
                                                             HallOfFameDemo_Exit };

static BOOL HallOfFameDemo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    HallOfFameDemoWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_HALL_OF_FAME_DEMO, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(HallOfFameDemoWork), HEAPID_HALL_OF_FAME_DEMO);
    MI_CpuFill8(wk, 0, sizeof(HallOfFameDemoWork));
    wk->param = param;
    JoinAvenue_RecordActivity(1);
    return TRUE;
}

static BOOL HallOfFameDemo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (HallOfFameDemoSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL HallOfFameDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_HALL_OF_FAME_DEMO);
    return TRUE;
}
