#ifndef POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H
#define POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H

#include "types.h"
#include "struct_decls.h"

void PoketradeSave_OpenMessageWindow(PokemonTradeWork *wk, BOOL instant, u32 x, u32 y, u32 width, u32 height);
void PoketradeSave_OpenMessageWindowAtPos(PokemonTradeWork *wk, BOOL instant, BOOL bottom);
void PoketradeSave_OpenMessageWindowTop(PokemonTradeWork *wk);
void PoketradeSave_ShowTimeIcon(PokemonTradeWork *wk);
void PoketradeSave_CloseMessageWindow(PokemonTradeWork *wk);
void PoketradeSave_CloseMessageWindowClear(PokemonTradeWork *wk);
void PoketradeSave_CreateMessageResources(PokemonTradeWork *wk);
void PoketradeSave_DeleteMessageResources(PokemonTradeWork *wk);
BOOL PoketradeSave_IsMessagePrintDone(PokemonTradeWork *wk);
void PoketradeSave_CloseTaskMenu(PokemonTradeWork *wk);
void PoketradeSave_OpenTaskMenuAt(PokemonTradeWork *wk, const u32 *items, int count, u32 right, u32 bottom);
void PoketradeSave_OpenTaskMenu(PokemonTradeWork *wk, const u32 *items, int count);
void PoketradeSave_DrawOfferCardWindow(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void PoketradeSave_DeleteOfferStatusIcons(PokemonTradeWork *wk);
void PoketradeSave_CloseSummaryPage(PokemonTradeWork *wk, BOOL hideWindows);
void PoketradeSave_OpenSummaryScreen(PokemonTradeWork *wk, PartyPkm *pkm);
void PoketradeSave_DrawSummaryPage(PokemonTradeWork *wk, int page, PartyPkm *pkm);
void PoketradeSave_ShowSummaryMon(PokemonTradeWork *wk, PartyPkm *pkm, int side, BOOL reload);
void PoketradeSave_DeleteOfferCardWindow(PokemonTradeWork *wk, int side);
void PoketradeSave_CloseSummaryWindow(PokemonTradeWork *wk, BOOL clear);
void PoketradeSave_DrawOfferListWindows(PokemonTradeWork *wk, u32 a1);
void PoketradeSave_DeleteOfferListWindows(PokemonTradeWork *wk);
void PoketradeSave_SetupWindowRegsSummary(PokemonTradeWork *wk);
void PoketradeSave_DrawPartnerOfferCard(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void PoketradeSave_DeletePartnerOfferCard(PokemonTradeWork *wk, int side);
AppTaskMenuWin *PoketradeSave_OpenTaskMenuWindow(PokemonTradeWork *wk, u32 msg);
void PoketradeSave_CloseTaskMenuWindow(PokemonTradeWork *wk, BOOL showActor, BOOL showButton);
BOOL PoketradeSave_IsTouchOutsideArea(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_MESSAGE_H
