#ifndef POKEBW2_NITRO_WM_H
#define POKEBW2_NITRO_WM_H

#include "types.h"

// NitroSDK's wireless manager. The functions keep their default names; the comments give the SDK functions they
// appear to be

// The signal strength, 0 (none) to 3
typedef enum {
    WM_LINK_LEVEL_0,
    WM_LINK_LEVEL_1,
    WM_LINK_LEVEL_2,
    WM_LINK_LEVEL_3,
} WMLinkLevel;

WMLinkLevel func_020810fc(void); // WM_GetLinkLevel
// The next TGID for a parent, counting up from the time (swan names its state tgid_bak)
u16 func_020812b8(void);

// What the parent's callback gets when a child connects or disconnects
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u8 macAddress[6];
    u16 aid;
    u16 reason;
} WMStartParentCallback;

// The results of the WM functions: they all start an operation and give OPERATING when it has begun, with its result
// to come to the callback
enum {
    WM_ERRCODE_SUCCESS,
    WM_ERRCODE_FAILED,
    WM_ERRCODE_OPERATING,
};

// The state that a callback reports, as the callback's function sees it
enum {
    WM_STATECODE_PARENT_START = 0,
    WM_STATECODE_BEACON_SENT = 2,
    WM_STATECODE_PARENT_NOT_FOUND = 4,
    WM_STATECODE_PARENT_FOUND = 5,
    WM_STATECODE_CONNECTED = 7,
    WM_STATECODE_DISCONNECTED = 9,
    WM_STATECODE_MP_START = 10,
    WM_STATECODE_MPEND_IND = 11,
    WM_STATECODE_MP_IND = 12,
    WM_STATECODE_MPACK_IND = 13,
    WM_STATECODE_PORT_RECV = 0x15,
    WM_STATECODE_DISCONNECTED_FROM_MYSELF = 0x1a,
};

// The first fields of every callback's argument
typedef struct {
    u16 apiid;
    u16 errcode;
} WMCallback;

typedef void (*WMCallbackFunc)(void *arg);

// What a parent gives out in its beacon: the game's ID and the user's data. Its size is 0x38
typedef struct {
    u8 *userGameInfo;
    u16 userGameInfoLength;
    u16 padding;
    u32 ggid;
    u16 tgid;
    u16 entryFlag;
    u16 maxEntry;
    u16 multiBootFlag;
    u16 KS_Flag;
    u16 CS_Flag;
    u16 beaconPeriod;
    u8 rsv1[0x18];
    u16 channel;
    u16 parentMaxSize;
    u16 childMaxSize;
} WMParentParam;

// The game's part of a beacon
typedef struct {
    u16 magicNumber;
    u8 ver;
    u8 platform;
    u32 ggid;
    u16 tgid;
    u8 userGameInfoLength;
    u8 gameNameCount_attribute;
    u16 parentMaxSize;
    u16 childMaxSize;
    u8 userGameInfo[0x70];
} WMGameInfo;

// The description of a parent that a scan found. Its size is 0xc0
typedef struct {
    u16 length;
    u16 rssi;
    u8 bssid[6];
    u16 ssidLength;
    u8 ssid[32];
    u16 capaInfo;
    u16 rateSetBasic;
    u16 rateSetSupport;
    u16 beaconPeriod;
    u16 dtimPeriod;
    u16 channel;
    u16 cfpPeriod;
    u16 cfpMaxDuration;
    u16 gameInfoLength;
    u16 otherElementCount;
    WMGameInfo gameInfo;
} WMBssDesc;

// The scan's parameters. Its size is 0x44
typedef struct {
    WMBssDesc *scanBuf;
    u16 scanBufSize;
    u16 channelList;
    u16 maxChannelTime;
    u16 bssid[3];
    u16 scanType;
    u16 ssidLength;
    u8 ssid[32];
    u8 ssidMatchMask[16];
} WMScanExParam;

// The callback of the scan, with the parents it found
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u16 channel;
    u16 reserved;
    u16 bssDescCount;
    WMBssDesc *bssDesc[16];
    u16 linkLevel[16];
} WMStartScanExCallback;

// The callback of the start of the multi-point mode
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
} WMStartMPCallback;

// The callback of the child's connection: the parent's answer and the child's AID
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u16 aid;
} WMStartConnectCallback;

// The callback of the measure of a channel: how busy it was
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 channel;
    u16 ccaBusyRatio;
} WMMeasureChannelCallback;

// What a callback of a port gets when a packet came in
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 port;
    u8 unk_08[4];
    u8 *data;
    u16 length;
    u16 aid;
} WMPortRecvCallback;

// What the callback of a send of data gets back as its own argument
typedef struct {
    u16 apiid;
    u16 errcode;
    u8 unk_04[0x1c];
    void *arg;
} WMPortSendCallback;

// The data that a data sharing keeps, and the set that it reads out. Their sizes are 0x820 and 0x200
typedef struct {
    u8 unk_00[0x820];
} WMDataSharingInfo;
typedef struct {
    u8 unk_00[0x200];
} WMDataSet;

int func_02080e7c(WMCallbackFunc callback);                                    // WM_SetIndCallback
int func_02080eac(u16 port, WMCallbackFunc callback, void *arg);               // WM_SetPortCallback
u32 func_02080f74(void);                                                       // WM_GetMPSendBufferSize
u32 func_02080fbc(void);                                                       // WM_GetMPReceiveBufferSize
u16 func_020810cc(void);                                                       // WM_GetAllowedChannel
u16 func_0208115c(void);                                                       // WM_GetDispersionBeaconPeriod
u16 func_020811a4(void);                                                       // WM_GetDispersionScanPeriod
int func_020813c4(void *wmBuf, WMCallbackFunc callback, u16 dmaNo);            // WM_Initialize
int func_020813d0(void *wmBuf, WMCallbackFunc callback, u16 dmaNo, int extra); // WM_Initialize, with a fourth argument
int func_02081424(WMCallbackFunc callback);                                    // WM_Reset
int func_02081448(WMCallbackFunc callback);                                    // WM_End
int func_02081474(WMCallbackFunc callback, WMParentParam *param);              // WM_SetParentParameter
int func_02081594(WMCallbackFunc callback);                                    // WM_StartParent
int func_020815a0(WMCallbackFunc callback);                                    // WM_EndParent
int func_020815c8(WMCallbackFunc callback, WMScanExParam *param);              // WM_StartScanEx
int func_020816b4(WMCallbackFunc callback);                                    // WM_EndScan
// WM_StartConnectEx
int func_020816dc(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid, BOOL powerSave, u16 authMode);
int func_02081774(WMCallbackFunc callback, u16 aid); // WM_Disconnect
// WM_StartMP
int func_0208195c(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize,
                  u16 mpFreq);
// WM_SetMPDataToPortEx
int func_02081998(WMCallbackFunc callback, void *arg, const void *sendData, u16 sendDataSize, u16 destBitmap, u16 port,
                  u16 prio);
int func_02081a68(WMCallbackFunc callback); // WM_EndMP
// WM_StartDataSharing
int func_02081bd8(WMDataSharingInfo *dsi, u16 port, u16 aidBitmap, u16 dataLength, BOOL doubleMode);
int func_02081dbc(WMDataSharingInfo *dsi);                                                    // WM_EndDataSharing
int func_02081df0(WMDataSharingInfo *dsi, const void *data, WMDataSet *dataSet);              // WM_StepDataSharing
void *func_020823ac(const WMDataSharingInfo *dsi, const WMDataSet *dataSet, u16 aid);         // WM_GetSharedDataAddress
// WM_SetGameInfo
int func_020824bc(WMCallbackFunc callback, const void *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid, u8 attr);
// WM_SetLifeTime
int func_02082560(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime);
// WM_MeasureChannel
int func_0208259c(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime);

#endif // POKEBW2_NITRO_WM_H
