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

void PokemonTrade_ResetTouchbarBackButton(PokemonTradeWork *wk);
void PokemonTrade_ClearNetAutoExit(PokemonTradeWork *wk);
BOOL PokemonTrade_IsMonValid(const u8 *data);
u8 PokemonTrade_GetStoredBoxIndex(PokemonTradeWork *wk);
// Whether the trade is with another machine
BOOL PokemonTrade_IsNetwork(PokemonTradeWork *wk);
BOOL PokemonTrade_IsNegoType(PokemonTradeWork *wk);
BOOL PokemonTrade_IsWifiMode(PokemonTradeWork *wk);
int PokemonTrade_GetColumnBox(int a, PokemonTradeWork *wk);
int PokemonTrade_GetColumnSlot(int a, int b);
PartyPkm *PokemonTrade_CopyPartyPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk);
BoxPkm *PokemonTrade_GetBoxPkm(BoxSaveAccessor *boxes, int box, int slot, PokemonTradeWork *wk);
TradeBoxEntry *PokemonTrade_GetBoxEntry(int box, int slot, PokemonTradeWork *wk);
void PokemonTrade_SetState(PokemonTradeWork *wk, PokemonTradeState state);
u32 PokemonTrade_GetSideValue(PokemonTradeWork *wk, int side);
void PokemonTrade_SetSideValue(PokemonTradeWork *wk, int side, u32 value);
// The Pokémon of a side of the trade: this machine's (0) or the other's (1)
PartyPkm *PokemonTrade_GetPkm(PokemonTradeWork *wk, int side);
void PokemonTrade_FadeOutToEnd(PokemonTradeWork *wk);
void PokemonTrade_SetSideMon(PokemonTradeWork *wk, int side, PartyPkm *pkm);
BOOL PokemonTrade_IsSelfMonReady(PokemonTradeWork *wk);
BOOL PokemonTrade_IsCurrentSideMonInvalid(PokemonTradeWork *wk);
BOOL PokemonTrade_IsOtherSideMonInvalid(PokemonTradeWork *wk);
BOOL PokemonTrade_IsMonTradeable(PokemonTradeWork *wk);
void PokemonTrade_StepShowWaitMessageA(PokemonTradeWork *wk);
void PokemonTrade_StepResetForNewRound(PokemonTradeWork *wk);
void PokemonTrade_StepRebuildBrowseScene(PokemonTradeWork *wk);

void PokemonTrade_StepWaitCursorSync(PokemonTradeWork *wk);
void PokemonTrade_DeleteAllResources(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_PROC_H
