#ifndef POKEBW2_FIELD_EVENT_HALL_OF_FAME_MACHINE_H
#define POKEBW2_FIELD_EVENT_HALL_OF_FAME_MACHINE_H

#include "nitro/fx.h"
#include "system/game_event.h"

struct EventHallOfFameMachineData {
    u16 heapId;
    u16 unk02;
    Field *field;
    VecFx32 basePosition;
    u8 ballCount;
    u8 spawnedCount;
    u16 frameCounter;
    FieldPropSystem *propSystem;
    FieldPropHandle *ballHandles[6];
    FieldChunkPropHolder *centerProp;
    FieldPropHandle *centerHandle;
};

GameEvent *EventHallOfFameMachine_Create(GameSystem *gsys, GameEvent *parent);
GameEventReturnCode EventHallOfFameMachine_Callback(GameEvent *event, u32 *state, void *data);
void EventHallOfFameMachine_Init(EventHallOfFameMachineData *work, GameSystem *gsys);
void EventHallOfFameMachine_End(EventHallOfFameMachineData *work);
void EventHallOfFameMachine_SpawnMonsBall(EventHallOfFameMachineData *work);
void EventHallOfFameMachine_StartAnimations(EventHallOfFameMachineData *work);
BOOL EventHallOfFameMachine_IsAnimationDone(EventHallOfFameMachineData *work);

#endif // POKEBW2_FIELD_EVENT_HALL_OF_FAME_MACHINE_H
