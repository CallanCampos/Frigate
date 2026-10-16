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

void func_02008da4(TrainerDataSave *data) {
    MIi_CpuClear32(0, data, sizeof(TrainerDataSave));
    func_020089c0(&data->config);
    func_02008b40(&data->playerInfo);
    func_02008c40(&data->playTime);
    data->unk2C = 0;
}

PlayerInfo *SaveControl_GetPlayerInfo(SaveControl *save) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return &data->playerInfo;
}

TrainerDataSave *getTrainerDataBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
}

PlayTime *func_02008de8(SaveControl *save) {
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

BOOL func_02008e24(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return data->markers[index] == TRAINER_DATA_MARKER_SET;
}

void func_02008e48(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->markers[index] = TRAINER_DATA_MARKER_SET;
}

void func_02008e60(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->markers[index] = 0;
}

u32 *func_02008e74(SaveControl *save, u32 index) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    return &data->unk5C[index];
}

void func_02008e88(SaveControl *save, PlayerState *state) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    data->playerInfo = state->playerInfo;
}

void func_02008ea8(SaveControl *save, PlayerState *state) {
    TrainerDataSave *data = SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_DATA);
    state->playerInfo = data->playerInfo;
}
