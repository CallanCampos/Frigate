#include "types.h"
#include "app/download_play.h"
#include "gfl/std.h"
#include "nitro/fs.h"
#include "nitro/mb.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// The parent of a DS Download Play session (download_play.c is a descriptive name: the ROM embeds no file name for
// this file). It hands the child program's ROM file to NitroSDK's multiboot library through MB_RegisterFile and sends
// it in segments, tracking each child's state in bitmaps by AID. It is a second copy of the DS Download Play parent in
// src/ov181/mbp.c, adapted without the heap, the asserts or the started flag, and with the state moved into the
// caller's work, which is why the child bitmap table below is filled at run time. The functions the two copies share
// unchanged are written to match ov181's.

// The console's settings, which the system leaves in main memory for the ARM9: the firmware's user data, whose name
// field sits at 6 and whose length byte follows it. This file reads those two and nothing else
typedef struct {
    u8 unk_00[6];
    u16 nickName[MB_USER_NAME_LENGTH];
    u8 nickNameLength;
} DownloadPlayUserSettings;

#define DOWNLOAD_PLAY_USER_SETTINGS ((const DownloadPlayUserSettings *)0x02fffc80)

// The DMA channel the library uses (src/ov181/mbp.c passes 1)
#define DOWNLOAD_PLAY_DMA_NO 2

// The state of each child, by AID
enum {
    DOWNLOAD_PLAY_CHILD_NONE,
    DOWNLOAD_PLAY_CHILD_CONNECTING,
    DOWNLOAD_PLAY_CHILD_REQUEST,
    DOWNLOAD_PLAY_CHILD_ENTRY,
    DOWNLOAD_PLAY_CHILD_DOWNLOADING,
    DOWNLOAD_PLAY_CHILD_BOOTABLE,
    DOWNLOAD_PLAY_CHILD_REBOOT,
};

static DownloadPlayWork *sWork;

// The children's bitmaps by state and the state and callback names. src/ov181/mbp.c declares them as three tables and
// initializes the bitmaps with the addresses of its own state; this file's state is in its caller's work, so
// DownloadPlay_GetChildBmp fills them instead, and the ROM keeps them as one 0x74-byte data object whose first 24
// bytes are zero. The strings are the ROM's
typedef struct {
    u16 *childBmps[6];
    const char *stateNames[8];
    const char *callbackNames[15];
} DownloadPlayTables;

DownloadPlayTables DownloadPlay_Tables = {
    { NULL, NULL, NULL, NULL, NULL, NULL },
    {
        "DWCi_MOV_MBP_STATE_STOP",
        "DWCi_MOV_MBP_STATE_IDLE",
        "DWCi_MOV_MBP_STATE_ENTRY",
        "DWCi_MOV_MBP_STATE_DATASENDING",
        "DWCi_MOV_MBP_STATE_REBOOTING",
        "DWCi_MOV_MBP_STATE_COMPLETE",
        "DWCi_MOV_MBP_STATE_CANCEL",
        "DWCi_MOV_MBP_STATE_ERROR",
    },
    {
        "MB_COMM_PSTATE_NONE",
        "MB_COMM_PSTATE_INIT_COMPLETE",
        "MB_COMM_PSTATE_CONNECTED",
        "MB_COMM_PSTATE_DISCONNECTED",
        "MB_COMM_PSTATE_KICKED",
        "MB_COMM_PSTATE_REQ_ACCEPTED",
        "MB_COMM_PSTATE_SEND_PROCEED",
        "MB_COMM_PSTATE_SEND_COMPLETE",
        "MB_COMM_PSTATE_BOOT_REQUEST",
        "MB_COMM_PSTATE_BOOT_STARTABLE",
        "MB_COMM_PSTATE_REQUESTED",
        "MB_COMM_PSTATE_MEMBER_FULL",
        "MB_COMM_PSTATE_END",
        "MB_COMM_PSTATE_ERROR",
        "MB_COMM_PSTATE_WAIT_TO_SEND",
    },
};

static void DownloadPlay_ChangeState(u16 state);
static BOOL DownloadPlay_RegistFile(const MBGameRegistry *gameInfo);
static void DownloadPlay_AcceptChild(u16 childAid);
static void DownloadPlay_KickChild(u16 childAid);
static void DownloadPlay_StartDownload(u16 childAid);
static void DownloadPlay_CheckRebootEnd(void);
static void DownloadPlay_ParentStateCallback(u16 childAid, u32 status, void *arg);
static int DownloadPlay_GetChildState(u16 childAid);

// Accessors of the name tables, which nothing calls, so MWCC doesn't emit them. As in src/ov181/mbp.c, they stand for
// functions the linker dropped from the ROM: reading the tables keeps them in the file's .data, as the ROM has them.
// Their bodies are guesses
static const char *DownloadPlay_GetStateName(u16 state) {
    return DownloadPlay_Tables.stateNames[state];
}

static const char *DownloadPlay_GetCallbackName(u32 status) {
    return DownloadPlay_Tables.callbackNames[status];
}

// Takes a child out of every state's bitmap, with the interrupts off while the six words change
static inline void DownloadPlay_DisconnectChildFromBmp(u16 childAid) {
    u16 aidMask = ~(1 << childAid);
    u32 enabled = OS_DisableInterrupts();

    sWork->state.connectChildBmp &= aidMask;
    sWork->state.requestChildBmp &= aidMask;
    sWork->state.entryChildBmp &= aidMask;
    sWork->state.downloadChildBmp &= aidMask;
    sWork->state.bootableChildBmp &= aidMask;
    sWork->state.rebootChildBmp &= aidMask;
    OS_RestoreInterrupts(enabled);
}

static inline void DownloadPlay_DisconnectChild(u16 childAid) {
    DownloadPlay_DisconnectChildFromBmp(childAid);
    MB_DisconnectChild(childAid);
}

void DownloadPlay_SetWork(DownloadPlayWork *work) {
    sWork = work;
    func_ov182_021b9bbc(work->loaderContext);
    sWork->mbWorkp = NULL;
    sWork->fileBufp = NULL;
}

void DownloadPlay_Init(u32 ggid, u32 tgid) {
    const DownloadPlayUserSettings *settings = DOWNLOAD_PLAY_USER_SETTINGS;
    MBUserInfo user;

    user.favoriteColor = 1;
    user.nameLength = settings->nickNameLength;
    MI_CpuCopy8(settings->nickName, user.name, MB_USER_NAME_LENGTH * 2);
    user.playerNo = 0;
    sWork->state = (const DownloadPlayState){ 0 };
    sWork->mbWorkp = sWork->mbWork;
    if (MB_Init(sWork->mbWorkp, &user, ggid, tgid, DOWNLOAD_PLAY_DMA_NO) != MB_ERRCODE_SUCCESS) {
        OS_Terminate();
    }
    MB_SetParentCommParam(MB_COMM_PARENT_SEND_MIN, DOWNLOAD_PLAY_CHILD_MAX);
    MB_CommSetParentStateCallback(DownloadPlay_ParentStateCallback);
    DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_IDLE);
}

void DownloadPlay_Start(const MBGameRegistry *gameInfo, u16 channel) {
    DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_ENTRY);
    if (MB_StartParent(channel) != MB_ERRCODE_SUCCESS) {
        DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_ERROR);
        return;
    }
    if (!DownloadPlay_RegistFile(gameInfo)) {
        OS_Terminate();
    }
}

static BOOL DownloadPlay_RegistFile(const MBGameRegistry *gameInfo) {
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
        sWork->fileBufp = sWork->fileBuf;
        if (sWork->fileBufp != NULL) {
            if (MB_ReadSegment(p_file, sWork->fileBufp, DOWNLOAD_PLAY_FILE_BUF_SIZE) &&
                MB_RegisterFile(gameInfo, sWork->fileBufp)) {
                ret = TRUE;
            }
        }
    }
    if (p_file == &file) {
        FS_CloseFile(&file);
    }
    return ret;
}

static void DownloadPlay_AcceptChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_ACCEPT)) {
        DownloadPlay_DisconnectChild(childAid);
    }
}

static void DownloadPlay_KickChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_KICK)) {
        DownloadPlay_DisconnectChild(childAid);
        return;
    }
    {
        u32 enabled = OS_DisableInterrupts();

        sWork->state.requestChildBmp &= ~(1 << childAid);
        sWork->state.connectChildBmp &= ~(1 << childAid);
        OS_RestoreInterrupts(enabled);
    }
}

static void DownloadPlay_StartDownload(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_DOWNLOAD)) {
        DownloadPlay_DisconnectChild(childAid);
        return;
    }
    {
        u32 enabled = OS_DisableInterrupts();

        sWork->state.entryChildBmp &= ~(1 << childAid);
        sWork->state.downloadChildBmp |= 1 << childAid;
        OS_RestoreInterrupts(enabled);
    }
}

// Starts sending the program: every connected child that is in the entry bitmap gets its download started, and a
// child that is connected without having asked for one is dropped
void DownloadPlay_StartSending(void) {
    u16 childAid;

    DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_DATASENDING);
    for (childAid = 1; childAid < 16; childAid++) {
        if (!(sWork->state.connectChildBmp & (1 << childAid))) {
            continue;
        }
        if (sWork->state.requestChildBmp & (1 << childAid)) {
            continue;
        }
        if (!(sWork->state.entryChildBmp & (1 << childAid))) {
            DownloadPlay_DisconnectChild(childAid);
            continue;
        }
        DownloadPlay_StartDownload(childAid);
    }
}

BOOL DownloadPlay_IsBootableAll(void) {
    u16 childAid;

    if (sWork->state.connectChildBmp == 0) {
        return FALSE;
    }
    for (childAid = 1; childAid < 16; childAid++) {
        if (!(sWork->state.connectChildBmp & (1 << childAid))) {
            continue;
        }
        if (!MB_CommIsBootable(childAid)) {
            return FALSE;
        }
    }
    return TRUE;
}

void DownloadPlay_StartRebootAll(void) {
    u16 childAid;
    u16 sentChild = 0;

    for (childAid = 1; childAid < 16; childAid++) {
        if (!(sWork->state.bootableChildBmp & (1 << childAid))) {
            continue;
        }
        if (!MB_CommBootRequest(childAid)) {
            DownloadPlay_DisconnectChild(childAid);
            continue;
        }
        sentChild |= 1 << childAid;
    }
    if (sentChild == 0) {
        DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_ERROR);
        return;
    }
    DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_REBOOTING);
}

void DownloadPlay_Cancel(void) {
    DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_CANCEL);
    MB_End();
}

// Ends the library once every child has rebooted into the downloaded program
static void DownloadPlay_CheckRebootEnd(void) {
    if (sWork->state.state == DOWNLOAD_PLAY_STATE_REBOOTING &&
        sWork->state.connectChildBmp == sWork->state.rebootChildBmp) {
        MB_End();
    }
}

static void DownloadPlay_ParentStateCallback(u16 childAid, u32 status, void *arg) {
    switch (status) {
    case MB_COMM_PSTATE_INIT_COMPLETE:
        break;
    case MB_COMM_PSTATE_CONNECTED: {
        WMStartParentCallback *p = arg;

        if (DownloadPlay_GetState() != DOWNLOAD_PLAY_STATE_ENTRY) {
            break;
        }
        // The original takes the bitmap's address before turning the interrupts off, and copies the MAC address
        // through one base pointer; without either local the function no longer matches
        {
            u16 *bmp = &sWork->state.connectChildBmp;
            u32 enabled = OS_DisableInterrupts();

            *bmp |= 1 << childAid;
            OS_RestoreInterrupts(enabled);
        }
        {
            u8 *macAddress = sWork->childInfo[childAid - 1].macAddress;

            macAddress[0] = p->macAddress[0];
            macAddress[1] = p->macAddress[1];
            macAddress[2] = p->macAddress[2];
            macAddress[3] = p->macAddress[3];
            macAddress[4] = p->macAddress[4];
            macAddress[5] = p->macAddress[5];
        }
        sWork->childInfo[childAid - 1].playerNo = childAid;
        break;
    }
    case MB_COMM_PSTATE_DISCONNECTED:
        if (DownloadPlay_GetChildState(childAid) != DOWNLOAD_PLAY_CHILD_REBOOT) {
            DownloadPlay_DisconnectChildFromBmp(childAid);
            DownloadPlay_CheckRebootEnd();
        }
        break;
    case MB_COMM_PSTATE_KICKED:
        break;
    case MB_COMM_PSTATE_REQUESTED: {
        const MBUserInfo *userInfo;

        if (DownloadPlay_GetState() != DOWNLOAD_PLAY_STATE_ENTRY) {
            DownloadPlay_KickChild(childAid);
            break;
        }
        sWork->state.requestChildBmp |= 1 << childAid;
        DownloadPlay_AcceptChild(childAid);
        userInfo = MB_CommGetChildUser(childAid);
        if (userInfo != NULL) {
            MI_CpuCopy8(userInfo, &sWork->childInfo[childAid - 1].user, sizeof(MBUserInfo));
        }
        break;
    }
    case MB_COMM_PSTATE_REQ_ACCEPTED:
        break;
    case MB_COMM_PSTATE_WAIT_TO_SEND:
        sWork->state.requestChildBmp &= ~(1 << childAid);
        sWork->state.entryChildBmp |= 1 << childAid;
        if (DownloadPlay_GetState() == DOWNLOAD_PLAY_STATE_DATASENDING) {
            DownloadPlay_StartDownload(childAid);
        }
        break;
    case MB_COMM_PSTATE_SEND_PROCEED:
        break;
    case MB_COMM_PSTATE_SEND_COMPLETE:
        sWork->state.downloadChildBmp &= ~(1 << childAid);
        sWork->state.bootableChildBmp |= 1 << childAid;
        break;
    case MB_COMM_PSTATE_BOOT_REQUEST:
        break;
    case MB_COMM_PSTATE_BOOT_STARTABLE:
        sWork->state.bootableChildBmp &= ~(1 << childAid);
        sWork->state.rebootChildBmp |= 1 << childAid;
        DownloadPlay_CheckRebootEnd();
        break;
    case MB_COMM_PSTATE_MEMBER_FULL:
        break;
    case MB_COMM_PSTATE_END:
        if (DownloadPlay_GetState() == DOWNLOAD_PLAY_STATE_REBOOTING) {
            DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_COMPLETE);
        } else {
            DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_STOP);
        }
        if (sWork->fileBufp != NULL) {
            sWork->fileBufp = NULL;
        }
        if (sWork->mbWorkp != NULL) {
            sWork->mbWorkp = NULL;
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
            DownloadPlay_ChangeState(DOWNLOAD_PLAY_STATE_ERROR);
            break;
        }
        break;
    }
    default:
        OS_Terminate();
    }
}

static void DownloadPlay_ChangeState(u16 state) {
    sWork->state.state = state;
}

u16 DownloadPlay_GetState(void) {
    return sWork->state.state;
}

u16 DownloadPlay_GetChildBmp(int bmpType) {
    DownloadPlay_Tables.childBmps[0] = &sWork->state.connectChildBmp;
    DownloadPlay_Tables.childBmps[1] = &sWork->state.requestChildBmp;
    DownloadPlay_Tables.childBmps[2] = &sWork->state.entryChildBmp;
    DownloadPlay_Tables.childBmps[3] = &sWork->state.downloadChildBmp;
    DownloadPlay_Tables.childBmps[4] = &sWork->state.bootableChildBmp;
    DownloadPlay_Tables.childBmps[5] = &sWork->state.rebootChildBmp;
    return *DownloadPlay_Tables.childBmps[bmpType];
}

static int DownloadPlay_GetChildState(u16 childAid) {
    DownloadPlayState tmpState;
    u16 bitmap = 1 << childAid;
    u32 enabled = OS_DisableInterrupts();

    if ((sWork->state.connectChildBmp & bitmap) == 0) {
        OS_RestoreInterrupts(enabled);
        return DOWNLOAD_PLAY_CHILD_NONE;
    }
    MI_CpuCopy8(&sWork->state, &tmpState, sizeof(DownloadPlayState));
    OS_RestoreInterrupts(enabled);
    if (tmpState.requestChildBmp & bitmap) {
        return DOWNLOAD_PLAY_CHILD_REQUEST;
    }
    if (tmpState.entryChildBmp & bitmap) {
        return DOWNLOAD_PLAY_CHILD_ENTRY;
    }
    if (tmpState.downloadChildBmp & bitmap) {
        return DOWNLOAD_PLAY_CHILD_DOWNLOADING;
    }
    if (tmpState.bootableChildBmp & bitmap) {
        return DOWNLOAD_PLAY_CHILD_BOOTABLE;
    }
    if (tmpState.rebootChildBmp & bitmap) {
        return DOWNLOAD_PLAY_CHILD_REBOOT;
    }
    return DOWNLOAD_PLAY_CHILD_CONNECTING;
}

DownloadPlayChildInfo *DownloadPlay_GetChildInfo(u16 childAid) {
    if (!(sWork->state.connectChildBmp & (1 << childAid))) {
        return NULL;
    }
    return &sWork->childInfo[childAid - 1];
}

u16 DownloadPlay_GetPlayerNoByMac(const u8 *macAddress) {
    u16 childAid;

    for (childAid = 1; childAid < DOWNLOAD_PLAY_CHILD_MAX + 1; childAid++) {
        const u8 *childMac;

        if (!(sWork->state.connectChildBmp & (1 << childAid))) {
            continue;
        }
        childMac = sWork->childInfo[childAid - 1].macAddress;
        if (macAddress[0] == childMac[0] && macAddress[1] == childMac[1] && macAddress[2] == childMac[2] &&
            macAddress[3] == childMac[3] && macAddress[4] == childMac[4] && macAddress[5] == childMac[5]) {
            return sWork->childInfo[childAid - 1].playerNo;
        }
    }
    return 0;
}
