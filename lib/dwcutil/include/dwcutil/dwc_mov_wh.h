#ifndef POKEBW2_DWCUTIL_DWC_MOV_WH_H
#define POKEBW2_DWCUTIL_DWC_MOV_WH_H

#include "types.h"
#include "dwcutil/dwc_mov_mbp.h"
#include "nitro/wm.h"

// The DWC utility's wireless helper for DS Download Play in overlay 182 (dwc_mov_wh.c): it measures how busy each
// channel is and keeps the ones that were least busy. The functions have the SDK's names (see dwc_mov_wh.c); the
// types, the constants and the data are ours

// The helper's states (WIH_STATE_*), with overlay 30's helper's numbering: the states this file does not have are
// kept, because the state it passes on is the number the state callbacks know
typedef enum {
    WIH_STATE_STOP,
    WIH_STATE_IDLE,
    WIH_STATE_SCANNING,
    WIH_STATE_BUSY,
    WIH_STATE_CONNECTED,
    WIH_STATE_DATASHARING,
    WIH_STATE_KEYSHARING,
    WIH_STATE_MEASURECHANNEL,
    WIH_STATE_CONNECT_FAIL,
    WIH_STATE_ERROR,
    WIH_STATE_FATAL,
} WIHState;

// How this machine connects (WIH_CONNECTMODE_*), which the parent passes: in the multi-point mode, with the keys
// shared, or with the data shared. Overlay 30's helper numbers them the same way
typedef enum {
    WIH_CONNECTMODE_MP_PARENT,
    WIH_CONNECTMODE_MP_CHILD,
    WIH_CONNECTMODE_KS_PARENT,
    WIH_CONNECTMODE_KS_CHILD,
    WIH_CONNECTMODE_DS_PARENT,
    WIH_CONNECTMODE_DS_CHILD,
} WIHConnectMode;

// The helper's own errors, which it keeps as the first error after the manager's (WMErrCode): the wireless has no
// channel it may use, and no channel is left to measure. Overlay 30's helper reports the same numbers
enum {
    WIH_ERRCODE_NO_RADIO = 0x16,
    WIH_ERRCODE_NOMORE_CHANNEL = 0x18,
};

// What the helper prints its state changes and its failures through. Nothing in this build sets it
typedef void (*WHLogPrint)(u32 level, const char *format, ...);
// What the scan calls with each child that connects, to say whether it may stay
typedef BOOL (*WHScanFilter)(WMStartParentCallback *info);
// What fills the WEP key of the work, given the parent parameter, and gives the mode to set the wireless to
typedef u16 (*WHWepKeySetup)(u16 *key, const WMParentParam *parentParam);

// The helper's work, which the DS Download Play parent allocates: MovMbpWork's loaderContext is one of these,
// and the parent's own state fills the gap between the two ends of it
typedef struct {
    // The parameter the parent sets up and hands to the wireless
    WMParentParam sParentParam;
    u8 unk_38[8];
    // The state the helper is in (WIH_STATE_*)
    WIHState sysState;
    // How this machine connects (WIH_CONNECTMODE_*)
    WIHConnectMode sConnectMode;
    u32 unk_48;
    // What the scan calls with each child that connects, which nothing in this build sets
    WHScanFilter sScanFilter;
    u16 sAid;
    u16 sBitmap;
    // The error that was reported first
    int lastError;
    // The value the channel is chosen from, which each measurement advances
    u32 seed;
    u16 channel;
    // The least busy that a channel was, and the channels that were as little busy
    u16 minBusy;
    u16 bestBitmap;
    u8 unk_62[0x1e];
    // The buffer that the wireless works in
    u8 wmBuf[0xf00];
    // The buffers of the multi-point mode, and their sizes
    u8 sendBuf[0xe0];
    u8 recvBuf[0x180];
    u8 unk_11e0[0xc0];
    u32 sendBufSize;
    u32 recvBufSize;
    u8 unk_12a8[0x100];
    // The three words at the end of the parent's part of the work, which its file leaves to the helper to clear
    u32 unk_13a8;
    // What fills the WEP key and gives the mode to set the wireless to
    WHWepKeySetup unk_13ac;
    u32 unk_13b0;
    u8 unk_13b4[0xc];
    // The WEP key that unk_13ac fills
    u16 wepKey[0x10];
    // The data sharing, the set it works in, and the one that the key sharing uses
    WMDataSharingInfo dataSharing;
    WMDataSet dataSet;
    WMKeySetBuf keySharing;
} WHWork;

// What the file that follows this one in the overlay calls
void DWCi_MOV_WH_SetGgid(u32 ggid);
u16 DWCi_MOV_WH_GetBitmap(void);
int DWCi_MOV_WH_GetSystemState(void);
u16 DWCi_MOV_WH_GetMeasureChannel(void);
BOOL DWCi_MOV_WH_Initialize(void);
BOOL DWCi_MOV_WH_ParentConnect(u16 mode, u16 tgid, u16 channel);
void DWCi_MOV_WH_SetJudgeAcceptFunc(WHScanFilter filter);
u8 *DWCi_MOV_WH_GetSharedDataAdr(u16 netId);
BOOL DWCi_MOV_WH_StepDS(u8 *data);
void DWCi_MOV_WH_Reset(void);
void DWCi_MOV_WH_Finalize(void);
BOOL DWCi_MOV_WH_End(void);

// The child's connection, which nothing in the game calls (see dwc_mov_wh.c)
void DWCi_MOV_WH_ChildConnect(void);

#endif // POKEBW2_DWCUTIL_DWC_MOV_WH_H
