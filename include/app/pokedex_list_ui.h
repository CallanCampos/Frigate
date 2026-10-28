#ifndef POKEBW2_APP_POKEDEX_LIST_UI_H
#define POKEBW2_APP_POKEDEX_LIST_UI_H

#include "types.h"
#include "app/pokedex_list_main.h"

// pokedex_list_ui.c: the keys and touches of the Pokédex's list. The ROM doesn't name this file; pokedex_list_ui.c is a
// guess after the search screen's pokedex_search_ui.c. The names are ours

// What PokedexListUI_Main returns besides -1: the picture of the species, the icon of a row from 1, and the buttons
enum {
    POKEDEX_LIST_UI_MON,
    POKEDEX_LIST_UI_ROW,
    POKEDEX_LIST_UI_START = POKEDEX_LIST_UI_ROW + POKEDEX_LIST_ROW_COUNT,
    POKEDEX_LIST_UI_SELECT,
    POKEDEX_LIST_UI_SHORTCUT,
    POKEDEX_LIST_UI_EXIT,
    POKEDEX_LIST_UI_RETURN,
    POKEDEX_LIST_UI_UNOVA,
    POKEDEX_LIST_UI_NATIONAL,
    POKEDEX_LIST_UI_HABITAT,
    POKEDEX_LIST_UI_CANCEL,
};

int PokedexListUI_Main(PokedexListWork *wk);

#endif // POKEBW2_APP_POKEDEX_LIST_UI_H
