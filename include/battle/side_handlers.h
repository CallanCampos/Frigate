#ifndef POKEBW2_BATTLE_SIDE_HANDLERS_H
#define POKEBW2_BATTLE_SIDE_HANDLERS_H

// Overlay 169's side_handlers.c, named descriptively after overlay 167's handler files: the event handlers of the
// effects of one side of the battle (Reflect, Light Screen, Spikes and the rest), and the table that keeps them.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

// Clears the side effects of both sides
void SideEffectsInit(void);
// Sets a side effect, or adds a layer to it (Spikes and Toxic Spikes), and returns its event item, or NULL when
// it is set and can't take another layer (name guessed from the table of EventAddSide* functions it searches)
BattleEventItem *SideEffectEventAdd(u8 side, u32 effect, BattleCondition cont);
// How many layers of a side effect are set
u32 GetSideEffectLayers(u8 side, u32 effect);
// Whether a side effect is set
BOOL IsSideEffectSet(u8 side, u32 effect);
// Removes a side effect, and returns whether it was set
BOOL ServerDisplay_RemoveSideEffect(u8 side, u32 effect);
// Counts a turn of every timed side effect, and removes the ones that run out, calling the function with each
void SideEffectsEndTurn(void (*callback)(u8 side, u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);

#endif // POKEBW2_BATTLE_SIDE_HANDLERS_H
