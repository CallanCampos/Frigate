#include "types.h"
#include "app/oekaki.h"
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

static void OekakiComm_RecvPicture(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvTouch(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvAllTouch(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvPictureEnd(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvLeaving(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvNewcomer(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_RecvParentEnd(int netId, int size, const void *data, void *work, NetHandle *handle);
static void OekakiComm_SendPicture(OekakiWork *wk, int index, NetHandle *handle);
static void *OekakiComm_GetPictureRecvBuffer(int netId, void *work, int size);

// The commands from OEKAKI_NET_CMD_BASE
static const NetCommand sOekakiCommands[OEKAKI_NET_CMD_COUNT] = {
    { OekakiComm_RecvPicture, OekakiComm_GetPictureRecvBuffer },
    { OekakiComm_RecvTouch, NULL },
    { OekakiComm_RecvAllTouch, NULL },
    { OekakiComm_RecvNewcomer, NULL },
    { OekakiComm_RecvPictureEnd, NULL },
    { OekakiComm_RecvSignal, NULL },
    { OekakiComm_RecvParentEnd, NULL },
    { OekakiComm_RecvLeaving, NULL },
};

void OekakiComm_AddCommands(OekakiWork *wk) {
    func_02040c20(OEKAKI_NET_CMD_BASE, sOekakiCommands, OEKAKI_NET_CMD_COUNT, wk);
}

static void OekakiComm_RecvPicture(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const OekakiPictureChunk *chunk = data;
    OekakiWork *wk = work;

    if (func_0203ffc4() != 0) {
        // A child puts the chunk into its picture
        if (chunk->index * OEKAKI_CHUNK_SIZE > OEKAKI_PICTURE_SIZE) {
            sys_memcpy32_fast(chunk, wk->picture + chunk->index * OEKAKI_CHUNK_SIZE, 400);
        } else {
            sys_memcpy32_fast(chunk, wk->picture + chunk->index * OEKAKI_CHUNK_SIZE, OEKAKI_CHUNK_SIZE);
        }
        sys_memcpy32_fast(wk->picture, GFL_BitmapGetPixelData(BmpWin_GetBitmap(wk->drawWindow)), OEKAKI_PICTURE_SIZE);
        BmpWin_TransferNow(wk->drawWindow);
    } else {
        // The parent sends the next one
        if (wk->sendChunk * OEKAKI_CHUNK_SIZE < OEKAKI_PICTURE_SIZE) {
            wk->sendChunk++;
            OekakiComm_SendPicture(wk, wk->sendChunk, handle);
        } else {
            Oekaki_RequestSend(wk, OEKAKI_NET_CMD_PICTURE_END, 0);
        }
    }
}

static void OekakiComm_RecvTouch(int netId, int size, const void *data, void *work, NetHandle *handle) {
    OekakiWork *wk = work;

    if (netId != 0) {
        Oekaki_QueuePush((OekakiTouchInfo *)data, &wk->queue, netId);
    }
}

static void OekakiComm_RecvAllTouch(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const OekakiTouchInfo *touch = data;
    OekakiWork *wk = work;
    int i;

    if (wk != NULL && netId == 0) {
        for (i = 0; i < OEKAKI_MEMBER_MAX; i++) {
            wk->allTouch[i] = touch[i];
        }
    }
}

static void OekakiComm_RecvPictureEnd(int netId, int size, const void *data, void *work, NetHandle *handle) {
    OekakiWork *wk = work;

    Oekaki_SetStatus(wk, OEKAKI_SEQ_NEWCOMER_END, 0);
    sys_memset32_fast(0, wk->picture, OEKAKI_PICTURE_SIZE);
    wk->shareNum = UnionApp_CountJoined(wk->param->app);
    wk->shareBit = UnionApp_GetJoined(wk->param->app);
    wk->banFlag = OEKAKI_BAN_NONE;
}

static void OekakiComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const OekakiSignal *signal = data;
    OekakiWork *wk = work;

    if (netId != 0) {
        // A member's signal reaches the parent, which answers it
        if (func_0203ffc4() == 0) {
            wk->signal.netId = signal->netId;
            wk->signal.count = signal->count;
            wk->signal.type = signal->type;
            wk->signal.ok = signal->ok;
            wk->signal.netId = netId;
            wk->signal.count = wk->shareNum;
            switch (signal->type) {
            case OEKAKI_SIGNAL_LEAVE_REQUEST:
                if (wk->shareNum != UnionApp_CountJoined(wk->param->app) ||
                    wk->shareNum != MATH_CountPopulation(UnionApp_GetJoined(wk->param->app))) {
                    wk->signal.ok = FALSE;
                } else {
                    wk->signal.ok = TRUE;
                    UnionApp_OpenEntry(wk->param->app);
                }
                break;
            case OEKAKI_SIGNAL_LEAVE:
                break;
            }
            Oekaki_RequestSend(wk, OEKAKI_NET_CMD_SIGNAL, 0);
        }
    } else {
        switch (signal->type) {
        case OEKAKI_SIGNAL_LEAVE_REQUEST: {
            u8 id = signal->netId;

            if (id == func_0203ffc4()) {
                if (!signal->ok) {
                    Oekaki_SetStatus(wk, OEKAKI_SEQ_END_REFUSED, id);
                } else {
                    wk->signalCount = signal->count;
                    Oekaki_SetStatus(wk, OEKAKI_SEQ_END_OK, signal->netId);
                    wk->leaving = TRUE;
                }
            }
            break;
        }
        case OEKAKI_SIGNAL_LEAVE:
            if (!wk->leaving) {
                Oekaki_SetStatus(wk, OEKAKI_SEQ_MEMBER_LEFT, signal->netId);
            }
            break;
        }
    }
}

static void OekakiComm_RecvLeaving(int netId, int size, const void *data, void *work, NetHandle *handle) {
    OekakiWork *wk = work;

    if (func_0203ffc4() == 0) {
        wk->leavingBits |= 1 << netId;
    }
}

static void OekakiComm_RecvNewcomer(int netId, int size, const void *data, void *work, NetHandle *handle) {
    OekakiWork *wk = work;
    const u8 *id = data;

    if (!wk->parentEnded) {
        Oekaki_SetStatus(wk, OEKAKI_SEQ_NEWCOMER, *id);
        // The parent starts sending the picture
        if (func_0203ffc4() == 0 && wk->procSeq == OEKAKI_PROC_SEQ_MAIN) {
            wk->sendChunk = 0;
            OekakiComm_SendPicture(wk, 0, handle);
        }
    }
}

static void OekakiComm_RecvParentEnd(int netId, int size, const void *data, void *work, NetHandle *handle) {
    OekakiWork *wk = work;

    wk->parentEnded = TRUE;
    if (func_0203ffc4() != 0) {
        Oekaki_SetStatus(wk, OEKAKI_SEQ_PARENT_END_MSG, 0);
    }
}

// Sends a chunk of the picture to every member
// The original narrows what UnionApp_GetJoined returns, which union_app.h declares as a u8 already
static void OekakiComm_SendPicture(OekakiWork *wk, int index, NetHandle *handle) {
    u8 *pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(wk->drawWindow));
    int i;
    u32 *data;
    u32 checksum;
    u8 joined;

    sys_memcpy32_fast(pixels + index * OEKAKI_CHUNK_SIZE, wk->sendChunkData.data, OEKAKI_CHUNK_SIZE);
    data = wk->sendChunkData.data;
    for (checksum = 0, i = 0; i < OEKAKI_CHUNK_SIZE / 4; i++) {
        checksum ^= data[i];
    }
    wk->sendChunkData.checksum = checksum;
    wk->sendChunkData.index = index;
    joined = UnionApp_GetJoined(wk->param->app);
    func_02040f84(func_02040414(GFL_NET_NETID_SERVER), joined, OEKAKI_NET_CMD_PICTURE, sizeof(OekakiPictureChunk),
                  &wk->sendChunkData);
}

static void *OekakiComm_GetPictureRecvBuffer(int netId, void *work, int size) {
    OekakiWork *wk = work;

    return &wk->recvChunks[netId];
}

void OekakiComm_LimitEntry(OekakiWork *wk, u8 limit) {
    UnionApp_LimitEntry(wk->param->app, limit);
}
