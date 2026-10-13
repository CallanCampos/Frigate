#ifndef POKEBW2_BATTLE_POS_HANDLERS_H
#define POKEBW2_BATTLE_POS_HANDLERS_H

// Overlay 169's pos_handlers.c, named descriptively after overlay 167's handler files: the event handlers of the
// effects that stay at a position (Wish, Lunar Dance, Healing Wish, Future Sight, Baton Pass). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

BOOL PosEventAdd(u32 effect, u8 pos, u8 monId, const u32 *args, u8 argCount);
// Whether no event of the position effect is set yet (name guessed from PosEventAdd refusing to add a second one)
BOOL IsPosEffectFree(u32 effect, u8 pos);

#endif // POKEBW2_BATTLE_POS_HANDLERS_H
