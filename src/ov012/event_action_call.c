#include "types.h"
#include "field/event_action_call.h"
#include "field/event_actor_move.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"

const u32 ACMD_QUEUE_WALK_N_8F[2] = { ACMD(0xc, 1), ACMD_END };
const u32 ACMD_QUEUE_WALK_S_8F[2] = { ACMD(0xd, 1), ACMD_END };
const u32 ACMD_QUEUE_WALK_W_8F[2] = { ACMD(0xe, 1), ACMD_END };
const u32 ACMD_QUEUE_WALK_E_8F[2] = { ACMD(0xf, 1), ACMD_END };

const fx32 JUMP_ANY_HEIGHT_TABLE[16] = {
    2 * FX32_ONE,
    4 * FX32_ONE,
    6 * FX32_ONE,
    8 * FX32_ONE,
    9 * FX32_ONE,
    10 * FX32_ONE,
    10 * FX32_ONE,
    10 * FX32_ONE,
    9 * FX32_ONE,
    8 * FX32_ONE,
    6 * FX32_ONE,
    5 * FX32_ONE,
    3 * FX32_ONE,
    2 * FX32_ONE,
    0,
    0,
};

GameEvent *CallEventPrepareResidentActorsForZoneChange(GameSystem *gsys, Field *field) {
    GameEvent *event =
        GameEvent_Create(gsys, NULL, EventPrepareResidentActors_Callback, sizeof(PrepareResidentActorsWork));
    PrepareResidentActorsWork *work = GameEvent_GetData(event);
    work->gameSystem = gsys;
    work->field = field;
    work->gameData = GSYS_GetGameData(gsys);
    return event;
}

GameEventReturnCode EventActionCall_Callback(GameEvent *event, u32 *state, void *data) {
    EventActionCallWork *work = data;
    MMSys *mmSys = Field_GetActorSystem(work->field);
    switch (*state) {
    case 0: {
        FieldActor *actor = EventActionCall_FindActor(mmSys, work->actorId);
        if (actor == NULL) {
            return GAMEEVENT_DONE;
        }
        FieldAcmdTCB *task = FieldAcmdTCB_Create(actor, work->action);
        EventActionCall_AddTCB(work, task);
        (*state)++;
        break;
    }
    case 1:
        if (!EventActionCall_UpdateTCBs(work)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventActionCall_Create(GameSystem *gsys, Field *field, u16 actorId, const u32 *action) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventActionCall_Callback, sizeof(EventActionCallWork));
    EventActionCallWork *work = GameEvent_GetData(event);
    EventActionCall_ClearTCBs(work);
    work->gameSystem = gsys;
    work->field = field;
    work->gameData = GSYS_GetGameData(gsys);
    work->actorId = actorId;
    work->action = action;
    return event;
}

GameEvent *CallMoveOneTileFrontEvent(GameSystem *gsys, Field *field) {
    u32 direction = GetActorFaceDir(FieldPlayer_GetActor(Field_GetPlayer(field)));
    const u32 *queue;
    switch (direction) {
    case 0:
        queue = ACMD_QUEUE_WALK_N_8F;
        break;
    case 1:
        queue = ACMD_QUEUE_WALK_S_8F;
        break;
    case 2:
        queue = ACMD_QUEUE_WALK_W_8F;
        break;
    case 3:
        queue = ACMD_QUEUE_WALK_E_8F;
        break;
    }
    return EventActionCall_Create(gsys, field, 0xff, queue);
}

FieldActor *EventActionCall_FindActor(MMSys *mmSys, u16 actorId) {
    FieldActor *actor;

    if (actorId == 0xf2) {
        actor = FindActorByMoveCode(mmSys, MOVE_CODE_FOLLOW_PLAYER);
    } else if (actorId != 0xf1) {
        actor = FindFieldActor(mmSys, actorId);
    }
    // Left unset for 0xf1, which returns whatever the register holds: the actor system
    return actor;
}

void EventActionCall_ClearTCBs(EventActionCallWork *work) {
    int i;
    for (i = 0; i < 8; i++) {
        work->tasks[i] = NULL;
    }
}

void EventActionCall_AddTCB(EventActionCallWork *work, FieldAcmdTCB *task) {
    int i;
    for (i = 0; i < 8; i++) {
        if (work->tasks[i] == NULL) {
            work->tasks[i] = task;
            return;
        }
    }
}

BOOL EventActionCall_UpdateTCBs(EventActionCallWork *work) {
    int i;
    BOOL active = 0;
    for (i = 0; i < 8; i++) {
        if (work->tasks[i] != NULL) {
            if (FieldAcmdTCB_CheckEnded(work->tasks[i]) == TRUE) {
                FieldAcmdTCB_Remove(work->tasks[i]);
                work->tasks[i] = NULL;
            } else {
                active = 1;
            }
        }
    }
    return active;
}

GameEventReturnCode EventActorJump_Callback(GameEvent *event, u32 *state, void *data) {
    EventActorJumpWork *work = data;
    VecFx32 position;
    fx32 landingY;
    fx32 y;
    BOOL done = FALSE;

    switch (*state) {
    case 0:
        position = work->start;
        position.y += JUMP_ANY_HEIGHT_TABLE[work->frame];
        work->frame += 2;
        position.x += FX_Div(FX_Mul(work->displacement.x, work->frame * FX32_ONE), 15 * FX32_ONE);
        position.z += FX_Div(FX_Mul(work->displacement.z, work->frame * FX32_ONE), 15 * FX32_ONE);
        SetActorWPosValue(work->actor, &position);
        if (work->frame > NELEMS(JUMP_ANY_HEIGHT_TABLE) - 1) {
            (*state)++;
        }
        break;
    case 1:
        CopyActorWPos(work->actor, &position);
        landingY = work->start.y + work->displacement.y;
        y = position.y - 8 * FX32_ONE;
        if (y > landingY) {
            position.y = y;
            SetActorWPosValue(work->actor, &position);
        } else {
            position.y = landingY;
            done = TRUE;
            position.x = work->start.x + work->displacement.x;
            position.z = work->start.z + work->displacement.z;
            SetActorWPosAll(work->actor, &position, GetActorFaceDir(work->actor));
            GFL_SndSEPlay(0x67b);
            FldeffDust_Start(work->actor, work->effects);
        }
        break;
    }
    return done;
}

GameEvent *EventActorJump_Create(GameSystem *gsys, FieldActor *actor, const VecFx32 *start, const VecFx32 *end) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventActorJump_Callback, sizeof(EventActorJumpWork));
    EventActorJumpWork *work = GameEvent_GetData(event);
    work->actor = actor;
    work->effects = Field_GetFieldEffects(GSYS_GetField(gsys));
    work->start = *start;
    VEC_Subtract(end, start, &work->displacement);
    GFL_SndSEPlay(0x55e);
    return event;
}

GameEventReturnCode EventActorLinearMove_Callback(GameEvent *event, u32 *state, void *data) {
    EventActorLinearMoveWork *work = data;
    fx32 progress;
    VecFx32 fromStart;
    VecFx32 fromEnd;
    VecFx32 position;

    GameEvent_GetGameSystem(event);
    switch (*state) {
    case 0:
        EnableActorMovement(work->actor);
        SetActorMovementFlag(work->actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW);
        *state = 1;
        break;
    case 1:
        work->frame++;
        progress = FX32_CONST((f32)work->frame / (f32)work->duration);
        vecfx_mul(&work->start, FX32_ONE - progress, &fromStart);
        vecfx_mul(&work->end, progress, &fromEnd);
        VEC_Add(&fromStart, &fromEnd, &position);
        SetActorWPosValue(work->actor, &position);
        if (work->duration <= work->frame) {
            *state = 2;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventActorLinearMove_Create(GameSystem *gsys, FieldActor *actor, const VecFx32 *start, const VecFx32 *end,
                                       s32 duration) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventActorLinearMove_Callback, sizeof(EventActorLinearMoveWork));
    EventActorLinearMoveWork *work = GameEvent_GetData(event);

    work->field = GSYS_GetField(gsys);
    work->frame = 0;
    work->duration = duration;
    work->actor = actor;
    VEC_Set(&work->start, start->x, start->y, start->z);
    VEC_Set(&work->end, end->x, end->y, end->z);
    return event;
}

FieldAcmdTCB *FieldAcmdTCB_CreateWalkOneTile(FieldActor *actor, u32 direction) {
    const u32 *queue;
    switch (direction) {
    case 0:
        queue = ACMD_QUEUE_WALK_N_8F;
        break;
    case 1:
        queue = ACMD_QUEUE_WALK_S_8F;
        break;
    case 2:
        queue = ACMD_QUEUE_WALK_W_8F;
        break;
    case 3:
        queue = ACMD_QUEUE_WALK_E_8F;
        break;
    }
    FldAct_FreeAcmd(actor);
    return FieldAcmdTCB_Create(actor, queue);
}
