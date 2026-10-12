// The multiboot library's use of the wireless manager: the setup of the parent (MB_Init, MB_StartParent), its callbacks
// for the manager's replies, the scan that the child side of the library starts, and the sending of frames (libmb).
// The names are descriptive, except swan's MB_*
#include "mb_private.h"

// What the wireless manager is set up with: how long a connection may be silent, for the manager's four lifetimes
// (table, cam, frame and mp), whether to start in power-save, and the parent's name (a wide string)
static u16 sParentName[24] = L"multiboot";
static MBWmConfig sWmConfig = {
    0x28, 0x28, 0xffff, 5, 1, sParentName, -1, -1,
};

// The parameters of the scan for parents
static WMScanExParam sScanParam;

// What the library works with: the DMA channel and the buffer for the wireless manager, whether to filter the parents
// by their game's ID, and the library's memory, all set by MB_Init
static u16 sDmaNo;
static void *sWmBuf;
static u32 sFilterGgid;
static MBWmWork *sWmWork;
MBCommWork *gMBCommWork;

static BOOL MBWm_IsMultibootBeacon(const WMBssDesc *desc);
static int MBWm_NextChannel(WMScanExParam *param);
static BOOL MBWm_CanSend(void);
static void MBWm_SetLifeTime(void);
static void MBWm_FinishEnd(void *arg);
static void MBWm_ParentCallback(void *arg);
static void MBWm_ChildPortCallback(void *arg);
static void MBWm_ChildCallback(void *arg);
static int MBWm_RandomBeaconPeriod(void);
static BOOL MBWm_CheckBufferSize(u32 parentSize, u32 childSize, u32 children);
static int MBWm_StartInit(void);
static int MBWm_Start(int channel);
static int MBWm_Reset(void);
static void MBWm_ResetTask(MBTask *task);
static BOOL MBWm_End(void);
static void MBWm_SetScanTime(u16 time);
static int MBWm_SetMPData(WMCallbackFunc callback, const void *data, u16 size, u16 unused, u16 aidBitmap);
static void MBWm_ReportError(u16 apiid, int result);

// Whether a beacon is one of a program that offers a multiboot download
static BOOL MBWm_IsMultibootBeacon(const WMBssDesc *desc) {
    if (desc->gameInfo.magicNumber != 1 || !(desc->gameInfo.gameNameCount_attribute & MB_BEACON_ATTR_MULTIBOOT)) {
        return FALSE;
    }
    return TRUE;
}

// Moves the scan on to the next channel that is allowed
static int MBWm_NextChannel(WMScanExParam *param) {
    u16 allowed = func_020810cc();
    u16 channel;
    u16 i;

    if (allowed == 0) {
        return 0;
    }
    channel = param->channelList;
    i = 0;
    do {
        if ((allowed & channel) && param->channelList != channel) {
            param->channelList = channel;
            break;
        }
        i++;
        channel = (channel == 0) ? 1 : channel << 1;
    } while (i < 16);
    return 1;
}

// Whether frames may go out: the multi-point mode runs, nothing is on its way, the end has not been asked for and a
// child is connected
static BOOL MBWm_CanSend(void) {
    BOOL ready = FALSE;
    BOOL idle = FALSE;
    BOOL running = FALSE;

    if (sWmWork->mpStarted == 1 && sWmWork->sending == 0) {
        running = TRUE;
    }
    if (running && sWmWork->ending == 0) {
        idle = TRUE;
    }
    if (idle && sWmWork->connectedMask != 0) {
        ready = TRUE;
    }
    return ready;
}

// Registers the indication callback and sets the connection lifetimes
static void MBWm_SetLifeTime(void) {
    int result;

    result = func_02080e7c(MBWm_ParentCallback);
    MBWm_ReportError(MB_WM_API_INDICATION, result);
    result = func_02082560(MBWm_ParentCallback, sWmConfig.lifeTable, sWmConfig.lifeCam, sWmConfig.lifeFrame,
                           sWmConfig.lifeMp);
    MBWm_ReportError(MB_WM_API_SET_LIFETIME, result);
}

// Tells the library that the wireless manager has ended
static void MBWm_FinishEnd(void *arg) {
    sWmWork->started = 0;
    gMBCommWork->initialized = 0;
    if (sWmWork->commCallback != NULL) {
        sWmWork->commCallback(MB_WM_EVENT_END, arg);
    }
}

// The parent's callback for the wireless manager's replies
static void MBWm_ParentCallback(void *arg) {
    WMCallback *cb = arg;

    switch (cb->apiid) {
    case MB_WM_API_INITIALIZE:
        if (cb->errcode != 0) {
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        MBWm_SetLifeTime();
        return;
    case MB_WM_API_SET_LIFETIME:
        if (cb->errcode != 0) {
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        MBWm_ReportError(MB_WM_API_SET_PARENT_PARAM, func_02081474(MBWm_ParentCallback, &sWmWork->parentParam));
        return;
    case MB_WM_API_SET_PARENT_PARAM:
        sWmWork->commCallback(MB_WM_EVENT_INIT_COMPLETE, arg);
        MBWm_ReportError(MB_WM_API_BEACON_INDICATION, func_0208252c(MBWm_ParentCallback, TRUE));
        return;
    case MB_WM_API_BEACON_INDICATION:
        if (sWmWork->ending == 0) {
            if (cb->errcode != 0) {
                sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
                return;
            }
            MBWm_ReportError(MB_WM_API_START_PARENT, func_02081544(MBWm_ParentCallback, sWmConfig.powerOn));
            return;
        } else {
            if (cb->errcode != 0) {
                sWmWork->ending = 0;
                sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
                return;
            }
            MBWm_FinishEnd(arg);
            return;
        }
    case MB_WM_API_START_PARENT: {
        WMStartParentCallback *parent = arg;

        if (parent->errcode != 0) {
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        switch (parent->state) {
        case WM_STATECODE_PARENT_START:
            sWmWork->connectedMask = 0;
            sWmWork->mpStarted = 0;
            return;
        case WM_STATECODE_CONNECTED:
            if (sWmWork->ending == 1) {
                return;
            }
            sWmWork->connectedMask |= 1 << parent->aid;
            sWmWork->commCallback(MB_WM_EVENT_CONNECTED, arg);
            if (sWmWork->mpStarted == 0 && gMBCommWork->mpRequested == 0) {
                BOOL singleMode = TRUE;

                gMBCommWork->mpRequested = 1;
                if (sWmWork->mpFreq != 0) {
                    singleMode = FALSE;
                }
                MBWm_ReportError(MB_WM_API_START_MP,
                                 func_020818f8(MBWm_ParentCallback, sWmWork->recvBuf, sWmWork->recvBufSize,
                                               sWmWork->sendBuf, sWmWork->sendBufSize, singleMode, 0, 0, 0,
                                               1, 1));
                return;
            }
            if (MBWm_CanSend()) {
                sWmWork->commCallback(MB_WM_EVENT_MP_READY, NULL);
            }
            return;
        case WM_STATECODE_DISCONNECTED:
            sWmWork->connectedMask &= ~(1 << parent->aid);
            sWmWork->commCallback(MB_WM_EVENT_DISCONNECTED, arg);
            return;
        case WM_STATECODE_BEACON_SENT:
            if (sWmWork->ending == 1) {
                return;
            }
            sWmWork->commCallback(MB_WM_EVENT_BEACON_SENT, arg);
            return;
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            return;
        default:
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
    }
    case MB_WM_API_START_MP: {
        WMStartMPCallback *mp = arg;

        gMBCommWork->mpRequested = 0;
        switch (mp->state) {
        case WM_STATECODE_MP_START:
            sWmWork->mpStarted = 1;
            if (sWmWork->ending == 0) {
                sWmWork->commCallback(MB_WM_EVENT_MP_READY, NULL);
            }
            return;
        case WM_STATECODE_MPEND_IND:
            sWmWork->commCallback(MB_WM_EVENT_MP_RECEIVED, mp->recvBuf);
            return;
        default:
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
    }
    case MB_WM_API_SET_MP_DATA: {
        if (MB_PARENT_WORK->multiChild != 0) {
            u32 i;
            u32 children = 0;

            for (i = 0; i < 15; i++) {
                if (MB_PARENT_WORK->childState[i] != 0) {
                    children++;
                    if (children >= 2) {
                        break;
                    }
                }
            }
            if (children == 1) {
                func_0207c160(0x1964);
            }
        }
        sWmWork->sending = 0;
        if (cb->errcode == 0) {
            sWmWork->commCallback(MB_WM_EVENT_MP_SENT_OK, arg);
            if (sWmWork->ending == 0) {
                sWmWork->commCallback(MB_WM_EVENT_MP_READY, NULL);
                return;
            }
        } else if (cb->errcode == 0xa) {
            sWmWork->commCallback(MB_WM_EVENT_MP_SEND_BUSY, arg);
        } else {
            sWmWork->commCallback(MB_WM_EVENT_MP_SEND_FAILED, arg);
            if (sWmWork->ending == 0) {
                sWmWork->commCallback(MB_WM_EVENT_MP_READY, NULL);
                return;
            }
        }
        return;
    }
    case MB_WM_API_RESET:
        if (gMBCommWork->started == 0) {
            if (cb->errcode != 0) {
                sWmWork->ending = 0;
                sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
                return;
            }
            sWmWork->connectedMask = 0;
            sWmWork->mpStarted = 0;
            MBWm_ReportError(MB_WM_API_END, func_02081448(MBWm_ParentCallback));
            return;
        } else {
            func_02080eac(1, NULL, NULL);
            func_02080e7c(NULL);
            if (cb->errcode != 0) {
                sWmWork->ending = 0;
                sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
                return;
            }
            MBWm_ReportError(MB_WM_API_BEACON_INDICATION, func_0208252c(MBWm_ParentCallback, FALSE));
            return;
        }
    case MB_WM_API_END:
        if (cb->errcode != 0) {
            sWmWork->ending = 0;
            sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        MBWm_FinishEnd(arg);
        return;
    case MB_WM_API_DISCONNECT:
        if (cb->errcode == 0) {
            sWmWork->connectedMask &= ~((WMDisconnectCallback *)arg)->goneAidBitmap;
        }
        return;
    case MB_WM_API_INDICATION: {
        WMIndCallback *ind = arg;

        switch (ind->state) {
        case 0x10:
            sWmWork->commCallback(MB_WM_EVENT_IND_0, arg);
            return;
        case 0x11:
            sWmWork->commCallback(MB_WM_EVENT_IND_1, arg);
            return;
        case 0x12:
            sWmWork->commCallback(MB_WM_EVENT_IND_2, arg);
            return;
        case 0x13:
            sWmWork->commCallback(MB_WM_EVENT_IND_3, arg);
            return;
        case 0x16:
            sys_exit();
            return;
        case 0x14:
        case 0x15:
        case 0x17:
            break;
        }
        break;
    }
    default:
        sWmWork->commCallback(MB_WM_EVENT_WM_ERROR, arg);
        return;
    }
}

// The callback for what a parent sends the child on its port
static void MBWm_ChildPortCallback(void *arg) {
    WMPortRecvCallback *cb = arg;

    if (cb->errcode == 0) {
        switch (cb->state) {
        case WM_STATECODE_CONNECTED:
            break;
        case WM_STATECODE_DISCONNECTED:
            break;
        case WM_STATECODE_PORT_RECV:
            sWmWork->commCallback(MB_WM_EVENT_PORT_RECV, arg);
            break;
        case 0x19:
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            break;
        }
    }
}

// The callback of the scan for parents
static void MBWm_ChildCallback(void *arg) {
    MBWmWork *work = sWmWork;
    WMCallback *cb = arg;

    switch (cb->apiid) {
    case MB_WM_API_INITIALIZE:
        if (cb->errcode != 0) {
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        work->commCallback(MB_WM_EVENT_INIT_COMPLETE, arg);
        MBWm_ReportError(MB_WM_API_SET_LIFETIME,
                         func_02082560(MBWm_ChildCallback, sWmConfig.lifeTable, sWmConfig.lifeCam, sWmConfig.lifeFrame,
                                       sWmConfig.lifeMp));
        return;
    case MB_WM_API_SET_LIFETIME:
        if (cb->errcode != 0) {
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        if (sScanParam.channelList == 0) {
            sScanParam.channelList = 1;
        }
        if (sScanParam.maxChannelTime == 0) {
            sScanParam.maxChannelTime = 200;
        }
        sScanParam.bssid[0] = 0xff;
        sScanParam.bssid[1] = 0xff;
        sScanParam.bssid[2] = 0xff;
        sScanParam.bssid[3] = 0xff;
        sScanParam.bssid[4] = 0xff;
        sScanParam.bssid[5] = 0xff;
        sScanParam.scanType = 1;
        sScanParam.ssidLength = 0;
        work->scanning = 1;
        work->scanSwitchChannel = 1;
        MBWm_ReportError(MB_WM_API_START_SCAN, func_020815c8(MBWm_ChildCallback, &sScanParam));
        return;
    case MB_WM_API_START_SCAN: {
        WMStartScanExCallback *scan = arg;

        if (scan->errcode != 0) {
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        switch (scan->state) {
        case WM_STATECODE_PARENT_NOT_FOUND:
            work->commCallback(MB_WM_EVENT_SCAN_NONE, arg);
            if (work->scanning == 0) {
                return;
            }
            if (work->scanSwitchChannel != 0 && !MBWm_NextChannel(&sScanParam)) {
                MBWm_End();
            }
            MBWm_ReportError(MB_WM_API_START_SCAN, func_020815c8(MBWm_ChildCallback, &sScanParam));
            return;
        case WM_STATECODE_PARENT_FOUND: {
            u8 usable = 0;
            u8 i;

            for (i = 0; i < scan->bssDescCount; i++) {
                WMBssDesc *desc = scan->bssDesc[i];
                BOOL sizeOk = FALSE;
                BOOL lengthOk = FALSE;
                BOOL magicOk = FALSE;
                u16 infoLength = desc->gameInfoLength;
                int k;

                if (infoLength >= 0x10 && infoLength <= 0x80) {
                    sizeOk = TRUE;
                }
                if (sizeOk && infoLength == desc->gameInfo.userGameInfoLength + 0x10) {
                    lengthOk = TRUE;
                }
                if (lengthOk && desc->gameInfo.magicNumber == 1) {
                    magicOk = TRUE;
                }
                if (!magicOk || desc->length != (desc->gameInfoLength + 1 + 0x40) / 2 || !MBWm_IsMultibootBeacon(desc)) {
                    usable++;
                    continue;
                }
                if (sFilterGgid == 1
                    && (desc->gameInfo.ggid & sWmConfig.ggidMask) == (sWmConfig.ggid & sWmConfig.ggidMask)) {
                    usable++;
                    continue;
                }
                for (k = 0; k < work->scanCount; k++) {
                    MBScanEntry *entry = &work->scanEntries[k];
                    BOOL same = FALSE;
                    int b;

                    for (b = 0; b < 6; b++) {
                        if (desc->bssid[b] != entry->bssid[b]) {
                            break;
                        }
                    }
                    if (b == 6) {
                        same = TRUE;
                    }
                    if (same) {
                        entry->gameInfoLength = desc->gameInfoLength;
                        entry->gameInfo = desc->gameInfo;
                        cp15_invalidateDC(entry, 0xc0);
                        func_020780e8(sDmaNo, desc, entry, 0xc0, TRUE);
                        work->lastScanIndex = k;
                        work->commCallback(MB_WM_EVENT_SCAN_FOUND, &scan->linkLevel[i]);
                        break;
                    }
                }
                if (k < work->scanCount) {
                    continue;
                }
                if (k < 16) {
                    MBScanEntry *entry = &work->scanEntries[k];

                    work->scanCount = k + 1;
                    sys_memcpy(desc->bssid, entry->bssid, sizeof(entry->bssid));
                    entry->channel = desc->channel;
                    entry->linkLevel = scan->linkLevel[i];
                    MI_CpuCopy16(&desc->gameInfo, &entry->gameInfo, sizeof(entry->gameInfo));
                    cp15_invalidateDC(entry, 0xc0);
                    func_020780e8(sDmaNo, desc, entry, 0xc0, TRUE);
                    work->lastScanIndex = k;
                    work->commCallback(MB_WM_EVENT_SCAN_FOUND, &scan->linkLevel[i]);
                    break;
                }
            }
            if (scan->bssDescCount == usable) {
                work->commCallback(MB_WM_EVENT_SCAN_NONE, arg);
            }
            if (work->scanning != 0) {
                if (work->scanSwitchChannel != 0 && !MBWm_NextChannel(&sScanParam)) {
                    MBWm_End();
                }
                MBWm_ReportError(MB_WM_API_START_SCAN, func_020815c8(MBWm_ChildCallback, &sScanParam));
                return;
            }
            return;
        }
        case WM_STATECODE_BEACON_SENT:
            return;
        default:
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
    }
    case MB_WM_API_END_SCAN:
        if (cb->errcode != 0) {
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        MBWm_ReportError(MB_WM_API_START_CONNECT, func_020816dc(MBWm_ChildCallback, work->target, NULL, TRUE, 0));
        return;
    case MB_WM_API_START_CONNECT: {
        WMStartConnectCallback *connect = arg;

        if (connect->errcode != 0) {
            work->scanCount = 0;
            work->commCallback(MB_WM_EVENT_CONNECT_FAILED, arg);
            return;
        }
        switch (connect->state) {
        case WM_STATECODE_CONNECTED:
            work->connectedAid = connect->aid;
            work->commCallback(MB_WM_EVENT_CONNECT_PARENT, arg);
            work->mpStarted = 1;
            if (func_02080eac(1, MBWm_ChildPortCallback, NULL) == 0) {
                BOOL singleMode = FALSE;

                if (work->mpFreq == 0) {
                    singleMode = TRUE;
                }
                MBWm_ReportError(MB_WM_API_START_MP,
                                 func_020818f8(MBWm_ChildCallback, work->recvBuf, work->recvBufSize, work->sendBuf,
                                               work->sendBufSize, singleMode, 0, 0, 0, 1, 1));
                return;
            }
            return;
        case WM_STATECODE_DISCONNECTED:
            work->commCallback(MB_WM_EVENT_CONNECT_LOST, arg);
            work->connectedMask = 0;
            work->mpStarted = 0;
            return;
        case 6:
            work->connectedMask = 0;
            work->mpStarted = 1;
            return;
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            return;
        default:
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
    }
    case MB_WM_API_START_MP: {
        WMStartMPCallback *mp = arg;

        switch (mp->state) {
        case WM_STATECODE_MP_START:
            work->mpStarted = 1;
            if (MBWm_CanSend()) {
                work->commCallback(MB_WM_EVENT_MP_READY, NULL);
            }
            return;
        case WM_STATECODE_MP_IND:
        case WM_STATECODE_MPACK_IND:
            return;
        default:
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
    }
    case MB_WM_API_SET_MP_DATA: {
        int event;

        work->sending = 0;
        if (cb->errcode == 0) {
            event = MB_WM_EVENT_MP_SENT_OK;
        } else if (cb->errcode == 9) {
            event = MB_WM_EVENT_MP_SEND_BUSY;
        } else {
            event = MB_WM_EVENT_MP_SEND_FAILED;
        }
        work->commCallback(event, arg);
        if (sWmWork->ending == 0) {
            sWmWork->commCallback(MB_WM_EVENT_MP_READY, NULL);
        }
        return;
    }
    case MB_WM_API_RESET:
        if (cb->errcode != 0) {
            work->ending = 0;
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        work->connectedMask = 0;
        work->mpStarted = 0;
        MBWm_ReportError(MB_WM_API_END, func_02081448(MBWm_ChildCallback));
        return;
    case MB_WM_API_END:
        if (cb->errcode != 0) {
            work->ending = 0;
            work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
            return;
        }
        MBWm_FinishEnd(arg);
        return;
    case MB_WM_API_PORT:
        if (MBWm_CanSend()) {
            work->commCallback(MB_WM_EVENT_MP_READY, NULL);
        }
        return;
    case MB_WM_API_INDICATION:
        if (((WMIndCallback *)arg)->state == 0x16) {
            sys_exit();
        }
        return;
    default:
        work->commCallback(MB_WM_EVENT_WM_ERROR, arg);
        return;
    }
}

// A number from 0 to 19 that depends on the console's address and the time
static int MBWm_RandomBeaconPeriod(void) {
    u8 mac[6];
    u32 sum;
    int i;

    func_0207c33c(mac);
    for (i = 0, sum = 0; i < 6; i++) {
        sum += mac[i];
    }
    return (sum + MB_TIME_SEED) * 7 % 20;
}

int MB_Init(void *work, const MBUserInfo *user, u32 ggid, u32 tgid, u32 dma) {
    MBWmWork *wmWork;
    MBCommWork *commWork;
    u32 irq;
    u16 *name;
    int i;

    if (gMBCommWork != NULL && gMBCommWork->initialized != 0) {
        return MB_ERRCODE_INVALID_STATE;
    }
    wmWork = (MBWmWork *)(((u32)work + 0x1f) & ~0x1f);
    commWork = (MBCommWork *)(((u32)wmWork + sizeof(MBWmWork) + 0x1f) & ~0x1f);
    if (tgid == MB_TGID_AUTO) {
        tgid = func_020812b8();
    }
    irq = CPU_IRQDisable();
    sWmConfig.lifeTable = 0xffff;
    sWmConfig.lifeFrame = 5;
    sWmConfig.lifeCam = 0x28;
    sWmConfig.lifeMp = 0x28;
    sWmConfig.powerOn = 1;
    sDmaNo = dma;
    sWmWork = wmWork;
    gMBCommWork = commWork;
    MI_CpuClear32(wmWork, sizeof(MBWmWork));
    MI_CpuClear16(commWork, sizeof(MBCommWork));
    name = wmWork->userName;
    for (i = 0; i < user->nameLength; i++) {
        *name++ = user->name[i];
    }
    name = wmWork->gameName;
    for (i = 0; i < 16; i++) {
        u16 c = *sWmConfig.name;

        if (c == 0) {
            break;
        }
        sWmConfig.name++;
        *name++ = c;
    }
    sys_memcpy(user, &commWork->user, sizeof(MBUserInfo));
    if (user->nameLength < MB_USER_NAME_LENGTH) {
        commWork->user.name[user->nameLength] = 0;
    }
    wmWork->parentSendSize = 0x100;
    wmWork->childSendSize = 8;
    wmWork->sendBufSize = 0;
    wmWork->recvBufSize = 0;
    wmWork->mpFreq = 1;
    wmWork->recvBuf = commWork->recvBuf;
    wmWork->parentParam.entryFlag = 0;
    wmWork->parentParam.multiBootFlag = 0;
    wmWork->parentParam.CS_Flag = 1;
    wmWork->parentParam.KS_Flag = 0;
    wmWork->parentParam.ggid = ggid;
    wmWork->parentParam.tgid = tgid;
    wmWork->parentParam.beaconPeriod = MBWm_RandomBeaconPeriod() + 200;
    wmWork->parentParam.maxEntry = 15;
    wmWork->sending = 0;
    wmWork->started = 0;
    commWork->initialized = 1;
    commWork->mpRequested = 0;
    CPU_SetIRQMask(irq);
    return MB_SUCCESS;
}

// Whether the sizes of the frames fit the buffer that the wireless manager is given
static BOOL MBWm_CheckBufferSize(u32 parentSize, u32 childSize, u32 children) {
    if (parentSize > 0x1fe || parentSize < 0xe4) {
        return FALSE;
    }
    if (childSize > 0x10 || childSize < 8) {
        return FALSE;
    }
    if ((int)((parentSize + 0x26) * 4 + 0x14a + ((childSize + 0x20) * 4 + 0x70) * children) < 0x15e0) {
        return TRUE;
    }
    return FALSE;
}

BOOL MB_SetParentCommParam(u16 sendSize, u16 maxChildren) {
    u32 irq = CPU_IRQDisable();

    if (sWmWork->started != 0) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    if (!MBWm_CheckBufferSize(sendSize, 8, maxChildren)) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    sWmWork->parentParam.maxEntry = maxChildren;
    sWmWork->parentSendSize = sendSize;
    sWmWork->childSendSize = 8;
    CPU_SetIRQMask(irq);
    return TRUE;
}

// Starts the wireless manager, or the parent's setup if it is already running
static int MBWm_StartInit(void) {
    int result;

    sWmWork->mpStarted = 0;
    sWmWork->connectedMask = 0;
    sWmWork->ending = 0;
    sWmWork->gameName[8] = 0;
    MBWm_SetScanTime(30);
    if (gMBCommWork->started == 0) {
        do {
            result = func_020813c4(sWmBuf, sWmWork->wmCallback, sDmaNo);
        } while (result == WM_ERRCODE_BUSY);
        if (result != WM_ERRCODE_OPERATING) {
            return MB_ERRCODE_WM_FAILURE;
        }
        func_02080e7c(sWmWork->wmCallback);
        sWmWork->started = 1;
        return MB_SUCCESS;
    }
    func_02080e7c(sWmWork->wmCallback);
    sWmWork->started = 1;
    MBWm_SetLifeTime();
    return MB_SUCCESS;
}

// Sets the library's work up for a parent on a channel and starts the wireless manager
static int MBWm_Start(int channel) {
    MBCommPStateCallback callback;
    u32 irq = CPU_IRQDisable();
    int result;
    int i;

    sWmWork->parentParam.channel = channel;
    sWmBuf = (void *)(((u32)gMBCommWork + sizeof(MBCommParentWork) + 0x1f) & ~0x1f);
    callback = MB_PARENT_WORK->callback;
    MI_CpuClear16(&MB_PARENT_WORK->childUser, sizeof(MBCommParentWork) - sizeof(MBCommWork));
    MB_CommSetParentStateCallback(callback);
    gMBCommWork->blockSize = sWmWork->parentSendSize - 6;
    MBFrame_SetPayloadSize(sWmWork->childSendSize);
    MBFrame_SetBuffer(&MB_PARENT_WORK->fragmentBuf);
    for (i = 0; i < 15; i++) {
        MB_PARENT_WORK->childState[i] = 0;
        MB_PARENT_WORK->childFile[i] = -1;
    }
    MB_PARENT_WORK->fileCount = 0;
    MI_CpuClear16(MB_PARENT_WORK->files, sizeof(MB_PARENT_WORK->files));
    sys_memset(MB_PARENT_WORK->response, 0, sizeof(MB_PARENT_WORK->response));
    sWmWork->mode = 1;
    sWmWork->commCallback = MBComm_OnWmEvent;
    sWmWork->wmCallback = MBWm_ParentCallback;
    sWmWork->parentParam.parentMaxSize = sWmWork->parentSendSize;
    sWmWork->sendBufSize = (sWmWork->parentParam.parentMaxSize + 0x23) & ~0x1f;
    sWmWork->parentParam.childMaxSize = sWmWork->childSendSize;
    sWmWork->recvBufSize = (((sWmWork->parentParam.childMaxSize + 0xe) * 15 + 0x29) & ~0x1f) * 2;
    MBGameInfo_Init();
    result = MBWm_StartInit();
    CPU_SetIRQMask(irq);
    MB_PARENT_WORK->multiChild = func_0207ca48(15, 1);
    return result;
}

int MB_StartParent(int channel) {
    gMBCommWork->started = 1;
    return MBWm_Start(channel);
}

static int MBWm_Reset(void) {
    int result = func_02081424(sWmWork->wmCallback);

    MBWm_ReportError(MB_WM_API_RESET, result);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

static void MBWm_ResetTask(MBTask *task) {
    MBWm_Reset();
}

// Asks the wireless manager to end, once what it is sending is done
static BOOL MBWm_End(void) {
    BOOL result = TRUE;
    u32 irq = CPU_IRQDisable();

    if (sWmWork->started == 0) {
        MBWm_FinishEnd(NULL);
    } else if (sWmWork->ending == 0) {
        sWmWork->scanning = 0;
        sWmWork->ending = 1;
        if (MBTask_IsThreadStarted()) {
            MBTask_EndThread(MBWm_ResetTask);
            result = FALSE;
        } else {
            result = MBWm_Reset();
        }
    }
    CPU_SetIRQMask(irq);
    return result;
}

void MB_End(void) {
    u32 irq = CPU_IRQDisable();

    if (gMBCommWork->started == 0) {
        sys_exit();
    }
    MBWm_End();
    CPU_SetIRQMask(irq);
}

void MB_DisconnectChild(u16 aid) {
    func_02081774(MBWm_ParentCallback, aid);
    if (aid != 0 && aid < 16) {
        MB_PARENT_WORK->childUnk148a[aid - 1] = 0;
        sys_memset(&MB_PARENT_WORK->childRequestGgid[aid - 1], 0, sizeof(u32));
        sys_memset(&MB_PARENT_WORK->childUser[aid - 1], 0, sizeof(MBUserInfo));
        MBFrame_ClearChild(aid);
        MB_PARENT_WORK->response[aid - 1] = 0;
        MBComm_LeaveFile(aid);
        if (MB_PARENT_WORK->childBitmap & (1 << aid)) {
            MB_PARENT_WORK->childCount--;
            MB_PARENT_WORK->childBitmap &= ~(1 << aid);
        }
        MB_PARENT_WORK->childState[aid - 1] = 0;
    }
}

static void MBWm_SetScanTime(u16 time) {
    sScanParam.maxChannelTime = time;
}

static int MBWm_SetMPData(WMCallbackFunc callback, const void *data, u16 size, u16 unused, u16 aidBitmap) {
    int result = func_02081998(callback, NULL, data, size, aidBitmap, 1, 3);

    MBWm_ReportError(MB_WM_API_SET_MP_DATA, result);
    return result;
}

// Sends a frame to the children in a bitmap with the wireless manager. Gives 1 when it cannot be sent now
int MBWm_SendToChildren(const void *data, u32 size, u32 aidBitmap) {
    u16 length = size;
    u16 bitmap = aidBitmap;
    MBWmWork *work = sWmWork;
    int result;

    if (work->mpStarted == 0 || work->ending == 1) {
        return 1;
    }
    switch (work->mode) {
    case 1:
        result = MBWm_SetMPData(work->wmCallback, data, length, work->mpFreq == 0 ? 1000 : 0, bitmap);
        if (result == WM_ERRCODE_OPERATING) {
            sWmWork->sending = 1;
        }
        if (result == WM_ERRCODE_OPERATING) {
            return 0;
        }
        return result;
    case 2:
        result = MBWm_SetMPData(MBWm_ChildCallback, data, length, 0, bitmap);
        if (result == WM_ERRCODE_OPERATING) {
            sWmWork->sending = 1;
        }
        if (result == WM_ERRCODE_OPERATING) {
            return 0;
        }
        return result;
    default:
        return 1;
    }
}

u32 MBWm_GetGgid(void) {
    return sWmWork->parentParam.ggid;
}

u16 MBWm_GetTgid(void) {
    return sWmWork->parentParam.tgid;
}

// The attributes of the parent's beacon, from its flags
u8 MBWm_GetAttribute(void) {
    return (u8)((sWmWork->parentParam.entryFlag ? 1 : 0) | (sWmWork->parentParam.multiBootFlag ? 2 : 0) |
                (sWmWork->parentParam.KS_Flag ? 4 : 0) | (sWmWork->parentParam.CS_Flag ? 8 : 0));
}

BOOL MBWm_IsStarted(void) {
    if (sWmWork->started == 1) {
        return TRUE;
    }
    return FALSE;
}

// Tells the library's callback when a call to the wireless manager did not start its operation
static void MBWm_ReportError(u16 apiid, int result) {
    if (result != WM_ERRCODE_OPERATING && result != WM_ERRCODE_SUCCESS) {
        WMCallback cb;

        cb.apiid = apiid;
        cb.errcode = result;
        sWmWork->commCallback(MB_WM_EVENT_ERROR, &cb);
    }
}
