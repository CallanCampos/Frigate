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
static const FieldActorMoveCode sMoveCode00 = { 0, MoveCodeNone_Init, MoveCodeNone_Move, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode01 = { 0x1, MoveCodeNone_Init, MoveCodeNone_Move, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode02 = { 0x2, FieldActorMovecode_InitTurnRandomSet0,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode03 = { 0x3, FieldActorMovecode_InitWalkRandomSet11,
                                                FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode04 = { 0x3, FieldActorMovecode_InitWalkRandomSet12,
                                                FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode05 = { 0x3, FieldActorMovecode_InitWalkRandomSet13,
                                                FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode42 = { 0x3, FieldActorMovecode_InitWalkRandomSet13Bump,
                                                FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode06 = { 0x3, FieldActorMovecode_InitTurnRandomSet1,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode07 = { 0x3, FieldActorMovecode_InitTurnRandomSet2,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode08 = { 0x3, FieldActorMovecode_InitTurnRandomSet3,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode09 = { 0x3, FieldActorMovecode_InitTurnRandomSet4,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0A = { 0x3, FieldActorMovecode_InitTurnRandomSet5,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0B = { 0x3, FieldActorMovecode_InitTurnRandomSet6,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0C = { 0x3, FieldActorMovecode_InitTurnRandomSet7,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0D = { 0x3, FieldActorMovecode_InitTurnRandomSet8,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2D = { 0x3, FieldActorMovecode_InitTurnRandomSet9,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2E = { 0x3, FieldActorMovecode_InitTurnRandomSet10,
                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0E = { 0x3, FieldActorMovecode_InitFixedDirUp, FieldActorMovecode_MoveFixedDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode0F = { 0x3, FieldActorMovecode_InitFixedDirDown,
                                                FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode10 = { 0x3, FieldActorMovecode_InitFixedDirLeft,
                                                FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode11 = { 0x3, FieldActorMovecode_InitFixedDirRight,
                                                FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode12 = { 0x3, FieldActorMovecode_InitSpinClockwise, FieldActorMovecode_MoveSpin,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode13 = { 0x3, FieldActorMovecode_InitSpinCounterClockwise,
                                                FieldActorMovecode_MoveSpin, MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode30 = { 0x3, FieldActorMovecode_InitSwing, FieldActorMovecode_MoveSwing,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode14 = { 0x3, FieldActorMovecode_InitBackForth, FieldActorMovecode_MoveBackForth,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode15 = { 0x3, FieldActorMovecode_InitRoutePatrolSet14,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode16 = { 0x3, FieldActorMovecode_InitRoutePatrolSet15,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode17 = { 0x3, FieldActorMovecode_InitRoutePatrolSet16,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode18 = { 0x3, FieldActorMovecode_InitRoutePatrolSet17,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode19 = { 0x3, FieldActorMovecode_InitRoutePatrolSet18,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1A = { 0x3, FieldActorMovecode_InitRoutePatrolSet19,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1B = { 0x3, FieldActorMovecode_InitRoutePatrolSet20,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1C = { 0x3, FieldActorMovecode_InitRoutePatrolSet21,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1D = { 0x3, FieldActorMovecode_InitRoutePatrolSet22,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1E = { 0x3, FieldActorMovecode_InitRoutePatrolSet23,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1F = { 0x3, FieldActorMovecode_InitRoutePatrolSet24,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode20 = { 0x3, FieldActorMovecode_InitRoutePatrolSet25,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode21 = { 0x3, FieldActorMovecode_InitRoutePatrolSet26,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode22 = { 0x3, FieldActorMovecode_InitRoutePatrolSet27,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode23 = { 0x3, FieldActorMovecode_InitRoutePatrolSet28,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode24 = { 0x3, FieldActorMovecode_InitRoutePatrolSet29,
                                                FieldActorMovecode_MoveRoutePatrol, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode25 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet30,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode26 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet31,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode27 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet32,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode28 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet33,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode29 = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet34,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2A = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet35,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2B = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet36,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2C = { 0x3, FieldActorMovecode_InitRoutePatrolSwapSet37,
                                                FieldActorMovecode_MoveRoutePatrolSwap, MoveCodeNone_Delete,
                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode2F = { 0x3, FieldActorMove_InitLookPlayer, FieldActorMove_MoveLookPlayer,
                                                FieldActorMove_DeleteLookPlayer, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode31 = { 0x3, FieldActorMove_InitLookFellow, FieldActorMove_MoveLookFellow,
                                                FieldActorMove_DeleteLookFellow, FieldActorMove_ClearFellowTracking };

static const FieldActorMoveCode sMoveCode32 = { 0x33, FieldActorMove_InitHideKind0Code33, FieldActorMove_MoveHide,
                                                FieldActorMove_DeleteHide, FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCode33 = { 0x34, FieldActorMove_InitHideKind1Code34, FieldActorMove_MoveHide,
                                                FieldActorMove_DeleteHide, FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCode34 = { 0x35, FieldActorMove_InitHideKind1Code35, FieldActorMove_MoveHide,
                                                FieldActorMove_DeleteHide, FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCode35 = { 0x36, FieldActorMove_InitHideKind0Code36, FieldActorMove_MoveHide,
                                                FieldActorMove_DeleteHide, FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCode36 = { 0x37, FieldActorMove_InitFaceDirSet0, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode37 = { 0x38, FieldActorMove_InitFaceDirSet1, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode38 = { 0x39, FieldActorMove_InitFaceDirSet2, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode39 = { 0x3a, FieldActorMove_InitFaceDirSet3, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3A = { 0x3b, FieldActorMove_InitFaceDirFlagSet0, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3B = { 0x3c, FieldActorMove_InitFaceDirFlagSet1, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3C = { 0x3d, FieldActorMove_InitFaceDirFlagSet2, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3D = { 0x3e, FieldActorMove_InitFaceDirFlagSet3, FieldActorMove_MoveFaceDir,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3E = { 0x3f, FieldActorMove_InitWallWalkLeftHand, FieldActorMove_MoveWallWalk,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode3F = { 0x40, FieldActorMove_InitWallWalkRightHand, FieldActorMove_MoveWallWalk,
                                                MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode40 = { 0x41, FieldActorMove_InitWallWalkLeftHandReversing,
                                                FieldActorMove_MoveWallWalk, MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode41 = { 0x42, FieldActorMove_InitWallWalkRightHandReversing,
                                                FieldActorMove_MoveWallWalk, MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode43 = { 0x44, FieldCommActor_MoveInit, FieldCommActor_Move,
                                                FieldCommActor_MoveDelete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode44 = { 0x45, RailActor_StayInit, RailActor_StayMove, RailActor_StayDelete,
                                                RailActor_Recover };

static const FieldActorMoveCode sMoveCode45 = { 0x46, RailActor_WaitInitAll, RailActor_WaitMove, RailActor_WaitDelete,
                                                RailActor_Recover };

static const FieldActorMoveCode sMoveCode46 = { 0x47, RailActor_WaitInitUpLeft, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode47 = { 0x48, RailActor_WaitInitUpRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode48 = { 0x49, RailActor_WaitInitDownLeft, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode49 = { 0x4a, RailActor_WaitInitDownRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4A = { 0x4b, RailActor_WaitInitUpDownLeft, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4B = { 0x4c, RailActor_WaitInitUpDownRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4C = { 0x4d, RailActor_WaitInitUpLeftRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4D = { 0x4e, RailActor_WaitInitDownLeftRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4E = { 0x4f, RailActor_WaitInitUpDown, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode4F = { 0x50, RailActor_WaitInitLeftRight, RailActor_WaitMove,
                                                RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode50 = { 0x51, RailActor_WalkInitAll, RailActor_WalkMove, RailActor_WalkDelete,
                                                RailActor_Recover };

static const FieldActorMoveCode sMoveCode51 = { 0x52, RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode52 = { 0x53, RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode53 = { 0x54, RailActor_PatrolInit, RailActor_PatrolMove,
                                                RailActor_PatrolDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCode54 = { 0x55, FieldActorMove_InitHideKind2Code55, FieldActorMove_MoveHide,
                                                FieldActorMove_DeleteHide, FieldActorMove_RecoverHide };

const FieldActorMoveCode *const ACTOR_MOVE_CODE_FUNCTION_TABLES[] = {
    &sMoveCode00, &sMoveCode01, &sMoveCode02, &sMoveCode03, &sMoveCode04, &sMoveCode05, &sMoveCode06, &sMoveCode07,
    &sMoveCode08, &sMoveCode09, &sMoveCode0A, &sMoveCode0B, &sMoveCode0C, &sMoveCode0D, &sMoveCode0E, &sMoveCode0F,
    &sMoveCode10, &sMoveCode11, &sMoveCode12, &sMoveCode13, &sMoveCode14, &sMoveCode15, &sMoveCode16, &sMoveCode17,
    &sMoveCode18, &sMoveCode19, &sMoveCode1A, &sMoveCode1B, &sMoveCode1C, &sMoveCode1D, &sMoveCode1E, &sMoveCode1F,
    &sMoveCode20, &sMoveCode21, &sMoveCode22, &sMoveCode23, &sMoveCode24, &sMoveCode25, &sMoveCode26, &sMoveCode27,
    &sMoveCode28, &sMoveCode29, &sMoveCode2A, &sMoveCode2B, &sMoveCode2C, &sMoveCode2D, &sMoveCode2E, NULL,
    &sMoveCode2F, &sMoveCode30, &sMoveCode31, &sMoveCode32, &sMoveCode33, &sMoveCode34, &sMoveCode35, &sMoveCode36,
    &sMoveCode37, &sMoveCode38, &sMoveCode39, &sMoveCode3A, &sMoveCode3B, &sMoveCode3C, &sMoveCode3D, &sMoveCode3E,
    &sMoveCode3F, &sMoveCode40, &sMoveCode41, &sMoveCode42, &sMoveCode43, &sMoveCode44, &sMoveCode45, &sMoveCode46,
    &sMoveCode47, &sMoveCode48, &sMoveCode49, &sMoveCode4A, &sMoveCode4B, &sMoveCode4C, &sMoveCode4D, &sMoveCode4E,
    &sMoveCode4F, &sMoveCode50, &sMoveCode51, &sMoveCode52, &sMoveCode53, &sMoveCode54,
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
// the same on the rails, and RAIL_ACMD_NEXT_STATE_TABLE, which only moves on to the next state, for the others
const ActorAcmdFunc *const ACTOR_ACTION_COMMAND_TABLES_RAIL[] = {
    RAIL_ACMD_FACE_UP_TABLE,
    RAIL_ACMD_FACE_DOWN_TABLE,
    RAIL_ACMD_FACE_LEFT_TABLE,
    RAIL_ACMD_FACE_RIGHT_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_WALK_UP_16F_TABLE,
    RAIL_ACMD_WALK_DOWN_16F_TABLE,
    RAIL_ACMD_WALK_LEFT_16F_TABLE,
    RAIL_ACMD_WALK_RIGHT_16F_TABLE,
    RAIL_ACMD_WALK_UP_8F_TABLE,
    RAIL_ACMD_WALK_DOWN_8F_TABLE,
    RAIL_ACMD_WALK_LEFT_8F_TABLE,
    RAIL_ACMD_WALK_RIGHT_8F_TABLE,
    RAIL_ACMD_WALK_UP_4F_TABLE,
    RAIL_ACMD_WALK_DOWN_4F_TABLE,
    RAIL_ACMD_WALK_LEFT_4F_TABLE,
    RAIL_ACMD_WALK_RIGHT_4F_TABLE,
    RAIL_ACMD_WALK_UP_2F_TABLE,
    RAIL_ACMD_WALK_DOWN_2F_TABLE,
    RAIL_ACMD_WALK_LEFT_2F_TABLE,
    RAIL_ACMD_WALK_RIGHT_2F_TABLE,
    RAIL_ACMD_STAY_WALK_UP_32F_TABLE,
    RAIL_ACMD_STAY_WALK_DOWN_32F_TABLE,
    RAIL_ACMD_STAY_WALK_LEFT_32F_TABLE,
    RAIL_ACMD_STAY_WALK_RIGHT_32F_TABLE,
    RAIL_ACMD_STAY_WALK_UP_16F_TABLE,
    RAIL_ACMD_STAY_WALK_DOWN_16F_TABLE,
    RAIL_ACMD_STAY_WALK_LEFT_16F_TABLE,
    RAIL_ACMD_STAY_WALK_RIGHT_16F_TABLE,
    RAIL_ACMD_STAY_WALK_UP_8F_TABLE,
    RAIL_ACMD_STAY_WALK_DOWN_8F_TABLE,
    RAIL_ACMD_STAY_WALK_LEFT_8F_TABLE,
    RAIL_ACMD_STAY_WALK_RIGHT_8F_TABLE,
    RAIL_ACMD_STAY_WALK_UP_4F_TABLE,
    RAIL_ACMD_STAY_WALK_DOWN_4F_TABLE,
    RAIL_ACMD_STAY_WALK_LEFT_4F_TABLE,
    RAIL_ACMD_STAY_WALK_RIGHT_4F_TABLE,
    RAIL_ACMD_STAY_WALK_UP_2F_TABLE,
    RAIL_ACMD_STAY_WALK_DOWN_2F_TABLE,
    RAIL_ACMD_STAY_WALK_LEFT_2F_TABLE,
    RAIL_ACMD_STAY_WALK_RIGHT_2F_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_JUMP_UP_1G_8F_TABLE,
    RAIL_ACMD_JUMP_DOWN_1G_8F_TABLE,
    RAIL_ACMD_JUMP_LEFT_1G_8F_TABLE,
    RAIL_ACMD_JUMP_RIGHT_1G_8F_TABLE,
    RAIL_ACMD_JUMP_UP_2G_16F_TABLE,
    RAIL_ACMD_JUMP_DOWN_2G_16F_TABLE,
    RAIL_ACMD_JUMP_LEFT_2G_16F_TABLE,
    RAIL_ACMD_JUMP_RIGHT_2G_16F_TABLE,
    ACMD_WAIT1_F_TABLE,
    ACMD_WAIT2_F_TABLE,
    ACMD_WAIT4_F_TABLE,
    ACMD_WAIT8_F_TABLE,
    ACMD_WAIT15_F_TABLE,
    ACMD_WAIT16_F_TABLE,
    ACMD_WAIT32_F_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    ACMD_SET_HIDDEN_FLAG_TABLE,
    ACMD_CLEAR_HIDDEN_FLAG_TABLE,
    ACMD_SET_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_CLEAR_FACE_DIR_LOCK_FLAG_TABLE,
    ACMD_SET_ANIME_PAUSE_FLAG_TABLE,
    ACMD_CLEAR_ANIME_PAUSE_FLAG_TABLE,
    ACMD_MARK_WAIT_EXCL_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_DASH_UP_4F_TABLE,
    RAIL_ACMD_DASH_DOWN_4F_TABLE,
    RAIL_ACMD_DASH_LEFT_4F_TABLE,
    RAIL_ACMD_DASH_RIGHT_4F_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    ACMD_STAY_WALK_BOB_INIT_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    ACMD_STAY_IDLE8_F_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    ACMD_MARK_WAIT_EXCL_NO_SE_TABLE,
    ACMD_MARK_WAIT_NOTE_NO_SE_TABLE,
    RAIL_ACMD_NEXT_STATE_TABLE,
    NULL,
};

// The commands of each direction (up, down, left and right) of a command that differs only by direction, by command,
// except that 0x75 comes before 0x69, 0x91 and 0x95 before 0x79, and 0xa7 before 0xa3
static const u32 sAcmdDirRow00[] = { 0, 0x1, 0x2, 0x3 };

static const u32 sAcmdDirRow04[] = { 0x4, 0x5, 0x6, 0x7 };

static const u32 sAcmdDirRow08[] = { 0x8, 0x9, 0xa, 0xb };

static const u32 sAcmdDirRow0C[] = { 0xc, 0xd, 0xe, 0xf };

static const u32 sAcmdDirRow10[] = { 0x10, 0x11, 0x12, 0x13 };

static const u32 sAcmdDirRow14[] = { 0x14, 0x15, 0x16, 0x17 };

static const u32 sAcmdDirRow18[] = { 0x18, 0x19, 0x1a, 0x1b };

static const u32 sAcmdDirRow1C[] = { 0x1c, 0x1d, 0x1e, 0x1f };

static const u32 sAcmdDirRow20[] = { 0x20, 0x21, 0x22, 0x23 };

static const u32 sAcmdDirRow24[] = { 0x24, 0x25, 0x26, 0x27 };

static const u32 sAcmdDirRow28[] = { 0x28, 0x29, 0x2a, 0x2b };

static const u32 sAcmdDirRow2C[] = { 0x2c, 0x2d, 0x2e, 0x2f };

static const u32 sAcmdDirRow30[] = { 0x30, 0x31, 0x32, 0x33 };

static const u32 sAcmdDirRow34[] = { 0x34, 0x35, 0x36, 0x37 };

static const u32 sAcmdDirRow38[] = { 0x38, 0x39, 0x3a, 0x3b };

static const u32 sAcmdDirRow4C[] = { 0x4c, 0x4d, 0x4e, 0x4f };

static const u32 sAcmdDirRow50[] = { 0x50, 0x51, 0x52, 0x53 };

static const u32 sAcmdDirRow54[] = { 0x54, 0x55, 0x56, 0x57 };

static const u32 sAcmdDirRow58[] = { 0x58, 0x59, 0x5a, 0x5b };

static const u32 sAcmdDirRow5C[] = { 0x5c, 0x5d, 0x5c, 0x5d };

static const u32 sAcmdDirRow5E[] = { 0x5e, 0x5f, 0x5e, 0x5f };

static const u32 sAcmdDirRow60[] = { 0x60, 0x61, 0x62, 0x63 };

static const u32 sAcmdDirRow75[] = { 0x75, 0x76, 0x77, 0x78 };

static const u32 sAcmdDirRow69[] = { 0x69, 0x6a, 0x6b, 0x6c };

static const u32 sAcmdDirRow6D[] = { 0x6d, 0x6e, 0x6f, 0x70 };

static const u32 sAcmdDirRow71[] = { 0x71, 0x72, 0x73, 0x74 };

static const u32 sAcmdDirRow91[] = { 0x91, 0x92, 0x93, 0x94 };

static const u32 sAcmdDirRow95[] = { 0x95, 0x96, 0x97, 0x98 };

static const u32 sAcmdDirRow79[] = { 0x79, 0x7a, 0x7b, 0x7c };

static const u32 sAcmdDirRow7D[] = { 0x7d, 0x7e, 0x7f, 0x80 };

static const u32 sAcmdDirRow81[] = { 0x81, 0x82, 0x83, 0x84 };

static const u32 sAcmdDirRow85[] = { 0x85, 0x86, 0x87, 0x88 };

static const u32 sAcmdDirRow89[] = { 0x89, 0x8a, 0x8b, 0x8c };

static const u32 sAcmdDirRow8D[] = { 0x8d, 0x8e, 0x8f, 0x90 };

static const u32 sAcmdDirRow9B[] = { 0x9b, 0x9c, 0x9d, 0x9e };

static const u32 sAcmdDirRowA7[] = { 0xa7, 0xa8, 0xa9, 0xaa };

static const u32 sAcmdDirRowA3[] = { 0xa3, 0xa4, 0xa5, 0xa6 };

const u32 *const ACMD_DIR_LUT[] = {
    sAcmdDirRow00, sAcmdDirRow0C, sAcmdDirRow58, sAcmdDirRow14, sAcmdDirRow10, sAcmdDirRowA7, sAcmdDirRowA3,
    sAcmdDirRow04, sAcmdDirRow08, sAcmdDirRow18, sAcmdDirRow1C, sAcmdDirRow20, sAcmdDirRow24, sAcmdDirRow28,
    sAcmdDirRow2C, sAcmdDirRow30, sAcmdDirRow34, sAcmdDirRow38, sAcmdDirRow4C, sAcmdDirRow50, sAcmdDirRow54,
    sAcmdDirRow5C, sAcmdDirRow5E, sAcmdDirRow60, sAcmdDirRow69, sAcmdDirRow6D, sAcmdDirRow75, sAcmdDirRow71,
    sAcmdDirRow79, sAcmdDirRow7D, sAcmdDirRow81, sAcmdDirRow85, sAcmdDirRow89, sAcmdDirRow8D, sAcmdDirRow91,
    sAcmdDirRow95, sAcmdDirRow9B, NULL,
};
