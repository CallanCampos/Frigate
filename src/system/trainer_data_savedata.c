#include "types.h"
#include "field/player_state.h"
#include "nitro/mi.h"
#include "save/save_control.h"
#include "save/trainer_data_savedata.h"

// The save block with the options, the player's info and the play time. The ROM has no string for the file, so the
// name is a guess. Block 0x1b.

#define SAVE_BLOCK_TRAINER_DATA 0x1b

u32 getSizeofTrainerData(void) {
    return sizeof(TrainerDataSave);
}

void TrainerDataSave_Init(TrainerDataSave *data) {
    MIi_CpuClear32(0, data, sizeof(TrainerDataSave));
    Config_Init(&data->config);
    PlayerInfo_Init(&data->playerInfo);
    PlayTime_Init(&data->playTime);
    data->unk2C = 0;
}

PlayerInfo *SaveControl_GetPlayerInfo(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return &data->playerInfo;
}

TrainerDataSave *getTrainerDataBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
}

PlayTime *SaveControl_GetPlayTime(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return &data->playTime;
}

u8 func_02008df4(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return data->unk2C;
}

void func_02008e04(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->unk2C = 1;
}

void func_02008e14(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->unk2C = 0;
}

BOOL TrainerDataSave_IsMarkerSet(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return data->markers[index] == TRAINER_DATA_MARKER_SET;
}

void TrainerDataSave_SetMarker(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->markers[index] = TRAINER_DATA_MARKER_SET;
}

void TrainerDataSave_ClearMarker(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->markers[index] = 0;
}

u32 *func_02008e74(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return &data->unk5C[index];
}

void TrainerDataSave_StorePlayerInfo(SaveControl *save, PlayerState *state) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->playerInfo = state->playerInfo;
}

void TrainerDataSave_LoadPlayerInfo(SaveControl *save, PlayerState *state) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    state->playerInfo = data->playerInfo;
}
