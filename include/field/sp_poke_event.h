#ifndef POKEBW2_FIELD_SP_POKE_EVENT_H
#define POKEBW2_FIELD_SP_POKE_EVENT_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// sp_poke_event.c, overlay 129, named after what it does (the ROM names only the overlay's other file,
// sp_poke_gimmick.c): the events of the special Pokémon, which overlay 12's scrcmd_sp_poke_gimmick.c runs

// A jump of the Pokémon: mode 0 sets it off from the start, which is the player, and 1 lands it at the end. frames is
// how long the jump takes
GameEvent *SpPokeEvent_CreateJump(GameSystem *gsys, u16 mode, const VecFx32 *start, const VecFx32 *end, fx32 height,
                               u32 frames);
void SpPokeEvent_Appear(GameSystem *gsys, u16 a1, const VecFx32 *pos);
GameEvent *SpPokeEvent_CreateWaitFrame(GameSystem *gsys, u16 a1);
GameEvent *SpPokeEvent_CreateWaitAnimEnd(GameSystem *gsys, u16 a1);
// Shows actor 2 at pos and, at a frame of its animation, replaces the actor actorId with the zone's own definition of
// the NPC uid
GameEvent *SpPokeEvent_CreateAppear(GameSystem *gsys, const VecFx32 *pos, u16 actorId, u16 uid);

#endif // POKEBW2_FIELD_SP_POKE_EVENT_H
