#include "app/download_play_wih.h"
#include "nitro/os.h"
#include "nitro/wm.h"

// download_play_wih.c is the wireless helper of the DS Download Play range of overlay 182: it measures how busy each
// channel is, one after the other, and keeps the ones that were least busy

static u16 func_ov182_021ba238(u16 channel);
static void func_ov182_021ba2a8(void *arg);
static int func_ov182_021ba33c(WMCallbackFunc callback, u16 channel);

// Takes the work that the parent's DownloadPlay_SetWork hands over and starts it out at state 0, with the three words
// at the end of it cleared
void func_ov182_021b9bbc(void *loaderContext) {
    gWork.work = loaderContext;
    gWork.work->sysState = WIH_STATE_STOP;
    gWork.work->unk_13a8 = 0;
    gWork.work->unk_13ac = 0;
    gWork.work->unk_13b0 = 0;
}

// Measures how busy each channel is, one after the other. FALSE when the measurement cannot start
BOOL func_ov182_021ba1a8(void) {
    u16 mac[3];
    u16 result;

    OS_GetMacAddress((u8 *)mac);
    gWork.work->seed = OS_GetVBlankCount() + mac[0] + mac[1] + mac[2];
    gWork.work->seed = gWork.work->seed * 69069 + 12345;
    gWork.work->channel = 0;
    gWork.work->minBusy = 101;
    func_ov182_021b9be4(WIH_STATE_BUSY);
    result = func_ov182_021ba238(1);
    if (result == 0x18) {
        func_ov182_021b9c34(0x18);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    if (result != WM_ERRCODE_OPERATING) {
        func_ov182_021b9c34(result);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return FALSE;
    }
    return TRUE;
}

// Measures the next channel that is allowed, from the one given. 0x18 once there are none left
static u16 func_ov182_021ba238(u16 channel) {
    u16 allowed = WM_GetAllowedChannel();

    if (allowed == 0x8000) {
        func_ov182_021b9c34(3);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return 3;
    }
    if (allowed == 0) {
        func_ov182_021b9c34(0x16);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return 0x18;
    }
    if (!((1 << (channel - 1)) & allowed)) {
        do {
            channel++;
            if (channel > 16) {
                return 0x18;
            }
        } while (!((1 << (channel - 1)) & allowed));
    }
    return func_ov182_021ba33c(func_ov182_021ba2a8, channel);
}

// What the measure of a channel calls: keeps the least busy channel, and goes on to the next one
static void func_ov182_021ba2a8(void *arg) {
    WMMeasureChannelCallback *cb = arg;
    u16 channel;
    u16 busy;
    u16 next;

    if (cb->errcode != WM_ERRCODE_SUCCESS) {
        func_ov182_021b9c34(cb->errcode);
        func_ov182_021b9be4(WIH_STATE_ERROR);
        return;
    }
    channel = cb->channel;
    busy = cb->ccaBusyRatio;
    if (cb->unk_0c != NULL) {
        cb->unk_0c(channel, busy);
    }
    if (gWork.work->minBusy > busy) {
        gWork.work->minBusy = busy;
        gWork.work->bestBitmap = 1 << (channel - 1);
    } else if (gWork.work->minBusy == busy) {
        gWork.work->bestBitmap |= 1 << (channel - 1);
    }
    next = func_ov182_021ba238(channel + 1);
    if (next == 0x18 && gWork.work->sysState != WIH_STATE_ERROR) {
        func_ov182_021b9be4(WIH_STATE_MEASURECHANNEL);
        return;
    }
    if (next != WM_ERRCODE_OPERATING) {
        func_ov182_021b9be4(WIH_STATE_ERROR);
    }
}

static int func_ov182_021ba33c(WMCallbackFunc callback, u16 channel) {
    return WM_MeasureChannel(callback, 3, 0x11, channel, 30);
}
