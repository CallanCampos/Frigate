#ifndef POKEBW2_APP_DPC_UI_H
#define POKEBW2_APP_DPC_UI_H

#include "types.h"
#include "app/dpc_main.h"

// The Hall of Fame PC's input, in dpc_ui.c (a guessed name). Our names

// What the player did
enum {
    DPCUI_LEFT,
    DPCUI_RIGHT,
    DPCUI_CLOSE,
    DPCUI_RETURN,
    DPCUI_DECIDE,
    // A touched Pokémon: DPCUI_POKE plus its place in the team
    DPCUI_POKE,
    DPCUI_NONE = 0xff,
};

// The input of a page, and of the page while a Pokémon's data is shown
u32 DpcUi_PageMain(DendouPCWork *wk);
u32 DpcUi_PokeMain(DendouPCWork *wk);

#endif // POKEBW2_APP_DPC_UI_H
