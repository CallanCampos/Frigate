#ifndef POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_PAIR_H
#define POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_PAIR_H

// Overlay 36's field_actor_movecode_pair.c, whose name is a guess: the move codes that follow another actor or the
// player, hide an actor under a field effect, turn it with the player and walk it along a wall

#include "types.h"
#include "struct_decls.h"

// The functions of the move codes, which field_actor_movedata.c's tables list
void FieldActorMove_InitLookPlayer(FieldActor *actor);
void FieldActorMove_MoveLookPlayer(FieldActor *actor);
void FieldActorMove_DeleteLookPlayer(FieldActor *actor);
void FieldActorMove_InitLookFellow(FieldActor *actor);
void FieldActorMove_MoveLookFellow(FieldActor *actor);
void FieldActorMove_DeleteLookFellow(FieldActor *actor);
void FieldActorMove_ClearFellowTracking(FieldActor *actor);
void FieldActorMove_InitHideKind0Code33(FieldActor *actor);
void FieldActorMove_InitHideKind1Code34(FieldActor *actor);
void FieldActorMove_InitHideKind1Code35(FieldActor *actor);
void FieldActorMove_InitHideKind0Code36(FieldActor *actor);
void FieldActorMove_InitHideKind2Code55(FieldActor *actor);
void FieldActorMove_MoveHide(FieldActor *actor);
void FieldActorMove_DeleteHide(FieldActor *actor);
void FieldActorMove_RecoverHide(FieldActor *actor);
void FieldActorMove_InitFaceDirSet0(FieldActor *actor);
void FieldActorMove_InitFaceDirSet1(FieldActor *actor);
void FieldActorMove_InitFaceDirSet2(FieldActor *actor);
void FieldActorMove_InitFaceDirSet3(FieldActor *actor);
void FieldActorMove_InitFaceDirFlagSet0(FieldActor *actor);
void FieldActorMove_InitFaceDirFlagSet1(FieldActor *actor);
void FieldActorMove_InitFaceDirFlagSet2(FieldActor *actor);
void FieldActorMove_InitFaceDirFlagSet3(FieldActor *actor);
void FieldActorMove_MoveFaceDir(FieldActor *actor);
void FieldActorMove_InitWallWalkLeftHand(FieldActor *actor);
void FieldActorMove_InitWallWalkRightHand(FieldActor *actor);
void FieldActorMove_InitWallWalkLeftHandReversing(FieldActor *actor);
void FieldActorMove_InitWallWalkRightHandReversing(FieldActor *actor);
void FieldActorMove_MoveWallWalk(FieldActor *actor);

// The field effect that the actor hides in, and the setting of the flag that it has one
FieldEffectTask *FieldActorMove_GetHideEffect(FieldActor *actor);
void FieldActorMove_SetHideNoEffect(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_PAIR_H
