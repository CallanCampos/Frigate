#include "types.h"
#include "save/musical_dist_save.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The downloaded musical, extra save block 6: reading it for the musical, and writing the one Game Sync downloaded.
// The name is the ROM's string, from GFL_HeapAllocate's calls. The function names are ours

#define SAVE_EXTRA_BLOCK_MUSICAL_DIST 6

u32 func_0200cc94(void) {
    return MUSICAL_DIST_SAVE_SIZE;
}

void func_0200cc9c(void *block) {
}

void *func_0200cca0(GameData *gameData, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    MusicalDistWork *wk = GFL_HeapAllocate(heapId, sizeof(MusicalDistWork), TRUE, "musical_dist_save.c", 82);
    u32 result;

    wk->block = GFL_HeapAllocate(heapId, MUSICAL_DIST_BUFFER_SIZE, TRUE, "musical_dist_save.c", 84);
    result = func_02007560(save, SAVE_EXTRA_BLOCK_MUSICAL_DIST, heapId, wk->block, MUSICAL_DIST_BUFFER_SIZE);
    wk->heapId = heapId;
    wk->gameData = gameData;
    wk->seq = 0;
    if (result == 1) {
        wk->loaded = TRUE;
        wk->block = getAddressOfExtraSaveBlk(save, SAVE_EXTRA_BLOCK_MUSICAL_DIST, 0);
        wk->blockHeader = wk->block;
        wk->data = (u8 *)wk->block + sizeof(MusicalDistHeader);
    } else {
        wk->loaded = FALSE;
        wk->block = getAddressOfExtraSaveBlk(save, SAVE_EXTRA_BLOCK_MUSICAL_DIST, 0);
        wk->blockHeader = wk->block;
        wk->data = (u8 *)wk->block + sizeof(MusicalDistHeader);
    }
    return wk;
}

void func_0200cd10(void *work) {
    MusicalDistWork *wk = work;

    freeIntermediateSaveExtraBlksAfterLoad(GameData_GetSaveControl(wk->gameData), SAVE_EXTRA_BLOCK_MUSICAL_DIST);
    if (wk->block != NULL) {
        GFL_HeapFree(wk->block);
    }
    GFL_HeapFree(wk);
}

void *func_0200cd34(GameData *gameData, void *data, u32 size, HeapID heapId) {
    MusicalDistWork *wk = func_0200cca0(gameData, heapId);

    if (wk->block != NULL) {
        wk->header.size = size;
        sys_memcpy(&wk->header, wk->block, sizeof(MusicalDistHeader));
        sys_memcpy(data, wk->data, size);
    }
    return wk;
}

BOOL func_0200cd64(void *work) {
    MusicalDistWork *wk = work;

    switch (wk->seq) {
    case 0:
        if (wk->block == NULL) {
            func_0200cd10(wk);
            return TRUE;
        }
        {
            MusicalSave *musical = getAddressOfMusicalDataInfo(GameData_GetSaveControl(wk->gameData));
            ArcTool *arc;
            void *file;
            MsgData *msgData;
            StrBuf *title;
            u32 size;

            func_0200af08(musical, 1);
            arc = GFL_ArcSysCreateMemoryHandle(wk->data, wk->header.size, HEAPID_TAIL(wk->heapId));
            file = GFL_ArcToolReadHeapNewLZGetLen(arc, 1, FALSE, HEAPID_TAIL(wk->heapId), &size);
            msgData = GFL_MsgDataCreateFromHandle(file, HEAPID_TAIL(wk->heapId));
            title = GFL_MsgDataLoadStrbufNew(msgData, 0);
            GFL_StrBufStoreString(title, (u16 *)func_0200af14(musical), 0x25);
            GFL_StrBufFree(title);
            GFL_MsgDataFree(msgData);
            GFL_HeapFree(file);
            GFL_ArcToolFree(arc);
            func_020178c4(wk->gameData, SAVE_EXTRA_BLOCK_MUSICAL_DIST);
            wk->seq++;
        }
        break;
    case 1:
        if (func_020178f4(wk->gameData, SAVE_EXTRA_BLOCK_MUSICAL_DIST) == 2) {
            func_0200cd10(wk);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void *func_0200ce44(void *work) {
    MusicalDistWork *wk = work;

    return wk->data;
}

u32 func_0200ce48(void *work) {
    MusicalDistWork *wk = work;

    return wk->blockHeader->size;
}
