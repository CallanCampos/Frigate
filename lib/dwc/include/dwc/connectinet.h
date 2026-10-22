#ifndef POKEBW2_DWC_CONNECTINET_H
#define POKEBW2_DWC_CONNECTINET_H

#include "types.h"
#include "nitro/wm.h"

// Connecting to the internet (TWL-DWC 5.3's dwc_connectinet.c, in ARM9 main): the library starts the auto connect of
// the DS and DSi (AC) with the console's Wi-Fi settings, and when that fails it looks for a Nintendo Zone access point
// with the WDS scan and connects to the one it finds. The ROM names none of it: the names are
// NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of NitroDWC have them, where the code shows
// them, and ours elsewhere. The access point information that the same file gives is in dwc/apinfo.h, and the WDS scan,
// a file of its own, is in dwc/wdsscan.h.

// What a connection is in (DWCInetControl.state), and what DWC_GetInetStatus reports. The names follow the SDK's
// DWC_CONNECTINET_STATE_* and its order from memory; the code confirms IDLE, OPERATING, CONNECTED, DISCONNECTED, ERROR
// and FATAL_ERROR (the game's caller waits for CONNECTED), and OPERATED as the state the poll sets once the AC has a
// result. DISCONNECTING is only ever reported, never kept. The SDK's DWCInetResult
typedef enum DWCInetResult {
    // There is no connection control
    DWC_CONNECTINET_STATE_NOT_INITIALIZED = 0,
    // Started, not connecting yet
    DWC_CONNECTINET_STATE_IDLE = 1,
    // Connecting
    DWC_CONNECTINET_STATE_OPERATING = 2,
    // The AC has ended, and the status settles its result
    DWC_CONNECTINET_STATE_OPERATED = 3,
    // Connected
    DWC_CONNECTINET_STATE_CONNECTED = 4,
    DWC_CONNECTINET_STATE_DISCONNECTING = 5,
    // The link was lost after connecting
    DWC_CONNECTINET_STATE_DISCONNECTED = 6,
    // The connection failed, with an error of the AC class
    DWC_CONNECTINET_STATE_ERROR = 7,
    // The connection failed for good
    DWC_CONNECTINET_STATE_FATAL_ERROR = 8,
} DWCInetResult;

// Where the connection is with the WDS scan (DWCInetControl.scanStage)
enum {
    DWC_WDS_STATE_IDLE = 0,
    DWC_WDS_STATE_PROCESS = 1,
    // The scan found an access point, which is waiting in DWCInetControl.ap
    DWC_WDS_STATE_COMPLETED = 2,
    DWC_WDS_STATE_FAILED = 3,
    // The failure was reported as an error
    DWC_INET_SCAN_REPORTED = 4,
    // Reported as a state of its own, and nothing in main sets it
    DWC_INET_SCAN_5 = 5,
};

// An access point to connect to: what the WDS scan found, which the connection starts with in place of the console's
// settings. Its size is 0x50
typedef struct DWCWDSData {
    char ssid[32];
    // The WEP key: its first 5, 13 or 16 bytes, by the mode
    u8 wep[32];
    // 0 for none, 1 for WEP 40, 2 for WEP 104 and 3 for WEP 128
    int wepMode;
    u16 mtu;
    // The area digit and the nine characters of the spot's name that a Nintendo access point tells
    char apnum[10];
} DWCWDSData;

// A DWCInetControl of the SDK, which the game gives the library to keep the connection in. Its size is 0x68
typedef struct DWCInetControl {
    // The AC's last result: 0 while it runs, 5 once it is connected, a negative number for an error
    int ac_state;
    u16 state;
    // 1 while the connection is up and being watched for a loss
    u16 online;
    u16 dmaNo;
    u16 powerMode;
    // The TCP receive window that the sockets of a DSi get (SOCLConfig.rwin), or 0 for the default
    int rwin;
    // Nonzero lets the AC skip its check that the DSi's user agreed to the EULA
    u8 skipEulaCheck;
    // Set once the connection cannot try again: it succeeded, it failed for good, or it was given the access point
    u8 ended;
    int processWDSstate;
    DWCWDSData nspotInfo;
} DWCInetControl;

// Starts a connection control, a DWCInetControl that the game gives: it takes the DMA channel for the AC and the
// priority of SSL. The power mode is not used: the control always gets mode 1. DWC_InitInetEx.
void DWC_InitInetEx(DWCInetControl *control, u16 dmaNo, u16 powerMode, u16 sslPriority);
// Starts connecting with the console's Wi-Fi settings: DWC_ConnectInetAsync
void DWC_ConnectInetAsync(void);
// Whether the connection has ended, in which case it also settles the status: DWC_CheckInet
BOOL DWC_CheckInet(void);
// Takes the next step of the connection: DWC_ProcessInet
void DWC_ProcessInet(void);
// The connection's state, one of the above: DWC_GetInetStatus
DWCInetResult DWC_GetInetStatus(void);
// Ends the connection and frees the control, waiting for the AC to finish: DWC_CleanupInet
void DWC_CleanupInet(void);
// How good the link is, from 0 to 3: DWC_GetLinkLevel
WMLinkLevel DWC_GetLinkLevel(void);
// What the game calls every frame once it is connected: takes a step of the connection (DWC_ProcessInet), and if the
// link was lost sets the error for it and returns TRUE: DWC_UpdateConnection
BOOL DWC_UpdateConnection(void);

#endif // POKEBW2_DWC_CONNECTINET_H
