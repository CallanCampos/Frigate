#include "asm/field_script.inc"
#include "text/script/nimbasa_city_gym.h"

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
    ScriptEntriesEnd

Script_2:
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_005D
    WorkSetConst EVENT_WORK_0x409b, 0
    Cmd_025A 0
    VMJump L_00F2

L_005D:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007A
    Cmd_025A 0
    VMJump L_00F2

L_007A:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0097
    Cmd_025A 1
    VMJump L_00F2

L_0097:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B4
    Cmd_025A 2
    VMJump L_00F2

L_00B4:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D1
    Cmd_025A 3
    VMJump L_00F2

L_00D1:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00EE
    Cmd_025A 4
    VMJump L_00F2

L_00EE:
    Cmd_025A 0

L_00F2:
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010D
    VMJump L_01F5

L_010D:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0132
    ActorSetGPos 2, 9, 6, 2, 3
    VMJump L_01F5

L_0132:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0163
    ActorSetGPos 2, 9, 6, 2, 3
    ActorSetGPos 3, 9, 6, 3, 3
    VMJump L_01F5

L_0163:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AC
    ActorSetGPos 2, 9, 6, 2, 3
    ActorSetGPos 3, 9, 6, 3, 3
    ActorSetGPos 0, 21, 6, 2, 2
    ActorSetGPos 4, 15, 6, 4, 1
    VMJump L_01F5

L_01AC:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F5
    ActorSetGPos 2, 9, 6, 2, 3
    ActorSetGPos 3, 9, 6, 3, 3
    ActorSetGPos 0, 21, 6, 2, 2
    ActorSetGPos 4, 15, 6, 4, 1
    VMJump L_01F5

L_01F5:
    VMHalt

Script_13:
    FieldGetContinueFlag 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0210
    Cmd_025B

L_0210:
    VMStackPush EVENT_WORK_0x409b
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023A
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023A
    Cmd_025F 6

L_023A:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 3
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0269
    VMCall L_02F2
    VMJump L_02EC

L_0269:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DE
    VMStackPush EVENT_WORK_0x4001
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A3
    // "A shining example of a Trainer...[f000]븁\u0000\nSince that's what you are, you should\nbe able to collect all the Gym Badges[f000]븀\u0000\nand reach the Pokémon League![f000]븁\u0000\nThen, you and your Pokémon\nwill shine even brighter!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_ShiningExampleTrainerSince, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D8

L_02A3:
    VMStackPushFlag EVENT_FLAG_0x02f8
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02CA
    // "I want my relationship with my Pokémon\nto be less like sun and moon[f000]븀\u0000\nand more like two blazing suns![f000]븁\u0000\nThere'd be no limit to how bright\nwe could shine!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WantRelationshipPokemonLess, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02D8

L_02CA:
    // "What my Pokémon want to do\nand what I want to do...[f000]븀\u0000\nWhen they are the same...[f000]븁\u0000\nWe have more than twice\nthe energy to shine!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WhatPokemonWantWhat, 0, 0
    LastKeyWait
    ActorMsgClose

L_02D8:
    VMJump L_02EC

L_02DE:
    // "Elesa: A model always has to make\nother people's dreams a reality[f000]븀\u0000\nwithout losing sight of herself.[f000]븁\u0000\nIt's similar for Trainers and Pokémon.\nTrainers have to give everything they[f000]븀\u0000\nhave to make their Pokémon feel like[f000]븀\u0000\nthey can win, no matter the situation.[f000]븁\u0000\nWhen I realized that, modeling\nbecame that much more fun."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_ElesaModelAlwaysHas, 0, 0
    LastKeyWait
    ActorMsgClose

L_02EC:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_02F2:
    // "Welcome to the main stage![f000]븁\u0000\nMy beloved Pokémon\nand your Pokémon shall compete![f000]븁\u0000\nWe're going to see whose\nstar shines brightest![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WelcomeMainStageBeloved, 4, 2, 0
    ActorMsgClose
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_032B
    CallTrainerBattle TRAINER_LEADER_ELESA_2, 0, 0
    VMJump L_0333

L_032B:
    CallTrainerBattle TRAINER_LEADER_ELESA, 0, 0

L_0333:
    WorkSetConst 0x8020, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0398
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 3672, 0, 0xed000, 0xf8000, 0x6802f, 0x3f000, 1
    EvCameraWait
    ActorCmdExec 2, Movement_124C
    ActorCmdExec 3, Movement_124C
    ActorCmdExec 0, Movement_124C
    ActorCmdWait
    Cmd_025F 4
    Cmd_025F 5
    CallTrainerBattleEnd
    VMJump L_039A

L_0398:
    CallTrainerLose

L_039A:
    // "Well...[f000]븁\u0000\nNow, you...you're an even more wonderful\nTrainer than I expected.[f000]븁\u0000\nYour sweet fighting style\nswept me off my feet![f000]븀\u0000\nTake this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WellNowYoureEven, 4, 2, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 3
    TrainerCardAddBadge 3
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8021, 0
    TrainerCardGetSex 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DB
    PlayFieldEffect 6
    VMJump L_03DF

L_03DB:
    PlayFieldEffect 58

L_03DF:
    MEWait
    WorkSetConst 0x8021, 0
    Cmd_025F 2
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received the\nBolt Badge from Elesa![f000]븁\u0000"
    SystemMsg NimbasaCityGym_Text_ReceivedBoltBadgeFrom, 2
    InfoMsgClose
    // "If you have four Badges, including this\nBolt Badge, Pokémon up to Lv. 50,[f000]븀\u0000\nincluding traded Pokémon, will obey you.[f000]븁\u0000\nAlso, here's this move I like.\nFeel free to use it, um, if you want to.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_IfHaveFourBadges, 4, 2, 0
    ActorMsgClose
    Cmd_025F 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 399
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    Cmd_025F 2
    // "Volt Switch lets the Pokémon switch with\na different Pokémon after attacking.[f000]븁\u0000\nOf course, if you don't have another\nPokémon in your party, you can't switch."
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_VoltSwitchLetsPokemon, 4, 2, 0
    MsgWaitAdvance
    ActorMsgClose
    EvCameraMoveToDefault 30
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    Cmd_025F 0
    FlagSet EVENT_FLAG_0x0971
    FlagSet EVENT_FLAG_0x0108
    WorkSetConst EVENT_WORK_0x409c, 1
    WorkSetConst EVENT_WORK_0x40c0, 1
    FlagReset EVENT_FLAG_0x02f9
    FlagReset EVENT_FLAG_0x02fa
    HollowRivalCmd_0262 1, 6
    VMReturn

Script_4:
    ActorsPauseAll
    Cmd_025C 1, 2, 0
    VMSleep 16
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_055F
    TrainerBGMPlayPush TRAINER_BEAUTY_NIKOLA
    ActorCmdExec 2, Movement_09B0
    ActorCmdWait
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    WorkCmpConst 0x8022, 14
    VMJumpIf CMP_EQ, L_04C2
    VMJump L_04D2

L_04C2:
    ActorCmdExec 2, Movement_09B8
    ActorCmdWait
    VMJump L_0522

L_04D2:
    WorkCmpConst 0x8022, 15
    VMJumpIf CMP_EQ, L_04E5
    VMJump L_04F5

L_04E5:
    ActorCmdExec 2, Movement_0AA4
    ActorCmdWait
    VMJump L_0522

L_04F5:
    WorkCmpConst 0x8022, 16
    VMJumpIf CMP_EQ, L_0508
    VMJump L_0518

L_0508:
    ActorCmdExec 2, Movement_0C78
    ActorCmdWait
    VMJump L_0522

L_0518:
    ActorCmdExec 2, Movement_0AA4
    ActorCmdWait

L_0522:
    // "Welcome to the Nimbasa Gym![f000]븁\u0000\nA stylish Pokémon battle and\nfashion show created by Pokémon[f000]븀\u0000\nand Trainers is starting now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WelcomeNimbasaGymStylish, 2, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_BEAUTY_NIKOLA, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0557
    CallTrainerBattleEnd
    VMJump L_0559

L_0557:
    CallTrainerLose

L_0559:
    VMJump L_0569

L_055F:
    ActorCmdExec 2, Movement_0B78
    ActorCmdWait

L_0569:
    Cmd_025C 0, 2, 0
    Cmd_025F 1
    Cmd_025D 1
    Cmd_025F 0
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0609
    WorkCmpConst 0x8022, 14
    VMJumpIf CMP_EQ, L_05A3
    VMJump L_05B3

L_05A3:
    ActorCmdExec 2, Movement_0D64
    ActorCmdWait
    VMJump L_0603

L_05B3:
    WorkCmpConst 0x8022, 15
    VMJumpIf CMP_EQ, L_05C6
    VMJump L_05D6

L_05C6:
    ActorCmdExec 2, Movement_0DAC
    ActorCmdWait
    VMJump L_0603

L_05D6:
    WorkCmpConst 0x8022, 16
    VMJumpIf CMP_EQ, L_05E9
    VMJump L_05F9

L_05E9:
    ActorCmdExec 2, Movement_0DF0
    ActorCmdWait
    VMJump L_0603

L_05F9:
    ActorCmdExec 2, Movement_0DAC
    ActorCmdWait

L_0603:
    VMJump L_0613

L_0609:
    ActorCmdExec 2, Movement_0DAC
    ActorCmdWait

L_0613:
    ActorSetGPos 2, 9, 6, 2, 3
    WorkSetConst EVENT_WORK_0x409b, 1
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    Cmd_025C 1, 3, 0
    VMSleep 16
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0728
    TrainerBGMPlayPush TRAINER_BEAUTY_FLEMING
    ActorCmdExec 3, Movement_09B0
    ActorCmdWait
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PlayerGetGPos 0x8024, 0x8025
    WorkCmpConst 0x8024, 14
    VMJumpIf CMP_EQ, L_068B
    VMJump L_069B

L_068B:
    ActorCmdExec 3, Movement_09EC
    ActorCmdWait
    VMJump L_06EB

L_069B:
    WorkCmpConst 0x8024, 15
    VMJumpIf CMP_EQ, L_06AE
    VMJump L_06BE

L_06AE:
    ActorCmdExec 3, Movement_0AD0
    ActorCmdWait
    VMJump L_06EB

L_06BE:
    WorkCmpConst 0x8024, 16
    VMJumpIf CMP_EQ, L_06D1
    VMJump L_06E1

L_06D1:
    ActorCmdExec 3, Movement_0CAC
    ActorCmdWait
    VMJump L_06EB

L_06E1:
    ActorCmdExec 3, Movement_0AD0
    ActorCmdWait

L_06EB:
    // "Are you beautiful as a Trainer?\nSurprise me![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_BeautifulTrainerSurprise, 3, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_BEAUTY_FLEMING, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0720
    CallTrainerBattleEnd
    VMJump L_0722

L_0720:
    CallTrainerLose

L_0722:
    VMJump L_0732

L_0728:
    ActorCmdExec 3, Movement_0BA4
    ActorCmdWait

L_0732:
    Cmd_025C 0, 3, 0
    Cmd_025F 1
    Cmd_025D 2
    Cmd_025F 0
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D2
    WorkCmpConst 0x8024, 14
    VMJumpIf CMP_EQ, L_076C
    VMJump L_077C

L_076C:
    ActorCmdExec 3, Movement_0D64
    ActorCmdWait
    VMJump L_07CC

L_077C:
    WorkCmpConst 0x8024, 15
    VMJumpIf CMP_EQ, L_078F
    VMJump L_079F

L_078F:
    ActorCmdExec 3, Movement_0DAC
    ActorCmdWait
    VMJump L_07CC

L_079F:
    WorkCmpConst 0x8024, 16
    VMJumpIf CMP_EQ, L_07B2
    VMJump L_07C2

L_07B2:
    ActorCmdExec 3, Movement_0DF0
    ActorCmdWait
    VMJump L_07CC

L_07C2:
    ActorCmdExec 3, Movement_0DAC
    ActorCmdWait

L_07CC:
    VMJump L_07DC

L_07D2:
    ActorCmdExec 3, Movement_0DAC
    ActorCmdWait

L_07DC:
    ActorSetGPos 3, 9, 6, 3, 3
    WorkSetConst EVENT_WORK_0x409b, 2
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    Cmd_025C 1, 0, 0
    VMSleep 16
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0964
    TrainerBGMPlayPush TRAINER_BEAUTY_AMP_RE
    ActorCmdExec 0, Movement_09B0
    ActorCmdWait
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    PlayerGetGPos 0x8026, 0x8027
    WorkCmpConst 0x8026, 14
    VMJumpIf CMP_EQ, L_0854
    VMJump L_0864

L_0854:
    ActorCmdExec 0, Movement_0A3C
    ActorCmdWait
    VMJump L_08B4

L_0864:
    WorkCmpConst 0x8026, 15
    VMJumpIf CMP_EQ, L_0877
    VMJump L_0887

L_0877:
    ActorCmdExec 0, Movement_0B18
    ActorCmdWait
    VMJump L_08B4

L_0887:
    WorkCmpConst 0x8026, 16
    VMJumpIf CMP_EQ, L_089A
    VMJump L_08AA

L_089A:
    ActorCmdExec 0, Movement_0CFC
    ActorCmdWait
    VMJump L_08B4

L_08AA:
    ActorCmdExec 0, Movement_0B18
    ActorCmdWait

L_08B4:
    // "The show is coming to its finale.\nNow, I'll see if you are worthy[f000]븀\u0000\nto stand on the same stage as Elesa![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_ShowComingItsFinale, 0, 0, 0
    ActorMsgClose
    CallTrainerBattle TRAINER_BEAUTY_AMP_RE, 0, 0
    TrainerBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08E9
    CallTrainerBattleEnd
    VMJump L_08EB

L_08E9:
    CallTrainerLose

L_08EB:
    WorkCmpConst 0x8026, 14
    VMJumpIf CMP_EQ, L_08FE
    VMJump L_090E

L_08FE:
    ActorCmdExec 0, Movement_0D64
    ActorCmdWait
    VMJump L_095E

L_090E:
    WorkCmpConst 0x8026, 15
    VMJumpIf CMP_EQ, L_0921
    VMJump L_0931

L_0921:
    ActorCmdExec 0, Movement_0DAC
    ActorCmdWait
    VMJump L_095E

L_0931:
    WorkCmpConst 0x8026, 16
    VMJumpIf CMP_EQ, L_0944
    VMJump L_0954

L_0944:
    ActorCmdExec 0, Movement_0DF0
    ActorCmdWait
    VMJump L_095E

L_0954:
    ActorCmdExec 0, Movement_0DAC
    ActorCmdWait

L_095E:
    VMJump L_096E

L_0964:
    ActorCmdExec 0, Movement_0BEC
    ActorCmdWait

L_096E:
    Cmd_025C 0, 0, 0
    ActorSetGPos 0, 21, 6, 2, 2
    VMCall L_0E38
    BGMAmbienceResume
    WorkSetConst EVENT_WORK_0x409b, 3
    FlagSet EVENT_FLAG_0x097d
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_09A8:
    Move 75, 1
    MoveEnd

Movement_09B0:
    Move 189, 1
    MoveEnd

Movement_09B8:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_09EC:
    Move 15, 1
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 15, 1
    Move 61, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_0A3C:
    Move 9, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_0AA4:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 3
    MoveEnd

Movement_0AD0:
    Move 15, 1
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 15, 1
    Move 61, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 3
    MoveEnd

Movement_0B18:
    Move 9, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 3
    MoveEnd

Movement_0B78:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    MoveEnd

Movement_0BA4:
    Move 15, 1
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 15, 1
    Move 61, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    MoveEnd

Movement_0BEC:
    Move 9, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 12, 10
    MoveEnd

Movement_0C78:
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_0CAC:
    Move 15, 1
    Move 13, 1
    Move 14, 2
    Move 12, 1
    Move 15, 1
    Move 61, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_0CFC:
    Move 9, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 2, 1
    Move 0, 1
    Move 3, 1
    Move 1, 1
    Move 61, 1
    Move 13, 2
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_0D64:
    Move 12, 1
    Move 15, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 12, 8
    MoveEnd

Movement_0DAC:
    Move 12, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 12, 8
    MoveEnd

Movement_0DF0:
    Move 12, 1
    Move 14, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 1, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 12, 8
    MoveEnd

L_0E38:
    EvCameraInit
    EvCameraUnbind
    BGMFadeOutAll 90
    EvCameraMoveTo 3672, 0, 0xed000, 0xf8000, 0x6802f, 0x3f000, 70
    EvCameraWait
    BGMPlay SEQ_BGM_ERECTRIC_GYM_02
    Cmd_025C 1, 4, 3
    VMSleep 12
    Cmd_025C 1, 4, 1
    VMSleep 12
    Cmd_025C 1, 4, 2
    VMSleep 12
    ActorCmdExec 4, Movement_0F0C
    ActorCmdWait
    Cmd_025F 3
    FadeEx 12, 0, 16, 2
    FadeExWait
    FadeEx 12, 16, 0, 2
    VMSleep 4
    FadeExWait
    FadeEx 12, 0, 16, 2
    FadeExWait
    FadeEx 12, 16, 0, 2
    FadeExWait
    Cmd_025C 0, 4, 3
    Cmd_025C 0, 4, 1
    Cmd_025C 0, 4, 2
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EF3
    Cmd_025F 5

L_0EF3:
    Cmd_025F 1
    Cmd_025D 3
    Cmd_025F 0
    EvCameraMoveToDefault 90
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn
    .balign 4, 0

Movement_0F0C:
    Move 13, 1
    Move 100, 1
    MoveEnd

Script_12:
    ActorsPauseAll
    VMCall L_0F26
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0F26:
    WordSetPlayerName 0
    ActorSetGPos 2, 14, 3, 24, 1
    ActorSetGPos 3, 15, 3, 24, 1
    ActorSetGPos 0, 16, 3, 24, 1
    ActorSetGPos 4, 15, 3, 26, 1
    ActorCmdExec 2, Movement_11D4
    ActorCmdExec 3, Movement_11D4
    ActorCmdExec 0, Movement_11D4
    ActorCmdExec 4, Movement_11D4
    ActorCmdWait
    ActorCmdExec 255, Movement_1264
    ActorCmdWait
    // "Wait![f000]븁\u0000\nPlease walk with us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WaitPleaseWalkUs, 4, 0, 0
    ActorMsgClose
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    PlayerGetGPos 0x8028, 0x8029
    WorkCmpConst 0x8028, 14
    VMJumpIf CMP_EQ, L_0FB8
    VMJump L_0FD4

L_0FB8:
    ActorCmdExec 4, Movement_11F4
    VMSleep 16
    ActorCmdExec 255, Movement_11DC
    ActorCmdWait
    VMJump L_1048

L_0FD4:
    WorkCmpConst 0x8028, 15
    VMJumpIf CMP_EQ, L_0FE7
    VMJump L_1003

L_0FE7:
    ActorCmdExec 4, Movement_120C
    VMSleep 24
    ActorCmdExec 255, Movement_11E4
    ActorCmdWait
    VMJump L_1048

L_1003:
    WorkCmpConst 0x8028, 16
    VMJumpIf CMP_EQ, L_1016
    VMJump L_1032

L_1016:
    ActorCmdExec 4, Movement_121C
    VMSleep 16
    ActorCmdExec 255, Movement_11EC
    ActorCmdWait
    VMJump L_1048

L_1032:
    ActorCmdExec 4, Movement_121C
    VMSleep 24
    ActorCmdExec 255, Movement_11EC
    ActorCmdWait

L_1048:
    Cmd_025F 2
    MultiMsg 25, 3, 5, 1
    VMSleep 10
    MultiMsg 26, 19, 8, 2
    VMSleep 10
    MultiMsg 27, 2, 15, 3
    VMSleep 30
    MsgWinCloseNo 1
    VMSleep 5
    MsgWinCloseNo 2
    VMSleep 5
    MsgWinCloseNo 3
    SEWait
    ActorCmdExec 2, Movement_1234
    ActorCmdExec 3, Movement_1234
    ActorCmdExec 0, Movement_1234
    ActorCmdWait
    ActorCmdExec 255, Movement_123C
    VMSleep 4
    ActorCmdExec 2, Movement_123C
    ActorCmdExec 3, Movement_123C
    ActorCmdExec 0, Movement_123C
    ActorCmdExec 4, Movement_123C
    MultiMsg 28, 22, 6, 1
    VMSleep 10
    MultiMsg 29, 2, 9, 2
    VMSleep 10
    MultiMsg 30, 19, 13, 3
    VMSleep 30
    MsgWinCloseNo 1
    VMSleep 5
    MsgWinCloseNo 2
    VMSleep 5
    MsgWinCloseNo 3
    ActorCmdWait
    WorkCmpConst 0x8028, 14
    VMJumpIf CMP_EQ, L_1125
    VMJump L_113D

L_1125:
    ActorCmdExec 4, Movement_127C
    ActorCmdExec 255, Movement_126C
    ActorCmdWait
    VMJump L_11A5

L_113D:
    WorkCmpConst 0x8028, 15
    VMJumpIf CMP_EQ, L_1150
    VMJump L_1168

L_1150:
    ActorCmdExec 4, Movement_126C
    ActorCmdExec 255, Movement_127C
    ActorCmdWait
    VMJump L_11A5

L_1168:
    WorkCmpConst 0x8028, 16
    VMJumpIf CMP_EQ, L_117B
    VMJump L_1193

L_117B:
    ActorCmdExec 4, Movement_126C
    ActorCmdExec 255, Movement_127C
    ActorCmdWait
    VMJump L_11A5

L_1193:
    ActorCmdExec 4, Movement_126C
    ActorCmdExec 255, Movement_127C
    ActorCmdWait

L_11A5:
    // "A shining example of a Trainer...[f000]븁\u0000\nSince that's what you are, you should\nbe able to collect all the Gym Badges[f000]븀\u0000\nand reach the Pokémon League![f000]븁\u0000\nThen, you and your Pokémon\nwill shine even brighter!"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_ShiningExampleTrainerSince, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    Cmd_025F 0
    WorkSetConst EVENT_WORK_0x409c, 2
    WorkSetConst EVENT_WORK_0x4001, 1
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    VMReturn
    .balign 4, 0

Movement_11D4:
    Move 13, 8
    MoveEnd

Movement_11DC:
    Move 15, 1
    MoveEnd

Movement_11E4:
    Move 34, 1
    MoveEnd

Movement_11EC:
    Move 14, 1
    MoveEnd

Movement_11F4:
    Move 13, 1
    Move 34, 1
    Move 71, 1
    Move 15, 1
    Move 72, 1
    MoveEnd

Movement_120C:
    Move 14, 1
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_121C:
    Move 13, 1
    Move 35, 1
    Move 71, 1
    Move 14, 1
    Move 72, 1
    MoveEnd

Movement_1234:
    Move 13, 1
    MoveEnd

Movement_123C:
    Move 13, 18
    MoveEnd
    Move 0, 1
    MoveEnd

Movement_124C:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd

Movement_1264:
    Move 32, 1
    MoveEnd

Movement_126C:
    Move 35, 1
    MoveEnd

Movement_1274:
    Move 33, 1
    MoveEnd

Movement_127C:
    Move 34, 1
    MoveEnd

Script_7:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12B3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Isn't this Gym beautiful when it's lit up?[f000]븁\u0000\nThe Pokémon also look beautiful\nin this lighting!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_IsntGymBeautifulWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_12C7

L_12B3:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You and your Pokémon\nsparkle and shine!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_PokemonSparkleShine, 0, 0
    LastKeyWait
    ActorMsgClose

L_12C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_12FC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Ms. Elesa is strong and beautiful...[f000]븁\u0000\nDon't make the mistake of thinking\nshe's the same as us."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_MsElesaStrongBeautiful, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1310

L_12FC:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Beauty isn't just about looks.[f000]븁\u0000\nStrength is a part of beauty,\nboth for you and for Elesa."
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_BeautyIsntJustAbout, 0, 0
    LastKeyWait
    ActorMsgClose

L_1310:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1345
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Being able to see a match between\na challenger and Elesa this close...[f000]븁\u0000\nThat makes even us gleam with joy!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_BeingAbleSeeMatch, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_1359

L_1345:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "That was a beautiful Pokémon battle\nyou and Elesa had![f000]븁\u0000\nI was captivated by it!\nWhat an absolutely marvelous show!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_BeautifulPokemonBattleElesa, 0, 0
    LastKeyWait
    ActorMsgClose

L_1359:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 3
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_13EB
    VMStackPushFlag EVENT_FLAG_0x006e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_13D7
    // "What do you think? Surprised, right?[f000]븁\u0000\nThis Gym is, speaking frankly, a glittering\nfashion show and a dazzling stage![f000]븁\u0000\nWell, for now, I'll give you this![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WhatThinkSurprisedRight, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "In this Pokémon Gym, we'll have you\nproceed by defeating the Trainers waiting[f000]븀\u0000\non the catwalk![f000]븁\u0000\nBy the way, Electric-type Pokémon don't\ndo well against Ground-type moves...[f000]븁\u0000\nOh![f000]븁\u0000\nBut Ground-type moves don't work\nagainst a Pokémon called Emolga,[f000]븀\u0000\nso please be careful!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_PokemonGymWellHave, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x006e
    VMJump L_13E5

L_13D7:
    // "In this Pokémon Gym, we'll have you\nproceed by defeating the Trainers waiting[f000]븀\u0000\non the catwalk![f000]븁\u0000\nBy the way, Electric-type Pokémon don't\ndo well against Ground-type moves...[f000]븁\u0000\nOh![f000]븁\u0000\nBut Ground-type moves don't work\nagainst a Pokémon called Emolga,[f000]븀\u0000\nso please be careful!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_PokemonGymWellHave, 0, 0
    LastKeyWait
    ActorMsgClose

L_13E5:
    VMJump L_13F9

L_13EB:
    // "Elesa uses sparkling, bright\nElectric-type attacks![f000]븁\u0000\nBut the combination of you and your\nPokémon shone even brighter![f000]븁\u0000\nWhy, you're...you're...\na supermodel![f000]븁\u0000\nWell, no. You're just a really good\nTrainer with great Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_ElesaUsesSparklingBright, 0, 0
    LastKeyWait
    ActorMsgClose

L_13F9:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    PlayerGetGPos 0x802a, 0x802b
    ActorCmdExec 1, Movement_09A8
    ActorCmdWait
    ActorWalkRoute 1, 0x802a, 58, 0, 8, 0
    ActorCmdWait
    ActorCmdExec 1, Movement_1274
    ActorCmdWait
    // "What do you think? Surprised, right?[f000]븁\u0000\nThis Gym is, speaking frankly, a glittering\nfashion show and a dazzling stage![f000]븁\u0000\nWell, for now, I'll give you this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_WhatThinkSurprisedRight, 1, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "In this Pokémon Gym, we'll have you\nproceed by defeating the Trainers waiting[f000]븀\u0000\non the catwalk![f000]븁\u0000\nBy the way, Electric-type Pokémon don't\ndo well against Ground-type moves...[f000]븁\u0000\nOh![f000]븁\u0000\nBut Ground-type moves don't work\nagainst a Pokémon called Emolga,[f000]븀\u0000\nso please be careful!"
    ActorMsg MSGFILE_SCRIPT, NimbasaCityGym_Text_PokemonGymWellHave, 1, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 1, 17, 58, 0, 8, 1
    ActorCmdWait
    ActorCmdExec 1, Movement_1274
    ActorCmdWait
    FlagSet EVENT_FLAG_0x006e
    WorkSetConst EVENT_WORK_0x4142, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    TrainerCardHasBadge 0x8008, 3
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14CF
    // "Nimbasa City Pokémon Gym[f000]븁\u0000\nLeader: Elesa\nCertified Trainers:"
    InfoMsg NimbasaCityGym_Text_NimbasaCityPokemonGym, 2
    VMJump L_14F2

L_14CF:
    VMStackPushFlag EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14ED
    // "Nimbasa City Pokémon Gym[f000]븁\u0000\nLeader: Elesa\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000"
    InfoMsg NimbasaCityGym_Text_NimbasaCityPokemonGym_2, 2
    VMJump L_14F2

L_14ED:
    // "Nimbasa City Pokémon Gym[f000]븁\u0000\nLeader: Elesa\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg NimbasaCityGym_Text_NimbasaCityPokemonGym_3, 2

L_14F2:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
