#include "dwc_util_move.h"
#include "app/download_play_sys.h"
#include "dwc_util_connection.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "nitro/mb.h"
#include "nitro/mi.h"
#include "nitro/os.h"

typedef struct DWCUtilMove {
    DownloadPlaySysContext context;
    // The rest of the 0x1e280 bytes that the flow takes, which DownloadPlaySysContext doesn't lay out
    u8 unk_context[0x1e280 - sizeof(DownloadPlaySysContext)];
    MBUserInfo user;
    DWCUtilMoveCallback callback;
    DWCUtilTask *task;
    // Whether a console downloaded the program before there was a callback to tell, and whether the move ended
    u8 entered;
    u8 ended;
} DWCUtilMove;

static void func_ov182_021ae1e8(DWCUtilTask *task, void *arg);
static void func_ov182_021ae254(DWCUtilTask *task, void *arg);

static DownloadPlaySysGame sGame = {
    "dwc:/move/child.srl", NULL, NULL, "dwc:/move/banner.char", "dwc:/move/banner.plt", 0x159, 0,
};
static DWCUtilMove *sMove;

void func_ov182_021ae134(DWCUtilMoveCallback callback) {
    DownloadPlaySysGame game;

    sMove = DWCUtilHeap_AllocClear(sizeof(DWCUtilMove), 32);
    sMove->callback = callback;
    sMove->entered = FALSE;
    sMove->ended = FALSE;
    game = sGame;
    game.gameNamep = DWCUtilMsg_Get(data_ov182_021c40b8, 0);
    game.gameIntroductionp = DWCUtilMsg_Get(data_ov182_021c40b8, 1);
    game.maxPlayerNum = func_ov182_021ae058();
    func_ov182_021ba768(&sMove->context, &game);
    if (!func_ov182_021ba918()) {
        OS_Terminate();
    }
    sMove->task = DWCUtilTask_Create(0, func_ov182_021ae254, NULL, 0x78);
}

void func_ov182_021ae1d4(void) {
    DWCUtilTask_Create(0, func_ov182_021ae1e8, NULL, 0x78);
}

static void func_ov182_021ae1e8(DWCUtilTask *task, void *arg) {
    if (func_ov182_021ba81c()) {
        sMove->ended = TRUE;
        DWCUtilTask_QueueRemoval(0, task);
    }
}

BOOL func_ov182_021ae210(void) {
    if (sMove == NULL) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021ae224(DWCUtilMoveCallback callback) {
    sMove->callback = callback;
}

void func_ov182_021ae238(void) {
    func_ov182_021ba950();
}

const MBUserInfo *func_ov182_021ae240(void) {
    return &sMove->user;
}

static void func_ov182_021ae254(DWCUtilTask *task, void *arg) {
    u8 state;
    u8 changed;

    func_ov182_021ba974();
    if (sMove->entered && !sMove->ended) {
        if (sMove->callback == NULL) {
            return;
        }
        sMove->callback(DWC_UTIL_MOVE_EVENT_ENTERED);
        return;
    }
    func_ov182_021babdc(&state, &changed);
    switch (state) {
    case DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED:
        if (!changed) {
            return;
        }
        MI_CpuCopy16(func_ov182_021bac14(), &sMove->user, sizeof(MBUserInfo));
        if (sMove->callback != NULL) {
            sMove->callback(DWC_UTIL_MOVE_EVENT_ENTERED);
            return;
        }
        sMove->entered = TRUE;
        return;
    case DOWNLOAD_PLAY_SYS_STATE_CHILD_LEFT:
        if (!changed) {
            return;
        }
        if (sMove->callback != NULL) {
            sMove->callback(DWC_UTIL_MOVE_EVENT_CHILD_LEFT);
            return;
        }
        sMove->ended = TRUE;
        return;
    case DOWNLOAD_PLAY_SYS_STATE_ENDED_WIRELESS:
    case DOWNLOAD_PLAY_SYS_STATE_ENDED_PEER:
        if (!changed) {
            return;
        }
        if (sMove->callback != NULL) {
            sMove->callback(DWC_UTIL_MOVE_EVENT_ENDED);
            return;
        }
        sMove->ended = TRUE;
        return;
    case DOWNLOAD_PLAY_SYS_STATE_ENDED_CANCEL:
    case DOWNLOAD_PLAY_SYS_STATE_ENDED_ERROR:
        if (!changed) {
            return;
        }
        if (sMove->callback != NULL) {
            sMove->callback(DWC_UTIL_MOVE_EVENT_FAILED);
            return;
        }
        sMove->ended = TRUE;
        return;
    case DOWNLOAD_PLAY_SYS_STATE_DONE:
        if (!changed) {
            return;
        }
        if (sMove->callback != NULL) {
            sMove->callback(DWC_UTIL_MOVE_EVENT_DONE);
            return;
        }
        sMove->ended = TRUE;
        return;
    case DOWNLOAD_PLAY_SYS_STATE_ENDED:
        DWCUtilTask_QueueRemoval(0, task);
        DWCUtilHeap_FreeAndNull(&sMove);
        return;
    case DOWNLOAD_PLAY_SYS_STATE_NONE:
    case DOWNLOAD_PLAY_SYS_STATE_READY:
    case DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS:
    case DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT:
    case DOWNLOAD_PLAY_SYS_STATE_ENTRY:
    case DOWNLOAD_PLAY_SYS_STATE_SENDING:
    case DOWNLOAD_PLAY_SYS_STATE_START_CONNECT:
    case DOWNLOAD_PLAY_SYS_STATE_CONNECT:
    case DOWNLOAD_PLAY_SYS_STATE_CONNECT_PEER:
    case DOWNLOAD_PLAY_SYS_STATE_CONNECT_ASKED:
    case DOWNLOAD_PLAY_SYS_STATE_PEER_DONE:
    case DOWNLOAD_PLAY_SYS_STATE_END_PARENT:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PARENT:
    case DOWNLOAD_PLAY_SYS_STATE_SENT:
    case DOWNLOAD_PLAY_SYS_STATE_WAIT_END_WIRELESS:
    default:
        return;
    }
}
