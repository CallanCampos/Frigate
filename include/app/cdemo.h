#ifndef POKEBW2_APP_CDEMO_H
#define POKEBW2_APP_CDEMO_H

#include "types.h"
#include "gfl/proc.h"

// The demos the game plays before the title screen, the Game Freak logo and the opening (ov264, cdemo_main.c). The
// names are ours

// The demos: the Game Freak logo, and the opening. Demo 1 has a script of its own that nothing starts
#define CDEMO_GF_LOGO 0
#define CDEMO_OPENING 2

typedef struct {
    // Which demo plays
    u16 demo;
    // Whether A, B or Start skips the demo
    u8 canSkip;
    // Set when the demo was skipped
    u8 skipped;
} CDemoParam;

extern const GameProcFunctions CDEMO_PROC_FUNCTIONS;

#endif // POKEBW2_APP_CDEMO_H
