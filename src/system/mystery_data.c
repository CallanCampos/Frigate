#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/mystery_gift.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/game_data.h"

// The mystery gift album: the cards received and the flag of each gift ID, encrypted in the save's block 0x22, or
// in the data outside the save before there is one. The file's name is the ROM's; the function names from swan
// are mysteryGiftBlock, mgEncryptData and mgDecryptData, and the statics are ours.

#define OVERLAY_SAVE_OUTSIDE OVERLAY_ID(331)

static BOOL isSlotUsed(MysteryGiftSave *save, u32 slot);
static void removeCard(MysteryGiftSave *save, u32 slot);
static MysteryGift *getUsedCard(MysteryGiftSave *save, u32 slot);
static BOOL hasFreeSlot(MysteryGiftSave *save);
static u32 getKind(MysteryGiftSave *save);
static u32 getCardMax(MysteryGiftSave *save);
static void *getData(MysteryGiftSave *save);
static u8 *getReceivedFlags(MysteryGiftSave *save);
static MysteryGift *getCard(MysteryGiftSave *save, u32 slot);
static void encrypt(MysteryGiftSave *save);
static void decrypt(MysteryGiftSave *save);

u32 func_0200a5f0(void) {
    return sizeof(MysteryGiftSaveData);
}

void func_0200a5f8(MysteryGiftSaveData *data) {
    MysteryGiftSave save;

    MI_CpuFill8(&save, 0, sizeof(MysteryGiftSave));
    save.kind = MG_DATA_SAVE;
    save.data = data;
    MI_CpuFill8(data, 0, sizeof(MysteryGiftSaveData));
    encrypt(&save);
}

BOOL func_0200a628(u32 kind) {
    if (kind != 0 && kind < 5) {
        return TRUE;
    }
    return FALSE;
}

static BOOL hasFreeSlot(MysteryGiftSave *save) {
    u32 i;
    u32 max = getCardMax(save);

    for (i = 0; i < max; i++) {
        if (!func_0200a628(getCard(save, i)->kind)) {
            return TRUE;
        }
    }
    return FALSE;
}

// The count fetched and dropped here and below is what the code does, likely an assert compiled out
static BOOL isSlotUsed(MysteryGiftSave *save, u32 slot) {
    MysteryGift *card = getCard(save, slot);

    getCardMax(save);
    // BUG: the bound is the save's twelve cards whatever the kind; the gifts outside the save have three
    if (slot < MG_CARD_MAX && func_0200a628(card->kind)) {
        return TRUE;
    }
    return FALSE;
}

static void removeCard(MysteryGiftSave *save, u32 slot) {
    u32 max = getCardMax(save);

    if (isSlotUsed(save, slot)) {
        return;
    }
    for (; slot < max - 1; slot++) {
        *getCard(save, slot) = *getCard(save, slot + 1);
    }
    MI_CpuFill8(getCard(save, slot), 0, sizeof(MysteryGift));
}

static MysteryGift *getUsedCard(MysteryGiftSave *save, u32 slot) {
    MysteryGift *card = getCard(save, slot);

    if (slot < getCardMax(save) && func_0200a628(card->kind)) {
        return card;
    }
    return NULL;
}

BOOL func_0200a71c(MysteryGiftSave *save, u32 slot, MysteryGift *gift) {
    BOOL found = FALSE;
    MysteryGift *card;

    getCardMax(save);
    decrypt(save);
    card = getUsedCard(save, slot);
    if (card != NULL) {
        MI_CpuCopy8(card, gift, sizeof(MysteryGift));
        found = TRUE;
    }
    encrypt(save);
    return found;
}

BOOL func_0200a750(MysteryGiftSave *save, const MysteryGift *gift) {
    BOOL saved = FALSE;
    u32 max = getCardMax(save);
    u32 i;

    decrypt(save);
    if (hasFreeSlot(save) == TRUE) {
        for (i = 0; i < max; i++) {
            MysteryGift *card = getCard(save, i);
            if (!func_0200a628(card->kind)) {
                MI_CpuCopy8(gift, card, sizeof(MysteryGift));
                saved = TRUE;
                break;
            }
        }
    }
    encrypt(save);
    return saved;
}

void func_0200a7b0(MysteryGiftSave *save, u32 slot) {
    u32 max = getCardMax(save);

    decrypt(save);
    if (slot < max) {
        getCard(save, slot)->kind = 0;
        removeCard(save, slot);
    }
    encrypt(save);
}

BOOL func_0200a7e4(MysteryGiftSave *save) {
    BOOL free;

    decrypt(save);
    free = hasFreeSlot(save);
    encrypt(save);
    return free;
}

BOOL func_0200a800(MysteryGiftSave *save, u32 slot) {
    BOOL used;

    decrypt(save);
    used = isSlotUsed(save, slot);
    encrypt(save);
    return used;
}

BOOL func_0200a820(MysteryGiftSave *save, u32 slot) {
    BOOL delivered = FALSE;
    u32 max = getCardMax(save);

    decrypt(save);
    if (slot < max && getCard(save, slot)->delivered) {
        delivered = TRUE;
    }
    encrypt(save);
    return delivered;
}

void func_0200a858(MysteryGiftSave *save, u32 slot) {
    u32 max = getCardMax(save);

    decrypt(save);
    if (slot < max) {
        getCard(save, slot)->delivered = 1;
    }
    encrypt(save);
}

BOOL func_0200a88c(MysteryGiftSave *save) {
    u32 i;
    BOOL any = FALSE;
    u32 max = getCardMax(save);

    decrypt(save);
    for (i = 0; i < max; i++) {
        if (isSlotUsed(save, i) == TRUE) {
            any = TRUE;
        }
    }
    encrypt(save);
    return any;
}

BOOL func_0200a8c4(MysteryGiftSave *save, u32 id) {
    BOOL received = FALSE;
    u8 *flags = getReceivedFlags(save);

    decrypt(save);
    if (id < MG_ID_MAX && ((1 << (id & 7)) & flags[id >> 3])) {
        received = TRUE;
    }
    encrypt(save);
    return received;
}

void func_0200a900(MysteryGiftSave *save, u32 id) {
    u8 *flags = getReceivedFlags(save);

    decrypt(save);
    if (id < MG_ID_MAX) {
        flags[id >> 3] |= (u8)(1 << (id & 7));
    }
    encrypt(save);
}

BOOL func_0200a938(const MysteryGiftRecvData *data) {
    s32 i;
    s32 count = 0;

    // Counts the bytes of the data 16 at a time, to no end
    do {
        for (i = 0; i < 16; i++) {
            count++;
        }
    } while (count < sizeof(MysteryGiftRecvData));
    if (data->crc == getCRC16(data, sizeof(MysteryGiftRecvData) - sizeof(data->crc))) {
        return TRUE;
    }
    return FALSE;
}

void func_0200a970(MysteryGiftSave *save, u32 slot1, u32 slot2) {
    MysteryGift temp;

    decrypt(save);
    if (isSlotUsed(save, slot1) && isSlotUsed(save, slot2)) {
        MysteryGift *card1 = getUsedCard(save, slot1);
        MysteryGift *card2 = getUsedCard(save, slot2);
        MI_CpuCopy8(card1, &temp, sizeof(MysteryGift));
        MI_CpuCopy8(card2, card1, sizeof(MysteryGift));
        MI_CpuCopy8(&temp, card2, sizeof(MysteryGift));
    }
    encrypt(save);
}

void func_0200a9d4(MysteryGiftSave *save, GameData *gameData) {
    if (func_0200aa6c(save) == MG_DATA_OUTSIDE) {
        SaveOutside_StartSave(save->outside);
    } else {
        func_0201782c(gameData);
    }
}

u32 func_0200a9f4(MysteryGiftSave *save, GameData *gameData) {
    if (func_0200aa6c(save) == MG_DATA_OUTSIDE) {
        if (SaveOutside_Save(save->outside)) {
            return 2;
        }
        return 0;
    }
    return func_02017850(gameData);
}

MysteryGiftSave *mysteryGiftBlock(SaveControl *save, u32 mode, HeapID heapId) {
    MysteryGiftSave *giftSave = GFL_HeapAllocate(heapId, sizeof(MysteryGiftSave), FALSE, "mystery_data.c", 608);
    MI_CpuFill8(giftSave, 0, sizeof(MysteryGiftSave));
    func_0200aa74(giftSave, save, mode, heapId);
    return giftSave;
}

void func_0200aa54(MysteryGiftSave *save) {
    func_0200aaec(save);
    GFL_HeapFree(save);
}

u32 func_0200aa64(MysteryGiftSave *save) {
    return getCardMax(save);
}

u32 func_0200aa6c(MysteryGiftSave *save) {
    return getKind(save);
}

void func_0200aa74(MysteryGiftSave *giftSave, SaveControl *save, u32 mode, HeapID heapId) {
    BOOL useSave;

    MI_CpuFill8(giftSave, 0, sizeof(MysteryGiftSave));
    switch (mode) {
    case 0:
        useSave = TRUE;
        break;
    case 1:
        useSave = SaveControl_IsDataAlreadyPresent(save);
        break;
    default:
        useSave = TRUE;
        break;
    }
    if (useSave) {
        giftSave->data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_MYSTERY_GIFT);
        giftSave->kind = MG_DATA_SAVE;
    } else {
        GFL_OvlLoad(OVERLAY_SAVE_OUTSIDE);
        giftSave->outside = SaveOutside_Load(heapId);
        giftSave->data = SaveOutside_GetGifts(giftSave->outside);
        giftSave->kind = MG_DATA_OUTSIDE;
        if (!SaveOutside_IsGiftsLoaded(giftSave->outside)) {
            MI_CpuFill8(giftSave->data, 0, sizeof(SaveOutsideGifts));
            encrypt(giftSave);
        }
    }
}

void func_0200aaec(MysteryGiftSave *save) {
    switch (save->kind) {
    case MG_DATA_OUTSIDE:
        SaveOutside_Free(save->outside);
        GFL_OvlUnload(OVERLAY_SAVE_OUTSIDE);
        break;
    case MG_DATA_SAVE:
        break;
    }
    MI_CpuFill8(save, 0, sizeof(MysteryGiftSave));
}

static u32 getKind(MysteryGiftSave *save) {
    return save->kind;
}

static u32 getCardMax(MysteryGiftSave *save) {
    switch (save->kind) {
    case MG_DATA_OUTSIDE:
        return MG_CARD_MAX_OUTSIDE;
    case MG_DATA_SAVE:
        return MG_CARD_MAX;
    }
    return 0;
}

static void *getData(MysteryGiftSave *save) {
    return save->data;
}

static u8 *getReceivedFlags(MysteryGiftSave *save) {
    switch (save->kind) {
    case MG_DATA_OUTSIDE:
        return ((SaveOutsideGifts *)getData(save))->receivedFlags;
    case MG_DATA_SAVE:
        return ((MysteryGiftSaveData *)getData(save))->receivedFlags;
    }
    return NULL;
}

static MysteryGift *getCard(MysteryGiftSave *save, u32 slot) {
    getCardMax(save);
    switch (save->kind) {
    case MG_DATA_OUTSIDE:
        return &((SaveOutsideGifts *)getData(save))->cards[slot];
    case MG_DATA_SAVE:
        return &((MysteryGiftSaveData *)getData(save))->cards[slot];
    }
    return NULL;
}

static void encrypt(MysteryGiftSave *save) {
    void *data = getData(save);
    mgEncryptData(data, getKind(save));
}

static void decrypt(MysteryGiftSave *save) {
    void *data = getData(save);
    mgDecryptData(data, getKind(save));
}

void mgEncryptData(void *data, u32 kind) {
    u32 size;
    u16 *tail;

    switch (kind) {
    case MG_DATA_OUTSIDE:
        size = sizeof(SaveOutsideGifts);
        tail = &((SaveOutsideGifts *)data)->checksum;
        break;
    case MG_DATA_SAVE:
        size = sizeof(MysteryGiftSaveData);
        tail = &((MysteryGiftSaveData *)data)->checksum;
        break;
    }
    tail[1] = (OS_GetVBlankCount() << 8) | OS_GetVBlankCount();
    if (tail[1] == 0) {
        tail[1] = 1;
    }
    tail[0] = addUpDataStream(data, size - 4);
    decryptData(data, size - 4, tail[0] + (tail[1] << 16));
}

void mgDecryptData(void *data, u32 kind) {
    u32 size;
    u16 *tail;

    switch (kind) {
    case MG_DATA_OUTSIDE:
        size = sizeof(SaveOutsideGifts);
        tail = &((SaveOutsideGifts *)data)->checksum;
        break;
    case MG_DATA_SAVE:
        size = sizeof(MysteryGiftSaveData);
        tail = &((MysteryGiftSaveData *)data)->checksum;
        break;
    }
    _decryptData(data, size - 4, tail[0] + (tail[1] << 16));
    tail[1] = 0;
}
