#ifndef POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H
#define POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H

// Event function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

GameEvent *EventFieldEffect_CreateBattleCutin(GameSystem *gsys, void *field3d, u32 selection, u32 param);

// The encounter effects' create functions, called with the field
GameEvent *func_ov146_021f59e0(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f59ec(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f59f8(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a04(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a10(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a1c(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a28(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a34(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a40(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a4c(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a58(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a64(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a70(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a7c(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a88(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5a94(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5aa0(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5aac(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5ab8(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5ac4(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5ad0(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5adc(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5ae8(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5af4(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5b00(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5b0c(GameSystem *gsys, Field *field, u32 param);
GameEvent *func_ov146_021f5b18(GameSystem *gsys, Field *field, u32 param);
GameEvent *EventEncountEffectCutin_Create(GameSystem *gsys, Field *field, u32 selection, u32 param);
GameEventReturnCode EventEncountEffectCutin_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventEncountEffectCutinFallback_Callback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_EVENT_ENCOUNTER_CUTIN_H
