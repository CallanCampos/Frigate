// NitroWiFi's WCM, its backend for the DSi's wireless (mode 2), in the LTD autoload's .ltdmain: the steps that
// wcm.c's calls take through TwlSDK's NWM where the DS's take them through WM, and NWM's callbacks, which move WCM's
// state machine and notify WCM's user. It mirrors wcm.c's WM backend step for step. The ROM names none of it: the
// file's name is descriptive, and the functions keep their default names.
#include "nitrowifi/wcm_dsi.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"
#include "nitrowifi/wcm_internal.h"
#include "twl/nwm.h"

// The index of the command 8's connection, from the access point's capability bits
static u8 sConnectIndex;

static void func_02769e60(void);
static void func_0276a0ac(void *arg);
static void func_0276a3d4(void *arg);
static void func_0276a6b4(void *arg);
static void func_0276a8dc(void *arg);
static void func_0276a9e4(void *arg);
static void func_0276ab40(void *arg);
static void func_0276ac94(const u8 *bssid, const u8 *essid, u32 option);
static void func_0276adcc(void *arg);
static void func_0276ae24(void *arg);

// clang-format off
// TwlSDK's section for the DSi-only code that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on
#pragma section LTDMAIN begin

// The startup's first step: sets NWM up in the work and loads the device
s32 func_02769930(void) {
    WCMWork *work = WCM_GetWork();

    switch (NWM_Init(work->nwmSystemBuffer, 0x3000, (u8)work->dmaNo)) {
    case 0:
        break;
    case 3:
        WCM_SetState(11);
        return 7;
    case 4:
        return 5;
    case 5:
    default:
        WCM_SetState(11);
        return 7;
    }
    work->nwmConnectionFailureReason = 0;
    switch (NWM_LoadDevice(func_0276a0ac)) {
    case 2:
        WCM_SetState(2);
        work->pendingNotify = 1;
        break;
    case 6:
        WCM_SetState(12);
        return 1;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

s32 func_02769a30(void) {
    WCMWork *work = WCM_GetWork();

    switch (NWM_Close(func_0276a0ac)) {
    case 2:
        WCM_SetState(4);
        work->pendingNotify = 2;
        break;
    case 6:
        return 4;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

s32 func_02769aa4(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCM_GetWork();

    switch (work->state) {
    case 5:
        func_0276ac94(bssid, essid, option);
        return 2;
    case 6:
        func_0276ac94(bssid, essid, option);
        return 0;
    case 3:
        break;
    default:
        return 1;
    }
    func_0276ac94(bssid, essid, option);
    DC_InvalidateRange(work->nwmScanRequest.buffer, work->nwmScanRequest.bufferSize);
    work->nwmScanCount++;
    switch (NWM_StartScan(func_0276a3d4, &work->nwmScanRequest)) {
    case 2:
        WCM_SetState(5);
        work->pendingNotify = 3;
        break;
    case 6:
        return 4;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

s32 func_02769b80(void) {
    NWMPmkParam pmk;
    WCMWork *work = WCM_GetWork();
    s32 result;

    switch (work->connectionStorage.mode) {
    case 0:
    case 1:
    case 2:
    case 3: {
        u16 authMode = 0; // Open system, or 1 for a shared key

        if ((work->searchOption & WCM_OPTION_AUTH_SHARED) == WCM_OPTION_AUTH_SHARED) {
            authMode = 1;
        }
        result = NWM_Connect(func_0276a0ac, work->connectionStorage.mode, work->connectionStorage.argument,
                             work->connectionStorage.data, authMode);
        break;
    }
    case 4:
    case 5:
    case 6:
    case 7:
        MI_CpuCopy8(work->connectionStorage.data, pmk.pmk, sizeof(pmk.pmk));
        pmk.type = work->connectionStorage.mode;
        result = func_02703960(func_0276a0ac, &pmk);
        break;
    }
    switch (result) {
    case 2:
        WCM_SetState(8);
        work->pendingNotify = 5;
        break;
    case 6:
        return 4;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

s32 func_02769c70(void) {
    WCMWork *work = WCM_GetWork();

    switch (func_02703854(func_0276ab40, 0)) {
    case 2:
        WCM_SetState(10);
        work->pendingNotify = 6;
        break;
    case 6:
        return 4;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

// Brings NWM back down from the state it is in, one command at a time
s32 func_02769ce8(void) {
    WCMWork *work = WCM_GetWork();
    s32 result;

    switch (func_027028b0()) {
    case 1:
        result = NWM_End();
        if (result == 0) {
            WCM_SetState(1);
            work->pendingNotify = 0;
            return 0;
        }
        break;
    case 2:
        result = NWM_UnloadDevice(func_0276a0ac);
        break;
    case 3:
        result = NWM_Close(func_0276a0ac);
        break;
    case 4:
        result = NWM_Disconnect(func_0276a8dc);
        break;
    default:
        work->resetPending = 1;
        result = 4;
        break;
    }
    switch (result) {
    case 2:
        WCM_SetState(13);
        work->pendingNotify = 9;
        break;
    case 6:
        return 4;
    case 3:
    default:
        WCM_SetState(11);
        return 7;
    }
    return 3;
}

s32 func_02769dc4(const u8 *dst, const void *buffer, u16 byteLength, void (*completion)(s32 result)) {
    WCMGlobal *global = WCM_GetGlobal();
    NWMCallback callback;

    if (completion == NULL) {
        callback = func_0276adcc;
    } else {
        global->sendCompletion = completion;
        callback = func_0276ae24;
    }
    switch (NWM_SendFrame(callback, dst, buffer, byteLength)) {
    case 2:
        break;
    case 3:
    case 5:
    case 6:
    default:
        return -5;
    }
    if (completion == NULL) {
        OS_SleepThread(&global->sendQueue);
        switch (global->sendStatus) {
        case 0:
            break;
        case 1:
        default:
            return -5;
        }
    }
    return byteLength;
}

// The channels that the DSi's wireless allows, from bit 0 for channel 1
s32 func_02769e54(void) {
    return (u16)(func_02702980() >> 1);
}

// What wcm.c's reset request and its completion do, at once, since NWM has no reset command: ends the operation of
// the state WCM is in
static void func_02769e60(void) {
    WCMWork *work = WCM_GetWork();

    if (work->resetPending == 0) {
        // The reset is requested and completes here
        work->resetPending = 1;
        work->resetPending = 0;
        work->nwmAssociationId = 0;
        switch (work->state) {
        case 5:
        case 6:
            WCM_SetState(3);
            WCM_NotifyResult(1, NULL, 0, 0x29b);
            break;
        case 7:
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x2a1);
            break;
        case 8: {
            u16 reason = work->nwmConnectionFailureReason;

            work->nwmConnectionFailureReason = 0;
            WCM_SetState(3);
            WCM_NotifyResult(1, &work->connectBss, reason, 0x2ab);
            break;
        }
        case 9:
        case 12:
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 1, 0x2b3);
            break;
        case 10:
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 0, 0x2ba);
            break;
        case 13:
            switch (NWM_Close(func_0276a0ac)) {
            case 2:
                break;
            case 6:
                WCM_SetState(12);
                WCM_NotifyResult(1, NULL, 0, 0x2c8);
                break;
            case 3:
            default:
                WCM_SetState(11);
                WCM_NotifyResult(7, NULL, 0, 0x2d1);
                break;
            }
            break;
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, NULL, work->state, 0x2d7);
            break;
        }
    }
}

// The callback of NWM's device and connection commands, which takes the startup, the cleanup and the connection on
// to their next command
static void func_0276a0ac(void *arg) {
    const NWMEventHeader *event = arg;
    s32 result = 13;
    WCMWork *work = WCM_GetWork();

    switch (event->result) {
    case 0:
        switch (event->cmd) {
        case 3:
            result = NWM_Open(func_0276a0ac);
            break;
        case 4:
            result = NWM_End();
            switch (result) {
            case 0:
                WCM_SetState(1);
                WCM_NotifyResult(0, NULL, 0, 0x300);
                break;
            case 4:
            default:
                WCM_SetState(11);
                WCM_NotifyResult(7, NULL, 0, 0x309);
                break;
            }
            return;
        case 5: {
            u8 mac[6];

            OS_GetMacAddress(mac);
            WCM_SetMacAddress(mac);
        }
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x316);
            return;
        case 6:
            result = NWM_UnloadDevice(func_0276a0ac);
            break;
        case 13:
        case 18:
            result =
                func_027035f4(func_0276a0ac, (work->searchOption & WCM_OPTION_POWER_ACTIVE) == WCM_OPTION_POWER_ACTIVE
                                                 ? NWM_POWER_MODE_ACTIVE
                                                 : NWM_POWER_MODE_SAVE);
            break;
        case 14:
            result = NWM_SetReceivingFrameBuffer(func_0276a9e4, work->ioBuffer.nwm, sizeof(work->ioBuffer.nwm), 0);
            break;
        }
        switch (result) {
        case 2:
            break;
        case 6:
            WCM_SetState(12);
            WCM_NotifyResult(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x34a);
            break;
        case 3:
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x353);
            break;
        }
        break;
    case 1:
        WCM_SetState(12);
        WCM_NotifyResult(1, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x372);
        break;
    case 10:
        WCM_SetState(11);
        WCM_NotifyResult(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x378);
        break;
    case 3:
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, work->pendingNotify == WCM_NOTIFY_CONNECT ? &work->connectBss : NULL, 0, 0x385);
        break;
    }
}

// The scan's callback: reports the access points found and scans the next channel, until the search stops
static void func_0276a3d4(void *arg) {
    const NWMScanEvent *event = arg;
    s32 result = 13;
    WCMWork *work = WCM_GetWork();

    switch (event->header.result) {
    case 0:
        if (work->state == 5) {
            WCM_SetState(6);
            WCM_NotifyResult(0, NULL, 0, 0x3a3);
        }
        switch (work->state) {
        case 6:
            work->pendingNotify = 7;
            if (event->bssDescCount != 0) {
                int i;

                DC_InvalidateRange(work->nwmScanRequest.buffer, work->nwmScanRequest.bufferSize);
                for (i = 0; i < event->bssDescCount; i++) {
                    WCM_OnBeaconReceived((const WMBssDesc *)event->bssDesc[i], event->linkLevel[i]);
                    WCM_Notify(7, 0, (s32)event->bssDesc[i], arg, 0x3b9);
                }
            }
            if ((work->searchOption & 0xc00000) == 0xc00000) {
                u32 count = MATH_CountPopulation(work->searchOption & 0x3ffe);

                if (count > 0) {
                    if (work->nwmScanCount % count == 0) {
                        WCM_Notify(8, 0, work->nwmScanCount, NULL, 0x3c7);
                    }
                }
            }
            work->nwmScanRequest.channelList =
                (u16)((1 << WCM_PickChannel((u16)(32 - MATH_CountLeadingZerosFunc(event->channel)))) >> 1);
            DC_StoreRange(work->nwmScanRequest.buffer, work->nwmScanRequest.bufferSize);
            work->nwmScanCount++;
            result = NWM_StartScan(func_0276a3d4, &work->nwmScanRequest);
            break;
        case 7:
            result = 2;
            WCM_SetState(3);
            WCM_NotifyResult(0, NULL, 0, 0x3d7);
            break;
        case 13:
            func_02769e60();
            return;
        }
        switch (result) {
        case 2:
            break;
        case 6:
            WCM_SetState(12);
            WCM_NotifyResult(1, NULL, 0, 0x3f3);
            break;
        case 3:
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, NULL, 0, 0x3fc);
            break;
        }
        break;
    case 1:
        func_02769e60();
        break;
    case 3:
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, NULL, 0, 0x40d);
        break;
    }
}

// The callback of the command 8: the connection made, or why it failed
static void func_0276a6b4(void *arg) {
    const NWMConnectEvent *event = arg;
    WCMWork *work = WCM_GetWork();
    u16 nwmState = func_027028b0();

    switch (event->header.result) {
    case 0:
        if (nwmState == 4) {
            if (work->state == 12) {
                WCM_SetState(8);
                func_02769e60();
                return;
            }
            work->nwmAssociationId = event->aid;
            WCM_SetState(9);
            WCM_NotifyResult(0, &work->connectBss, 0, 0x436);
        } else {
            WCM_SetState(11);
            WCM_NotifyResult(7, &work->connectBss, 0, 0x43c);
        }
        break;
    case 1:
        switch (event->status) {
        case 1:
        case 2:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            switch (work->state) {
            case 9:
            case 10:
                work->pendingNotify = 6;
            case 8:
                work->nwmAssociationId = 0;
                switch (event->status) {
                case 5:
                    work->nwmConnectionFailureReason = 13;
                    break;
                case 10:
                case 11:
                    work->nwmConnectionFailureReason = 15;
                    break;
                case 7:
                    work->nwmConnectionFailureReason = 17;
                    break;
                }
            case 12:
                func_02769e60();
                break;
            }
            break;
        case 3:
            if (work->state == 8) {
                work->nwmAssociationId = 0;
                func_02769e60();
            }
            break;
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, &work->connectBss, event->status, 0x481);
            break;
        }
        break;
    case 5:
        WCM_SetState(8);
        func_02769e60();
        break;
    case 3:
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x495);
        break;
    }
}

// The disconnection's callback
static void func_0276a8dc(void *arg) {
    const NWMEventHeader *event = arg;
    WCMWork *work = WCM_GetWork();

    switch (event->result) {
    case 0:
        if (work->state == 12) {
            WCM_SetState(10);
            func_02769e60();
        } else if (work->state == 13) {
            func_02769e60();
        } else {
            work->nwmAssociationId = 0;
            WCM_SetState(3);
            WCM_NotifyResult(0, &work->connectBss, 0, 0x4bd);
        }
        break;
    case 1:
    case 3:
        WCM_SetState(10);
        func_02769e60();
        break;
    case 4:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x4cf);
        break;
    }
}

// The callback of the receive buffer: once it is set, connects to the access point; then hands WCM each frame received
static void func_0276a9e4(void *arg) {
    const NWMReceiveEvent *event = arg;
    WCMWork *work = WCM_GetWork();

    switch (event->header.result) {
    case 0:
        if (work->state == 12) {
            WCM_SetState(8);
            func_02769e60();
            return;
        }
        if (work->connectBss.capaInfo & 2) {
            sConnectIndex = 1;
        } else {
            sConnectIndex = 0;
        }
        switch (func_027032e0(func_0276a6b4, sConnectIndex, (const NWMBlockParam *)&work->connectBss)) {
        case 2:
            break;
        case 6:
            WCM_SetState(12);
            WCM_NotifyResult(1, &work->connectBss, 0, 0x506);
            break;
        case 3:
        default:
            WCM_SetState(11);
            WCM_NotifyResult(7, &work->connectBss, 0, 0x50f);
            break;
        }
        break;
    case 11: {
        s32 rssi;

        DC_InvalidateRange(event->frame, 0x620);
        rssi = event->rssi;
        if (rssi < 0) {
            rssi = 0;
        }
        if (rssi > 0x100) {
            rssi = 0xff;
        }
        WCM_AddRssiSample((u8)rssi);
        WCM_DispatchRecvCallback(event->frame + 6, event->frame, event->frame + 14, event->length - 14);
        break;
    }
    }
}

// The callback of the command 0xc, which readies the disconnection: disconnects
static void func_0276ab40(void *arg) {
    const NWMEventHeader *event = arg;
    WCMWork *work = WCM_GetWork();

    switch (event->result) {
    case 0:
        if (work->state == 12) {
            WCM_SetState(10);
            func_02769e60();
        } else {
            switch (NWM_Disconnect(func_0276a8dc)) {
            case 2:
                WCM_SetState(10);
                work->pendingNotify = 6;
                break;
            case 6:
                WCM_SetState(12);
                WCM_NotifyResult(1, &work->connectBss, 0, 0x555);
                break;
            case 3:
                WCM_SetState(10);
                func_02769e60();
                break;
            default:
                WCM_SetState(11);
                WCM_NotifyResult(7, &work->connectBss, 0, 0x562);
                break;
            }
        }
        break;
    case 1:
    case 3:
    default:
        WCM_SetState(11);
        WCM_NotifyResult(7, &work->connectBss, 0, 0x56e);
        break;
    }
}

// Fills in the scan request of a search
static void func_0276ac94(const u8 *bssid, const u8 *essid, u32 option) {
    WCMWork *work = WCM_GetWork();
    u16 channelTime;

    WCM_SetSearchOption(option);
    work->nwmScanRequest.buffer = work->ioBuffer.nwm;
    work->nwmScanRequest.bufferSize = sizeof(work->ioBuffer.nwm);
    work->nwmScanRequest.channelList = (u16)((1 << WCM_PickChannel(0)) >> 1);
    work->nwmScanRequest.scanType = (work->searchOption & 0x300000) == 0x300000;
    channelTime = work->scanChannelTime;
    if (channelTime == 0) {
        channelTime = func_02702998(work->nwmScanRequest.scanType);
    }
    work->nwmScanRequest.dwellTime = channelTime;
    if (bssid == NULL) {
        MI_CpuCopy8(WCMiAnyBssid, work->nwmScanRequest.bssid, 6);
    } else {
        MI_CpuCopy8(bssid, work->nwmScanRequest.bssid, 6);
    }
    if (essid == NULL || essid == WCMiAnyEssid) {
        MI_CpuCopy8(WCMiAnyEssid, work->nwmScanRequest.ssid, 0x20);
        work->nwmScanRequest.ssidLength = 0;
        work->nwmScanRequest.ssidMatchLength = 0;
    } else {
        s32 i;

        MI_CpuCopy8(essid, work->nwmScanRequest.ssid, 0x20);
        for (i = 0; i < 0x20; i++) {
            if (essid[i] == 0) {
                break;
            }
        }
        work->nwmScanRequest.ssidLength = (u16)i;
        work->nwmScanRequest.ssidMatchLength = (u16)i;
    }
    work->nwmScanCount = 0;
}

// The callback of a frame sent while the sender waits for it
static void func_0276adcc(void *arg) {
    WCMGlobal *global = WCM_GetGlobal();
    const NWMEventHeader *event = arg;

    if (event->cmd != 11) {
        return;
    }
    global->sendStatus = event->result;
    if (event->result == 0) {
        WCM_CancelTimeout();
    }
    OS_WakeupThread(&global->sendQueue);
}

// The callback of a frame sent with a completion function, which it runs
static void func_0276ae24(void *arg) {
    WCMGlobal *global = WCM_GetGlobal();
    const NWMEventHeader *event = arg;
    s32 status;
    void (*completion)(s32);

    if (event->cmd != 11) {
        return;
    }
    global->sendStatus = event->result;
    if (event->result == 0) {
        WCM_CancelTimeout();
    }
    switch (global->sendStatus) {
    case 0:
        status = 0;
        break;
    case 1:
    default:
        status = -5;
        break;
    }
    completion = global->sendCompletion;
    global->sendCompletion = NULL;
    WCM_ReleaseIrqOwnedMutex(&global->mutex);
    completion(status);
}

#pragma section LTDMAIN end
