#include "types.h"
#include "app/drawing.h"
#include "field/union_app.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/math.h"

// The drawing board's net commands. The ROM has no name for the file, which is named after what it holds; pokeplatinum
// has the same commands in comm_command_oekaki.c. Names are ours

static void DrawingComm_RecvPicture(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvTouch(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvAllTouch(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvPictureEnd(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvLeaving(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvNewcomer(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_RecvParentEnd(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DrawingComm_SendPicture(DrawingWork *wk, int index, NetHandle *handle);
static void *DrawingComm_GetPictureRecvBuffer(int netId, void *work, int size);

// The commands from DRAWING_NET_CMD_BASE
static const NetCommand sDrawingCommands[DRAWING_NET_CMD_COUNT] = {
    { DrawingComm_RecvPicture, DrawingComm_GetPictureRecvBuffer },
    { DrawingComm_RecvTouch, NULL },
    { DrawingComm_RecvAllTouch, NULL },
    { DrawingComm_RecvNewcomer, NULL },
    { DrawingComm_RecvPictureEnd, NULL },
    { DrawingComm_RecvSignal, NULL },
    { DrawingComm_RecvParentEnd, NULL },
    { DrawingComm_RecvLeaving, NULL },
};

void DrawingComm_AddCommands(DrawingWork *wk) {
    NetCommand_AddTable(DRAWING_NET_CMD_BASE, sDrawingCommands, DRAWING_NET_CMD_COUNT, wk);
}

static void DrawingComm_RecvPicture(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const DrawingPictureChunk *chunk = data;
    DrawingWork *wk = work;

    if (NetSystem_GetNetId() != 0) {
        // A child puts the chunk into its picture
        if (chunk->index * DRAWING_CHUNK_SIZE > DRAWING_PICTURE_SIZE) {
            MIi_CpuCopyFast(chunk, wk->picture + chunk->index * DRAWING_CHUNK_SIZE, 400);
        } else {
            MIi_CpuCopyFast(chunk, wk->picture + chunk->index * DRAWING_CHUNK_SIZE, DRAWING_CHUNK_SIZE);
        }
        MIi_CpuCopyFast(wk->picture, GFL_BitmapGetPixelData(BmpWin_GetBitmap(wk->drawWindow)), DRAWING_PICTURE_SIZE);
        BmpWin_TransferNow(wk->drawWindow);
    } else {
        // The parent sends the next one
        if (wk->sendChunk * DRAWING_CHUNK_SIZE < DRAWING_PICTURE_SIZE) {
            wk->sendChunk++;
            DrawingComm_SendPicture(wk, wk->sendChunk, handle);
        } else {
            Drawing_RequestSend(wk, DRAWING_NET_CMD_PICTURE_END, 0);
        }
    }
}

static void DrawingComm_RecvTouch(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DrawingWork *wk = work;

    if (netId != 0) {
        Drawing_QueuePush((DrawingTouchInfo *)data, &wk->queue, netId);
    }
}

static void DrawingComm_RecvAllTouch(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const DrawingTouchInfo *touch = data;
    DrawingWork *wk = work;
    int i;

    if (wk != NULL && netId == 0) {
        for (i = 0; i < DRAWING_MEMBER_MAX; i++) {
            wk->allTouch[i] = touch[i];
        }
    }
}

static void DrawingComm_RecvPictureEnd(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DrawingWork *wk = work;

    Drawing_SetStatus(wk, DRAWING_SEQ_NEWCOMER_END, 0);
    MIi_CpuClearFast(0, wk->picture, DRAWING_PICTURE_SIZE);
    wk->shareNum = UnionApp_CountJoined(wk->param->app);
    wk->shareBit = UnionApp_GetJoined(wk->param->app);
    wk->banFlag = DRAWING_BAN_NONE;
}

static void DrawingComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const DrawingSignal *signal = data;
    DrawingWork *wk = work;

    if (netId != 0) {
        // A member's signal reaches the parent, which answers it
        if (NetSystem_GetNetId() == 0) {
            wk->signal.netId = signal->netId;
            wk->signal.count = signal->count;
            wk->signal.type = signal->type;
            wk->signal.ok = signal->ok;
            wk->signal.netId = netId;
            wk->signal.count = wk->shareNum;
            switch (signal->type) {
            case DRAWING_SIGNAL_LEAVE_REQUEST:
                if (wk->shareNum != UnionApp_CountJoined(wk->param->app) ||
                    wk->shareNum != MATH_CountPopulation(UnionApp_GetJoined(wk->param->app))) {
                    wk->signal.ok = FALSE;
                } else {
                    wk->signal.ok = TRUE;
                    UnionApp_OpenEntry(wk->param->app);
                }
                break;
            case DRAWING_SIGNAL_LEAVE:
                break;
            }
            Drawing_RequestSend(wk, DRAWING_NET_CMD_SIGNAL, 0);
        }
    } else {
        switch (signal->type) {
        case DRAWING_SIGNAL_LEAVE_REQUEST: {
            u8 id = signal->netId;

            if (id == NetSystem_GetNetId()) {
                if (!signal->ok) {
                    Drawing_SetStatus(wk, DRAWING_SEQ_END_REFUSED, id);
                } else {
                    wk->signalCount = signal->count;
                    Drawing_SetStatus(wk, DRAWING_SEQ_END_OK, signal->netId);
                    wk->leaving = TRUE;
                }
            }
            break;
        }
        case DRAWING_SIGNAL_LEAVE:
            if (!wk->leaving) {
                Drawing_SetStatus(wk, DRAWING_SEQ_MEMBER_LEFT, signal->netId);
            }
            break;
        }
    }
}

static void DrawingComm_RecvLeaving(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DrawingWork *wk = work;

    if (NetSystem_GetNetId() == 0) {
        wk->leavingBits |= 1 << netId;
    }
}

static void DrawingComm_RecvNewcomer(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DrawingWork *wk = work;
    const u8 *id = data;

    if (!wk->parentEnded) {
        Drawing_SetStatus(wk, DRAWING_SEQ_NEWCOMER, *id);
        // The parent starts sending the picture
        if (NetSystem_GetNetId() == 0 && wk->procSeq == DRAWING_PROC_SEQ_MAIN) {
            wk->sendChunk = 0;
            DrawingComm_SendPicture(wk, 0, handle);
        }
    }
}

static void DrawingComm_RecvParentEnd(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DrawingWork *wk = work;

    wk->parentEnded = TRUE;
    if (NetSystem_GetNetId() != 0) {
        Drawing_SetStatus(wk, DRAWING_SEQ_PARENT_END_MSG, 0);
    }
}

// Sends a chunk of the picture to every member
// The original narrows what UnionApp_GetJoined returns, which union_app.h declares as a u8 already
static void DrawingComm_SendPicture(DrawingWork *wk, int index, NetHandle *handle) {
    u8 *pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(wk->drawWindow));
    int i;
    u32 *data;
    u32 checksum;
    u8 joined;

    MIi_CpuCopyFast(pixels + index * DRAWING_CHUNK_SIZE, wk->sendChunkData.data, DRAWING_CHUNK_SIZE);
    data = wk->sendChunkData.data;
    for (checksum = 0, i = 0; i < DRAWING_CHUNK_SIZE / 4; i++) {
        checksum ^= data[i];
    }
    wk->sendChunkData.checksum = checksum;
    wk->sendChunkData.index = index;
    joined = UnionApp_GetJoined(wk->param->app);
    NetCommand_SendHuge(NetHandle_Get(GFL_NET_NETID_SERVER), joined, DRAWING_NET_CMD_PICTURE,
                        sizeof(DrawingPictureChunk), &wk->sendChunkData);
}

static void *DrawingComm_GetPictureRecvBuffer(int netId, void *work, int size) {
    DrawingWork *wk = work;

    return &wk->recvChunks[netId];
}

void DrawingComm_LimitEntry(DrawingWork *wk, u8 limit) {
    UnionApp_LimitEntry(wk->param->app, limit);
}
