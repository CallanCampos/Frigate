#include "types.h"
#include "app/hall_of_fame_pc.h"
#include "app/hall_of_fame_pc_main.h"
#include "app/hall_of_fame_pc_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The Hall of Fame PC's proc. The ROM doesn't name this file; the name is a guess, after the screen's subject

static BOOL HallOfFamePCProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL HallOfFamePCProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL HallOfFamePCProc_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions HALL_OF_FAME_PC_PROC_FUNCTIONS = { HallOfFamePCProc_Init, HallOfFamePCProc_Main,
                                                           HallOfFamePCProc_Exit };

static BOOL HallOfFamePCProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    HallOfFamePCWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_HALL_OF_FAME_PC, 0x60000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(HallOfFamePCWork), HEAPID_HALL_OF_FAME_PC);
    MI_CpuFill8(wk, 0, sizeof(HallOfFamePCWork));
    wk->param = param;
    return TRUE;
}

static BOOL HallOfFamePCProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (HallOfFamePCSeq_Main(work) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL HallOfFamePCProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_HALL_OF_FAME_PC);
    return TRUE;
}
