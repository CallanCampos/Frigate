#ifndef POKEBW2_APP_DOWNLOAD_PLAY_WIH_H
#define POKEBW2_APP_DOWNLOAD_PLAY_WIH_H

#include "types.h"
#include "app/download_play.h"
#include "nitro/wm.h"

// The wireless helper of the DS Download Play range of overlay 182 (src/ov182/download_play_wih.c): it measures how
// busy each channel is and keeps the ones that were least busy. The names are ours

// The helper's states (WIH_STATE_*), with overlay 30's helper's numbering: the states this file does not have are
// kept, because the state it passes on is the number the state callbacks know
enum {
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
};

// How this machine connects (WIH_CONNECTMODE_*), which the parent passes: in the multi-point mode, with the keys
// shared, or with the data shared. Overlay 30's helper numbers them the same way
enum {
    WIH_CONNECTMODE_MP_PARENT,
    WIH_CONNECTMODE_MP_CHILD,
    WIH_CONNECTMODE_KS_PARENT,
    WIH_CONNECTMODE_KS_CHILD,
    WIH_CONNECTMODE_DS_PARENT,
    WIH_CONNECTMODE_DS_CHILD,
};

// The state names, one per state before WIH_STATE_FATAL, which the state change passes to the work's log
extern const char *const stateNames[];

// The messages the helper logs, one for each call site. Their text is the file's own delinked .data, which the file
// does not write yet, so they are declared here until it does. The names are ours
extern const char data_ov182_021c18d4[]; // the state it moves from
extern const char data_ov182_021c18dc[]; // the state it moves to
extern const char data_ov182_021c18e0[]; // a child that connected
extern const char data_ov182_021c190c[]; // a child that disconnected
extern const char data_ov182_021c1938[]; // a state code that is not known
extern const char data_ov182_021c1958[]; // key sharing would not start
extern const char data_ov182_021c1988[]; // the end of the parent mode failed
extern const char data_ov182_021c19b0[]; // the receive buffer size
extern const char data_ov182_021c19c8[]; // the send buffer size
extern const char data_ov182_021c19e0[]; // how busy the measured channel was
extern const char data_ov182_021c19f8[]; // the channel that was chosen
extern const char data_ov182_021c1a10[]; // the connect mode that is not a parent one
extern const char data_ov182_021c1a2c[]; // the data sharing had nothing to send
extern const char data_ov182_021c1a5c[]; // the data sharing failed
extern const char data_ov182_021c1a90[]; // the finalize found the helper idle
extern const char data_ov182_021c1ab4[]; // the finalize ends what the helper is doing

// What the helper prints its state changes and its failures through, which it takes from the first word of the work
// pointer. Nothing in this build sets it
typedef void (*WHLogPrint)(u32 level, const char *format, ...);
// What the scan calls with each child that connects, to say whether it may stay
typedef BOOL (*WHScanFilter)(void *info);
// What fills the WEP key of the work and gives the mode to set the wireless to
typedef u16 (*WHWepKeySetup)(void *key);

// The helper's work, which the DS Download Play parent allocates: DownloadPlayWork's loaderContext is one of these,
// and the parent's own state fills the gap between the two ends of it
typedef struct {
    // The parameter the parent sets up and hands to the wireless
    WMParentParam sParentParam;
    u8 unk_38[8];
    // The state the helper is in (WIH_STATE_*)
    int sysState;
    // How this machine connects (WIH_CONNECTMODE_*)
    int sConnectMode;
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
    u8 wepKey[0x20];
    // The data sharing, the set it works in, and the one that the key sharing uses
    WMDataSharingInfo dataSharing;
    WMDataSet dataSet;
    WMDataSharingInfo keySharing;
} WHWork;

// The helper's work, and what it prints through, which the file's functions reach the helper's work through
typedef struct {
    WHLogPrint logPrint;
    WHWork *work;
} WHWorkPointer;

extern WHWorkPointer gWork;

// What the file that follows this one in the overlay calls
void func_ov182_021ba180(u32 ggid);
u16 func_ov182_021ba18c(void);
int func_ov182_021ba19c(void);
u16 func_ov182_021ba350(void);
BOOL func_ov182_021ba42c(void);
BOOL func_ov182_021ba4f8(u16 mode, u16 tgid, u16 channel);
void func_ov182_021ba5d0(WHScanFilter filter);
u8 *func_ov182_021ba5dc(u16 netId);
BOOL func_ov182_021ba5fc(u8 *data);
void func_ov182_021ba668(void);
void func_ov182_021ba67c(void);
BOOL func_ov182_021ba730(void);

#endif // POKEBW2_APP_DOWNLOAD_PLAY_WIH_H
