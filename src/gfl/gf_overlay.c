#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "nitro/fs.h"
#include "nitro/os.h"

// The memory an overlay is loaded to
enum {
    OVL_REGION_MAIN,
    OVL_REGION_ITCM,
    OVL_REGION_DTCM,
};

#define HW_ITCM 0x01000000
#define HW_ITCM_END 0x02000000
#define HW_DTCM_SIZE 0x4000

typedef struct {
    u32 id;
    BOOL loaded;
} OvlEntry;

// The overlays loaded to each region, with room for as many as each can hold at once
typedef struct {
    u8 mainCount;
    u8 itcmCount;
    u8 dtcmCount;
    OvlEntry *main;
    OvlEntry *itcm;
    OvlEntry *dtcm;
} OvlManager;

static void GFL_OvlEntryUnload(OvlEntry *entry);
static BOOL GFL_OvlCheckMemoryCollision(u32 id);
static OvlEntry *GFL_OvlGetList(u32 region);
static int GFL_OvlGetListCount(u32 region);
static u32 GFL_OvlGetMemoryRegion(u32 id);
static BOOL GFL_OvlGetMemoryRange(u32 id, u32 *start, u32 *end);

static OvlManager *sOvlManager;

void GFL_OvlManagerInit(HeapID heapId, u32 mainCount, u32 itcmCount, u32 dtcmCount) {
    u32 size = (mainCount + itcmCount + dtcmCount) * sizeof(OvlEntry) + sizeof(OvlManager);

    sOvlManager = GFL_HeapAllocate(heapId, size, FALSE, "gf_overlay.c", 108);
    sys_memset32(0, sOvlManager, size);
    sOvlManager->mainCount = mainCount;
    sOvlManager->itcmCount = itcmCount;
    sOvlManager->dtcmCount = dtcmCount;
    sOvlManager->main = (OvlEntry *)(sOvlManager + 1);
    sOvlManager->itcm = sOvlManager->main + mainCount;
    sOvlManager->dtcm = sOvlManager->itcm + itcmCount;
}

static void GFL_OvlEntryUnload(OvlEntry *entry) {
    sys_unload_overlay(MI_PROCESSOR_ARM9, entry->id);
    entry->loaded = FALSE;
}

void GFL_OvlUnload(u32 overlayId) {
    u32 region;
    OvlEntry *list;
    int count;
    int i;

    if (overlayId == OVERLAY_NONE) {
        return;
    }
    region = GFL_OvlGetMemoryRegion(overlayId);
    list = GFL_OvlGetList(region);
    count = GFL_OvlGetListCount(region);
    for (i = 0; i < count; i++) {
        if (list[i].loaded == TRUE && overlayId == list[i].id) {
            GFL_OvlEntryUnload(&list[i]);
            return;
        }
    }
}

BOOL GFL_OvlLoad(u32 overlayId) {
    u32 dma = FS_DMA_NOT_USE;
    int region;
    OvlEntry *list;
    int count;
    int i;
    BOOL loaded;
    BOOL result;

    if (overlayId == OVERLAY_NONE) {
        return TRUE;
    }
    if (!GFL_OvlCheckMemoryCollision(overlayId)) {
        return FALSE;
    }
    region = GFL_OvlGetMemoryRegion(overlayId);
    list = GFL_OvlGetList(region);
    count = GFL_OvlGetListCount(region);
    for (i = 0; i < count; i++) {
        OvlEntry *entry = &list[i];

        if (entry->loaded == FALSE) {
            entry->loaded = TRUE;
            entry->id = overlayId;
            break;
        }
    }
    if (i >= count) {
        GFL_ASSERT(0);
        return FALSE;
    }
    // Tightly coupled memory can't be written by DMA
    if (region == OVL_REGION_ITCM || region == OVL_REGION_DTCM) {
        dma = fs_set_dma_id(FS_DMA_NOT_USE);
    }
    loaded = sys_load_overlay(MI_PROCESSOR_ARM9, overlayId);
    if (region == OVL_REGION_ITCM || region == OVL_REGION_DTCM) {
        fs_set_dma_id(dma);
    }
    result = FALSE;
    if (loaded) {
        result = TRUE;
    }
    return result;
}

// Returns FALSE if the overlay would overwrite one that is loaded
static BOOL GFL_OvlCheckMemoryCollision(u32 id) {
    u32 start;
    u32 end;
    u32 region;
    OvlEntry *list;
    int count;
    int i;

    if (!GFL_OvlGetMemoryRange(id, &start, &end)) {
        return FALSE;
    }
    region = GFL_OvlGetMemoryRegion(id);
    list = GFL_OvlGetList(region);
    count = GFL_OvlGetListCount(region);
    for (i = 0; i < count; i++) {
        u32 loadedStart;
        u32 loadedEnd;

        if (list[i].loaded == TRUE && GFL_OvlGetMemoryRange(list[i].id, &loadedStart, &loadedEnd) == TRUE) {
            if ((start >= loadedStart && start < loadedEnd) || (end > loadedStart && end <= loadedEnd)
                || (start <= loadedStart && end >= loadedEnd)) {
                GFL_ASSERT(0);
                return FALSE;
            }
        }
    }
    return TRUE;
}

static OvlEntry *GFL_OvlGetList(u32 region) {
    switch (region) {
    case OVL_REGION_MAIN:
    default:
        return sOvlManager->main;
    case OVL_REGION_ITCM:
        return sOvlManager->itcm;
    case OVL_REGION_DTCM:
        return sOvlManager->dtcm;
    }
}

static int GFL_OvlGetListCount(u32 region) {
    switch (region) {
    case OVL_REGION_MAIN:
        return sOvlManager->mainCount;
    case OVL_REGION_ITCM:
        return sOvlManager->itcmCount;
    case OVL_REGION_DTCM:
        return sOvlManager->dtcmCount;
    }
    return 0;
}

static u32 GFL_OvlGetMemoryRegion(u32 id) {
    FSOverlayInfo info;
    u32 address;

    sys_read_overlay_header(&info, MI_PROCESSOR_ARM9, id);
    address = (u32)info.ramAddress;
    if (address <= HW_ITCM_END && address >= HW_ITCM) {
        return OVL_REGION_ITCM;
    }
    if (address <= HW_DTCM + HW_DTCM_SIZE && address >= HW_DTCM) {
        return OVL_REGION_DTCM;
    }
    return OVL_REGION_MAIN;
}

static BOOL GFL_OvlGetMemoryRange(u32 id, u32 *start, u32 *end) {
    FSOverlayInfo info;

    if (!sys_read_overlay_header(&info, MI_PROCESSOR_ARM9, id)) {
        return FALSE;
    }
    *start = (u32)info.ramAddress;
    *end = *start + (info.ramSize + info.bssSize);
    return TRUE;
}
