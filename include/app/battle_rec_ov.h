#ifndef POKEBW2_APP_BATTLE_REC_OV_H
#define POKEBW2_APP_BATTLE_REC_OV_H

#include "types.h"
#include "gfl/heap.h"
#include "save/battle_rec.h"
#include "struct_decls.h"

// battle_rec_ov.c in overlay 201: what the Global Data Server client and the Battle Recorder do with the loaded battle
// video (battle_rec.c in main) on its way to the server and into the save. The ROM does not name the file: the name
// is a guess, after the Generation 4 games' file of the same functions. Our names

// Saves the loaded video to a slot, a step each call: 0 while it runs, then the save's result (2 or 3). The first
// step encrypts the body again, after marking the video as one that may be uploaded if secure is set
u32 BattleRecOv_SaveVideo(GameData *gameData, u32 slot, BOOL secure, u16 *seq, u16 *saveSeq, HeapID heapId);
// Encrypts the loaded video's body
void BattleRecOv_EncryptBody(SaveControl *save);
// Gives the loaded video the number that the server gave it, and saves it to the player's own slot, a step each call
u32 BattleRecOv_SaveDataNumber(GameData *gameData, u64 dataNumber, u16 *seq, u16 *saveSeq, HeapID heapId);
// Saves the loaded video to a slot as one that may be uploaded, a step each call
u32 BattleRecOv_SaveSecureVideo(GameData *gameData, u32 slot, u16 *seq, u16 *saveSeq, HeapID heapId);
// Starts clearing the video of an extra save block, and runs the save, a step each call: the save's result
void BattleRecOv_StartDeleteVideo(GameData *gameData, HeapID heapId, u32 block);
u32 BattleRecOv_DeleteVideoMain(GameData *gameData, u32 block);
// The loaded video
BattleRecVideo *BattleRecOv_GetVideo(void);

#endif // POKEBW2_APP_BATTLE_REC_OV_H
