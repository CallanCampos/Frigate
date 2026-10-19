#ifndef POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H
#define POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H

// Event function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

GameEvent *EventFieldEffect_CreateBattleCutin(GameSystem *gsys, Fld3DCi *ci, u32 selection, u32 param);

// The encounter effects' create functions, called with the field
GameEvent *EventEncountEffectCutin_Create01(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create02(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create03(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create04(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create05(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create06(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create07(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create08(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create09(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create10(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create11(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create12(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create00(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create15(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create14(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create16(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create13(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create17(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create18(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create19(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create20(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create21(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create22(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create23(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create24(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create25(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create26(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create(GameSystem *gsys, Field *field, u32 selection, u32 param);
GameEventReturnCode EventEncountEffectCutin_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventEncountEffectCutinFallback_Callback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H
