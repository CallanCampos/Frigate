// The multiboot library's use of the wireless manager: the setup of the parent (MB_Init, MB_StartParent), its callbacks
// for the manager's replies, the scan that the child side of the library starts, and the sending of frames (libmb).
// It is the SDK's mb_wm_base.c, as ntrtwl's reconstruction of NitroSDK has it. The names are NitroSDK's, as that
// reconstruction has them, except MBWm_IsMultibootBeacon and the file's objects other than the lifetimes and the
// power-save flag (sParentName and the rest), which are ours
#include "mb_private.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// What the wireless manager is set up with: the parent's name (a wide string), whether to start in power-save, how
// long a connection may be silent, for the manager's four lifetimes (frame, cam, table and mp), and the game ID that
// the scan's filter compares, through its mask. They are separate statics, as in ntrtwl's NitroSDK: under -ipa file
// MWCC sorts a file's statics by size, and this declaration order is the one whose sort gives the ROM's layout
static u16 sParentName[24] = L"multiboot";
static BOOL mbi_power_save_mode = TRUE;
static u16 mbi_life_frame = 5;
static u16 mbi_life_cam = 0x28;
static u16 mbi_life_table_no = 0xffff;
static u16 mbi_life_mp = 0x28;
static u32 sParentGgid = 0xffffffff;
static u32 sParentGgidMask = 0xffffffff;

// The parameters of the scan for parents, 32-aligned as the ROM places them, then what the library works with: its
// memory, the DMA channel and the buffer for the wireless manager, and whether to filter the parents by their game's
// ID, all set by MB_Init. The order is again the one that the size sort turns into the ROM's
static WMScanExParam sScanParam ATTRIBUTE_ALIGN(32);
MB_CommCommonWork *mbc;
static u16 sDmaNo;
static MBiParam *sWmWork;
static u32 sFilterGgid;
static void *sWmBuf;

static BOOL MBWm_IsMultibootBeacon(const WMBssDesc *desc);
static int changeScanChannel(WMScanExParam *param);
static BOOL MBi_IsSendEnabled(void);
static void MBi_OnInitializeDone(void);
static void MBi_EndCommon(void *arg);
static void MBi_ParentCallback(void *arg);
static void MBi_ChildPortCallback(void *arg);
static void MBi_ChildCallback(void *arg);
static int MBi_GetBeaconPeriodDispersion(void);
static BOOL MBi_IsCommSizeValid(u32 parentSize, u32 childSize, u32 children);
static int MBi_StartCommon(void);
static int MBi_StartParentCore(int channel);
static int MBi_CallReset(void);
static void MBi_OnReset(MBiTaskInfo *task);
static BOOL MBi_CommEnd(void);
static void MBi_SetMaxScanTime(u16 time);
static int MBi_SetMPData(WMCallbackFunc callback, const void *data, u16 size, u16 unused, u16 aidBitmap);
static void MBi_CheckWmErrcode(u16 apiid, int result);

// Whether a beacon is one of a program that offers a multiboot download
static BOOL MBWm_IsMultibootBeacon(const WMBssDesc *desc) {
    if (desc->gameInfo.magicNumber != 1 || !(desc->gameInfo.gameNameCount_attribute & WM_ATTR_FLAG_MB)) {
        return FALSE;
    }
    return TRUE;
}

// Moves the scan on to the next channel that is allowed
static int changeScanChannel(WMScanExParam *param) {
    u16 allowed = WM_GetAllowedChannel();
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
static BOOL MBi_IsSendEnabled(void) {
    BOOL ready = FALSE;
    BOOL idle = FALSE;
    BOOL running = FALSE;

    if (sWmWork->mpStarted == 1 && sWmWork->mpBusy == 0) {
        running = TRUE;
    }
    if (running && sWmWork->endReq == 0) {
        idle = TRUE;
    }
    if (idle && sWmWork->child_bitmap != 0) {
        ready = TRUE;
    }
    return ready;
}

// Registers the indication callback and sets the connection lifetimes
static void MBi_OnInitializeDone(void) {
    int result;

    result = WM_SetIndCallback(MBi_ParentCallback);
    MBi_CheckWmErrcode(WM_APIID_INDICATION, result);
    result = WM_SetLifeTime(MBi_ParentCallback, mbi_life_table_no, mbi_life_cam, mbi_life_frame, mbi_life_mp);
    MBi_CheckWmErrcode(WM_APIID_SET_LIFETIME, result);
}

// Tells the library that the wireless manager has ended
static void MBi_EndCommon(void *arg) {
    sWmWork->mbIsStarted = 0;
    mbc->isMbInitialized = 0;
    if (sWmWork->callback != NULL) {
        sWmWork->callback(MB_CALLBACK_END_COMPLETE, arg);
    }
}

// The parent's callback for the wireless manager's replies
static void MBi_ParentCallback(void *arg) {
    WMCallback *cb = arg;

    switch (cb->apiid) {
    case WM_APIID_INITIALIZE:
        if (cb->errcode != 0) {
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        MBi_OnInitializeDone();
        return;
    case WM_APIID_SET_LIFETIME:
        if (cb->errcode != 0) {
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        MBi_CheckWmErrcode(WM_APIID_SET_P_PARAM, WM_SetParentParameter(MBi_ParentCallback, &sWmWork->parentParam));
        return;
    case WM_APIID_SET_P_PARAM:
        sWmWork->callback(MB_CALLBACK_INIT_COMPLETE, arg);
        MBi_CheckWmErrcode(WM_APIID_SET_BEACON_IND, WM_SetBeaconIndication(MBi_ParentCallback, TRUE));
        return;
    case WM_APIID_SET_BEACON_IND:
        if (sWmWork->endReq == 0) {
            if (cb->errcode != 0) {
                sWmWork->callback(MB_CALLBACK_ERROR, arg);
                return;
            }
            MBi_CheckWmErrcode(WM_APIID_START_PARENT, WMi_StartParentEx(MBi_ParentCallback, mbi_power_save_mode));
            return;
        } else {
            if (cb->errcode != 0) {
                sWmWork->endReq = 0;
                sWmWork->callback(MB_CALLBACK_ERROR, arg);
                return;
            }
            MBi_EndCommon(arg);
            return;
        }
    case WM_APIID_START_PARENT: {
        WMStartParentCallback *parent = arg;

        if (parent->errcode != 0) {
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        switch (parent->state) {
        case WM_STATECODE_PARENT_START:
            sWmWork->child_bitmap = 0;
            sWmWork->mpStarted = 0;
            return;
        case WM_STATECODE_CONNECTED:
            if (sWmWork->endReq == 1) {
                return;
            }
            sWmWork->child_bitmap |= 1 << parent->aid;
            sWmWork->callback(MB_CALLBACK_CHILD_CONNECTED, arg);
            if (sWmWork->mpStarted == 0 && mbc->start_mp_busy == 0) {
                BOOL singleMode = TRUE;

                mbc->start_mp_busy = 1;
                if (sWmWork->contSend != 0) {
                    singleMode = FALSE;
                }
                MBi_CheckWmErrcode(WM_APIID_START_MP, WM_StartMPEx(MBi_ParentCallback, (u16 *)sWmWork->recvBuf,
                                                                   sWmWork->recvBufSize, (u16 *)sWmWork->sendBuf,
                                                                   sWmWork->sendBufSize, singleMode, 0, 0, 0, 1, 1));
                return;
            }
            if (MBi_IsSendEnabled()) {
                sWmWork->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
            }
            return;
        case WM_STATECODE_DISCONNECTED:
            sWmWork->child_bitmap &= ~(1 << parent->aid);
            sWmWork->callback(MB_CALLBACK_CHILD_DISCONNECTED, arg);
            return;
        case WM_STATECODE_BEACON_SENT:
            if (sWmWork->endReq == 1) {
                return;
            }
            sWmWork->callback(MB_CALLBACK_BEACON_SENT, arg);
            return;
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            return;
        default:
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
    }
    case WM_APIID_START_MP: {
        WMStartMPCallback *mp = arg;

        mbc->start_mp_busy = 0;
        switch (mp->state) {
        case WM_STATECODE_MP_START:
            sWmWork->mpStarted = 1;
            if (sWmWork->endReq == 0) {
                sWmWork->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
            }
            return;
        case WM_STATECODE_MPEND_IND:
            sWmWork->callback(MB_CALLBACK_MP_PARENT_RECV, mp->recvBuf);
            return;
        default:
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
    }
    case WM_APIID_SET_MP_DATA: {
        if (pPwork->useWvrFlag != 0) {
            u32 i;
            u32 children = 0;

            for (i = 0; i < 15; i++) {
                if (pPwork->p_comm_state[i] != 0) {
                    children++;
                    if (children >= 2) {
                        break;
                    }
                }
            }
            if (children == 1) {
                OS_SpinWaitSysCycles(0x1964);
            }
        }
        sWmWork->mpBusy = 0;
        if (cb->errcode == 0) {
            sWmWork->callback(MB_CALLBACK_MP_PARENT_SENT, arg);
            if (sWmWork->endReq == 0) {
                sWmWork->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
                return;
            }
        } else if (cb->errcode == 0xa) {
            sWmWork->callback(MB_CALLBACK_SEND_QUEUE_FULL_ERR, arg);
        } else {
            sWmWork->callback(MB_CALLBACK_MP_PARENT_SENT_ERR, arg);
            if (sWmWork->endReq == 0) {
                sWmWork->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
                return;
            }
        }
        return;
    }
    case WM_APIID_RESET:
        if (mbc->is_started_ex == 0) {
            if (cb->errcode != 0) {
                sWmWork->endReq = 0;
                sWmWork->callback(MB_CALLBACK_ERROR, arg);
                return;
            }
            sWmWork->child_bitmap = 0;
            sWmWork->mpStarted = 0;
            MBi_CheckWmErrcode(WM_APIID_END, WM_End(MBi_ParentCallback));
            return;
        } else {
            WM_SetPortCallback(1, NULL, NULL);
            WM_SetIndCallback(NULL);
            if (cb->errcode != 0) {
                sWmWork->endReq = 0;
                sWmWork->callback(MB_CALLBACK_ERROR, arg);
                return;
            }
            MBi_CheckWmErrcode(WM_APIID_SET_BEACON_IND, WM_SetBeaconIndication(MBi_ParentCallback, FALSE));
            return;
        }
    case WM_APIID_END:
        if (cb->errcode != 0) {
            sWmWork->endReq = 0;
            sWmWork->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        MBi_EndCommon(arg);
        return;
    case WM_APIID_DISCONNECT:
        if (cb->errcode == 0) {
            sWmWork->child_bitmap &= ~((WMDisconnectCallback *)arg)->disconnectedBitmap;
        }
        return;
    case WM_APIID_INDICATION: {
        WMIndCallback *ind = arg;

        switch (ind->state) {
        case 0x10:
            sWmWork->callback(MB_CALLBACK_BEACON_RECV, arg);
            return;
        case 0x11:
            sWmWork->callback(MB_CALLBACK_DISASSOCIATE, arg);
            return;
        case 0x12:
            sWmWork->callback(MB_CALLBACK_REASSOCIATE, arg);
            return;
        case 0x13:
            sWmWork->callback(MB_CALLBACK_AUTHENTICATE, arg);
            return;
        case 0x16:
            OS_Terminate();
            return;
        case 0x14:
        case 0x15:
        case 0x17:
            break;
        }
        break;
    }
    default:
        sWmWork->callback(MB_CALLBACK_ERROR, arg);
        return;
    }
}

// The callback for what a parent sends the child on its port
static void MBi_ChildPortCallback(void *arg) {
    WMPortRecvCallback *cb = arg;

    if (cb->errcode == 0) {
        switch (cb->state) {
        case WM_STATECODE_CONNECTED:
            break;
        case WM_STATECODE_DISCONNECTED:
            break;
        case WM_STATECODE_PORT_RECV:
            sWmWork->callback(MB_CALLBACK_MP_CHILD_RECV, arg);
            break;
        case 0x19:
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            break;
        }
    }
}

// The callback of the scan for parents
static void MBi_ChildCallback(void *arg) {
    WMCallback *cb = arg;
    MBiParam *const work = sWmWork;

    switch (cb->apiid) {
    case WM_APIID_INITIALIZE:
        if (cb->errcode != 0) {
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        work->callback(MB_CALLBACK_INIT_COMPLETE, arg);
        MBi_CheckWmErrcode(WM_APIID_SET_LIFETIME, WM_SetLifeTime(MBi_ChildCallback, mbi_life_table_no, mbi_life_cam,
                                                                 mbi_life_frame, mbi_life_mp));
        return;
    case WM_APIID_SET_LIFETIME:
        if (cb->errcode != 0) {
            work->callback(MB_CALLBACK_ERROR, arg);
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
        work->scanning_flag = 1;
        work->scan_channel_flag = 1;
        MBi_CheckWmErrcode(WM_APIID_START_SCAN_EX, WM_StartScanEx(MBi_ChildCallback, &sScanParam));
        return;
    case WM_APIID_START_SCAN_EX: {
        WMStartScanExCallback *scan = arg;

        if (scan->errcode != 0) {
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        switch (scan->state) {
        case 3:
            return;
        case WM_STATECODE_PARENT_FOUND: {
            u8 i;
            u8 usable;
            WMBssDesc *desc;
            ParentInfo *entries;
            int k;

            usable = 0;
            for (i = 0; i < scan->bssDescCount; i++) {
                desc = scan->bssDesc[i];
                entries = work->parent_info;
                if (!WM_IsValidGameBeacon(desc) || !MBWm_IsMultibootBeacon(desc)) {
                    usable++;
                    continue;
                }
                if (sFilterGgid == 1 && (desc->gameInfo.ggid & sParentGgidMask) == (sParentGgid & sParentGgidMask)) {
                    usable++;
                    continue;
                }
                for (k = 0; k < work->found_parent_count; k++) {
                    if (WM_IsBssidEqual(desc->bssid, entries[k].bssid)) {
                        entries[k].gameInfoLength = desc->gameInfoLength;
                        entries[k].gameInfo = desc->gameInfo;
                        DC_InvalidateRange(&work->parent_info[k], 0xc0);
                        MI_DmaCopy16(sDmaNo, desc, &work->parent_info[k], 0xc0);
                        work->last_found_parent_no = k;
                        work->callback(MB_CALLBACK_PARENT_FOUND, &scan->linkLevel[i]);
                        goto restart;
                    }
                }
                if (k < 16) {
                    work->found_parent_count = k + 1;
                    MI_CpuCopy8(desc->bssid, entries[k].bssid, sizeof(desc->bssid));
                    entries[k].channel = desc->channel;
                    entries[k].linkLevel = scan->linkLevel[i];
                    MI_CpuCopy16(&desc->gameInfo, &entries[k].gameInfo, sizeof(WMGameInfo));
                    DC_InvalidateRange(&work->parent_info[k], 0xc0);
                    MI_DmaCopy16(sDmaNo, desc, &work->parent_info[k], 0xc0);
                    work->last_found_parent_no = k;
                    work->callback(MB_CALLBACK_PARENT_FOUND, &scan->linkLevel[i]);
                    goto restart;
                }
            }
            if (scan->bssDescCount == usable) {
                work->callback(MB_CALLBACK_PARENT_NOT_FOUND, arg);
            }
        restart:
            if (!work->scanning_flag) {
                return;
            }
            if (work->scan_channel_flag) {
                if (!changeScanChannel(&sScanParam)) {
                    MBi_CommEnd();
                }
            }
            MBi_CheckWmErrcode(WM_APIID_START_SCAN_EX, WM_StartScanEx(MBi_ChildCallback, &sScanParam));
            return;
        }
        case WM_STATECODE_PARENT_NOT_FOUND:
            work->callback(MB_CALLBACK_PARENT_NOT_FOUND, arg);
            if (!work->scanning_flag) {
                return;
            }
            if (work->scan_channel_flag) {
                if (!changeScanChannel(&sScanParam)) {
                    MBi_CommEnd();
                }
            }
            MBi_CheckWmErrcode(WM_APIID_START_SCAN_EX, WM_StartScanEx(MBi_ChildCallback, &sScanParam));
            return;
        default:
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
    }
    case WM_APIID_END_SCAN:
        if (cb->errcode != 0) {
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        MBi_CheckWmErrcode(WM_APIID_START_CONNECT, WM_StartConnect(MBi_ChildCallback, work->pInfo, NULL));
        return;
    case WM_APIID_START_CONNECT: {
        WMStartConnectCallback *connect = arg;

        if (connect->errcode != 0) {
            work->found_parent_count = 0;
            work->callback(MB_CALLBACK_CONNECT_FAILED, arg);
            return;
        }
        switch (connect->state) {
        case 6:
            work->child_bitmap = 0;
            work->mpStarted = 1;
            return;
        case WM_STATECODE_CONNECTED:
            work->my_aid = connect->aid;
            work->callback(MB_CALLBACK_CONNECTED_TO_PARENT, arg);
            work->child_bitmap = 1;
            if (WM_SetPortCallback(1, MBi_ChildPortCallback, NULL) != 0) {
                return;
            }
            MBi_CheckWmErrcode(WM_APIID_START_MP,
                               WM_StartMPEx(MBi_ChildCallback, (u16 *)work->recvBuf, work->recvBufSize,
                                            (u16 *)work->sendBuf, work->sendBufSize, work->contSend ? 0 : 1, 0, 0, 0, 1,
                                            1));
            return;
        case WM_STATECODE_DISCONNECTED:
            work->callback(MB_CALLBACK_DISCONNECTED_FROM_PARENT, arg);
            work->child_bitmap = 0;
            work->mpStarted = 0;
            return;
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            return;
        default:
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
    }
    case WM_APIID_START_MP: {
        WMStartMPCallback *mp = arg;

        switch (mp->state) {
        case WM_STATECODE_MP_START:
            work->mpStarted = 1;
            if (MBi_IsSendEnabled()) {
                work->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
            }
            return;
        case WM_STATECODE_MP_IND:
        case WM_STATECODE_MPACK_IND:
            return;
        default:
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
    }
    case WM_APIID_SET_MP_DATA:
        work->mpBusy = 0;
        if (cb->errcode == 0) {
            work->callback(MB_CALLBACK_MP_CHILD_SENT, arg);
        } else if (cb->errcode == 9) {
            work->callback(MB_CALLBACK_MP_CHILD_SENT_TIMEOUT, arg);
        } else {
            work->callback(MB_CALLBACK_MP_CHILD_SENT_ERR, arg);
        }
        if (sWmWork->endReq == 0) {
            work->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
        }
        return;
    case WM_APIID_RESET:
        if (cb->errcode != 0) {
            work->endReq = 0;
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        sWmWork->child_bitmap = 0;
        sWmWork->mpStarted = 0;
        MBi_CheckWmErrcode(WM_APIID_END, WM_End(MBi_ChildCallback));
        return;
    case WM_APIID_END:
        if (cb->errcode != 0) {
            work->endReq = 0;
            work->callback(MB_CALLBACK_ERROR, arg);
            return;
        }
        MBi_EndCommon(arg);
        return;
    case WM_APIID_START_KS:
        if (MBi_IsSendEnabled()) {
            work->callback(MB_CALLBACK_MP_SEND_ENABLE, NULL);
        }
        return;
    case WM_APIID_INDICATION:
        switch (((WMIndCallback *)arg)->state) {
        case 0x16:
            OS_Terminate();
            return;
        case 0x17:
            return;
        }
        return;
    default:
        work->callback(MB_CALLBACK_ERROR, arg);
        return;
    }
}

// A number from 0 to 19 that depends on the console's address and the time
static int MBi_GetBeaconPeriodDispersion(void) {
    u8 mac[6];
    u32 sum;
    int i;

    OS_GetMacAddress(mac);
    for (i = 0, sum = 0; i < 6; i++) {
        sum += mac[i];
    }
    return (sum + MB_TIME_SEED) * 7 % 20;
}

int MB_Init(void *work, const MBUserInfo *user, u32 ggid, u32 tgid, u32 dma) {
    MBiParam *wmWork;
    MB_CommCommonWork *commWork;
    OSIntrMode irq;
    u16 *name;
    int i;

    if (mbc != NULL && mbc->isMbInitialized != 0) {
        return MB_ERRCODE_INVALID_STATE;
    }
    wmWork = (MBiParam *)(((uptr)work + 0x1f) & ~0x1f);
    commWork = (MB_CommCommonWork *)(((uptr)wmWork + sizeof(MBiParam) + 0x1f) & ~0x1f);
    if (tgid == MB_TGID_AUTO) {
        tgid = WM_GetNextTgid();
    }
    irq = OS_DisableInterrupts();
    mbi_life_table_no = 0xffff;
    mbi_life_frame = 5;
    mbi_life_cam = 0x28;
    mbi_life_mp = 0x28;
    mbi_power_save_mode = 1;
    sDmaNo = dma;
    sWmWork = wmWork;
    mbc = commWork;
    MI_CpuClear32(wmWork, sizeof(MBiParam));
    MI_CpuClear16(commWork, sizeof(MB_CommCommonWork));
    name = wmWork->uname;
    for (i = 0; i < user->nameLength; i++) {
        *name++ = user->name[i];
    }
    {
        // The cursor is a static, as in ntrtwl's NitroSDK
        static const u16 *game = sParentName;

        // BUG: the static cursor stays at the name's end after the first call, so a second MB_Init copies no name
#ifdef BUGFIX
        game = sParentName;
#endif
        name = wmWork->gameName;
        for (i = 0; i < 16; i++) {
            if (*game == 0) {
                break;
            }
            *name++ = *game++;
        }
    }
    MI_CpuCopy8(user, &commWork->user, sizeof(MBUserInfo));
    if (user->nameLength < MB_USER_NAME_LENGTH) {
        commWork->user.name[user->nameLength] = 0;
    }
    wmWork->p_sendlen = 0x100;
    wmWork->p_recvlen = 8;
    wmWork->sendBufSize = 0;
    wmWork->recvBufSize = 0;
    wmWork->contSend = 1;
    wmWork->recvBuf = (WMMpRecvBuf *)commWork->recvbuf;
    wmWork->parentParam.entryFlag = 0;
    wmWork->parentParam.multiBootFlag = 0;
    wmWork->parentParam.CS_Flag = 1;
    wmWork->parentParam.KS_Flag = 0;
    wmWork->parentParam.ggid = ggid;
    wmWork->parentParam.tgid = tgid;
    wmWork->parentParam.beaconPeriod = MBi_GetBeaconPeriodDispersion() + 200;
    wmWork->parentParam.maxEntry = 15;
    wmWork->mpBusy = 0;
    wmWork->mbIsStarted = 0;
    commWork->isMbInitialized = 1;
    commWork->start_mp_busy = 0;
    OS_RestoreInterrupts(irq);
    return MB_ERRCODE_SUCCESS;
}

// Whether the sizes of the frames fit the buffer that the wireless manager is given
static BOOL MBi_IsCommSizeValid(u32 parentSize, u32 childSize, u32 children) {
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
    OSIntrMode irq = OS_DisableInterrupts();

    if (sWmWork->mbIsStarted != 0) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    if (!MBi_IsCommSizeValid(sendSize, 8, maxChildren)) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    sWmWork->parentParam.maxEntry = maxChildren;
    sWmWork->p_sendlen = sendSize;
    sWmWork->p_recvlen = 8;
    OS_RestoreInterrupts(irq);
    return TRUE;
}

// Starts the wireless manager, or the parent's setup if it is already running
static int MBi_StartCommon(void) {
    int result;

    sWmWork->mpStarted = 0;
    sWmWork->child_bitmap = 0;
    sWmWork->endReq = 0;
    sWmWork->gameName[8] = 0;
    MBi_SetMaxScanTime(30);
    if (mbc->is_started_ex == 0) {
        do {
            result = WM_Initialize(sWmBuf, sWmWork->callback_ptr, sDmaNo);
        } while (result == WM_ERRCODE_WM_DISABLE);
        if (result != WM_ERRCODE_OPERATING) {
            return MB_ERRCODE_WM_FAILURE;
        }
        WM_SetIndCallback(sWmWork->callback_ptr);
        sWmWork->mbIsStarted = 1;
        return MB_ERRCODE_SUCCESS;
    }
    WM_SetIndCallback(sWmWork->callback_ptr);
    sWmWork->mbIsStarted = 1;
    MBi_OnInitializeDone();
    return MB_ERRCODE_SUCCESS;
}

// Sets the library's work up for a parent on a channel and starts the wireless manager
static int MBi_StartParentCore(int channel) {
    MBCommPStateCallback callback;
    OSIntrMode irq = OS_DisableInterrupts();
    int result;
    int i;

    sWmWork->parentParam.channel = channel;
    sWmBuf = (void *)(((uptr)mbc + sizeof(MB_CommPWork) + 0x1f) & ~0x1f);
    callback = pPwork->parent_callback;
    MI_CpuClear16(&pPwork->childUser, sizeof(MB_CommPWork) - sizeof(MB_CommCommonWork));
    MB_CommSetParentStateCallback(callback);
    mbc->block_size_max = sWmWork->p_sendlen - 6;
    MBi_SetChildMPMaxSize(sWmWork->p_recvlen);
    MBi_SetParentPieceBuffer(&pPwork->req_data_buf);
    for (i = 0; i < 15; i++) {
        pPwork->p_comm_state[i] = 0;
        pPwork->fileid_of_child[i] = -1;
    }
    pPwork->file_num = 0;
    MI_CpuClear16(pPwork->fileinfo, sizeof(pPwork->fileinfo));
    MI_CpuFill8(pPwork->req2child, 0, sizeof(pPwork->req2child));
    sWmWork->mode = 1;
    sWmWork->callback = MBi_CommParentCallback;
    sWmWork->callback_ptr = MBi_ParentCallback;
    sWmWork->parentParam.parentMaxSize = sWmWork->p_sendlen;
    sWmWork->sendBufSize = (sWmWork->parentParam.parentMaxSize + 0x23) & ~0x1f;
    sWmWork->parentParam.childMaxSize = sWmWork->p_recvlen;
    sWmWork->recvBufSize = (((sWmWork->parentParam.childMaxSize + 0xe) * 15 + 0x29) & ~0x1f) * 2;
    MB_InitSendGameInfoStatus();
    result = MBi_StartCommon();
    OS_RestoreInterrupts(irq);
    pPwork->useWvrFlag = PXI_IsCallbackReady(15, 1);
    return result;
}

int MB_StartParent(int channel) {
    mbc->is_started_ex = 1;
    return MBi_StartParentCore(channel);
}

static int MBi_CallReset(void) {
    int result = WM_Reset(sWmWork->callback_ptr);

    MBi_CheckWmErrcode(WM_APIID_RESET, result);
    if (result == WM_ERRCODE_OPERATING) {
        result = 0;
    }
    return result;
}

static void MBi_OnReset(MBiTaskInfo *task) {
    MBi_CallReset();
}

// Asks the wireless manager to end, once what it is sending is done
static BOOL MBi_CommEnd(void) {
    BOOL result = TRUE;
    OSIntrMode irq = OS_DisableInterrupts();

    if (sWmWork->mbIsStarted == 0) {
        MBi_EndCommon(NULL);
    } else if (sWmWork->endReq == 0) {
        sWmWork->scanning_flag = 0;
        sWmWork->endReq = 1;
        if (MBi_IsTaskAvailable()) {
            MBi_EndTaskThread(MBi_OnReset);
            result = FALSE;
        } else {
            result = MBi_CallReset();
        }
    }
    OS_RestoreInterrupts(irq);
    return result;
}

void MB_End(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (mbc->is_started_ex == 0) {
        OS_Terminate();
    }
    MBi_CommEnd();
    OS_RestoreInterrupts(irq);
}

void MB_DisconnectChild(u16 aid) {
    WM_Disconnect(MBi_ParentCallback, aid);
    if (aid != 0 && aid < 16) {
        pPwork->childversion[aid - 1] = 0;
        MI_CpuFill8(&pPwork->childggid[aid - 1], 0, sizeof(u32));
        MI_CpuFill8(&pPwork->childUser[aid - 1], 0, sizeof(MBUserInfo));
        MBi_ClearParentPieceBuffer(aid);
        pPwork->req2child[aid - 1] = 0;
        MBComm_LeaveFile(aid);
        if (pPwork->child_entry_bmp & (1 << aid)) {
            pPwork->child_num--;
            pPwork->child_entry_bmp &= ~(1 << aid);
        }
        pPwork->p_comm_state[aid - 1] = 0;
    }
}

static void MBi_SetMaxScanTime(u16 time) {
    sScanParam.maxChannelTime = time;
}

static int MBi_SetMPData(WMCallbackFunc callback, const void *data, u16 size, u16 unused, u16 aidBitmap) {
    int result = WM_SetMPDataToPortEx(callback, NULL, data, size, aidBitmap, 1, 3);

    MBi_CheckWmErrcode(WM_APIID_SET_MP_DATA, result);
    return result;
}

// Sends a frame to the children in a bitmap with the wireless manager. Gives 1 when it cannot be sent now
int MBi_SendMP(const void *data, u32 size, u32 aidBitmap) {
    u16 length = size;
    u16 bitmap = aidBitmap;
    MBiParam *work = sWmWork;
    int result;

    if (work->mpStarted == 0 || work->endReq == 1) {
        return 1;
    }
    switch (work->mode) {
    case 1:
        result = MBi_SetMPData(work->callback_ptr, data, length, work->contSend == 0 ? 1000 : 0, bitmap);
        if (result == WM_ERRCODE_OPERATING) {
            sWmWork->mpBusy = 1;
        }
        if (result == WM_ERRCODE_OPERATING) {
            return 0;
        }
        return result;
    case 2:
        result = MBi_SetMPData(MBi_ChildCallback, data, length, 0, bitmap);
        if (result == WM_ERRCODE_OPERATING) {
            sWmWork->mpBusy = 1;
        }
        if (result == WM_ERRCODE_OPERATING) {
            return 0;
        }
        return result;
    default:
        return 1;
    }
}

u32 MBi_GetGgid(void) {
    return sWmWork->parentParam.ggid;
}

u16 MBi_GetTgid(void) {
    return sWmWork->parentParam.tgid;
}

// The attributes of the parent's beacon, from its flags
u8 MBi_GetAttribute(void) {
    return (u8)((sWmWork->parentParam.entryFlag ? 1 : 0) | (sWmWork->parentParam.multiBootFlag ? 2 : 0) |
                (sWmWork->parentParam.KS_Flag ? 4 : 0) | (sWmWork->parentParam.CS_Flag ? 8 : 0));
}

BOOL MBi_IsStarted(void) {
    if (sWmWork->mbIsStarted == 1) {
        return TRUE;
    }
    return FALSE;
}

// Tells the library's callback when a call to the wireless manager did not start its operation
static void MBi_CheckWmErrcode(u16 apiid, int result) {
    if (result != WM_ERRCODE_OPERATING && result != WM_ERRCODE_SUCCESS) {
        WMCallback cb;

        cb.apiid = apiid;
        cb.errcode = result;
        sWmWork->callback(MB_CALLBACK_API_ERROR, &cb);
    }
}
