#ifndef POKEBW2_APP_IRC_APPBAR_H
#define POKEBW2_APP_IRC_APPBAR_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// Overlay 175's irc_appbar.c: the button at the bottom right of the compatibility checker's screens, which says CANCEL
// or BACK. It is drawn as a task menu's window next to a cursor-less sprite. Names are descriptive guesses

typedef struct IrcAppBar IrcAppBar;

// The labels of the button
#define IRC_APPBAR_LABEL_CANCEL 0
#define IRC_APPBAR_LABEL_BACK 1

IrcAppBar *IrcAppBar_Create(u32 unused0, ClActUnit *unit, u8 bg, u8 palette, u8 objPalette, u32 unused5, Font *font,
                            PrintQueue *queue, HeapID heapId);
void IrcAppBar_Free(IrcAppBar *bar);
// Runs the button, which flashes when touched
void IrcAppBar_Update(IrcAppBar *bar);
// What the button gave: 1 once it was touched and has flashed, otherwise -1
int IrcAppBar_GetResult(IrcAppBar *bar);
BOOL IrcAppBar_IsPressed(IrcAppBar *bar);
void IrcAppBar_Reset(IrcAppBar *bar);
BOOL IrcAppBar_IsPressedOrDone(IrcAppBar *bar);
void IrcAppBar_SetVisible(IrcAppBar *bar, BOOL visible);
// Whether the button reacts, and goes back to waiting to be touched
void IrcAppBar_SetActive(IrcAppBar *bar, BOOL active);
void IrcAppBar_SetLabel(IrcAppBar *bar, int label);

#endif // POKEBW2_APP_IRC_APPBAR_H
