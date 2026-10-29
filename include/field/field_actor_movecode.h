#ifndef POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_H
#define POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_H

// Overlay 36's field_actor_movecode.c, whose name is a guess: the move codes by which actors look around, wander,
// turn, pace and walk around a square

#include "types.h"
#include "struct_decls.h"

// The functions of the move codes, which field_actor_movedata.c's tables list
void FieldActorMovecode_InitTurnRandomSet0(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet1(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet2(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet3(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet4(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet5(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet6(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet7(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet8(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet9(FieldActor *actor);
void FieldActorMovecode_InitTurnRandomSet10(FieldActor *actor);
void FieldActorMovecode_MoveTurnRandom(FieldActor *actor);
void func_ov036_021913dc(FieldActor *actor);
void FieldActorMovecode_InitWalkRandomSet11(FieldActor *actor);
void FieldActorMovecode_InitWalkRandomSet12(FieldActor *actor);
void FieldActorMovecode_InitWalkRandomSet13(FieldActor *actor);
void FieldActorMovecode_InitWalkRandomSet13Bump(FieldActor *actor);
void FieldActorMovecode_MoveWalkRandom(FieldActor *actor);
void FieldActorMovecode_MoveFixedDir(FieldActor *actor);
void FieldActorMovecode_InitFixedDirUp(FieldActor *actor);
void FieldActorMovecode_InitFixedDirDown(FieldActor *actor);
void FieldActorMovecode_InitFixedDirLeft(FieldActor *actor);
void FieldActorMovecode_InitFixedDirRight(FieldActor *actor);
void FieldActorMovecode_InitSpinLeft(FieldActor *actor);
void FieldActorMovecode_InitSpinRight(FieldActor *actor);
void FieldActorMovecode_MoveSpin(FieldActor *actor);
void FieldActorMovecode_InitSwing(FieldActor *actor);
void FieldActorMovecode_MoveSwing(FieldActor *actor);
void FieldActorMovecode_InitBackForth(FieldActor *actor);
void FieldActorMovecode_MoveBackForth(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet14(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet15(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet16(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet17(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet18(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet19(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet20(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet21(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet22(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet23(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet24(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet25(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet26(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet27(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet28(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSet29(FieldActor *actor);
void FieldActorMovecode_MoveRoutePatrol(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet30(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet31(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet32(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet33(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet34(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet35(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet36(FieldActor *actor);
void FieldActorMovecode_InitRoutePatrolSwapSet37(FieldActor *actor);
void FieldActorMovecode_MoveRoutePatrolSwap(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_ACTOR_MOVECODE_H
