#ifndef POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H
#define POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H

// Overlay 36's fldmmdl_movecode_pair.c, whose name is a guess: the move codes that follow another actor or the
// player, hide an actor under a field effect, turn it with the player and walk it along a wall

#include "types.h"
#include "struct_decls.h"

// The functions of the move codes, which fldmmdl_movedata.c's tables list
void FldmmdlMove_InitLookPlayer(FieldActor *actor);
void FldmmdlMove_MoveLookPlayer(FieldActor *actor);
void FldmmdlMove_DeleteLookPlayer(FieldActor *actor);
void FldmmdlMove_InitLookFellow(FieldActor *actor);
void FldmmdlMove_MoveLookFellow(FieldActor *actor);
void FldmmdlMove_DeleteLookFellow(FieldActor *actor);
void FldmmdlMove_ClearFellowTracking(FieldActor *actor);
void FldmmdlMove_InitHideKind0Code33(FieldActor *actor);
void FldmmdlMove_InitHideKind1Code34(FieldActor *actor);
void FldmmdlMove_InitHideKind1Code35(FieldActor *actor);
void FldmmdlMove_InitHideKind0Code36(FieldActor *actor);
void FldmmdlMove_InitHideKind2Code55(FieldActor *actor);
void FldmmdlMove_MoveHide(FieldActor *actor);
void FldmmdlMove_DeleteHide(FieldActor *actor);
void FldmmdlMove_RecoverHide(FieldActor *actor);
void FldmmdlMove_InitFaceDirSet0(FieldActor *actor);
void FldmmdlMove_InitFaceDirSet1(FieldActor *actor);
void FldmmdlMove_InitFaceDirSet2(FieldActor *actor);
void FldmmdlMove_InitFaceDirSet3(FieldActor *actor);
void FldmmdlMove_InitFaceDirFlagSet0(FieldActor *actor);
void FldmmdlMove_InitFaceDirFlagSet1(FieldActor *actor);
void FldmmdlMove_InitFaceDirFlagSet2(FieldActor *actor);
void FldmmdlMove_InitFaceDirFlagSet3(FieldActor *actor);
void FldmmdlMove_MoveFaceDir(FieldActor *actor);
void FldmmdlMove_InitWallWalkLeftHand(FieldActor *actor);
void FldmmdlMove_InitWallWalkRightHand(FieldActor *actor);
void FldmmdlMove_InitWallWalkLeftHandReversing(FieldActor *actor);
void FldmmdlMove_InitWallWalkRightHandReversing(FieldActor *actor);
void FldmmdlMove_MoveWallWalk(FieldActor *actor);

// The field effect that the actor hides in, and the setting of the flag that it has one
FieldEffectTask *FldmmdlMove_GetHideEffect(FieldActor *actor);
void FldmmdlMove_SetHideNoEffect(FieldActor *actor);

#endif // POKEBW2_FIELD_FLDMMDL_MOVECODE_PAIR_H
