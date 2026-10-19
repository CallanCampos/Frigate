#include "types.h"
#include "constants/species.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "save/battle_rec.h"
#include "save/save_control.h"
#include "save/tournament_block.h"
#include "system/game_data.h"

// battle_rec.c (the name its allocation passes): the battle video that is loaded, and loading it from and saving it
// to the extra save blocks of the four slots. Its body is encrypted with its CRC as the key.

static BattleRecVideo *BattleRec_AllocVideo(HeapID heapId);
static BOOL BattleRec_LoadSlot(SaveControl *save, BattleRecVideo *video, HeapID heapId, u32 *result, u32 slot);
static void BattleRec_MakeHeader(SaveControl *save, BattleRecHeader *header, BattleRecBody *body, u32 modeIndex,
                                 u32 counter);
static BOOL BattleRec_IsEmpty(SaveControl *save, BattleRecVideo *video);
static BOOL BattleRec_IsValid(SaveControl *save, BattleRecVideo *video);
static void __decryptData(void *data, u32 size, u32 seed);
static u32 BattleRec_GetExtraBlock(u32 slot);

// The battle modes that a header's mode can be. A video's mode is given as an index into them
static const u16 sBattleRecModes[BATTLE_REC_MODE_MAX] = {
    0x018, 0x218, 0x098, 0x298, 0x019, 0x219, 0x099, 0x299, 0x01a, 0x21a, 0x09a, 0x29a, 0x01b, 0x21b,
    0x09b, 0x29b, 0x01c, 0x21c, 0x09c, 0x29c, 0x000, 0x001, 0x004, 0x028, 0x029, 0x02a, 0x02b, 0x0aa,
    0x068, 0x069, 0x06a, 0x06b, 0x0ea, 0x038, 0x0b8, 0x039, 0x0b9, 0x03a, 0x0ba, 0x03b, 0x0bb,
};

BattleRecVideo *data_02140f64;

// The key that a body is encrypted with, of its CRC
#define BATTLE_REC_KEY(body) ((body)->crc + (((body)->crc ^ 0xffff) << 16))

static BattleRecVideo *BattleRec_AllocVideo(HeapID heapId) {
    BattleRecVideo *video = GFL_HeapAllocate(heapId, BATTLE_REC_BLOCK_SIZE, TRUE, "battle_rec.c", 104);

    clearVSPlayerBlk(video);
    return video;
}

void BattleRec_Init(HeapID heapId) {
    if (data_02140f64 != NULL) {
        GFL_HeapFree(data_02140f64);
        data_02140f64 = NULL;
    }
    data_02140f64 = BattleRec_AllocVideo(heapId);
}

void freeVSPlayerBlkClearPtr(void) {
    GFL_HeapFree(data_02140f64);
    data_02140f64 = NULL;
}

void BattleRec_FreeVideo(BattleRecVideo *video) {
    GFL_HeapFree(video);
}

u32 getSizeOfVSPlayer(void) {
    return sizeof(BattleRecVideo);
}

void clearVSPlayerBlk(BattleRecVideo *video) {
    MIi_CpuClear32(0, video, sizeof(BattleRecVideo));
}

BOOL func_0200bb80(void) {
    if (addUpDataStream((u8 *)&data_02140f64->body, sizeof(BattleRecBody)) != 0) {
        return TRUE;
    }
    return FALSE;
}

void clearVSPlayer(void) {
    clearVSPlayerBlk(data_02140f64);
}

void func_0200bbb4(void) {
    __decryptData(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
}

void func_0200bbdc(void) {
    decryptVSPlayer(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
}

static BOOL BattleRec_LoadSlot(SaveControl *save, BattleRecVideo *video, HeapID heapId, u32 *result, u32 slot) {
    BattleRecBody *body;

    *result = SaveControl_LoadExtraBlock(save, BattleRec_GetExtraBlock(slot), heapId, video, BATTLE_REC_BLOCK_SIZE);
    freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(slot));
    if (*result == 0) {
        *result = 0;
        return TRUE;
    }
    if (*result != 1) {
        *result = 3;
        return TRUE;
    }

    body = &video->body;
    __decryptData(body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(body));
    if (BattleRec_IsEmpty(save, video) == TRUE) {
        *result = 0;
        return TRUE;
    }
    if (!BattleRec_IsValid(save, video)) {
        *result = 2;
        return TRUE;
    }
    *result = 1;
    return TRUE;
}

void BattleRec_LoadToCurrent(SaveControl *save, HeapID heapId, u32 *result, u32 slot) {
    if (data_02140f64 != NULL) {
        clearVSPlayerBlk(data_02140f64);
    } else {
        BattleRec_Init(heapId);
    }
    BattleRec_LoadSlot(save, data_02140f64, heapId, result, slot);
}

BattleRecVideo *BattleRec_LoadVideo(SaveControl *save, HeapID heapId, u32 *result, u32 slot) {
    BattleRecVideo *video = BattleRec_AllocVideo(heapId);

    BattleRec_LoadSlot(save, video, heapId, result, slot);
    return video;
}

// Whether a slot has a valid video, without keeping it. a2 is the u32 result of BattleRec_LoadToCurrent
u32 BattleRec_IsSlotValid(SaveControl *save, HeapID a1, void *a2, u32 a3) {
    u32 *result = a2;
    BattleRecVideo *video;
    BattleRecBody *body;

    *result = SaveControl_OpenExtraBlock(save, BattleRec_GetExtraBlock(a3), a1);
    video = getAddressOfExtraSaveBlk(save, BattleRec_GetExtraBlock(a3), 0);
    if (*result == 0) {
        *result = 0;
        freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(a3));
        return FALSE;
    }
    if (*result != 1) {
        *result = 3;
        freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(a3));
        return FALSE;
    }

    body = &video->body;
    __decryptData(body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(body));
    if (BattleRec_IsEmpty(save, video) == TRUE) {
        *result = 0;
        freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(a3));
        return FALSE;
    }
    if (!BattleRec_IsValid(save, video)) {
        *result = 2;
        freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(a3));
        return FALSE;
    }
    *result = 1;
    freeIntermediateSaveExtraBlksAfterLoad(save, BattleRec_GetExtraBlock(a3));
    return TRUE;
}

u32 BattleRec_SaveSlot(GameData *gameData, BattleRecVideo *video, u32 slot, u16 *seq, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    u32 result;

    switch (*seq) {
    case 0:
        SaveControl_InitExtraBlock(save, BattleRec_GetExtraBlock(slot), heapId, data_02140f64, BATTLE_REC_BLOCK_SIZE);
        GameData_StartSaveBlock(gameData, BattleRec_GetExtraBlock(slot));
        (*seq)++;
        break;
    case 1:
        result = GameData_UpdateSaveBlock(gameData, BattleRec_GetExtraBlock(slot));
        if (result == 2 || result == 3) {
            freeIntermediateSaveExtraBlksAfterLoad2(save, BattleRec_GetExtraBlock(slot));
            return result;
        }
        break;
    }
    return 0;
}

// Makes the loaded video's header, signs and encrypts the video and saves it to slot a4, a step each call like
// BattleRec_SaveSlot. a1 is the heap, a2 the index of the battle mode and a3 the header's counter. 3 if no video is
// loaded
u32 BattleRec_FinishAndSaveSlot(GameData *gameData, HeapID a1, u32 a2, u32 a3, u32 a4, u16 *a5, u16 *a6) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    BattleRecHeader *header;
    BattleRecBody *body;

    switch (*a5) {
    case 0:
        if (data_02140f64 == NULL) {
            return 3;
        }
        header = &data_02140f64->header;
        body = &data_02140f64->body;
        BattleRec_MakeHeader(save, header, body, a2, a3);
        header->magic = BATTLE_REC_MAGIC;
        header->crc = getCRC16(header, BATTLE_REC_HEADER_CRC_SIZE);
        body->magic = BATTLE_REC_MAGIC;
        BattleRec_UpdateBodyCRC(body);
        decryptVSPlayer(body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(body));
        *a6 = 0;
        (*a5)++;
        break;
    case 1:
        return BattleRec_SaveSlot(gameData, data_02140f64, a4, a6, a1);
    }
    return 0;
}

void BattleRec_GetModeCounts(u16 mode, s32 *clientCount, s32 *pkmCount) {
    s32 i;

    for (i = 0; i < BATTLE_REC_MODE_MAX; i++) {
        if (mode == sBattleRecModes[i]) {
            switch (i) {
            case 16:
            case 17:
            case 18:
            case 19:
            case 22:
                *clientCount = 4;
                *pkmCount = 3;
                return;
            }
        }
    }
    *clientCount = 2;
    *pkmCount = 6;
}

static void BattleRec_MakeHeader(SaveControl *save, BattleRecHeader *header, BattleRecBody *body, u32 modeIndex,
                                 u32 counter) {
    s32 client;
    s32 i;
    s32 n = 0;
    s32 clientCount;
    s32 pkmCount;
    u16 mode;

    MI_CpuFill8(header, 0, sizeof(BattleRecHeader));
    mode = sBattleRecModes[modeIndex];
    BattleRec_GetModeCounts(mode, &clientCount, &pkmCount);
    for (client = 0; client < clientCount; client++) {
        for (i = 0; i < pkmCount; i++) {
            BattleRecPkm *pkm = &body->parties[client].pkm[i];

            if (!pkm->isEgg && !pkm->badEgg) {
                header->species[n] = pkm->species;
                header->formGender[n] = (pkm->form & 0x7f) | (pkm->gender << 7);
            }
            n++;
        }
    }
    header->counter = counter;
    header->mode = mode;
    header->version = BATTLE_REC_VERSION;
}

// Whether a video is not one: a slot that was never saved to
static BOOL BattleRec_IsEmpty(SaveControl *save, BattleRecVideo *video) {
    BattleRecHeader *header = &video->header;

    if (video->body.magic != BATTLE_REC_MAGIC || header->magic != BATTLE_REC_MAGIC) {
        return TRUE;
    }
    return FALSE;
}

// Whether a video's magic numbers and CRCs are right
static BOOL BattleRec_IsValid(SaveControl *save, BattleRecVideo *video) {
    BattleRecBody *body = &video->body;
    BattleRecHeader *header = &video->header;
    u16 crc;

    if (body->magic != BATTLE_REC_MAGIC || header->magic != BATTLE_REC_MAGIC) {
        return FALSE;
    }
    crc = getCRC16(header, BATTLE_REC_HEADER_CRC_SIZE);
    if (crc != header->crc) {
        return FALSE;
    }
    crc = getCRC16(body, BATTLE_REC_BODY_CRC_SIZE);
    if (crc == body->crc) {
        return TRUE;
    }
    return FALSE;
}

static void __decryptData(void *data, u32 size, u32 seed) {
    _decryptData(data, size, seed);
}

// The extra save block of a slot
static u32 BattleRec_GetExtraBlock(u32 slot) {
    switch (slot) {
    default:
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    }
}

void decryptVSPlayer(void *data, u32 size, u32 seed) {
    decryptData(data, size, seed);
}

GdsProfile *getVSPlayerAllocation(void) {
    return (GdsProfile *)data_02140f64->profile;
}

BattleRecBody *BattleRec_GetBody(void) {
    return &data_02140f64->body;
}

BattleRecHeader *BattleRec_GetHeader(void) {
    return &data_02140f64->header;
}

BattleRecHeader *BattleRec_GetVideoHeader(BattleRecVideo *video) {
    return &video->header;
}

void BattleRec_StoreDownloaded(const GdsProfile *profile, const BattleRecHeader *header, const BattleRecBody *body,
                               SaveControl *save) {
    MI_CpuCopy8(header, &data_02140f64->header, sizeof(BattleRecHeader));
    MI_CpuCopy8(body, &data_02140f64->body, sizeof(BattleRecBody));
    MI_CpuCopy8(profile, data_02140f64->profile, sizeof(data_02140f64->profile));
    __decryptData(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
}

u64 BattleRec_GetHeaderValue(BattleRecHeader *header, u32 id, u32 index) {
    s32 i;

    switch (id) {
    case BATTLE_REC_HEADER_SPECIES:
        if (header->species[index] > SPECIES_GENESECT) {
            return 0;
        }
        return header->species[index];
    case BATTLE_REC_HEADER_FORM:
        return header->formGender[index] & 0x7f;
    case BATTLE_REC_HEADER_COUNTER:
        if (header->counter > 9999) {
            return 9999;
        }
        return header->counter;
    case BATTLE_REC_HEADER_MODE:
        for (i = 0; i < BATTLE_REC_MODE_MAX; i++) {
            if (header->mode == sBattleRecModes[i]) {
                return i;
            }
        }
        return 0;
    case BATTLE_REC_HEADER_DATA_NUMBER:
        return header->dataNumber;
    case BATTLE_REC_HEADER_UNK2A:
        return header->unk2A;
    case BATTLE_REC_HEADER_GENDER:
        return (header->formGender[index] & 0x80) >> 7;
    case BATTLE_REC_HEADER_VERSION:
        return header->version;
    }
    return 0;
}

// Whether a video of this version can be played
u32 BattleRec_IsVersionSupported(u8 a0) {
    if (a0 <= BATTLE_REC_VERSION) {
        return TRUE;
    }
    return FALSE;
}

void BattleRec_UpdateBodyCRC(BattleRecBody *body) {
    body->crc = getCRC16(body, BATTLE_REC_BODY_CRC_SIZE);
}

// Load and unload battle_rec_tool.c's overlay
void BattleRec_LoadToolOverlay(void) {
    GFL_OvlLoad(OVERLAY_ID(273));
}

void BattleRec_UnloadToolOverlay(void) {
    GFL_OvlUnload(OVERLAY_ID(273));
}
