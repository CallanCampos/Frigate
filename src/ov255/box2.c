#include "types.h"
#include "app/box2.h"
#include "app/box2_main.h"
#include "app/box2_seq.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "save/box.h"

// The PC box's proc. The ROM doesn't name this file; it is named after the proc's overlay, box2, since its other files
// are box2_main.c, box2_seq.c and box2_ui.c

static BOOL Box2Proc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Box2Proc_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL Box2Proc_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions BOX2_PROC_FUNCTIONS = { Box2Proc_Init, Box2Proc_Main, Box2Proc_Exit };

static BOOL Box2Proc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    Box2SysWork *syswk;
    u32 count;
    u16 tray;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BOX2, 0x10000);
    syswk = GFL_ProcInitSubsystem(proc, sizeof(Box2SysWork), HEAPID_BOX2);
    MI_CpuFill8(syswk, 0, sizeof(Box2SysWork));
    syswk->param = param;
    syswk->tray = BoxSaveAccessor_GetLastOpenedBox(((Box2Param *)param)->boxes);
    syswk->trayMax = BoxSaveAccessor_GetAvailableBoxCount(syswk->param->boxes);
    syswk->getTray = BOX2_GET_NONE;
    syswk->nextSeq = BOX2SEQ_START;

    // Once every box that is open holds a Pokémon, more boxes open
    if (syswk->trayMax != BOX2_TRAY_MAX) {
        if (syswk->trayMax == 8) {
            count = 8;
        } else if (syswk->trayMax == 16) {
            count = 16;
        }
        for (tray = 0; tray < count; tray++) {
            if (howManyPokesInGeneralAreInBox(syswk->param->boxes, tray) == 0) {
                break;
            }
        }
        if (tray == count) {
            syswk->trayMax = BoxSaveAccessor_UnlockMoreBoxes(syswk->param->boxes);
        }
    }

    Box2Main_InitSettings(syswk);
    syswk->procManager = CreateGameProcManager(HEAPID_BOX2);
    return TRUE;
}

static BOOL Box2Proc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    Box2SysWork *syswk = work;

    syswk->procMgrResult = GFL_ProcMgrUpdate(syswk->procManager);
    if (Box2Seq_Main(syswk, state) == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Box2Proc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    Box2SysWork *syswk = work;

    saveLastOpenedBoxIdx(syswk->param->boxes, syswk->tray);
    FreeGameProcManager(syswk->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BOX2);
    return TRUE;
}
