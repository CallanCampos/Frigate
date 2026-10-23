#ifndef POKEBW2_TWL_NWM_H
#define POKEBW2_TWL_NWM_H

#include "types.h"
#include "twl/pm.h"

// TwlSDK's NWM, the DSi's wireless manager, ARM9 side, in the LTD autoload (nwm_setup.c, nwm_events.c and nwm.c). It
// does for the DSi's own wireless chip what NitroSDK's WM does for the DS's: the ARM9 sends the ARM7 commands through
// the PXI tag 0x16, in buffers of a system area that the game hands to the library (NWM_Init), and the ARM7 answers
// with events that run the command's callback. The functions, types and values have the names of libntr's
// reconstruction of TwlSDK's headers (nwm.h, and nwm_arm9_private.h for the NWMi_ functions), by the API ID each
// command sends and what the code does; the ones still named by address are not known. The game calls the functions
// through thumb veneers that the linker made.

typedef enum NWMRetCode {
    NWM_RETCODE_SUCCESS = 0,
    NWM_RETCODE_FAILED = 1,
    NWM_RETCODE_OPERATING = 2,     // The command was sent: its callback gets the result
    NWM_RETCODE_ILLEGAL_STATE = 3, // Not initialized, or not in a state that takes the command, or already busy with it
    NWM_RETCODE_NWM_DISABLE = 4,   // The wireless is not available, or the ARM7 is not ready for the tag
    NWM_RETCODE_INVALID_PARAM = 5,
    NWM_RETCODE_FIFO_ERROR = 6,  // No free command buffer, or the FIFO refused the word
    NWM_RETCODE_FATAL_ERROR = 7, // NWM_Open returns it when the wireless is turned off in the settings
    NWM_RETCODE_NETBUF_ERROR = 8,
    NWM_RETCODE_WMI_ERROR = 9,
    NWM_RETCODE_SDIO_ERROR = 10,
    NWM_RETCODE_RECV_IND = 11,   // A receive buffer is full: the callback of the command that set it gives it back
    NWM_RETCODE_INDICATION = 12, // A sent frame's event that does not run the frame's callback
    NWM_RETCODE_MAX
} NWMRetCode;

// The commands, by their API IDs: the first halfword of the buffer that goes to the ARM7 and of the event that comes
// back. They also index the callbacks and the ARM7's flags of the commands it is running
typedef enum NWMApiid {
    NWM_APIID_INIT = 0,
    NWM_APIID_RESET,
    NWM_APIID_END,
    NWM_APIID_LOAD_DEVICE,
    NWM_APIID_UNLOAD_DEVICE,
    NWM_APIID_OPEN,
    NWM_APIID_CLOSE,
    NWM_APIID_START_SCAN,
    NWM_APIID_CONNECT,
    NWM_APIID_DISCONNECT,
    NWM_APIID_SET_RECEIVING_BUF,
    NWM_APIID_SEND_FRAME,
    NWM_APIID_UNSET_RECEIVING_BUF,
    NWM_APIID_SET_WEPKEY,
    NWM_APIID_SET_PS_MODE,
    NWM_APIID_SET_WPA_KEY,
    NWM_APIID_SET_WPA_PARAMS,
    NWM_APIID_CREATE_QOS,
    NWM_APIID_SET_WPA_PSK,
    NWM_APIID_INSTALL_FIRMWARE,
    NWM_APIID_ASYNC_KIND_MAX,
    NWM_APIID_INDICATION = 128,
    NWM_APIID_UNKNOWN = 255
} NWMApiid;

// What the ARM7 reports as the library's state, which the commands check
typedef enum NWMState {
    NWM_STATE_NONE,
    NWM_STATE_INITIALIZED,
    NWM_STATE_LOADED,
    NWM_STATE_DISCONNECTED,
    NWM_STATE_INFRA_CONNECTED,
    NWM_STATE_ADHOC_CONNECTED
} NWMState;

// The argument of NWM_SetPowerSaveMode, which WCM gives from its power saving option after it connects
typedef enum NWMPowerMode { NWM_POWERMODE_ACTIVE, NWM_POWERMODE_STANDARD, NWM_POWERMODE_UAPSD } NWMPowerMode;

// The kind of network that NWM_Connect joins
typedef enum NWMNwType { NWM_NWTYPE_INFRA, NWM_NWTYPE_ADHOC, NWM_NWTYPE_WPS, NWM_NWTYPE_NUM } NWMNwType;

// The protocols that the received frames are sorted by, each with its callback
typedef enum NWMFramePort { NWM_PORT_IPV4_ARP, NWM_PORT_EAPOL, NWM_PORT_OTHERS, NWM_PORT_NUM } NWMFramePort;

// What a command does when the ARM7 is done; the argument is the event that it sent back
typedef void (*NWMCallbackFunc)(void *arg);

// A scan result: its length in bytes, the access point's fields, and from
// NWM_BSS_DESC_ELEMENTS_OFFSET the 802.11 information elements of the beacon, each an ID, a length and its bytes
#define NWM_BSS_DESC_ELEMENTS_OFFSET 0x40

typedef struct NWMBssDesc {
    u16 length;
    s16 rssi;
    u8 bssid[6];
    u16 ssidLength;
    u8 ssid[32];
    u16 capaInfo;
    struct {
        u16 basic;
        u16 support;
    } rateSet;
    u16 beaconPeriod;
    u16 dtimPeriod;
    u16 channel;
    u16 cfpPeriod;
    u16 cfpMaxDuration;
    u16 reserved;
    u16 elementCount;
    u16 elements[1];
} NWMBssDesc;

// An information element of a scan result: its ID, its length and its bytes
typedef struct NWMInfoElements {
    u8 id;
    u8 length;
    u16 element[1];
} NWMInfoElements;

// The start of the event that a command's callback gets: the API ID of the command it answers and its result
typedef struct NWMCallback {
    u16 apiid;
    u16 retcode;
} NWMCallback;

// The event of a scan (NWM_StartScanEx): the channel scanned, as its bit, and the access points found on it
typedef struct NWMStartScanCallback {
    NWMCallback header;
    u32 channelList;
    u8 reserved[2];
    u16 bssDescCount;
    u32 allBssDescSize;
    NWMBssDesc *bssDesc[32];
    u16 linkLevel[32];
} NWMStartScanCallback;

// The event of NWM_Connect: the association ID, and the reason of a failed connection
typedef struct NWMConnectCallback {
    NWMCallback header;
    u16 channel;
    u8 bssid[6];
    s16 rssi;
    u16 aid;
    u16 reason;
} NWMConnectCallback;

// The start of a received frame, its 802.3 header: the two MAC addresses and the protocol, then the frame's data
typedef struct NWMRecvFrameHdr {
    u8 da[6];
    u8 sa[6];
    u8 pid[2];
    u8 frame[2];
} NWMRecvFrameHdr;

// The event of a frame received in the buffer of NWM_SetReceivingFrameBuffer: the frame and its length
typedef struct NWMReceivingFrameCallback {
    NWMCallback header;
    u16 port;
    s16 rssi;
    u32 length;
    NWMRecvFrameHdr *recvBuf;
} NWMReceivingFrameCallback;

// What a scan is asked for (the SDK's NWMScanExParam, up to its ssidMatchLength). The command takes its fields from
// here, and the library zeroes the buffer
typedef struct NWMScanExParam {
    NWMBssDesc *scanBuf;
    u16 scanBufSize;
    u16 channelList;
    u16 channelDwellTime;
    u16 scanType;
    u8 bssid[6];
    u16 ssidLength; // At most 32
    u8 ssid[32];
    u16 ssidMatchLength; // At most 32
} NWMScanExParam;

// What NWM_SetWPAPSK takes: an authentication mode from 4 to 7 and the 32 bytes of the pre-shared key
typedef struct NWMWpaParam {
    u16 auth;
    u8 psk[32];
} NWMWpaParam;

// NWM_Init: sets the library up with a 32-byte-aligned system area of the given size (whose beginning holds the work,
// its buffers and its callbacks) and a DMA channel to clear it with, and takes the PXI tag
NWMRetCode NWM_Init(void *sysBuf, u32 bufSize, u8 dmaNo);
// SUCCESS if the library is initialized, else ILLEGAL_STATE
NWMRetCode NWMi_CheckInitialized(void);
// The library's state (NWMState), 0 when it is not initialized
u16 NWM_GetState(void);
// A halfword (at 0x9e) of the ARM7's status block
u16 func_02702980(void);
// A number from the console's MAC address and the VBlank count: 30 to 39 when the scan type is 1, else 105 to 114
u16 NWM_GetDispersionScanPeriod(u16 scanType);
NWMRetCode NWM_End(void);
// Also puts the callbacks that exit the application when the console sleeps and before it exits
NWMRetCode NWM_LoadDevice(NWMCallbackFunc callback);
NWMRetCode NWM_UnloadDevice(NWMCallbackFunc callback);
// For a console with the wireless on
NWMRetCode NWM_Open(NWMCallbackFunc callback);
NWMRetCode NWM_Close(NWMCallbackFunc callback);
NWMRetCode NWM_StartScanEx(NWMCallbackFunc callback, const NWMScanExParam *param);
// The first information element of a scan result with the ID, or NULL
NWMInfoElements *NWM_GetInfoElements(NWMBssDesc *bssDesc, u8 elementID);
// The first vendor-specific element (ID 0xdd) of a scan result that begins with the four bytes (the OUI and the
// type), or NULL; any other ID finds none
NWMInfoElements *NWM_GetVenderInfoElements(NWMBssDesc *bssDesc, u8 elementID, const u8 ouiType[4]);
// Connects to the access point of the scan result, of which the ARM7 gets a copy
NWMRetCode NWM_Connect(NWMCallbackFunc callback, u8 nwType, const NWMBssDesc *pBdesc);
NWMRetCode NWM_Disconnect(NWMCallbackFunc callback);
// The WEP mode is 0 to 3 (none, 40, 104 and 128 bits), and the 0x50 bytes of the keys go along
NWMRetCode NWM_SetWEPKey(NWMCallbackFunc callback, u16 wepmode, u16 wepkeyid, const u8 *wepkey, u16 authMode);
NWMRetCode NWM_SetPowerSaveMode(NWMCallbackFunc callback, NWMPowerMode powerSave);
// NWM_SendFrame: sends the frame, whose first 12 bytes the library fills with the two MAC addresses. The callback runs
// when the ARM7 is done with it
NWMRetCode NWM_SendFrame(NWMCallbackFunc callback, const u8 *destAddr, const u16 *sendFrame, u16 sendFrameSize);
// NWM_SetReceivingFrameBuffer: gives the ARM7 a buffer (zeroed) to receive frames in, and the callback that the
// events of the protocol's port run
NWMRetCode NWM_SetReceivingFrameBuffer(NWMCallbackFunc callback, u8 *recvBuf, u16 recvBufSize, u16 protocol);
NWMRetCode NWM_UnsetReceivingFrameBuffer(NWMCallbackFunc callback, u16 protocol);
NWMRetCode NWM_SetWPAPSK(NWMCallbackFunc callback, NWMWpaParam *param);

#endif // POKEBW2_TWL_NWM_H
