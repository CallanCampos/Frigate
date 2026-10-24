#ifndef POKEBW2_FIELD_EVENT_ACTOR_MOVE_H
#define POKEBW2_FIELD_EVENT_ACTOR_MOVE_H

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct EventActorJumpWork {
    FieldEffects *effects;
    FieldActor *actor;
    s16 frame;
    u16 pad0A;
    VecFx32 start;
    VecFx32 displacement;
};

// Moves an actor in a straight line over a number of frames
typedef struct {
    Field *field;
    s32 frame;
    s32 duration;
    VecFx32 start;
    VecFx32 end;
    FieldActor *actor;
} EventActorLinearMoveWork;

// The height of a jump at each frame
extern const fx32 JUMP_ANY_HEIGHT_TABLE[16];

GameEventReturnCode EventActorJump_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventActorJump_Create(GameSystem *gsys, FieldActor *actor, const VecFx32 *start, const VecFx32 *end);
GameEventReturnCode EventActorLinearMove_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventActorLinearMove_Create(GameSystem *gsys, FieldActor *actor, const VecFx32 *start, const VecFx32 *end,
                                       s32 duration);

#endif // POKEBW2_FIELD_EVENT_ACTOR_MOVE_H
