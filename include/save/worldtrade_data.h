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
    // When the deposited Pokémon was traded away, and when the player last traded for a deposited one, as
    // year << 24 | month << 16 | day << 8 | week
    u32 depositTradeDate;
    u32 exchangeDate;
}; // size: 0xf8

u32 WorldTradeData_GetBlockSize(void);
void WorldTradeData_Init(WorldTradeData *data);
u16 WorldTradeData_GetFlag(WorldTradeData *data);
void WorldTradeData_SetFlag(WorldTradeData *data, u16 flag);
// Copies the deposited Pokémon out, or in
void WorldTradeData_GetPkm(WorldTradeData *data, PartyPkm *pkm);
PartyPkm *WorldTradeData_GetPkmPtr(WorldTradeData *data);
// Its callers pass the box the Pokémon came from as well, which it ignores
void WorldTradeData_SetPkm(WorldTradeData *data, PartyPkm *pkm, u32 box);
void WorldTradeData_SetDepositTradeDate(WorldTradeData *data, u32 value);
void WorldTradeData_SetExchangeDate(WorldTradeData *data, u32 value);
u16 func_0200b4fc(WorldTradeData *data);

#endif // POKEBW2_SAVE_WORLDTRADE_DATA_H
