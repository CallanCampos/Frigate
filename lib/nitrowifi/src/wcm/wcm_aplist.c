#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"
#include "twl/nwm.h"
#include "wcm_internal.h"
#include <stddef.h>

// NitroWiFi's WCM (overlay 11): the list of the access points that searches found, in the buffer that
// WCM_StartupAsync's caller gives. TwlSDK's libwcm names the object aplist.o; Black 1's decompilation (aletheia) has
// the same functions, in this order, in its wcm_aplist.c, and the names are the SDK's. The ARM word pair after
// WCMi_GetGameInfo is the linker's long-branch veneer to NWM_GetVenderInfoElements in the LTD autoload.

// A vendor OUI and the type byte after it, which WCMi_GetGameInfo copies as a whole
typedef struct WCMVendorOui {
    u8 bytes[4];
} WCMVendorOui;

// Nintendo's vendor OUI (00:09:BF) and type 0, which WCMi_GetGameInfo looks for among a beacon's vendor-specific
// information elements
static const WCMVendorOui sNintendoOui = { { 0x00, 0x09, 0xbf, 0x00 } };

static WMBssDesc *WCM_PointApListEx(u16 index);
static WCMApList *WcmAllocApList(void);
static WCMApList *WcmGetOldestApList(void);
static WCMApList *WcmSearchApList(const u8 *bssid);
static WCMApList *WcmSearchIndexedApList(u32 id);
static void WcmAppendApList(WCMApList *entry);
static void *WCMi_GetGameInfo(const WMBssDesc *bss);

void WCM_ClearApList(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCMi_GetSystemWork();

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return;
    }
    if (work->apBuffer != NULL && (s32)work->apBufferBytes > 0) {
        MI_CpuFill8(work->apBuffer, 0, work->apBufferBytes);
    }
    OS_RestoreInterrupts(irq);
}

s32 WCM_CountApList(void) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCMi_GetSystemWork();
    s32 count = 0;
    WCMApListHeader *header;

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    header = (WCMApListHeader *)work->apBuffer;
    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        count = header->count;
    }
    OS_RestoreInterrupts(irq);
    return count;
}

s32 WCM_LockApList(s32 lock) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCMi_GetSystemWork();

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    if (lock != 0) {
        lock = work->apListLocked ? 1 : 0;
        work->apListLocked = 1;
    } else {
        lock = work->apListLocked ? 1 : 0;
        work->apListLocked = 0;
    }
    OS_RestoreInterrupts(irq);
    return lock;
}

WMBssDesc *WCM_PointApList(u16 index) {
    return WCM_PointApListEx(index);
}

static WMBssDesc *WCM_PointApListEx(u16 index) {
    u32 irq = OS_DisableInterrupts();
    WCMWork *work = WCMi_GetSystemWork();
    WCMApList *entry;

    if (work == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    entry = WcmSearchIndexedApList(index);
    if (entry == NULL) {
        OS_RestoreInterrupts(irq);
        return NULL;
    }
    OS_RestoreInterrupts(irq);
    return (WMBssDesc *)entry->data;
}

void WCMi_EntryApList(const WMBssDesc *bss, u16 scanLinkLevel) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApList *entry;

    if (work == NULL || work->apListLocked != 0) {
        return;
    }
    if (OS_IsRunOnTwl() && work->wirelessMode == 2 && WCMi_GetGameInfo(bss) != NULL) {
        return;
    }
    if (work->wirelessMode == 1 && bss->gameInfoLength != 0) {
        return;
    }
    entry = WcmSearchApList(bss->bssid);
    if (entry == NULL) {
        entry = WcmAllocApList();
    }
    if (entry == NULL && work->scanMode == 1) {
        entry = WcmGetOldestApList();
    }
    if (entry != NULL) {
        WCMi_BssDescCopy(bss, (WMBssDesc *)entry->data);
        entry->scanLinkLevel = scanLinkLevel;
        WcmAppendApList(entry);
    }
}

static WCMApList *WcmAllocApList(void) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    s32 i;
    WCMApList *entries;
    WCMApList *entry = NULL;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        u32 max = (work->apBufferBytes - sizeof(WCMApListHeader)) / sizeof(WCMApList);

        if (max > 0) {
            if (max > header->count) {
                i = 0;
                if (i < max) {
                    entries = header->entries;
                    do {
                        entry = entries + i;
                        if (!entry->inUse) {
                            break;
                        }
                        i++;
                    } while (i < max);
                }
                if (i < max) {
                    entry->inUse = TRUE;
                    entry->id = header->count;
                    entry->next = NULL;
                    entry->prev = header->tail;
                    header->tail = entry;
                    if (entry->prev != NULL) {
                        entry->prev->next = entry;
                    } else {
                        header->head = entry;
                    }
                    header->count++;
                }
            }
        }
    }
    return entry;
}

static WCMApList *WcmGetOldestApList(void) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        return header->head;
    }
    return NULL;
}

static WCMApList *WcmSearchApList(const u8 *bssid) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApList *entry = NULL;

    if (bssid == NULL) {
        return NULL;
    }
    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            if (WCM_CompareBssID(((WMBssDesc *)entry->data)->bssid, bssid)) {
                break;
            }
        }
    }
    return entry;
}

static WCMApList *WcmSearchIndexedApList(u32 id) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApList *entry = NULL;

    if (header != NULL && work->apBufferBytes > sizeof(WCMApListHeader)) {
        for (entry = header->head; entry != NULL; entry = entry->next) {
            if (entry->id == id) {
                break;
            }
        }
    }
    return entry;
}

static void WcmAppendApList(WCMApList *entry) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMApListHeader *header = (WCMApListHeader *)work->apBuffer;
    WCMApList *walk;

    if (entry == NULL) {
        return;
    }
    if (header == NULL || work->apBufferBytes <= sizeof(WCMApListHeader)) {
        return;
    }
    walk = header->head;
    if (walk != NULL) {
        do {
            if (walk == entry) {
                if (walk->prev != NULL) {
                    walk->prev->next = walk->next;
                } else {
                    header->head = walk->next;
                }
                if (walk->next != NULL) {
                    walk->next->prev = walk->prev;
                } else {
                    header->tail = walk->prev;
                }
                break;
            }
            walk = walk->next;
        } while (walk != NULL);
    }
    // Append (or re-append) at the tail
    entry->next = NULL;
    entry->prev = header->tail;
    header->tail = entry;
    if (entry->prev != NULL) {
        entry->prev->next = entry;
    } else {
        header->head = entry;
    }
    // A fresh entry, never in the list, gets a new id; one found and moved keeps its id
    if (walk == NULL) {
        entry->id = header->count;
        header->count++;
    }
}

// Copies the descriptor prefix and retains selected IEs in the destination's raw 0x40..0xbf tail.
void WCMi_BssDescCopy(const WMBssDesc *source, WMBssDesc *destination) {
    WCMWork *work;
    const u8 *ie;
    u8 *output;
    u8 total;
    u16 n;
    u8 tag;
    u8 len;

    work = WCMi_GetSystemWork();
    MIi_CpuClearFast(0, destination, sizeof(WMBssDesc));
    MIi_CpuCopyFast(source, destination, offsetof(WMBssDesc, gameInfo));
    destination->otherElementCount = 0;
    if (work->wirelessMode == 1) {
        destination->length = 0x20;
    } else {
        destination->length = 0x40;
    }
    total = 0;
    {
        // Microsoft's vendor OUI (00:50:F2) with the WPA type 1, and with the WMM type 2 and subtype 0
        u8 wpaOui[4] = { 0x00, 0x50, 0xf2, 0x01 };
        u8 wmmOui[5] = { 0x00, 0x50, 0xf2, 0x02, 0x00 };

        ie = (const u8 *)&source->gameInfo;
        output = (u8 *)&destination->gameInfo;
        for (n = 0; n < source->otherElementCount; n++) {
            tag = ie[0];
            len = ie[1];
            if (tag == 0x32 || tag == 0x2a || tag == 0x2f || tag == 7 || tag == 0x30) {
                MI_CpuCopy(ie, output, (u8)(len + 2));
                output += len + 2;
                total += (u8)(len + 2);
                destination->otherElementCount++;
            } else if (tag == 0xdd) {
                if (MI_CpuComp8(ie + 2, wpaOui, 4) == 0 || MI_CpuComp8(ie + 2, wmmOui, 5) == 0) {
                    MI_CpuCopy(ie, output, (u8)(len + 2));
                    output += len + 2;
                    total += (u8)(len + 2);
                    destination->otherElementCount++;
                }
            }
            ie += len + 2;
        }
    }
    // The original adds the elements' length in a branch per mode, the DS one through a 16-bit mask
    if (work->wirelessMode == 1) {
        destination->length += (u16)(total & 0xffff);
    } else {
        destination->length += total;
    }
}

static void *WCMi_GetGameInfo(const WMBssDesc *bss) {
    WCMWork *work = WCMi_GetSystemWork();
    WCMVendorOui oui = sNintendoOui;

    if (bss == NULL) {
        return NULL;
    }
    if (OS_IsRunOnTwl()) {
        if (work->wirelessMode == 2) {
            NWMInfoElements *ie = NULL;

            if (bss->otherElementCount != 0) {
                ie = NWM_GetVenderInfoElements((NWMBssDesc *)bss, 0xdd, oui.bytes);
            }
            if (ie != NULL) {
                // The game info starts 8 bytes into the element's data
                const WMGameInfo *info = (const WMGameInfo *)&ie->element[4];

                if (info->magicNumber == 1) {
                    return (void *)info;
                }
            }
        }
    } else if (work->wirelessMode == 1) {
        u16 len = bss->gameInfoLength;

        if (len != 0) {
            BOOL valid = FALSE;

            if (len >= 0x10 && len <= 0x80 && len == bss->gameInfo.userGameInfoLength + 0x10 &&
                bss->gameInfo.magicNumber == 1 && bss->length == (len + 1 + offsetof(WMBssDesc, gameInfo)) / 2) {
                valid = TRUE;
            }
            if (valid == TRUE) {
                return (void *)&bss->gameInfo;
            }
        }
    }
    return NULL;
}
