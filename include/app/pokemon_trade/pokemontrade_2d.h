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
BOOL Poketrade2D_BuildMarkingTablesFrom(PokemonTradeWork *wk, int box);
// Create the cell actor system for the trade demo, which has few sprites, and for the trade
void Poketrade2D_InitClactSystemMain(PokemonTradeWork *wk);
// Whether a species' name starts with an initial, from 0 for A
BOOL Poketrade2D_IsBoxIconMatch(u32 species, u32 initial);
void Poketrade2D_PlaceBoxIconActors(PokemonTradeWork *wk);
int Poketrade2D_GetScrollColumn(PokemonTradeWork *wk);
// Whether the cursor's column is on screen, and the third column on screen, where the cursor goes if not
BOOL Poketrade2D_GetVisibleColumnRange(PokemonTradeWork *wk, int *column);
void Poketrade2D_InitClactSystemSub(PokemonTradeWork *wk);
void Poketrade2D_LoadSubBackgroundGfx(PokemonTradeWork *wk);
void Poketrade2D_NoOp(PokemonTradeWork *wk);
void Poketrade2D_LoadSummaryBackground(PokemonTradeWork *wk, int side, int page);
void Poketrade2D_LoadBackgroundGfx(PokemonTradeWork *wk);
void Poketrade2D_FreeBackgroundPalette(PokemonTradeWork *wk);
void Poketrade2D_LoadMainGfxAndMessageWindow(PokemonTradeWork *wk);
void Poketrade2D_CreateFontResource(PokemonTradeWork *wk);
void Poketrade2D_FreeMainResources(PokemonTradeWork *wk);
void Poketrade2D_RegisterIconCharResources(PokemonTradeWork *wk);
void Poketrade2D_ClearSummaryBackground(PokemonTradeWork *wk);
void Poketrade2D_DrawBoxGridScreen(PokemonTradeWork *wk);
void Poketrade2D_CreateBoxWindows(PokemonTradeWork *wk);
void Poketrade2D_CreateBoxIconActors(PokemonTradeWork *wk);
void Poketrade2D_DeleteAllBoxIcons(PokemonTradeWork *wk);
void Poketrade2D_DeleteOfferIcons(PokemonTradeWork *wk);
void Poketrade2D_DeleteBoxWindows(PokemonTradeWork *wk);
void Poketrade2D_DrawBoxNames(PokemonTradeWork *wk);
// Greys an icon out or not
void Poketrade2D_SetIconObjMode(PokemonTradeWork *wk, ClActor *icon, BOOL grey);
// The Pokémon icon at a point of the screen. If asked, the box and slot of its Pokémon, NULL if the icon shows none,
// and the column of the strip, the row and the icon column it is in
ClActor *Poketrade2D_HitTestBoxIcons(PokemonTradeWork *wk, int x, int y, int *box, int *slot, int *column, int *row,
                                     int *index);
// Sets up the strip's icons for its scroll, loading their characters in the V-blank if asked
void Poketrade2D_UpdateVisibleColumns(BoxSaveAccessor *boxes, PokemonTradeWork *wk, BOOL async);
// Copies the characters of the icons of a box's Pokémon, or of the party's after the last box, one box a call
void Poketrade2D_LoadBoxIconGfx(PokemonTradeWork *wk, int box);
// Sets up the trade demo's BGs 6 and 7, with the graphics of the trade or of the other demo
void Poketrade2D_SetupSummaryBgs(PokemonTradeWork *wk, BOOL other);
void Poketrade2D_ExitSummaryBgs(PokemonTradeWork *wk);
void Poketrade2D_InitDisplayAndClact(PokemonTradeWork *wk);
void Poketrade2D_SetupMainBgs(PokemonTradeWork *wk);
void Poketrade2D_InitG3DSystem(PokemonTradeWork *wk);
void Poketrade2D_SetupSubBgs(PokemonTradeWork *wk);
void Poketrade2D_ExitSubBgs(PokemonTradeWork *wk);
// Sets the display up for the trade, or for the trade demo
void Poketrade2D_SetDisplayConfig(int config);
void Poketrade2D_RegisterMarkerResources(PokemonTradeWork *wk);
void Poketrade2D_ReleaseMarkerResources(PokemonTradeWork *wk);
void Poketrade2D_RegisterIconSpriteResources(PokemonTradeWork *wk);
void Poketrade2D_DeleteMarkingIcon(PokemonTradeSprite *sprite);
void Poketrade2D_LoadMainPalette(PokemonTradeWork *wk);
void Poketrade2D_CreateTouchbarActors(PokemonTradeWork *wk);
void Poketrade2D_DeleteTouchbarActors(PokemonTradeWork *wk);
void Poketrade2D_CreateMarkingIcon(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void Poketrade2D_CreatePokerusIcon(PokemonTradeWork *wk, int side, BOOL summary, PartyPkm *pkm);
void Poketrade2D_CreateSummaryIcons(PokemonTradeWork *wk, PartyPkm *pkm, BOOL isEgg);
void Poketrade2D_DeleteSummaryIcons(PokemonTradeWork *wk);
void Poketrade2D_UploadWindowPalette(PokemonTradeWork *wk, u32 palette, u32 type);
void Poketrade2D_CreateOfferWindows(PokemonTradeWork *wk);
void Poketrade2D_DeleteOfferWindowActors(PokemonTradeWork *wk);
void Poketrade2D_UpdateOfferWindowActors(PokemonTradeWork *wk);
void Poketrade2D_LoadSubScreenGfx(PokemonTradeWork *wk);
void Poketrade2D_ClearSubScreen(PokemonTradeWork *wk);
void Poketrade2D_PlaceTouchbarActors(PokemonTradeWork *wk, int side, BOOL level);
void Poketrade2D_DeleteOfferIconActor(PokemonTradeWork *wk, int side, int index);
void Poketrade2D_SetOfferIconsVisible(PokemonTradeWork *wk, int side, BOOL visible);
void Poketrade2D_CreateOfferIconActor(PokemonTradeWork *wk, int side, int index, PartyPkm *pkm, BOOL onMain,
                                      BOOL visible);
void Poketrade2D_UpdateOfferIconHighlight(PokemonTradeWork *wk, int side, int index);
void Poketrade2D_DeleteAllOfferIconActors(PokemonTradeWork *wk);
void Poketrade2D_CreateFloatingIconActor(PokemonTradeWork *wk, int column, int row, int x, int y, BoxPkm *pkm);
void Poketrade2D_DeleteFloatingIconActor(PokemonTradeWork *wk);
void Poketrade2D_StartIconFlight(PokemonTradeWork *wk);
BOOL Poketrade2D_UpdateIconFlight(PokemonTradeWork *wk);
void Poketrade2D_SetupSubBlend(PokemonTradeWork *wk);
void Poketrade2D_CreateMarkerActor(PokemonTradeWork *wk, int index);
void Poketrade2D_SetMarkerActorVisible(PokemonTradeWork *wk, int index, BOOL visible);
void Poketrade2D_DeleteMarkerActor(PokemonTradeWork *wk, int index);
void Poketrade2D_SetMarkerActorAnim(PokemonTradeWork *wk, u32 index);
void Poketrade2D_CreateMarkerButtonActor(PokemonTradeWork *wk, u32 index, int side);
void Poketrade2D_DeleteMarkerButtonPair(PokemonTradeWork *wk, int side);
void Poketrade2D_MovePickedIconActor(PokemonTradeWork *wk, int index);
void Poketrade2D_DeletePickedIconActor(PokemonTradeWork *wk);
BOOL Poketrade2D_StepCursorSlide(PokemonTradeWork *wk);
void Poketrade2D_SetupCursorBg(PokemonTradeWork *wk);
void Poketrade2D_LoadMonPalette(PokemonTradeWork *wk, int side, PartyPkm *pkm);
void Poketrade2D_CreateCursorActor(PokemonTradeWork *wk, u8 x, u8 y);
void Poketrade2D_DeleteCursorActor(PokemonTradeWork *wk);
BOOL Poketrade2D_ClearCursorIfDisabled(PokemonTradeWork *wk);
void Poketrade2D_FadePaletteAll(PokemonTradeWork *wk, BOOL dim);
// Set the planes of the main or the sub screen shown, in the next V-blank
void Poketrade2D_ScheduleMainPlaneVisible(PokemonTradeWork *wk, int planes);
void Poketrade2D_ScheduleSubPlaneVisible(PokemonTradeWork *wk, int planes);

#endif // POKEBW2_APP_POKEMON_TRADE_POKEMONTRADE_2D_H
