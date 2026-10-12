#ifndef POKEBW2_APP_WIFI_NOTE_H
#define POKEBW2_APP_WIFI_NOTE_H

#include "types.h"
#include "dwc/dwc.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pal Pad (overlay 204), the key item that lists the Wi-Fi friends, registers Friend Codes and shows the player's
// own. The ROM has no name for its file; "wifi_note.c" is pokeplatinum's name for the same screen. The names are ours

#define OVERLAY_WIFI_NOTE OVERLAY_ID(204)
// The screen's heap, a child of HEAPID_USER
#define HEAPID_WIFI_NOTE 0x34

// How the Pal Pad was left
enum {
    // With the return button, back to the bag
    WIFI_NOTE_RESULT_RETURN,
    // With the X button, back to the field
    WIFI_NOTE_RESULT_EXIT,
};

typedef struct {
    GameData *gameData;
    // WIFI_NOTE_RESULT_*
    u32 result;
} WifiNoteParam;

extern const GameProcFunctions WIFI_NOTE_PROC_FUNCTIONS;

// The friend key of a friend's data, 0 if it holds none (DWC, not declared in dwc/dwc.h)
u64 func_02057e3c(const DWCFriendData *friendData);

#endif // POKEBW2_APP_WIFI_NOTE_H
