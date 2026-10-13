// The Battle Recorder's screens: a stack of procs, each started with the parameter that its before function fills
// from the screen it follows. The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"

// The steps of BrProcSys_Main
enum {
    BR_PROC_SYS_SEQ_CHANGE,
    BR_PROC_SYS_SEQ_START,
    BR_PROC_SYS_SEQ_MAIN,
    BR_PROC_SYS_SEQ_END,
    BR_PROC_SYS_SEQ_EXIT,
};

// A screen of the stack
typedef struct {
    BOOL is_use;
    void *param;
    const BrProcData *cp_data;
    u32 procID;
} BrProcStack;

struct BrProcSys {
    GameProcManager *procMgr;
    const BrProcData *tbl;
    u16 tbl_max;
    void *work;
    HeapID heapId;
    BOOL isEnd;
    u32 seq;
    BrProcStack prev;
    BrProcStack now;
    BrProcStack next;
    u32 stack[BR_PROC_SYS_STACK_MAX];
    u32 stack_num;
    BrProcSysRecovery *recovery;
    // Set until the first screen of a restored stack starts
    BOOL isRecovery;
};

static void BrProcSys_SetStack(BrProcStack *p_stack, u32 procID, const BrProcData *cp_data);
static void BrProcSys_ClearStack(BrProcStack *p_stack);

BrProcSys *BrProcSys_Init(u16 procID, const BrProcData *tbl, u16 tbl_max, void *work, BrProcSysRecovery *recovery,
                          HeapID heapId) {
    BrProcSys *p_wk;
    u32 i = 0;

    p_wk = GFL_HeapAllocate(heapId, sizeof(BrProcSys), FALSE, "br_proc_sys.c", 119);
    sys_memset(p_wk, 0, sizeof(BrProcSys));
    p_wk->heapId = heapId;
    p_wk->tbl_max = tbl_max;
    p_wk->work = work;
    p_wk->tbl = tbl;
    p_wk->recovery = recovery;
    p_wk->procMgr = CreateGameProcManager(heapId);
    BrProcSys_ClearStack(&p_wk->now);
    BrProcSys_ClearStack(&p_wk->next);
    BrProcSys_ClearStack(&p_wk->prev);

    if (p_wk->recovery->stack_num != 0) {
        for (i = 0; i < p_wk->recovery->stack_num; i++) {
            BrProcSys_Push(p_wk, p_wk->recovery->stack[i]);
        }
        sys_memset(p_wk->recovery, 0, sizeof(BrProcSysRecovery));
        p_wk->isRecovery = TRUE;
    } else {
        BrProcSys_Push(p_wk, procID);
    }
    return p_wk;
}

void BrProcSys_Exit(BrProcSys *p_wk) {
    FreeGameProcManager(p_wk->procMgr);
    GFL_HeapFree(p_wk);
}

void BrProcSys_Main(BrProcSys *p_wk) {
    BrProcStack *p_now = &p_wk->now;
    BrProcStack *p_prev = &p_wk->prev;
    BrProcStack *p_next = &p_wk->next;

    switch (p_wk->seq) {
    case BR_PROC_SYS_SEQ_CHANGE:
        *p_prev = *p_now;
        *p_now = *p_next;
        p_wk->seq = BR_PROC_SYS_SEQ_START;
        break;
    case BR_PROC_SYS_SEQ_START: {
        u32 preID;

        GFL_ASSERT(p_now->is_use);
        GFL_ASSERT(p_now->cp_data->before_func);
        p_now->param = GFL_HeapAllocate(p_wk->heapId, p_now->cp_data->paramSize, FALSE, "br_proc_sys.c", 210);
        sys_memset(p_now->param, 0, p_now->cp_data->paramSize);
        preID = BR_PROCID_RECOVERY;
        if (!p_wk->isRecovery) {
            preID = p_prev->procID;
        }
        p_now->cp_data->before_func(p_now->param, p_wk->work, p_prev->param, preID);
        p_wk->isRecovery = FALSE;
        if (p_prev->param != NULL) {
            GFL_HeapFree(p_prev->param);
        }
        QueueGameProc(p_wk->procMgr, p_now->cp_data->overlayId, p_now->cp_data->procFuncs, p_now->param);
        p_wk->seq = BR_PROC_SYS_SEQ_MAIN;
        break;
    }
    case BR_PROC_SYS_SEQ_MAIN:
        if (!GFL_ProcMgrUpdate(p_wk->procMgr)) {
            p_wk->seq = BR_PROC_SYS_SEQ_END;
        }
        break;
    case BR_PROC_SYS_SEQ_END:
        GFL_ASSERT(p_now->is_use);
        if (p_now->cp_data->after_func != NULL) {
            p_now->cp_data->after_func(p_now->param, p_wk->work);
        }
        if (p_wk->stack_num != 0) {
            p_wk->seq = BR_PROC_SYS_SEQ_CHANGE;
        } else {
            if (p_now->param != NULL) {
                GFL_HeapFree(p_now->param);
            }
            p_wk->seq = BR_PROC_SYS_SEQ_EXIT;
        }
        break;
    case BR_PROC_SYS_SEQ_EXIT:
        p_wk->isEnd = TRUE;
        break;
    }
}

BOOL BrProcSys_IsEnd(BrProcSys *p_wk) {
    return p_wk->isEnd;
}

HeapID BrProcSys_GetHeapID(BrProcSys *p_wk) {
    return p_wk->heapId;
}

void BrProcSys_Pop(BrProcSys *p_wk) {
    BrProcStack stack;

    if (p_wk->stack_num != 0) {
        p_wk->stack_num--;
        if (p_wk->stack_num != 0) {
            u16 procID = *(p_wk->stack + p_wk->stack_num - 1);

            BrProcSys_SetStack(&stack, procID, &p_wk->tbl[procID]);
            p_wk->next = stack;
        } else {
            BrProcSys_ClearStack(&p_wk->next);
        }
    } else {
        BrProcSys_ClearStack(&p_wk->next);
    }
}

void BrProcSys_Push(BrProcSys *p_wk, u16 procID) {
    BrProcStack stack;
    const BrProcData *cp_data = &p_wk->tbl[procID];

    GFL_ASSERT(procID < p_wk->tbl_max);
    BrProcSys_SetStack(&stack, procID, cp_data);
    GFL_ASSERT(p_wk->stack_num < BR_PROC_SYS_STACK_MAX);
    p_wk->stack[p_wk->stack_num] = procID;
    p_wk->stack_num++;
    p_wk->next = stack;
}

void BrProcSys_Interrupt(BrProcSys *p_wk) {
    sys_memcpy(p_wk->stack, p_wk->recovery->stack, sizeof(p_wk->stack));
    p_wk->recovery->stack_num = p_wk->stack_num;
    BrProcSys_Abort(p_wk);
}

void BrProcSys_Abort(BrProcSys *p_wk) {
    while (p_wk->stack_num != 0) {
        BrProcSys_Pop(p_wk);
    }
}

static void BrProcSys_SetStack(BrProcStack *p_stack, u32 procID, const BrProcData *cp_data) {
    sys_memset(p_stack, 0, sizeof(BrProcStack));
    p_stack->is_use = TRUE;
    p_stack->param = NULL;
    p_stack->procID = procID;
    p_stack->cp_data = cp_data;
}

static void BrProcSys_ClearStack(BrProcStack *p_stack) {
    sys_memset(p_stack, 0, sizeof(BrProcStack));
    p_stack->procID = BR_PROCID_NONE;
}
