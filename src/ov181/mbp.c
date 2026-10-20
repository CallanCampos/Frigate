#include "types.h"
#include "app/mb_parent/mbp.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "nitro/fs.h"
#include "nitro/mb.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitro/wm.h"

// The DS Download Play parent of NitroSDK's demos (mbp.c), which Game Freak took over with its heap, its asserts and
// a fixed-size file buffer. It hands the child program to the library and tracks each child's state in bitmaps by AID

// How many children may connect
#define MBP_CHILD_MAX 1
// The size of the file buffer, which holds the child program's segment
#define MBP_FILE_BUF_SIZE 0x10000
#define MBP_DMA_NO 1

// The state of each child
enum {
    MBP_CHILDSTATE_NONE,
    MBP_CHILDSTATE_CONNECTING,
    MBP_CHILDSTATE_REQUEST,
    MBP_CHILDSTATE_ENTRY,
    MBP_CHILDSTATE_DOWNLOADING,
    MBP_CHILDSTATE_BOOTABLE,
    MBP_CHILDSTATE_REBOOT,
};

typedef struct {
    u16 state;
    // The children in each state, by AID
    u16 connectChildBmp;
    u16 requestChildBmp;
    u16 entryChildBmp;
    u16 downloadChildBmp;
    u16 bootableChildBmp;
    u16 rebootChildBmp;
    // Whether the library runs
    BOOL started;
} MBPState;

typedef struct {
    MBUserInfo user;
    u8 macAddress[6];
    u16 playerNo;
} MBPChildInfo;

static BOOL MBP_RegistFile(const MBGameRegistry *gameInfo);
static void MBP_AcceptChild(u16 childAid);
static void MBP_KickChild(u16 childAid);
static void MBP_StartDownload(u16 childAid);
static void MBP_CheckRebootEnd(void);
static void MBP_ParentStateCallback(u16 childAid, u32 status, void *arg);
static void MBP_ChangeState(u16 state);
static int MBP_GetChildState(u16 childAid);
static u16 MBP_GetChildBmp(int bmpType);
static const char *MBP_GetStateName(u16 state);
static const char *MBP_GetCallbackName(u32 status);

static HeapID sHeapId = 0xffff;
static u8 *sFilebuf;
static MBPState mbpState;
static MBPChildInfo childInfo[MBP_CHILD_MAX];
// The library's work
static void *sCWork;

// The bitmaps of the children by state, and the names of the states and of the callback's statuses for debug output.
// Only the demo's functions that the game doesn't call read them. The names of these and of the file's statics are
// ours, except mbpState and childInfo, which are the demo's
u16 *MBP_ChildBmps[] = {
    &mbpState.connectChildBmp,  &mbpState.requestChildBmp,  &mbpState.entryChildBmp,
    &mbpState.downloadChildBmp, &mbpState.bootableChildBmp, &mbpState.rebootChildBmp,
};

const char *MBP_CallbackNames[] = {
    "MB_COMM_PSTATE_NONE",         "MB_COMM_PSTATE_INIT_COMPLETE",
    "MB_COMM_PSTATE_CONNECTED",    "MB_COMM_PSTATE_DISCONNECTED",
    "MB_COMM_PSTATE_KICKED",       "MB_COMM_PSTATE_REQ_ACCEPTED",
    "MB_COMM_PSTATE_SEND_PROCEED", "MB_COMM_PSTATE_SEND_COMPLETE",
    "MB_COMM_PSTATE_BOOT_REQUEST", "MB_COMM_PSTATE_BOOT_STARTABLE",
    "MB_COMM_PSTATE_REQUESTED",    "MB_COMM_PSTATE_MEMBER_FULL",
    "MB_COMM_PSTATE_END",          "MB_COMM_PSTATE_ERROR",
    "MB_COMM_PSTATE_WAIT_TO_SEND",
};

const char *MBP_StateNames[] = {
    "MBP_STATE_STOP",      "MBP_STATE_IDLE",     "MBP_STATE_ENTRY",  "MBP_STATE_DATASENDING",
    "MBP_STATE_REBOOTING", "MBP_STATE_COMPLETE", "MBP_STATE_CANCEL", "MBP_STATE_ERROR",
};

// Accessors of the tables, which nothing calls, so MWCC doesn't emit them. They stand for the demo's functions that
// the linker dropped from the ROM: reading the tables keeps them in the file's shared .data, as the ROM has them.
// Their bodies are guesses
static u16 MBP_GetChildBmp(int bmpType) {
    return *MBP_ChildBmps[bmpType];
}

static const char *MBP_GetStateName(u16 state) {
    return MBP_StateNames[state];
}

static const char *MBP_GetCallbackName(u32 status) {
    return MBP_CallbackNames[status];
}

static inline void MBP_DisconnectChildFromBmp(u16 aid) {
    u16 aidMask = ~(1 << aid);
    u32 enabled = OS_DisableInterrupts();

    mbpState.connectChildBmp &= aidMask;
    mbpState.requestChildBmp &= aidMask;
    mbpState.entryChildBmp &= aidMask;
    mbpState.downloadChildBmp &= aidMask;
    mbpState.bootableChildBmp &= aidMask;
    mbpState.rebootChildBmp &= aidMask;
    OS_RestoreInterrupts(enabled);
}

static inline void MBP_DisconnectChild(u16 aid) {
    MBP_DisconnectChildFromBmp(aid);
    MB_DisconnectChild(aid);
}

void MBP_Init(HeapID heapId, u32 ggid, u32 tgid) {
    MBUserInfo myUser;
    OSOwnerInfoEx info;

    OS_GetOwnerInfoEx(&info);
    myUser.favoriteColor = info.favoriteColor;
    myUser.nameLength = info.nickNameLength;
    MI_CpuCopy8(info.nickName, myUser.name, info.nickNameLength * 2);
    sHeapId = heapId;
    // The parent is player 0
    myUser.playerNo = 0;
    mbpState = (const MBPState){ MBP_STATE_STOP, 0, 0, 0, 0, 0, 0 };
    sCWork = allocConfigDSSoftwareFeature(sHeapId, MB_SYSTEM_BUF_SIZE, "mbp.c", 160);
    if (MB_Init(sCWork, &myUser, ggid, tgid, MBP_DMA_NO) != MB_SUCCESS) {
        GFL_ASSERT_MSG(0, "ERROR in MB_Init\n");
    }
    MB_SetParentCommParam(MB_COMM_PARENT_SEND_MIN, MBP_CHILD_MAX);
    MB_CommSetParentStateCallback(MBP_ParentStateCallback);
    MBP_ChangeState(MBP_STATE_IDLE);
}

void MBP_Start(const MBGameRegistry *gameInfo, u16 channel) {
    MBP_ChangeState(MBP_STATE_ENTRY);
    if (MB_StartParent(channel) != MB_SUCCESS) {
        MBP_ChangeState(MBP_STATE_ERROR);
        return;
    }
    mbpState.started = TRUE;
    if (!MBP_RegistFile(gameInfo)) {
        GFL_ASSERT_MSG(0, "Illegal multiboot gameInfo\n");
    }
}

static BOOL MBP_RegistFile(const MBGameRegistry *gameInfo) {
    FSFile file;
    FSFile *p_file;
    BOOL ret = FALSE;

    if (gameInfo->romFilePathp == NULL) {
        p_file = NULL;
    } else {
        p_file = &file;
        FS_InitFile(p_file);
        if (!FS_OpenFile(p_file, gameInfo->romFilePathp)) {
            return ret;
        }
    }
    if (MB_GetSegmentLength(p_file) != 0) {
        sFilebuf = GFL_HeapAllocate(sHeapId, MBP_FILE_BUF_SIZE, TRUE, "mbp.c", 289);
        if (sFilebuf != NULL) {
            if (MB_ReadSegment(p_file, sFilebuf, MBP_FILE_BUF_SIZE) && MB_RegisterFile(gameInfo, sFilebuf)) {
                ret = TRUE;
            }
            if (!ret) {
                GFL_HeapFree(sFilebuf);
                // BUG: The buffer is freed but kept, so MBP_FreeBuffers or the end of the library frees it again
#ifdef BUGFIX
                sFilebuf = NULL;
#endif
            }
        }
    }
    if (p_file == &file) {
        FS_CloseFile(&file);
    }
    return ret;
}

static void MBP_AcceptChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_ACCEPT)) {
        MBP_DisconnectChild(childAid);
    }
}

static void MBP_KickChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_KICK)) {
        MBP_DisconnectChild(childAid);
        return;
    }
    {
        u32 enabled = OS_DisableInterrupts();

        mbpState.requestChildBmp &= ~(1 << childAid);
        mbpState.connectChildBmp &= ~(1 << childAid);
        OS_RestoreInterrupts(enabled);
    }
}

static void MBP_StartDownload(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_DOWNLOAD)) {
        MBP_DisconnectChild(childAid);
        return;
    }
    {
        u32 enabled = OS_DisableInterrupts();

        mbpState.entryChildBmp &= ~(1 << childAid);
        mbpState.downloadChildBmp |= 1 << childAid;
        OS_RestoreInterrupts(enabled);
    }
}

BOOL MBP_IsBootableAll(void) {
    u16 aid;

    if (mbpState.connectChildBmp == 0) {
        return FALSE;
    }
    for (aid = 1; aid < 16; aid++) {
        if (!(mbpState.connectChildBmp & (1 << aid))) {
            continue;
        }
        if (!MB_CommIsBootable(aid)) {
            return FALSE;
        }
    }
    return TRUE;
}

void MBP_StartRebootAll(void) {
    u16 aid;
    u16 sentChild = 0;

    for (aid = 1; aid < 16; aid++) {
        if (!(mbpState.bootableChildBmp & (1 << aid))) {
            continue;
        }
        if (!MB_CommBootRequest(aid)) {
            MBP_DisconnectChild(aid);
            continue;
        }
        sentChild |= 1 << aid;
    }
    if (sentChild == 0) {
        MBP_ChangeState(MBP_STATE_ERROR);
        return;
    }
    MBP_ChangeState(MBP_STATE_REBOOTING);
}

void MBP_Cancel(void) {
    MBP_ChangeState(MBP_STATE_CANCEL);
    if (mbpState.started == TRUE) {
        mbpState.started = FALSE;
        MB_End();
    }
}

// Ends the library once every child has rebooted into the downloaded program
static void MBP_CheckRebootEnd(void) {
    if (mbpState.state == MBP_STATE_REBOOTING && mbpState.connectChildBmp == mbpState.rebootChildBmp &&
        mbpState.started == TRUE) {
        mbpState.started = FALSE;
        MB_End();
    }
}

static void MBP_ParentStateCallback(u16 childAid, u32 status, void *arg) {
    switch (status) {
    case MB_COMM_PSTATE_INIT_COMPLETE:
        break;
    case MB_COMM_PSTATE_CONNECTED: {
        WMStartParentCallback *p = arg;

        if (MBP_GetState() != MBP_STATE_ENTRY) {
            break;
        }
        {
            u32 enabled = OS_DisableInterrupts();

            mbpState.connectChildBmp |= 1 << childAid;
            OS_RestoreInterrupts(enabled);
        }
        childInfo[childAid - 1].macAddress[0] = p->macAddress[0];
        childInfo[childAid - 1].macAddress[1] = p->macAddress[1];
        childInfo[childAid - 1].macAddress[2] = p->macAddress[2];
        childInfo[childAid - 1].macAddress[3] = p->macAddress[3];
        childInfo[childAid - 1].macAddress[4] = p->macAddress[4];
        childInfo[childAid - 1].macAddress[5] = p->macAddress[5];
        childInfo[childAid - 1].playerNo = childAid;
        break;
    }
    case MB_COMM_PSTATE_DISCONNECTED:
        if (MBP_GetChildState(childAid) != MBP_CHILDSTATE_REBOOT) {
            MBP_DisconnectChildFromBmp(childAid);
            MBP_CheckRebootEnd();
        }
        break;
    case MB_COMM_PSTATE_KICKED:
        break;
    case MB_COMM_PSTATE_REQUESTED: {
        const MBUserInfo *userInfo;

        if (MBP_GetState() != MBP_STATE_ENTRY) {
            MBP_KickChild(childAid);
            break;
        }
        mbpState.requestChildBmp |= 1 << childAid;
        MBP_AcceptChild(childAid);
        userInfo = MB_CommGetChildUser(childAid);
        if (userInfo != NULL) {
            MI_CpuCopy8(userInfo, &childInfo[childAid - 1].user, sizeof(MBUserInfo));
        }
        break;
    }
    case MB_COMM_PSTATE_REQ_ACCEPTED:
        break;
    case MB_COMM_PSTATE_WAIT_TO_SEND:
        mbpState.requestChildBmp &= ~(1 << childAid);
        mbpState.entryChildBmp |= 1 << childAid;
        MBP_StartDownload(childAid);
        break;
    case MB_COMM_PSTATE_SEND_PROCEED:
        break;
    case MB_COMM_PSTATE_SEND_COMPLETE:
        mbpState.downloadChildBmp &= ~(1 << childAid);
        mbpState.bootableChildBmp |= 1 << childAid;
        break;
    case MB_COMM_PSTATE_BOOT_REQUEST:
        break;
    case MB_COMM_PSTATE_BOOT_STARTABLE:
        mbpState.bootableChildBmp &= ~(1 << childAid);
        mbpState.rebootChildBmp |= 1 << childAid;
        MBP_CheckRebootEnd();
        break;
    case MB_COMM_PSTATE_MEMBER_FULL:
        break;
    case MB_COMM_PSTATE_END:
        if (MBP_GetState() == MBP_STATE_REBOOTING) {
            MBP_ChangeState(MBP_STATE_COMPLETE);
        } else {
            MBP_ChangeState(MBP_STATE_STOP);
        }
        if (sFilebuf != NULL) {
            GFL_HeapFree(sFilebuf);
            sFilebuf = NULL;
        }
        if (sCWork != NULL) {
            GFL_NetFreeAligned32(sCWork);
            sCWork = NULL;
        }
        break;
    case MB_COMM_PSTATE_ERROR: {
        MBErrorStatus *cb = arg;

        switch (cb->errcode) {
        case MB_ERRCODE_WM_FAILURE:
            break;
        case MB_ERRCODE_INVALID_PARAM:
        case MB_ERRCODE_INVALID_STATE:
        case MB_ERRCODE_FATAL:
            MBP_ChangeState(MBP_STATE_ERROR);
            break;
        }
        break;
    }
    default:
        GFL_ASSERT_MSG(0, "Get illegal parent state.\n");
    }
}

static void MBP_ChangeState(u16 state) {
    mbpState.state = state;
}

u16 MBP_GetState(void) {
    return mbpState.state;
}

static int MBP_GetChildState(u16 childAid) {
    MBPState tmpState;
    u16 bitmap = 1 << childAid;
    u32 enabled = OS_DisableInterrupts();

    if ((mbpState.connectChildBmp & bitmap) == 0) {
        OS_RestoreInterrupts(enabled);
        return MBP_CHILDSTATE_NONE;
    }
    MI_CpuCopy8(&mbpState, &tmpState, sizeof(MBPState));
    OS_RestoreInterrupts(enabled);
    if (tmpState.requestChildBmp & bitmap) {
        return MBP_CHILDSTATE_REQUEST;
    }
    if (tmpState.entryChildBmp & bitmap) {
        return MBP_CHILDSTATE_ENTRY;
    }
    if (tmpState.downloadChildBmp & bitmap) {
        return MBP_CHILDSTATE_DOWNLOADING;
    }
    if (tmpState.bootableChildBmp & bitmap) {
        return MBP_CHILDSTATE_BOOTABLE;
    }
    if (tmpState.rebootChildBmp & bitmap) {
        return MBP_CHILDSTATE_REBOOT;
    }
    return MBP_CHILDSTATE_CONNECTING;
}

void MBP_FreeBuffers(void) {
    if (sFilebuf != NULL) {
        GFL_HeapFree(sFilebuf);
        sFilebuf = NULL;
    }
    if (sCWork != NULL) {
        GFL_NetFreeAligned32(sCWork);
        sCWork = NULL;
    }
}

BOOL MBP_IsStarted(void) {
    return mbpState.started;
}
