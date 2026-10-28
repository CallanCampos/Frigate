#ifndef POKEBW2_APP_HALL_OF_FAME_PC_UI_H
#define POKEBW2_APP_HALL_OF_FAME_PC_UI_H

#include "types.h"
#include "app/hall_of_fame_pc_main.h"

// The Hall of Fame PC's input, in hall_of_fame_pc_ui.c (a guessed name). Our names

// What the player did
enum {
    HALL_OF_FAME_PC_UI_LEFT,
    HALL_OF_FAME_PC_UI_RIGHT,
    HALL_OF_FAME_PC_UI_CLOSE,
    HALL_OF_FAME_PC_UI_RETURN,
    HALL_OF_FAME_PC_UI_DECIDE,
    // A touched Pokémon: HALL_OF_FAME_PC_UI_POKE plus its place in the team
    HALL_OF_FAME_PC_UI_POKE,
    HALL_OF_FAME_PC_UI_NONE = 0xff,
};

// The input of a page, and of the page while a Pokémon's data is shown
u32 HallOfFamePCUi_PageMain(HallOfFamePCWork *wk);
u32 HallOfFamePCUi_PokeMain(HallOfFamePCWork *wk);

#endif // POKEBW2_APP_HALL_OF_FAME_PC_UI_H
