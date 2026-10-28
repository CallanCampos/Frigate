#ifndef POKEBW2_FIELD_FLDMMDL_MOVECODE_H
#define POKEBW2_FIELD_FLDMMDL_MOVECODE_H

// Overlay 36's fldmmdl_movecode.c, whose name is a guess: the move codes by which actors look around, wander,
// turn, pace and walk around a square

#include "types.h"
#include "struct_decls.h"

// The functions of the move codes, which fldmmdl_movedata.c's tables list
void FldmmdlMovecode_InitTurnRandomSet0(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet1(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet2(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet3(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet4(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet5(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet6(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet7(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet8(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet9(FieldActor *actor);
void FldmmdlMovecode_InitTurnRandomSet10(FieldActor *actor);
void FldmmdlMovecode_MoveTurnRandom(FieldActor *actor);
void func_ov036_021913dc(FieldActor *actor);
void FldmmdlMovecode_InitWalkRandomSet11(FieldActor *actor);
void FldmmdlMovecode_InitWalkRandomSet12(FieldActor *actor);
void FldmmdlMovecode_InitWalkRandomSet13(FieldActor *actor);
void FldmmdlMovecode_InitWalkRandomSet13Bump(FieldActor *actor);
void FldmmdlMovecode_MoveWalkRandom(FieldActor *actor);
void FldmmdlMovecode_MoveFixedDir(FieldActor *actor);
void FldmmdlMovecode_InitFixedDirUp(FieldActor *actor);
void FldmmdlMovecode_InitFixedDirDown(FieldActor *actor);
void FldmmdlMovecode_InitFixedDirLeft(FieldActor *actor);
void FldmmdlMovecode_InitFixedDirRight(FieldActor *actor);
void FldmmdlMovecode_InitSpinClockwise(FieldActor *actor);
void FldmmdlMovecode_InitSpinCounterClockwise(FieldActor *actor);
void FldmmdlMovecode_MoveSpin(FieldActor *actor);
void FldmmdlMovecode_InitSwing(FieldActor *actor);
void FldmmdlMovecode_MoveSwing(FieldActor *actor);
void FldmmdlMovecode_InitBackForth(FieldActor *actor);
void FldmmdlMovecode_MoveBackForth(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet14(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet15(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet16(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet17(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet18(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet19(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet20(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet21(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet22(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet23(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet24(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet25(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet26(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet27(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet28(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSet29(FieldActor *actor);
void FldmmdlMovecode_MoveRoutePatrol(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet30(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet31(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet32(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet33(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet34(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet35(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet36(FieldActor *actor);
void FldmmdlMovecode_InitRoutePatrolSwapSet37(FieldActor *actor);
void FldmmdlMovecode_MoveRoutePatrolSwap(FieldActor *actor);

#endif // POKEBW2_FIELD_FLDMMDL_MOVECODE_H
