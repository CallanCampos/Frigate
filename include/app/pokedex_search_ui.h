#ifndef POKEBW2_APP_POKEDEX_SEARCH_UI_H
#define POKEBW2_APP_POKEDEX_SEARCH_UI_H

#include "types.h"
#include "app/pokedex_search_main.h"

// pokedex_search_ui.c: the keys and touches of the Pokédex's search screen. The ROM doesn't name this file;
// pokedex_search_ui.c is a guess after pokedex_search_main.c. The names are ours

// The positions of the main page's cursor, which PokedexSearchUI_Main returns
enum {
    POKEDEX_SEARCH_UI_ORDER,
    POKEDEX_SEARCH_UI_INITIAL,
    POKEDEX_SEARCH_UI_TYPE,
    POKEDEX_SEARCH_UI_COLOR,
    POKEDEX_SEARCH_UI_SHAPE,
    POKEDEX_SEARCH_UI_START,
    POKEDEX_SEARCH_UI_RESET,
    POKEDEX_SEARCH_UI_SHORTCUT,
    POKEDEX_SEARCH_UI_EXIT,
    POKEDEX_SEARCH_UI_RETURN,
};

// What PokedexSearchUI_ListMain and PokedexSearchUI_ShapeListMain return besides -1: the return button, a row from 1,
// the cancel of a key and the reset of the criterion
enum {
    POKEDEX_SEARCH_UI_LIST_RETURN,
    POKEDEX_SEARCH_UI_LIST_ROW,
    POKEDEX_SEARCH_UI_LIST_CANCEL = 8,
    POKEDEX_SEARCH_UI_LIST_RESET,
};

// The palettes of a position of the main page's cursor
enum {
    POKEDEX_SEARCH_UI_PALETTE_OFF = 1,
    POKEDEX_SEARCH_UI_PALETTE_PRESSED,
    POKEDEX_SEARCH_UI_PALETTE_ON,
};

void PokedexSearchUI_InitCursor(PokedexSearchWork *wk, u32 pos);
void PokedexSearchUI_ExitCursor(PokedexSearchWork *wk);
// palette is a POKEDEX_SEARCH_UI_PALETTE_*
void PokedexSearchUI_SetCursorPalette(PokedexSearchWork *wk, u32 pos, u32 palette);
// The main page: a position chosen, or a CURSOR_MOVE_* code
u32 PokedexSearchUI_Main(PokedexSearchWork *wk);
// Whether the search's result has been read: a touch, A or B, or 90 frames
BOOL PokedexSearchUI_WaitResult(PokedexSearchWork *wk);
int PokedexSearchUI_ListMain(PokedexSearchWork *wk);
int PokedexSearchUI_ShapeListMain(PokedexSearchWork *wk);

#endif // POKEBW2_APP_POKEDEX_SEARCH_UI_H
