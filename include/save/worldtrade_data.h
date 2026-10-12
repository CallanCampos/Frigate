#ifndef POKEBW2_SAVE_WORLDTRADE_DATA_H
#define POKEBW2_SAVE_WORLDTRADE_DATA_H

#include "types.h"
#include "pml/poke_party.h"
#include "struct_decls.h"

// The Global Trade Station's save block, which keeps the Pokémon the player deposited. The header's name is a guess

struct WorldTradeData {
    // The deposited Pokémon
    PartyPkm pkm;
    u8 unkDC[0x10];
    u16 flag;
    u16 unkEE;
    u32 unkF0;
    u32 unkF4;
}; // size: 0xf8

u32 WorldTradeData_GetBlockSize(void);
void WorldTradeData_Init(WorldTradeData *data);
u16 func_0200b4a8(WorldTradeData *data);
void func_0200b4b0(WorldTradeData *data, u16 flag);
// Copies the deposited Pokémon out, or in
void WorldTradeData_GetPkm(WorldTradeData *data, PartyPkm *pkm);
PartyPkm *func_0200b4d0(WorldTradeData *data);
// Its callers pass the box the Pokémon came from as well, which it ignores
void WorldTradeData_SetPkm(WorldTradeData *data, PartyPkm *pkm, u32 box);
void func_0200b4ec(WorldTradeData *data, u32 value);
void func_0200b4f4(WorldTradeData *data, u32 value);
u16 func_0200b4fc(WorldTradeData *data);

#endif // POKEBW2_SAVE_WORLDTRADE_DATA_H
