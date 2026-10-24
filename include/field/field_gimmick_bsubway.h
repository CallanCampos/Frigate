#ifndef POKEBW2_FIELD_FIELD_GIMMICK_BSUBWAY_H
#define POKEBW2_FIELD_FIELD_GIMMICK_BSUBWAY_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// field_gimmick_bsubway.c, overlay 108: the gimmick of the Battle Subway's zones 67 to 76. Overlay 36's gimmick table
// calls the first three; script plugin 1 (overlay 50) drives the rest

void FieldGimmickBSubway_Setup(Field *field);
void FieldGimmickBSubway_End(Field *field);
void FieldGimmickBSubway_Move(Field *field);
// Starts the field effect of the zone's train, which FieldGimmickBSubway_GetTrainEffect returns
void FieldGimmickBSubway_StartTrainEffect(Field *field, u32 a1, const VecFx32 *pos);
FieldEffectTask *FieldGimmickBSubway_GetTrainEffect(Field *field);
// Stops the shaking of zone 75's train
void FieldGimmickBSubway_StopShake(Field *field);

#endif // POKEBW2_FIELD_FIELD_GIMMICK_BSUBWAY_H
