#ifndef POKEBW2_APP_ZKNSEARCH_UI_H
#define POKEBW2_APP_ZKNSEARCH_UI_H

#include "types.h"
#include "app/zknsearch_main.h"

// zknsearch_ui.c: the keys and touches of the Pokédex's search screen. The ROM doesn't name this file;
// zknsearch_ui.c is a guess after zknsearch_main.c. The names are ours

// The positions of the main page's cursor, which ZknSearchUI_Main returns
enum {
    ZKNSEARCH_UI_ORDER,
    ZKNSEARCH_UI_INITIAL,
    ZKNSEARCH_UI_TYPE,
    ZKNSEARCH_UI_COLOR,
    ZKNSEARCH_UI_SHAPE,
    ZKNSEARCH_UI_START,
    ZKNSEARCH_UI_RESET,
    ZKNSEARCH_UI_SHORTCUT,
    ZKNSEARCH_UI_EXIT,
    ZKNSEARCH_UI_RETURN,
};

// What ZknSearchUI_ListMain and ZknSearchUI_ShapeListMain return besides -1: the return button, a row from 1, the
// cancel of a key and the reset of the criterion
#define ZKNSEARCH_UI_LIST_RETURN 0
#define ZKNSEARCH_UI_LIST_ROW 1
#define ZKNSEARCH_UI_LIST_CANCEL 8
#define ZKNSEARCH_UI_LIST_RESET 9

// The palettes of a position of the main page's cursor
#define ZKNSEARCH_UI_PALETTE_OFF 1
#define ZKNSEARCH_UI_PALETTE_PRESSED 2
#define ZKNSEARCH_UI_PALETTE_ON 3

void ZknSearchUI_InitCursor(ZknSearchWork *wk, u32 pos);
void ZknSearchUI_ExitCursor(ZknSearchWork *wk);
void ZknSearchUI_SetCursorPalette(ZknSearchWork *wk, u32 pos, u32 palette);
// The main page: a position chosen, or a CURSOR_MOVE_* code
u32 ZknSearchUI_Main(ZknSearchWork *wk);
// Whether the search's result has been read: a touch, A or B, or 90 frames
BOOL ZknSearchUI_WaitResult(ZknSearchWork *wk);
int ZknSearchUI_ListMain(ZknSearchWork *wk);
int ZknSearchUI_ShapeListMain(ZknSearchWork *wk);

#endif // POKEBW2_APP_ZKNSEARCH_UI_H
