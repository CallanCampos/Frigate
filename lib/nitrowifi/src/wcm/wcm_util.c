#include "nitro/os.h"
#include "nitro/wm.h"
#include "nitrowifi/wcm.h"
#include "wcm_internal.h"

// NitroWiFi's WCM (overlay 11): the comparison of BSSIDs and the link level, from a running average of the signal.
// TwlSDK's libwcm names the object util.o; Black 1's decompilation (aletheia) has the same functions, in this order, in
// its wcm_util.c, and the names are the SDK's.

// The last 16 signal samples, a ring, and the count of samples until it is full, then 16 plus the next index
static u8 sWcmRssiSamples[16];
static u8 sWcmRssiCursor = 0;

static BOOL WcmCompareMemory(const void *a, const void *b, u32 size);
static WMLinkLevel WCM_CalcLinkLevel(u32 rssi);
static u8 WCMi_GetRssiAverage(void);

static BOOL WcmCompareMemory(const void *a, const void *b, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (((const u8 *)a)[i] != ((const u8 *)b)[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL WCM_CompareBssID(const u8 *a, const u8 *b) {
    return WcmCompareMemory(a, b, 6);
}

WMLinkLevel WCM_GetLinkLevel(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCMi_GetSystemWork();
    WMLinkLevel quality = WM_LINK_LEVEL_0;

    if (work != NULL && work->state == 9) {
        quality = WCM_CalcLinkLevel(WCMi_GetRssiAverage());
    }
    OS_RestoreInterrupts(irq);
    return quality;
}

static WMLinkLevel WCM_CalcLinkLevel(u32 rssi) {
    switch (WCMi_GetSystemWork()->wirelessMode) {
    case 1:
    default:
        if (rssi >= 0x1c) {
            return WM_LINK_LEVEL_3;
        }
        if (rssi >= 0x16) {
            return WM_LINK_LEVEL_2;
        }
        if (rssi >= 0x10) {
            return WM_LINK_LEVEL_1;
        }
        return WM_LINK_LEVEL_0;
    case 2:
        if (rssi >= 0x16) {
            return WM_LINK_LEVEL_3;
        }
        if (rssi >= 0x11) {
            return WM_LINK_LEVEL_2;
        }
        if (rssi >= 0xc) {
            return WM_LINK_LEVEL_1;
        }
        return WM_LINK_LEVEL_0;
    }
}

static u8 WCMi_GetRssiAverage(void) {
    s32 sum = 0;
    s32 i;

    if (sWcmRssiCursor > 16) {
        for (i = 0; i < 16; i++) {
            sum += sWcmRssiSamples[i];
        }
        sum /= 16;
    } else if (sWcmRssiCursor > 0) {
        for (i = 0; i < sWcmRssiCursor; i++) {
            sum += sWcmRssiSamples[i];
        }
        sum /= sWcmRssiCursor;
    }
    return (u8)sum;
}

// Adds a signal-level sample to the running RSSI average's circular buffer
// Decodes the receive record's high signal byte. The original arithmetic is mode-dependent;
// no physical units for the encoded input or the resulting sample have been established.
void WCMi_ShelterRssi(s32 rssi) {
    switch (WCMi_GetSystemWork()->wirelessMode) {
    case 2:
        break;
    case 1:
    default:
        if (rssi & 2) {
            rssi = (u8)(rssi >> 2);
        } else {
            rssi = (u8)(25 + (rssi >> 2));
        }
        break;
    }
    sWcmRssiSamples[sWcmRssiCursor % 16] = rssi;
    if (sWcmRssiCursor >= 16) {
        sWcmRssiCursor = (u8)(((1 + sWcmRssiCursor) % 16) + 16);
    } else {
        sWcmRssiCursor++;
    }
}
