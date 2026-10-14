#ifndef POKEBW2_DWC_WCM_H
#define POKEBW2_DWC_WCM_H

#include "types.h"

// NitroWiFi's wireless connection manager (WCM), in overlays 11 and 12, which DWC's settings in overlay 182 drive. The
// ROM names none of it: the names are those of the SDK's WCM calls that match what the functions do and where the
// settings call them. Their symbols keep the default names until they are confirmed

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

typedef struct {
    s32 dmano;
    void *pbdbuffer;
    s32 nbdbuffer;
    s32 nbdmode;
} WCMConfig;

typedef void (*WCMNotify)(WCMNotice *notice);

s32 func_ov011_0215bbec(void *work, s32 size);                         // WCM_Init
void func_ov011_0215bce0(void);                                        // WCM_Finish
s32 func_ov011_0215bd58(WCMConfig *config, WCMNotify notify);          // WCM_StartupAsync, with a configuration
s32 func_ov011_0215be00(void);                                         // WCM_CleanupAsync
s32 func_ov011_0215be88(const u8 *bssid, const u8 *essid, u32 option); // WCM_SearchAsync
s32 func_ov011_0215bf78(const void *bssDesc, const void *wepDesc, u32 wepMode); // WCM_ConnectAsync
s32 func_ov011_0215c0ac(void);                                                  // WCM_DisconnectAsync
s32 func_ov011_0215c5d0(void);                                                  // WCM_CountApList
s32 func_ov011_0215c60c(s32 lock);                                              // WCM_LockApList
void *func_ov011_0215c65c(u16 index);                                           // WCM_PointApList
// WCM_SendDCFDataEx: sends a frame to the MAC address dst as a header and the data after it, returning a negative
// number on an error. NitroWiFi's CPS sends every frame through it
s32 func_ov011_0215cba4(const u8 *dst, const u8 *header, s32 headerLen, const u8 *data, s32 dataLen);
// Copies the MAC address of the wireless hardware to mac (WCM_GetMacAddress's role)
void func_ov011_0215ccc0(u8 *mac);
// The MAC address of the access point the connection is with, or NULL while there is none (WCM_GetApMacAddress's
// role); NitroWiFi's SOCL takes the link to be up while there is one
const u8 *func_ov011_0215cb14(void);
// Sets the function that every received data frame goes to (WCM_SetRecvDCFCallback's role)
void func_ov011_0215cb8c(void (*callback)(u8 *src, u8 *dst, u8 *data, u32 len));

// The "any" BSSID and ESSID that a search takes when the caller gives none
extern const u8 data_ov011_02184fdc[];
extern const u8 data_ov011_02184fe4[];

#endif // POKEBW2_DWC_WCM_H
