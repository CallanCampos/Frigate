#include "asm/field_script.inc"
#include "text/script/driftveil_city_gym.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntry Script_21
    ScriptEntry Script_22
    ScriptEntry Script_23
    ScriptEntry Script_24
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_9:
    VMHalt

Script_10:
    TrainerCardHasBadge 0x8008, 4
    VMStackPush 0x8008
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AE
    ActorSetGPos 0, 9, 0, 84, 1
    VMStackPushFlag EVENT_FLAG_0x03e9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00AE
    ActorSetGPos 7, 11, 1, 72, 1

L_00AE:
    VMHalt

Script_2:
    ActorsPauseAll
    Cmd_0186
    FadeInBlack
    Cmd_018D 0
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 4
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E9
    // "Harrumph! Kept me waitin', didn't ya, kid?[f000]븁\u0000\nAll right, time to see what\nya can do![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HarrumphKeptWaitinDidnt, 7, 0, 0
    ActorMsgClose
    WorkSetConst 0x8021, 0
    GameGetDifficulty 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011C
    CallTrainerBattle TRAINER_LEADER_CLAY_2, 0, 0
    VMJump L_0124

L_011C:
    CallTrainerBattle TRAINER_LEADER_CLAY, 0, 0

L_0124:
    WorkSetConst 0x8021, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0149
    CallTrainerBattleEnd
    VMJump L_014B

L_0149:
    CallTrainerLose

L_014B:
    // "Phew...\nYou're really somethin'![f000]븁\u0000\nLi'l whippersnapper Trainers\nwho pack a real punch keep[f000]븀\u0000\nshowin' up one after another.[f000]븁\u0000\nMrmph.\nHere! Take this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_PhewYoureReallySomethin, 7, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 4
    TrainerCardAddBadge 4
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8022, 0
    TrainerCardGetSex 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018C
    PlayFieldEffect 7
    VMJump L_0190

L_018C:
    PlayFieldEffect 59

L_0190:
    MEWait
    WorkSetConst 0x8022, 0
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received the Quake Badge\nfrom Clay![f000]븁\u0000"
    SystemMsg DriftveilCityGym_Text_ReceivedQuakeBadgeFrom, 0
    InfoMsgClose
    // "So this is yer fifth Badge, huh?[f000]븁\u0000\nIf that's so, Pokémon up to Lv. 60\nwill obey ya.[f000]븁\u0000\nHere!\nTake this, too![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_YerFifthBadgeHuh, 7, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 405
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    TrainerFlagSet TRAINER_WORKER_MAYNARD
    TrainerFlagSet TRAINER_WORKER_PASQUAL
    TrainerFlagSet TRAINER_WORKER_TIBOR
    TrainerFlagSet TRAINER_WORKER_NIEL
    TrainerFlagSet TRAINER_WORKER_TAVARIUS
    TrainerFlagSet TRAINER_WORKER_NOEL
    TrainerFlagSet TRAINER_WORKER_FRIEDRICH
    FlagSet EVENT_FLAG_0x0972
    ActorCmdExec 7, Movement_073C
    ActorCmdWait
    VMSleep 30
    PlayerGetDir 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0216
    VMJump L_0224

L_0216:
    ActorCmdExec 7, Movement_072C
    VMJump L_0245

L_0224:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0237
    VMJump L_0245

L_0237:
    ActorCmdExec 7, Movement_0734
    VMJump L_0245

L_0245:
    ActorCmdWait
    // "Well, I suppose...\nCome thisaway![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_WellSupposeComeThisaway, 7, 0, 0
    ActorMsgClose
    ActorCmdExec 7, Movement_06A4
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0270
    VMJump L_028C

L_0270:
    ActorCmdExec 255, Movement_0690
    ActorCmdWait
    VMSleep 10
    ActorCmdExec 255, Movement_072C
    VMJump L_02B1

L_028C:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_029F
    VMJump L_02B1

L_029F:
    VMSleep 10
    ActorCmdExec 255, Movement_06A4
    VMJump L_02B1

L_02B1:
    ActorCmdWait
    FadeOutBlack
    Cmd_018D 2
    FadeWait
    SEPlay SEQ_SE_FLD_59
    VMSleep 40
    SEStop
    SEPlay SEQ_SE_FLD_59
    VMSleep 40
    SEStop
    SEPlay SEQ_SE_FLD_59
    RTReserveScript 3
    MapChangeCore ZONE_DRIFTVEIL_CITY_GYM_2, 6, 0, 4, 1
    VMJump L_0322

L_02E9:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0312
    // "Clay: Mrrmph! You, huh?\nRemember what I done told ya?[f000]븁\u0000\nIf ya think ya can go, go wherever,\nand if ya think ya can do somethin',[f000]븀\u0000\nkeep doin' it.[f000]븁\u0000\nDo things how ya want!\nDecide yer own limits."
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_ClayMrrmphHuhRemember, 7, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0322

L_0312:
    // "I don't know how much potential ya got,\nbut if ya think ya can go, go wherever,[f000]븀\u0000\nand if ya think ya can do somethin',[f000]븀\u0000\nkeep doin' it.[f000]븁\u0000\nDo things how ya want!\nDecide yer own limits."
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DontKnowHowMuch, 7, 0, 0
    LastKeyWait
    ActorMsgClose

L_0322:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 4
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B4
    VMStackPushFlag EVENT_FLAG_0x006f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03A0
    // "Welcome to the Driftveil Gym!\nThis is for you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_WelcomeDriftveilGym, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Gym Leader Clay uses\nGround-type Pokémon![f000]븁\u0000\nWell, just between you and me,\nGround-type Pokémon aren't good against[f000]븀\u0000\nWater-type attacks.[f000]븁\u0000\nThey also don't like Grass-\nor Ice-type attacks![f000]븁\u0000\nOh, and in this Gym, the area you\nwalk on will light up.[f000]븁\u0000\nWhere you've been is a hint\nfor where you need to go!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_GymLeaderClayUses, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x006f
    VMJump L_03AE

L_03A0:
    // "Gym Leader Clay uses\nGround-type Pokémon![f000]븁\u0000\nWell, just between you and me,\nGround-type Pokémon aren't good against[f000]븀\u0000\nWater-type attacks.[f000]븁\u0000\nThey also don't like Grass-\nor Ice-type attacks![f000]븁\u0000\nOh, and in this Gym, the area you\nwalk on will light up.[f000]븁\u0000\nWhere you've been is a hint\nfor where you need to go!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_GymLeaderClayUses, 0, 0
    LastKeyWait
    ActorMsgClose

L_03AE:
    VMJump L_03C2

L_03B4:
    // "Every now and again, I hope you look at\nthe Quake Badge you won here and[f000]븀\u0000\nremember your battle with Clay!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_EveryNowAgainHope, 0, 0
    LastKeyWait
    ActorMsgClose

L_03C2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    VMCall L_04B0
    SEPlay SEQ_SE_FLD_61
    SEWait
    VMCall L_04C2
    Cmd_018C 0, 0
    VMCall L_058C
    Cmd_018C 0, 1
    VMCall L_0656
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    VMCall L_04B0
    SEPlay SEQ_SE_FLD_61
    SEWait
    VMCall L_04C2
    Cmd_018C 1, 0
    VMCall L_058C
    Cmd_018C 1, 1
    VMCall L_0656
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    VMCall L_04B0
    SEPlay SEQ_SE_FLD_61
    SEWait
    VMCall L_04C2
    Cmd_018C 2, 0
    VMCall L_058C
    Cmd_018C 2, 1
    VMCall L_0656
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    VMCall L_04B0
    SEPlay SEQ_SE_FLD_61
    SEWait
    VMCall L_04C2
    Cmd_018C 3, 0
    VMCall L_058C
    Cmd_018C 3, 1
    VMCall L_0656
    SEWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_04B0:
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 pressed the\nswitch on the elevator!"
    InfoMsg DriftveilCityGym_Text_PressedSwitchElevator, 2
    MsgWaitAdvance
    MsgWinCloseAll
    VMReturn

L_04C2:
    EvCameraInit
    EvCameraUnbind
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_04D9
    VMJump L_04F7

L_04D9:
    EvCameraMoveTo 10840, 0, 0xed000, 0x110000, 0xc6000, 0x1be000, 30
    VMJump L_058A

L_04F7:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_050A
    VMJump L_0528

L_050A:
    EvCameraMoveTo 10840, 0, 0xed000, 0x2f0000, 0xc6000, 0x1be000, 30
    VMJump L_058A

L_0528:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_053B
    VMJump L_0559

L_053B:
    EvCameraMoveTo 10840, 0, 0xed000, 0x110000, 0xc6000, 0x3ce000, 30
    VMJump L_058A

L_0559:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_056C
    VMJump L_058A

L_056C:
    EvCameraMoveTo 10840, 0, 0xed000, 0x2f0000, 0xc6000, 0x3ce000, 30
    VMJump L_058A

L_058A:
    VMReturn

L_058C:
    EvCameraWait
    LastKeyWait
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_05A3
    VMJump L_05C1

L_05A3:
    EvCameraMoveTo 9688, 0, 0xed000, 0x118000, 0, 0x1d8000, 30
    VMJump L_0654

L_05C1:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_05D4
    VMJump L_05F2

L_05D4:
    EvCameraMoveTo 9688, 0, 0xed000, 0x2f8000, 0, 0x1d8000, 30
    VMJump L_0654

L_05F2:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0605
    VMJump L_0623

L_0605:
    EvCameraMoveTo 9688, 0, 0xed000, 0x118000, 0, 0x3e8000, 30
    VMJump L_0654

L_0623:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_0636
    VMJump L_0654

L_0636:
    EvCameraMoveTo 9688, 0, 0xed000, 0x2f8000, 0, 0x3e8000, 30
    VMJump L_0654

L_0654:
    VMReturn

L_0656:
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_8:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_FLD_61
    // "[f000]Ā\u0001\u0000 pressed the\nswitch on the elevator!"
    InfoMsg DriftveilCityGym_Text_PressedSwitchElevator, 2
    SEWait
    MsgWaitAdvance
    MsgWinCloseAll
    FadeOutBlack
    Cmd_018D 1
    FadeWait
    RTReserveScript 2
    MapChangeCore ZONE_DRIFTVEIL_CITY_GYM_2, 8, 0, 4, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_0690:
    Move 3, 1
    Move 73, 1
    Move 14, 1
    Move 74, 1
    MoveEnd

Movement_06A4:
    Move 13, 1
    MoveEnd

Movement_06AC:
    Move 12, 1
    MoveEnd

Movement_06B4:
    Move 15, 1
    MoveEnd
    VMStackMul
    VMNop2
    PokePartyGetSpecies 0, 13
    VMHalt
    PokePartyGetSpecies 0, 12
    VMHalt
    .byte 0xfe
    .balign 4, 0

Movement_06D4:
    Move 15, 2
    MoveEnd
    VMStackMul
    VMHalt
    .byte 0xfe
    .balign 4, 0
    Move 13, 3
    MoveEnd
    Move 12, 3
    MoveEnd

Movement_06F4:
    Move 15, 3
    MoveEnd

Movement_06FC:
    Move 14, 3
    MoveEnd

Movement_0704:
    Move 0, 1
    MoveEnd

Movement_070C:
    Move 1, 1
    MoveEnd

Movement_0714:
    Move 2, 1
    MoveEnd

Movement_071C:
    Move 3, 1
    MoveEnd

Movement_0724:
    Move 32, 1
    MoveEnd

Movement_072C:
    Move 33, 1
    MoveEnd

Movement_0734:
    Move 34, 1
    MoveEnd

Movement_073C:
    Move 35, 1
    MoveEnd

Movement_0744:
    Move 189, 1
    MoveEnd

Script_11:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40b9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 6, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_NOEL
    ActorCmdWait
    // "My Pokémon dig because they\nbelieve they're gonna find something,[f000]븀\u0000\nand they battle because they believe[f000]븀\u0000\nthey're gonna win![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_PokemonDigBecauseThey, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_NOEL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A8
    CallTrainerBattleEnd
    VMJump L_07AA

L_07A8:
    CallTrainerLose

L_07AA:
    // "Clay's awesome!\nHe can just tell if minerals will be there![f000]븁\u0000\nGot it? Another way to say it is\nthat Clay will be wherever[f000]븀\u0000\nthere are oodles of ores!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_ClaysAwesomeHeCan, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40b9, 1
    VMJump L_07D8

L_07C4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Clay's awesome!\nHe can just tell if minerals will be there![f000]븁\u0000\nGot it? Another way to say it is\nthat Clay will be wherever[f000]븀\u0000\nthere are oodles of ores!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_ClaysAwesomeHeCan, 0, 0
    LastKeyWait
    ActorMsgClose

L_07D8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    ActorCmdExec 6, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_NOEL
    ActorCmdWait
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PlayerGetGPos 0x8023, 0x8024
    WorkCmpConst 0x8023, 5
    VMJumpIf CMP_EQ, L_0813
    VMJump L_0821

L_0813:
    ActorCmdExec 6, Movement_071C
    VMJump L_088C

L_0821:
    WorkCmpConst 0x8023, 6
    VMJumpIf CMP_EQ, L_0834
    VMJump L_0842

L_0834:
    ActorCmdExec 6, Movement_06B4
    VMJump L_088C

L_0842:
    WorkCmpConst 0x8023, 7
    VMJumpIf CMP_EQ, L_0855
    VMJump L_0863

L_0855:
    ActorCmdExec 6, Movement_06D4
    VMJump L_088C

L_0863:
    WorkCmpConst 0x8023, 8
    VMJumpIf CMP_EQ, L_0876
    VMJump L_0884

L_0876:
    ActorCmdExec 6, Movement_06F4
    VMJump L_088C

L_0884:
    ActorCmdExec 6, Movement_0724

L_088C:
    ActorCmdWait
    ActorCmdExec 255, Movement_0714
    ActorCmdWait
    // "My Pokémon dig because they\nbelieve they're gonna find something,[f000]븀\u0000\nand they battle because they believe[f000]븀\u0000\nthey're gonna win![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_PokemonDigBecauseThey, 6, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_NOEL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08CD
    CallTrainerBattleEnd
    VMJump L_08CF

L_08CD:
    CallTrainerLose

L_08CF:
    // "Clay's awesome!\nHe can just tell if minerals will be there![f000]븁\u0000\nGot it? Another way to say it is\nthat Clay will be wherever[f000]븀\u0000\nthere are oodles of ores!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_ClaysAwesomeHeCan, 6, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40b9, 1
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40ba
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_096F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 1, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_TAVARIUS
    ActorCmdWait
    // "The one you meet when you get on this\nconveyor is none other than me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_OneMeetWhenGet, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_TAVARIUS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0953
    CallTrainerBattleEnd
    VMJump L_0955

L_0953:
    CallTrainerLose

L_0955:
    // "Have you used the elevator?[f000]븁\u0000\nIf you get on the elevator,\nyou can figure out where you[f000]븀\u0000\nhaven't been, right?"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HaveUsedElevatorIf, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ba, 1
    VMJump L_0983

L_096F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Have you used the elevator?[f000]븁\u0000\nIf you get on the elevator,\nyou can figure out where you[f000]븀\u0000\nhaven't been, right?"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HaveUsedElevatorIf, 0, 0
    LastKeyWait
    ActorMsgClose

L_0983:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    ActorCmdExec 1, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_TAVARIUS
    ActorCmdWait
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    PlayerGetGPos 0x8025, 0x8026
    WorkCmpConst 0x8026, 61
    VMJumpIf CMP_EQ, L_09BE
    VMJump L_09CC

L_09BE:
    ActorCmdExec 1, Movement_06AC
    VMJump L_09D4

L_09CC:
    ActorCmdExec 1, Movement_0724

L_09D4:
    ActorCmdWait
    ActorCmdExec 255, Movement_070C
    ActorCmdWait
    // "The one you meet when you get on this\nconveyor is none other than me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_OneMeetWhenGet, 1, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_TAVARIUS, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A15
    CallTrainerBattleEnd
    VMJump L_0A17

L_0A15:
    CallTrainerLose

L_0A17:
    // "Have you used the elevator?[f000]븁\u0000\nIf you get on the elevator,\nyou can figure out where you[f000]븀\u0000\nhaven't been, right?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HaveUsedElevatorIf, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40ba, 1
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40bb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AB7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 2, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_TIBOR
    ActorCmdWait
    // "I have a riddle for you!\nDo you know what is distant but close?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HaveRiddleKnowWhat, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_TIBOR, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A9B
    CallTrainerBattleEnd
    VMJump L_0A9D

L_0A9B:
    CallTrainerLose

L_0A9D:
    // "Something distant but close...[f000]븁\u0000\nI'm talking about Clay, who is near here\nbut is rather reserved.[f000]븁\u0000\nSorry... I think you were expecting\nsomething more interesting!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_SomethingDistantButClose, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bb, 1
    VMJump L_0ACB

L_0AB7:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Something distant but close...[f000]븁\u0000\nI'm talking about Clay, who is near here\nbut is rather reserved.[f000]븁\u0000\nSorry... I think you were expecting\nsomething more interesting!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_SomethingDistantButClose, 0, 0
    LastKeyWait
    ActorMsgClose

L_0ACB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    ActorCmdExec 2, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_TIBOR
    ActorCmdWait
    ActorCmdExec 255, Movement_0704
    ActorCmdWait
    // "I have a riddle for you!\nDo you know what is distant but close?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HaveRiddleKnowWhat, 2, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_TIBOR, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B20
    CallTrainerBattleEnd
    VMJump L_0B22

L_0B20:
    CallTrainerLose

L_0B22:
    // "Something distant but close...[f000]븁\u0000\nI'm talking about Clay, who is near here\nbut is rather reserved.[f000]븁\u0000\nSorry... I think you were expecting\nsomething more interesting!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_SomethingDistantButClose, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bb, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0BB6
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 3, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_NIEL
    ActorCmdWait
    // "Just because you work hard doesn't\nmean you're gonna get what you want![f000]븁\u0000\nBut if you don't work hard,\nthere are many things you can't do.[f000]븁\u0000\nLet me show you how tough\nmy hard work has made me![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_JustBecauseWorkHard, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_NIEL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B9A
    CallTrainerBattleEnd
    VMJump L_0B9C

L_0B9A:
    CallTrainerLose

L_0B9C:
    // "Driftveil City's Pokémon Gym is soaked in\nthe sweat and tears of the Pokémon that[f000]븀\u0000\nworked so hard to dig it out of the rock.[f000]븀\u0000\nSo...well...it smells kinda funny!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DriftveilCitysPokemonGym, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bc, 1
    VMJump L_0BCA

L_0BB6:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Driftveil City's Pokémon Gym is soaked in\nthe sweat and tears of the Pokémon that[f000]븀\u0000\nworked so hard to dig it out of the rock.[f000]븀\u0000\nSo...well...it smells kinda funny!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DriftveilCitysPokemonGym, 0, 0
    LastKeyWait
    ActorMsgClose

L_0BCA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    ActorCmdExec 3, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_NIEL
    ActorCmdWait
    ActorCmdExec 3, Movement_06F4
    ActorCmdWait
    // "Just because you work hard doesn't\nmean you're gonna get what you want![f000]븁\u0000\nBut if you don't work hard,\nthere are many things you can't do.[f000]븁\u0000\nLet me show you how tough\nmy hard work has made me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_JustBecauseWorkHard, 3, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_NIEL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C1F
    CallTrainerBattleEnd
    VMJump L_0C21

L_0C1F:
    CallTrainerLose

L_0C21:
    // "Driftveil City's Pokémon Gym is soaked in\nthe sweat and tears of the Pokémon that[f000]븀\u0000\nworked so hard to dig it out of the rock.[f000]븀\u0000\nSo...well...it smells kinda funny!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DriftveilCitysPokemonGym, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bc, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40bd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CB5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 4, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_PASQUAL
    ActorCmdWait
    // "Me and my Pokémon are\nprofessional tunnelers![f000]븁\u0000\nMy Pokémon can't be outdug\nor outburrowed! We have no rival![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_PokemonProfessionalTunnelersPokemon, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_PASQUAL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C99
    CallTrainerBattleEnd
    VMJump L_0C9B

L_0C99:
    CallTrainerLose

L_0C9B:
    // "Drilbur and Excadrill know the move\nDrill Run![f000]븁\u0000\nWhen I order them to use that move,\nI get all wound up!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DrilburExcadrillKnowMove, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bd, 1
    VMJump L_0CC9

L_0CB5:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Drilbur and Excadrill know the move\nDrill Run![f000]븁\u0000\nWhen I order them to use that move,\nI get all wound up!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DrilburExcadrillKnowMove, 0, 0
    LastKeyWait
    ActorMsgClose

L_0CC9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    ActorCmdExec 4, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_PASQUAL
    ActorCmdWait
    ActorCmdExec 255, Movement_070C
    ActorCmdWait
    // "Me and my Pokémon are\nprofessional tunnelers![f000]븁\u0000\nMy Pokémon can't be outdug\nor outburrowed! We have no rival![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_PokemonProfessionalTunnelersPokemon, 4, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_PASQUAL, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D1E
    CallTrainerBattleEnd
    VMJump L_0D20

L_0D1E:
    CallTrainerLose

L_0D20:
    // "Drilbur and Excadrill know the move\nDrill Run![f000]븁\u0000\nWhen I order them to use that move,\nI get all wound up!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_DrilburExcadrillKnowMove, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bd, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40be
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DB4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 5, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_MAYNARD
    ActorCmdWait
    // "Here in the darkness...[f000]븁\u0000\nI proceeded step by step\nwhile feeling my Pokémon's every breath![f000]븁\u0000\nI'll show you the power of the bonds\nmy Pokémon and I built in this way![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HereDarknessProceededStep, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_MAYNARD, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D98
    CallTrainerBattleEnd
    VMJump L_0D9A

L_0D98:
    CallTrainerLose

L_0D9A:
    // "Isn't darkness great?[f000]븁\u0000\nIt's the space of dreams where\nyou don't know what's even there!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_IsntDarknessGreatIts, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40be, 1
    VMJump L_0DC8

L_0DB4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Isn't darkness great?[f000]븁\u0000\nIt's the space of dreams where\nyou don't know what's even there!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_IsntDarknessGreatIts, 0, 0
    LastKeyWait
    ActorMsgClose

L_0DC8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_23:
    ActorsPauseAll
    ActorCmdExec 5, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_MAYNARD
    ActorCmdWait
    ActorCmdExec 255, Movement_0704
    ActorCmdWait
    // "Here in the darkness...[f000]븁\u0000\nI proceeded step by step\nwhile feeling my Pokémon's every breath![f000]븁\u0000\nI'll show you the power of the bonds\nmy Pokémon and I built in this way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_HereDarknessProceededStep, 5, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_MAYNARD, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E1D
    CallTrainerBattleEnd
    VMJump L_0E1F

L_0E1D:
    CallTrainerLose

L_0E1F:
    // "Isn't darkness great?[f000]븁\u0000\nIt's the space of dreams where\nyou don't know what's even there!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_IsntDarknessGreatIts, 5, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40be, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40bf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EB3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorCmdExec 8, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_FRIEDRICH
    ActorCmdWait
    // "When I say dig,\nyou say, “How low?\"[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_WhenSayDigSay, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_FRIEDRICH, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E97
    CallTrainerBattleEnd
    VMJump L_0E99

L_0E97:
    CallTrainerLose

L_0E99:
    // "Life is filled with pitfalls!\nIf you fall in, do your best to crawl out!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_LifeFilledPitfallsIf, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bf, 1
    VMJump L_0EC7

L_0EB3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Life is filled with pitfalls!\nIf you fall in, do your best to crawl out!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_LifeFilledPitfallsIf, 0, 0
    LastKeyWait
    ActorMsgClose

L_0EC7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_24:
    ActorsPauseAll
    ActorCmdExec 8, Movement_0744
    TrainerBGMPlayPush TRAINER_WORKER_FRIEDRICH
    ActorCmdWait
    ActorCmdExec 8, Movement_06FC
    ActorCmdWait
    // "When I say dig,\nyou say, “How low?\"[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_WhenSayDigSay, 8, 0, 0
    MsgWinCloseAll
    CallTrainerBattle TRAINER_WORKER_FRIEDRICH, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F1C
    CallTrainerBattleEnd
    VMJump L_0F1E

L_0F1C:
    CallTrainerLose

L_0F1E:
    // "Life is filled with pitfalls!\nIf you fall in, do your best to crawl out!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCityGym_Text_LifeFilledPitfallsIf, 8, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40bf, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
