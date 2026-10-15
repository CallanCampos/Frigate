#ifndef POKEBW2_NITROWIFI_WCM_H
#define POKEBW2_NITROWIFI_WCM_H

#include "types.h"

// NitroWiFi's wireless connection manager (WCM), in overlays 11 and 12, which DWC's settings in overlay 182 drive. The
// ROM names none of it: the names are those of the SDK's WCM calls that match what the functions do and where the
// settings call them.

// What a WCM notification reports: the kind of the operation that ended, and its result
typedef struct {
    s16 notify;
    s16 result;
    s32 parameter;
} WCMNotice;

// notify
enum {
    WCM_NOTIFY_COMMON,
    WCM_NOTIFY_STARTUP,
    WCM_NOTIFY_CLEANUP,
    WCM_NOTIFY_BEGIN_SEARCH,
    WCM_NOTIFY_END_SEARCH,
    WCM_NOTIFY_CONNECT,
    WCM_NOTIFY_DISCONNECT,
    WCM_NOTIFY_FOUND_AP,
};

// What the calls answer; the call was taken, and the notification follows
#define WCM_RESULT_ACCEPT 3

// A WCM_Init's work, and the configuration that WCM_StartupAsync starts with
#define WCM_WORK_SIZE 0x2300

// The configuration is consumed during startup. Its writable AP-list buffer is borrowed until replaced or Finish.
// WCM aligns the buffer upward to four bytes, subtracts the skipped bytes, and clears the usable region.
// At least 12 aligned bytes are needed for the AP-list header.
typedef struct {
    s32 dmano;       // WCM uses the low two bits.
    void *pbdbuffer; // Borrowed AP-list storage.
    s32 nbdbuffer;   // Buffer size in bytes, including any leading alignment bytes.
    s32 nbdmode;     // AP-list mode, copied into WCM's work.
} WCMConfig;

typedef void (*WCMNotify)(WCMNotice *notice);

s32 WCM_Init(void *work, s32 size);
// WCM_Init selects mode 1 (DS). The core also accepts 2 (DSi, requiring 0x4800 bytes), or 0 to use the stored
// preference or hardware default. Main's direct caller selects mode 2; this parameter is a mode, not a boolean.
s32 WCM_InitCore(void *work, s32 size, s32 requestedMode);
void WCM_Finish(void);
s32 WCM_StartupAsync(WCMConfig *config, WCMNotify notify);
s32 WCM_CleanupAsync(void);
// Either NULL pointer requests a stop; option is ignored on that path.
s32 WCM_SearchAsync(const u8 *bssid, const u8 *essid, u32 option);
// Descriptive name: requests a scan stop, with completion through WCM_NOTIFY_END_SEARCH.
s32 WCM_StopSearchAsync(void);
// Accepted descriptors are copied into WCM's work during this call; their pointers are not retained.
// option is applied through WCM_SetSearchOption before the connection backend starts.
s32 WCM_ConnectAsync(const void *bssDesc, const void *wepDesc, u32 option);
s32 WCM_DisconnectAsync(void);
// Descriptive name: restores idle operation through the selected backend, resetting an active connection if needed.
s32 WCM_RestoreIdleAsync(void);
s32 WCM_GetState(void);
// Sets the search's channel option bits, returning the previous value
s32 WCM_SetSearchOption(u32 option);
// The channels a search or connect may still try, kept for WCM_PickChannel's callers
s32 WCM_GetChannelMask(void);
s32 WCM_CountApList(void);
s32 WCM_LockApList(s32 lock);
void *WCM_PointApList(u16 index);
// A beacon or probe response NitroSDK's WM handed WCM; fills or touches its WCM_PointApList entry
s32 WCM_OnBeaconReceived(void *frame, u16 param);
// The channel and BSSID WCM is connected to, or 0/NULL while there is none
s32 WCM_GetConnectedApInfo(u16 *channel);
void WCM_DispatchRecvCallback(u8 *src, u8 *dst, u8 *data, u32 len);
// A DMA or WM wait's completion callback, decrementing its remaining count and waking its thread at zero
void WCM_OnDmaDone(void *info);
// Sends a frame to the MAC address dst as a header and the data after it, returning a negative number on an error.
// NitroWiFi's CPS sends every frame through it (WCM_SendDCFDataEx's role)
s32 WCM_SendDCFDataEx(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen);
// Copies the MAC address of the wireless hardware to mac
void WCM_GetMacAddress(u8 *mac);
// The MAC address of the access point the connection is with, or NULL while there is none; NitroWiFi's SOCL takes
// the link to be up while there is one
const u8 *WCM_GetApMacAddress(void);
// Sets the function that every received data frame goes to
void WCM_SetRecvDCFCallback(void (*callback)(u8 *src, u8 *dst, u8 *data, u32 len));

// The "any" BSSID and ESSID that a search takes when the caller gives none
extern const u8 WCMiAnyBssid[];
extern const u8 WCMiAnyEssid[];

#endif // POKEBW2_NITROWIFI_WCM_H
