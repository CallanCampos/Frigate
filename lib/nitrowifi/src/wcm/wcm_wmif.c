#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm.h"
#include "wcm_internal.h"

// NitroWiFi's WCM (overlay 11): its backend for the DS's wireless, which takes the steps of wcm_system.c's operations
// through NitroSDK's WM, and WM's callbacks, which move WCM's state machine and notify WCM's user. TwlSDK's libwcm
// names the object wmif.o; Black 1's decompilation (aletheia) has the same functions, in this order, in its
// wcm_wmif.c, and the names are the SDK's.

static void WcmWmReset(void);
static void WcmWmcbIndication(void *ind);
static void WcmWmcbCommon(void *ind);
static void WcmWmcbScanEx(void *ind);
static void WcmWmcbEndScan(void *ind);
static void WcmWmcbConnect(void *ind);
static void WcmWmcbDisconnect(void *ind);
static void WcmWmcbStartDCF(void *ind);
static void WcmWmcbEndDCF(void *ind);
static void WcmWmcbReset(void *ind);
static void WcmEditScanExParam(const u8 *bssid, const u8 *essid, u32 option);
static void WcmWmcbSetDCFData(void *ind);
static void WcmWmcbSetDCFDataAsync(void *ind);

// The startup FSM's WM_Initialize-adjacent first step: brings up the hardware and reads its MAC address
s32 WCMi_WmifStartupAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (WM_Init(work->startupBuffer, (u16)work->dmaNo)) {
    case 0:
        break;
    case 3:
        WCMi_SetPhase(11);
        return 7;
    case 4:
        return 5;
    // WM_ERRCODE_INVALID_PARAM, which the SDK lists with the default
    case 6:
    default:
        WCMi_SetPhase(11);
        return 7;
    }
    if (WM_SetIndCallback((WMCallbackFunc)WcmWmcbIndication) != 0) {
        WCMi_SetPhase(11);
        return 7;
    }
    {
        u8 mac[6];

        OS_GetMacAddress(mac);
        WCMi_CpsifSetMacAddress(mac);
    }
    switch (WM_Enable((WMCallbackFunc)WcmWmcbCommon)) {
    case 2:
        WCMi_SetPhase(2);
        work->pendingNotify = 1;
        break;
    case 8:
        WCMi_SetPhase(12);
        return 1;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(11);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifCleanupAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (WM_PowerOff((WMCallbackFunc)WcmWmcbCommon)) {
    case 2:
        WCMi_SetPhase(4);
        work->pendingNotify = 2;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(0xb);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifBeginSearchAsync(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (work->state) {
    case 5:
        WcmEditScanExParam(bssid, essid, option);
        return 2;
    case 6:
        WcmEditScanExParam(bssid, essid, option);
        return 0;
    case 3:
        break;
    default:
        return 1;
    }
    WcmEditScanExParam(bssid, essid, option);
    DC_InvalidateRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
    work->scanCount++;
    switch (WM_StartScanEx((WMCallbackFunc)WcmWmcbScanEx, &work->scanRequest)) {
    case 2:
        WCMi_SetPhase(5);
        work->pendingNotify = 3;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(0xb);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifConnectAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    if (work->state == 3) {
        work->connectBss.rateSetSupport = work->connectBss.rateSetBasic | 3;
    }
    switch (WM_SetLifeTime((WMCallbackFunc)WcmWmcbCommon, 0xffff, 0x50, 0xffff, 0xffff)) {
    case 2:
        WCMi_SetPhase(8);
        work->pendingNotify = 5;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(0xb);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifDisconnectAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();

    switch (WM_EndDCF((WMCallbackFunc)WcmWmcbEndDCF)) {
    case 2:
        WCMi_SetPhase(0xa);
        work->pendingNotify = 6;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(0xb);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifTerminateAsync(void) {
    WCMWork *work = WCMi_GetSystemWork();
    const WMStatus *status = WMi_GetStatusAddress();
    s32 result;

    DC_InvalidateRange((void *)status, 2);
    switch (status->state) {
    case 0:
        result = WM_Finish();
        if (result == 0) {
            WCMi_SetPhase(1);
            work->pendingNotify = 0;
            return 0;
        }
        break;
    case 1:
        result = WM_Disable((WMCallbackFunc)WcmWmcbCommon);
        break;
    case 2:
        result = WM_PowerOff((WMCallbackFunc)WcmWmcbCommon);
        break;
    default:
        work->resetPending = 1;
        result = WM_Reset((WMCallbackFunc)WcmWmcbReset);
        break;
    }
    switch (result) {
    case 2:
        WCMi_SetPhase(0xd);
        work->pendingNotify = 9;
        break;
    case 8:
        return 4;
    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
    case 3:
    default:
        WCMi_SetPhase(0xb);
        return 7;
    }
    return 3;
}

s32 WCMi_WmifSetDCFData(const u8 *dst, const void *buffer, u16 byteLen, void (*completion)(s32)) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    WMCallbackFunc callback;

    if (completion == NULL) {
        callback = WcmWmcbSetDCFData;
    } else {
        global->sendCompletion = completion;
        callback = WcmWmcbSetDCFDataAsync;
    }
    switch (WM_SetDCFData(callback, dst, buffer, byteLen)) {
    case 2:
        break;
    // WM_ERRCODE_ILLEGAL_STATE, WM_ERRCODE_INVALID_PARAM and WM_ERRCODE_FIFO_ERROR, which the SDK lists with the
    // default
    case 3:
    case 6:
    case 8:
    default:
        return -5;
    }
    if (completion == NULL) {
        OS_SleepThread(&global->sendQueue);
        switch (global->sendStatus) {
        case 0:
            break;
        // WM_ERRCODE_FAILED, which the SDK lists with the default
        case 1:
        default:
            return -5;
        }
    }
    return byteLen;
}

u16 WCMi_WmifGetAllowedChannel(void) {
    u16 allowed = WM_GetAllowedChannel();
    u16 lowerMask = 1;
    u16 upperMask = 0x8000;

    if (allowed == 0) {
        return 0;
    }
    // Keep the inclusive span, including any gaps in the hardware's allowed-channel mask.
    while (!(upperMask & allowed)) {
        upperMask = (upperMask >> 1) | 0x8000;
    }
    while (!(lowerMask & allowed)) {
        lowerMask = (lowerMask << 1) | 1;
    }
    upperMask <<= 1;
    lowerMask >>= 1;
    return (u16) ~(upperMask | lowerMask);
}

static void WcmWmReset(void) {
    WCMWork *work = WCMi_GetSystemWork();

    if (work->resetPending == 0) {
        work->resetPending = 1;
        if (WM_Reset((WMCallbackFunc)WcmWmcbReset) != 2) {
            WCMi_SetPhase(0xb);
            WCMi_Notify(7, NULL, 0, 0x24f);
        }
    }
}

// WM's indication callback, installed in WCMi_WmifStartupAsync: dispatches disconnect indications by the current state
static void WcmWmcbIndication(void *ind) {
    const WMIndCallback *indication = ind;
    WCMWork *work = WCMi_GetSystemWork();

    if (indication->errcode == 8) {
        if (indication->state == 0x16 && indication->reason == 0x25) {
            switch (work->state) {
            case 8:
                WCMi_SetPhase(0xc);
                break;
            case 9:
            case 12:
                WcmWmReset();
                break;
            case 10:
                WCMi_SetPhase(0xc);
                break;
            }
        }
    }
}

static void WcmWmcbCommon(void *ind) {
    const WMCallback *callback = (const WMCallback *)ind;
    int result = 20;
    WCMWork *work = WCMi_GetSystemWork();

    switch (callback->errcode) {
    case 0:
        switch (callback->apiid) {
        case 3:
            result = WM_PowerOn((WMCallbackFunc)WcmWmcbCommon);
            break;
        case 4:
            result = WM_Finish();
            switch (result) {
            case 0:
                WCMi_SetPhase(1);
                WCMi_Notify(0, NULL, 0, 0x2a4);
                break;
            // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
            case 4:
            default:
                WCMi_SetPhase(11);
                WCMi_Notify(7, NULL, 0, 0x2ad);
                break;
            }
            return;
        case 5:
            WCMi_SetPhase(3);
            WCMi_Notify(0, NULL, 0, 0x2b4);
            return;
        case 6:
            result = WM_Disable((WMCallbackFunc)WcmWmcbCommon);
            break;
        case 29:
            result = WM_SetBeaconIndication((WMCallbackFunc)WcmWmcbCommon, 0);
            break;
        case 25:
            result = WM_SetWEPKeyEx((WMCallbackFunc)WcmWmcbCommon, work->connectionStorage.mode,
                                    work->connectionStorage.argument, work->connectionStorage.data);
            break;
        case 39:
            result = WM_StartConnectEx(
                (WMCallbackFunc)WcmWmcbConnect, &work->connectBss, NULL,
                (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE ? FALSE : TRUE,
                (work->searchOption & WCM_OPTION_AUTH_SHAREDKEY) == WCM_OPTION_AUTH_SHAREDKEY ? (u16)1 : (u16)0);
            break;
        }
        switch (result) {
        case 2:
            break;
        case 8:
            WCMi_SetPhase(12);
            WCMi_Notify(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x2f0);
            break;
        // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
        case 3:
        default:
            WCMi_SetPhase(11);
            WCMi_Notify(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x2f9);
            break;
        }
        break;
    case 1:
        WCMi_SetPhase(12);
        WCMi_Notify(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x31f);
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCMi_SetPhase(11);
        WCMi_Notify(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x329);
        break;
    }
}

static void WcmWmcbScanEx(void *ind) {
    const WMStartScanExCallback *callback = ind;
    int result = 0x14;
    WCMWork *work = WCMi_GetSystemWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 5) {
            WCMi_SetPhase(6);
            WCMi_Notify(0, NULL, 0, 0x346);
        }
        switch (work->state) {
        case 6:
            work->pendingNotify = 7;
            if (callback->state == WM_STATECODE_PARENT_FOUND) {
                int i;

                DC_InvalidateRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
                for (i = 0; i < callback->bssDescCount; i++) {
                    WCMi_EntryApList(callback->bssDesc[i], callback->linkLevel[i]);
                    WCMi_NotifyEx(7, 0, (s32)callback->bssDesc[i], ind, 0x35c);
                }
            }
            if ((work->searchOption & 0xc00000) == 0xc00000) {
                u32 count = MATH_CountPopulation(work->searchOption & 0x3ffe);

                if (count > 0) {
                    if (work->scanCount % count == 0) {
                        WCMi_NotifyEx(8, 0, work->scanCount, NULL, 0x36a);
                    }
                }
            }
            work->scanRequest.channelList =
                (u16)((1 << WCMi_GetNextScanChannel((u16)(32 - MATH_CountLeadingZerosFunc(callback->channelList)))) >>
                      1);
            DC_StoreRange(work->scanRequest.scanBuf, work->scanRequest.scanBufSize);
            work->scanCount++;
            result = WM_StartScanEx((WMCallbackFunc)WcmWmcbScanEx, &work->scanRequest);
            break;
        case 7:
            result = WM_EndScan((WMCallbackFunc)WcmWmcbEndScan);
            break;
        case 0xd:
            WcmWmReset();
            return;
        }
        switch (result) {
        case 2:
            break;
        case 8:
            WCMi_SetPhase(0xc);
            WCMi_Notify(1, NULL, 0, 0x395);
            break;
        // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
        case 3:
        default:
            WCMi_SetPhase(0xb);
            WCMi_Notify(7, NULL, 0, 0x39e);
            break;
        }
        break;
    case 1:
        WcmWmReset();
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, NULL, 0, 0x3af);
        break;
    }
}

static void WcmWmcbEndScan(void *ind) {
    switch (((const WMCallback *)ind)->errcode) {
    case 0:
        WCMi_SetPhase(3);
        WCMi_Notify(0, NULL, 0, 0x3c5);
        return;
    case 1:
        WcmWmReset();
        return;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCMi_SetPhase(11);
        WCMi_Notify(7, NULL, 0, 0x3d5);
        return;
    }
}

static void WcmWmcbConnect(void *ind) {
    const WMStartConnectCallback *callback = ind;
    WCMWork *work = WCMi_GetSystemWork();
    int result;

    switch (callback->errcode) {
    case 0:
        switch (callback->state) {
        case 8:
        case 9:
            switch (work->state) {
            case 10:
                work->associationId = 0;
            case 8:
                WCMi_SetPhase(0xc);
                break;
            case 9:
                work->associationId = 0;
                work->pendingNotify = 6;
            case 12:
                WcmWmReset();
                break;
            default:
                break;
            }
            break;
        case 6:
        case 0x1a:
            break;
        case 7:
            if (work->state == 0xc) {
                WCMi_SetPhase(8);
                WcmWmReset();
            } else if (callback->aid >= 1 && callback->aid <= 0x7d7) {
                work->associationId = callback->aid;
                result = WM_StartDCF((WMCallbackFunc)WcmWmcbStartDCF, (WMDcfRecvBuf *)work->ioBuffer.receive,
                                     sizeof(work->ioBuffer.receive));
                switch (result) {
                case 2:
                    break;
                case 8:
                    WCMi_SetPhase(0xc);
                    WCMi_Notify(1, &work->connectBss, 0, 0x430);
                    break;
                // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
                case 3:
                default:
                    WCMi_SetPhase(0xb);
                    WCMi_Notify(7, &work->connectBss, 0, 0x439);
                    break;
                }
            } else {
                WcmWmReset();
            }
            break;
        default:
            WCMi_SetPhase(0xb);
            WCMi_Notify(7, &work->connectBss, callback->state, 0x447);
            break;
        }
        break;
    case 1:
        work->connectionFailureReason = callback->wlStatus;
    case 6:
    case 11:
    case 12:
        WCMi_SetPhase(8);
        WcmWmReset();
        break;
    // WM_ERRCODE_ILLEGAL_STATE and WM_ERRCODE_WM_DISABLE, whose warnings the SDK leaves out of this build
    case 3:
    case 4:
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, &work->connectBss, 0, 0x45e);
        break;
    }
}

static void WcmWmcbDisconnect(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCMi_GetSystemWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 0xc) {
            WCMi_SetPhase(0xa);
            WcmWmReset();
        } else {
            work->associationId = 0;
            WCMi_SetPhase(3);
            WCMi_Notify(0, &work->connectBss, 0, 0x481);
        }
        break;
    case 1:
    case 3:
        WCMi_SetPhase(0xa);
        WcmWmReset();
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, &work->connectBss, 0, 0x493);
        break;
    }
}

static void WcmWmcbStartDCF(void *ind) {
    const WMStartDCFCallback *callback = ind;
    WCMWork *work = WCMi_GetSystemWork();

    switch (callback->errcode) {
    case 0:
        switch (callback->state) {
        case 0xe:
            if (work->state == 0xc) {
                WCMi_SetPhase(8);
                WcmWmReset();
            } else {
                WCMi_SetPhase(9);
                WCMi_Notify(0, &work->connectBss, 0, 0x4b7);
            }
            break;
        case 0xf:
            WCMi_ShelterRssi((u8)(callback->recvBuf->rate_rssi >> 8));
            DC_InvalidateRange(callback->recvBuf, 0x620);
            WCMi_CpsifRecvCallback(callback->recvBuf->srcAdrs, callback->recvBuf->destAdrs,
                                   (u8 *)callback->recvBuf->data, callback->recvBuf->length);
            break;
        default:
            WCMi_SetPhase(0xb);
            WCMi_Notify(7, &work->connectBss, callback->state, 0x4c6);
            break;
        }
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, &work->connectBss, 0, 0x4d1);
        break;
    }
}

static void WcmWmcbEndDCF(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCMi_GetSystemWork();

    switch (callback->errcode) {
    case 0:
        if (work->state == 0xc) {
            WCMi_SetPhase(0xa);
            WcmWmReset();
        } else {
            switch (WM_Disconnect((WMCallbackFunc)WcmWmcbDisconnect, 0)) {
            case 2:
                break;
            case 8:
                WCMi_SetPhase(0xc);
                WCMi_Notify(1, &work->connectBss, 0, 0x4fa);
                break;
            case 3:
                WCMi_SetPhase(0xa);
                WcmWmReset();
                break;
            default:
                WCMi_SetPhase(0xb);
                WCMi_Notify(7, &work->connectBss, 0, 0x506);
                break;
            }
        }
        break;
    case 1:
    case 3:
        WCMi_SetPhase(0xa);
        WcmWmReset();
        break;
    // WM_ERRCODE_WM_DISABLE, whose warning the SDK leaves out of this build
    case 4:
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, &work->connectBss, 0, 0x519);
        break;
    }
}

static void WcmWmcbReset(void *ind) {
    const WMCallback *callback = ind;
    WCMWork *work = WCMi_GetSystemWork();
    int result;

    switch (callback->errcode) {
    case 0:
        work->resetPending = 0;
        work->associationId = 0;
        switch (work->state) {
        case 5:
        case 6:
            WCMi_SetPhase(3);
            WCMi_Notify(1, NULL, 0, 0x53c);
            break;
        case 7:
            WCMi_SetPhase(3);
            WCMi_Notify(0, NULL, 0, 0x542);
            break;
        case 8: {
            u16 reason = work->connectionFailureReason;

            work->connectionFailureReason = 0;
            if (reason == 0x12) {
                if ((work->connectBss.rateSetSupport & 0x24) != 0x24) {
                    work->connectBss.rateSetSupport |= 0x24;
                    result = WM_StartConnectEx(
                        (WMCallbackFunc)WcmWmcbConnect, &work->connectBss, NULL,
                        (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE ? FALSE : TRUE,
                        (work->searchOption & WCM_OPTION_AUTH_SHAREDKEY) == WCM_OPTION_AUTH_SHAREDKEY ? (u16)1
                                                                                                      : (u16)0);
                    switch (result) {
                    case 2:
                        break;
                    case 8:
                        WCMi_SetPhase(0xc);
                        WCMi_Notify(1, &work->connectBss, reason, 0x563);
                        break;
                    // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
                    case 3:
                    default:
                        WCMi_SetPhase(0xb);
                        WCMi_Notify(7, &work->connectBss, reason, 0x56c);
                        break;
                    }
                    return;
                }
            }
            WCMi_SetPhase(3);
            WCMi_Notify(1, &work->connectBss, reason, 0x574);
            break;
        }
        case 9:
        case 12:
            WCMi_SetPhase(3);
            WCMi_Notify(0, &work->connectBss, 1, 0x57c);
            break;
        case 10:
            WCMi_SetPhase(3);
            WCMi_Notify(0, &work->connectBss, 0, 0x583);
            break;
        case 13:
            result = WM_PowerOff((WMCallbackFunc)WcmWmcbCommon);
            switch (result) {
            case 2:
                break;
            case 8:
                WCMi_SetPhase(0xc);
                WCMi_Notify(1, NULL, 0, 0x591);
                break;
            // WM_ERRCODE_ILLEGAL_STATE, whose warning the SDK leaves out of this build
            case 3:
            default:
                WCMi_SetPhase(0xb);
                WCMi_Notify(7, NULL, 0, 0x59a);
                break;
            }
            break;
        default:
            WCMi_SetPhase(0xb);
            WCMi_Notify(7, NULL, work->state, 0x5a0);
            break;
        }
        break;
    default:
        WCMi_SetPhase(0xb);
        WCMi_Notify(7, NULL, 0, 0x5a7);
        break;
    }
}

static void WcmEditScanExParam(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCMi_GetSystemWork();
    u16 channelTime;

    WCM_UpdateOption(option);
    work->scanRequest.scanBuf = (WMBssDesc *)work->ioBuffer.scan;
    work->scanRequest.scanBufSize = 0x400;
    work->scanRequest.channelList = (u16)((1 << WCMi_GetNextScanChannel(0)) >> 1);
    channelTime = work->scanChannelTime;
    if (channelTime == 0) {
        channelTime = WM_GetDispersionScanPeriod();
    }
    work->scanRequest.maxChannelTime = channelTime;
    work->scanRequest.scanType = ((work->searchOption & 0x300000) != 0x300000);
    if (bssid == NULL) {
        MI_CpuCopy8(WCM_Bssid_Any, work->scanRequest.bssid, 6);
    } else {
        MI_CpuCopy8(bssid, work->scanRequest.bssid, 6);
    }
    if (essid == NULL || essid == WCM_Essid_Any) {
        MI_CpuCopy8(WCM_Essid_Any, work->scanRequest.ssid, 0x20);
        work->scanRequest.ssidLength = 0;
    } else {
        s32 i;

        MI_CpuCopy8(essid, work->scanRequest.ssid, 0x20);
        for (i = 0; i < 0x20; i++) {
            if (essid[i] == 0) {
                break;
            }
        }
        work->scanRequest.ssidLength = (u16)i;
    }
    work->scanCount = 0;
}

static void WcmWmcbSetDCFData(void *ind) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    const WMCallback *result = ind;

    if (result->apiid != 0x12) {
        return;
    }
    global->sendStatus = result->errcode;
    if (result->errcode == 0) {
        WCMi_ResetKeepAliveAlarm();
    }
    OS_WakeupThread(&global->sendQueue);
}

static void WcmWmcbSetDCFDataAsync(void *ind) {
    WCMCpsifWork *global = WCMi_GetCpsifWork();
    const WMCallback *result = ind;
    s32 status;
    void (*completion)(s32);

    if (result->apiid != 0x12) {
        return;
    }
    global->sendStatus = result->errcode;
    if (result->errcode == 0) {
        WCMi_ResetKeepAliveAlarm();
    }
    switch (global->sendStatus) {
    case 0:
        status = 0;
        break;
    // WM_ERRCODE_FAILED, which the SDK lists with the default
    case 1:
    default:
        status = -5;
        break;
    }
    completion = global->sendCompletion;
    global->sendCompletion = NULL;
    WCMi_CpsifUnlockMutexInIRQ(&global->mutex);
    completion(status);
}
