// The Hall of Fame machine's event: the balls of the team going onto the machine and its animations (a descriptive
// name). Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), in English: swan's EventDendouMachine
// is EventHallOfFameMachine
#include "types.h"
#include "field/event_hall_of_fame_machine.h"
#include "field/field.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_party.h"
#include "field/field_prop.h"
#include "gfl/calctool.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "pml/poke_party.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Where the balls go, from the machine's position
// clang-format off
static const VecFx32 sBallPositions[6] = {
    { FX32_CONST(-4), FX32_CONST(14), FX32_CONST(-3) },
    { FX32_CONST(4), FX32_CONST(14), FX32_CONST(-3) },
    { FX32_CONST(9), FX32_CONST(14), FX32_CONST(4) },
    { FX32_CONST(4), FX32_CONST(14), FX32_CONST(8) },
    { FX32_CONST(-4), FX32_CONST(14), FX32_CONST(8) },
    { FX32_CONST(-9), FX32_CONST(14), FX32_CONST(4) },
};

// clang-format on

GameEvent *EventHallOfFameMachine_Create(GameSystem *gsys, GameEvent *parent) {
    GameEvent *event;

    event = GameEvent_Create(gsys, parent, EventHallOfFameMachine_Callback, sizeof(EventHallOfFameMachineData));
    EventHallOfFameMachine_Init(GameEvent_GetData(event), gsys);
    return event;
}

GameEventReturnCode EventHallOfFameMachine_Callback(GameEvent *event, u32 *state, void *data) {
    EventHallOfFameMachineData *work;

    work = data;
    switch (*state) {
    case 0:
        if (work->frameCounter % 20 == 0) {
            EventHallOfFameMachine_SpawnMonsBall(work);
            if (work->ballCount <= work->spawnedCount) {
                (*state)++;
                work->frameCounter = 0;
            }
        }
        break;
    case 1:
        if (work->frameCounter > 10) {
            EventHallOfFameMachine_StartAnimations(work);
            (*state)++;
            work->frameCounter = 0;
        }
        break;
    case 2:
        if (EventHallOfFameMachine_IsAnimationDone(work)) {
            (*state)++;
        }
        break;
    case 3:
        EventHallOfFameMachine_End(work);
        return GAMEEVENT_DONE;
    }
    work->frameCounter++;
    return GAMEEVENT_CONTINUE;
}

void EventHallOfFameMachine_Init(EventHallOfFameMachineData *work, GameSystem *gsys) {
    u32 count;
    VecFx32 playerPosition;
    FieldPropAreaBounds bounds;
    Field *field;
    FieldG3DMapper *mapper;
    u16 heapId;
    FieldChunkPropHolder **props;

    field = GSYS_GetField(gsys);
    mapper = Field_GetG3DMapper(field);
    heapId = Field_GetHeapID(field);
    work->heapId = heapId;
    work->field = field;
    work->ballCount = countNonEggsInParty(gsys);
    work->spawnedCount = 0;
    work->propSystem = FieldG3DMapper_GetBMSystem(mapper);
    work->centerProp = NULL;
    if (work->ballCount > 6) {
        work->ballCount = 6;
    }
    FieldPlayer_GetWPos(Field_GetPlayer(work->field), &playerPosition);
    bounds.minZ = playerPosition.z - (5 << 16);
    bounds.maxZ = playerPosition.z + (5 << 16);
    bounds.minX = playerPosition.x - (5 << 16);
    bounds.maxX = playerPosition.x + (5 << 16);
    props = FieldPropSystem_FindPropsInArea(work->propSystem, &bounds, 8, &count);
    if (props != NULL) {
        work->centerProp = props[0];
        FieldChunkPropHolder_GetPosAbs(props[0], &work->basePosition);
    }
    GFL_HeapFree(props);
    if (work->centerProp != NULL) {
        work->centerHandle = FieldPropSystem_CreateHandleFromExisting(work->propSystem, work->centerProp);
    }
}

void EventHallOfFameMachine_End(EventHallOfFameMachineData *work) {
    if (work->centerProp != NULL) {
        FieldPropHandle_Free(work->centerHandle);
    }
}

void EventHallOfFameMachine_SpawnMonsBall(EventHallOfFameMachineData *work) {
    SRTMatrix transform;
    u8 index;

    index = work->spawnedCount;
    if (work->ballCount > index) {
        VEC_Set(&transform.scale, FX32_ONE, FX32_ONE, FX32_ONE);
        MAT3_RotationEulerZYX(0, 0, 0, &transform.rotation);
        VEC_Add(&work->basePosition, &sBallPositions[index], &transform.translation);
        work->ballHandles[index] = FieldPropSystem_CreateHandleNew(work->propSystem, 0x62, &transform);
        work->spawnedCount++;
        GFL_SndSEPlay(0x568);
    }
}

void EventHallOfFameMachine_StartAnimations(EventHallOfFameMachineData *work) {
    s32 i;

    for (i = 0; i < work->spawnedCount; i++) {
        FieldPropHandle_CallAnmCmd(work->ballHandles[i], 0, 2);
    }
    if (work->centerProp != NULL) {
        FieldPropHandle_CallAnmCmd(work->centerHandle, 0, 0);
    }
}

BOOL EventHallOfFameMachine_IsAnimationDone(EventHallOfFameMachineData *work) {
    if (work->centerProp != NULL) {
        return FieldPropHandle_IsAnmFinished(work->centerHandle);
    }
    return TRUE;
}

int countNonEggsInParty(GameSystem *gsys) {
    return howManyPartyPokesAreNotEggs(GameData_GetParty(GSYS_GetGameData(gsys)));
}
