#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H

#include "types.h"
#include "struct_decls.h"

// Whether a Pokémon can't be offered in a negotiation
BOOL PokemonTrade_CanTradeMon(PokemonTradeWork *wk, BoxPkm *pkm);
void PoketradeNego_FreeOfferMons(PokemonTradeWork *wk);
void PoketradeNego_ClearOfferSlots(PokemonTradeWork *wk);
BOOL PoketradeNego_CheckCancelTouch(PokemonTradeWork *wk);
void PoketradeNego_CloseCancelMenu(PokemonTradeWork *wk, u32 a1, u32 a2);
BOOL PoketradeNego_CanConfirmOffers(PokemonTradeWork *wk);
BOOL PoketradeNego_HasOffer(PokemonTradeWork *wk);
// The index of a Pokémon among the ones this player offers, or -1
int PoketradeNego_FindOfferIndex(PokemonTradeWork *wk, int box, int slot);
void PoketradeNego_StoreRemoteOffer(PokemonTradeWork *wk, int index, PartyPkm *pkm);
int PoketradeNego_FindFreeOfferIndex(PokemonTradeWork *wk, int side, int slot, int box);
void PoketradeNego_RemoveOffer(PokemonTradeWork *wk, int side, int index);
void PoketradeNego_RegisterIconResources(PokemonTradeWork *wk, u32 vram);
void PoketradeNego_ReleaseIconResources(PokemonTradeWork *wk);
BOOL PoketradeNego_AddSelectedAsOffer(PokemonTradeWork *wk, int slot, int box);
void PoketradeNego_DrawPlayerNames(PokemonTradeWork *wk);
void PoketradeNego_DrawOfferStatusLabel(PokemonTradeWork *wk, int side, u32 msg, BOOL force);
void PoketradeNego_DeleteHeaderWindows(PokemonTradeWork *wk);
void PoketradeNego_StepShowCancelMessage(PokemonTradeWork *wk);
BOOL PoketradeNego_IsMonOwnOffer(PokemonTradeWork *wk, int column, int row);
BOOL PoketradeNego_IsMonPeerSelected(PokemonTradeWork *wk, int column, int row);
void PoketradeNego_StepSendOfferRemoval(PokemonTradeWork *wk);
void PoketradeNego_InitOfferMarkers(PokemonTradeWork *wk);
void PoketradeNego_SetOfferMarkersState(PokemonTradeWork *wk, BOOL a1);
void PoketradeNego_UpdateOfferMarkers(PokemonTradeWork *wk);
void PoketradeNego_PollMarkerTouch(PokemonTradeWork *wk);
void PoketradeNego_TickMarkerTimers(PokemonTradeWork *wk);
void PoketradeNego_FlashMarker(PokemonTradeWork *wk, u32 index, int side);
void PoketradeNego_ResetMarkers(PokemonTradeWork *wk);
void PoketradeNego_ExitMarkers(PokemonTradeWork *wk);
void PoketradeNego_ShowOfferSlotMon(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void PoketradeNego_TickOfferSlotTimers(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_NEGO_H
