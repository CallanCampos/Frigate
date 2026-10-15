#include "types.h"
#include "field/event_pokemon_center.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_prop.h"
#include "gfl/calctool.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/game_system.h"

// Where the balls go on the healing machine, from its position
// clang-format off
static const VecFx32 POKECEN_HEAL_MONSBALL_POSITIONS[6] = {
    { FX32_CONST(-4), FX32_CONST(14), FX32_CONST(-3) },
    { FX32_CONST(4), FX32_CONST(14), FX32_CONST(-3) },
    { FX32_CONST(9), FX32_CONST(14), FX32_CONST(4) },
    { FX32_CONST(4), FX32_CONST(14), FX32_CONST(8) },
    { FX32_CONST(-4), FX32_CONST(14), FX32_CONST(8) },
    { FX32_CONST(-9), FX32_CONST(14), FX32_CONST(4) },
};

// clang-format on

GameEvent *EventPokeCenHeal_Create(GameSystem *gsys, GameEvent *parent, u8 ballCount) {
    GameEvent *event;

    event = GameEvent_Create(gsys, parent, EventPokeCenHeal_Callback, sizeof(EventPokeCenHealData));
    EventPokeCenHeal_Init(GameEvent_GetData(event), gsys, ballCount);
    return event;
}

GameEventReturnCode EventPokeCenHeal_Callback(GameEvent *event, u32 *state, void *data) {
    EventPokeCenHealData *work;

    work = data;
    switch (*state) {
    case 0:
        if (work->frameCounter % 15 == 0) {
            EventPokeCenHeal_SpawnMonsBall(work);
            if (work->ballCount <= work->animationCount) {
                EventPokeCenHeal_ChangeState(work, state, 1);
            }
        }
        break;
    case 1:
        if (work->frameCounter > 10) {
            EventPokeCenHeal_StartAnimations(work);
            GameEvent_ChainNext(event, EventMEPlay_Create(Field_GetGameSystem(work->field), 0x514));
            EventPokeCenHeal_ChangeState(work, state, 2);
        }
        break;
    case 2:
        if (EventPokeCenHeal_IsAnimationDone(work)) {
            EventPokeCenHeal_PauseAnimation(work);
            EventPokeCenHeal_ChangeState(work, state, 3);
        }
        break;
    case 3:
        GameEvent_ChainNext(event, EventPushBGMFinish_Create(Field_GetGameSystem(work->field), 0, 6));
        EventPokeCenHeal_ChangeState(work, state, 4);
        break;
    case 4:
        EventPokeCenHeal_End(work);
        return GAMEEVENT_DONE;
    }
    work->frameCounter++;
    return GAMEEVENT_CONTINUE;
}

void EventPokeCenHeal_Init(EventPokeCenHealData *work, GameSystem *gsys, u8 ballCount) {
    u32 count;
    VecFx32 playerPosition;
    FieldPropAreaBounds bounds;
    Field *field;
    u16 heapId;
    FieldG3DMapper *mapper;
    FieldChunkPropHolder **props;

    field = GSYS_GetField(gsys);
    heapId = Field_GetHeapID(field);
    mapper = Field_GetG3DMapper(field);
    work->heapId = heapId;
    work->ballCount = ballCount;
    work->field = field;
    work->animationCount = 0;
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
    props = FieldPropSystem_FindPropsInArea(work->propSystem, &bounds, 3, &count);
    if (props != NULL) {
        work->centerProp = props[0];
        FieldChunkPropHolder_GetPosAbs(props[0], &work->basePosition);
    }
    GFL_HeapFree(props);
}

void EventPokeCenHeal_End(EventPokeCenHealData *work) {
    s32 i;

    for (i = 0; i < work->ballCount; i++) {
        FieldPropHandle_Free(work->ballHandles[i]);
    }
}

void EventPokeCenHeal_ChangeState(EventPokeCenHealData *work, u32 *state, u32 newState) {
    *state = newState;
    work->frameCounter = 0;
}

void EventPokeCenHeal_SpawnMonsBall(EventPokeCenHealData *work) {
    SRTMatrix transform;
    u8 index;

    index = work->animationCount;
    if (work->ballCount > index) {
        VEC_Set(&transform.scale, FX32_ONE, FX32_ONE, FX32_ONE);
        MAT3_RotationEulerZYX(0, 0, 0, &transform.rotation);
        VEC_Add(&work->basePosition, &POKECEN_HEAL_MONSBALL_POSITIONS[index], &transform.translation);
        work->ballHandles[index] = FieldPropSystem_CreateHandleNew(work->propSystem, 0x62, &transform);
        work->animationCount++;
        GFL_SndSEPlay(0x568);
    }
}

void EventPokeCenHeal_StartAnimations(EventPokeCenHealData *work) {
    s32 i;

    for (i = 0; i < work->animationCount; i++) {
        FieldPropHandle_CallAnmCmd(work->ballHandles[i], 0, 0);
    }
    if (work->centerProp != NULL) {
        FieldChunkPropHolder_CallAnmCmd(work->propSystem, work->centerProp, 0, 0);
    }
}

void EventPokeCenHeal_PauseAnimation(EventPokeCenHealData *work) {
    s32 i;

    for (i = 0; i < work->animationCount; i++) {
        FieldPropHandle_CallAnmCmd(work->ballHandles[i], 0, 3);
    }
}

BOOL EventPokeCenHeal_IsAnimationDone(EventPokeCenHealData *work) {
    s32 i;

    for (i = 0; i < work->animationCount; i++) {
        if (FieldPropHandle_IsAnmIdle(work->ballHandles[i], 0) != TRUE) {
            return FALSE;
        }
    }
    return TRUE;
}
