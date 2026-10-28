// The actors' tables of functions: the move codes' by move code (ACTOR_MOVE_CODE_FUNCTION_TABLES), the action
// commands' by command, for the actors on the grid (ACTOR_ACTION_COMMAND_TABLES) and on the rails
// (ACTOR_ACTION_COMMAND_TABLES_RAIL), and the rows of the commands that differ only by direction (ACMD_DIR_LUT). The
// table names are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). The ROM gives this file no name, so
// field_actor_movedata.c is a guess. It holds only data, whose .rodata (0x021cc004-0x021cd0f0 in Black 2) is one run
// sorted by size after field_actor_movecode_pair.c's, laid out as the original's only in this order of definitions. The
// tables were first written by mariano54 on their parallel-decomp branch

#include "types.h"
#include "constants/directions.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_actor_internal.h"
#include "field/field_actor_movecode.h"
#include "field/field_actor_movecode_pair.h"
#include "field/field_comm_actor.h"
#include "field/fld_act_rail.h"

// The functions of each move code, by move code, except that 0x2d and 0x2e follow 0xd, 0x31 follows 0x13 and 0x43
// follows 0x5, the codes whose functions come just before theirs
static const FieldActorMoveCode data_ov036_021cc59c = { 0, MoveCodeNone_Init, MoveCodeNone_Move, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc5b0 = { 0x1, MoveCodeNone_Init, MoveCodeNone_Move, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc420 = { 0x2, FieldActorMovecode_InitTurnRandomSet0,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc8e4 = { 0x3, FieldActorMovecode_InitWalkRandomSet11,
                                                        FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc8d0 = { 0x3, FieldActorMovecode_InitWalkRandomSet12,
                                                        FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc3f8 = { 0x3, FieldActorMovecode_InitWalkRandomSet13,
                                                        FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc268 = { 0x3, FieldActorMovecode_InitWalkRandomSet13Bump,
                                                        FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc894 = { 0x3, FieldActorMovecode_InitTurnRandomSet1,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc880 = { 0x3, FieldActorMovecode_InitTurnRandomSet2,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc86c = { 0x3, FieldActorMovecode_InitTurnRandomSet3,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc3d0 = { 0x3, FieldActorMovecode_InitTurnRandomSet4,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc844 = { 0x3, FieldActorMovecode_InitTurnRandomSet5,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc830 = { 0x3, FieldActorMovecode_InitTurnRandomSet6,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc2a4 = { 0x3, FieldActorMovecode_InitTurnRandomSet7,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc2b8 = { 0x3, FieldActorMovecode_InitTurnRandomSet8,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc7f4 = { 0x3, FieldActorMovecode_InitTurnRandomSet9,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc7e0 = { 0x3, FieldActorMovecode_InitTurnRandomSet10,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc7cc = { 0x3, FieldActorMovecode_InitFixedDirUp,
                                                        FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc380 = { 0x3, FieldActorMovecode_InitFixedDirDown,
                                                        FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc7a4 = { 0x3, FieldActorMovecode_InitFixedDirLeft,
                                                        FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc790 = { 0x3, FieldActorMovecode_InitFixedDirRight,
                                                        FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc358 = { 0x3, FieldActorMovecode_InitSpinClockwise,
                                                        FieldActorMovecode_MoveSpin, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc2f4 = { 0x3, FieldActorMovecode_InitSpinCounterClockwise,
                                                        FieldActorMovecode_MoveSpin, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc754 = { 0x3, FieldActorMovecode_InitSwing, FieldActorMovecode_MoveSwing,
                                                        MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc740 = { 0x3, FieldActorMovecode_InitBackForth,
                                                        FieldActorMovecode_MoveBackForth, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc72c = { 0x3, FieldActorMovecode_InitRoutePatrolSet14,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc330 = { 0x3, FieldActorMovecode_InitRoutePatrolSet15,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc704 = { 0x3, FieldActorMovecode_InitRoutePatrolSet16,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc3a8 = { 0x3, FieldActorMovecode_InitRoutePatrolSet17,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc45c = { 0x3, FieldActorMovecode_InitRoutePatrolSet18,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc470 = { 0x3, FieldActorMovecode_InitRoutePatrolSet19,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc6b4 = { 0x3, FieldActorMovecode_InitRoutePatrolSet20,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc6a0 = { 0x3, FieldActorMovecode_InitRoutePatrolSet21,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc68c = { 0x3, FieldActorMovecode_InitRoutePatrolSet22,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc4ac = { 0x3, FieldActorMovecode_InitRoutePatrolSet23,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc664 = { 0x3, FieldActorMovecode_InitRoutePatrolSet24,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc650 = { 0x3, FieldActorMovecode_InitRoutePatrolSet25,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc4e8 = { 0x3, FieldActorMovecode_InitRoutePatrolSet26,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc4fc = { 0x3, FieldActorMovecode_InitRoutePatrolSet27,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc614 = { 0x3, FieldActorMovecode_InitRoutePatrolSet28,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc600 = { 0x3, FieldActorMovecode_InitRoutePatrolSet29,
                                                        FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc5ec = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet30,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc538 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet31,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc5c4 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet32,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc560 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet33,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc574 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet34,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc588 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet35,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc5d8 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet36,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc628 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet37,
                                                        FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc63c = { 0x3, FieldActorMove_InitLookPlayer,
                                                        FieldActorMove_MoveLookPlayer, FieldActorMove_DeleteLookPlayer,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc678 = { 0x3, FieldActorMove_InitLookFellow,
                                                        FieldActorMove_MoveLookFellow, FieldActorMove_DeleteLookFellow,
                                                        FieldActorMove_ClearFellowTracking };

static const FieldActorMoveCode data_ov036_021cc6c8 = { 0x33, FieldActorMove_InitHideKind0Code33,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

static const FieldActorMoveCode data_ov036_021cc6dc = { 0x34, FieldActorMove_InitHideKind1Code34,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

static const FieldActorMoveCode data_ov036_021cc6f0 = { 0x35, FieldActorMove_InitHideKind1Code35,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

static const FieldActorMoveCode data_ov036_021cc718 = { 0x36, FieldActorMove_InitHideKind0Code36,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

static const FieldActorMoveCode data_ov036_021cc768 = { 0x37, FieldActorMove_InitFaceDirSet0,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc77c = { 0x38, FieldActorMove_InitFaceDirSet1,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc7b8 = { 0x39, FieldActorMove_InitFaceDirSet2,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc808 = { 0x3a, FieldActorMove_InitFaceDirSet3,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc81c = { 0x3b, FieldActorMove_InitFaceDirFlagSet0,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc858 = { 0x3c, FieldActorMove_InitFaceDirFlagSet1,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc8a8 = { 0x3d, FieldActorMove_InitFaceDirFlagSet2,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc8bc = { 0x3e, FieldActorMove_InitFaceDirFlagSet3,
                                                        FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc434 = { 0x3f, FieldActorMove_InitWallWalkLeftHand,
                                                        FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc254 = { 0x40, FieldActorMove_InitWallWalkRightHand,
                                                        FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc40c = { 0x41, FieldActorMove_InitWallWalkLeftHandReversing,
                                                        FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc27c = { 0x42, FieldActorMove_InitWallWalkRightHandReversing,
                                                        FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc3e4 = { 0x44, FieldCommActor_MoveInit, FieldCommActor_Move,
                                                        FieldCommActor_MoveDelete, MoveCodeNone_Return };

static const FieldActorMoveCode data_ov036_021cc290 = { 0x45, RailActor_StayInit, RailActor_StayMove,
                                                        RailActor_StayDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc3bc = { 0x46, RailActor_WaitInitAll, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc2cc = { 0x47, RailActor_WaitInitUpLeft, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc394 = { 0x48, RailActor_WaitInitUpRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc2e0 = { 0x49, RailActor_WaitInitDownLeft, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc36c = { 0x4a, RailActor_WaitInitDownRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc308 = { 0x4b, RailActor_WaitInitUpDownLeft, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc31c = { 0x4c, RailActor_WaitInitUpDownRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc344 = { 0x4d, RailActor_WaitInitUpLeftRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc448 = { 0x4e, RailActor_WaitInitDownLeftRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc484 = { 0x4f, RailActor_WaitInitUpDown, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc498 = { 0x50, RailActor_WaitInitLeftRight, RailActor_WaitMove,
                                                        RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc4c0 = { 0x51, RailActor_WalkInitAll, RailActor_WalkMove,
                                                        RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc4d4 = { 0x52, RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                        RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc510 = { 0x53, RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                        RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc524 = { 0x54, RailActor_PatrolInit, RailActor_PatrolMove,
                                                        RailActor_PatrolDelete, RailActor_Recover };

static const FieldActorMoveCode data_ov036_021cc54c = { 0x55, FieldActorMove_InitHideKind2Code55,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

const FieldActorMoveCode *const ACTOR_MOVE_CODE_FUNCTION_TABLES[] = {
    &data_ov036_021cc59c, &data_ov036_021cc5b0, &data_ov036_021cc420, &data_ov036_021cc8e4,
    &data_ov036_021cc8d0, &data_ov036_021cc3f8, &data_ov036_021cc894, &data_ov036_021cc880,
    &data_ov036_021cc86c, &data_ov036_021cc3d0, &data_ov036_021cc844, &data_ov036_021cc830,
    &data_ov036_021cc2a4, &data_ov036_021cc2b8, &data_ov036_021cc7cc, &data_ov036_021cc380,
    &data_ov036_021cc7a4, &data_ov036_021cc790, &data_ov036_021cc358, &data_ov036_021cc2f4,
    &data_ov036_021cc740, &data_ov036_021cc72c, &data_ov036_021cc330, &data_ov036_021cc704,
    &data_ov036_021cc3a8, &data_ov036_021cc45c, &data_ov036_021cc470, &data_ov036_021cc6b4,
    &data_ov036_021cc6a0, &data_ov036_021cc68c, &data_ov036_021cc4ac, &data_ov036_021cc664,
    &data_ov036_021cc650, &data_ov036_021cc4e8, &data_ov036_021cc4fc, &data_ov036_021cc614,
    &data_ov036_021cc600, &data_ov036_021cc5ec, &data_ov036_021cc538, &data_ov036_021cc5c4,
    &data_ov036_021cc560, &data_ov036_021cc574, &data_ov036_021cc588, &data_ov036_021cc5d8,
    &data_ov036_021cc628, &data_ov036_021cc7f4, &data_ov036_021cc7e0, NULL,
    &data_ov036_021cc63c, &data_ov036_021cc754, &data_ov036_021cc678, &data_ov036_021cc6c8,
    &data_ov036_021cc6dc, &data_ov036_021cc6f0, &data_ov036_021cc718, &data_ov036_021cc768,
    &data_ov036_021cc77c, &data_ov036_021cc7b8, &data_ov036_021cc808, &data_ov036_021cc81c,
    &data_ov036_021cc858, &data_ov036_021cc8a8, &data_ov036_021cc8bc, &data_ov036_021cc434,
    &data_ov036_021cc254, &data_ov036_021cc40c, &data_ov036_021cc27c, &data_ov036_021cc268,
    &data_ov036_021cc3e4, &data_ov036_021cc290, &data_ov036_021cc3bc, &data_ov036_021cc2cc,
    &data_ov036_021cc394, &data_ov036_021cc2e0, &data_ov036_021cc36c, &data_ov036_021cc308,
    &data_ov036_021cc31c, &data_ov036_021cc344, &data_ov036_021cc448, &data_ov036_021cc484,
    &data_ov036_021cc498, &data_ov036_021cc4c0, &data_ov036_021cc4d4, &data_ov036_021cc510,
    &data_ov036_021cc524, &data_ov036_021cc54c,
};

const ActorAcmdFunc *const ACTOR_ACTION_COMMAND_TABLES[] = {
    ACMD_DIR_TABLE,      data_ov036_021cad0c, data_ov036_021cad14,  data_ov036_021caccc, ACMD_WALK32_TABLE,
    data_ov036_021cb580, data_ov036_021cb574, data_ov036_021cb568,  ACMD_WALK16_TABLE,   data_ov036_021cb550,
    data_ov036_021cb544, data_ov036_021cb538, ACMD_WALK8_TABLE,     data_ov036_021cb520, data_ov036_021cb514,
    data_ov036_021cb508, ACMD_WALK4_TABLE,    data_ov036_021cb4f0,  data_ov036_021cb4e4, data_ov036_021cb4d8,
    ACMD_WALK2_TABLE,    data_ov036_021cb4c0, data_ov036_021cb4b4,  data_ov036_021cb4a8, ACMD_TURN32_TABLE,
    data_ov036_021cb460, data_ov036_021cb454, data_ov036_021cb448,  ACMD_TURN16_TABLE,   data_ov036_021cb430,
    data_ov036_021cb424, data_ov036_021cb418, ACMD_TURN8_TABLE,     data_ov036_021cb400, data_ov036_021cb3f4,
    data_ov036_021cb3e8, ACMD_TURN4_TABLE,    data_ov036_021cb010,  data_ov036_021cb3c4, data_ov036_021cb3b8,
    ACMD_TURN2_TABLE,    data_ov036_021cb3a0, data_ov036_021cb394,  data_ov036_021cb388, ACMD_JUMP16F,
    data_ov036_021cb370, data_ov036_021cb364, data_ov036_021cb358,  ACMD_JUMP8F,         data_ov036_021cb340,
    data_ov036_021cb334, data_ov036_021cb328, data_ov036_021cb31c,  data_ov036_021cb310, data_ov036_021cb304,
    data_ov036_021cb2f8, data_ov036_021cb2ec, data_ov036_021cb2e0,  data_ov036_021cb2d4, data_ov036_021cb2c8,
    data_ov036_021cb28c, data_ov036_021cb280, data_ov036_021cb250,  data_ov036_021cb25c, data_ov036_021cb274,
    data_ov036_021cb2b0, data_ov036_021cb244, data_ov036_021cb238,  data_ov036_021cb22c, data_ov036_021cad04,
    data_ov036_021cacec, data_ov036_021cacac, data_ov036_021cacdc,  data_ov036_021cacd4, data_ov036_021cacfc,
    data_ov036_021cadb8, data_ov036_021cb184, data_ov036_021cb178,  data_ov036_021cb16c, data_ov036_021cb160,
    data_ov036_021cb124, data_ov036_021cb118, data_ov036_021cb10c,  data_ov036_021cb100, data_ov036_021cb49c,
    data_ov036_021cb490, data_ov036_021cb484, data_ov036_021cb478,  ACMD_RUN4F_TABLE,    data_ov036_021cb0e8,
    data_ov036_021cb0dc, data_ov036_021cb0d0, ACMD_JUMP3G16F_TABLE, data_ov036_021cb1e4, data_ov036_021cb2a4,
    data_ov036_021cb298, data_ov036_021cb0c4, data_ov036_021cb0b8,  data_ov036_021cb0ac, data_ov036_021cb0a0,
    data_ov036_021cb064, data_ov036_021cb004, data_ov036_021caff8,  data_ov036_021cb1b4, data_ov036_021cafec,
    data_ov036_021cafd4, data_ov036_021cafc8, data_ov036_021cafbc,  data_ov036_021cafb0, data_ov036_021caf80,
    data_ov036_021caf8c, data_ov036_021cafa4, data_ov036_021cb1f0,  data_ov036_021cb208, data_ov036_021cb214,
    data_ov036_021cb268, data_ov036_021cb3d0, data_ov036_021caee4,  data_ov036_021caed8, data_ov036_021caecc,
    data_ov036_021cad94, data_ov036_021caeb4, data_ov036_021caea8,  data_ov036_021cae9c, data_ov036_021cae90,
    data_ov036_021cae84, data_ov036_021cae78, data_ov036_021cae6c,  data_ov036_021cae60, data_ov036_021cae54,
    data_ov036_021cae48, data_ov036_021cae3c, data_ov036_021cae00,  data_ov036_021cae18, data_ov036_021cae24,
    data_ov036_021caf50, data_ov036_021caf5c, data_ov036_021caf98,  data_ov036_021cb1fc, data_ov036_021cb220,
    data_ov036_021cb598, data_ov036_021cadd0, data_ov036_021cad1c,  data_ov036_021caf08, data_ov036_021cadac,
    data_ov036_021caf44, data_ov036_021caf38, data_ov036_021caf2c,  data_ov036_021cadc4, data_ov036_021cb1d8,
    data_ov036_021cb1cc, data_ov036_021caefc, data_ov036_021caef0,  data_ov036_021cada0, data_ov036_021cad40,
    data_ov036_021cad88, data_ov036_021cad7c, data_ov036_021cad70,  data_ov036_021caddc, data_ov036_021cb1a8,
    data_ov036_021cb19c, data_ov036_021cb190, data_ov036_021cafe0,  data_ov036_021cb154, data_ov036_021cb148,
    data_ov036_021cb13c, data_ov036_021caec0, data_ov036_021cb094,  data_ov036_021cb088, data_ov036_021cb07c,
    data_ov036_021cb070, data_ov036_021cadf4, data_ov036_021cacbc,  data_ov036_021caf20, data_ov036_021caf68,
    data_ov036_021cacc4, data_ov036_021cb5a4, data_ov036_021cad4c,  data_ov036_021cade8, data_ov036_021cae0c,
    data_ov036_021cace4, data_ov036_021cb058, data_ov036_021cb04c,  data_ov036_021cb040, data_ov036_021cb01c,
    data_ov036_021cae30, data_ov036_021cb034, data_ov036_021cb028,  data_ov036_021caf14, data_ov036_021cad28,
    data_ov036_021cb1c0, data_ov036_021cacb4, data_ov036_021caf74,
};

// The rail actors' functions of the action commands: fld_act_rail.c's, field_actor_acmd.c's for the commands that work
// the same on the rails, and data_ov036_021cf394, which only moves on to the next state, for the others
const ActorAcmdFunc *const ACTOR_ACTION_COMMAND_TABLES_RAIL[] = {
    data_ov036_021cf384,
    data_ov036_021cf38c,
    data_ov036_021cf37c,
    data_ov036_021cf374,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf588,
    data_ov036_021cf57c,
    data_ov036_021cf570,
    data_ov036_021cf564,
    data_ov036_021cf558,
    data_ov036_021cf54c,
    data_ov036_021cf540,
    data_ov036_021cf534,
    data_ov036_021cf528,
    data_ov036_021cf51c,
    data_ov036_021cf510,
    data_ov036_021cf504,
    data_ov036_021cf4f8,
    data_ov036_021cf4c8,
    data_ov036_021cf4d4,
    data_ov036_021cf4ec,
    data_ov036_021cf3b4,
    data_ov036_021cf5d0,
    data_ov036_021cf480,
    data_ov036_021cf474,
    data_ov036_021cf468,
    data_ov036_021cf45c,
    data_ov036_021cf450,
    data_ov036_021cf444,
    data_ov036_021cf438,
    data_ov036_021cf414,
    data_ov036_021cf420,
    data_ov036_021cf4e0,
    data_ov036_021cf5a0,
    data_ov036_021cf3fc,
    data_ov036_021cf39c,
    data_ov036_021cf48c,
    data_ov036_021cf3d8,
    data_ov036_021cf3c0,
    data_ov036_021cf3cc,
    data_ov036_021cf42c,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf5ac,
    data_ov036_021cf498,
    data_ov036_021cf3e4,
    data_ov036_021cf408,
    data_ov036_021cf5b8,
    data_ov036_021cf3a8,
    data_ov036_021cf5c4,
    data_ov036_021cf3f0,
    data_ov036_021cb28c,
    data_ov036_021cb280,
    data_ov036_021cb250,
    data_ov036_021cb25c,
    data_ov036_021cb274,
    data_ov036_021cb2b0,
    data_ov036_021cb244,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cad04,
    data_ov036_021cacec,
    data_ov036_021cacac,
    data_ov036_021cacdc,
    data_ov036_021cacd4,
    data_ov036_021cacfc,
    data_ov036_021cadb8,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf594,
    data_ov036_021cf4bc,
    data_ov036_021cf4b0,
    data_ov036_021cf4a4,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cad40,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cafe0,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cad28,
    data_ov036_021cb1c0,
    data_ov036_021cf394,
    NULL,
};

// The commands of each direction (up, down, left and right) of a command that differs only by direction, by command,
// except that 0x75 comes before 0x69, 0x91 and 0x95 before 0x79, and 0xa7 before 0xa3
static const u32 data_ov036_021cc0c4[] = { 0, 0x1, 0x2, 0x3 };

static const u32 data_ov036_021cc144[] = { 0x4, 0x5, 0x6, 0x7 };

static const u32 data_ov036_021cc044[] = { 0x8, 0x9, 0xa, 0xb };

static const u32 data_ov036_021cc094[] = { 0xc, 0xd, 0xe, 0xf };

static const u32 data_ov036_021cc004[] = { 0x10, 0x11, 0x12, 0x13 };

static const u32 data_ov036_021cc164[] = { 0x14, 0x15, 0x16, 0x17 };

static const u32 data_ov036_021cc0a4[] = { 0x18, 0x19, 0x1a, 0x1b };

static const u32 data_ov036_021cc1f4[] = { 0x1c, 0x1d, 0x1e, 0x1f };

static const u32 data_ov036_021cc204[] = { 0x20, 0x21, 0x22, 0x23 };

static const u32 data_ov036_021cc154[] = { 0x24, 0x25, 0x26, 0x27 };

static const u32 data_ov036_021cc0f4[] = { 0x28, 0x29, 0x2a, 0x2b };

static const u32 data_ov036_021cc064[] = { 0x2c, 0x2d, 0x2e, 0x2f };

static const u32 data_ov036_021cc234[] = { 0x30, 0x31, 0x32, 0x33 };

static const u32 data_ov036_021cc104[] = { 0x34, 0x35, 0x36, 0x37 };

static const u32 data_ov036_021cc014[] = { 0x38, 0x39, 0x3a, 0x3b };

static const u32 data_ov036_021cc114[] = { 0x4c, 0x4d, 0x4e, 0x4f };

static const u32 data_ov036_021cc024[] = { 0x50, 0x51, 0x52, 0x53 };

static const u32 data_ov036_021cc054[] = { 0x54, 0x55, 0x56, 0x57 };

static const u32 data_ov036_021cc1d4[] = { 0x58, 0x59, 0x5a, 0x5b };

static const u32 data_ov036_021cc134[] = { 0x5c, 0x5d, 0x5c, 0x5d };

static const u32 data_ov036_021cc1a4[] = { 0x5e, 0x5f, 0x5e, 0x5f };

static const u32 data_ov036_021cc034[] = { 0x60, 0x61, 0x62, 0x63 };

static const u32 data_ov036_021cc184[] = { 0x75, 0x76, 0x77, 0x78 };

static const u32 data_ov036_021cc0e4[] = { 0x69, 0x6a, 0x6b, 0x6c };

static const u32 data_ov036_021cc214[] = { 0x6d, 0x6e, 0x6f, 0x70 };

static const u32 data_ov036_021cc1b4[] = { 0x71, 0x72, 0x73, 0x74 };

static const u32 data_ov036_021cc1c4[] = { 0x91, 0x92, 0x93, 0x94 };

static const u32 data_ov036_021cc074[] = { 0x95, 0x96, 0x97, 0x98 };

static const u32 data_ov036_021cc0d4[] = { 0x79, 0x7a, 0x7b, 0x7c };

static const u32 data_ov036_021cc084[] = { 0x7d, 0x7e, 0x7f, 0x80 };

static const u32 data_ov036_021cc1e4[] = { 0x81, 0x82, 0x83, 0x84 };

static const u32 data_ov036_021cc174[] = { 0x85, 0x86, 0x87, 0x88 };

static const u32 data_ov036_021cc0b4[] = { 0x89, 0x8a, 0x8b, 0x8c };

static const u32 data_ov036_021cc124[] = { 0x8d, 0x8e, 0x8f, 0x90 };

static const u32 data_ov036_021cc224[] = { 0x9b, 0x9c, 0x9d, 0x9e };

static const u32 data_ov036_021cc244[] = { 0xa7, 0xa8, 0xa9, 0xaa };

static const u32 data_ov036_021cc194[] = { 0xa3, 0xa4, 0xa5, 0xa6 };

const u32 *const ACMD_DIR_LUT[] = {
    data_ov036_021cc0c4, data_ov036_021cc094,
    data_ov036_021cc1d4, data_ov036_021cc164,
    data_ov036_021cc004, data_ov036_021cc244,
    data_ov036_021cc194, data_ov036_021cc144,
    data_ov036_021cc044, data_ov036_021cc0a4,
    data_ov036_021cc1f4, data_ov036_021cc204,
    data_ov036_021cc154, data_ov036_021cc0f4,
    data_ov036_021cc064, data_ov036_021cc234,
    data_ov036_021cc104, data_ov036_021cc014,
    data_ov036_021cc114, data_ov036_021cc024,
    data_ov036_021cc054, data_ov036_021cc134,
    data_ov036_021cc1a4, data_ov036_021cc034,
    data_ov036_021cc0e4, data_ov036_021cc214,
    data_ov036_021cc184, data_ov036_021cc1b4,
    data_ov036_021cc0d4, data_ov036_021cc084,
    data_ov036_021cc1e4, data_ov036_021cc174,
    data_ov036_021cc0b4, data_ov036_021cc124,
    data_ov036_021cc1c4, data_ov036_021cc074,
    data_ov036_021cc224, NULL,
};
