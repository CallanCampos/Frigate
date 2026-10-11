#include "asm/field_script.inc"
#include "text/script/mistralton_city_gym.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_2:
    FlagSet EVENT_FLAG_0x01f0
    VMHalt

Script_3:
    VMStackPush EVENT_WORK_0x409f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0049
    ActorSetGPos 5, 11, 0, 37, 1
    VMJump L_0049

L_0049:
    Gym0601FanAmbienceStart
    VMHalt

Script_4:
    Gym0601FanAmbienceStart
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 5
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    VMCall L_00F6
    VMJump L_00F0

L_007E:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x40cb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00B7
    // "I wonder what Professor Juniper\nis up to?[f000]븁\u0000\nI did promise her a ride in my plane..."
    ActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_WonderWhatProfessorJuniper, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00F0

L_00B7:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    // "Two things that are both really fun:\nflying my own plane and having my[f000]븀\u0000\nPokémon take me places using Fly!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_TwoThingsBothReally, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00F0

L_00E0:
    // "Skyla: Are you and your Pokémon well?\nOur battle together was a ton of fun.[f000]븁\u0000\nWhenever my Pokémon think of our battle,\nthey want to start training again.[f000]븁\u0000\nIt might be a cool idea to take my plane\nand go on a training trip together!"
    ActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_SkylaPokemonWellOur, 3, 0, 0
    LastKeyWait
    ActorMsgClose

L_00F0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00F6:
    // "Hee-hee!\nI've been waiting for you.[f000]븁\u0000\nYou're a tough Trainer who can face the\nwind and not get blown off your feet![f000]븁\u0000\nI'm kinda excited about this battle!\nWhy don't you and I have some fun?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_HeeHeeIveBeen, 0, 0
    ActorMsgClose
    WorkSetConst 0x8020, 0
    GameGetDifficulty 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012D
    CallTrainerBattle TRAINER_LEADER_SKYLA_2, 0, 0
    VMJump L_0135

L_012D:
    CallTrainerBattle TRAINER_LEADER_SKYLA, 0, 0

L_0135:
    WorkSetConst 0x8020, 0
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
    // "You're an amazing Pokémon Trainer.[f000]븁\u0000\nMy Pokémon and I are happy\nbecause for the first time in quite a[f000]븀\u0000\nwhile--about two years, I'd say--we[f000]븀\u0000\ncould fight with our full strength.[f000]븁\u0000\nThis is an official League Gym Badge.\nBut this is just a stepping-stone.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_YoureAmazingPokemonTrainer, 0, 0
    ActorMsgClose
    TrainerCardSaveGymVictoryParty 5
    TrainerCardAddBadge 5
    WordSetPlayerName 0
    MEPlay SEQ_ME_BADGE
    WorkSetConst 0x8021, 0
    TrainerCardGetSex 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019E
    PlayFieldEffect 8
    VMJump L_01A2

L_019E:
    PlayFieldEffect 60

L_01A2:
    MEWait
    WorkSetConst 0x8021, 0
    // "[f000]Ā\u0001\u0000 received the\nJet Badge from Skyla.[f000]븁\u0000"
    SystemMsg MistraltonCityGym_Text_ReceivedJetBadgeFrom, 0
    InfoMsgClose
    // "Wow, hot stuff![f000]븁\u0000\nWith that many Gym Badges,\nPokémon up to Lv. 70 will obey you.[f000]븁\u0000\nAlso, I want you to have this TM\nso that you'll always remember[f000]븀\u0000\nthis Pokémon battle.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_WowHotStuffMany, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 389
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "I wonder what Professor Juniper\nis up to?[f000]븁\u0000\nI did promise her a ride in my plane..."
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_WonderWhatProfessorJuniper, 0, 0
    LastKeyWait
    ActorMsgClose
    VMStackPush EVENT_WORK_0x40c2
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0211
    HollowRivalCmd_0262 0, 3
    HollowRivalCmd_0262 1, 18
    VMJump L_0217

L_0211:
    HollowRivalCmd_0262 1, 19

L_0217:
    TrainerFlagSet TRAINER_PILOT_FLYNN
    TrainerFlagSet TRAINER_PILOT_WINSLOW
    TrainerFlagSet TRAINER_PILOT_EWING
    TrainerFlagSet TRAINER_PILOT_CHASE
    TrainerFlagSet TRAINER_PILOT_ELRON
    FlagSet EVENT_FLAG_0x0973
    WorkSetConst EVENT_WORK_0x40c1, 1
    WorkAdd EVENT_WORK_0x40c2, 1
    VMReturn

Script_5:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x409f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0326
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    PlayerGetGPos 0x8022, 0x8023
    ActorSetGPos 5, 0x8022, 0, 37, 2
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    ActorCmdExec 255, Movement_032C
    ActorCmdWait
    Cmd_028E 5
    VMSleep 30
    ActorCmdExec 255, Movement_0334
    ActorCmdWait
    VMStackPushFlag EVENT_FLAG_0x0070
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DD
    // "Ow...[f000]븁\u0000\nOh, I'm sorry![f000]븁\u0000\nAllow me to apologize by giving you this\nFresh Water! Take it, please.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_OwOhImSorry, 5, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_0x0070

L_02DD:
    ActorWalkRoute 5, 12, 46, 1, 8, 0
    ActorCmdExec 5, Movement_033C
    ActorCmdWait
    ActorWalkRoute 255, 11, 46, 1, 8, 0
    ActorCmdExec 255, Movement_0344
    ActorCmdWait
    VMCall L_034C
    // "This Gym is a wind tunnel![f000]븁\u0000\nWhen the propellers in back start\nspinning quickly, you'll be blown[f000]븀\u0000\naway like a certain someone just was![f000]븁\u0000\nWhen you think the wind will blow,\nstay hidden behind a wall[f000]븀\u0000\nand wait for it to stop.[f000]븁\u0000\nBy the way, Flying-type Pokémon have\nmore weaknesses than you might expect,[f000]븀\u0000\nincluding to Rock-, Electric-, and[f000]븀\u0000\nIce-type moves![f000]븁\u0000"
    InfoMsg MistraltonCityGym_Text_GymWindTunnelWhen_2, 2
    MsgWinCloseAll
    VMCall L_036A
    WorkSetConst EVENT_WORK_0x409f, 1

L_0326:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_032C:
    Move 75, 1
    MoveEnd

Movement_0334:
    Move 12, 1
    MoveEnd

Movement_033C:
    Move 2, 1
    MoveEnd

Movement_0344:
    Move 3, 1
    MoveEnd

L_034C:
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 4056, 0, 0xed000, 0xb8000, 0, 0x143000, 90
    VMReturn

L_036A:
    EvCameraWait
    EvCameraMoveToDefault 60
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMReturn

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    TrainerCardHasBadge 0x8008, 5
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03AD
    // "This Gym is a wind tunnel![f000]븁\u0000\nWhen the propellers in back start\nspinning quickly, you'll be blown[f000]븀\u0000\naway like a certain someone just was![f000]븁\u0000\nWhen you think the wind will blow,\nstay hidden behind a wall[f000]븀\u0000\nand wait for it to stop.[f000]븁\u0000\nBy the way, Flying-type Pokémon have\nmore weaknesses than you might expect,[f000]븀\u0000\nincluding to Rock-, Electric-, and[f000]븀\u0000\nIce-type moves!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_GymWindTunnelWhen, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_03BB

L_03AD:
    // "Letting yourself be blown away is almost\nlike being hit by a Pokémon move![f000]븀\u0000\nIt's kinda fun![f000]븁\u0000\nOh, yeah! Congrats on defeating\nthe Gym Leader!"
    ParentActorMsg MSGFILE_SCRIPT, MistraltonCityGym_Text_LettingYourselfBlownAway, 0, 0
    LastKeyWait
    ActorMsgClose

L_03BB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x8024, 0
    TrainerCardHasBadge 0x8008, 5
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03F4
    WordSetPlayerName 0
    // "Mistralton Pokémon Gym[f000]븁\u0000\nGym Leader: Skyla\nCertified Trainers:"
    InfoMsg MistraltonCityGym_Text_MistraltonPokemonGymGym, 2
    VMJump L_0420

L_03F4:
    VMStackPushFlag EVENT_FLAG_ARRIVED_UNDELLA_TOWN
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0415
    WordSetPlayerName 0
    // "Mistralton Pokémon Gym[f000]븁\u0000\nGym Leader: Skyla\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000"
    InfoMsg MistraltonCityGym_Text_MistraltonPokemonGymGym_2, 2
    VMJump L_0420

L_0415:
    WordSetLoadRivalName 1
    WordSetPlayerName 0
    // "Mistralton Pokémon Gym[f000]븁\u0000\nGym Leader: Skyla\nCertified Trainers:[f000]븀\u0000\n[f000]Ā\u0001\u0000, [f000]Ā\u0001\u0001"
    InfoMsg MistraltonCityGym_Text_MistraltonPokemonGymGym_3, 2

L_0420:
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
