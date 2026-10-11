#ifndef POKEBW2_FIELD_GAME_CLEAR_SAVE_H
#define POKEBW2_FIELD_GAME_CLEAR_SAVE_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The screen after the Champion's defeat (overlay 295): it registers the team in the Hall of Fame, saves the game and
// says so. The file's name is a guess, from what it does: the ROM has no string for it

typedef struct {
    GameSystem *gsys;
    // Whether the Champion was beaten before: the first clear sets the time of the first clear and a record
    u32 repeat;
} GameClearSaveParam;

extern const GameProcFunctions GAME_CLEAR_SAVE_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_GAME_CLEAR_SAVE_H
