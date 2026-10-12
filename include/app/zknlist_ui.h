#ifndef POKEBW2_APP_ZKNLIST_UI_H
#define POKEBW2_APP_ZKNLIST_UI_H

#include "types.h"
#include "app/zknlist_main.h"

// zknlist_ui.c: the keys and touches of the Pokédex's list. The ROM doesn't name this file; zknlist_ui.c is a guess
// after the search screen's zknsearch_ui.c. The names are ours

// What ZknListUI_Main returns besides -1: the picture of the species, the icon of a row from 1, and the buttons
enum {
    ZKNLIST_UI_MON,
    ZKNLIST_UI_ROW,
    ZKNLIST_UI_START = ZKNLIST_UI_ROW + ZKNLIST_ROW_COUNT,
    ZKNLIST_UI_SELECT,
    ZKNLIST_UI_SHORTCUT,
    ZKNLIST_UI_EXIT,
    ZKNLIST_UI_RETURN,
    ZKNLIST_UI_UNOVA,
    ZKNLIST_UI_NATIONAL,
    ZKNLIST_UI_HABITAT,
    ZKNLIST_UI_CANCEL,
};

int ZknListUI_Main(ZknListWork *wk);

#endif // POKEBW2_APP_ZKNLIST_UI_H
