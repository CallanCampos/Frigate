#ifndef POKEBW2_APP_DOWNLOAD_PLAY_WIH_H
#define POKEBW2_APP_DOWNLOAD_PLAY_WIH_H

#include "types.h"
#include "app/download_play.h"

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
};

// What runs as the helper moves between its states, which is given the state it moves to
typedef void (*WHStateCallback)(int state);

// The helper's work, which the DS Download Play parent allocates: DownloadPlayWork's loaderContext is one of these,
// and the parent's own state fills the gap between the two ends of it
typedef struct {
    // What runs for the state that it moves to (data_ov182_021c18ac)
    WHStateCallback *stateCallbacks;
    // The parent's state (its DownloadPlayState) and the rest of what the helper does not read
    u8 unk_04[0x3c];
    // The state the helper is in (WIH_STATE_*)
    int sysState;
    u8 unk_44[0x10];
    // The error that was reported first
    int lastError;
    // The value the channel is chosen from, which each measurement advances
    u32 seed;
    u16 channel;
    // The least busy that a channel was, and the channels that were as little busy
    u16 minBusy;
    u16 bestBitmap;
    u8 unk_62[0x1346];
    // The three words at the end of the work, which the parent's file leaves to the helper to clear
    u32 unk_13a8;
    u32 unk_13ac;
    u32 unk_13b0;
} WHWork;

// The helper's work, and the word before it, which the file's functions reach the helper's work through
typedef struct {
    u32 unk_00;
    WHWork *work;
} WHWorkPointer;

extern WHWorkPointer gWork;

// The two functions of the file's first bytes, which are not written yet: the state change, which runs the
// stateCallbacks entry of the state it moves to, and the error report, which keeps the first error until the state is
// an error
void func_ov182_021b9be4(int state);
void func_ov182_021b9c34(int errcode);

#endif // POKEBW2_APP_DOWNLOAD_PLAY_WIH_H
