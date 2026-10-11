#ifndef POKEBW2_GFL_WIH_H
#define POKEBW2_GFL_WIH_H

#include "types.h"
#include "gfl/heap.h"

// wih.c, in overlay 30: the wireless helper, which grew out of NitroSDK's demo wh.c; the DS Download Play parent uses it to
// pick a channel and to shut the wireless down. The comments give the wh.c functions these appear to be

// The filter that the parent passes each child that connects to (its callback from the wireless), with the filter's work
// (the GFL net work): it says whether the child may stay
typedef BOOL (*NetScanFilter)(const void *info, void *work);
// What the helper calls with the AID of a child that has connected, or of the parent once this machine has connected
typedef void (*WHConnectCallback)(int netId);
// What it calls with the data a machine sent, with the machine's AID, and with no data when a machine has disconnected
typedef BOOL (*WHReceiver)(u16 netId, u8 *data, u16 size);
// What it calls once data has gone out
typedef BOOL (*WHSendCallback)(BOOL ok);
// What the scan calls with each beacon that is a game's, to say whether to go on to connect to it
typedef BOOL (*WHScanCallback)(const void *beacon);
// What the scan calls with each beacon that it finds, whatever it is, with the GFL net work and the signal strength
typedef void (*WHScanInfoCallback)(const void *beacon, void *work, u16 linkLevel);

// How the parent filters the children that connect, and how many frames the scan waits before it starts again
void WH_SetScanFilter(NetScanFilter filter);
void WH_SetScanTime(u16 time);

// The helper's states (WH_SYSSTATE_*)
enum {
    WH_SYSSTATE_STOP,
    WH_SYSSTATE_IDLE,
    WH_SYSSTATE_SCANNING,
    WH_SYSSTATE_BUSY,
    WH_SYSSTATE_CONNECTED,
    WH_SYSSTATE_DATASHARING,
    WH_SYSSTATE_KEYSHARING,
    WH_SYSSTATE_MEASURECHANNEL,
    WH_SYSSTATE_CONNECT_FAIL,
    WH_SYSSTATE_ERROR,
    WH_SYSSTATE_FATAL,
};

// How a machine connects (WH_CONNECTMODE_*): in the multi-point mode, with the keys shared, or with the data shared
enum {
    WH_CONNECTMODE_MP_PARENT,
    WH_CONNECTMODE_MP_CHILD,
    WH_CONNECTMODE_KS_PARENT,
    WH_CONNECTMODE_KS_CHILD,
    WH_CONNECTMODE_DS_PARENT,
    WH_CONNECTMODE_DS_CHILD,
};

// Called when the wireless has ended, with whether that succeeded. The callers ignore its result
typedef BOOL (*WHCallback)(BOOL success);

int WH_GetSystemState(void);  // WH_GetSystemState
BOOL WH_StartMeasureChannel(void); // WH_StartMeasureChannel
u16 WH_GetMeasureChannel(void);  // WH_GetMeasureChannel
// Starts the helper with its work on the heap given
BOOL WH_Initialize(HeapID heapId, WHCallback callback, u32 extended);
// Frees the helper's work
void WH_Release(void); // WH_Release
BOOL WH_Finalize(void);                // WH_Finalize
BOOL WH_End(WHCallback callback); // WH_End
// The wireless error that WH_GetLastError records, and the data the helper received from a machine
int WH_GetLastError(void);
u8 *WH_GetSharedDataAdr(u16 netId);
BOOL WH_StepDS(u8 *data);
// Sets what runs when a machine disconnects
void WH_SetConnectCallback(WHConnectCallback callback);

// What net_whpipe.c calls
void WH_SetGgid(u32 ggid);                   // WH_SetGgid
void WH_SetUserGameInfo(const void *data, u16 size); // WH_SetUserGameInfo
u16 WH_GetBitmap(void);                        // WH_GetBitmap
// Scans for parents, calling back with each beacon it finds
BOOL WH_StartScan(WHScanCallback callback, const u8 *mac, u16 channel); // WH_StartScan
// Connects to a parent as a child, by its MAC address
BOOL WH_ChildConnectAuto(u16 mode, const u8 *mac, u16 channel); // WH_ChildConnectAuto
// Sets the channel and MAC address of the parent to connect to
void WH_SetScanTarget(u16 channel, const u8 *mac);
BOOL WH_EndScan(void);
// Starts as a parent (mode, tgid, channel, most children)
BOOL WH_ParentConnect(u16 mode, u16 tgid, u16 channel, u16 maxEntry);
// Sets what the helper calls with the data it receives from a machine
void WH_SetReceiver(WHReceiver callback); // WH_SetReceiver
// Sends data to the parent or the children, then calls back
BOOL WH_SendData(u8 *data, u16 size, WHSendCallback callback); // WH_SendData
u16 WH_GetAid(void);
// Sets what the helper calls with each beacon the child receives
void WH_SetScanCallback(WHScanCallback callback);
// Sends a beacon of the data, the game's GGID and the TGID
void WH_SetGameInfo(const void *data, int size, u32 ggid, int tgid); // WH_SetGameInfo
u16 WH_GetBeaconCount(void);
// Sets whether the scan stops starting again
void WH_SetScanPaused(BOOL paused);
// Sets what the scan calls with each beacon that it finds
void WH_SetScanInfoCallback(WHScanInfoCallback callback);
void WH_UpdateScan(void); // Run each frame: starts the scan again
void WH_Reset(void);


#endif // POKEBW2_GFL_WIH_H
