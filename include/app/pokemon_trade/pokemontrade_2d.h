#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_2D_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_2D_H

#include "types.h"
#include "app/ui/ui_scene.h"
#include "gfl/clact.h"
#include "struct_decls.h"

// Overlay 139's OBJ resources, and the actor made of them
struct PokemonTradeSprite {
    UIObjRes res;
    ClActor *actor;
};

// Works out the colours of this machine's box, one a frame, and the party's after the last box; TRUE when that is
// done
BOOL func_ov194_021c2c04(PokemonTradeWork *wk, int box);
// Create the cell actor system for the trade demo, which has few sprites, and for the trade
void func_ov194_021c2c44(PokemonTradeWork *wk);
// Whether a species' name starts with an initial, from 0 for A
BOOL func_ov194_021c38a8(u32 species, u32 initial);
void func_ov194_021c3820(PokemonTradeWork *wk);
int func_ov194_021c3bc0(PokemonTradeWork *wk);
// Whether the cursor's column is on screen, and the third column on screen, where the cursor goes if not
BOOL func_ov194_021c3c10(PokemonTradeWork *wk, int *column);
void func_ov194_021c2c64(PokemonTradeWork *wk);
void func_ov194_021c2d34(PokemonTradeWork *wk);
void func_ov194_021c2d74(PokemonTradeWork *wk);
void func_ov194_021c2ef0(PokemonTradeWork *wk, int side, int page);
void func_ov194_021c2c84(PokemonTradeWork *wk);
void func_ov194_021c2d0c(PokemonTradeWork *wk);
void func_ov194_021c2d78(PokemonTradeWork *wk);
void func_ov194_021c2de8(PokemonTradeWork *wk);
void func_ov194_021c2e04(PokemonTradeWork *wk);
void func_ov194_021c2e6c(PokemonTradeWork *wk);
void func_ov194_021c2f78(PokemonTradeWork *wk);
void func_ov194_021c30b8(PokemonTradeWork *wk);
void func_ov194_021c3224(PokemonTradeWork *wk);
void func_ov194_021c3480(PokemonTradeWork *wk);
void func_ov194_021c368c(PokemonTradeWork *wk);
void func_ov194_021c36e4(PokemonTradeWork *wk);
void func_ov194_021c3374(PokemonTradeWork *wk);
void func_ov194_021c339c(PokemonTradeWork *wk);
// Greys an icon out or not
void func_ov194_021c38bc(PokemonTradeWork *wk, ClActor *icon, BOOL grey);
// The Pokémon icon at a point of the screen. If asked, the box and slot of its Pokémon, NULL if the icon shows none,
// and the column of the strip, the row and the icon column it is in
ClActor *func_ov194_021c3fa8(PokemonTradeWork *wk, int x, int y, int *box, int *slot, int *column, int *row,
                             int *index);
// Sets up the strip's icons for its scroll, loading their characters in the V-blank if asked
void func_ov194_021c3c68(BoxSaveAccessor *boxes, PokemonTradeWork *wk, BOOL async);
// Copies the characters of the icons of a box's Pokémon, or of the party's after the last box, one box a call
void func_ov194_021c3e9c(PokemonTradeWork *wk, int box);
// Sets up the trade demo's BGs 6 and 7, with the graphics of the trade or of the other demo
void func_ov194_021c4088(PokemonTradeWork *wk, BOOL other);
void func_ov194_021c41d0(PokemonTradeWork *wk);
void func_ov194_021c41fc(PokemonTradeWork *wk);
void func_ov194_021c4234(PokemonTradeWork *wk);
void func_ov194_021c43c0(PokemonTradeWork *wk);
void func_ov194_021c4484(PokemonTradeWork *wk);
void func_ov194_021c45a8(PokemonTradeWork *wk);
// Sets the display up for the trade, or for the trade demo
void func_ov194_021c45ec(int config);
void func_ov194_021c4600(PokemonTradeWork *wk);
void func_ov194_021c466c(PokemonTradeWork *wk);
void func_ov194_021c46a4(PokemonTradeWork *wk);
void func_ov194_021c4cfc(PokemonTradeSprite *sprite);
void func_ov194_021c475c(PokemonTradeWork *wk);
void func_ov194_021c479c(PokemonTradeWork *wk);
void func_ov194_021c49e8(PokemonTradeWork *wk);
void func_ov194_021c4c00(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void func_ov194_021c4d18(PokemonTradeWork *wk, int side, BOOL summary, PartyPkm *pkm);
void func_ov194_021c4ec0(PokemonTradeWork *wk, PartyPkm *pkm, BOOL isEgg);
void func_ov194_021c5060(PokemonTradeWork *wk);
void func_ov194_021c5098(PokemonTradeWork *wk, u32 palette, u32 type);
void func_ov194_021c5abc(PokemonTradeWork *wk);
void func_ov194_021c5bf0(PokemonTradeWork *wk);
void func_ov194_021c5c80(PokemonTradeWork *wk);
void func_ov194_021c4a68(PokemonTradeWork *wk);
void func_ov194_021c4b88(PokemonTradeWork *wk);
void func_ov194_021c4970(PokemonTradeWork *wk, int side, BOOL level);
void func_ov194_021c50d8(PokemonTradeWork *wk, int side, int index);
void func_ov194_021c510c(PokemonTradeWork *wk, int side, BOOL visible);
void func_ov194_021c5138(PokemonTradeWork *wk, int side, int index, PartyPkm *pkm, BOOL onMain, BOOL visible);
void func_ov194_021c5244(PokemonTradeWork *wk, int side, int index);
void func_ov194_021c52bc(PokemonTradeWork *wk);
void func_ov194_021c52e0(PokemonTradeWork *wk, int column, int row, int x, int y, BoxPkm *pkm);
void func_ov194_021c54ec(PokemonTradeWork *wk);
void func_ov194_021c5348(PokemonTradeWork *wk);
BOOL func_ov194_021c5460(PokemonTradeWork *wk);
void func_ov194_021c5504(PokemonTradeWork *wk);
void func_ov194_021c551c(PokemonTradeWork *wk, int index);
void func_ov194_021c5594(PokemonTradeWork *wk, int index, BOOL visible);
void func_ov194_021c55ac(PokemonTradeWork *wk, int index);
void func_ov194_021c55c8(PokemonTradeWork *wk, u32 index);
void func_ov194_021c55e4(PokemonTradeWork *wk, u32 index, int side);
void func_ov194_021c56b8(PokemonTradeWork *wk, int side);
void func_ov194_021c56f8(PokemonTradeWork *wk, int index);
void func_ov194_021c57a8(PokemonTradeWork *wk);
BOOL func_ov194_021c57c4(PokemonTradeWork *wk);
void func_ov194_021c5994(PokemonTradeWork *wk);
void func_ov194_021c5d10(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void func_ov194_021c5dd8(PokemonTradeWork *wk, u8 x, u8 y);
void func_ov194_021c5e5c(PokemonTradeWork *wk);
BOOL func_ov194_021c5e80(PokemonTradeWork *wk);
void func_ov194_021c5fe4(PokemonTradeWork *wk, BOOL dim);
// Set the planes of the main or the sub screen shown, in the next V-blank
void func_ov194_021c5ff4(PokemonTradeWork *wk, int planes);
void func_ov194_021c600c(PokemonTradeWork *wk, int planes);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_2D_H
