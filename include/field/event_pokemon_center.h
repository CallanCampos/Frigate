#ifndef POKEBW2_FIELD_EVENT_POKEMON_CENTER_H
#define POKEBW2_FIELD_EVENT_POKEMON_CENTER_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Partial layout for the healing machine's animation state.
struct EventPokeCenHealData {
    u16 heapId;
    u16 unk02;
    Field *field;
    VecFx32 basePosition;
    u8 ballCount;
    u8 animationCount;
    u16 frameCounter;
    FieldPropSystem *propSystem;
    FieldPropHandle *ballHandles[6];
    FieldChunkPropHolder *centerProp;
};

GameEvent *EventPokeCenHeal_Create(GameSystem *gsys, GameEvent *parent, u8 ballCount);
GameEventReturnCode EventPokeCenHeal_Callback(GameEvent *event, u32 *state, void *data);
void EventPokeCenHeal_Init(EventPokeCenHealData *work, GameSystem *gsys, u8 ballCount);
void EventPokeCenHeal_End(EventPokeCenHealData *work);
void EventPokeCenHeal_ChangeState(EventPokeCenHealData *work, u32 *state, u32 newState);
void EventPokeCenHeal_SpawnMonsBall(EventPokeCenHealData *work);
void EventPokeCenHeal_StartAnimations(EventPokeCenHealData *work);
void EventPokeCenHeal_PauseAnimation(EventPokeCenHealData *work);
BOOL EventPokeCenHeal_IsAnimationDone(EventPokeCenHealData *work);

#endif // POKEBW2_FIELD_EVENT_POKEMON_CENTER_H
