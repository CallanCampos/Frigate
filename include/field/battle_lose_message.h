#ifndef POKEBW2_FIELD_BATTLE_LOSE_MESSAGE_H
#define POKEBW2_FIELD_BATTLE_LOSE_MESSAGE_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The message after a lost battle (overlay 295): the Trainer scurried to a Pokémon Center or back home. Its parameter
// is event_battle_lose.h's BattleLoseData. The file's name is a guess, from what it does: the ROM has no string for it

extern const GameProcFunctions BATTLE_LOSE_MESSAGE_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_BATTLE_LOSE_MESSAGE_H
