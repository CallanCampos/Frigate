#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_PROC_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_PROC_H

#include "types.h"
#include "struct_decls.h"

// A Pokémon's icon in the strip
struct TradeBoxEntry {
    u16 species;
    u8 form;
    u8 sex;
};

typedef void (*PokemonTradeState)(PokemonTradeWork *wk);

void func_ov194_021b76e0(PokemonTradeWork *wk);
void func_ov194_021b772c(PokemonTradeWork *wk);
BOOL func_ov194_021b774c(const u8 *data);
u8 func_ov194_021b7778(PokemonTradeWork *wk);
// Whether the trade is with another machine
BOOL PokemonTrade_IsNetwork(PokemonTradeWork *wk);
BOOL PokemonTrade_IsNegoType(PokemonTradeWork *wk);
BOOL func_ov194_021b783c(PokemonTradeWork *wk);
int PokemonTrade_GetColumnBox(int a, PokemonTradeWork *wk);
int PokemonTrade_GetColumnSlot(int a, int b);
PartyPkm *PokemonTrade_CopyPartyPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk);
BoxPkm *PokemonTrade_GetBoxPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk);
TradeBoxEntry *PokemonTrade_GetBoxEntry(int box, int slot, PokemonTradeWork *wk);
void PokemonTrade_SetState(PokemonTradeWork *wk, PokemonTradeState state);
u32 func_ov194_021b7c80(PokemonTradeWork *wk, int side);
void func_ov194_021b7cc0(PokemonTradeWork *wk, int side, u32 value);
// The Pokémon of a side of the trade: this machine's (0) or the other's (1)
PartyPkm *PokemonTrade_GetPkm(PokemonTradeWork *wk, int side);
void PokemonTrade_FadeOutToEnd(PokemonTradeWork *wk);
void func_ov194_021b81c8(PokemonTradeWork *wk, int side, PartyPkm *pkm);
BOOL func_ov194_021b8234(PokemonTradeWork *wk);
BOOL func_ov194_021b8330(PokemonTradeWork *wk);
BOOL func_ov194_021b8350(PokemonTradeWork *wk);
BOOL func_ov194_021b8370(PokemonTradeWork *wk);
void func_ov194_021b9c44(PokemonTradeWork *wk);
void func_ov194_021b9a38(PokemonTradeWork *wk);
void func_ov194_021ba924(PokemonTradeWork *wk);

void func_ov194_021b94c0(PokemonTradeWork *wk);
void func_ov194_021bb4b4(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_PROC_H
