#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_LIFT_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_LIFT_H

#include "types.h"
#include "struct_decls.h"

// The lift gimmick of the Pokémon League, overlay 106. Overlay 36's gimmick table calls these
void GimmickLeagueLift_Init(Field *field);
void GimmickLeagueLift_End(Field *field);
void GimmickLeagueLift_Update(Field *field);
// The lift's arrival, which overlay 12's event_league_lift.c runs
void GimmickLeagueLift_PlayLiftAnim(Field *field);
void GimmickLeagueLift_StopLiftAnim(Field *field);
BOOL GimmickLeagueLift_IsLiftAnimDone(Field *field);
void GimmickLeagueLift_HideArrivalModel(Field *field);
void GimmickLeagueLift_PlayArrivalModel(Field *field);
BOOL GimmickLeagueLift_IsArrivalModelDone(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_LIFT_H
