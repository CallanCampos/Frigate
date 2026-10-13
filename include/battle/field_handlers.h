#ifndef POKEBW2_BATTLE_FIELD_HANDLERS_H
#define POKEBW2_BATTLE_FIELD_HANDLERS_H

// Overlay 169's field_handlers.c, named descriptively after overlay 167's handler files: the event handlers of the
// field effects (weather, Trick Room, Gravity and the rest). Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "struct_decls.h"

BattleEventItem *FieldEffectEventAdd(u32 effect, u8 monId);
void FieldEffectEventRemove(BattleEventItem *item);

#endif // POKEBW2_BATTLE_FIELD_HANDLERS_H
