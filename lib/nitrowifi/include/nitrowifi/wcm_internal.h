#ifndef POKEBW2_NITROWIFI_WCM_INTERNAL_H
#define POKEBW2_NITROWIFI_WCM_INTERNAL_H

#include "nitrowifi/wcm.h"
#include "twl/nwm.h"

// Descriptive private interface recovered from WCM and its LTD callers. The original header identity and
// source-file boundaries are unconfirmed; these declarations do not establish additional SDK API names.
typedef struct WCMWork WCMWork;
typedef struct WCMCpsifWork WCMCpsifWork;

// WCM's work, which WCM_Init's caller gives it: WCM_WORK_SIZE bytes for the DS's wireless (mode 1), 0x4800 for the
// DSi's (mode 2), whose backend in the LTD autoload (wcm_dsi.c) drives NWM where the DS's drives WM. The part from
// 0x13a0 is each backend's own. Most of the connect and search scratch area remains unmapped.
struct WCMWork {
    u8 unk_0000[0x600];
    union {
        u8 scan[0x400];
        u8 receive[0x620];
        u8 nwm[0xc40]; // The DSi's scan results and received frames
    } ioBuffer;        // 0x600, the scan and DCF receive operations reuse this storage
    // Descriptive private storage recovered from WCM's copies and WM command arguments.
    struct {
        u8 data[0x50];
        u8 mode;     // Secondary descriptor mode.
        u8 argument; // Legacy key index or variable-copy byte count; mode zero retains the previous byte.
        u8 padding[2];
    } connectionStorage;  // 0x1240..0x1293; no original SDK type identity established
    OSAlarm alarm;        // 0x1294
    s32 dmaNo;            // 0x12c0, from WCMConfig.dmano
    void *apBuffer;       // 0x12c4, from WCMConfig.pbdbuffer: a WCMApListHeader followed by its entries
    u32 apBufferBytes;    // 0x12c8, from WCMConfig.nbdbuffer
    u32 scanMode;         // 0x12cc, from WCMConfig.nbdmode
    WCMNotify notify;     // 0x12d0
    u32 searchOption;     // 0x12d4
    u16 scanChannelTime;  // 0x12d8, zero selects the WM default
    s16 pendingNotify;    // 0x12da, notification ID consumed by WCM_NotifyResult
    u8 apListLocked;      // 0x12dc
    u8 resetPending;      // 0x12dd, set before reset submission; successful reset completion clears it
    u8 state;             // 0x12de
    u8 wirelessMode;      // 0x12df, selected by WCM_InitCore: 1 for DS, 2 for DSi
    WMBssDesc connectBss; // 0x12e0, the access point's description, copied from WCM_ConnectAsync's argument
    union {
        // The DS's
        struct {
            u8 startupBuffer[0xf00];   // 0x13a0, passed to WM initialization
            WMScanExParam scanRequest; // 0x22a0, including the 16-byte SSID match mask
            u32 scanCount;             // 0x22e4
            u16 associationId; // 0x22e8, from the successful connection callback; cleared on disconnect or reset
            u16 connectionFailureReason; // 0x22ea, retained across reset for retry and notification
            u8 unk_22ec[WCM_WORK_SIZE - 0x22ec];
        };
        // The DSi's, with the same fields for NWM
        struct {
            u8 nwmSystemBuffer[0x3000];    // 0x13a0, passed to NWM_Init
            NWMScanExParam nwmScanRequest; // 0x43a0
            u8 unk_43d8[8];
            u32 nwmScanCount;               // 0x43e0
            u16 nwmAssociationId;           // 0x43e4
            u16 nwmConnectionFailureReason; // 0x43e6
        };
    };
};

// sWcmGlobal, right after WCM's anchor: the globals WCMi_InitCpsif sets up once
struct WCMCpsifWork {
    u8 initialized; // 0x00, set once WCMi_InitCpsif has initialized the send state
    u8 unk_01[3];
    OSThreadQueue sendQueue; // 0x04
    OSMutex mutex;           // 0x0c
    u16 sendStatus;          // 0x24
    u8 padding_26[2];
    WCMRecvInd recvCallback;     // 0x28
    void (*sendCompletion)(s32); // 0x2c
    u8 ownMac[6];                // 0x30
    u8 unk_36[2];
};

// Borrowed internal storage. WCM_Finish invalidates the work pointer; the globals have static lifetime.
// Callers must obey WCM's initialization and IRQ synchronization requirements.
WCMWork *WCMi_GetSystemWork(void);
WCMCpsifWork *WCM_GetGlobal(void);
void WcmSetPhase(s32 newState);
u16 WcmGetNextScanChannel(s32 channelIndex);

// Notification callbacks run synchronously with temporary notice storage and must not retain its pointer.
// Mixed pointer/integer payloads use ARM32 words; a host port must widen this representation.
void WcmNotifyEx(s16 notify, s16 result, s32 parameter, void *param1, s32 param2);
void WCM_NotifyResult(s16 result, void *parameter, u32 detail, u32 line);

// Adds a mode-dependent encoded signal sample; its physical units are unconfirmed.
void WCMi_ShelterRssi(s32 rssi);
// Cancels the timeout alarm and rearms it while connected in state 9.
void WCMi_ResetKeepAliveAlarm(void);
// Copies six MAC-address bytes into WCM's globals; the source pointer is not retained.
void WCM_SetMacAddress(const u8 *mac);

#endif // POKEBW2_NITROWIFI_WCM_INTERNAL_H
