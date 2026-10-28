#ifndef POKEBW2_FIELD_EVENT_TRAINER_EYE_H
#define POKEBW2_FIELD_EVENT_TRAINER_EYE_H

#include "types.h"
#include "field/trainer_script.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// event_trainer_eye.c: the Trainers who see the player and walk up to battle. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The script of the special Trainers, whose Trainer ID is their actor's UID
#define SCRID_SPECIAL_TRAINER 0x29f9

typedef struct EventTrainerEyeWork EventTrainerEyeWork;

// The event of the Trainers that have seen the player, or NULL if none has
GameEvent *EventTrainerEye_CheckAll(Field *field);
// How far the actor, facing dir, sees the player within range, or -1 if it doesn't
int EventTrainerEyeSub_GetPlayerDistance(FieldActor *actor, u16 dir, int range, EncountSystem *encounter);
// The event that walks one or two Trainers up to the player
GameEvent *EventTrainerEye_Create(GameSystem *gsys, EventTrainerEyeWork *eye1, EventTrainerEyeWork *eye2);
EventTrainerEyeWork *EventTrainerEye_CreateData(HeapID heapId, Field *field, const TrainerClashData *data, BOOL isPair);
// The Trainer ID of a Trainer actor
u16 GetNPCTrainerID(FieldActor *actor);

#endif // POKEBW2_FIELD_EVENT_TRAINER_EYE_H
