#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_CMDQ_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_CMDQ_H

// A queue of the requests of the Wi-Fi Club's 2D map (overlay 203), named after the ROM's "wf2dmap_cmdq.c"

#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"

typedef struct WF2DMAP_REQCMDQ WF2DMAP_REQCMDQ;

// A queue that holds num requests
WF2DMAP_REQCMDQ *WF2DMAP_REQCMDQSysInit(u32 num, u32 heapId);
void WF2DMAP_REQCMDQSysExit(WF2DMAP_REQCMDQ *queue);
// Drops the oldest request when the queue is full
void WF2DMAP_REQCMDQSysCmdPush(WF2DMAP_REQCMDQ *queue, const WF2DMAP_REQCMD *cmd);
// Returns FALSE when the queue is empty
BOOL WF2DMAP_REQCMDQSysCmdPop(WF2DMAP_REQCMDQ *queue, WF2DMAP_REQCMD *cmd);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_CMDQ_H
