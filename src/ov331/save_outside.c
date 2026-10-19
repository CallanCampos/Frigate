#include "types.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "save/key_info.h"
#include "save/mystery_gift.h"
#include "save/save_control.h"
#include "save/save_outside.h"

// Data outside the save, at the end of the card: the gifts at GIFTS_ADDR and their backup copy at GIFTS_BACKUP_ADDR,
// then the keys. A block is a header followed by its data, whose CRC it keeps. The file's name is the ROM's

#define GIFTS_ADDR 0x7e000
#define GIFTS_BACKUP_ADDR 0x7e400
#define KEYS_ADDR 0x7e800
#define SAVE_OUTSIDE_SIZE 0xb00
#define KEYS_SIZE 0x300

#define SAVE_OUTSIDE_MAGIC 0xa10f49ae

// The size of the key system's data: the key information and the Memory Link's
#define KEY_DATA_SIZE 0x214

enum {
    BLOCK_GIFTS,
    BLOCK_GIFTS_BACKUP,
    BLOCK_KEYS,
    BLOCK_COUNT,
};

// What the load found of the gifts' copies
enum {
    GIFTS_OK,
    GIFTS_MAIN_BROKEN,
    GIFTS_BACKUP_BROKEN,
    GIFTS_BROKEN,
};

typedef struct {
    u32 magic;
    // Counts the saves, so that the newer copy wins
    u32 count;
    u16 crc;
    // The size of the data, which isn't saved
    u16 size;
} SaveOutsideHeader;

typedef struct {
    SaveOutsideHeader header;
    SaveOutsideGifts gifts;
} SaveOutsideGiftsBlock;

typedef struct {
    SaveOutsideHeader header;
    u8 data[KEY_DATA_SIZE];
} SaveOutsideKeysBlock;

struct SaveOutside {
    SaveOutsideGiftsBlock gifts;
    SaveOutsideKeysBlock keys;
    u32 giftsState;
    u8 giftsLoaded;
    u8 saveSeq;
    // The copy of the gifts that is written first
    u8 saveSlot;
    u8 keysLoaded;
    u16 lockId;
    u8 keysBroken;
};

SaveOutside *SaveOutside_Load(HeapID heapId) {
    int latest = -1;
    SaveOutside *work = GFL_HeapAllocate(heapId, sizeof(SaveOutside), TRUE, "save_outside.c", 121);
    SaveOutsideHeader *blocks[BLOCK_COUNT];
    BOOL loaded[BLOCK_COUNT];
    BOOL broken[BLOCK_COUNT];
    int i;

    blocks[BLOCK_GIFTS] = &work->gifts.header;
    blocks[BLOCK_GIFTS_BACKUP] = GFL_HeapAllocate(heapId, sizeof(SaveOutsideGiftsBlock), TRUE, "save_outside.c", 123);
    blocks[BLOCK_KEYS] = &work->keys.header;
    for (i = 0; i < BLOCK_COUNT; i++) {
        loaded[i] = FALSE;
        broken[i] = FALSE;
    }
    loaded[BLOCK_GIFTS] = func_0203bf30(GIFTS_ADDR, blocks[BLOCK_GIFTS], sizeof(SaveOutsideGiftsBlock));
    loaded[BLOCK_GIFTS_BACKUP] =
        func_0203bf30(GIFTS_BACKUP_ADDR, blocks[BLOCK_GIFTS_BACKUP], sizeof(SaveOutsideGiftsBlock));
    loaded[BLOCK_KEYS] = func_0203bf30(KEYS_ADDR, blocks[BLOCK_KEYS], sizeof(SaveOutsideKeysBlock));
    blocks[BLOCK_GIFTS]->size = sizeof(SaveOutsideGifts);
    blocks[BLOCK_GIFTS_BACKUP]->size = sizeof(SaveOutsideGifts);
    blocks[BLOCK_KEYS]->size = KEY_DATA_SIZE;

    for (i = 0; i < BLOCK_COUNT; i++) {
        if (loaded[i] == TRUE) {
            SaveOutsideHeader *header = blocks[i];

            if (header->magic != SAVE_OUTSIDE_MAGIC) {
                loaded[i] = FALSE;
            } else if (header->crc != getCRC16(header + 1, header->size)) {
                broken[i] = TRUE;
            }
        }
    }

    work->giftsLoaded = FALSE;
    if (loaded[BLOCK_GIFTS] == FALSE && loaded[BLOCK_GIFTS_BACKUP] == FALSE) {
        // Nothing was saved yet, so the gifts are cleared below
    } else if (loaded[BLOCK_GIFTS] == TRUE && loaded[BLOCK_GIFTS_BACKUP] == FALSE) {
        if (broken[BLOCK_GIFTS] == TRUE) {
            work->giftsState = GIFTS_BROKEN;
        } else {
            latest = BLOCK_GIFTS;
        }
    } else if (loaded[BLOCK_GIFTS] == FALSE && loaded[BLOCK_GIFTS_BACKUP] == TRUE) {
        if (broken[BLOCK_GIFTS_BACKUP] == TRUE) {
            work->giftsState = GIFTS_BROKEN;
        } else {
            latest = BLOCK_GIFTS_BACKUP;
        }
    } else if (loaded[BLOCK_GIFTS] == TRUE && loaded[BLOCK_GIFTS_BACKUP] == TRUE) {
        if (broken[BLOCK_GIFTS] == TRUE && broken[BLOCK_GIFTS_BACKUP] == TRUE) {
            work->giftsState = GIFTS_BROKEN;
        } else if (broken[BLOCK_GIFTS] == FALSE && broken[BLOCK_GIFTS_BACKUP] == FALSE) {
            u32 backupCount = blocks[BLOCK_GIFTS_BACKUP]->count;
            u32 count = blocks[BLOCK_GIFTS]->count;

            // The count wraps around from 0xffffffff to 0
            if (count >= backupCount || (count == 0 && backupCount == 0xffffffff)) {
                latest = BLOCK_GIFTS;
            } else {
                latest = BLOCK_GIFTS_BACKUP;
            }
        } else if (broken[BLOCK_GIFTS] == FALSE) {
            work->giftsState = GIFTS_BACKUP_BROKEN;
            latest = BLOCK_GIFTS;
        } else {
            latest = BLOCK_GIFTS_BACKUP;
            work->giftsState = GIFTS_MAIN_BROKEN;
        }
    }

    work->keysLoaded = FALSE;
    work->keysBroken = FALSE;
    if (loaded[BLOCK_KEYS] == FALSE) {
        KeySystemData_Init(work->keys.data);
    } else if (broken[BLOCK_KEYS] == TRUE) {
        work->keysBroken = TRUE;
        KeySystemData_Init(work->keys.data);
    } else {
        work->keysLoaded = TRUE;
    }

    if (latest == BLOCK_GIFTS_BACKUP) {
        MI_CpuCopy8(blocks[BLOCK_GIFTS_BACKUP], blocks[BLOCK_GIFTS], sizeof(SaveOutsideGiftsBlock));
    }
    GFL_HeapFree(blocks[BLOCK_GIFTS_BACKUP]);
    if (latest != -1) {
        work->giftsLoaded = TRUE;
    } else {
        MI_CpuFill8(&work->gifts, 0, sizeof(SaveOutsideGiftsBlock));
        mgEncryptData(&work->gifts.gifts, MG_DATA_OUTSIDE);
    }
    return work;
}

void SaveOutside_Free(SaveOutside *work) {
    GFL_HeapFree(work);
}

void SaveOutside_StartSave(SaveOutside *work) {
    work->saveSeq = 0;
    if (work->giftsState == GIFTS_BACKUP_BROKEN) {
        work->saveSlot = 1;
    } else {
        work->saveSlot = 0;
    }
    work->gifts.header.magic = SAVE_OUTSIDE_MAGIC;
    work->gifts.header.count++;
    work->gifts.header.crc = getCRC16(&work->gifts.gifts, sizeof(SaveOutsideGifts));
    work->keys.header.magic = SAVE_OUTSIDE_MAGIC;
    work->keys.header.count++;
    work->keys.header.crc = getCRC16(work->keys.data, KEY_DATA_SIZE);
    GCTX_HIDBlockSleep(0x80);
    GCTX_HIDBlockSoftReset(2);
}

BOOL SaveOutside_Save(SaveOutside *work) {
    BOOL result;

    switch (work->saveSeq) {
    case 0:
    case 2:
        work->lockId =
            func_0203bf00(work->saveSlot == 0 ? GIFTS_ADDR : GIFTS_BACKUP_ADDR, work, sizeof(SaveOutsideGiftsBlock));
        work->saveSeq++;
        break;
    case 1:
    case 3:
    case 5:
        if (func_0203bf18(work->lockId, &result) == TRUE) {
            work->saveSlot ^= 1;
            work->saveSeq++;
        }
        break;
    case 4:
        work->lockId = func_0203bf00(KEYS_ADDR, &work->keys, sizeof(SaveOutsideKeysBlock));
        work->saveSeq++;
        break;
    default:
        work->giftsState = GIFTS_OK;
        work->giftsLoaded = TRUE;
        GCTX_HIDUnblockSoftReset(2);
        GCTX_HIDUnblockSleep(0x80);
        return TRUE;
    }
    return FALSE;
}

BOOL SaveOutside_IsGiftsLoaded(SaveOutside *work) {
    return work->giftsLoaded;
}

BOOL SaveOutside_IsGiftsBroken(SaveOutside *work) {
    if (work->giftsState == GIFTS_BROKEN) {
        return TRUE;
    }
    return FALSE;
}

SaveOutsideGifts *SaveOutside_GetGifts(SaveOutside *work) {
    return &work->gifts.gifts;
}

void SaveOutside_CopyGiftsToSave(SaveOutside *work, SaveControl *save) {
    MysteryGiftSaveData *giftSave = SaveControl_GetBlockPtr(save, SAVE_BLOCK_MYSTERY_GIFT);
    SaveOutsideGifts *gifts = &work->gifts.gifts;
    int i;

    mgDecryptData(giftSave, MG_DATA_SAVE);
    mgDecryptData(gifts, MG_DATA_OUTSIDE);
    for (i = 0; i < 0x100; i++) {
        giftSave->receivedFlags[i] = gifts->receivedFlags[i];
    }
    for (i = 0; i < 3; i++) {
        giftSave->cards[i] = gifts->cards[i];
    }
    mgEncryptData(giftSave, MG_DATA_SAVE);
    mgEncryptData(gifts, MG_DATA_OUTSIDE);
}

void SaveOutside_Erase(HeapID heapId) {
    void *buf = GFL_HeapAllocate(heapId, SAVE_OUTSIDE_SIZE, FALSE, "save_outside.c", 431);

    MIi_CpuClear32(0xffffffff, buf, SAVE_OUTSIDE_SIZE);
    func_0203bee8(GIFTS_ADDR, buf, SAVE_OUTSIDE_SIZE);
    GFL_HeapFree(buf);
}

void SaveOutside_EraseKeys(HeapID heapId) {
    void *buf = GFL_HeapAllocate(heapId, KEYS_SIZE, FALSE, "save_outside.c", 448);

    MIi_CpuClear32(0xffffffff, buf, KEYS_SIZE);
    func_0203bee8(KEYS_ADDR, buf, KEYS_SIZE);
    GFL_HeapFree(buf);
}

BOOL SaveOutside_IsKeysLoaded(SaveOutside *work) {
    return work->keysLoaded;
}

BOOL SaveOutside_IsKeysBroken(SaveOutside *work) {
    return work->keysBroken;
}

void *SaveOutside_GetKeyData(SaveOutside *work) {
    return work->keys.data;
}

void SaveOutside_CopyKeysToSave(SaveOutside *work, SaveControl *save) {
    KeyInfoSave *keyInfo = KeySystemData_GetKeyInfo(SaveOutside_GetKeyData(work));

    KeyInfoSave_Copy(getKeyInfoSaveBlk(save), keyInfo);
}
