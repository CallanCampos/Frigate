#ifndef POKEBW2_APP_ZKNSEARCH_OBJ_H
#define POKEBW2_APP_ZKNSEARCH_OBJ_H

#include "types.h"
#include "app/zknsearch_main.h"

// zknsearch_obj.c: the actors of the Pokédex's search screen. The ROM doesn't name this file; zknsearch_obj.c is a
// guess after zknsearch_main.c. The names are ours

void ZknSearchObj_Init(ZknSearchWork *wk);
void ZknSearchObj_Exit(ZknSearchWork *wk);
void ZknSearchObj_Update(ZknSearchWork *wk);
void ZknSearchObj_SetAnmSeq(ZknSearchWork *wk, u32 id, u32 anm);
// Sets an actor's animation and plays it
void ZknSearchObj_SetAnm(ZknSearchWork *wk, u32 id, u32 anm);
u32 ZknSearchObj_GetAnm(ZknSearchWork *wk, u32 id);
BOOL ZknSearchObj_IsAnmPlaying(ZknSearchWork *wk, u32 id);
void ZknSearchObj_SetVisible(ZknSearchWork *wk, u32 id, BOOL visible);
BOOL ZknSearchObj_IsVisible(ZknSearchWork *wk, u32 id);
void ZknSearchObj_SetBlend(ZknSearchWork *wk, u32 id, BOOL blend);
void ZknSearchObj_SetPos(ZknSearchWork *wk, u32 id, s16 x, s16 y, u16 surface);
void ZknSearchObj_GetPos(ZknSearchWork *wk, u32 id, s16 *x, s16 *y, u16 surface);
void ZknSearchObj_SetBGPriority(ZknSearchWork *wk, u32 id, u32 priority);
// Hides the actors of the lists
void ZknSearchObj_HideList(ZknSearchWork *wk);
// Show the actors of each page
void ZknSearchObj_SetMain(ZknSearchWork *wk);
void ZknSearchObj_SetOrder(ZknSearchWork *wk);
void ZknSearchObj_SetInitial(ZknSearchWork *wk);
void ZknSearchObj_SetType(ZknSearchWork *wk);
void ZknSearchObj_SetColor(ZknSearchWork *wk);
void ZknSearchObj_SetShape(ZknSearchWork *wk);
// The chosen shape on the top screen
void ZknSearchObj_PutShape(ZknSearchWork *wk);
// The shapes of the rows on both screens, from the list's scroll
void ZknSearchObj_PutShapeIcons(ZknSearchWork *wk);
// Puts the scroll bar where the list's scroll is
void ZknSearchObj_UpdateScrollBar(ZknSearchWork *wk);
// Dims the page buttons at the list's ends; keep leaves one that is playing its press alone
void ZknSearchObj_SetPageButtons(ZknSearchWork *wk, BOOL keep);
// A mark at y of the touch screen's rows, or of the top screen's
void ZknSearchObj_PutMark(ZknSearchWork *wk, int index, s16 y, BOOL touchScreen);
void ZknSearchObj_HideMarks(ZknSearchWork *wk);
// The mark of a row of the touch screen, or none
void ZknSearchObj_PutRowMark(ZknSearchWork *wk, u16 row, BOOL visible);
void ZknSearchObj_PutTypeMarks(ZknSearchWork *wk, u8 type1, u8 type2);
void ZknSearchObj_PutShapeMark(ZknSearchWork *wk, s16 y, BOOL touchScreen);
void ZknSearchObj_PutShapeRowMark(ZknSearchWork *wk, u16 row, BOOL visible);
// The mark of a list's item, if its row is on either screen
void ZknSearchObj_PutItemMark(ZknSearchWork *wk, u32 index, u16 item);
void ZknSearchObj_PutShapeItemMark(ZknSearchWork *wk);
void ZknSearchObj_PutShapeIcon(ZknSearchWork *wk, u16 shape, s16 y, BOOL touchScreen);
// Move the marks and the shapes with the list's rows
void ZknSearchObj_ScrollMarks(ZknSearchWork *wk, s8 delta);
void ZknSearchObj_ScrollShapeIcons(ZknSearchWork *wk, s8 delta);
// The search's progress bar after count frames
void ZknSearchObj_MoveLoadingBar(ZknSearchWork *wk, u32 count);

#endif // POKEBW2_APP_ZKNSEARCH_OBJ_H
