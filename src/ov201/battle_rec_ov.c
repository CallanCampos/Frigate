// What the Global Data Server client and the Battle Recorder do with the loaded battle video on its way to the server
// and into the save. The ROM does not name the file: the name is a guess, after the Generation 4 games'
// battle_rec_ov.c, which holds the same functions after the GDS client's files.

#include "types.h"
#include "app/battle_rec_ov.h"
#include "gfl/backup_system.h"
#include "gfl/std.h"
#include "save/battle_rec.h"
#include "save/save_control.h"
#include "system/game_data.h"

// What the video saves in: each step, then the save
enum {
    SAVE_STEP_PREPARE,
    SAVE_STEP_WRITE,
};

// The key that a body is encrypted with, of its CRC
#define BATTLE_REC_KEY(body) ((body)->crc + (((body)->crc ^ 0xffff) << 16))

u32 BattleRecOv_SaveVideo(GameData *gameData, u32 slot, BOOL secure, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case SAVE_STEP_PREPARE:
        if (secure == TRUE) {
            gBattleRecVideo->header.secure = secure;
            gBattleRecVideo->header.magic = BATTLE_REC_MAGIC;
            gBattleRecVideo->header.crc = getCRC16(&gBattleRecVideo->header, BATTLE_REC_HEADER_CRC_SIZE);
        }
        decryptVSPlayer(&gBattleRecVideo->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&gBattleRecVideo->body));
        (*seq)++;
        break;
    case SAVE_STEP_WRITE:
        return BattleRec_SaveSlot(gameData, gBattleRecVideo, slot, saveSeq, heapId);
    }
    return 0;
}

void BattleRecOv_EncryptBody(SaveControl *save) {
    decryptVSPlayer(&gBattleRecVideo->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&gBattleRecVideo->body));
}

u32 BattleRecOv_SaveDataNumber(GameData *gameData, u64 dataNumber, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case SAVE_STEP_PREPARE:
        gBattleRecVideo->header.dataNumber = dataNumber;
        (*seq)++;
        break;
    case SAVE_STEP_WRITE:
        return BattleRec_SaveSlot(gameData, gBattleRecVideo, 0, saveSeq, heapId);
    }
    return 0;
}

u32 BattleRecOv_SaveSecureVideo(GameData *gameData, u32 slot, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case SAVE_STEP_PREPARE:
        gBattleRecVideo->header.secure = TRUE;
        gBattleRecVideo->header.magic = BATTLE_REC_MAGIC;
        gBattleRecVideo->header.crc = getCRC16(&gBattleRecVideo->header, BATTLE_REC_HEADER_CRC_SIZE);
        decryptVSPlayer(&gBattleRecVideo->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&gBattleRecVideo->body));
        (*seq)++;
        break;
    case SAVE_STEP_WRITE:
        return BattleRec_SaveSlot(gameData, gBattleRecVideo, slot, saveSeq, heapId);
    }
    return 0;
}

void BattleRecOv_StartDeleteVideo(GameData *gameData, HeapID heapId, u32 block) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    SaveControl_OpenExtraBlock(save, block, heapId);
    clearVSPlayerBlk(getAddressOfExtraSaveBlk(save, block, 0));
    GameData_StartSaveBlock(gameData, block);
}

u32 BattleRecOv_DeleteVideoMain(GameData *gameData, u32 block) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    u32 result = GameData_UpdateSaveBlock(gameData, block);

    if (result == SAVE_RESULT_OK || result == SAVE_RESULT_NG) {
        freeIntermediateSaveExtraBlksAfterLoad(save, block);
    }
    return result;
}

BattleRecVideo *BattleRecOv_GetVideo(void) {
    return gBattleRecVideo;
}
