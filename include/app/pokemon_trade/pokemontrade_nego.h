#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H

#include "types.h"
#include "struct_decls.h"

// Whether a Pokémon can't be offered in a negotiation
BOOL func_ov194_021bbe60(PokemonTradeWork *wk, BoxPkm *pkm);
void func_ov194_021bbf18(PokemonTradeWork *wk);
void func_ov194_021bbf5c(PokemonTradeWork *wk);
BOOL func_ov194_021bbff0(PokemonTradeWork *wk);
void func_ov194_021bc038(PokemonTradeWork *wk, u32 a1, u32 a2);
BOOL func_ov194_021bc04c(PokemonTradeWork *wk);
BOOL func_ov194_021bc098(PokemonTradeWork *wk);
// The index of a Pokémon among the ones this player offers, or -1
int func_ov194_021bc0f0(PokemonTradeWork *wk, int box, int slot);
void func_ov194_021bc124(PokemonTradeWork *wk, int index, PartyPkm *pkm);
int func_ov194_021bc178(PokemonTradeWork *wk, int side, int slot, int box);
void func_ov194_021bc29c(PokemonTradeWork *wk, int side, int index);
void func_ov194_021bc2d0(PokemonTradeWork *wk, u32 vram);
void func_ov194_021bc330(PokemonTradeWork *wk);
BOOL func_ov194_021bc35c(PokemonTradeWork *wk, int slot, int box);
void func_ov194_021bc434(PokemonTradeWork *wk);
void func_ov194_021bc6b4(PokemonTradeWork *wk, int side, u32 msg, BOOL force);
void func_ov194_021bc784(PokemonTradeWork *wk);
void func_ov194_021be380(PokemonTradeWork *wk);
BOOL func_ov194_021be3bc(PokemonTradeWork *wk, int column, int row);
BOOL func_ov194_021be45c(PokemonTradeWork *wk, int column, int row);
void func_ov194_021be4b0(PokemonTradeWork *wk);
void func_ov194_021be534(PokemonTradeWork *wk);
void func_ov194_021be554(PokemonTradeWork *wk, BOOL a1);
void func_ov194_021be578(PokemonTradeWork *wk);
void func_ov194_021be598(PokemonTradeWork *wk);
void func_ov194_021be610(PokemonTradeWork *wk);
void func_ov194_021be648(PokemonTradeWork *wk, u32 index, int side);
void func_ov194_021be688(PokemonTradeWork *wk);
void func_ov194_021be6ac(PokemonTradeWork *wk);
void func_ov194_021be6c0(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void func_ov194_021be720(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H
