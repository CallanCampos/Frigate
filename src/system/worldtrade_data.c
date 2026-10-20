#include "types.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "save/worldtrade_data.h"

// The Global Trade Station's save block, which keeps the Pokémon the player deposited. The ROM has no string for
// the file, so the name is a guess (swan's SaveControl_GetWorldTradeData). Block 0x2e.

#define SAVE_BLOCK_WORLDTRADE 0x2e

WorldTradeData *SaveControl_GetWorldTradeData(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_WORLDTRADE);
}

u32 WorldTradeData_GetBlockSize(void) {
    return sizeof(WorldTradeData);
}

void WorldTradeData_Init(WorldTradeData *data) {
    MIi_CpuClearFast(0, data, sizeof(WorldTradeData));
}

u16 WorldTradeData_GetFlag(WorldTradeData *data) {
    return data->flag;
}

void WorldTradeData_SetFlag(WorldTradeData *data, u16 flag) {
    data->flag = flag;
}

void WorldTradeData_GetPkm(WorldTradeData *data, PartyPkm *pkm) {
    MI_CpuCopy8(&data->pkm, pkm, PokeParty_GetPkmRawSize());
}

PartyPkm *WorldTradeData_GetPkmPtr(WorldTradeData *data) {
    return &data->pkm;
}

void WorldTradeData_SetPkm(WorldTradeData *data, PartyPkm *pkm, u32 box) {
    MI_CpuCopy8(pkm, &data->pkm, PokeParty_GetPkmRawSize());
}

void func_0200b4ec(WorldTradeData *data, u32 value) {
    data->depositTradeDate = value;
}

void func_0200b4f4(WorldTradeData *data, u32 value) {
    data->exchangeDate = value;
}

u16 func_0200b4fc(WorldTradeData *data) {
    return data->unkEE;
}
