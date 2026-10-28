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
    ACMD_DIR_TABLE,
    ACMD_FACE_DOWN_TABLE,
    ACMD_FACE_LEFT_TABLE,
    ACMD_FACE_RIGHT_TABLE,
    ACMD_WALK32_TABLE,
    ACMD_WALK_DOWN32_F_TABLE,
    ACMD_WALK_LEFT32_F_TABLE,
    ACMD_WALK_RIGHT32_F_TABLE,
    ACMD_WALK16_TABLE,
    ACMD_WALK_DOWN16_F_TABLE,
    ACMD_WALK_LEFT16_F_TABLE,
    ACMD_WALK_RIGHT16_F_TABLE,
    ACMD_WALK8_TABLE,
    ACMD_WALK_DOWN8_F_TABLE,
    ACMD_WALK_LEFT8_F_TABLE,
    ACMD_WALK_RIGHT8_F_TABLE,
    ACMD_WALK4_TABLE,
    ACMD_WALK_DOWN4_F_TABLE,
    ACMD_WALK_LEFT4_F_TABLE,
    ACMD_WALK_RIGHT4_F_TABLE,
    ACMD_WALK2_TABLE,
    ACMD_WALK_DOWN2_F_TABLE,
    ACMD_WALK_LEFT2_F_TABLE,
    ACMD_WALK_RIGHT2_F_TABLE,
    ACMD_TURN32_TABLE,
    ACMD_STAY_WALK_DOWN32_F_TABLE,
    ACMD_STAY_WALK_LEFT32_F_TABLE,
    ACMD_STAY_WALK_RIGHT32_F_TABLE,
    ACMD_TURN16_TABLE,
    ACMD_STAY_WALK_DOWN16_F_TABLE,
    ACMD_STAY_WALK_LEFT16_F_TABLE,
    ACMD_STAY_WALK_RIGHT16_F_TABLE,
    ACMD_TURN8_TABLE,
    ACMD_STAY_WALK_DOWN8_F_TABLE,
    ACMD_STAY_WALK_LEFT8_F_TABLE,
    ACMD_STAY_WALK_RIGHT8_F_TABLE,
    ACMD_TURN4_TABLE,
    ACMD_STAY_WALK_DOWN4_F_TABLE,
    ACMD_STAY_WALK_LEFT4_F_TABLE,
    ACMD_STAY_WALK_RIGHT4_F_TABLE,
    ACMD_TURN2_TABLE,
    ACMD_STAY_WALK_DOWN2_F_TABLE,
    ACMD_STAY_WALK_LEFT2_F_TABLE,
    ACMD_STAY_WALK_RIGHT2_F_TABLE,
    ACMD_JUMP16F,
    ACMD_STAY_JUMP_DOWN16_F_TABLE,
    ACMD_STAY_JUMP_LEFT16_F_TABLE,
    ACMD_STAY_JUMP_RIGHT16_F_TABLE,
    ACMD_JUMP8F,
    ACMD_STAY_JUMP_DOWN8_F_TABLE,
    ACMD_STAY_JUMP_LEFT8_F_TABLE,
    ACMD_STAY_JUMP_RIGHT8_F_TABLE,
    ACMD_JUMP_UP1_G8_F_TABLE,
    ACMD_JUMP_DOWN1_G8_F_TABLE,
    ACMD_JUMP_LEFT1_G8_F_TABLE,
    ACMD_JUMP_RIGHT1_G8_F_TABLE,
    ACMD_JUMP_UP2_G16_F_TABLE,
    ACMD_JUMP_DOWN2_G16_F_TABLE,
    ACMD_JUMP_LEFT2_G16_F_TABLE,
    ACMD_JUMP_RIGHT2_G16_F_TABLE,
    ACMD_WAIT1_F_TABLE,
    ACMD_WAIT2_F_TABLE,
    ACMD_WAIT4_F_TABLE,
    ACMD_WAIT8_F_TABLE,
    ACMD_WAIT15_F_TABLE,
    ACMD_WAIT16_F_TABLE,
    ACMD_WAIT32_F_TABLE,
    ACMD_RISE_INIT_TABLE,
    ACMD_FALL_INIT_TABLE,
    ACMD_SET_HIDDEN_FLAG_TABLE,
    ACMD_CLEAR_HIDDEN_FLAG_TABLE,
    ACMD_SET_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_CLEAR_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_SET_ANIME_PAUSE_FLAG_TABLE,
    ACMD_CLEAR_ANIME_PAUSE_FLAG_TABLE,
    ACMD_MARK_WAIT_EXCL_TABLE,
    ACMD_WALK_UP6_F_TABLE,
    ACMD_WALK_DOWN6_F_TABLE,
    ACMD_WALK_LEFT6_F_TABLE,
    ACMD_WALK_RIGHT6_F_TABLE,
    ACMD_WALK_UP3_F_TABLE,
    ACMD_WALK_DOWN3_F_TABLE,
    ACMD_WALK_LEFT3_F_TABLE,
    ACMD_WALK_RIGHT3_F_TABLE,
    ACMD_WALK_UP1_F_TABLE,
    ACMD_WALK_DOWN1_F_TABLE,
    ACMD_WALK_LEFT1_F_TABLE,
    ACMD_WALK_RIGHT1_F_TABLE,
    ACMD_RUN4F_TABLE,
    ACMD_DASH_DOWN4_F_TABLE,
    ACMD_DASH_LEFT4_F_TABLE,
    ACMD_DASH_RIGHT4_F_TABLE,
    ACMD_JUMP3G16F_TABLE,
    ACMD_JUMP_RIGHT1_G16_F_TABLE,
    ACMD_JUMP_LEFT3_G12_F_TABLE,
    ACMD_JUMP_RIGHT3_G12_F_TABLE,
    ACMD_WALK_UP7_F_TABLE,
    ACMD_WALK_DOWN7_F_TABLE,
    ACMD_WALK_LEFT7_F_TABLE,
    ACMD_WALK_RIGHT7_F_TABLE,
    ACMD_ANIME_WAIT_END_INIT10_ANIME_WAIT_END_STOP_TABLE,
    ACMD_HIDE_OFF_INIT_TABLE,
    ACMD_STAY_WALK32_F40_F_TABLE,
    data_ov036_021cb1b4,
    ACMD_STAY_WALK16_F41_F_TABLE,
    ACMD_STEP_UP_FACE_LEFT8_F_TABLE,
    ACMD_STEP_DOWN_FACE_RIGHT8_F_TABLE,
    ACMD_STEP_LEFT_FACE_DOWN8_F_TABLE,
    ACMD_STEP_RIGHT_FACE_UP8_F_TABLE,
    ACMD_STEP_UP_FACE_RIGHT8_F_TABLE,
    ACMD_STEP_DOWN_FACE_LEFT8_F_TABLE,
    ACMD_STEP_LEFT_FACE_UP8_F_TABLE,
    data_ov036_021cb1f0,
    ACMD_STEP_UP_FACE_DOWN8_F_TABLE,
    ACMD_STEP_DOWN_FACE_UP8_F_TABLE,
    ACMD_STEP_LEFT_FACE_RIGHT8_F_TABLE,
    ACMD_STEP_RIGHT_FACE_LEFT8_F_TABLE,
    ACMD_JUMP_UP3_G24_F_NO_SE_TABLE,
    ACMD_JUMP_DOWN3_G24_F_NO_SE_TABLE,
    ACMD_JUMP_LEFT3_G24_F_NO_SE_TABLE,
    ACMD_JUMP_RIGHT3_G24_F_NO_SE_TABLE,
    ACMD_DASH_STEP_UP_FACE_LEFT4_F_TABLE,
    ACMD_DASH_STEP_DOWN_FACE_RIGHT4_F_TABLE,
    ACMD_DASH_STEP_LEFT_FACE_DOWN4_F_TABLE,
    ACMD_DASH_STEP_RIGHT_FACE_UP4_F_TABLE,
    ACMD_DASH_STEP_UP_FACE_RIGHT4_F_TABLE,
    ACMD_DASH_STEP_DOWN_FACE_LEFT4_F_TABLE,
    ACMD_DASH_STEP_LEFT_FACE_UP4_F_TABLE,
    data_ov036_021cae60,
    ACMD_DASH_STEP_UP_FACE_DOWN4_F_TABLE,
    ACMD_DASH_STEP_DOWN_FACE_UP4_F_TABLE,
    ACMD_DASH_STEP_LEFT_FACE_RIGHT4_F_TABLE,
    ACMD_DASH_STEP_RIGHT_FACE_LEFT4_F_TABLE,
    ACMD_HOP_UP_FACE_LEFT8_F_TABLE,
    ACMD_HOP_DOWN_FACE_RIGHT8_F_TABLE,
    ACMD_HOP_LEFT_FACE_DOWN8_F_TABLE,
    ACMD_HOP_RIGHT_FACE_UP8_F_TABLE,
    ACMD_HOP_UP_FACE_RIGHT8_F_TABLE,
    ACMD_HOP_DOWN_FACE_LEFT8_F_TABLE,
    ACMD_HOP_LEFT_FACE_UP8_F_TABLE,
    data_ov036_021cb598,
    ACMD_HOP_UP_FACE_DOWN8_F_TABLE,
    ACMD_HOP_DOWN_FACE_UP8_F_TABLE,
    ACMD_HOP_LEFT_FACE_RIGHT8_F_TABLE,
    ACMD_HOP_RIGHT_FACE_LEFT8_F_TABLE,
    ACMD_STEP_UP_FACE_DOWN4_F_TABLE,
    ACMD_STEP_DOWN_FACE_UP4_F_TABLE,
    ACMD_STEP_LEFT_FACE_RIGHT4_F_TABLE,
    ACMD_STEP_RIGHT_FACE_LEFT4_F_TABLE,
    ACMD_STEP_UP_FACE_DOWN2_F_TABLE,
    ACMD_STEP_DOWN_FACE_UP2_F_TABLE,
    ACMD_STEP_LEFT_FACE_RIGHT2_F_TABLE,
    ACMD_STEP_RIGHT_FACE_LEFT2_F_TABLE,
    data_ov036_021cada0,
    ACMD_STAY_WALK_BOB_INIT_TABLE,
    ACMD_WALK_UP16_F_FAST_ANIME_TABLE,
    ACMD_WALK_DOWN16_F_FAST_ANIME_TABLE,
    ACMD_WALK_LEFT16_F_FAST_ANIME_TABLE,
    ACMD_WALK_RIGHT16_F_FAST_ANIME_TABLE,
    ACMD_MARK_WAIT_QUESTION_TABLE,
    ACMD_MARK_WAIT_NOTE_TABLE,
    ACMD_MARK_WAIT_ELLIPSIS_TABLE,
    ACMD_STAY_IDLE8_F_TABLE,
    ACMD_DASH_UP6_F_TABLE,
    ACMD_DASH_DOWN6_F_TABLE,
    ACMD_DASH_LEFT6_F_TABLE,
    ACMD_DASH_RIGHT6_F_TABLE,
    ACMD_WALK_UP12_F_TABLE,
    ACMD_WALK_DOWN12_F_TABLE,
    ACMD_WALK_LEFT12_F_TABLE,
    ACMD_WALK_RIGHT12_F_TABLE,
    ACMD_STAY_WALK22_F_INIT_TABLE,
    ACMD_SET_ANIME10_TABLE,
    ACMD_DASH_LEFT2_F_TABLE,
    ACMD_FLY_UP_INIT_TABLE,
    data_ov036_021cacc4,
    ACMD_ANIME_WAIT_END_FACE_DOWN_INIT_TABLE,
    ACMD_DRAW_READY_WAIT_INIT10_TABLE,
    ACMD_DRAW_READY_WAIT_INIT11_TABLE,
    ACMD_DRAW_READY_WAIT_INIT12_TABLE,
    ACMD_RESET_DRAW_OFFSET_TABLE,
    ACMD_ANIME_WAIT_END_INIT10_ANIME_WAIT_END_TABLE,
    ACMD_ANIME_WAIT_END_INIT11_ANIME_WAIT_END_STOP_TABLE,
    ACMD_ANIME_WAIT_END_INIT11_ANIME_WAIT_END_TABLE,
    ACMD_ANIME_WAIT_END_INIT10_SE_TABLE,
    ACMD_ANIME_WAIT_END_INIT11_SE_TABLE,
    ACMD_ANIME_WAIT_END_INIT12_ANIME_WAIT_END_STOP_TABLE,
    ACMD_ANIME_WAIT_END_INIT12_ANIME_WAIT_END_TABLE,
    ACMD_FLY_READY_INIT_TABLE,
    ACMD_MARK_WAIT_EXCL_NO_SE_TABLE,
    ACMD_MARK_WAIT_NOTE_NO_SE_TABLE,
    data_ov036_021cacb4,
    ACMD_DRAW_READY_WAIT_INIT13_TABLE,
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
    ACMD_WAIT1_F_TABLE,
    ACMD_WAIT2_F_TABLE,
    ACMD_WAIT4_F_TABLE,
    ACMD_WAIT8_F_TABLE,
    ACMD_WAIT15_F_TABLE,
    ACMD_WAIT16_F_TABLE,
    ACMD_WAIT32_F_TABLE,
    data_ov036_021cf394,
    data_ov036_021cf394,
    ACMD_SET_HIDDEN_FLAG_TABLE,
    ACMD_CLEAR_HIDDEN_FLAG_TABLE,
    ACMD_SET_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_CLEAR_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_SET_ANIME_PAUSE_FLAG_TABLE,
    ACMD_CLEAR_ANIME_PAUSE_FLAG_TABLE,
    ACMD_MARK_WAIT_EXCL_TABLE,
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
    ACMD_STAY_WALK_BOB_INIT_TABLE,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    data_ov036_021cf394,
    ACMD_STAY_IDLE8_F_TABLE,
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
    ACMD_MARK_WAIT_EXCL_NO_SE_TABLE,
    ACMD_MARK_WAIT_NOTE_NO_SE_TABLE,
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
