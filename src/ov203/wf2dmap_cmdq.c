#include "types.h"
#include "app/wf2dmap/wf2dmap_cmdq.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "gfl/heap.h"

// A queue of the requests of the Wi-Fi Club's 2D map, named after the ROM's "wf2dmap_cmdq.c"

// A ring buffer with one free slot, so num is one more than the queue holds
struct WF2DMAP_REQCMDQ {
    WF2DMAP_REQCMD *buff;
    u32 num;
    u16 top;
    u16 tail;
};

WF2DMAP_REQCMDQ *WF2DMAP_REQCMDQSysInit(u32 num, u32 heapId) {
    WF2DMAP_REQCMDQ *queue;

    queue = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_REQCMDQ), TRUE, "wf2dmap_cmdq.c", 211);
    queue->num = num + 1;
    queue->buff = GFL_HeapAllocate(heapId, sizeof(WF2DMAP_REQCMD) * queue->num, TRUE, "wf2dmap_cmdq.c", 216);
    queue->top = 0;
    queue->tail = 0;
    return queue;
}

void WF2DMAP_REQCMDQSysExit(WF2DMAP_REQCMDQ *queue) {
    GFL_HeapFree(queue->buff);
    GFL_HeapFree(queue);
}

void WF2DMAP_REQCMDQSysCmdPush(WF2DMAP_REQCMDQ *queue, const WF2DMAP_REQCMD *cmd) {
    WF2DMAP_REQCMD dropped;

    if (queue->top == (queue->tail + 1) % queue->num) {
        WF2DMAP_REQCMDQSysCmdPop(queue, &dropped);
    }
    queue->buff[queue->tail] = *cmd;
    queue->tail = (queue->tail + 1) % queue->num;
}

BOOL WF2DMAP_REQCMDQSysCmdPop(WF2DMAP_REQCMDQ *queue, WF2DMAP_REQCMD *cmd) {
    if (queue->tail == queue->top) {
        return FALSE;
    }
    *cmd = queue->buff[queue->top];
    queue->top = (queue->top + 1) % queue->num;
    return TRUE;
}
