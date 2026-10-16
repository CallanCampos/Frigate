#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "nitro/os.h"

// A process loads its overlay, then runs its init, main and exit functions in turn, each until it returns TRUE
enum {
    PROC_STATE_LOAD,
    PROC_STATE_INIT,
    PROC_STATE_MAIN,
    PROC_STATE_EXIT,
};

// What GFL_ProcMgrUpdateCore returns
enum {
    PROCMGR_DONE,
    PROCMGR_RUNNING,
    PROCMGR_PROC_ENDED,
};

struct GameProc {
    const GameProcFunctions *functions;
    s32 overlayId;
    u32 state;
    u32 subState;
    void *param;
    void *work;
    // The process that queued this one, which runs again when it ends
    GameProc *parent;
    GameProc *child;
};

struct GameProcManager {
    HeapID heapId;
    s32 unk4;
    GameProc *proc;
    // A process to run next, after the current one or in place of it
    BOOL queued;
    BOOL replace;
    s32 nextOverlayId;
    const GameProcFunctions *nextFunctions;
    void *nextParam;
};

static GameProcManager *GFL_ProcMgrCreate(HeapID heapId);
static void GFL_ProcMgrSwitchQueued(GameProcManager *manager);
static u32 GFL_ProcMgrUpdateCore(GameProcManager *manager);
static void GFL_ProcMgrFree(GameProcManager *manager);
static void GFL_ProcMgrReplaceProc(GameProcManager *manager, s32 overlayId, const GameProcFunctions *functions,
                                   void *param);
static void GFL_ProcMgrQueueProc(GameProcManager *manager, s32 overlayId, const GameProcFunctions *functions,
                                 void *param);
static GameProc *GFL_ProcCreate(s32 overlayId, const GameProcFunctions *functions, void *param, HeapID heapId);
static void GFL_ProcFree(GameProc *proc);
static BOOL GFL_ProcUpdate(GameProc *proc);

static GameProcManager *sGameProcManager;

void GCTX_ProcMgrInit(HeapID heapId) {
    sGameProcManager = GFL_ProcMgrCreate(heapId);
}

u32 GCTX_ProcMgrUpdate(void) {
    return GFL_ProcMgrUpdateCore(sGameProcManager);
}

void GCTX_ProcMgrQueueProc(s32 overlayId, const GameProcFunctions *functions, void *param) {
    GFL_ProcMgrQueueProc(sGameProcManager, overlayId, functions, param);
}

void GCTX_ProcMgrReplaceProc(s32 overlayId, const GameProcFunctions *functions, void *param) {
    GFL_ProcMgrReplaceProc(sGameProcManager, overlayId, functions, param);
}

GameProcManager *CreateGameProcManager(HeapID heapId) {
    return GFL_ProcMgrCreate(heapId);
}

BOOL GFL_ProcMgrUpdate(GameProcManager *manager) {
    return GFL_ProcMgrUpdateCore(manager);
}

void FreeGameProcManager(GameProcManager *manager) {
    GFL_ProcMgrFree(manager);
}

void QueueGameProc(GameProcManager *manager, s32 overlayId, const GameProcFunctions *functions, void *param) {
    GFL_ProcMgrQueueProc(manager, overlayId, functions, param);
}

static GameProcManager *GFL_ProcMgrCreate(HeapID heapId) {
    GameProcManager *manager = GFL_HeapAllocate(heapId, sizeof(GameProcManager), FALSE, "procsys.c", 197);

    manager->heapId = heapId;
    manager->unk4 = OVERLAY_NONE;
    manager->proc = NULL;
    manager->queued = FALSE;
    manager->replace = FALSE;
    manager->nextOverlayId = OVERLAY_NONE;
    manager->nextFunctions = NULL;
    manager->nextParam = NULL;
    return manager;
}

static void GFL_ProcMgrSwitchQueued(GameProcManager *manager) {
    GameProc *proc =
        GFL_ProcCreate(manager->nextOverlayId, manager->nextFunctions, manager->nextParam, manager->heapId);

    proc->parent = manager->proc;
    manager->proc->child = proc;
    manager->proc = proc;
    manager->queued = FALSE;
    manager->nextOverlayId = OVERLAY_NONE;
    manager->nextFunctions = NULL;
    manager->nextParam = NULL;
}

static u32 GFL_ProcMgrUpdateCore(GameProcManager *manager) {
    BOOL ended;

    if (manager->proc == NULL) {
        return PROCMGR_DONE;
    }
    if (manager->queued) {
        GFL_ProcMgrSwitchQueued(manager);
    }
    ended = GFL_ProcUpdate(manager->proc);
    if (ended == TRUE) {
        if (manager->replace) {
            GameProc *parent = manager->proc->parent;

            GFL_ProcFree(manager->proc);
            manager->proc =
                GFL_ProcCreate(manager->nextOverlayId, manager->nextFunctions, manager->nextParam, manager->heapId);
            if (parent != NULL) {
                parent->child = manager->proc;
            }
            manager->proc->parent = parent;
            manager->replace = FALSE;
            manager->nextOverlayId = OVERLAY_NONE;
            manager->nextFunctions = NULL;
            manager->nextParam = NULL;
        } else {
            GameProc *parent = manager->proc->parent;

            GFL_ProcFree(manager->proc);
            manager->proc = parent;
            if (parent != NULL) {
                parent->child = NULL;
            }
        }
    }
    if (manager->proc == NULL) {
        return PROCMGR_DONE;
    }
    return ended == TRUE ? PROCMGR_PROC_ENDED : PROCMGR_RUNNING;
}

// Freeing the manager while a process runs halts the system
static void GFL_ProcMgrFree(GameProcManager *manager) {
    if (manager->proc != NULL) {
        OS_Halt();
    }
    GFL_HeapFree(manager);
}

static void GFL_ProcMgrReplaceProc(GameProcManager *manager, s32 overlayId, const GameProcFunctions *functions,
                                   void *param) {
    manager->replace = TRUE;
    manager->nextOverlayId = overlayId;
    manager->nextFunctions = functions;
    manager->nextParam = param;
}

static void GFL_ProcMgrQueueProc(GameProcManager *manager, s32 overlayId, const GameProcFunctions *functions,
                                 void *param) {
    if (manager->proc == NULL) {
        manager->proc = GFL_ProcCreate(overlayId, functions, param, manager->heapId);
    } else {
        manager->queued = TRUE;
        manager->nextOverlayId = overlayId;
        manager->nextFunctions = functions;
        manager->nextParam = param;
    }
}

static GameProc *GFL_ProcCreate(s32 overlayId, const GameProcFunctions *functions, void *param, HeapID heapId) {
    GameProc *proc = GFL_HeapAllocate(heapId, sizeof(GameProc), FALSE, "procsys.c", 354);

    proc->functions = functions;
    proc->overlayId = overlayId;
    proc->state = PROC_STATE_LOAD;
    proc->subState = 0;
    proc->param = param;
    proc->work = NULL;
    proc->parent = NULL;
    proc->child = NULL;
    return proc;
}

static void GFL_ProcFree(GameProc *proc) {
    GFL_HeapFree(proc);
}

void *GFL_ProcInitSubsystem(GameProc *proc, u32 size, HeapID heapId) {
    void *work = GFL_HeapAllocate(heapId, size, FALSE, "procsys.c", 390);

    proc->work = work;
    return work;
}

void GFL_ProcReleaseSubsystem(GameProc *proc) {
    GFL_HeapFree(proc->work);
    proc->work = NULL;
}

// Returns TRUE once the process has ended
static BOOL GFL_ProcUpdate(GameProc *proc) {
    switch (proc->state) {
    case PROC_STATE_LOAD:
        if (proc->overlayId != OVERLAY_NONE) {
            GFL_OvlLoad(proc->overlayId);
        }
        proc->state = PROC_STATE_INIT;
        // fallthrough
    case PROC_STATE_INIT:
        if (proc->functions->init(proc, &proc->subState, proc->param, proc->work) == TRUE) {
            proc->state = PROC_STATE_MAIN;
            proc->subState = 0;
        }
        break;
    case PROC_STATE_MAIN:
        if (proc->functions->main(proc, &proc->subState, proc->param, proc->work) == TRUE) {
            proc->state = PROC_STATE_EXIT;
            proc->subState = 0;
        }
        break;
    case PROC_STATE_EXIT:
        if (proc->functions->exit(proc, &proc->subState, proc->param, proc->work) == TRUE) {
            if (proc->overlayId != OVERLAY_NONE) {
                GFL_OvlUnload(proc->overlayId);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}
