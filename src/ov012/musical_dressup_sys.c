#include "types.h"
#include "field/dup_fitting.h"
#include "field/musical.h"
#include "field/musical_dressup_sys.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"

typedef struct {
    // The copy of the parameter that overlay 208 gets
    MusicalDressUpParam *dressUp;
    // Overlay 208's work
    DupFittingWork *work;
    u32 result;
    MusicalDressUpParam *param;
} MusicalDressUpWork;

// The proc's steps: overlay 208's dressing room is made, run until it gives a result, and freed
enum {
    MUSICAL_DRESSUP_STEP_CREATE,
    MUSICAL_DRESSUP_STEP_MAIN,
    MUSICAL_DRESSUP_STEP_FREE,
};

static BOOL MusicalDressUpProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalDressUpProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalDressUpProc_Main(GameProc *proc, u32 *state, void *param, void *work);

GameProcFunctions MUSICAL_DRESSUP_PROC_FUNCTIONS = {
    MusicalDressUpProc_Init,
    MusicalDressUpProc_Main,
    MusicalDressUpProc_Exit,
};

static BOOL MusicalDressUpProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalDressUpParam *dressUp = param;
    MusicalDressUpWork *wk;

    *state = MUSICAL_DRESSUP_STEP_CREATE;
    GFL_OvlLoad(OVERLAY_ID(208));
    GFL_OvlLoad(OVERLAY_ID(209));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MUSICAL_DRESSUP, 0x40000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MusicalDressUpWork), HEAPID_MUSICAL_DRESSUP);
    wk->dressUp =
        GFL_HeapAllocate(HEAPID_MUSICAL_DRESSUP, sizeof(MusicalDressUpParam), FALSE, "musical_dressup_sys.c", 80);
    wk->param = dressUp;
    wk->dressUp->comm = dressUp->comm;
    wk->dressUp->poke = dressUp->poke;
    wk->dressUp->save = dressUp->save;
    return TRUE;
}

static BOOL MusicalDressUpProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalDressUpWork *wk = work;

    GFL_HeapFree(wk->dressUp);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MUSICAL_DRESSUP);
    GFL_OvlUnload(OVERLAY_ID(209));
    GFL_OvlUnload(OVERLAY_ID(208));
    return TRUE;
}

static BOOL MusicalDressUpProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalDressUpWork *wk = work;
    u32 result;

    switch (*state) {
    case MUSICAL_DRESSUP_STEP_CREATE:
        wk->work = DupFitting_Create(wk->dressUp, HEAPID_MUSICAL_DRESSUP);
        *state = MUSICAL_DRESSUP_STEP_MAIN;
        if (wk->param->comm != NULL && MusComm_IsParent(wk->param->comm) == TRUE) {
            MusComm_StartSendingSound(wk->param->comm);
        }
        break;
    case MUSICAL_DRESSUP_STEP_MAIN:
        result = DupFitting_Main(wk->work);
        if (result != 0) {
            wk->result = result;
            *state = MUSICAL_DRESSUP_STEP_FREE;
        }
        break;
    case MUSICAL_DRESSUP_STEP_FREE:
        DupFitting_Free(wk->work);
        return TRUE;
    }
    return FALSE;
}

MusicalDressUpParam *MusicalDressUpParam_Create(HeapID heapId, MusicalPoke *poke, SaveControl *save) {
    MusicalDressUpParam *param =
        GFL_HeapAllocate(heapId, sizeof(MusicalDressUpParam), FALSE, "musical_dressup_sys.c", 168);

    param->comm = NULL;
    param->save = getAddressOfMusicalDataInfo(save);
    param->poke = poke;
    return param;
}

void MusicalDressUpParam_Free(MusicalDressUpParam *param) {
    GFL_HeapFree(param);
}
