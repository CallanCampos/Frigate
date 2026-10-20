#ifndef POKEBW2_BATTLE_BTL_MOVE_H
#define POKEBW2_BATTLE_BTL_MOVE_H

#include "types.h"
#include "struct_decls.h"

// The moves' event handlers (not decompiled)

BOOL MoveEvent_AddItem(BattleMon *mon, u16 move, u32 speed);
void MoveEvent_RemoveItem(BattleMon *mon, u16 move);
void RemoveForce(BattleMon *mon, u16 move);

#endif // POKEBW2_BATTLE_BTL_MOVE_H
