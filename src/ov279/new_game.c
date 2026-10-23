#include "types.h"
#include "constants/items.h"
#include "gfl/random.h"
#include "save/bag.h"
#include "save/cygnus_save_control.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/new_game.h"

// Overlay 279 is only loaded to set up the save data of a new game

void InitItemBag_(GameData *gameData, u32 heapId);
void InitDreamRadarFlagSave_(GameData *gameData);

void InitItemBag_(GameData *gameData, u32 heapId) {
    BagSave *bag = GameData_GetBag(gameData);
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);

    BagSave_Init(bag);
    if (getTrainerGender(player) == GENDER_MALE) {
        BagSave_AddItem(bag, ITEM_XTRANSCEIVER_MALE, 1, heapId);
    } else {
        BagSave_AddItem(bag, ITEM_XTRANSCEIVER_FEMALE, 1, heapId);
    }
    BagSave_AddItem(bag, ITEM_PAL_PAD, 1, heapId);
}

void InitDreamRadarFlagSave_(GameData *gameData) {
    DreamRadarSave *save = GetDreamRadarSaveBlock(GameData_GetSaveControl(gameData));

    // Value 1 gets a random 32-bit number, the others are cleared
    SetDreamRadarFlag(save, DREAM_RADAR_FLAG_SEED, GFL_RandomLC(0));
    SetDreamRadarFlag(save, DREAM_RADAR_FLAG_READ, 0);
    SetDreamRadarFlag(save, DREAM_RADAR_FLAG_RECEIVED, 0);
}

void InitItemBag(GameData *gameData, u32 heapId) {
    InitItemBag_(gameData, heapId);
}

void InitDreamRadarFlagSave(GameData *gameData, u32 heapId) {
    InitDreamRadarFlagSave_(gameData);
}
