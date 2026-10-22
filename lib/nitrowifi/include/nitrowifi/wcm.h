#ifndef POKEBW2_NITROWIFI_WCM_H
#define POKEBW2_NITROWIFI_WCM_H

#include "types.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// NitroWiFi's wireless connection manager (WCM), in overlays 11 and 12, which DWC's settings in overlay 182 drive. The
// ROM names none of it: established SDK names are kept where identified; descriptive reconstruction names are
// noted below.

// A word of a WCM notification, which carries a number or a pointer
typedef union {
    s32 n;
    void *p;
} WCMNoticeParameter;

// What a WCM notification reports: the kind of the operation that ended, its result and three words about it (for a
// found access point, the first is its WMBssDesc; for a connection, the second is why it failed)
typedef struct {
    s16 notify;
    s16 result;
    WCMNoticeParameter parameter[3];
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
    WCM_NOTIFY_SEARCH_AROUND, // A search went once through the channels
    WCM_NOTIFY_TERMINATE,     // The WCM ended without being asked
};

// What the calls answer; the call was taken, and the notification follows
#define WCM_RESULT_ACCEPT 3

// A WCM_Init's work, and the configuration that WCM_StartupAsync starts with
#define WCM_WORK_SIZE 0x2300

// The bytes of an access point's description that an AP list entry keeps (WCM_PointApList's result)
#define WCM_APLIST_SIZE 192

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

// The function that every received data frame goes to
typedef void (*WCMRecvInd)(const u8 *srcAddr, const u8 *dstAddr, const u8 *buf, s32 len);

s32 WCM_Init(void *work, s32 size);
// WCM_Init selects mode 1 (DS). The core also accepts 2 (DSi, requiring 0x4800 bytes), or 0 to use the stored
// preference or hardware default. Main's direct caller selects mode 2; this parameter is a mode, not a boolean.
s32 WCM_InitCore(void *work, s32 size, s32 requestedMode);
// Releases the work buffer: 0, or 1 when WCM is not initialized or not idle (state 1)
s32 WCM_Finish(void);
s32 WCM_StartupAsync(WCMConfig *config, WCMNotify notify);
s32 WCM_CleanupAsync(void);
// Either NULL pointer requests a stop; option is ignored on that path.
s32 WCM_SearchAsync(void *bssid, void *essid, u32 option);
// Descriptive name: requests a scan stop, with completion through WCM_NOTIFY_END_SEARCH.
s32 WCM_StopSearchAsync(void);
// Accepted descriptors are copied into WCM's work during this call; their pointers are not retained.
// option is applied through WCM_SetSearchOption before the connection backend starts.
s32 WCM_ConnectAsync(void *bssDesc, void *wepDesc, u32 option);
s32 WCM_DisconnectAsync(void);
// Descriptive name: restores idle operation through the selected backend, resetting an active connection if needed.
s32 WCM_RestoreIdleAsync(void);
s32 WCM_GetState(void);
// Updates the selected option fields and ORs the entire normalized incoming word, returning the previous word.
u32 WCM_SetSearchOption(u32 option);
// The options of WCM_ConnectAsync that pick the power saving and the authentication (descriptive names). Each is a
// pair of bits: the upper one says that the option is given, the lower one is its value (WCM_SetSearchOption sets or
// clears 0x10000 and 0x40000 by it), and the connection reads them back as "!= 0x30000" for power saving and
// "== 0xc0000" for a shared key
#define WCM_OPTION_POWER_SAVE 0x20000
#define WCM_OPTION_POWER_ACTIVE 0x30000
#define WCM_OPTION_AUTH_OPEN 0x80000
#define WCM_OPTION_AUTH_SHARED 0xc0000
// DS mode fills gaps between the lowest and highest hardware-allowed channel bits; DSi uses its backend mask.
// Returns zero outside states 3 through 10.
s32 WCM_GetChannelMask(void);
// Clears the AP-list storage with IRQs disabled, invalidating borrowed AP-list records regardless of list lock.
void WCM_ClearApListBuffer(void);
// Descriptive name: returns a computed level from 0 through 3 in connected state 9, or zero otherwise.
WMLinkLevel WCM_UpdateLinkQuality(void);
s32 WCM_CountApList(void);
s32 WCM_LockApList(s32 lock);
WMBssDesc *WCM_PointApList(u16 index);
// A beacon or probe response NitroSDK's WM handed WCM; fills or touches its WCM_PointApList entry
void WCM_OnBeaconReceived(const WMBssDesc *bss, u16 scanLinkLevel);
// Descriptive name: destination is a raw 0xc0-byte retained descriptor with selected IEs packed after offset 0x40.
void WCM_FillApEntry(const WMBssDesc *source, WMBssDesc *destination);
// Descriptive name: borrowed SSID bytes and optional length, or NULL/zero while disconnected. The bytes need not be
// NUL-terminated and remain owned by WCM; a connection change or Finish can invalidate the pointer.
u8 *WCM_GetConnectedApSsid(u16 *ssidLength);
void WCM_DispatchRecvCallback(const u8 *src, const u8 *dst, const u8 *data, s32 len);
// Descriptive name: releases one mutex level only when its owner is the IRQ-handler address used as a special marker.
void WCM_ReleaseIrqOwnedMutex(OSMutex *mutex);
// Sends to the six-byte address dst, copying headerLen/dataLen bytes into WCM's work during this call. Assembly
// precedes backend length validation. Returns -1 without work or -4 for an invalid state/reset in progress;
// after a send attempt it returns headerLen + dataLen even if the backend rejected the assembled frame.
s32 WCM_SendDCFDataEx(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen);
// Copies the MAC address of the wireless hardware to mac
void WCM_GetMacAddress(u8 *mac);
// The MAC address of the access point the connection is with, or NULL while there is none; NitroWiFi's SOCL takes
// the link to be up while there is one
u8 *WCM_GetApMacAddress(void);
// Sets the function that every received data frame goes to
void WCM_SetRecvDCFCallback(WCMRecvInd callback);

// The "any" BSSID and ESSID that a search takes when the caller gives none
extern const u8 WCMiAnyBssid[];
extern const u8 WCMiAnyEssid[];

#endif // POKEBW2_NITROWIFI_WCM_H
