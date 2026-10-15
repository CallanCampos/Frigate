#ifndef POKEBW2_FIELD_EVENT_DENDOU_MACHINE_H
#define POKEBW2_FIELD_EVENT_DENDOU_MACHINE_H

#include "nitro/fx.h"
#include "system/game_event.h"

struct EventDendouMachineData {
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

GameEvent *EventDendouMachine_Create(GameSystem *gsys, GameEvent *parent);
GameEventReturnCode EventDendouMachine_Callback(GameEvent *event, u32 *state, void *data);
void EventDendouMachine_Init(EventDendouMachineData *work, GameSystem *gsys);
void EventDendouMachine_End(EventDendouMachineData *work);
void EventDendouMachine_SpawnMonsBall(EventDendouMachineData *work);
void EventDendouMachine_StartAnimations(EventDendouMachineData *work);
BOOL EventDendouMachine_IsAnimationDone(EventDendouMachineData *work);

#endif // POKEBW2_FIELD_EVENT_DENDOU_MACHINE_H
