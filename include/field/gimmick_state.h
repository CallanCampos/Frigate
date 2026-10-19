#ifndef POKEBW2_FIELD_GIMMICK_STATE_H
#define POKEBW2_FIELD_GIMMICK_STATE_H

#include "types.h"
#include "struct_decls.h"

// Overlay 90, which sets up the saved state of some gimmicks: 2 (zone 63, the gym of Nimbasa City), 10 (zone 121
// in Opelucid City) and 48 (zone 585 in Nimbasa City)
void GimmickState_SetNimbasaGym(GameSystem *gsys, u16 value);
void GimmickState_ResetOpelucid(GameSystem *gsys);
void GimmickState_SetNimbasaFlags(GameSystem *gsys, BOOL first, BOOL second);

#endif // POKEBW2_FIELD_GIMMICK_STATE_H
