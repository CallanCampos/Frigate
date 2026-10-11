#ifndef POKEBW2_APP_WIFICLUB_H
#define POKEBW2_APP_WIFICLUB_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Wi-Fi Club runs in overlay 203, and its proc table is in overlay 173, which also has to be loaded
#define OVERLAY_WIFICLUB OVERLAY_ID(203)
#define OVERLAY_WIFICLUB_MAIN OVERLAY_ID(173)

typedef struct {
    void *buffer;
    GameData *gameData;
    SaveControl *save;
    // What the player chose in the Wi-Fi Club
    u32 mode;
    u32 unk10;
    PokeParty *parties[2];
    Regulation *regulation;
    u8 unk20;
    u8 unk21[0x25];
    u8 unk46;
    // The friend's index in the friend list, plus 1
    u8 friendIndex;
    u8 unk48;
    u8 unk49;
    u8 unk4A[2];
} WifiClubData;

extern const GameProcFunctions WIFICLUB_PROC_FUNCTIONS;

void func_ov173_021a6240(void *buffer);

#endif // POKEBW2_APP_WIFICLUB_H
