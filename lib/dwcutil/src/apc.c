#include "apc.h"
#include "dwc/wcm.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#ifdef PLATFORM_PC
#include "port_wcm.h"

// The DS backend's active work grows with host pointers; the larger DSi union tail is not used here.
#define DWC_WCM_WORK_SIZE PORT_WCM_DS_WORK_SIZE
#else
#define DWC_WCM_WORK_SIZE WCM_WORK_SIZE
#endif

// The wrapper that DWC's settings put around the wireless connection manager (WCM, see wcm.h): one operation runs at a
// time, a state says which, and a callback is told how each ends. The file is the library's apc.o
// (libdwcutil.TWL.HYB.a), which aletheia's map of Black 1's SDK code to the library's objects (layout/symbols_sdk.txt)
// shows with the same nine functions in the same order. The ROM names none of it; the names are the DWC utility
// library's own, as that library's symbols and debug information have them, where the code shows them, and ours
// elsewhere.

// Where the wrapper is
enum {
    STATE_OFF,         // not initialized
    STATE_INITIALIZED, // the work is set up, the WCM is not started
    STATE_CLEANING_UP,
    STATE_IDLE,     // started, doing nothing
    STATE_STOPPING, // going to idle: the start, the end of a search or a disconnect is under way
    STATE_SEARCHING,
    STATE_SEARCH_STARTING,
    STATE_CONNECTED,
    STATE_CONNECTING,
};

typedef struct {
    void *wcmWork;           // 0x00
    u32 searchOption;        // 0x04
    WCMConfig *config;       // 0x08
    u32 connectOption;       // 0x0c, passed to WCM_UpdateOption during connection setup
    const u8 *bssid;         // 0x10
    const u8 *essid;         // 0x14
    int state;               // 0x18
    DWCWcmCallback callback; // 0x1c
    void *bssDesc;           // 0x20
    void *wepDesc;           // 0x24
} WcmWrapper;

// MWCC 2.0 lays the statics out in reverse
static u8 sEssid[0x20];
static u8 sBssid[6];
static WcmWrapper sWcm;

static void APCi_Notify(WCMNotice *notice);
static BOOL APCi_MoveIdleState(void);

static void APCi_Notify(WCMNotice *notice) {
    if (notice == NULL) {
        return;
    }
    switch (notice->notify) {
    case WCM_NOTIFY_STARTUP:
        if (notice->result == 0) {
            if (sWcm.state == STATE_STOPPING) {
                sWcm.state = STATE_IDLE;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_STARTED, 0);
                }
            } else if (sWcm.state == STATE_SEARCH_STARTING) {
                if (WCM_SearchAsync(sWcm.bssid, sWcm.essid, sWcm.searchOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else if (sWcm.state == STATE_CONNECTING) {
                if (WCM_ConnectAsync(sWcm.bssDesc, sWcm.wepDesc, sWcm.connectOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            }
        } else {
            sWcm.state = STATE_INITIALIZED;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_BEGIN_SEARCH:
        if (notice->result == 0) {
            if (sWcm.state == STATE_SEARCH_STARTING) {
                sWcm.state = STATE_SEARCHING;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_SEARCHING, 0);
                }
            }
        } else {
            sWcm.state = STATE_IDLE;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_SEARCH_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_CONNECT:
        if (notice->result == 0) {
            if (sWcm.state == STATE_CONNECTING) {
                sWcm.state = STATE_CONNECTED;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_CONNECTED, 0);
                }
            }
        } else {
            sWcm.state = STATE_IDLE;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_CONNECT_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_END_SEARCH:
        if (notice->result == 0) {
            if (sWcm.state == STATE_STOPPING) {
                sWcm.state = STATE_IDLE;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_SEARCH_ENDED, 0);
                }
            } else if (sWcm.state == STATE_SEARCH_STARTING) {
                if (WCM_SearchAsync(sWcm.bssid, sWcm.essid, sWcm.searchOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else if (sWcm.state == STATE_CLEANING_UP) {
                if (WCM_CleanupAsync() != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else if (sWcm.state == STATE_CONNECTING) {
                if (WCM_ConnectAsync(sWcm.bssDesc, sWcm.wepDesc, sWcm.connectOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            }
        } else {
            sWcm.state = STATE_IDLE;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_END_SEARCH_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_DISCONNECT:
        if (notice->result == 0) {
            if (sWcm.state == STATE_STOPPING) {
                sWcm.state = STATE_IDLE;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_DISCONNECTED, 0);
                }
            } else if (sWcm.state == STATE_SEARCH_STARTING) {
                if (WCM_SearchAsync(sWcm.bssid, sWcm.essid, sWcm.searchOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else if (sWcm.state == STATE_CLEANING_UP) {
                if (WCM_CleanupAsync() != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else if (sWcm.state == STATE_CONNECTING) {
                if (WCM_ConnectAsync(sWcm.bssDesc, sWcm.wepDesc, sWcm.connectOption) != WCM_RESULT_ACCEPT) {
                    sWcm.state = STATE_IDLE;
                    if (sWcm.callback != NULL) {
                        sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
                    }
                }
            } else {
                sWcm.state = STATE_IDLE;
            }
        } else {
            sWcm.state = STATE_IDLE;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_DISCONNECT_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_CLEANUP:
        if (notice->result == 0) {
            if (sWcm.state == STATE_CLEANING_UP) {
                WCM_Finish();
                sWcm.state = STATE_OFF;
                if (sWcm.callback != NULL) {
                    sWcm.callback(DWC_WCM_EVENT_CLEANED_UP, 0);
                }
            }
        } else {
            sWcm.state = STATE_IDLE;
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_FAILED, 0);
            }
        }
        break;
    case WCM_NOTIFY_FOUND_AP:
        if (sWcm.state == STATE_SEARCHING) {
            if (sWcm.callback != NULL) {
                sWcm.callback(DWC_WCM_EVENT_FOUND_AP, 0);
            }
        }
        break;
    default:
        if (sWcm.callback != NULL) {
            sWcm.callback(DWC_WCM_EVENT_UNKNOWN, 0);
        }
        break;
    }
}

// Stops what the state says is under way, and reports whether the WCM took it
static BOOL APCi_MoveIdleState(void) {
    switch (sWcm.state) {
    case STATE_SEARCHING:
        if (WCM_SearchAsync(NULL, NULL, 0) != WCM_RESULT_ACCEPT) {
            return FALSE;
        }
        break;
    case STATE_CONNECTED:
        if (WCM_DisconnectAsync() != WCM_RESULT_ACCEPT) {
            return FALSE;
        }
        break;
    case STATE_INITIALIZED:
        if (WCM_StartupAsync(sWcm.config, APCi_Notify) != WCM_RESULT_ACCEPT) {
            return FALSE;
        }
        break;
    case STATE_OFF:
    case STATE_CLEANING_UP:
    case STATE_IDLE:
    case STATE_STOPPING:
    case STATE_SEARCH_STARTING:
    case STATE_CONNECTING:
    default:
        return FALSE;
    }
    return TRUE;
}

int APC_GetAPList(void *dest, int max) {
    u8 *out = dest;
    int count;
    int i;

    WCM_LockApList(TRUE);
    count = WCM_CountApList();
    if (count > 0) {
        for (i = 0; i < count; i++) {
            if (i >= max) {
                break;
            }
            MIi_CpuCopy32(WCM_PointApList(i), out, 0xc0);
            out += 0xc0;
        }
    }
    WCM_LockApList(FALSE);
    return count;
}

BOOL APC_SearchStart(const u8 *bssid, const u8 *essid, int essidLength, u32 option) {
    u32 irq = OS_DisableInterrupts();
    int i;

    sWcm.searchOption = option;
    if (bssid != NULL) {
        for (i = 0; i < 6; i++) {
            sBssid[i] = *bssid++;
        }
        sWcm.bssid = sBssid;
    } else {
        MI_CpuFill8(sBssid, 0xff, 6);
        sWcm.bssid = WCM_Bssid_Any;
    }
    if (essid != NULL && essidLength > 0 && essidLength <= 0x20) {
        for (i = 0; i < essidLength; i++) {
            sEssid[i] = *essid++;
        }
        for (; i < 0x20; i++) {
            sEssid[i] = 0;
        }
        sWcm.essid = sEssid;
    } else {
        MI_CpuFill8(sEssid, 0xff, 0x20);
        sWcm.essid = WCM_Essid_Any;
    }
    if (sWcm.state == STATE_IDLE) {
        if (WCM_SearchAsync(sWcm.bssid, sWcm.essid, sWcm.searchOption) == WCM_RESULT_ACCEPT) {
            sWcm.state = STATE_SEARCH_STARTING;
            OS_RestoreInterrupts(irq);
            return TRUE;
        }
    } else if (APCi_MoveIdleState() == TRUE) {
        sWcm.state = STATE_SEARCH_STARTING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

BOOL APC_SearchEnd(void) {
    u32 irq = OS_DisableInterrupts();

    if (sWcm.state == STATE_SEARCHING && WCM_SearchAsync(NULL, NULL, 0) == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_STOPPING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

BOOL APC_Disconnect(void) {
    u32 irq = OS_DisableInterrupts();

    if (sWcm.state == STATE_CONNECTED && WCM_DisconnectAsync() == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_STOPPING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

BOOL APC_End(void) {
    u32 irq = OS_DisableInterrupts();

    if (sWcm.state == STATE_IDLE) {
        if (WCM_CleanupAsync() != WCM_RESULT_ACCEPT) {
            OS_RestoreInterrupts(irq);
            return FALSE;
        }
        sWcm.state = STATE_CLEANING_UP;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    if (APCi_MoveIdleState() == TRUE) {
        sWcm.state = STATE_CLEANING_UP;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

BOOL APC_Connect(const void *bssDesc, const void *wepDesc, u32 option) {
    u32 irq = OS_DisableInterrupts();

    sWcm.connectOption = option;
    if (wepDesc != NULL) {
        MI_CpuCopy8(wepDesc, sWcm.wepDesc, 0x50);
    } else {
        MI_CpuFill8(sWcm.wepDesc, 0, 0x50);
    }
    MIi_CpuCopy32(bssDesc, sWcm.bssDesc, 0xc0);
    if (APCi_MoveIdleState() == TRUE) {
        sWcm.state = STATE_CONNECTING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    if (sWcm.state == STATE_IDLE &&
        WCM_ConnectAsync(sWcm.bssDesc, sWcm.wepDesc, sWcm.connectOption) == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_CONNECTING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}

BOOL APC_Init(DWCWcmCallback callback, void *work, u32 size) {
    u32 irq = OS_DisableInterrupts();
    u8 *buf = work;

    sWcm.wepDesc = buf;
#ifdef PLATFORM_PC
    // WCMConfig contains a pointer, so its address needs the host's pointer alignment.
    sWcm.config = (WCMConfig *)(((uptr)buf + 0x50 + sizeof(uptr) - 1) & ~(sizeof(uptr) - 1));
#else
    sWcm.config = (WCMConfig *)(((u32)buf + 0x53) & ~3);
#endif
    sWcm.wcmWork = (void *)(((uptr)sWcm.config + sizeof(WCMConfig) + 0x1f) & ~0x1f);
    sWcm.bssDesc = (void *)(((uptr)sWcm.wcmWork + DWC_WCM_WORK_SIZE + 0x1f) & ~0x1f);
#ifdef PLATFORM_PC
    // The caller owns this buffer; reject a layout that would extend beyond it or a signed SDK byte count.
    if (work == NULL || (uptr)sWcm.bssDesc + 0xc0 + 0x1f < (uptr)buf ||
        (((uptr)sWcm.bssDesc + 0xc0 + 0x1f) & ~(uptr)0x1f) - (uptr)buf > size || size > 0x7fffffff) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
#endif
    sWcm.config->pbdbuffer = (void *)(((uptr)sWcm.bssDesc + 0xc0 + 0x1f) & ~0x1f);
    sWcm.config->nbdbuffer = (sptr)(buf + size) - (sptr)sWcm.config->pbdbuffer;
    sWcm.config->nbdmode = 0;
    sWcm.config->dmano = 3;
    sWcm.callback = callback;
    if (sWcm.state == STATE_OFF) {
        if (WCM_Init(sWcm.wcmWork, DWC_WCM_WORK_SIZE) != 0) {
            OS_RestoreInterrupts(irq);
            return FALSE;
        }
        sWcm.state = STATE_INITIALIZED;
    }
    if (sWcm.state == STATE_INITIALIZED) {
        if (WCM_StartupAsync(sWcm.config, APCi_Notify) != WCM_RESULT_ACCEPT) {
            OS_RestoreInterrupts(irq);
            return FALSE;
        }
        sWcm.state = STATE_STOPPING;
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    OS_RestoreInterrupts(irq);
    return FALSE;
}
