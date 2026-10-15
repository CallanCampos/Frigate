#include "dwc_wcm.h"
#include "dwc/wcm.h"
#include "gfl/std.h"
#include "nitro/os.h"

// The wrapper that DWC's settings put around the wireless connection manager (WCM, see wcm.h): one operation runs at a
// time, a state says which, and a callback is told how each ends. The ROM names none of it; the names are ours.

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
    u32 connectOption;       // 0x0c, passed to WCM_SetSearchOption during connection setup
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

static void WcmNotify(WCMNotice *notice);
static BOOL WcmAbort(void);

static void WcmNotify(WCMNotice *notice) {
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
static BOOL WcmAbort(void) {
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
        if (WCM_StartupAsync(sWcm.config, WcmNotify) != WCM_RESULT_ACCEPT) {
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

int DWC_Wcm_GetApList(void *dest, int max) {
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
            sys_memcpy32(WCM_PointApList(i), out, 0xc0);
            out += 0xc0;
        }
    }
    WCM_LockApList(FALSE);
    return count;
}

BOOL DWC_Wcm_Search(const u8 *bssid, const u8 *essid, int essidLength, u32 option) {
    u32 irq = CPU_IRQDisable();
    int i;

    sWcm.searchOption = option;
    if (bssid != NULL) {
        for (i = 0; i < 6; i++) {
            sBssid[i] = *bssid++;
        }
        sWcm.bssid = sBssid;
    } else {
        sys_memset(sBssid, 0xff, 6);
        sWcm.bssid = WCMiAnyBssid;
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
        sys_memset(sEssid, 0xff, 0x20);
        sWcm.essid = WCMiAnyEssid;
    }
    if (sWcm.state == STATE_IDLE) {
        if (WCM_SearchAsync(sWcm.bssid, sWcm.essid, sWcm.searchOption) == WCM_RESULT_ACCEPT) {
            sWcm.state = STATE_SEARCH_STARTING;
            CPU_SetIRQMask(irq);
            return TRUE;
        }
    } else if (WcmAbort() == TRUE) {
        sWcm.state = STATE_SEARCH_STARTING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

BOOL DWC_Wcm_EndSearch(void) {
    u32 irq = CPU_IRQDisable();

    if (sWcm.state == STATE_SEARCHING && WCM_SearchAsync(NULL, NULL, 0) == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_STOPPING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

BOOL DWC_Wcm_Disconnect(void) {
    u32 irq = CPU_IRQDisable();

    if (sWcm.state == STATE_CONNECTED && WCM_DisconnectAsync() == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_STOPPING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

BOOL DWC_Wcm_Cleanup(void) {
    u32 irq = CPU_IRQDisable();

    if (sWcm.state == STATE_IDLE) {
        if (WCM_CleanupAsync() != WCM_RESULT_ACCEPT) {
            CPU_SetIRQMask(irq);
            return FALSE;
        }
        sWcm.state = STATE_CLEANING_UP;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    if (WcmAbort() == TRUE) {
        sWcm.state = STATE_CLEANING_UP;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

BOOL DWC_Wcm_Connect(const void *bssDesc, const void *wepDesc, u32 option) {
    u32 irq = CPU_IRQDisable();

    sWcm.connectOption = option;
    if (wepDesc != NULL) {
        sys_memcpy(wepDesc, sWcm.wepDesc, 0x50);
    } else {
        sys_memset(sWcm.wepDesc, 0, 0x50);
    }
    sys_memcpy32(bssDesc, sWcm.bssDesc, 0xc0);
    if (WcmAbort() == TRUE) {
        sWcm.state = STATE_CONNECTING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    if (sWcm.state == STATE_IDLE &&
        WCM_ConnectAsync(sWcm.bssDesc, sWcm.wepDesc, sWcm.connectOption) == WCM_RESULT_ACCEPT) {
        sWcm.state = STATE_CONNECTING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}

BOOL DWC_Wcm_Start(DWCWcmCallback callback, void *work, u32 size) {
    u32 irq = CPU_IRQDisable();
    u8 *buf = work;

    sWcm.wepDesc = buf;
    sWcm.config = (WCMConfig *)(((u32)buf + 0x53) & ~3);
    sWcm.wcmWork = (void *)(((u32)sWcm.config + sizeof(WCMConfig) + 0x1f) & ~0x1f);
    sWcm.bssDesc = (void *)(((u32)sWcm.wcmWork + WCM_WORK_SIZE + 0x1f) & ~0x1f);
    sWcm.config->pbdbuffer = (void *)(((u32)sWcm.bssDesc + 0xc0 + 0x1f) & ~0x1f);
    sWcm.config->nbdbuffer = (s32)(buf + size) - (s32)sWcm.config->pbdbuffer;
    sWcm.config->nbdmode = 0;
    sWcm.config->dmano = 3;
    sWcm.callback = callback;
    if (sWcm.state == STATE_OFF) {
        if (WCM_Init(sWcm.wcmWork, WCM_WORK_SIZE) != 0) {
            CPU_SetIRQMask(irq);
            return FALSE;
        }
        sWcm.state = STATE_INITIALIZED;
    }
    if (sWcm.state == STATE_INITIALIZED) {
        if (WCM_StartupAsync(sWcm.config, WcmNotify) != WCM_RESULT_ACCEPT) {
            CPU_SetIRQMask(irq);
            return FALSE;
        }
        sWcm.state = STATE_STOPPING;
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    CPU_SetIRQMask(irq);
    return FALSE;
}
