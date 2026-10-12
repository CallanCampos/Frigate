#include "types.h"
#include "save/worldtrade_data.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/save_control.h"

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
    sys_memset32_fast(0, data, sizeof(WorldTradeData));
}

u16 func_0200b4a8(WorldTradeData *data) {
    return data->flag;
}

void func_0200b4b0(WorldTradeData *data, u16 flag) {
    data->flag = flag;
}

void WorldTradeData_GetPkm(WorldTradeData *data, PartyPkm *pkm) {
    sys_memcpy(&data->pkm, pkm, PokeParty_GetPkmRawSize());
}

PartyPkm *func_0200b4d0(WorldTradeData *data) {
    return &data->pkm;
}

void WorldTradeData_SetPkm(WorldTradeData *data, PartyPkm *pkm, u32 box) {
    sys_memcpy(pkm, &data->pkm, PokeParty_GetPkmRawSize());
}

void func_0200b4ec(WorldTradeData *data, u32 value) {
    data->unkF0 = value;
}

void func_0200b4f4(WorldTradeData *data, u32 value) {
    data->unkF4 = value;
}

u16 func_0200b4fc(WorldTradeData *data) {
    return data->unkEE;
}
