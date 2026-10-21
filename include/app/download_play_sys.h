#ifndef POKEBW2_APP_DOWNLOAD_PLAY_SYS_H
#define POKEBW2_APP_DOWNLOAD_PLAY_SYS_H

#include "types.h"
#include "app/download_play.h"
#include "app/download_play_wih.h"
#include "nitro/mb.h"

// The parent's own flow for the DS Download Play range of overlay 182 (src/ov182/download_play_sys.c). The ROM
// embeds no file name for it, so the name is ours; it names overlay 181's src/ov181/mb_parent_sys.c, which drives
// the same kind of flow over the MBComm_* layer, as its role counterpart. The offsets and the states are ours

// What the parent's flow is doing, the byte the flow itself and the overlay's screens file both read. The numbers are
// the original's; the names say what each state does in this file
enum {
    DOWNLOAD_PLAY_SYS_STATE_NONE = 0x00,
    DOWNLOAD_PLAY_SYS_STATE_READY = 0x01,
    DOWNLOAD_PLAY_SYS_STATE_START_WIRELESS = 0x02,
    DOWNLOAD_PLAY_SYS_STATE_INIT_PARENT = 0x03,
    DOWNLOAD_PLAY_SYS_STATE_ENTRY = 0x04,
    DOWNLOAD_PLAY_SYS_STATE_CHILD_ENTERED = 0x05,
    DOWNLOAD_PLAY_SYS_STATE_SENDING = 0x06,
    DOWNLOAD_PLAY_SYS_STATE_START_CONNECT = 0x07,
    DOWNLOAD_PLAY_SYS_STATE_CONNECT = 0x08,
    DOWNLOAD_PLAY_SYS_STATE_CONNECT_PEER = 0x09,
    DOWNLOAD_PLAY_SYS_STATE_CONNECT_ASKED = 0x0a,
    DOWNLOAD_PLAY_SYS_STATE_PEER_DONE = 0x0b,
    DOWNLOAD_PLAY_SYS_STATE_DONE = 0x0c,
    DOWNLOAD_PLAY_SYS_STATE_CHILD_LEFT = 0x0d,
    DOWNLOAD_PLAY_SYS_STATE_END_PARENT = 0x10,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PARENT = 0x11,
    DOWNLOAD_PLAY_SYS_STATE_SENT = 0x12,
    // Nothing in this file moves the flow to 0x13
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_WIRELESS = 0x13,
    DOWNLOAD_PLAY_SYS_STATE_ENDED_WIRELESS = 0x14,
    DOWNLOAD_PLAY_SYS_STATE_PEER_END = 0x15,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_PEER = 0x16,
    DOWNLOAD_PLAY_SYS_STATE_ENDED_PEER = 0x17,
    DOWNLOAD_PLAY_SYS_STATE_PEER_CANCEL = 0x18,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CANCEL = 0x19,
    DOWNLOAD_PLAY_SYS_STATE_ENDED_CANCEL = 0x1a,
    DOWNLOAD_PLAY_SYS_STATE_ERROR = 0x1b,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_ERROR = 0x1c,
    DOWNLOAD_PLAY_SYS_STATE_ENDED_ERROR = 0x1d,
    DOWNLOAD_PLAY_SYS_STATE_DISCONNECT = 0x1e,
    DOWNLOAD_PLAY_SYS_STATE_FAILED = 0x1f,
    DOWNLOAD_PLAY_SYS_STATE_END_CONNECT = 0x20,
    DOWNLOAD_PLAY_SYS_STATE_WAIT_END_CONNECT = 0x21,
    DOWNLOAD_PLAY_SYS_STATE_ENDED = 0x22,
};

// What the overlay's screens file asks of the flow once it is back at READY (endRequest)
enum {
    DOWNLOAD_PLAY_SYS_REQUEST_NONE,
    DOWNLOAD_PLAY_SYS_REQUEST_RESTART,
    DOWNLOAD_PLAY_SYS_REQUEST_END,
};

// How long a state that ends the wireless waits before it asks the wireless for its state, in frames
#define DOWNLOAD_PLAY_SYS_TIMEOUT 30

// The frames the data sharing exchanges, and the number of slots whose change the flow keeps
#define DOWNLOAD_PLAY_SYS_FRAME_NO 16

// A frame of the data sharing: the code that says what the sender wants, a word that goes with it, and the data
typedef struct {
    u16 code;
    u16 arg;
    u8 data[0x40];
} DownloadPlaySysFrame;

// The parent's flow work, the first 0xb00 bytes of the block the overlay's screens file hands the flow: the
// download-play parent's work (DownloadPlayWork) follows it
typedef struct {
    // The frame this console sends, and the frames the data sharing received from the two consoles
    DownloadPlaySysFrame sendFrame;
    u8 unk_044[0xbc];
    DownloadPlaySysFrame recvFrames[2];
    u8 unk_188[0x78];
    // The step of the data sharing, the code a child answers with, and how many frames have gone unanswered
    u16 dataShareStep;
    u16 childCode;
    u32 msgCounter;
    // Which of the received frames moved
    u32 frameChanged[DOWNLOAD_PLAY_SYS_FRAME_NO];
    u8 unk_248[0x400];
    // The session's own id and the channel the measurement chose
    u16 tgid;
    u16 channel;
    // The console's backup pages, which the start reads
    u8 backupPages[0x404];
    // The first 0x40 bytes of the data the parent sends, copied when the wireless comes up
    u8 gameData[0x40];
    // The state the flow is in (DOWNLOAD_PLAY_SYS_STATE_*) and the state it was in at the last poll
    u8 state;
    u8 prevState;
    // The byte the registry's player count held, which a child sends; whether this console leads the exchange
    u8 playerCount;
    u8 host;
    // How many of the peer's go-on codes the host has counted, and how many frames the entry has been waiting
    u32 hostCount;
    u32 entryFrames;
    // How long the current state has waited, which the states that end the wireless count
    u32 timer;
    // The child that connected, by player number less one
    DownloadPlayChildInfo *childInfo[DOWNLOAD_PLAY_CHILD_MAX];
    // The data the parent sends, 0x40 bytes per step of the data sharing
    u8 *sendData;
    u8 unk_aa8[4];
    // What the screens file asked of the flow (DOWNLOAD_PLAY_SYS_REQUEST_*)
    u8 endRequest;
    u8 unk_aad[3];
    // The frame the data sharing sends, which is this work's own sendFrame
    DownloadPlaySysFrame *sendFramep;
    // The game the children download, as the parent was handed it
    MBGameRegistry registry;
    u8 unk_af0[0x10];
} DownloadPlaySysWork;

// The block the overlay's screens file hands the flow: the flow's own work, then the download-play parent's
typedef struct {
    DownloadPlaySysWork sys;
    DownloadPlayWork play;
} DownloadPlaySysContext;

// The game that the overlay's screens file hands the flow to offer: an MBGameRegistry up to its player count, without
// the user parameters
typedef struct {
    const char *romFilePathp;
    u16 *gameNamep;
    u16 *gameIntroductionp;
    const char *iconCharPathp;
    const char *iconPalettePathp;
    u32 ggid;
    u8 maxPlayerNum;
    u8 pad[3];
} DownloadPlaySysGame;

// What the overlay's screens file calls
void func_ov182_021ba768(DownloadPlaySysContext *context, const DownloadPlaySysGame *registry);
BOOL func_ov182_021ba81c(void);
BOOL func_ov182_021ba918(void);
BOOL func_ov182_021ba950(void);
void func_ov182_021ba974(void);
void func_ov182_021babdc(u8 *state, u8 *changed);
const MBUserInfo *func_ov182_021bac14(void);

// Not written yet: the function that fills the parent's copy of the game registry, which the overlay's unassigned
// screens range owns
u8 *DWCUtilWifiSettings_GetPages(void);

#endif // POKEBW2_APP_DOWNLOAD_PLAY_SYS_H
