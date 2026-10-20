#include "asm/field_script.inc"
#include "text/script/opelucid_city_gym.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    Cmd_02A4
    WorkSetConst 0x8023, 0
    TrainerFlagGet TRAINER_VETERAN_LUCIUS, 0x8023
    VMStackPush EVENT_WORK_0x40cf
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007D
    WorkSetConst EVENT_WORK_0x4000, 1
    VMJump L_0083

L_007D:
    WorkSetConst EVENT_WORK_0x4000, 0

L_0083:
    WorkSetConst EVENT_WORK_0x4001, 0
    WorkSetConst EVENT_WORK_0x4002, 0
    WorkSetConst EVENT_WORK_0x4003, 0
    WorkSetConst 0x8023, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_DRGGYM_06
    ActorCmdExec 255, Movement_07CC
    ActorCmdWait
    ActorCmdExec 255, Movement_07C0
    ActorCmdWait
    Cmd_02A5 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_DRGGYM_06
    ActorCmdExec 255, Movement_07DC
    ActorCmdWait
    ActorCmdExec 255, Movement_07C0
    ActorCmdWait
    Cmd_02A5 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_DRGGYM_06
    ActorCmdExec 255, Movement_07EC
    ActorCmdWait
    ActorCmdExec 255, Movement_07C0
    ActorCmdWait
    Cmd_02A5 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_SW_DRGGYM_06
    ActorCmdExec 255, Movement_07FC
    ActorCmdWait
    ActorCmdExec 255, Movement_07C0
    ActorCmdWait
    Cmd_02A5 3
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    WorkSetConst 0x8024, 0

L_0133:
    CallTrainerBattle 0x8024, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015A
    CallTrainerBattleEnd
    VMJump L_015C

L_015A:
    CallTrainerLose

L_015C:
    TrainerFlagSet 0x8024
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 381
    WorkSetConst 0x8025, 0
    TrainerFlagGet 0x8024, 0x8025
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CE
    // "Always aim for the top,\nlike a dragon taking flight![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_AlwaysAimTopLike, 0, 0
    MsgWinCloseAll
    VMCall L_0133
    // "Trainers far stronger than I\nwait for you above![f000]븁\u0000\nThe ones on the right\nemphasize attack.[f000]븁\u0000\nThe ones on the left\nemphasize defense.[f000]븁\u0000\nReturn to the dragon's head,\nand step on the glowing triangle.[f000]븀\u0000\nThen, choose either right or left!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersFarStrongerThan, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush EVENT_WORK_0x40cf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C8
    WorkSetConst EVENT_WORK_0x40cf, 1

L_01C8:
    VMJump L_01DC

L_01CE:
    // "Trainers far stronger than I\nwait for you above![f000]븁\u0000\nThe ones on the right\nemphasize attack.[f000]븁\u0000\nThe ones on the left\nemphasize defense.[f000]븁\u0000\nReturn to the dragon's head,\nand step on the glowing triangle.[f000]븀\u0000\nThen, choose either right or left!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersFarStrongerThan, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01DC:
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 384
    WorkSetConst 0x8026, 0
    TrainerFlagGet 0x8024, 0x8026
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0289
    WorkSetConst 0x8027, 0
    PokePartyGetCount 0x8027, 2
    VMStackPush 0x8027
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0248
    // "If you want to battle with me,\nbring three or more healthy[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_IfWantBattleBring, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0283

L_0248:
    // "Triple Battles![f000]븁\u0000\nIt's a combination that raises\nthree Pokémon's destructive power![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TripleBattlesItsCombination, 0, 0
    MsgWinCloseAll
    VMCall L_0133
    Cmd_02A6
    VMStackPush EVENT_WORK_0x40d1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0275
    WorkSetConst EVENT_WORK_0x40d1, 1

L_0275:
    // "You are strong enough to challenge\nMayor Drayden![f000]븁\u0000\nYou have the power to unleash the\nfull force of your deep desire to win!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_StrongEnoughChallengeMayor, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0283:
    VMJump L_0297

L_0289:
    // "You are strong enough to challenge\nMayor Drayden![f000]븁\u0000\nYou have the power to unleash the\nfull force of your deep desire to win!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_StrongEnoughChallengeMayor, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0297:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 385
    WorkSetConst 0x8028, 0
    TrainerFlagGet 0x8024, 0x8028
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034A
    WorkSetConst 0x8029, 0
    PokePartyGetCount 0x8029, 2
    VMStackPush 0x8029
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0309
    // "If you plan on battling with me,\nbring three or more healthy[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_IfPlanBattlingBring, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0344

L_0309:
    // "In Rotation Battles, how the three\nPokémon follow one another is key.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_RotationBattlesHowThree, 0, 0
    MsgWinCloseAll
    VMCall L_0133
    Cmd_02A6
    VMStackPush EVENT_WORK_0x40d1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0336
    WorkSetConst EVENT_WORK_0x40d1, 1

L_0336:
    // "A battle between you and Mayor\nDrayden must be stirring![f000]븁\u0000\nYou must desire victory\nmore than anything! Now, go!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_BattleBetweenMayorDrayden, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0344:
    VMJump L_0358

L_034A:
    // "A battle between you and Mayor\nDrayden must be stirring![f000]븁\u0000\nYou must desire victory\nmore than anything! Now, go!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_BattleBetweenMayorDrayden, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0358:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 383
    WorkSetConst 0x802a, 0
    TrainerFlagGet 0x8024, 0x802a
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03D8
    // "Defense is everything![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_DefenseEverything, 0, 0
    MsgWinCloseAll
    VMCall L_0133
    Cmd_02A6
    VMStackPush EVENT_WORK_0x40d0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C4
    WorkSetConst EVENT_WORK_0x40d0, 1

L_03C4:
    // "The Trainers that wait above\nhave different fighting styles.[f000]븁\u0000\nThe one on the right fights\nonly Triple Battles.[f000]븁\u0000\nThe one on the left fights\nonly Rotation Battles.[f000]븁\u0000\nChoose the style of battle\nyou're better at![f000]븁\u0000\nRemember, they won't even battle you\nif you don't have three or more[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersWaitAboveHave, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03E6

L_03D8:
    // "The Trainers that wait above\nhave different fighting styles.[f000]븁\u0000\nThe one on the right fights\nonly Triple Battles.[f000]븁\u0000\nThe one on the left fights\nonly Rotation Battles.[f000]븁\u0000\nChoose the style of battle\nyou're better at![f000]븁\u0000\nRemember, they won't even battle you\nif you don't have three or more[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersWaitAboveHave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_03E6:
    WorkSetConst 0x802a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8024, 382
    WorkSetConst 0x802b, 0
    TrainerFlagGet 0x8024, 0x802b
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0460
    // "Offense is everything![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_OffenseEverything, 0, 0
    MsgWinCloseAll
    VMCall L_0133
    Cmd_02A6
    VMStackPush EVENT_WORK_0x40d0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044C
    WorkSetConst EVENT_WORK_0x40d0, 1

L_044C:
    // "As for the Trainers above me...[f000]븁\u0000\nThe one on the right fights\nonly Triple Battles.[f000]븁\u0000\nThe one on the left fights\nonly Rotation Battles.[f000]븁\u0000\nChoose the style of battle\nthat fits you better![f000]븁\u0000\nRemember, they won't even battle you\nif you don't have three or more[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersAboveOneRight, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_046E

L_0460:
    // "As for the Trainers above me...[f000]븁\u0000\nThe one on the right fights\nonly Triple Battles.[f000]븁\u0000\nThe one on the left fights\nonly Rotation Battles.[f000]븁\u0000\nChoose the style of battle\nthat fits you better![f000]븁\u0000\nRemember, they won't even battle you\nif you don't have three or more[f000]븀\u0000\nPokémon with you!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_TrainersAboveOneRight, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_046E:
    WorkSetConst 0x802b, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    ActorCmdExec 0, Movement_080C
    ActorCmdWait
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8021
    VMStackPushConst 25
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04BF
    WorkSub 0x8022, 1
    ActorWalkRoute 0, 0x8021, 0x8022, 4, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_081C
    ActorCmdWait

L_04BF:
    // "Welcome to the Opelucid City Gym![f000]븁\u0000\nHere's some Fresh Water. Stay hydrated![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_WelcomeOpelucidCityGym, 0, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMCall L_0588
    // "How do you like these dragon statues?[f000]븁\u0000\nThis Gym is the only place in Unova\nwith statues this size![f000]븁\u0000\nClimb on the dragon's head and aim\nfor the top of the other dragon, where[f000]븀\u0000\nDrayden, the Gym Leader, is waiting![f000]븁\u0000\nDragon-type Pokémon are his specialty.[f000]븁\u0000\nJust between you and me, Dragon types\nare weak to Ice-type moves as well as to[f000]븀\u0000\nmoves of their own type, meaning Dragon."
    InfoMsg OpelucidCityGym_Text_HowLikeTheseDragon, 2
    VMCall L_05A6
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPush 0x8021
    VMStackPushConst 25
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_052F
    ActorWalkRoute 0, 25, 51, 4, 8, 0
    ActorCmdWait
    ActorCmdExec 0, Movement_081C
    ActorCmdWait

L_052F:
    FlagSet EVENT_FLAG_0x0072
    WorkSetConst EVENT_WORK_0x4145, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 6
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0574
    // "How do you like these dragon statues?[f000]븁\u0000\nThis Gym is the only place in Unova\nwith statues this size![f000]븁\u0000\nClimb on the dragon's head and aim\nfor the top of the other dragon, where[f000]븀\u0000\nDrayden, the Gym Leader, is waiting![f000]븁\u0000\nDragon-type Pokémon are his specialty.[f000]븁\u0000\nJust between you and me, Dragon types\nare weak to Ice-type moves as well as to[f000]븀\u0000\nmoves of their own type, meaning Dragon."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_HowLikeTheseDragon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0582

L_0574:
    // "You defeated the Gym Leader!\nCongratulations![f000]븁\u0000\nThe collision of the two dragon statues\nhad quite an impact! Like something out[f000]븀\u0000\nof a Pokéstar Studios film!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_DefeatedGymLeaderCongratulations, 0, 0
    LastKeyWait
    ActorMsgClose

L_0582:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0588:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5976, 3840, 0xed000, 0x128000, 0x3c2000, 0x16f000, 150
    VMReturn

L_05A6:
    EvCameraWait
    EvCameraMoveToDefault 60
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 6
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_073A
    Cmd_02A8
    WorkSetConst 0x802c, 0
    Cmd_02B2 0, 0x802c
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_060C
    Cmd_02B5 0, 2
    // "Welcome. Thank you for coming. I am the\nOpelucid Pokémon Gym Leader, Drayden.[f000]븁\u0000\nAs the mayor, I've given everything\nto developing the city.[f000]븁\u0000\nAs a Trainer, I've simply been pursuing\ngreater strength.[f000]븁\u0000\nBut what I'm searching for now is a young\nTrainer who can show me a bright future.[f000]븁\u0000\nPerhaps you can show me that future,\nthe way [f000]Ā\u0001\u0002 and Iris have?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_WelcomeThankComingAm_2, 0, 0
    VMJump L_0616

L_060C:
    // "Welcome. Thank you for coming. I am the\nOpelucid Pokémon Gym Leader, Drayden.[f000]븁\u0000\nAs the mayor, I've given everything\nto developing the city.[f000]븁\u0000\nAs a Trainer, I've simply been pursuing\ngreater strength.[f000]븁\u0000\nBut what I'm searching for now is a young\nTrainer who can show me a bright future.[f000]븁\u0000\nPerhaps you can show me that future,\nthe way Iris has?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_WelcomeThankComingAm, 0, 0

L_0616:
    ActorMsgClose
    WorkSetConst 0x802d, 0
    GameGetDifficulty 0x802d
    VMStackPush 0x802d
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0643
    CallTrainerBattle TRAINER_LEADER_DRAYDEN_2, 0, 0
    VMJump L_064B

L_0643:
    CallTrainerBattle TRAINER_LEADER_DRAYDEN, 0, 0

L_064B:
    WorkSetConst 0x802d, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0670
    CallTrainerBattleEnd
    VMJump L_0672

L_0670:
    CallTrainerLose

L_0672:
    // "Wonderful.[f000]븁\u0000\nI'm grateful that we had a chance\nto meet and battle.[f000]븁\u0000\nIt reminded me that Pokémon battles\nare about working with others[f000]븀\u0000\nto meet our challenges together.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_WonderfulImGratefulWe, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 6
    TrainerCardAddBadge 6
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x802e, 0
    TrainerCardGetSex 0x802e
    Cmd_02A8
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06B3
    PlayFieldEffect 9
    VMJump L_06B7

L_06B3:
    PlayFieldEffect 61

L_06B7:
    MEWait
    Cmd_02A9
    WorkSetConst 0x802e, 0
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received the Legend Badge\nfrom Drayden![f000]븁\u0000"
    SystemMsg OpelucidCityGym_Text_ReceivedLegendBadgeFrom, 0
    InfoMsgClose
    // "Well, now. You've obtained seven of\nthe Gym Badges in Unova.[f000]븁\u0000\nThat means Pokémon up to Lv. 80,\nincluding those received in trades,[f000]븀\u0000\nwill follow your commands obediently.[f000]븁\u0000\nI also want you to have this.\nIt's a TM I'm particularly fond of.[f000]븁\u0000\nIt's called Dragon Tail.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_WellNowYouveObtained, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 409
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Dragon Tail is a move that returns the\ndamaged Pokémon to its Poké Ball.[f000]븁\u0000\nIn other words, it switches your\nopponent's Pokémon.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_DragonTailMoveReturns, 0, 0
    // "Professor Juniper asked me to tell you\nabout the legendary Dragon-type Pokémon[f000]븀\u0000\nthat created Unova.[f000]븁\u0000\nPlease wait outside."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_ProfessorJuniperAskedTell, 0, 0
    LastKeyWait
    ActorMsgClose
    TrainerFlagSet TRAINER_VETERAN_LUCIUS
    TrainerFlagSet TRAINER_VETERAN_RON
    TrainerFlagSet TRAINER_VETERAN_DENAE
    TrainerFlagSet TRAINER_VETERAN_RHONA
    TrainerFlagSet TRAINER_VETERAN_JERRY
    FlagSet EVENT_FLAG_0x0974
    WorkSetConst EVENT_WORK_0x40d6, 1
    HollowRivalCmd_0262 1, 24
    VMJump L_0748

L_073A:
    // "It is not the passage of time that\ncauses Pokémon and people to age.[f000]븁\u0000\nWhen the energy that flows in their\nheart dries up, that's when they get old.[f000]븁\u0000\nThe heart's energy is powered by\ntruth, ideals, or maybe dreams...[f000]븁\u0000\nNo doubt it changes depending on\nwhat you most hope for in life."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityGym_Text_NotPassageTimeCauses, 0, 0
    LastKeyWait
    ActorMsgClose

L_0748:
    WorkSetConst 0x802c, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x40cf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0775
    Cmd_02A7 0
    WorkSetConst EVENT_WORK_0x40cf, 2
    Cmd_02A6

L_0775:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x802f, 0
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 6
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07B1
    // "Opelucid Pokémon Gym[f000]븁\u0000\nGym Leader: Drayden\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0001"
    InfoMsg OpelucidCityGym_Text_OpelucidPokemonGymGym, 2
    VMJump L_07B6

L_07B1:
    // "Opelucid Pokémon Gym[f000]븁\u0000\nGym Leader: Drayden\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg OpelucidCityGym_Text_OpelucidPokemonGymGym_2, 2

L_07B6:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_07C0:
    Move 32, 1
    Move 0, 1
    MoveEnd

Movement_07CC:
    Move 33, 1
    Move 1, 1
    Move 13, 1
    MoveEnd

Movement_07DC:
    Move 32, 1
    Move 0, 1
    Move 12, 1
    MoveEnd

Movement_07EC:
    Move 35, 1
    Move 3, 1
    Move 15, 1
    MoveEnd

Movement_07FC:
    Move 34, 1
    Move 2, 1
    Move 14, 1
    MoveEnd

Movement_080C:
    Move 75, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_081C:
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
