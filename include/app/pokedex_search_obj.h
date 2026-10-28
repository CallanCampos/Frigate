#ifndef POKEBW2_APP_POKEDEX_SEARCH_OBJ_H
#define POKEBW2_APP_POKEDEX_SEARCH_OBJ_H

#include "types.h"
#include "app/pokedex_search_main.h"

// pokedex_search_obj.c: the actors of the Pokédex's search screen. The ROM doesn't name this file; pokedex_search_obj.c
// is a guess after pokedex_search_main.c. The names are ours

void PokedexSearchObj_Init(PokedexSearchWork *wk);
void PokedexSearchObj_Exit(PokedexSearchWork *wk);
void PokedexSearchObj_Update(PokedexSearchWork *wk);
void PokedexSearchObj_SetAnmSeq(PokedexSearchWork *wk, u32 id, u32 anm);
// Sets an actor's animation and plays it
void PokedexSearchObj_SetAnm(PokedexSearchWork *wk, u32 id, u32 anm);
u32 PokedexSearchObj_GetAnm(PokedexSearchWork *wk, u32 id);
BOOL PokedexSearchObj_IsAnmPlaying(PokedexSearchWork *wk, u32 id);
void PokedexSearchObj_SetVisible(PokedexSearchWork *wk, u32 id, BOOL visible);
BOOL PokedexSearchObj_IsVisible(PokedexSearchWork *wk, u32 id);
void PokedexSearchObj_SetBlend(PokedexSearchWork *wk, u32 id, BOOL blend);
void PokedexSearchObj_SetPos(PokedexSearchWork *wk, u32 id, s16 x, s16 y, u16 surface);
void PokedexSearchObj_GetPos(PokedexSearchWork *wk, u32 id, s16 *x, s16 *y, u16 surface);
void PokedexSearchObj_SetBGPriority(PokedexSearchWork *wk, u32 id, u32 priority);
// Hides the actors of the lists
void PokedexSearchObj_HideList(PokedexSearchWork *wk);
// Show the actors of each page
void PokedexSearchObj_SetMain(PokedexSearchWork *wk);
void PokedexSearchObj_SetOrder(PokedexSearchWork *wk);
void PokedexSearchObj_SetInitial(PokedexSearchWork *wk);
void PokedexSearchObj_SetType(PokedexSearchWork *wk);
void PokedexSearchObj_SetColor(PokedexSearchWork *wk);
void PokedexSearchObj_SetShape(PokedexSearchWork *wk);
// The chosen shape on the top screen
void PokedexSearchObj_PutShape(PokedexSearchWork *wk);
// The shapes of the rows on both screens, from the list's scroll
void PokedexSearchObj_PutShapeIcons(PokedexSearchWork *wk);
// Puts the scroll bar where the list's scroll is
void PokedexSearchObj_UpdateScrollBar(PokedexSearchWork *wk);
// Dims the page buttons at the list's ends; keep leaves one that is playing its press alone
void PokedexSearchObj_SetPageButtons(PokedexSearchWork *wk, BOOL keep);
// A mark at y of the touch screen's rows, or of the top screen's
void PokedexSearchObj_PutMark(PokedexSearchWork *wk, int index, s16 y, BOOL touchScreen);
void PokedexSearchObj_HideMarks(PokedexSearchWork *wk);
// The mark of a row of the touch screen, or none
void PokedexSearchObj_PutRowMark(PokedexSearchWork *wk, u16 row, BOOL visible);
void PokedexSearchObj_PutTypeMarks(PokedexSearchWork *wk, u8 type1, u8 type2);
void PokedexSearchObj_PutShapeMark(PokedexSearchWork *wk, s16 y, BOOL touchScreen);
void PokedexSearchObj_PutShapeRowMark(PokedexSearchWork *wk, u16 row, BOOL visible);
// The mark of a list's item, if its row is on either screen
void PokedexSearchObj_PutItemMark(PokedexSearchWork *wk, u32 index, u16 item);
void PokedexSearchObj_PutShapeItemMark(PokedexSearchWork *wk);
void PokedexSearchObj_PutShapeIcon(PokedexSearchWork *wk, u16 shape, s16 y, BOOL touchScreen);
// Move the marks and the shapes with the list's rows
void PokedexSearchObj_ScrollMarks(PokedexSearchWork *wk, s8 delta);
void PokedexSearchObj_ScrollShapeIcons(PokedexSearchWork *wk, s8 delta);
// The search's progress bar after count frames
void PokedexSearchObj_MoveLoadingBar(PokedexSearchWork *wk, u32 count);

#endif // POKEBW2_APP_POKEDEX_SEARCH_OBJ_H
