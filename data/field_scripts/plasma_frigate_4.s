#include "asm/field_script.inc"
#include "text/script/plasma_frigate_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_2:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x40f3
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ad
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0069
    ActorSetGPos 0, 7, 0, 9, 1
    VMJump L_0098

L_0069:
    VMStackPush EVENT_WORK_0x40f3
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01ad
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0098
    ActorSetGPos 0, 7, 2, 5, 1

L_0098:
    VMHalt

Script_4:
    VMHalt

Script_5:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x78000, 0, 0x2b000, 80
    ActorCmdExec 255, Movement_04C8
    ActorCmdWait
    EvCameraWait
    BGMPlay SEQ_BGM_E_ACHROMA
    ActorCmdExec 0, Movement_04D0
    ActorCmdWait
    // "Colress: Welcome![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ColressWelcome, 0, 0, 0
    // "I was asked by an acquaintance\nto help with his research.[f000]븁\u0000\nWhat I desire is to bring out\nthe entirety of Pokémon potential![f000]븁\u0000\nIf I can accomplish that,\nI don't care what it takes![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_AskedByAcquaintanceHelp, 0, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 0, 7, 7, 1, 8, 1
    EvCameraMoveTo 9688, 0, 0xed000, 0x78000, 0x2001f, 0x6a000, 24
    EvCameraWait
    ActorCmdWait
    // "If it means this strength must be\nbrought out by the interactions[f000]븀\u0000\nbetween Pokémon and Trainers,[f000]븀\u0000\nthen so be it![f000]븁\u0000\nIf it means you have to use a merciless\napproach, like Team Plasma's,[f000]븀\u0000\nand force out all of the Pokémon's power,[f000]븀\u0000\nthen so be it![f000]븁\u0000\nAnd yes, if the entire world\nis destroyed as a result,[f000]븀\u0000\nthen so be it...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_IfMeansStrengthMust, 0, 0, 0
    MsgWinCloseAll
    EvCameraMoveTo 9688, 0, 0xed000, 0x78000, 0, 0x98000, 24
    ActorWalkRoute 0, 7, 9, 1, 8, 1
    ActorCmdWait
    EvCameraWait
    // "That aside![f000]븁\u0000\nThe reason I have been traveling all over\nUnova and battling many Pokémon Trainers[f000]븀\u0000\nis because I was testing the viability of[f000]븀\u0000\nthis approach to bringing out the full[f000]븀\u0000\nstrength of Pokémon.[f000]븁\u0000\nIn that respect, you've done\nan amazing job.[f000]븁\u0000\nWell now! Tell me if you have\nthe answer I desire or not![f000]븁\u0000\nIf you're ready, come at me!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_AsideReasonHaveBeen, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    EvCameraMoveToDefault 40
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    BGMChangeMap
    WorkSetConst EVENT_WORK_0x40f3, 2
    VMHalt
    .balign 4, 0

Movement_0174:
    Move 15, 1
    MoveEnd

Movement_017C:
    Move 14, 1
    MoveEnd

Movement_0184:
    Move 15, 1
    Move 33, 1
    MoveEnd

Movement_0190:
    Move 15, 1
    Move 32, 1
    MoveEnd

L_019C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It looks like you're ready, then!\nOK! Let us begin![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_LooksLikeYoureReady, 0, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_TEAM_PLASMA_COLRESS, 0, 0
    VMCall L_045B
    // "Colress: So strong![f000]븁\u0000\nYou're a very strong Trainer indeed!\nSo let me ask you this![f000]븁\u0000\nAre you thinking of reaching\neven higher heights by understanding[f000]븀\u0000\neach other as Pokémon and Trainer?"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ColressStrongYoureVery, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F3
    // "I see...[f000]븁\u0000\nTo me, that is an ideal answer.[f000]븁\u0000\nTo think! That's what you\nactually believe![f000]븁\u0000\nYou bring out the power of your\nPokémon with respect and love![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_SeeIdealAnswerThink, 0, 0, 0
    VMJump L_01FF

L_01F3:
    // "I see...[f000]븁\u0000\nIs that answer my truth?[f000]븁\u0000\nEven if that's your answer,\ndon't give up![f000]븁\u0000\nThe reason you're so strong\nis that you treat your Pokémon[f000]븀\u0000\nwith respect and love![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_SeeAnswerTruthEven, 0, 0, 0

L_01FF:
    MsgWinCloseAll
    PlayerGetDir 0x8021
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0226
    ActorCmdExec 0, Movement_0498
    VMJump L_022E

L_0226:
    ActorCmdExec 0, Movement_04A0

L_022E:
    ActorCmdWait
    // "Excuse me for repeating myself,\nbut if it will make Pokémon stronger,[f000]븀\u0000\nI don't care what it takes![f000]븁\u0000\nIf interacting with Pokémon can increase\ntheir powers only to a certain point,[f000]븀\u0000\nthen their full strength must be brought[f000]븀\u0000\nout using a scientific approach.[f000]븀\u0000\nEven one without a conscience.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ExcuseRepeatingMyselfBut, 0, 0, 0
    MsgWinCloseAll
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0251
    VMJump L_025F

L_0251:
    ActorCmdExec 0, Movement_017C
    VMJump L_02C2

L_025F:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_0272
    VMJump L_0280

L_0272:
    ActorCmdExec 0, Movement_0174
    VMJump L_02C2

L_0280:
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_0293
    VMJump L_02A1

L_0293:
    ActorCmdExec 0, Movement_0184
    VMJump L_02C2

L_02A1:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_02B4
    VMJump L_02C2

L_02B4:
    ActorCmdExec 0, Movement_0190
    VMJump L_02C2

L_02C2:
    ActorCmdWait
    // "But you have shown me the\npotential of your approach.[f000]븁\u0000\nTo me, whether Team Plasma wins\nor whether you win will decide[f000]븀\u0000\nhow the relationship between[f000]븀\u0000\npeople and Pokémon should be![f000]븁\u0000\nSo where will this be settled?[f000]븁\u0000\nStep on the warp panel on the other\nside of the room where Kyurem is[f000]븀\u0000\nbeing held.[f000]븁\u0000\nGood luck in your battle!"
    ActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ButHaveShownPotential, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40f3, 3
    WorkSetConst EVENT_WORK_0x40f4, 1
    WorkSetConst EVENT_WORK_0x4125, 1
    VMReturn

Script_6:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f3
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0319
    VMCall L_019C
    VMJump L_0418

L_0319:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40f3
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0356
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "But you have shown me the\npotential of your approach.[f000]븁\u0000\nTo me, whether Team Plasma wins\nor whether you win will decide[f000]븀\u0000\nhow the relationship between[f000]븀\u0000\npeople and Pokémon should be![f000]븁\u0000\nSo where will this be settled?[f000]븁\u0000\nStep on the warp panel on the other\nside of the room where Kyurem is[f000]븀\u0000\nbeing held.[f000]븁\u0000\nGood luck in your battle!"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ButHaveShownPotential, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0418

L_0356:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01ad
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040A
    // "Colress: Could I say that you're\ncurious for coming clear out here?[f000]븁\u0000\nI dissolved Team Plasma.[f000]븁\u0000\nThe ones who are left here are deciding\nwhat they want to do.[f000]븁\u0000\nI would like to have them decide\nhow to live on their own![f000]븁\u0000\nThere are far more questions in this\nworld that don't have any answers![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_ColressCouldSayYoure, 0, 0
    // "Well then...[f000]븁\u0000\nPlease show me you and your\nPokémon's strength once again![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_WellThenPleaseShow, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_0480
    ActorCmdWait
    CallTrainerBattle TRAINER_COLRESS_2, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B6
    CallTrainerBattleEnd
    VMJump L_03B8

L_03B6:
    CallTrainerLose

L_03B8:
    // "In Pokémon battles where you can't\nafford to make a mistake,[f000]븀\u0000\nthe true nature of the Trainer's[f000]븀\u0000\npersonality becomes clear.[f000]븁\u0000\nI see![f000]븁\u0000\nIf Trainers believe in their Pokémon\nto the fullest extreme, as you do,[f000]븀\u0000\ntheir Pokémon partners will also[f000]븀\u0000\ngive everything they have![f000]븁\u0000\nSo that must be the answer\nI've been looking for.[f000]븁\u0000\nStill, I'm glad you won.[f000]븁\u0000\nFor one thing, I detested Ghetsis\nfrom the start![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_PokemonBattlesWhereCant, 0, 0
    // "Take this with you![f000]븁\u0000\nThis Master Ball can catch\nany kind of Pokémon![f000]븁\u0000\nGhetsis was saving it,\nbut you'll probably put it to better use![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_TakeMasterBallCan, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 1
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "If you'd like, please have a\nPokémon battle with me again![f000]븁\u0000\nIndeed, I'm having trouble deciding\nwhat to do next as well."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_IfYoudLikePlease, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x01ad
    FlagSet EVENT_FLAG_DAILY_0x0ae9
    VMJump L_0418

L_040A:
    // "If you'd like, please have a\nPokémon battle with me again![f000]븁\u0000\nIndeed, I'm having trouble deciding\nwhat to do next as well."
    ParentActorMsg MSGFILE_SCRIPT, PlasmaFrigate4_Text_IfYoudLikePlease, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0418:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    GameGetVersion 0x8020
    PlayerGetDir 0x8021
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044B
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_10, 26, 10, 32801
    VMJump L_0455

L_044B:
    MapChangeWarpPad ZONE_PLASMA_FRIGATE_13, 26, 10, 32801

L_0455:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_045B:
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_047A
    CallTrainerBattleEnd
    VMJump L_047C

L_047A:
    CallTrainerLose

L_047C:
    VMReturn
    .balign 4, 0

Movement_0480:
    Move 182, 1
    MoveEnd
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd

Movement_0498:
    Move 15, 1
    MoveEnd

Movement_04A0:
    Move 14, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_04C8:
    Move 32, 1
    MoveEnd

Movement_04D0:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
    Move 159, 1
    MoveEnd
