// What the Global Data Server client and the Battle Recorder do with the loaded battle video on its way to the server
// and into the save. The ROM does not name the file: the name is a guess, after the Generation 4 games' battle_rec_ov.c,
// which holds the same functions after the GDS client's files.

#include "types.h"
#include "app/battle_rec_ov.h"
#include "gfl/std.h"
#include "save/battle_rec.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The key that a body is encrypted with, of its CRC
#define BATTLE_REC_KEY(body) ((body)->crc + (((body)->crc ^ 0xffff) << 16))

u32 BattleRecOv_SaveVideo(GameData *gameData, u32 slot, BOOL secure, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case 0:
        if (secure == TRUE) {
            data_02140f64->header.unk2A = secure;
            data_02140f64->header.magic = BATTLE_REC_MAGIC;
            data_02140f64->header.crc = getCRC16(&data_02140f64->header, BATTLE_REC_HEADER_CRC_SIZE);
        }
        decryptVSPlayer(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
        (*seq)++;
        break;
    case 1:
        return func_0200bdd4(gameData, data_02140f64, slot, saveSeq, heapId);
    }
    return 0;
}

void BattleRecOv_EncryptBody(SaveControl *save) {
    decryptVSPlayer(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
}

u32 BattleRecOv_SaveDataNumber(GameData *gameData, u64 dataNumber, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case 0:
        data_02140f64->header.dataNumber = dataNumber;
        (*seq)++;
        break;
    case 1:
        return func_0200bdd4(gameData, data_02140f64, 0, saveSeq, heapId);
    }
    return 0;
}

u32 BattleRecOv_SaveSecureVideo(GameData *gameData, u32 slot, u16 *seq, u16 *saveSeq, HeapID heapId) {
    switch (*seq) {
    case 0:
        data_02140f64->header.unk2A = TRUE;
        data_02140f64->header.magic = BATTLE_REC_MAGIC;
        data_02140f64->header.crc = getCRC16(&data_02140f64->header, BATTLE_REC_HEADER_CRC_SIZE);
        decryptVSPlayer(&data_02140f64->body, BATTLE_REC_BODY_CRC_SIZE, BATTLE_REC_KEY(&data_02140f64->body));
        (*seq)++;
        break;
    case 1:
        return func_0200bdd4(gameData, data_02140f64, slot, saveSeq, heapId);
    }
    return 0;
}

void BattleRecOv_StartDeleteVideo(GameData *gameData, HeapID heapId, u32 block) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    func_020074ec(save, block, heapId);
    clearVSPlayerBlk(getAddressOfExtraSaveBlk(save, block, 0));
    func_020178c4(gameData, block);
}

u32 BattleRecOv_DeleteVideoMain(GameData *gameData, u32 block) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    u32 result = func_020178f4(gameData, block);

    if (result == 2 || result == 3) {
        freeIntermediateSaveExtraBlksAfterLoad(save, block);
    }
    return result;
}

BattleRecVideo *BattleRecOv_GetVideo(void) {
    return data_02140f64;
}
