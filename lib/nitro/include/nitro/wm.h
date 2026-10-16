#ifndef POKEBW2_NITRO_WM_H
#define POKEBW2_NITRO_WM_H

#include "types.h"

// NitroSDK's wireless manager. The functions have the names of the SDK functions they appear to be

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
enum {
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
    WM_ERRCODE_FATAL = 0x13,         // What the ARM7 reports in an indication when it can't go on: the game ends
};

// The state of the manager, in the first halfword of its status
enum {
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
};

// The number of an operation, which a callback starts with and a command to the ARM7 starts with. Only the ones that
// the code of the manager's files uses are named
enum {
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
    WM_APIID_SET_WEP_KEY = 0x14,
    WM_APIID_SET_GAME_INFO = 0x18,
    WM_APIID_SET_BEACON_IND = 0x19,
    WM_APIID_SET_LIFE_TIME = 0x1d,
    WM_APIID_MEASURE_CHANNEL = 0x1e,
    WM_APIID_START_SCAN_EX = 0x26,
    WM_APIID_SET_WEP_KEY_EX = 0x27,
    WM_APIID_ASYNC_KIND_MAX = 0x2c, // The commands are below it; the ARM7's own messages are above
    WM_APIID_INDICATION = 0x80,
    WM_APIID_PORT_SEND = 0x81,
    WM_APIID_PORT_RECV = 0x82,
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
    WM_STATECODE_PORT_INIT = 0x19,
    WM_STATECODE_DISCONNECTED_FROM_MYSELF = 0x1a,
};

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
    u8 unk_36[0xe];
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

// What a parent received from one child in a round of the multi-point mode: the length of the data, which is 0xffff if
// the child sent none, the child's AID, and the data
typedef struct {
    u16 length;
    u16 unk_02;
    u16 aid;
    u8 unk_06[4];
    u8 data[];
} WMMpRecvData;

// The buffer that the multi-point mode receives into: the number of children it has room for and the size of the room
// of each
typedef struct {
    u8 unk_00[4];
    u16 count;
    u16 entrySize;
    u8 unk_08[2];
    u8 entries[];
} WMMpRecvBuf;

// The callback of the start of the multi-point mode
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 padding;
    WMMpRecvBuf *recvBuf;
} WMStartMPCallback;

// The callback of a disconnection: the AIDs that are gone
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 unk_04;
    u16 unk_06;
    u16 unk_08;
    u16 goneAidBitmap;
} WMDisconnectCallback;

// What the indication callback gets
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 unk_06;
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

// The callback of the measure of a channel: how busy it was, and the caller's own callback for that result
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 wlCmdID;
    u16 wlResult;
    u16 channel;
    u16 ccaBusyRatio;
    // Called with the channel and with how busy it was; the SDK's own name for this field is not known
    void (*unk_0c)(u16 channel, u16 busy);
} WMMeasureChannelCallback;

// What a callback of a port gets: a packet that came in, or (state WM_STATECODE_CONNECTED and its like, and
// WM_STATECODE_PORT_INIT) a change of who is connected. The manager makes the second kind itself. Its size is 0x44
typedef struct {
    u16 apiid;
    u16 errcode;
    u16 state;
    u16 port;
    u8 *recvBuf; // The ARM7's buffer of the round of the multi-point mode that the packet is in
    u8 *data;
    u16 length;
    u16 aid;
    u8 macAddress[6];
    u16 seqNo;
    void *arg; // What the port was given with its callback
    u16 myAid;
    u16 connectedAidBitmap;
    u8 ssid[24];
    u16 reason;
    u16 unk_3e;
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
#define WM_DS_SET_COUNT 4
// The most data one set can hold after its first four bytes
#define WM_DS_SET_DATA_MAX (WM_DS_SET_SIZE - 4)

typedef struct {
    u16 aidBitmap;      // The AIDs whose data the set has, or that the parent is still to receive them from
    u16 receivedBitmap; // The AIDs whose data has come in
    u16 data[WM_DS_SET_DATA_MAX / 2];
} WMDataSet;

// The state of a data sharing, in WMDataSharingInfo.state
enum {
    WM_DS_STATE_OFF,
    WM_DS_STATE_RUNNING,
    WM_DS_STATE_SEND_FAILED = 4, // The queue was full: the next step sends again
    WM_DS_STATE_ERROR = 5,
};

// What a data sharing keeps: a ring of sets and the indexes into it (each a count modulo WM_DS_SET_COUNT). Its size is
// 0x820
typedef struct {
    WMDataSet ds[WM_DS_SET_COUNT];
    u16 seqNum[WM_DS_SET_COUNT]; // The sequence number of each set, 0xffff if it was not sent to anyone
    u16 writeIndex;              // The set that is being filled (on a child, the next one that comes in)
    u16 sendIndex;               // The set that goes out next (on a parent, the oldest one that has a sequence number)
    u16 readIndex;               // The next set to give to the game
    u16 aidBitmap;               // The AIDs that share, with this console's own; 0 when no sharing runs
    u16 dataLength;              // The bytes each console gives
    u16 stationNumber;           // How many consoles share
    u16 dataSetLength;           // The bytes of a set: its first four and the data of all
    u16 port;
    u16 doubleMode;    // 1 when two sets are in flight at a time
    u16 currentSeqNum; // The sequence number of the set that the game got last
    u16 state;         // WM_DS_STATE_*
    u16 unk_81e;
} WMDataSharingInfo;

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
#define WM_ARM7_BUF_SIZE 0x300
#define WM_STATUS_AREA_SIZE 0x800
#define WM_INDICATION_BUF_SIZE 0x100

// The commands to the ARM7 and the callbacks that come back for them are in buffers of 0x100 bytes, 10 of them, which
// the manager hands out from a queue. A buffer is free when its first halfword has the bit WM_COMMAND_BUF_FREE
#define WM_NUM_OF_COMMAND_BUFS 10
#define WM_COMMAND_BUF_SIZE 0x100
#define WM_COMMAND_BUF_FREE 0x8000
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
    BOOL mpFlag;  // TRUE from the start of the multi-point mode to its end
    BOOL dcfFlag; // TRUE from the start of the DCF mode to its end
    u8 unk_014[0x28];
    u16 mpSendSize; // The most that this side sends in a round of the multi-point mode
    u16 mpRecvSize; // The most that the other side sends
    u8 unk_040[0x32];
    u16 mpRecvBufSize; // The size of the part of the ARM7's buffer that holds a round that came in
    u8 unk_074[8];
    void *mpSendBuf; // The buffer that the multi-point mode sends from
    u8 unk_080[6];
    u16 unk_086;
    u8 unk_088[0x14];
    u16 mpSizeCheckOff; // 0 for the multi-point mode to check that its buffers have the sizes that it needs
    u8 unk_09e[0x1e];
    u16 linkLevel; // 0 to 3, as WMLinkLevel
    u8 unk_0be[8];
    u16 childMpFlag; // A child may start the multi-point mode only when this is 1
    u8 unk_0c8[0x30];
    u16 maxEntry; // How many children the parent takes
    u8 unk_0fa[0x88];
    u16 childBitmap; // The AIDs of the children that are connected, one bit each
    u8 unk_184[4];
    u16 aid; // This console's AID, 0 for the parent
    u8 unk_18a[0x646];
} WMStatus;

// The ARM9's part of the manager's buffer. Its size is 0x200
typedef struct {
    void *wm7buf;     // 0x00: the ARM7's part, sizeof(WMArm9Buf) after this
    WMStatus *status; // 0x04
    void *unk_08;
    void *unk_0c;                                          // 0x100 bytes after the status area
    WMCommandBuf *indBuf;                                  // 0x10: where the ARM7 puts what it reports on its own
    u16 dmaNo;                                             // 0x14: the DMA channel that WM_Init cleared the buffer with
    u16 unk_16;                                            // 0 from WM_Init: the status is invalidated in the cache
    WMCallbackFunc callbackTable[WM_APIID_ASYNC_KIND_MAX]; // 0x18: the callback of each operation
    WMCallbackFunc indCallback;                            // 0xc8
    WMCallbackFunc portCallbackTable[WM_NUM_OF_PORT];      // 0xcc
    void *portCallbackArgTable[WM_NUM_OF_PORT];            // 0x10c
    u32 connectedAidBitmap;                                // 0x14c
    u16 myAid;                                             // 0x150
    u8 unk_152[0xae];
} WMArm9Buf;

// The shared memory the manager reads: the channels that this console may use, one bit each, and the pool of signal
// strengths that the ARM7 writes, whose bit 0 the manager clears. They are the DSi's mirrors of the DS addresses
// 0x027ffcfa and 0x027fff96
#define HW_WM_ALLOWED_CHANNEL_BUF 0x02fffcfa
#define HW_WM_RSSI_POOL 0x02ffff96

// wm_system.c: the core that the commands to the ARM7 and their callbacks go through. All of these return a
// WM_ERRCODE_*, WM_ERRCODE_SUCCESS being 0
// WM_Init: starts the manager in the buffer, which must be WM_SYSTEM_BUF_SIZE bytes in main memory, 32-aligned, and
// cleared by the DMA channel dmaNo (0 to 3)
int WM_Init(void *wmSysBuf, u16 dmaNo);
// Ends the manager, which must be in the READY state
int WM_Finish(void);
// Sets the callback of an operation
void WMi_SetCallbackTable(u32 apiid, WMCallbackFunc callback);
// WMi_SendCommand: puts a command in a buffer, with paramNum more arguments (words) as its parameters, and sends it to
// the ARM7. WM_ERRCODE_OPERATING if sent, FIFO_ERROR if no buffer is free or the FIFO is full
int WMi_SendCommand(u16 apiid, u16 paramNum, ...);
// The same, with the command already made: size bytes at cmd
int WMi_SendCommandDirect(const void *cmd, u32 size);
// The ARM9 part of the buffer, NULL before WM_Init
WMArm9Buf *WMi_GetSystemWork(void);
// WM_ERRCODE_ILLEGAL_STATE until WM_Init has been called
int WMi_CheckInitialized(void);
// WM_ERRCODE_ILLEGAL_STATE if the manager is in the READY or STOP state
int WMi_CheckIdle(void);
// SUCCESS if the manager is in one of the paramNum states that follow, else ILLEGAL_STATE
int WMi_CheckStateEx(int paramNum, ...);
// The status that the ARM7 keeps, NULL before WM_Init
void *WMi_GetStatusAddress(void);
// WMi_GetMyAid and WMi_GetConnectedAidBitmap: this console's AID and the AIDs connected, as the last connection
// event left them
u16 WMi_GetMyAid(void);
u16 WMi_GetConnectedAidBitmap(void);
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
int WM_SetIndCallback(WMCallbackFunc callback);
int WM_SetPortCallback(u16 port, WMCallbackFunc callback, void *arg);
// Copies the status (0x7d0 bytes)
int WM_ReadStatus(WMStatus *statusBuf);
int WM_GetMPSendBufferSize(void);
int WM_GetMPReceiveBufferSize(void);
u16 WM_GetAllowedChannel(void);
u16 WM_GetDispersionBeaconPeriod(void);
u16 WM_GetDispersionScanPeriod(void);
// wm_standard.c
int WM_Enable(WMCallbackFunc callback);
int WM_Disable(WMCallbackFunc callback);
int WM_PowerOn(WMCallbackFunc callback);
int WM_PowerOff(WMCallbackFunc callback);
int WM_Initialize(void *wmBuf, WMCallbackFunc callback, u16 dmaNo);
// Like WM_Initialize, with a fourth argument that changes the flags to the ARM7: bit 1 is set when it is 0
int WM_InitializeForListening(void *wmBuf, WMCallbackFunc callback, u16 dmaNo, int extra);
int WM_Reset(WMCallbackFunc callback);
int WM_End(WMCallbackFunc callback);
int WM_SetParentParameter(WMCallbackFunc callback, WMParentParam *param);
// The data that a child sent in a round of the multi-point mode, or NULL
const WMMpRecvData *WM_ReadMPData(const WMMpRecvBuf *recvBuf, u16 aid);
// The elements of a beacon that are not the game's, if it has no game information
WMOtherElements WM_GetOtherElements(const WMBssDesc *bssDesc);
// The calls that start the parent: apiids 8 and 0x19, each with a flag. The first is WMi_StartParentEx
int WMi_StartParentEx(WMCallbackFunc callback, int enable);
int WM_SetBeaconIndication(WMCallbackFunc callback, int enable);
int WM_StartParent(WMCallbackFunc callback);
int WM_EndParent(WMCallbackFunc callback);
int WM_StartScanEx(WMCallbackFunc callback, const WMScanExParam *param);
int WM_EndScan(WMCallbackFunc callback);
// Like WM_StartMP, with five more parameters
int WM_StartMPEx(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize, u16 mpFreq,
                 u16 defaultRetryCount, BOOL minPollBmpMode, BOOL singlePacketMode, BOOL fixFreqMode,
                 BOOL ignoreFatalError);
int WM_StartConnectEx(WMCallbackFunc callback, const WMBssDesc *bssDesc, const u8 *ssid, BOOL powerSave, u16 authMode);
int WM_Disconnect(WMCallbackFunc callback, u16 aid);
int WM_StartMP(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize, void *sendBuf, u16 sendBufSize, u16 mpFreq);
int WM_SetMPDataToPortEx(WMCallbackFunc callback, void *arg, const void *sendData, u16 sendDataSize, u16 destBitmap,
                         u16 port, u16 prio);
int WM_EndMP(WMCallbackFunc callback);
// Starts the DCF mode of a child, with the buffer that it receives into
int WM_StartDCF(WMCallbackFunc callback, void *recvBuf, u16 recvBufSize);
// WM_SetDCFData: queues a DCF transmission to the six-byte destination address; buffer remains borrowed until
// completion
int WM_SetDCFData(WMCallbackFunc callback, const u8 *dst, const void *buffer, u16 byteLength);
int WM_EndDCF(WMCallbackFunc callback);
int WM_StartDataSharing(WMDataSharingInfo *dsi, u16 port, u16 aidBitmap, u16 dataLength, BOOL doubleMode);
int WM_EndDataSharing(WMDataSharingInfo *dsi);
int WM_StepDataSharing(WMDataSharingInfo *dsi, const void *data, WMDataSet *dataSet);
void *WM_GetSharedDataAddress(const WMDataSharingInfo *dsi, const WMDataSet *dataSet, u16 aid);
// wm_ks.c: the sharing of the keys of every console, two bytes each, in the double mode
int WM_StartKeySharing(WMDataSharingInfo *dsi, u16 port);
int WM_EndKeySharing(WMDataSharingInfo *dsi);
// wm_etc.c
// Mode 0 (off) to 3, with a key of 0x50 bytes unless it is off
int WM_SetWEPKey(WMCallbackFunc callback, u16 wepMode, const void *wepKey);
// Like WM_SetWEPKey, with the number of the key to use
int WM_SetWEPKeyEx(WMCallbackFunc callback, u16 wepMode, u16 wepKeyId, const void *wepKey);
int WM_SetGameInfo(WMCallbackFunc callback, const void *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid, u8 attr);
int WM_SetLifeTime(WMCallbackFunc callback, u16 tableNumber, u16 camLifeTime, u16 frameLifeTime, u16 mpLifeTime);
int WM_MeasureChannel(WMCallbackFunc callback, u16 ccaMode, u16 edThreshold, u16 channel, u16 measureTime);

#endif // POKEBW2_NITRO_WM_H
