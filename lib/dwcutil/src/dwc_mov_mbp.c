#include "types.h"
#include "dwcutil/dwc_mov_mbp.h"
#include "gfl/std.h"
#include "nitro/fs.h"
#include "nitro/mb.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// The DWC utility's parent of a DS Download Play session, libdwcutil.TWL.HYB.a's dwc_mov_mbp.o, as Black 1's
// decompilation (aletheia) has the same functions in the same order; the ROM embeds no file name, and the state names
// below are DWCi_MOV_MBP_STATE_*. The functions have those SDK names (ParentStateCallback and MBPi_CheckAllReboot among
// them); the types, the constants, the tables and the helpers without symbols are ours. It hands the child program's
// ROM file to NitroSDK's multiboot library through MB_RegisterFile and sends it in segments, tracking each child's
// state in bitmaps by AID. It is a second copy of the DS Download Play parent in src/ov181/mbp.c, adapted without the
// heap, the asserts or the started flag, and with the state moved into the caller's work, which is why the child bitmap
// table below is filled at run time. The functions the two copies share unchanged are written as ov181 writes them.

// The console's settings, which the system leaves in main memory for the ARM9: the firmware's user data, whose name
// field sits at 6 and whose length byte follows it. This file reads those two and nothing else
typedef struct {
    u8 unk_00[6];
    u16 nickName[MB_USER_NAME_LENGTH];
    u8 nickNameLength;
} MovMbpUserSettings;

#define MOV_MBP_USER_SETTINGS ((const MovMbpUserSettings *)0x02fffc80)

// The DMA channel the library uses (src/ov181/mbp.c passes 1)
#define MOV_MBP_DMA_NO 2

// The state of each child, by AID
enum {
    MOV_MBP_CHILD_NONE,
    MOV_MBP_CHILD_CONNECTING,
    MOV_MBP_CHILD_REQUEST,
    MOV_MBP_CHILD_ENTRY,
    MOV_MBP_CHILD_DOWNLOADING,
    MOV_MBP_CHILD_BOOTABLE,
    MOV_MBP_CHILD_REBOOT,
};

static MovMbpWork *sWork;

// The callback and state names and the children's bitmaps by state, the three tables of src/ov181/mbp.c. That file
// initializes the bitmaps with the addresses of its own state; this file's state is in its caller's work, so the
// bitmap table starts out zero and DWCi_MOV_MBP_GetChildBmp fills it. The original has that table in .data, as an
// explicit zero. The pragma covers it alone: over DWCi_MOV_MBP_Init it would copy the zeroed state from a .rodata
// template, which the ROM doesn't have. The tables' order sets the order of their strings in .data, which are the ROM's
const char *MovMbp_CallbackNames[] = {
    "MB_COMM_PSTATE_NONE",         "MB_COMM_PSTATE_INIT_COMPLETE",
    "MB_COMM_PSTATE_CONNECTED",    "MB_COMM_PSTATE_DISCONNECTED",
    "MB_COMM_PSTATE_KICKED",       "MB_COMM_PSTATE_REQ_ACCEPTED",
    "MB_COMM_PSTATE_SEND_PROCEED", "MB_COMM_PSTATE_SEND_COMPLETE",
    "MB_COMM_PSTATE_BOOT_REQUEST", "MB_COMM_PSTATE_BOOT_STARTABLE",
    "MB_COMM_PSTATE_REQUESTED",    "MB_COMM_PSTATE_MEMBER_FULL",
    "MB_COMM_PSTATE_END",          "MB_COMM_PSTATE_ERROR",
    "MB_COMM_PSTATE_WAIT_TO_SEND",
};

const char *MovMbp_StateNames[] = {
    "DWCi_MOV_MBP_STATE_STOP",        "DWCi_MOV_MBP_STATE_IDLE",      "DWCi_MOV_MBP_STATE_ENTRY",
    "DWCi_MOV_MBP_STATE_DATASENDING", "DWCi_MOV_MBP_STATE_REBOOTING", "DWCi_MOV_MBP_STATE_COMPLETE",
    "DWCi_MOV_MBP_STATE_CANCEL",      "DWCi_MOV_MBP_STATE_ERROR",
};

#pragma explicit_zero_data on
u16 *MovMbp_ChildBmps[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
#pragma explicit_zero_data reset

static void DWCi_MOV_MBP_ChangeState(u16 state);
static BOOL DWCi_MOV_MBP_RegistFile(const MBGameRegistry *gameInfo);
static void DWCi_MOV_MBP_AcceptChild(u16 childAid);
static void DWCi_MOV_MBP_KickChild(u16 childAid);
static void DWCi_MOV_MBP_StartDownload(u16 childAid);
static void MBPi_CheckAllReboot(void);
static void ParentStateCallback(u16 childAid, u32 status, void *arg);
static int DWCi_MOV_MBP_GetChildState(u16 childAid);

// Accessors of the name tables, which nothing calls, so MWCC doesn't emit them. As in src/ov181/mbp.c, they stand for
// functions the linker dropped from the ROM: reading the tables keeps them in the file's .data, as the ROM has them.
// Their bodies are guesses
static const char *MovMbp_GetStateName(u16 state) {
    return MovMbp_StateNames[state];
}

static const char *MovMbp_GetCallbackName(u32 status) {
    return MovMbp_CallbackNames[status];
}

// Takes a child out of every state's bitmap, with the interrupts off while the six words change
static inline void MovMbp_DisconnectChildFromBmp(u16 childAid) {
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

static inline void MovMbp_DisconnectChild(u16 childAid) {
    MovMbp_DisconnectChildFromBmp(childAid);
    MB_DisconnectChild(childAid);
}

void DWCi_MOV_MBP_MemInit(MovMbpWork *work) {
    sWork = work;
    DWCi_MOV_WH_MemInit(work->loaderContext);
    sWork->mbWorkp = NULL;
    sWork->fileBufp = NULL;
}

void DWCi_MOV_MBP_Init(u32 ggid, u32 tgid) {
    const MovMbpUserSettings *settings = MOV_MBP_USER_SETTINGS;
    MBUserInfo user;

    user.favoriteColor = 1;
    user.nameLength = settings->nickNameLength;
    MI_CpuCopy8(settings->nickName, user.name, MB_USER_NAME_LENGTH * 2);
    user.playerNo = 0;
    sWork->state = (const MovMbpState){ 0 };
    sWork->mbWorkp = sWork->mbWork;
    if (MB_Init(sWork->mbWorkp, &user, ggid, tgid, MOV_MBP_DMA_NO) != MB_ERRCODE_SUCCESS) {
        OS_Terminate();
    }
    MB_SetParentCommParam(MB_COMM_PARENT_SEND_MIN, MOV_MBP_CHILD_MAX);
    MB_CommSetParentStateCallback(ParentStateCallback);
    DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_IDLE);
}

void DWCi_MOV_MBP_Start(const MBGameRegistry *gameInfo, u16 channel) {
    DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_ENTRY);
    if (MB_StartParent(channel) != MB_ERRCODE_SUCCESS) {
        DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_ERROR);
        return;
    }
    if (!DWCi_MOV_MBP_RegistFile(gameInfo)) {
        OS_Terminate();
    }
}

static BOOL DWCi_MOV_MBP_RegistFile(const MBGameRegistry *gameInfo) {
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
            if (MB_ReadSegment(p_file, sWork->fileBufp, MOV_MBP_FILE_BUF_SIZE) &&
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

static void DWCi_MOV_MBP_AcceptChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_ACCEPT)) {
        MovMbp_DisconnectChild(childAid);
    }
}

static void DWCi_MOV_MBP_KickChild(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_KICK)) {
        MovMbp_DisconnectChild(childAid);
        return;
    }
    {
        u32 enabled = OS_DisableInterrupts();

        sWork->state.requestChildBmp &= ~(1 << childAid);
        sWork->state.connectChildBmp &= ~(1 << childAid);
        OS_RestoreInterrupts(enabled);
    }
}

static void DWCi_MOV_MBP_StartDownload(u16 childAid) {
    if (!MB_CommResponseRequest(childAid, MB_COMM_RESPONSE_REQUEST_DOWNLOAD)) {
        MovMbp_DisconnectChild(childAid);
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
void DWCi_MOV_MBP_StartDownloadAll(void) {
    u16 childAid;

    DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_DATASENDING);
    for (childAid = 1; childAid < 16; childAid++) {
        if (!(sWork->state.connectChildBmp & (1 << childAid))) {
            continue;
        }
        if (sWork->state.requestChildBmp & (1 << childAid)) {
            continue;
        }
        if (!(sWork->state.entryChildBmp & (1 << childAid))) {
            MovMbp_DisconnectChild(childAid);
            continue;
        }
        DWCi_MOV_MBP_StartDownload(childAid);
    }
}

BOOL DWCi_MOV_MBP_IsBootableAll(void) {
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

void DWCi_MOV_MBP_StartRebootAll(void) {
    u16 childAid;
    u16 sentChild = 0;

    for (childAid = 1; childAid < 16; childAid++) {
        if (!(sWork->state.bootableChildBmp & (1 << childAid))) {
            continue;
        }
        if (!MB_CommBootRequest(childAid)) {
            MovMbp_DisconnectChild(childAid);
            continue;
        }
        sentChild |= 1 << childAid;
    }
    if (sentChild == 0) {
        DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_ERROR);
        return;
    }
    DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_REBOOTING);
}

void DWCi_MOV_MBP_Cancel(void) {
    DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_CANCEL);
    MB_End();
}

// Ends the library once every child has rebooted into the downloaded program
static void MBPi_CheckAllReboot(void) {
    if (sWork->state.state == DWCi_MOV_MBP_STATE_REBOOTING &&
        sWork->state.connectChildBmp == sWork->state.rebootChildBmp) {
        MB_End();
    }
}

static void ParentStateCallback(u16 childAid, u32 status, void *arg) {
    switch (status) {
    case MB_COMM_PSTATE_INIT_COMPLETE:
        break;
    case MB_COMM_PSTATE_CONNECTED: {
        WMStartParentCallback *p = arg;

        if (DWCi_MOV_MBP_GetState() != DWCi_MOV_MBP_STATE_ENTRY) {
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
        if (DWCi_MOV_MBP_GetChildState(childAid) != MOV_MBP_CHILD_REBOOT) {
            MovMbp_DisconnectChildFromBmp(childAid);
            MBPi_CheckAllReboot();
        }
        break;
    case MB_COMM_PSTATE_KICKED:
        break;
    case MB_COMM_PSTATE_REQUESTED: {
        const MBUserInfo *userInfo;

        if (DWCi_MOV_MBP_GetState() != DWCi_MOV_MBP_STATE_ENTRY) {
            DWCi_MOV_MBP_KickChild(childAid);
            break;
        }
        sWork->state.requestChildBmp |= 1 << childAid;
        DWCi_MOV_MBP_AcceptChild(childAid);
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
        if (DWCi_MOV_MBP_GetState() == DWCi_MOV_MBP_STATE_DATASENDING) {
            DWCi_MOV_MBP_StartDownload(childAid);
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
        MBPi_CheckAllReboot();
        break;
    case MB_COMM_PSTATE_MEMBER_FULL:
        break;
    case MB_COMM_PSTATE_END:
        if (DWCi_MOV_MBP_GetState() == DWCi_MOV_MBP_STATE_REBOOTING) {
            DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_COMPLETE);
        } else {
            DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_STOP);
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
            DWCi_MOV_MBP_ChangeState(DWCi_MOV_MBP_STATE_ERROR);
            break;
        }
        break;
    }
    default:
        OS_Terminate();
    }
}

static void DWCi_MOV_MBP_ChangeState(u16 state) {
    sWork->state.state = state;
}

u16 DWCi_MOV_MBP_GetState(void) {
    return sWork->state.state;
}

u16 DWCi_MOV_MBP_GetChildBmp(MovMbpBmpType bmpType) {
    MovMbp_ChildBmps[MOV_MBP_BMP_CONNECT] = &sWork->state.connectChildBmp;
    MovMbp_ChildBmps[MOV_MBP_BMP_REQUEST] = &sWork->state.requestChildBmp;
    MovMbp_ChildBmps[MOV_MBP_BMP_ENTRY] = &sWork->state.entryChildBmp;
    MovMbp_ChildBmps[MOV_MBP_BMP_DOWNLOAD] = &sWork->state.downloadChildBmp;
    MovMbp_ChildBmps[MOV_MBP_BMP_BOOTABLE] = &sWork->state.bootableChildBmp;
    MovMbp_ChildBmps[MOV_MBP_BMP_REBOOT] = &sWork->state.rebootChildBmp;
    return *MovMbp_ChildBmps[bmpType];
}

static int DWCi_MOV_MBP_GetChildState(u16 childAid) {
    MovMbpState tmpState;
    u16 bitmap = 1 << childAid;
    u32 enabled = OS_DisableInterrupts();

    if ((sWork->state.connectChildBmp & bitmap) == 0) {
        OS_RestoreInterrupts(enabled);
        return MOV_MBP_CHILD_NONE;
    }
    MI_CpuCopy8(&sWork->state, &tmpState, sizeof(MovMbpState));
    OS_RestoreInterrupts(enabled);
    if (tmpState.requestChildBmp & bitmap) {
        return MOV_MBP_CHILD_REQUEST;
    }
    if (tmpState.entryChildBmp & bitmap) {
        return MOV_MBP_CHILD_ENTRY;
    }
    if (tmpState.downloadChildBmp & bitmap) {
        return MOV_MBP_CHILD_DOWNLOADING;
    }
    if (tmpState.bootableChildBmp & bitmap) {
        return MOV_MBP_CHILD_BOOTABLE;
    }
    if (tmpState.rebootChildBmp & bitmap) {
        return MOV_MBP_CHILD_REBOOT;
    }
    return MOV_MBP_CHILD_CONNECTING;
}

MovMbpChildInfo *DWCi_MOV_MBP_GetChildInfo(u16 childAid) {
    if (!(sWork->state.connectChildBmp & (1 << childAid))) {
        return NULL;
    }
    return &sWork->childInfo[childAid - 1];
}

u16 DWCi_MOV_MBP_GetPlayerNo(const u8 *macAddress) {
    u16 childAid;

    for (childAid = 1; childAid < MOV_MBP_CHILD_MAX + 1; childAid++) {
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
