#ifndef POKEBW2_APP_WIFIBATTLEMATCH_H
#define POKEBW2_APP_WIFIBATTLEMATCH_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 290's wifibattlematch_sys.c (named by its embedded string): the Battle Competition, which chains the Wi-Fi
// login, the Wi-Fi (overlay 260) or infrared (overlay 261) link, the team selection and exchange of overlay 263, the
// battle and overlay 306's result screen. It runs from the title screen's menu or from the field

#define OVERLAY_WIFIBATTLEMATCH OVERLAY_ID(290)

typedef struct {
    // The game data to play with; the title screen's menu has none, so the proc makes one and resets the game at the
    // end
    GameData *gameData;
    // The battle type, from 0 to 4 (single, double, triple, rotation, and another triple); picks the regulation
    u32 unk4;
    // 0 when started from the title screen's menu, 1 when started from the field
    u32 unk8;
    // Where the team comes from, 0 for the party and 1 for the battle box
    u32 unkC;
    // Set when the proc has to free the parameters itself
    u32 unk10;
} WifiBattleMatchParam;

extern const GameProcFunctions WIFIBATTLEMATCH_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_H
