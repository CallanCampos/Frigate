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

// The functions of each move code (constants/move_codes.h), by move code, except that MOVE_CODE_TURN_RANDOM_UP_DOWN
// and MOVE_CODE_TURN_RANDOM_LEFT_RIGHT follow MOVE_CODE_TURN_RANDOM_DOWN_LEFT_RIGHT, MOVE_CODE_SWING follows
// MOVE_CODE_SPIN_CLOCKWISE and MOVE_CODE_WALK_RANDOM_LEFT_RIGHT_BUMP follows MOVE_CODE_WALK_RANDOM_LEFT_RIGHT, the
// codes whose functions come just before theirs. The records from MOVE_CODE_WALK_RANDOM to
// MOVE_CODE_FOLLOW_PAIRED_TRAINER, and MOVE_CODE_WALK_RANDOM_LEFT_RIGHT_BUMP's, all give MOVE_CODE_WALK_RANDOM as their
// code
static const FieldActorMoveCode sMoveCodeNone = { MOVE_CODE_NONE, MoveCodeNone_Init, MoveCodeNone_Move,
                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCode1 = { MOVE_CODE_1, MoveCodeNone_Init, MoveCodeNone_Move, MoveCodeNone_Delete,
                                               MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandom = { MOVE_CODE_TURN_RANDOM, FieldActorMovecode_InitTurnRandomSet0,
                                                        FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWalkRandom = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitWalkRandomSet11,
                                                        FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                        MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWalkRandomUpDown = { MOVE_CODE_WALK_RANDOM,
                                                              FieldActorMovecode_InitWalkRandomSet12,
                                                              FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                              MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWalkRandomLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitWalkRandomSet13,
                                                                 FieldActorMovecode_MoveWalkRandom, MoveCodeNone_Delete,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWalkRandomLeftRightBump = { MOVE_CODE_WALK_RANDOM,
                                                                     FieldActorMovecode_InitWalkRandomSet13Bump,
                                                                     FieldActorMovecode_MoveWalkRandom,
                                                                     MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpLeft = { MOVE_CODE_WALK_RANDOM,
                                                              FieldActorMovecode_InitTurnRandomSet1,
                                                              FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                              MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpRight = { MOVE_CODE_WALK_RANDOM,
                                                               FieldActorMovecode_InitTurnRandomSet2,
                                                               FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                               MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomDownLeft = { MOVE_CODE_WALK_RANDOM,
                                                                FieldActorMovecode_InitTurnRandomSet3,
                                                                FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                                MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomDownRight = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitTurnRandomSet4,
                                                                 FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpDownLeft = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitTurnRandomSet5,
                                                                  FieldActorMovecode_MoveTurnRandom,
                                                                  func_ov036_021913dc, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpDownRight = { MOVE_CODE_WALK_RANDOM,
                                                                   FieldActorMovecode_InitTurnRandomSet6,
                                                                   FieldActorMovecode_MoveTurnRandom,
                                                                   func_ov036_021913dc, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                   FieldActorMovecode_InitTurnRandomSet7,
                                                                   FieldActorMovecode_MoveTurnRandom,
                                                                   func_ov036_021913dc, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomDownLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                     FieldActorMovecode_InitTurnRandomSet8,
                                                                     FieldActorMovecode_MoveTurnRandom,
                                                                     func_ov036_021913dc, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomUpDown = { MOVE_CODE_WALK_RANDOM,
                                                              FieldActorMovecode_InitTurnRandomSet9,
                                                              FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                              MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnRandomLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitTurnRandomSet10,
                                                                 FieldActorMovecode_MoveTurnRandom, func_ov036_021913dc,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFaceUp = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitFixedDirUp,
                                                    FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                    MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFaceDown = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitFixedDirDown,
                                                      FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                      MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFaceLeft = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitFixedDirLeft,
                                                      FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                      MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFaceRight = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitFixedDirRight,
                                                       FieldActorMovecode_MoveFixedDir, MoveCodeNone_Delete,
                                                       MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeSpinCounterclockwise = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitSpinLeft,
                                                                  FieldActorMovecode_MoveSpin, MoveCodeNone_Delete,
                                                                  MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeSpinClockwise = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitSpinRight,
                                                           FieldActorMovecode_MoveSpin, MoveCodeNone_Delete,
                                                           MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeSwing = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitSwing,
                                                   FieldActorMovecode_MoveSwing, MoveCodeNone_Delete,
                                                   MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWalkBackAndForth = { MOVE_CODE_WALK_RANDOM, FieldActorMovecode_InitBackForth,
                                                              FieldActorMovecode_MoveBackForth, MoveCodeNone_Delete,
                                                              MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteUpRightLeftDown = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet14,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteRightLeftDownUp = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet15,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteDownUpRightLeft = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet16,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteLeftDownUpRight = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet17,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteLeftRightDownUp = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet18,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteLeftRightDownUp2 = { MOVE_CODE_WALK_RANDOM,
                                                                   FieldActorMovecode_InitRoutePatrolSet19,
                                                                   FieldActorMovecode_MoveRoutePatrol,
                                                                   MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteDownUpLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet20,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteRightDownUpLeft = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet21,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteLeftUpDownRight = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet22,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteUpDownRightLeft = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet23,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteRightLeftUpDown = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet24,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteDownRightLeftUp = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet25,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteRightUpDownLeft = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet26,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteUpDownLeftRight = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet27,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteLeftRightUpDown = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet28,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRouteDownLeftRightUp = { MOVE_CODE_WALK_RANDOM,
                                                                  FieldActorMovecode_InitRoutePatrolSet29,
                                                                  FieldActorMovecode_MoveRoutePatrol,
                                                                  MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopUpLeftDownRight = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet30,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopDownRightUpLeft = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet31,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopLeftDownRightUp = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet32,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopRightUpLeftDown = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet33,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopUpRightDownLeft = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet34,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopDownLeftUpRight = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet35,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopLeftUpRightDown = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet36,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeLoopRightDownLeftUp = { MOVE_CODE_WALK_RANDOM,
                                                                 FieldActorMovecode_InitRoutePatrolSwapSet37,
                                                                 FieldActorMovecode_MoveRoutePatrolSwap,
                                                                 MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFollowPlayer = { MOVE_CODE_WALK_RANDOM, FieldActorMove_InitLookPlayer,
                                                          FieldActorMove_MoveLookPlayer,
                                                          FieldActorMove_DeleteLookPlayer, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeFollowPairedTrainer = { MOVE_CODE_WALK_RANDOM, FieldActorMove_InitLookFellow,
                                                                 FieldActorMove_MoveLookFellow,
                                                                 FieldActorMove_DeleteLookFellow,
                                                                 FieldActorMove_ClearFellowTracking };

static const FieldActorMoveCode sMoveCodeHide33 = { MOVE_CODE_HIDE_33, FieldActorMove_InitHideKind0Code33,
                                                    FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                    FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCodeHide34 = { MOVE_CODE_HIDE_34, FieldActorMove_InitHideKind1Code34,
                                                    FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                    FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCodeHide35 = { MOVE_CODE_HIDE_35, FieldActorMove_InitHideKind1Code35,
                                                    FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                    FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCodeHide36 = { MOVE_CODE_HIDE_36, FieldActorMove_InitHideKind0Code36,
                                                    FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                    FieldActorMove_RecoverHide };

static const FieldActorMoveCode sMoveCodeTurnWithPlayer0 = { MOVE_CODE_TURN_WITH_PLAYER_0,
                                                             FieldActorMove_InitFaceDirSet0, FieldActorMove_MoveFaceDir,
                                                             MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayer1 = { MOVE_CODE_TURN_WITH_PLAYER_1,
                                                             FieldActorMove_InitFaceDirSet1, FieldActorMove_MoveFaceDir,
                                                             MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayer2 = { MOVE_CODE_TURN_WITH_PLAYER_2,
                                                             FieldActorMove_InitFaceDirSet2, FieldActorMove_MoveFaceDir,
                                                             MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayer3 = { MOVE_CODE_TURN_WITH_PLAYER_3,
                                                             FieldActorMove_InitFaceDirSet3, FieldActorMove_MoveFaceDir,
                                                             MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayerFlag0 = { MOVE_CODE_TURN_WITH_PLAYER_FLAG_0,
                                                                 FieldActorMove_InitFaceDirFlagSet0,
                                                                 FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayerFlag1 = { MOVE_CODE_TURN_WITH_PLAYER_FLAG_1,
                                                                 FieldActorMove_InitFaceDirFlagSet1,
                                                                 FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayerFlag2 = { MOVE_CODE_TURN_WITH_PLAYER_FLAG_2,
                                                                 FieldActorMove_InitFaceDirFlagSet2,
                                                                 FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeTurnWithPlayerFlag3 = { MOVE_CODE_TURN_WITH_PLAYER_FLAG_3,
                                                                 FieldActorMove_InitFaceDirFlagSet3,
                                                                 FieldActorMove_MoveFaceDir, MoveCodeNone_Delete,
                                                                 MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWallWalkLeftHand = { MOVE_CODE_WALL_WALK_LEFT_HAND,
                                                              FieldActorMove_InitWallWalkLeftHand,
                                                              FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                              MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWallWalkRightHand = { MOVE_CODE_WALL_WALK_RIGHT_HAND,
                                                               FieldActorMove_InitWallWalkRightHand,
                                                               FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                               MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWallWalkLeftHandReversing = { MOVE_CODE_WALL_WALK_LEFT_HAND_REVERSING,
                                                                       FieldActorMove_InitWallWalkLeftHandReversing,
                                                                       FieldActorMove_MoveWallWalk, MoveCodeNone_Delete,
                                                                       MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeWallWalkRightHandReversing = { MOVE_CODE_WALL_WALK_RIGHT_HAND_REVERSING,
                                                                        FieldActorMove_InitWallWalkRightHandReversing,
                                                                        FieldActorMove_MoveWallWalk,
                                                                        MoveCodeNone_Delete, MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeCommActor = { MOVE_CODE_COMM_ACTOR, FieldCommActor_MoveInit,
                                                       FieldCommActor_Move, FieldCommActor_MoveDelete,
                                                       MoveCodeNone_Return };

static const FieldActorMoveCode sMoveCodeRailStay = { MOVE_CODE_RAIL_STAY, RailActor_StayInit, RailActor_StayMove,
                                                      RailActor_StayDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandom = { MOVE_CODE_RAIL_TURN_RANDOM, RailActor_WaitInitAll,
                                                            RailActor_WaitMove, RailActor_WaitDelete,
                                                            RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpLeft = { MOVE_CODE_RAIL_TURN_RANDOM_UP_LEFT,
                                                                  RailActor_WaitInitUpLeft, RailActor_WaitMove,
                                                                  RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpRight = { MOVE_CODE_RAIL_TURN_RANDOM_UP_RIGHT,
                                                                   RailActor_WaitInitUpRight, RailActor_WaitMove,
                                                                   RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomDownLeft = { MOVE_CODE_RAIL_TURN_RANDOM_DOWN_LEFT,
                                                                    RailActor_WaitInitDownLeft, RailActor_WaitMove,
                                                                    RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomDownRight = { MOVE_CODE_RAIL_TURN_RANDOM_DOWN_RIGHT,
                                                                     RailActor_WaitInitDownRight, RailActor_WaitMove,
                                                                     RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpDownLeft = { MOVE_CODE_RAIL_TURN_RANDOM_UP_DOWN_LEFT,
                                                                      RailActor_WaitInitUpDownLeft, RailActor_WaitMove,
                                                                      RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpDownRight = { MOVE_CODE_RAIL_TURN_RANDOM_UP_DOWN_RIGHT,
                                                                       RailActor_WaitInitUpDownRight,
                                                                       RailActor_WaitMove, RailActor_WaitDelete,
                                                                       RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpLeftRight = { MOVE_CODE_RAIL_TURN_RANDOM_UP_LEFT_RIGHT,
                                                                       RailActor_WaitInitUpLeftRight,
                                                                       RailActor_WaitMove, RailActor_WaitDelete,
                                                                       RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomDownLeftRight = { MOVE_CODE_RAIL_TURN_RANDOM_DOWN_LEFT_RIGHT,
                                                                         RailActor_WaitInitDownLeftRight,
                                                                         RailActor_WaitMove, RailActor_WaitDelete,
                                                                         RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomUpDown = { MOVE_CODE_RAIL_TURN_RANDOM_UP_DOWN,
                                                                  RailActor_WaitInitUpDown, RailActor_WaitMove,
                                                                  RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailTurnRandomLeftRight = { MOVE_CODE_RAIL_TURN_RANDOM_LEFT_RIGHT,
                                                                     RailActor_WaitInitLeftRight, RailActor_WaitMove,
                                                                     RailActor_WaitDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailWalkRandom = { MOVE_CODE_RAIL_WALK_RANDOM, RailActor_WalkInitAll,
                                                            RailActor_WalkMove, RailActor_WalkDelete,
                                                            RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailWalkRandomUpDown = { MOVE_CODE_RAIL_WALK_RANDOM_UP_DOWN,
                                                                  RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                                  RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailWalkRandomUpDown2 = { MOVE_CODE_RAIL_WALK_RANDOM_UP_DOWN_2,
                                                                   RailActor_WalkInitUpDown, RailActor_WalkMove,
                                                                   RailActor_WalkDelete, RailActor_Recover };

static const FieldActorMoveCode sMoveCodeRailPatrol = { MOVE_CODE_RAIL_PATROL, RailActor_PatrolInit,
                                                        RailActor_PatrolMove, RailActor_PatrolDelete,
                                                        RailActor_Recover };

static const FieldActorMoveCode sMoveCodeHideLeaves = { MOVE_CODE_HIDE_LEAVES, FieldActorMove_InitHideKind2Code55,
                                                        FieldActorMove_MoveHide, FieldActorMove_DeleteHide,
                                                        FieldActorMove_RecoverHide };

const FieldActorMoveCode *const ACTOR_MOVE_CODE_FUNCTION_TABLES[MOVE_CODE_MAX] = {
    &sMoveCodeNone,
    &sMoveCode1,
    &sMoveCodeTurnRandom,
    &sMoveCodeWalkRandom,
    &sMoveCodeWalkRandomUpDown,
    &sMoveCodeWalkRandomLeftRight,
    &sMoveCodeTurnRandomUpLeft,
    &sMoveCodeTurnRandomUpRight,
    &sMoveCodeTurnRandomDownLeft,
    &sMoveCodeTurnRandomDownRight,
    &sMoveCodeTurnRandomUpDownLeft,
    &sMoveCodeTurnRandomUpDownRight,
    &sMoveCodeTurnRandomUpLeftRight,
    &sMoveCodeTurnRandomDownLeftRight,
    &sMoveCodeFaceUp,
    &sMoveCodeFaceDown,
    &sMoveCodeFaceLeft,
    &sMoveCodeFaceRight,
    &sMoveCodeSpinCounterclockwise,
    &sMoveCodeSpinClockwise,
    &sMoveCodeWalkBackAndForth,
    &sMoveCodeRouteUpRightLeftDown,
    &sMoveCodeRouteRightLeftDownUp,
    &sMoveCodeRouteDownUpRightLeft,
    &sMoveCodeRouteLeftDownUpRight,
    &sMoveCodeRouteLeftRightDownUp,
    &sMoveCodeRouteLeftRightDownUp2,
    &sMoveCodeRouteDownUpLeftRight,
    &sMoveCodeRouteRightDownUpLeft,
    &sMoveCodeRouteLeftUpDownRight,
    &sMoveCodeRouteUpDownRightLeft,
    &sMoveCodeRouteRightLeftUpDown,
    &sMoveCodeRouteDownRightLeftUp,
    &sMoveCodeRouteRightUpDownLeft,
    &sMoveCodeRouteUpDownLeftRight,
    &sMoveCodeRouteLeftRightUpDown,
    &sMoveCodeRouteDownLeftRightUp,
    &sMoveCodeLoopUpLeftDownRight,
    &sMoveCodeLoopDownRightUpLeft,
    &sMoveCodeLoopLeftDownRightUp,
    &sMoveCodeLoopRightUpLeftDown,
    &sMoveCodeLoopUpRightDownLeft,
    &sMoveCodeLoopDownLeftUpRight,
    &sMoveCodeLoopLeftUpRightDown,
    &sMoveCodeLoopRightDownLeftUp,
    &sMoveCodeTurnRandomUpDown,
    &sMoveCodeTurnRandomLeftRight,
    NULL,
    &sMoveCodeFollowPlayer,
    &sMoveCodeSwing,
    &sMoveCodeFollowPairedTrainer,
    &sMoveCodeHide33,
    &sMoveCodeHide34,
    &sMoveCodeHide35,
    &sMoveCodeHide36,
    &sMoveCodeTurnWithPlayer0,
    &sMoveCodeTurnWithPlayer1,
    &sMoveCodeTurnWithPlayer2,
    &sMoveCodeTurnWithPlayer3,
    &sMoveCodeTurnWithPlayerFlag0,
    &sMoveCodeTurnWithPlayerFlag1,
    &sMoveCodeTurnWithPlayerFlag2,
    &sMoveCodeTurnWithPlayerFlag3,
    &sMoveCodeWallWalkLeftHand,
    &sMoveCodeWallWalkRightHand,
    &sMoveCodeWallWalkLeftHandReversing,
    &sMoveCodeWallWalkRightHandReversing,
    &sMoveCodeWalkRandomLeftRightBump,
    &sMoveCodeCommActor,
    &sMoveCodeRailStay,
    &sMoveCodeRailTurnRandom,
    &sMoveCodeRailTurnRandomUpLeft,
    &sMoveCodeRailTurnRandomUpRight,
    &sMoveCodeRailTurnRandomDownLeft,
    &sMoveCodeRailTurnRandomDownRight,
    &sMoveCodeRailTurnRandomUpDownLeft,
    &sMoveCodeRailTurnRandomUpDownRight,
    &sMoveCodeRailTurnRandomUpLeftRight,
    &sMoveCodeRailTurnRandomDownLeftRight,
    &sMoveCodeRailTurnRandomUpDown,
    &sMoveCodeRailTurnRandomLeftRight,
    &sMoveCodeRailWalkRandom,
    &sMoveCodeRailWalkRandomUpDown,
    &sMoveCodeRailWalkRandomUpDown2,
    &sMoveCodeRailPatrol,
    &sMoveCodeHideLeaves,
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
