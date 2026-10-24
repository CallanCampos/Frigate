#ifndef POKEBW2_NITRO_WM_H
#define POKEBW2_NITRO_WM_H

#include "types.h"
#include <stddef.h>

// NitroSDK's wireless manager. The names are NitroSDK's, as ntrtwl's reconstruction of NitroSDK has them, where it has
// the code, and ours, in the SDK's style, elsewhere

// The signal strength, 0 (none) to 3
typedef enum {
    WM_LINK_LEVEL_0,
    WM_LINK_LEVEL_1,
    WM_LINK_LEVEL_2,
    WM_LINK_LEVEL_3,
} WMLinkLevel;

WMLinkLevel WM_GetLinkLevel(void);
// The next TGID for a parent, counting up from the time (swan names its state tgid_bak)
u16 WM_GetNextTgid(void);

// What the parent's callback gets when a child connects or disconnects, with the child's SSID and the sizes that the
// parent and the children may send
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u8 macAddress[6];
    u16 aid;
    u16 reason;
    u8 ssid[24];
    u16 parentSize;
    u16 childSize;
} WMStartParentCallback;

// The results of the WM functions: they all start an operation and give OPERATING when it has begun, with its result
// to come to the callback
typedef enum {
    WM_ERRCODE_SUCCESS,
    WM_ERRCODE_FAILED,
    WM_ERRCODE_OPERATING,
    WM_ERRCODE_ILLEGAL_STATE = 3, // Not initialized, or in a state that the call is not for
    WM_ERRCODE_WM_DISABLE = 4,    // The ARM7's side of the manager is not running
    WM_ERRCODE_NO_DATASET = 5,    // The data sharing has no new set of data yet
    WM_ERRCODE_INVALID_PARAM = 6,
    WM_ERRCODE_NO_CHILD = 7,         // The child of the AID is not connected, or the parent has none
    WM_ERRCODE_FIFO_ERROR = 8,       // No command buffer is free, or the FIFO to the ARM7 is full
    WM_ERRCODE_SEND_QUEUE_FULL = 10, // The queue of the port has no room for the data
    WM_ERRCODE_FLASH_ERROR = 0x13,   // What the ARM7 reports in an indication when it can't go on: the game ends
} WMErrCode;

// The state of the manager, in the first halfword of its status
typedef enum {
    WM_STATE_READY,
    WM_STATE_STOP,
    WM_STATE_IDLE,
    WM_STATE_CLASS1,
    WM_STATE_TESTMODE,
    WM_STATE_SCAN,
    WM_STATE_CONNECT,
    WM_STATE_PARENT,
    WM_STATE_CHILD,
    WM_STATE_MP_PARENT,
    WM_STATE_MP_CHILD,
    WM_STATE_DCF_CHILD,
} WMState;

// The number of an operation, which a callback starts with and a command to the ARM7 starts with. Only the ones that
// the code of the manager's files uses are named
typedef enum {
    WM_APIID_INITIALIZE = 0,
    WM_APIID_RESET = 1,
    WM_APIID_END = 2,
    WM_APIID_ENABLE = 3,
    WM_APIID_DISABLE = 4,
    WM_APIID_POWER_ON = 5,
    WM_APIID_POWER_OFF = 6,
    WM_APIID_SET_P_PARAM = 7,
    WM_APIID_START_PARENT = 8,
    WM_APIID_END_PARENT = 9,
    WM_APIID_END_SCAN = 0xb,
    WM_APIID_START_CONNECT = 0xc,
    WM_APIID_DISCONNECT = 0xd,
    WM_APIID_START_MP = 0xe,
    WM_APIID_SET_MP_DATA = 0xf,
    WM_APIID_END_MP = 0x10,
    WM_APIID_START_DCF = 0x11,
    WM_APIID_SET_DCF_DATA = 0x12,
    WM_APIID_END_DCF = 0x13,
    WM_APIID_SET_WEPKEY = 0x14,
    WM_APIID_START_KS = 0x15,
    WM_APIID_SET_GAMEINFO = 0x18,
    WM_APIID_SET_BEACON_IND = 0x19,
    WM_APIID_SET_LIFETIME = 0x1d,
    WM_APIID_MEASURE_CHANNEL = 0x1e,
    WM_APIID_START_SCAN_EX = 0x26,
    WM_APIID_SET_WEPKEY_EX = 0x27,
    WM_NUM_OF_CALLBACK = 0x2c, // The commands are below it; the ARM7's own messages are above
    WM_APIID_INDICATION = 0x80,
    WM_APIID_PORT_SEND = 0x81,
    WM_APIID_PORT_RECV = 0x82,
} WMApiid;

// The state that a callback reports, as the callback's function sees it
typedef enum {
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
    WM_STATECODE_PORT_INIT = 0x19,
    WM_STATECODE_DISCONNECTED_FROM_MYSELF = 0x1a,
} WMStateCode;

// The first fields of every callback's argument
typedef struct {
    u16 apiid;
    u16 errcode;
} WMCallback;

typedef void (*WMCallbackFunc)(void *arg);

// The most that the game's information in a beacon can hold
#define WM_SIZE_USER_GAMEINFO 0x70

// The most bytes that a parent or a child sends in one round of the multi-point mode
#define WM_SIZE_MP_DATA_MAX 0x200

// What a parent gives out in its beacon: the game's ID and the user's data. Its size is 0x38
typedef struct {
    u16 *userGameInfo;
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
    u8 userGameInfo[WM_SIZE_USER_GAMEINFO];
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
    u8 bssid[6];
    u16 scanType;
    u16 ssidLength;
    u8 ssid[32];
    u16 ssidMatchLength; // How much of the SSID has to match, for the scan types 2 and 3: 32 at most
    u8 rsv2[0xe];
} WMScanExParam;

// The callback of the scan, with the parents it found
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u16 channelList;
    u8 reserved[2];
    u16 bssDescCount;
    WMBssDesc *bssDesc[16];
    u16 linkLevel[16];
} WMStartScanExCallback;

// What a parent received from one child in a round of the multi-point mode: the length of the data, which is 0xffff if
// the child sent none, the child's AID, and the data
typedef struct WMMpRecvData {
    u16 length;
    u16 rate_rssi;
    u16 aid;
    u16 noResponse;
    u16 wmHeader;
    u16 cdata[1];
} WMMpRecvData;

// What a parent received in a round of the multi-point mode: count entries of what each child sent, length bytes apart
typedef struct WMMpRecvHeader {
    u16 bitmap;
    u16 errBitmap;
    u16 count;
    u16 length;
    u16 txCount;
    WMMpRecvData data[1];
} WMMpRecvHeader;

// The buffer that the multi-point mode receives a frame into
typedef struct WMMpRecvBuf {
    u16 seqCtrl;
    u16 txop;
    u16 bitmap;
    u16 wmHeader;
    u16 data[2];
} WMMpRecvBuf;

// The buffer that the DCF mode receives a frame into: the frame's length, signal, addresses and data
typedef struct WMDcfRecvBuf {
    u16 frameID;
    u16 rsv1[2];
    u16 length;
    u16 rsv2[3];
    u16 rate_rssi; // The signal's strength in the high byte
    u16 rsv3[4];
    u8 destAdrs[6];
    u8 srcAdrs[6];
    u16 rsv4[4];
    u16 data[2];
} WMDcfRecvBuf;

// The callback of the start of the multi-point mode
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u8 reserved[2];
    WMMpRecvBuf *recvBuf;
} WMStartMPCallback;

// The callback of the DCF mode: its start (state 14) and each frame that comes in (state 15), in recvBuf
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u8 reserved[2];
    WMDcfRecvBuf *recvBuf;
} WMStartDCFCallback;

// The callback of a disconnection: the AIDs that are gone
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 tryBitmap;
    u16 disconnectedBitmap;
} WMDisconnectCallback;

// What the indication callback gets
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 reason;
} WMIndCallback;

// The callback of the child's connection: the parent's answer, the child's AID, why it was disconnected, the wireless
// status code of a failed connection, the parent's address and the sizes of the parent's and the child's data
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u16 aid;
    u16 reason;
    u16 wlStatus;
    u8 macAddress[6];
    u16 parentSize;
    u16 childSize;
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

// What a callback of a port gets: a packet that came in, or (state WM_STATECODE_CONNECTED and its like, and
// WM_STATECODE_PORT_INIT) a change of who is connected. The manager makes the second kind itself. Its size is 0x44
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 port;
    WMMpRecvBuf *recvBuf; // The ARM7's buffer of the round of the multi-point mode that the packet is in
    u16 *data;
    u16 length;
    u16 aid;
    u8 macAddress[6];
    u16 seqNo;
    void *arg; // What the port was given with its callback
    u16 myAid;
    u16 connectedAidBitmap;
    u8 ssid[24];
    u16 reason;
    u16 rsv;
    u16 maxSendDataSize;
    u16 maxRecvDataSize;
} WMPortRecvCallback;

// What the callback of a send of data gets back as its own argument
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 state;
    u16 port; // The port that the data was sent to
    u8 unk_0c[0xe];
    u16 seqNo;               // The sequence number that the data went out with
    WMCallbackFunc callback; // The one that the send was given
    void *arg;
} WMPortSendCallback;

// The data sharing: each console gives a few bytes in every round of the multi-point mode, and the parent hands
// everyone's data back as one set. A set starts with the AIDs it is for and the AIDs whose data it holds, then has the
// data of each AID in turn (the same length for all)
#define WM_DS_SET_SIZE 0x200
#define WM_DS_DATASET_NUM 4
// The most data one set can hold after its first four bytes
#define WM_DS_DATA_SIZE (WM_DS_SET_SIZE - 4)

typedef struct {
    u16 aidBitmap;      // The AIDs whose data the set has, or that the parent is still to receive them from
    u16 receivedBitmap; // The AIDs whose data has come in
    u16 data[WM_DS_DATA_SIZE / 2];
} WMDataSet;

// The state of a data sharing, in WMDataSharingInfo.state
typedef enum {
    WM_DS_STATE_READY,
    WM_DS_STATE_START,
    WM_DS_STATE_RETRY_SEND = 4, // The queue was full: the next step sends again
    WM_DS_STATE_ERROR = 5,
} WMDataSharingState;

// What a data sharing keeps: a ring of sets and the indexes into it (each a count modulo WM_DS_DATASET_NUM). Its size
// is 0x820
typedef struct {
    WMDataSet ds[WM_DS_DATASET_NUM];
    u16 seqNum[WM_DS_DATASET_NUM]; // The sequence number of each set, 0xffff if it was not sent to anyone
    u16 writeIndex;                // The set that is being filled (on a child, the next one that comes in)
    u16 sendIndex;     // The set that goes out next (on a parent, the oldest one that has a sequence number)
    u16 readIndex;     // The next set to give to the game
    u16 aidBitmap;     // The AIDs that share, with this console's own; 0 when no sharing runs
    u16 dataLength;    // The bytes each console gives
    u16 stationNumber; // How many consoles share
    u16 dataSetLength; // The bytes of a set: its first four and the data of all
    u16 port;
    u16 doubleMode;    // 1 when two sets are in flight at a time
    u16 currentSeqNum; // The sequence number of the set that the game got last
    u16 state;         // WM_DS_STATE_*
    u16 reserved;
} WMDataSharingInfo;

// What a key sharing keeps: a data sharing's
typedef WMDataSharingInfo WMKeySetBuf;

// The elements of a beacon that are not the game's: the ID of each, its length and where its data is. At most 16. The
// elements are in the order they are in the beacon; the sizes of WMOtherElement and WMOtherElements are 8 and 0x84
typedef struct {
    u8 id;
    u8 length;
    u8 *data;
} WMOtherElement;

typedef struct {
    u8 count;
    WMOtherElement element[16];
} WMOtherElements;

// The buffer that the manager works in, which the game gives to WM_Init, 32-aligned: its ARM9 part, the ARM7's part,
// the status that the ARM7 writes and two buffers of commands and callbacks. WM_Init clears all of it
#define WM_SYSTEM_BUF_SIZE 0xf00
#define WM_ARM7WM_BUF_SIZE 0x300
#define WM_STATUS_BUF_SIZE 0x800
#define WM_FIFO_BUF_SIZE 0x100

// The commands to the ARM7 and the callbacks that come back for them are in buffers of 0x100 bytes, 10 of them, which
// the manager hands out from a queue. A buffer is free when its first halfword has the bit WM_API_REQUEST_ACCEPTED
#define WM_BUF_MSG_NUM 10
#define WM_API_REQUEST_ACCEPTED 0x8000
#define WM_NUM_OF_PORT 16

typedef struct {
    u16 apiid;
    u16 unk_02;
    u32 param[63];
} WMCommandBuf;

// What the ARM7 keeps up to date in main memory for the ARM9 to read, after it has invalidated the cache for the part
// that it reads. Its size is 0x7d0. Only the fields that the code of the manager's first files reads are named
typedef struct {
    u16 state; // WM_STATE_*
    u8 unk_002[0xa];
    BOOL mp_flag;  // TRUE from the start of the multi-point mode to its end
    BOOL dcf_flag; // TRUE from the start of the DCF mode to its end
    u8 unk_014[0x28];
    u16 mp_maxSendSize; // The most that this side sends in a round of the multi-point mode
    u16 mp_maxRecvSize; // The most that the other side sends
    u8 unk_040[0x32];
    u16 mp_recvBufSize; // The size of the part of the ARM7's buffer that holds a round that came in
    u8 unk_074[8];
    void *mp_sendBuf; // The buffer that the multi-point mode sends from
    u8 unk_080[6];
    u16 mp_readyBitmap;
    u8 unk_088[0x14];
    u16 mp_ignoreSizePrecheckMode; // 0 for the multi-point mode to check that its buffers have the sizes that it needs
    u8 unk_09e[0x1e];
    u16 linkLevel; // 0 to 3, as WMLinkLevel
    u8 unk_0be[8];
    u16 pwrMgtMode; // A child may start the multi-point mode only when this is 1
    u8 unk_0c8[0x30];
    u16 maxEntry; // How many children the parent takes
    u8 unk_0fa[0x88];
    u16 child_bitmap; // The AIDs of the children that are connected, one bit each
    u8 pInfoBuf[4];
    u16 aid; // This console's AID, 0 for the parent
    u8 unk_18a[0x646];
} WMStatus;

// The ARM9's part of the manager's buffer. Its size is 0x200
typedef struct {
    void *WM7;        // 0x00: the ARM7's part, sizeof(WMArm9Buf) after this
    WMStatus *status; // 0x04
    void *indbuf;
    void *fifo9to7;                                   // 0x100 bytes after the status area
    WMCommandBuf *fifo7to9;                           // 0x10: where the ARM7 puts what it reports on its own
    u16 dmaNo;                                        // 0x14: the DMA channel that WM_Init cleared the buffer with
    u16 scanOnlyFlag;                                 // 0 from WM_Init: the status is invalidated in the cache
    WMCallbackFunc CallbackTable[WM_NUM_OF_CALLBACK]; // 0x18: the callback of each operation
    WMCallbackFunc indCallback;                       // 0xc8
    WMCallbackFunc portCallbackTable[WM_NUM_OF_PORT]; // 0xcc
    void *portCallbackArgument[WM_NUM_OF_PORT];       // 0x10c
    u32 connectedAidBitmap;                           // 0x14c
    u16 myAid;                                        // 0x150
    u8 reserved1[0xae];
} WMArm9Buf;

// The shared memory the manager reads: the channels that this console may use, one bit each, and the pool of signal
// strengths that the ARM7 writes, whose bit 0 the manager clears. They are the DSi's mirrors of the DS addresses
// 0x027ffcfa and 0x027fff96
#define HW_WM_ALLOWED_CHANNEL_BUF 0x02fffcfa
#define HW_WM_CALLBACK_CONTROL 0x02ffff96

// wm_system.c: the core that the commands to the ARM7 and their callbacks go through. All of these return a
// WM_ERRCODE_*, WM_ERRCODE_SUCCESS being 0
// WM_Init: starts the manager in the buffer, which must be WM_SYSTEM_BUF_SIZE bytes in main memory, 32-aligned, and
// cleared by the DMA channel dmaNo (0 to 3)
WMErrCode WM_Init(void *wmSysBuf, u16 dmaNo);
// Ends the manager, which must be in the READY state
WMErrCode WM_Finish(void);
// Sets the callback of an operation
void WMi_SetCallbackTable(WMApiid apiid, WMCallbackFunc callback);
// WMi_SendCommand: puts a command in a buffer, with paramNum more arguments (words) as its parameters, and sends it to
// the ARM7. WM_ERRCODE_OPERATING if sent, FIFO_ERROR if no buffer is free or the FIFO is full
WMErrCode WMi_SendCommand(WMApiid apiid, u16 paramNum, ...);
// The same, with the command already made: size bytes at cmd
WMErrCode WMi_SendCommandDirect(void *cmd, u32 size);
// The ARM9 part of the buffer, NULL before WM_Init
WMArm9Buf *WMi_GetSystemWork(void);
// WM_ERRCODE_ILLEGAL_STATE until WM_Init has been called
WMErrCode WMi_CheckInitialized(void);
// WM_ERRCODE_ILLEGAL_STATE if the manager is in the READY or STOP state
WMErrCode WMi_CheckIdle(void);
// SUCCESS if the manager is in one of the paramNum states that follow, else ILLEGAL_STATE
WMErrCode WMi_CheckStateEx(s32 paramNum, ...);
// The status that the ARM7 keeps, NULL before WM_Init
const WMStatus *WMi_GetStatusAddress(void);
// WM_GetAID and WM_GetConnectedAIDs: this console's AID and the AIDs connected, as the last connection
// event left them
u16 WM_GetAID(void);
u16 WM_GetConnectedAIDs(void);
// Ends the program if the console goes to sleep, till WMi_DeleteSleepCallback
void WMi_RegisterSleepCallback(void);
void WMi_DeleteSleepCallback(void);
// Whether wireless is usable: on a DSi, when it is available and not disabled by force. TRUE on a DS
BOOL WMi_IsWirelessAvailable(void);
// wm_common.c: WM_CheckInitialized, the same as WMi_CheckInitialized
int WM_CheckInitialized(void);
// NWM_CheckInitialized: ILLEGAL_STATE on a DS, and on a DSi what the DSi's code says about the wireless it is running
// on
int NWM_CheckInitialized(void);

// wm_sync.c
WMErrCode WM_SetIndCallback(WMCallbackFunc callback);
WMErrCode WM_SetPortCallback(u16 port, WMCallbackFunc callback, void *arg);
// Copies the status (0x7d0 bytes)
WMErrCode WM_ReadStatus(WMStatus *statusBuf);
int WM_GetMPSendBufferSize(void);
int WM_GetMPReceiveBufferSize(void);
u16 WM_GetAllowedChannel(void);
u16 WM_GetDispersionBeaconPeriod(void);
u16 WM_GetDispersionScanPeriod(void);
// wm_standard.c
WMErrCode WM_Enable(WMCallbackFunc callback);
WMErrCode WM_Disable(WMCallbackFunc callback);
WMErrCode WM_PowerOn(WMCallbackFunc callback);
WMErrCode WM_PowerOff(WMCallbackFunc callback);
WMErrCode WM_Initialize(void *wmBuf, WMCallbackFunc callback, u16 dmaNo);
// Like WM_Initialize, with a fourth argument that changes the flags to the ARM7: bit 1 is set when it is 0
WMErrCode WM_InitializeForListening(void *wmBuf, WMCallbackFunc callback, u16 dmaNo, BOOL extra);
WMErrCode WM_Reset(WMCallbackFunc callback);
WMErrCode WM_End(WMCallbackFunc callback);
WMErrCode WM_SetParentParameter(WMCallbackFunc callback, const WMParentParam *param);
// The data that a child sent in a round of the multi-point mode, or NULL
WMMpRecvData *WM_ReadMPData(const WMMpRecvHeader *header, u16 aid);
// The elements of a beacon that are not the game's, if it has no game information
WMOtherElements WM_GetOtherElements(const WMBssDesc *bssDesc);
// The calls that start the parent: apiids 8 and 0x19, each with a flag. The first is WMi_StartParentEx
WMErrCode WMi_StartParentEx(WMCallbackFunc callback, BOOL enable);
WMErrCode WM_SetBeaconIndication(WMCallbackFunc callback, u16 enable);
WMErrCode WM_StartParent(WMCallbackFunc callback);
WMErrCode WM_EndParent(WMCallbackFunc callback);
WMErrCode WM_StartScanEx(WMCallbackFunc callback, const WMScanExParam *param);
WMErrCode WM_EndScan(WMCallbackFunc callback);
// Like WM_StartMP, with five more parameters
WMErrCode WM_StartMPEx(WMCallbackFunc callback, u16 *recvBuf, u16 recvBufSize, u16 *sendBuf, u16 sendBufSize,
                       u16 mpFreq, u16 defaultRetryCount, BOOL minPollBmpMode, BOOL singlePacketMode, BOOL fixFreqMode,
                       BOOL ignoreFatalError);
WMErrCode WM_StartConnectEx(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid, BOOL powerSave,
                            u16 authMode);

// NitroSDK's WM_StartConnect: WM_StartConnectEx in power-save mode, with open-system authentication
static inline WMErrCode WM_StartConnect(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid) {
    return WM_StartConnectEx(callback, bssDesc, ssid, TRUE, 0);
}

// NitroSDK's WM_IsBssidEqual: whether two BSSIDs are the same
static inline BOOL WM_IsBssidEqual(const u8 *idp1, const u8 *idp2) {
    return idp1[0] == idp2[0] && idp1[1] == idp2[1] && idp1[2] == idp2[2] && idp1[3] == idp2[3] && idp1[4] == idp2[4] &&
           idp1[5] == idp2[5];
}

// NitroSDK's WM_IsValidGameInfo: whether a beacon's game info has a valid length and the magic number
static inline BOOL WM_IsValidGameInfo(const WMGameInfo *gameInfo, u16 gameInfoLength) {
    return gameInfoLength >= 0x10 && gameInfoLength <= sizeof(WMGameInfo) &&
           gameInfoLength == 0x10 + gameInfo->userGameInfoLength && gameInfo->magicNumber == 1;
}

// NitroSDK's WM_IsValidGameBeacon: whether a beacon carries a valid game info that fills it
static inline BOOL WM_IsValidGameBeacon(const WMBssDesc *bssDesc) {
    return WM_IsValidGameInfo(&bssDesc->gameInfo, bssDesc->gameInfoLength) &&
           bssDesc->length == (bssDesc->gameInfoLength + 1 + offsetof(WMBssDesc, gameInfo)) / sizeof(u16);
}
WMErrCode WM_Disconnect(WMCallbackFunc callback, u16 aid);
WMErrCode WM_StartMP(WMCallbackFunc callback, u16 *recvBuf, u16 recvBufSize, u16 *sendBuf, u16 sendBufSize, u16 mpFreq);
WMErrCode WM_SetMPDataToPortEx(WMCallbackFunc callback, void *arg, const u16 *sendData, u16 sendDataSize,
                               u16 destBitmap, u16 port, u16 prio);
WMErrCode WM_EndMP(WMCallbackFunc callback);
// Starts the DCF mode of a child, with the buffer that it receives into
WMErrCode WM_StartDCF(WMCallbackFunc callback, WMDcfRecvBuf *recvBuf, u16 recvBufSize);
// WM_SetDCFData: queues a DCF transmission to the six-byte destination address; buffer remains borrowed until
// completion
WMErrCode WM_SetDCFData(WMCallbackFunc callback, const u8 *dst, const u16 *buffer, u16 byteLength);
WMErrCode WM_EndDCF(WMCallbackFunc callback);
WMErrCode WM_StartDataSharing(WMDataSharingInfo *dsi, u16 port, u16 aidBitmap, u16 dataLength, BOOL doubleMode);
WMErrCode WM_EndDataSharing(WMDataSharingInfo *dsi);
WMErrCode WM_StepDataSharing(WMDataSharingInfo *dsi, const u16 *data, WMDataSet *dataSet);
u16 *WM_GetSharedDataAddress(WMDataSharingInfo *dsi, WMDataSet *dataSet, u16 aid);
// wm_ks.c: the sharing of the keys of every console, two bytes each, in the double mode
WMErrCode WM_StartKeySharing(WMKeySetBuf *buf, u16 port);
WMErrCode WM_EndKeySharing(WMKeySetBuf *buf);
// wm_etc.c
// Mode 0 (off) to 3, with a key of 0x50 bytes unless it is off
WMErrCode WM_SetWEPKey(WMCallbackFunc callback, u16 wepMode, const u16 *wepKey);
// Like WM_SetWEPKey, with the number of the key to use
WMErrCode WM_SetWEPKeyEx(WMCallbackFunc callback, u16 wepMode, u16 wepKeyId, const u8 *wepKey);
WMErrCode WM_SetGameInfo(WMCallbackFunc callback, const u16 *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid,
                         u8 attr);
WMErrCode WM_SetLifeTime(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime);
WMErrCode WM_MeasureChannel(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime);

#endif // POKEBW2_NITRO_WM_H
