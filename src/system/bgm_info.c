#include "types.h"
#include "gfl/sound.h"
#include "gfl/arc.h"
#include "gfl/heap.h"

// The BGM info archive: which interactive sound subsystem each BGM uses. The name is the ROM's string.

// The archive's one file: the count of BGMs in a byte, then for each its number in two bytes and its subsystem in one
#define ARCID_BGM_INFO 86

struct BGMInfo {
    u8 count;
    u16 *bgms;
    u8 *subsystems;
};

static void BGMInfo_LoadArcData(BGMInfo *info, HeapID heapId);

BGMInfo *BGMInfo_Create(HeapID heapId) {
    BGMInfo *info = GFL_HeapAllocate(heapId, sizeof(BGMInfo), FALSE, "bgm_info.c", 71);

    BGMInfo_LoadArcData(info, heapId);
    return info;
}

void BGMInfo_Free(BGMInfo *info) {
    GFL_HeapFree(info->bgms);
    GFL_HeapFree(info->subsystems);
    GFL_HeapFree(info);
}

u8 BGMInfo_GetISSSubsystem(BGMInfo *info, u16 bgm) {
    int i;

    for (i = 0; i < info->count; i++) {
        if (bgm == info->bgms[i]) {
            return info->subsystems[i];
        }
    }
    return 0;
}

static void BGMInfo_LoadArcData(BGMInfo *info, HeapID heapId) {
    int i;
    u32 offset = 0;

    GFL_ArcSysReadRange(&info->count, ARCID_BGM_INFO, 0, offset, sizeof(u8));
    offset += sizeof(u8);
    info->bgms = GFL_HeapAllocate(heapId, info->count * sizeof(u16), FALSE, "bgm_info.c", 175);
    info->subsystems = GFL_HeapAllocate(heapId, info->count, FALSE, "bgm_info.c", 176);
    for (i = 0; i < info->count; i++) {
        GFL_ArcSysReadRange(&info->bgms[i], ARCID_BGM_INFO, 0, offset, sizeof(u16));
        offset += sizeof(u16);
        GFL_ArcSysReadRange(&info->subsystems[i], ARCID_BGM_INFO, 0, offset, sizeof(u8));
        offset += sizeof(u8);
    }
}
