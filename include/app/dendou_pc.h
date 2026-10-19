#ifndef POKEBW2_APP_DENDOU_PC_H
#define POKEBW2_APP_DENDOU_PC_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Hall of Fame as the PC shows it, overlay 256: the teams that entered it, one to a page, with their Pokémon in
// a ring on the touch screen and the chosen one's data on the top screen. The ROM names none of its files; they are
// named after what they hold in the way of the PC box's (box2.c, box2_main.c, ...), with "dendou", the Hall of Fame,
// for the subject. Our names

// The screen's heap, a child of HEAPID_USER
#define HEAPID_DENDOU_PC 0x8c

// What the screen shows
enum {
    // The team that finished the story, of save block 0x40
    DENDOU_PC_MODE_FIRST_CLEAR,
    // The teams of the Hall of Fame, extra save block 8
    DENDOU_PC_MODE_HALL_OF_FAME,
};

// How the screen was left
enum {
    DENDOU_PC_RESULT_RETURN,
    DENDOU_PC_RESULT_CLOSE,
};

typedef struct {
    GameData *gameData;
    u16 mode;
    u16 result;
} DendouPCParam;

extern const GameProcFunctions DENDOU_PC_PROC_FUNCTIONS;

#endif // POKEBW2_APP_DENDOU_PC_H
