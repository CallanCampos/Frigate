#include "types.h"
#include "app/musical/sta_act_script.h"
#include "app/musical/sta_acting.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nnsys/fnd.h"
#include "system/vm.h"

// Overlay 209's sta_act_script.c: the stage's scripts and the tasks of their commands

static BOOL StaActScript_RunScript(StaActScriptSys *sys, StaActScript *script);

static const VMInitParam STA_ACT_SCRIPT_VM_PARAM = {
    16, 8, STA_SCRIPT_COMMANDS, STA_SCRIPT_COMMAND_COUNT, NULL, 0, 0,
};

StaActScriptSys *StaActScript_InitSystem(HeapID heapId, StaActing *stage) {
    u8 i;
    StaActScriptSys *sys = GFL_HeapAllocate(heapId, sizeof(StaActScriptSys), FALSE, "sta_act_script.c", 63);

    sys->heapId = heapId;
    sys->stage = stage;
    sys->tcbWork = GFL_HeapAllocate(sys->heapId, GFL_TCBMgrCalcAllocSize(16), FALSE, "sta_act_script.c", 67);
    sys->tcbMgr = GFL_TCBMgrCreate(16, sys->tcbWork);
    NNS_FndInitList(&sys->tasks, 0);
    for (i = 0; i < STA_SCRIPT_MAX; i++) {
        sys->scripts[i] = NULL;
    }
    return sys;
}

void StaActScript_TermSystem(StaActScriptSys *sys) {
    u8 i;
    StaActScriptTask *task;

    for (i = 0; i < STA_SCRIPT_MAX; i++) {
        if (sys->scripts[i] != NULL) {
            VM_Halt(sys->scripts[i]->vm);
            VM_Free(sys->scripts[i]->vm);
            if (sys->scripts[i]->flags & STA_SCRIPT_FLAG_OWN_FILE) {
                GFL_HeapFree(sys->scripts[i]->file);
            }
            GFL_HeapFree(sys->scripts[i]);
            sys->scripts[i] = NULL;
        }
    }
    task = NNS_FndGetNextListObject(&sys->tasks, NULL);
    while (task != NULL) {
        StaActScriptTask *next = NNS_FndGetNextListObject(&sys->tasks, task);

        StaActScript_DelTask(sys, task);
        task = next;
    }
    GFL_TCBMgrDelete(sys->tcbMgr);
    GFL_HeapFree(sys->tcbWork);
    GFL_HeapFree(sys);
}

void StaActScript_UpdateSystem(StaActScriptSys *sys) {
    BOOL allWaiting = TRUE;
    u32 count;

    for (count = StaActing_GetUpdateCount(sys->stage); count != 0; count--) {
        u8 i;

        for (i = 0; i < STA_SCRIPT_MAX; i++) {
            if (sys->scripts[i] != NULL) {
                if (StaActScript_RunScript(sys, sys->scripts[i]) == TRUE) {
                    GFL_HeapFree(sys->scripts[i]);
                    sys->scripts[i] = NULL;
                } else if ((sys->scripts[i]->flags & STA_SCRIPT_FLAG_SYNC) &&
                           !(sys->scripts[i]->flags & STA_SCRIPT_FLAG_WAIT_SYNC)) {
                    allWaiting = FALSE;
                }
            }
        }
        // Once every synced script waits, they all go on
        if (allWaiting == TRUE) {
            for (i = 0; i < STA_SCRIPT_MAX; i++) {
                if (sys->scripts[i] != NULL && (sys->scripts[i]->flags & STA_SCRIPT_FLAG_SYNC) &&
                    (sys->scripts[i]->flags & STA_SCRIPT_FLAG_WAIT_SYNC)) {
                    sys->scripts[i]->flags ^= STA_SCRIPT_FLAG_WAIT_SYNC;
                }
            }
        }
        GFL_TCBMgrUpdate(sys->tcbMgr);
    }
}

StaActScript *StaActScript_CreateScript(StaActScriptSys *sys, const void *data, BOOL sync) {
    u8 i;

    for (i = 0; i < STA_SCRIPT_MAX; i++) {
        if (sys->scripts[i] == NULL) {
            break;
        }
    }
    sys->scripts[i] = GFL_HeapAllocate(sys->heapId, sizeof(StaActScript), FALSE, "sta_act_script.c", 196);
    sys->scripts[i]->data = data;
    sys->scripts[i]->file = NULL;
    sys->scripts[i]->wait = 0;
    sys->scripts[i]->frame = 0;
    sys->scripts[i]->flags = 0;
    sys->scripts[i]->pokeNo = 0;
    sys->scripts[i]->index = i;
    if (sync == TRUE) {
        sys->scripts[i]->flags |= STA_SCRIPT_FLAG_SYNC;
    }
    sys->scripts[i]->sys = sys;
    sys->scripts[i]->vm = VM_Create(sys->heapId, &STA_ACT_SCRIPT_VM_PARAM);
    VM_ChangeEnv(sys->scripts[i]->vm, sys->scripts[i]);
    VM_LoadScript(sys->scripts[i]->vm, sys->scripts[i]->data);
    return sys->scripts[i];
}

void StaActScript_CreateScriptFile(StaActScriptSys *sys, u32 *file, u32 index, u8 pokeNo) {
    StaActScript *script = StaActScript_CreateScript(sys, (u8 *)file + file[index], FALSE);

    script->file = file;
    script->flags |= STA_SCRIPT_FLAG_OWN_FILE;
    script->pokeNo = pokeNo;
}

static BOOL StaActScript_RunScript(StaActScriptSys *sys, StaActScript *script) {
    if (script->wait != 0) {
        script->wait--;
    }
    if (script->wait == 0 && !(script->flags & STA_SCRIPT_FLAG_WAIT_SYNC)) {
        VM_Run(script->vm);
        if (script->flags & STA_SCRIPT_FLAG_END) {
            VM_Halt(script->vm);
            VM_Free(script->vm);
            if (script->flags & STA_SCRIPT_FLAG_OWN_FILE) {
                GFL_HeapFree(script->file);
            }
            return TRUE;
        }
    }
    script->frame++;
    return FALSE;
}

u8 StaActScript_GetScriptNum(StaActScriptSys *sys) {
    u8 i;
    u8 count = 0;

    for (i = 0; i < STA_SCRIPT_MAX; i++) {
        if (sys->scripts[i] != NULL) {
            count++;
        }
    }
    return count;
}

StaActScriptTask *StaActScript_AddTask(StaActScriptSys *sys, TCBFunc func, void *work, u32 priority) {
    StaActScriptTask *task = GFL_HeapAllocate(sys->heapId, sizeof(StaActScriptTask), FALSE, "sta_act_script.c", 277);

    task->tcb = GFL_TCBMgrAddTask(sys->tcbMgr, func, work, priority);
    if (task->tcb != NULL) {
        task->work = work;
        NNS_FndAppendListObject(&sys->tasks, task);
        return task;
    }
    GFL_HeapFree(task);
    return NULL;
}

void StaActScript_DelTask(StaActScriptSys *sys, StaActScriptTask *task) {
    GFL_HeapFree(task->work);
    GFL_TCBRemove(task->tcb);
    NNS_FndRemoveListObject(&sys->tasks, task);
    GFL_HeapFree(task);
}
