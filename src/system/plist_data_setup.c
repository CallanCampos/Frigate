#include "types.h"
#include "app/pokelist.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/mail.h"
#include "save/save_control.h"
#include "save/shortcut.h"
#include "system/game_data.h"

// Sets up the Pokémon list's parameters. Our names

void PokeListParam_Setup(PokeListParam *param, GameData *gameData, u32 mode, PokeParty *party) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    MIi_CpuClear32(0, param, sizeof(PokeListParam));
    param->mode = mode;
    param->party = party;
    param->bag = GameData_GetBag(gameData);
    param->unk08 = func_02009790(gameData);
    param->trainerData = getTrainerDataBlkAddress(save);
    param->reshZek = getReshZekBlkAddress(save);
    param->pokedex = GameData_GetPokedex(gameData);
    param->shortcut = SaveControl_GetShortcutSaveCore(save);
    param->trainerCard = getTrainerCardDataBlkAddress(gameData);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->season = GameData_GetSeason(gameData);
    param->unk48 = 0;
    param->result = 0;
    param->keyItemRegistered = FALSE;
}

PokeListParam *PokeListParam_Create(GameData *gameData, u32 mode, PokeParty *party, HeapID heapId) {
    PokeListParam *param = GFL_HeapAllocate(heapId, sizeof(PokeListParam), FALSE, "plist_data_setup.c", 84);

    PokeListParam_Setup(param, gameData, mode, party);
    return param;
}
