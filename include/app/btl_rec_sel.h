#ifndef POKEBW2_APP_BTL_REC_SEL_H
#define POKEBW2_APP_BTL_REC_SEL_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The screen that asks whether to save the battle that just ended as the Battle Video, overlay 306, named after the
// ROM's "btl_rec_sel.c". Battles against another player, the Battle Subway and Wi-Fi matches show it

#define OVERLAY_BTL_REC_SEL OVERLAY_ID(306)

// The screen's heap and its archive of graphics. Our names
#define HEAPID_BTL_REC_SEL 0x75
#define ARCID_BTL_REC_SEL 185

// What the screen is called with
typedef struct {
    GameData *gameData;
    // Whether the other player's game can record the battle; "You cannot record videos with this Trainer" if not
    BOOL compatible;
    // Whether the other player is at the same screen, which puts a time limit on the choice and waits for them at the
    // end
    BOOL comm;
    // Whether the battle was recorded at all; "The last battle cannot be recorded" if not
    BOOL recorded;
    // The battle mode's index and the header's counter, for func_0200be50
    u32 mode;
    s32 counter;
} BtlRecSelParam;

extern const GameProcFunctions BtlRecSelProcFunctions;

#endif // POKEBW2_APP_BTL_REC_SEL_H
