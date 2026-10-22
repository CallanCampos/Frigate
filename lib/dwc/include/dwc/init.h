#ifndef POKEBW2_DWC_INIT_H
#define POKEBW2_DWC_INIT_H

#include "types.h"
#include "dwc/memfunc.h"

// Starting DWC (TWL-DWC 5.3's dwc_init.c, in ARM9 main). The ROM names none of it; the SDK's names are in the comments,
// where the code shows them.

// What the start returns, from whether the Wi-Fi settings passed the checks of DWC_BM_Init and whether the console had
// no Wi-Fi ID, so that one was made. The names follow the SDK's DWC_INIT_RESULT_*; the game's caller shows only 3
#define DWC_INIT_RESULT_NORMAL 0
// The checks passed and a Wi-Fi ID was made
#define DWC_INIT_RESULT_CREATE_USERID 1
// The checks failed and a Wi-Fi ID was made
#define DWC_INIT_RESULT_DESTROY_USERID 2
// The checks failed and the console had its ID: what makes the game warn the player that the user info was lost
#define DWC_INIT_RESULT_DESTROY_OTHER_SETTING 3
// The work area could not be allocated
#define DWC_INIT_RESULT_OUT_OF_MEMORY 4

// The signature of the library's build, "[SDK+NINTENDO:DWC5.3...]", which the start refers to so that the link keeps it
extern const char DWCi_Signature[];

// Starts the library for the retail servers with the game's name and code, and the allocator and deallocator it uses,
// and returns one of the results above: DWC_InitForProduction
int DWC_InitForProduction(const char *gameName, u32 gameCode, DWCAllocFunc alloc, DWCFreeFunc free);
// The game code that the library was started with
u32 DWCi_GetGameCode(void);

#endif // POKEBW2_DWC_INIT_H
