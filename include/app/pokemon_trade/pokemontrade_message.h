#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H

#include "types.h"
#include "struct_decls.h"

void func_ov194_021bfcf8(PokemonTradeWork *wk, BOOL instant, u32 x, u32 y, u32 width, u32 height);
void func_ov194_021bfdf8(PokemonTradeWork *wk, BOOL instant, BOOL bottom);
void func_ov194_021bfe28(PokemonTradeWork *wk);
void func_ov194_021bfe34(PokemonTradeWork *wk);
void func_ov194_021bfe70(PokemonTradeWork *wk);
void func_ov194_021bfe9c(PokemonTradeWork *wk);
void func_ov194_021bfedc(PokemonTradeWork *wk);
void func_ov194_021bffac(PokemonTradeWork *wk);
BOOL func_ov194_021c00b0(PokemonTradeWork *wk);
void func_ov194_021c00fc(PokemonTradeWork *wk);
void func_ov194_021c0120(PokemonTradeWork *wk, const u32 *items, int count, u32 right, u32 bottom);
void func_ov194_021c0214(PokemonTradeWork *wk, const u32 *items, int count);
void func_ov194_021c0918(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void func_ov194_021c0aac(PokemonTradeWork *wk);
void func_ov194_021c0aec(PokemonTradeWork *wk, BOOL hideWindows);
void func_ov194_021c0b6c(PokemonTradeWork *wk, PartyPkm *pkm);
void func_ov194_021c0c04(PokemonTradeWork *wk, int page, PartyPkm *pkm);
void func_ov194_021c0fa0(PokemonTradeWork *wk, PartyPkm *pkm, int side, BOOL reload);
void func_ov194_021c123c(PokemonTradeWork *wk, int side);
void func_ov194_021c1288(PokemonTradeWork *wk, BOOL clear);
void func_ov194_021c12ec(PokemonTradeWork *wk, u32 a1);
void func_ov194_021c1484(PokemonTradeWork *wk);
void func_ov194_021c14b0(PokemonTradeWork *wk);
void func_ov194_021c1530(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void func_ov194_021c1740(PokemonTradeWork *wk, int side);
AppTaskMenuWin *func_ov194_021c1788(PokemonTradeWork *wk, u32 msg);
void func_ov194_021c1820(PokemonTradeWork *wk, BOOL showActor, BOOL showButton);
BOOL func_ov194_021c1884(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H
