#ifndef POKEBW2_APP_ZKNLIST_OBJ_H
#define POKEBW2_APP_ZKNLIST_OBJ_H

#include "types.h"
#include "app/zknlist_main.h"

// zknlist_obj.c: the actors of the Pokédex's list. The ROM doesn't name this file; zknlist_obj.c is a guess after the
// search screen's zknsearch_obj.c. The names are ours

void ZknListObj_Init(ZknListWork *wk);
void ZknListObj_Exit(ZknListWork *wk);
void ZknListObj_Update(ZknListWork *wk);
void ZknListObj_SetAnmSeq(ZknListWork *wk, u32 id, u32 anm);
void ZknListObj_EndAnm(ZknListWork *wk, u32 id);
// Sets an actor's animation and plays it
void ZknListObj_SetAnm(ZknListWork *wk, u32 id, u32 anm);
u32 ZknListObj_GetAnm(ZknListWork *wk, u32 id);
BOOL ZknListObj_IsAnmPlaying(ZknListWork *wk, u32 id);
void ZknListObj_SetVisible(ZknListWork *wk, u32 id, BOOL visible);
BOOL ZknListObj_IsVisible(ZknListWork *wk, u32 id);
void ZknListObj_SetBlend(ZknListWork *wk, u32 id, BOOL blend);
void ZknListObj_SetPos(ZknListWork *wk, u32 id, s16 x, s16 y, u16 surface);
void ZknListObj_GetPos(ZknListWork *wk, u32 id, s16 *x, s16 *y, u16 surface);
// Stops the blinking of the cursor's palette
void ZknListObj_StopBlink(ZknListWork *wk, BOOL stopped);
// The picture of a species, or the picture of an unknown one for 0
void ZknListObj_PutMon(ZknListWork *wk, u16 species);
// The icon of a species in a row at y of the touch screen's rows, or of the top screen's
void ZknListObj_PutIcon(ZknListWork *wk, u16 species, s16 y, BOOL touchScreen);
// Moves the icons with the list's rows
void ZknListObj_ScrollIcons(ZknListWork *wk, s8 delta);
// Remembers the icons that are shown, and hides those when the list jumps
void ZknListObj_SaveIcons(ZknListWork *wk);
void ZknListObj_HideSavedIcons(ZknListWork *wk);
// Animates the icon of a row of the touch screen, and stops the others
void ZknListObj_SelectIcon(ZknListWork *wk, u32 row);
// Puts the scroll bar where the list's scroll is
void ZknListObj_UpdateScrollBar(ZknListWork *wk);
// Dims the page buttons at the list's ends; keep leaves one that is playing its press alone
void ZknListObj_SetPageButtons(ZknListWork *wk, BOOL keep);

#endif // POKEBW2_APP_ZKNLIST_OBJ_H
