#ifndef POKEBW2_FIELD_EVENT_ENTRANCE_EFFECT_H
#define POKEBW2_FIELD_EVENT_ENTRANCE_EFFECT_H

// Overlay 36's event_entrance_effect.c: the events that play an entrance, such as a door, when the player comes out of
// it or goes into it during a warp. They drive entrance_camera.c's camera. The file name is descriptive

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The position in front of the player, where an entrance the player walks into stands
void EntranceEffect_GetEntryPos(Field *field, VecFx32 *pos);
// The events of the warp's entrance as the player comes out of it and goes into it
GameEvent *EntranceEffect_CreateA(WarpSequence *warp);
GameEvent *EntranceEffect_CreateB(WarpSequence *warp);

#endif // POKEBW2_FIELD_EVENT_ENTRANCE_EFFECT_H
