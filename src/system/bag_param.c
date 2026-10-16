#include "types.h"
#include "app/bag.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The bag's parameters. Our names

BagProcessData *BagParam_Create(GameData *gameData, const PlayerActionPerms *perms, u32 mode, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    BagProcessData *param = GFL_HeapAllocate(heapId, sizeof(BagProcessData), TRUE, "bag_param.c", 57);

    param->gameData = gameData;
    param->trainerData = getTrainerDataBlkAddress(save);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->cursor = func_0201734c(gameData);
    param->bag = GameData_GetBag(gameData);
    param->freeSpaceFilter = func_020088e8(param->cursor);
    if (perms != NULL) {
        MI_CpuCopy8(perms, &param->perms, sizeof(PlayerActionPerms));
    }
    param->mode = mode;
    param->isCycling = FieldPlayerState_GetExState(GameData_GetPlayerState(gameData)) == FLD_PLAYER_EXSTATE_CYCLING;
    param->dowsingActive = GameData_GetLastSubscreen(gameData) == 6;
    return param;
}
