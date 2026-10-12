#ifndef POKEBW2_APP_MEDALBOX_H
#define POKEBW2_APP_MEDALBOX_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Medal Box screen (overlay 145, medalbox.c, a name guessed from the ROM's "medalbox_data_sys.c"): the medal case
// on the top screen, and the medals and hint medals on the touch screen

#define OVERLAY_MEDALBOX OVERLAY_ID(145)

// What the screen returns in MedalBoxParam's result
#define MEDALBOX_RESULT_RETURN 0
#define MEDALBOX_RESULT_EXIT 1

typedef struct {
    GameData *gameData;
    u32 result;
} MedalBoxParam;

extern GameProcFunctions MEDALBOX_PROC_FUNCTIONS;

#endif // POKEBW2_APP_MEDALBOX_H
