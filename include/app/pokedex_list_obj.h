#ifndef POKEBW2_APP_POKEDEX_LIST_OBJ_H
#define POKEBW2_APP_POKEDEX_LIST_OBJ_H

#include "types.h"
#include "app/pokedex_list_main.h"

// pokedex_list_obj.c: the actors of the Pokédex's list. The ROM doesn't name this file; pokedex_list_obj.c is a guess
// after the search screen's pokedex_search_obj.c. The names are ours

void PokedexListObj_Init(PokedexListWork *wk);
void PokedexListObj_Exit(PokedexListWork *wk);
void PokedexListObj_Update(PokedexListWork *wk);
void PokedexListObj_SetAnmSeq(PokedexListWork *wk, u32 id, u32 anm);
void PokedexListObj_EndAnm(PokedexListWork *wk, u32 id);
// Sets an actor's animation and plays it
void PokedexListObj_SetAnm(PokedexListWork *wk, u32 id, u32 anm);
u32 PokedexListObj_GetAnm(PokedexListWork *wk, u32 id);
BOOL PokedexListObj_IsAnmPlaying(PokedexListWork *wk, u32 id);
void PokedexListObj_SetVisible(PokedexListWork *wk, u32 id, BOOL visible);
BOOL PokedexListObj_IsVisible(PokedexListWork *wk, u32 id);
void PokedexListObj_SetBlend(PokedexListWork *wk, u32 id, BOOL blend);
void PokedexListObj_SetPos(PokedexListWork *wk, u32 id, s16 x, s16 y, u16 surface);
void PokedexListObj_GetPos(PokedexListWork *wk, u32 id, s16 *x, s16 *y, u16 surface);
// Stops the blinking of the cursor's palette
void PokedexListObj_StopBlink(PokedexListWork *wk, BOOL stopped);
// The picture of a species, or the picture of an unknown one for 0
void PokedexListObj_PutMon(PokedexListWork *wk, u16 species);
// The icon of a species in a row at y of the touch screen's rows, or of the top screen's
void PokedexListObj_PutIcon(PokedexListWork *wk, u16 species, s16 y, BOOL touchScreen);
// Moves the icons with the list's rows
void PokedexListObj_ScrollIcons(PokedexListWork *wk, s8 delta);
// Remembers the icons that are shown, and hides those when the list jumps
void PokedexListObj_SaveIcons(PokedexListWork *wk);
void PokedexListObj_HideSavedIcons(PokedexListWork *wk);
// Animates the icon of a row of the touch screen, and stops the others
void PokedexListObj_SelectIcon(PokedexListWork *wk, u32 row);
// Puts the scroll bar where the list's scroll is
void PokedexListObj_UpdateScrollBar(PokedexListWork *wk);
// Dims the page buttons at the list's ends; keep leaves one that is playing its press alone
void PokedexListObj_SetPageButtons(PokedexListWork *wk, BOOL keep);

#endif // POKEBW2_APP_POKEDEX_LIST_OBJ_H
