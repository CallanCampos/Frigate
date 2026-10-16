#ifndef POKEBW2_APP_DOWNLOAD_PLAY_SYS_H
#define POKEBW2_APP_DOWNLOAD_PLAY_SYS_H

#include "types.h"
#include "app/download_play.h"
#include "app/download_play_wih.h"
#include "nitro/mb.h"

// The parent's own flow for the DS Download Play range of overlay 182 (src/ov182/download_play_sys.c). The ROM
// embeds no file name for it, so the name is ours; it names overlay 181's src/ov181/mb_parent_sys.c, which drives
// the same kind of flow over the MBComm_* layer, as its role counterpart. The offsets and the states are ours

// What the parent's flow is doing, the byte the flow itself and the overlay's screens file both set. The numbers
// are the original's
enum {
    DOWNLOAD_PLAY_SYS_STATE_STOP = 0x00,
    DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS = 0x01,
    DOWNLOAD_PLAY_SYS_STATE_MEASURE_CHANNEL = 0x02,
    DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT = 0x03,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_CHILDREN = 0x04,
    DOWNLOAD_PLAY_SYS_STATE_SEND_DATA = 0x05,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_SEND = 0x06,
    DOWNLOAD_PLAY_SYS_STATE_START_CONNECT = 0x07,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT = 0x08,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA = 0x09,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_KEY = 0x0a,
    DOWNLOAD_PLAY_SYS_STATE_CONNECT_FAILED = 0x0b,
    DOWNLOAD_PLAY_SYS_STATE_END_KEY = 0x0c,
    DOWNLOAD_PLAY_SYS_STATE_START_CONNECT_KEY = 0x0d,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_CONNECT_KEY = 0x0e,
    DOWNLOAD_PLAY_SYS_STATE_KEY_COMPLETE = 0x0f,
    DOWNLOAD_PLAY_SYS_STATE_END_CONNECT = 0x10,
    DOWNLOAD_PLAY_SYS_STATE_RESET = 0x11,
    DOWNLOAD_PLAY_SYS_STATE_FINISH = 0x12,
    DOWNLOAD_PLAY_SYS_STATE_START_END_CONNECT = 0x13,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT = 0x14,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END = DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT,
    DOWNLOAD_PLAY_SYS_STATE_END_WIRELESS = 0x15,
    DOWNLOAD_PLAY_SYS_STATE_START_DATA_SHARING = 0x16,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_DATA_SHARING = 0x17,
    DOWNLOAD_PLAY_SYS_STATE_START_RECEIVE = 0x18,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_RECEIVE = 0x19,
    DOWNLOAD_PLAY_SYS_STATE_RECEIVE_COMPLETE = 0x1a,
    DOWNLOAD_PLAY_SYS_STATE_DATA_SHARING_FAILED = 0x1b,
    DOWNLOAD_PLAY_SYS_STATE_END_DATA_SHARING = 0x1c,
    DOWNLOAD_PLAY_SYS_STATE_END_CHILDREN = 0x1d,
    DOWNLOAD_PLAY_SYS_STATE_END_PARENT = 0x1e,
    DOWNLOAD_PLAY_SYS_STATE_CANCEL = 0x21,
    DOWNLOAD_PLAY_SYS_STATE_SEND_COMPLETE = 0x22,
};
// How long a state that waits on the wireless waits before it gives up, in frames
#define DOWNLOAD_PLAY_SYS_TIMEOUT 30

// The size of the frames the data sharing exchanges, and the number of them whose change the flow reports
#define DOWNLOAD_PLAY_SYS_FRAME_SIZE 0x44
#define DOWNLOAD_PLAY_SYS_FRAME_NO 16

// The two ids the parent's session runs under, which the wireless takes as one word: the tgid in its first
// halfword, and the channel five bits below its top
typedef union {
    u32 word;

    struct {
        u16 tgid;
        u32 channel : 5;
    } ids;
} DownloadPlaySysIds;

// The parent's flow work, which follows the wireless helper's (WHWork) in the parent's loader context
// (DownloadPlayWork): the helper's own work is the 0xb00 bytes before this one. The frames the data sharing
// exchanges sit at the work pointer's own offsets above this struct's, because the helper's work carries them
typedef struct {
    // The frame the parent shares with the children, with the game's data at its start
    u8 dataShareFrame[DOWNLOAD_PLAY_SYS_FRAME_SIZE];
    u8 unk_044[0xbc];
    // The step of the data sharing, the code the peer reported in its frame, how many frames have been sent, and
    // which of the sixteen frames moved
    u16 dataShareStep;
    u16 peerCode;
    u32 msgCounter;
    u32 frameChanged[DOWNLOAD_PLAY_SYS_FRAME_NO];
    u8 unk_148[0x4fc];
    // What the parent's session runs under
    DownloadPlaySysIds ids;
    u8 unk_64c[0x3fc];
    // The parent parameter of each channel, which the measurement filled
    u8 unk_64c_1[0x460];
    // The state the flow is in (DOWNLOAD_PLAY_SYS_STATE_*), the state it was in, and what the caller passed
    u8 state;
    u8 prevState;
    u8 unk_a92;
    u8 host;
    u32 unk_a94;
    u16 unk_a98[2];
    // How long the current state has been in for, which the states that can time out count
    u32 timer;
    // The players the scan found, by player number less one
    u16 playerNo[2];
    // The parent parameter of each channel, which the parent connects with
    u16 *chanParam[0x10];
    // The frame the parent fills, the address of this work, and what the scan calls with each child
    u8 unk_aa8[8];
    void *unk_ab0;
    WHScanFilter scanFilter;
    // The game the children download, as the parent was handed it, and how many players it allows
    MBGameRegistry registry;
    u8 maxPlayerNum;
    u8 unk_ad1[0x33];
} DownloadPlaySysWork;

// The parent's flow work, which the function that starts the flow stores here, the way the helper keeps its own
extern DownloadPlaySysWork *gDownloadPlaySysWork;

#define DOWNLOAD_PLAY_SYS_WORK(name) DownloadPlaySysWork *name = gDownloadPlaySysWork

// What the overlay's screens file calls
void func_ov182_021ba768(DownloadPlaySysWork *work, const MBGameRegistry *registry);
BOOL func_ov182_021ba81c(void);
void func_ov182_021ba918(void);
void func_ov182_021ba974(void);
void func_ov182_021babdc(u8 *state, u8 *changed);
const MBUserInfo *func_ov182_021bac14(void);

// Not written yet: the function that fills the parent's copy of the game registry, which the overlay's unassigned
// screens range owns
u32 func_ov182_021b7e6c(void);

#endif // POKEBW2_APP_DOWNLOAD_PLAY_SYS_H
