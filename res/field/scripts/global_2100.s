#include "asm/field_script.inc"
#include "text/script/global_2100.h"

// Script plugin 13, from the zones that start its scripts

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
    ScriptEntriesEnd

Script_3:
    Cmd_02B2 0, EVENT_WORK_0x400f
    DebugPrint 80
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x404c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0063
    DebugPrint 99
    FlagReset EVENT_FLAG_0x03dd

L_0063:
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    DebugPrint 40
    FlagSet EVENT_FLAG_0x03dd

L_007E:
    WorkSetConst EVENT_WORK_0x400f, 0
    VMHalt

Script_4:
    VMHalt

Script_5:
    Cmd_02B2 0, EVENT_WORK_0x400f
    VMStackPush EVENT_WORK_0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0310
    DebugPrint 30
    RTGetZoneID EVENT_WORK_0x400c
    VMStackPushFlag EVENT_FLAG_0x03dd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0300
    WorkCmpConst EVENT_WORK_0x400c, 8
    VMJumpIf CMP_EQ, L_00CF
    VMJump L_00D9

L_00CF:
    ActorDelete 11
    VMJump L_0300

L_00D9:
    WorkCmpConst EVENT_WORK_0x400c, 20
    VMJumpIf CMP_EQ, L_00EC
    VMJump L_00F6

L_00EC:
    ActorDelete 11
    VMJump L_0300

L_00F6:
    WorkCmpConst EVENT_WORK_0x400c, 41
    VMJumpIf CMP_EQ, L_0109
    VMJump L_0113

L_0109:
    ActorDelete 12
    VMJump L_0300

L_0113:
    WorkCmpConst EVENT_WORK_0x400c, 65
    VMJumpIf CMP_EQ, L_0126
    VMJump L_0130

L_0126:
    ActorDelete 11
    VMJump L_0300

L_0130:
    WorkCmpConst EVENT_WORK_0x400c, 99
    VMJumpIf CMP_EQ, L_0143
    VMJump L_014D

L_0143:
    ActorDelete 10
    VMJump L_0300

L_014D:
    WorkCmpConst EVENT_WORK_0x400c, 109
    VMJumpIf CMP_EQ, L_0160
    VMJump L_016A

L_0160:
    ActorDelete 11
    VMJump L_0300

L_016A:
    WorkCmpConst EVENT_WORK_0x400c, 115
    VMJumpIf CMP_EQ, L_017D
    VMJump L_0187

L_017D:
    ActorDelete 11
    VMJump L_0300

L_0187:
    WorkCmpConst EVENT_WORK_0x400c, 122
    VMJumpIf CMP_EQ, L_019A
    VMJump L_01A4

L_019A:
    ActorDelete 12
    VMJump L_0300

L_01A4:
    WorkCmpConst EVENT_WORK_0x400c, 146
    VMJumpIf CMP_EQ, L_01B7
    VMJump L_01C1

L_01B7:
    ActorDelete 6
    VMJump L_0300

L_01C1:
    WorkCmpConst EVENT_WORK_0x400c, 1
    VMJumpIf CMP_EQ, L_01D4
    VMJump L_01DE

L_01D4:
    ActorDelete 9
    VMJump L_0300

L_01DE:
    WorkCmpConst EVENT_WORK_0x400c, 425
    VMJumpIf CMP_EQ, L_01F1
    VMJump L_01FB

L_01F1:
    ActorDelete 9
    VMJump L_0300

L_01FB:
    WorkCmpConst EVENT_WORK_0x400c, 435
    VMJumpIf CMP_EQ, L_020E
    VMJump L_0218

L_020E:
    ActorDelete 11
    VMJump L_0300

L_0218:
    WorkCmpConst EVENT_WORK_0x400c, 454
    VMJumpIf CMP_EQ, L_022B
    VMJump L_0235

L_022B:
    ActorDelete 11
    VMJump L_0300

L_0235:
    WorkCmpConst EVENT_WORK_0x400c, 472
    VMJumpIf CMP_EQ, L_0248
    VMJump L_0252

L_0248:
    ActorDelete 10
    VMJump L_0300

L_0252:
    WorkCmpConst EVENT_WORK_0x400c, 398
    VMJumpIf CMP_EQ, L_0265
    VMJump L_026F

L_0265:
    ActorDelete 10
    VMJump L_0300

L_026F:
    WorkCmpConst EVENT_WORK_0x400c, 407
    VMJumpIf CMP_EQ, L_0282
    VMJump L_028C

L_0282:
    ActorDelete 9
    VMJump L_0300

L_028C:
    WorkCmpConst EVENT_WORK_0x400c, 413
    VMJumpIf CMP_EQ, L_029F
    VMJump L_02A9

L_029F:
    ActorDelete 11
    VMJump L_0300

L_02A9:
    WorkCmpConst EVENT_WORK_0x400c, 443
    VMJumpIf CMP_EQ, L_02BC
    VMJump L_02C6

L_02BC:
    ActorDelete 11
    VMJump L_0300

L_02C6:
    WorkCmpConst EVENT_WORK_0x400c, 460
    VMJumpIf CMP_EQ, L_02D9
    VMJump L_02E3

L_02D9:
    ActorDelete 11
    VMJump L_0300

L_02E3:
    WorkCmpConst EVENT_WORK_0x400c, 602
    VMJumpIf CMP_EQ, L_02F6
    VMJump L_0300

L_02F6:
    ActorDelete 6
    VMJump L_0300

L_0300:
    FlagSet EVENT_FLAG_0x03dd
    WorkSetConst EVENT_WORK_0x400f, 0
    WorkSetConst EVENT_WORK_0x400c, 0

L_0310:
    VMHalt
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Cmd_00CE 0x8021
    TrainerCardGetBirthDate 0x8025, 0x8026
    RTCGetDate 0x8023, 0x8024
    VMStackPush 0x8023
    VMStackPush 0x8025
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPush 0x8026
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0389
    VMCall L_05E8
    VMJump L_03D8

L_0389:
    VMStackPush 0x8021
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_03A8
    VMCall L_04E3
    VMJump L_03D8

L_03A8:
    VMStackPushFlag EVENT_FLAG_0x0064
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CB
    VMCall L_05B8
    FlagSet EVENT_FLAG_0x0064
    VMJump L_03D8

L_03CB:
    WordSetPlayerName 0
    // "Great to see you, [f000]Ā\u0001\u0000!\nYou want the usual, right?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_GreatSeeWantUsual, 0, 0

L_03D8:
    YesNoWin 0x8022
    ActorMsgClose
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A7
    VMStackPush EVENT_WORK_0x4078
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_040A
    WorkSetConst EVENT_WORK_0x4078, 2

L_040A:
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_05A0
    ActorCmdWait
    PlayerSetSpecialSequence 8
    ActorCmdExec 255, Movement_05A8
    ActorCmdWait
    // "OK, I'll take your Pokémon for\na few seconds."
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_OkIllTakePokemon, 0, 0
    VMCall L_0625
    PokePartyRecoverAll
    RecordAdd 11, 1
    ActorMsgClose
    PlayerSetSpecialSequence 64
    ActorCmdExec 255, Movement_05B0
    ActorCmdWait
    PlayerSetSpecialSequence 8
    PokePartyCheckPokerus 0x8020
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0065
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0491
    FlagSet EVENT_FLAG_0x0065
    // "Oh... It looks like your Pokémon may be\ninfected with the Pokérus.[f000]븁\u0000\nLittle is known about the Pokérus,\nexcept that it is a microscopic life-form[f000]븀\u0000\nthat attaches to Pokémon.[f000]븁\u0000\nWhile infected, Pokémon are said to\ngrow exceptionally well."
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_OhLooksLikePokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04A1

L_0491:
    // "Thank you for waiting.[f000]븁\u0000\nWe've restored your Pokémon\nto full health.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_ThankWaitingWeveRestored, 0, 0
    VMCall L_0578

L_04A1:
    VMJump L_04AD

L_04A7:
    VMCall L_0578

L_04AD:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0

L_04E3:
    WorkSetConst 0x8028, 0
    RTCGetDayPart 0x8028
    WorkCmpConst 0x8028, 0
    VMJumpIf CMP_EQ, L_0500
    VMJump L_0510

L_0500:
    // "Good morning! Welcome to\nthe Pokémon Center.[f000]븁\u0000\nWe restore your tired Pokémon\nto full health.[f000]븁\u0000\nWould you like to rest your Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_GoodMorningWelcomePokemon, 0, 0
    VMJump L_0570

L_0510:
    WorkCmpConst 0x8028, 1
    VMJumpIf CMP_EQ, L_0523
    VMJump L_0533

L_0523:
    // "Hello, and welcome to\nthe Pokémon Center.[f000]븁\u0000\nWe restore your tired Pokémon\nto full health.[f000]븁\u0000\nWould you like to rest your Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_HelloWelcomePokemonCenter_2, 0, 0
    VMJump L_0570

L_0533:
    WorkCmpConst 0x8028, 2
    VMJumpIf CMP_EQ, L_0560
    WorkCmpConst 0x8028, 3
    VMJumpIf CMP_EQ, L_0560
    WorkCmpConst 0x8028, 4
    VMJumpIf CMP_EQ, L_0560
    VMJump L_0570

L_0560:
    // "Hello, and welcome to\nthe Pokémon Center.[f000]븁\u0000\nWe restore your tired Pokémon\nto full health.[f000]븁\u0000\nWould you like to rest your Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_HelloWelcomePokemonCenter, 0, 0
    VMJump L_0570

L_0570:
    WorkSetConst 0x8028, 0
    VMReturn

L_0578:
    ActorCmdExec 0x8011, Movement_0594
    ActorCmdWait
    // "We hope to see you again!"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_WeHopeSeeAgain, 0, 0
    LastKeyWait
    ActorMsgClose
    VMReturn
    .balign 4, 0

Movement_0594:
    Move 100, 1
    Move 62, 1
    MoveEnd

Movement_05A0:
    Move 102, 1
    MoveEnd

Movement_05A8:
    Move 0, 1
    MoveEnd

Movement_05B0:
    Move 104, 1
    MoveEnd

L_05B8:
    // "Hello, and welcome to\nthe Pokémon Center.[f000]븁\u0000\nWe restore your tired Pokémon\nto full health.[f000]븁\u0000\nWould you like to...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_HelloWelcomePokemonCenter_3, 0, 0
    ActorCmdExec 0x8011, Movement_05E0
    ActorCmdWait
    ActorMsgClose
    WordSetPlayerName 0
    // "Th-that Trainer Card!\nThat wonderful shade! That sparkle![f000]븁\u0000\nI've seen several Trainers with\nSilver Trainer Cards already...[f000]븁\u0000\nBut you're the first to top them all with\nthat impressive Trainer Card.[f000]븁\u0000\nOh, [f000]Ā\u0001\u0000, may I please heal\nyour Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_ThTrainerCardWonderful, 0, 0
    VMReturn
    .balign 4, 0

Movement_05E0:
    Move 75, 1
    MoveEnd

L_05E8:
    // "Welcome to the Pokémon Center.[f000]븁\u0000\nHey! Is today your birthday?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_WelcomePokemonCenterHey, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0619
    // "Happy birthday![f000]븁\u0000\nPlease keep visiting the Pokémon Center\nfor many years to come.[f000]븁\u0000\nNow, would you like to rest\nyour Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_HappyBirthdayPleaseKeep, 0, 0
    VMJump L_0623

L_0619:
    // "It isn't? Oh, I must have been confused.[f000]븁\u0000\nWould you like to rest your Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_IsntOhMustHave, 0, 0

L_0623:
    VMReturn

L_0625:
    WorkSetConst 0x8029, 0
    ActorCmdExec 0x8011, Movement_0654
    ActorCmdWait
    PokePartyGetCount 0x8029, 1
    PokecenPlayHealingSequence 0x8029
    ActorCmdExec 0x8011, Movement_065C
    ActorCmdWait
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x29
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .balign 4, 0

Movement_0654:
    Move 0, 1
    MoveEnd

Movement_065C:
    Move 1, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    FlagGet EVENT_FLAG_0x006a, 0x802a
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    PokePartyGetCount 0x802d, 4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_06C1
    // "I am sorry...\nYou can't enter the Union Room yet.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_AmSorryCantEnter, 0x8011, 4, 0

L_06C1:
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_06E0
    // "You have at least one Pokémon\nthat can't be taken in.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_HaveLeastOnePokemon, 0x8011, 4, 0

L_06E0:
    VMStackPush 0x802a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0797
    // "Welcome to the Pokémon Wireless\nClub Union Room.[f000]븁\u0000\nYou may interact directly with\nmany Trainers here.[f000]븁\u0000\nWould you like to enter the room?"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_WelcomePokemonWirelessClub, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32814
    ListMenuAdd 18, 65535, 18
    ListMenuAdd 20, 65535, 20
    ListMenuAdd 19, 65535, 19
    ListMenuShow
    WorkCmpConst 0x802e, 18
    VMJumpIf CMP_EQ, L_0745
    VMJump L_0759

L_0745:
    WorkSetConst 0x802b, 1
    Cmd_01DD 2, 0, 0
    VMJump L_0797

L_0759:
    WorkCmpConst 0x802e, 19
    VMJumpIf CMP_EQ, L_076C
    VMJump L_0778

L_076C:
    WorkSetConst 0x802b, 0
    VMJump L_0797

L_0778:
    WorkCmpConst 0x802e, 20
    VMJumpIf CMP_EQ, L_078B
    VMJump L_0797

L_078B:
    WorkSetConst 0x802c, 1
    VMJump L_0797

L_0797:
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D3
    // "The Trainers in the Union Room\nwill be those players around you[f000]븀\u0000\nwho have also entered the room.[f000]븁\u0000\nYou may trade your Pokémon here\nor have battles for two or four.[f000]븁\u0000\nAlso, you may exchange Eggs\nor draw pictures with other players.[f000]븁\u0000\nYou may also chat with other\nplayers in the room.[f000]븁\u0000\nOr you may locate friends in the room\nby touching their spoken words.[f000]븁\u0000\nWould you like to enter the room?"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_TrainersUnionRoomWill, 0x8011, 4, 0
    YesNoWin 0x802e
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07D3
    WorkSetConst 0x802b, 1

L_07D3:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0809
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0809
    ActorMsgClose
    RTCallGlobal 2005
    WorkSetConst 0x802b, 0

L_0809:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0846
    TimeSigCmd_00E5 0x8008, 0x8009
    VMStackPush 0x8008
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0846
    WordSetTrainerClassName 0, 0x8009
    // "Wait, Trainer![f000]븁\u0000\nYour title on your Trainer Card\nis still just “Pokémon Trainer,\" isn't it?[f000]븁\u0000\nIf you change your title, you can easily\nproject your image to others when you[f000]븀\u0000\ngreet them in the Union Room.[f000]븁\u0000\nLet's see...\nHow about [f000]Ď\u0001\u0000?[f000]븀\u0000\nWhat do you think?[f000]븁\u0000\nThat is the impression I got from you.\nFirst, let's change your title.[f000]븁\u0000\nIf you don't like the new one, you can\nchange it by selecting your Trainer Card.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_WaitTrainerTitleTrainer, 0x8011, 4, 0

L_0846:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0882
    // "DS Wireless Communications\nwill be launched."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_DsWirelessCommunicationsWill, 0x8011, 4, 0
    YesNoWin 0x802e
    VMStackPush 0x802e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0882
    WorkSetConst 0x802b, 0

L_0882:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08EA
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x802e, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x802e
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_08EA
    WorkSetConst 0x802b, 0

L_08EA:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0947
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x802e, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x802e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_092E
    WorkSetConst 0x802b, 0
    VMJump L_0947

L_092E:
    VMStackPush 0x802e
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0947
    WorkSetConst 0x802b, 0

L_0947:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_097E
    Cmd_02C5 16
    FunfestBGMReturn
    // "I hope you enjoy your time in\nthe Union Room.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_HopeEnjoyTimeUnion, 0x8011, 4, 0
    ActorMsgClose
    VMCall L_09CD
    PokePartyRecoverAll
    FieldSetNextZoneHere
    FlagSet EVENT_FLAG_CONTINUE_SCRIPT
    MapChangeUnionRoom

L_097E:
    VMStackPush 0x802b
    VMStackPushConst 1
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_09A1
    // "Please do visit again."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_PleaseVisitAgain, 0x8011, 4, 0
    LastKeyWait
    ActorMsgClose

L_09A1:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    VMCall L_09CD
    RTEndGlobal

L_09CD:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    WorkSetConst EVENT_WORK_CONTINUE_SCRIPT, 1
    PlayerGetGPos 0x802f, 0x8030
    WorkSub 0x8030, 3
    BMCreateHandleByGPos 0x8031, 5, 0x802f, 0x8030
    ActorCmdExec 0x8011, Movement_0A94
    VMSleep 16
    ActorCmdExec 255, Movement_0AA4
    ActorCmdWait
    SEPlay SEQ_SE_FLD_22
    BMHndAudioVisualAnmPlay 0x8031, 0
    BMHndAnmWait 0x8031
    SEWait
    ActorCmdExec 255, Movement_0AAC
    ActorCmdWait
    SEPlay SEQ_SE_FLD_22
    BMHndAudioVisualAnmPlay 0x8031, 1
    BMHndAnmWait 0x8031
    SEWait
    BMReleaseHandle 0x8031
    EvCameraInit
    EvCameraUnbind
    BMCreateHandleByGPos 0x8032, 6, 0x802f, 0x8030
    SEPlay SEQ_SE_FLD_23
    BMHndAudioVisualAnmPlay 0x8032, 1
    PlayerMoveToYAsync_ 0, 40, 64, 0
    BMHndAnmWait 0x8032
    FadeOutBlackQ
    FadeWait
    SEStop
    BMReleaseHandle 0x8032
    EvCameraRebind
    EvCameraEnd
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x32
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x31
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x30
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x2f
    .byte 0x80
    .balign 4, 0

Movement_0A94:
    Move 12, 1
    Move 14, 1
    Move 3, 1
    MoveEnd

Movement_0AA4:
    Move 12, 2
    MoveEnd

Movement_0AAC:
    Move 12, 2
    Move 1, 1
    MoveEnd
    WorkSetConst 0x8033, 0

Script_7:
    WorkSetConst 0x8033, 0
    VMCall L_0ADA
    RTEndGlobal

Script_8:
    WorkSetConst 0x8033, 1
    VMCall L_0ADA
    RTEndGlobal

L_0ADA:
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst EVENT_WORK_CONTINUE_SCRIPT, 0
    FlagReset EVENT_FLAG_CONTINUE_SCRIPT
    PlayerGetGPos 0x8034, 0x8035
    WorkAdd 0x8035, 3
    ActorFindByGPos 0x8036, 0x8010, 0x8034, 3, 0x8035
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B51
    WorkSub 0x8034, 1
    WorkSub 0x8035, 1
    ActorSetGPos 0x8036, 0x8034, 3, 0x8035, 3
    WorkGet 0x8011, 0x8036

L_0B51:
    EvCameraInit
    EvCameraUnbind
    ActorCmdExec 255, Movement_0C50
    ActorCmdWait
    PlayerGetGPos 0x8034, 0x8035
    BMCreateHandleByGPos 0x8037, 6, 0x8034, 0x8035
    BMHndAudioVisualAnmPlay 0x8037, 0
    BMHndAnmPause 0x8037
    VMStackPush 0x8033
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B94
    CallSeasonBanner
    VMJump L_0B96

L_0B94:
    FadeInBlackQ_

L_0B96:
    FadeWait
    SEPlay SEQ_SE_FLD_23
    BMHndAudioVisualAnmPlay 0x8037, 0
    PlayerMoveToYAsync_ 1, 40, 64, 0
    VMSleep 2
    ActorCmdExec 255, Movement_0C58
    ActorCmdWait
    BMHndAnmWait 0x8037
    SEStop
    BMReleaseHandle 0x8037
    BMCreateHandleByGPos 0x8038, 5, 0x8034, 0x8035
    SEPlay SEQ_SE_FLD_22
    BMHndAudioVisualAnmPlay 0x8038, 0
    BMHndAnmWait 0x8038
    SEWait
    EvCameraRebind
    EvCameraEnd
    ActorCmdExec 255, Movement_0C70
    ActorCmdWait
    ActorCmdExec 0x8011, Movement_0C64
    ActorCmdWait
    SEPlay SEQ_SE_FLD_22
    BMHndAudioVisualAnmPlay 0x8038, 1
    BMHndAnmWait 0x8038
    SEWait
    BMReleaseHandle 0x8038
    Plugin13_Cmd1005 4, 0x8039
    VMStackPush 0x8039
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C29
    VMCall L_100F

L_0C29:
    VMReturn
    .byte 0x28
    .byte 0x00
    .byte 0x39
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x38
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x37
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x36
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x35
    .byte 0x80
    .byte 0x00
    .byte 0x00
    .byte 0x28
    .byte 0x00
    .byte 0x34
    .byte 0x80
    .balign 4, 0

Movement_0C50:
    Move 69, 1
    MoveEnd

Movement_0C58:
    Move 70, 1
    Move 1, 1
    MoveEnd

Movement_0C64:
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_0C70:
    Move 13, 4
    MoveEnd

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x006a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0C99
    CallGeonet
    VMJump L_0CA2

L_0C99:
    // "It seems you can't use it yet."
    InfoMsg Global2100_Text_SeemsCantUseYet, 2
    LastKeyWait
    InfoMsgClose_0039

L_0CA2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    VMStackPush EVENT_WORK_0x404c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CED
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh?\nYou're...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_OhYoure, 0, 0
    MsgWinCloseAll
    RTGetZoneID EVENT_WORK_0x4192
    FadeOutBlack
    RTReserveScript 1
    FadeWait
    MapChangeCore ZONE_CHARGESTONE_CAVE_5, 9, 0, 19, 1
    VMJump L_0D01

L_0CED:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ParentActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 0, 0
    LastKeyWait
    ActorMsgClose

L_0D01:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    WorkCmpConst EVENT_WORK_0x4192, 8
    VMJumpIf CMP_EQ, L_0D20
    VMJump L_0D32

L_0D20:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0D32:
    WorkCmpConst EVENT_WORK_0x4192, 20
    VMJumpIf CMP_EQ, L_0D45
    VMJump L_0D57

L_0D45:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0D57:
    WorkCmpConst EVENT_WORK_0x4192, 41
    VMJumpIf CMP_EQ, L_0D6A
    VMJump L_0D7C

L_0D6A:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 12, 0, 0
    VMJump L_0FF1

L_0D7C:
    WorkCmpConst EVENT_WORK_0x4192, 65
    VMJumpIf CMP_EQ, L_0D8F
    VMJump L_0DA1

L_0D8F:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0DA1:
    WorkCmpConst EVENT_WORK_0x4192, 99
    VMJumpIf CMP_EQ, L_0DB4
    VMJump L_0DC6

L_0DB4:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 10, 0, 0
    VMJump L_0FF1

L_0DC6:
    WorkCmpConst EVENT_WORK_0x4192, 109
    VMJumpIf CMP_EQ, L_0DD9
    VMJump L_0DEB

L_0DD9:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0DEB:
    WorkCmpConst EVENT_WORK_0x4192, 115
    VMJumpIf CMP_EQ, L_0DFE
    VMJump L_0E10

L_0DFE:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0E10:
    WorkCmpConst EVENT_WORK_0x4192, 122
    VMJumpIf CMP_EQ, L_0E23
    VMJump L_0E35

L_0E23:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 12, 0, 0
    VMJump L_0FF1

L_0E35:
    WorkCmpConst EVENT_WORK_0x4192, 146
    VMJumpIf CMP_EQ, L_0E48
    VMJump L_0E5A

L_0E48:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 6, 0, 0
    VMJump L_0FF1

L_0E5A:
    WorkCmpConst EVENT_WORK_0x4192, 1
    VMJumpIf CMP_EQ, L_0E6D
    VMJump L_0E7F

L_0E6D:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 9, 0, 0
    VMJump L_0FF1

L_0E7F:
    WorkCmpConst EVENT_WORK_0x4192, 425
    VMJumpIf CMP_EQ, L_0E92
    VMJump L_0EA4

L_0E92:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 9, 0, 0
    VMJump L_0FF1

L_0EA4:
    WorkCmpConst EVENT_WORK_0x4192, 435
    VMJumpIf CMP_EQ, L_0EB7
    VMJump L_0EC9

L_0EB7:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0EC9:
    WorkCmpConst EVENT_WORK_0x4192, 454
    VMJumpIf CMP_EQ, L_0EDC
    VMJump L_0EEE

L_0EDC:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0EEE:
    WorkCmpConst EVENT_WORK_0x4192, 472
    VMJumpIf CMP_EQ, L_0F01
    VMJump L_0F13

L_0F01:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 10, 0, 0
    VMJump L_0FF1

L_0F13:
    WorkCmpConst EVENT_WORK_0x4192, 398
    VMJumpIf CMP_EQ, L_0F26
    VMJump L_0F38

L_0F26:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 10, 0, 0
    VMJump L_0FF1

L_0F38:
    WorkCmpConst EVENT_WORK_0x4192, 407
    VMJumpIf CMP_EQ, L_0F4B
    VMJump L_0F5D

L_0F4B:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 9, 0, 0
    VMJump L_0FF1

L_0F5D:
    WorkCmpConst EVENT_WORK_0x4192, 413
    VMJumpIf CMP_EQ, L_0F70
    VMJump L_0F82

L_0F70:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0F82:
    WorkCmpConst EVENT_WORK_0x4192, 443
    VMJumpIf CMP_EQ, L_0F95
    VMJump L_0FA7

L_0F95:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0FA7:
    WorkCmpConst EVENT_WORK_0x4192, 460
    VMJumpIf CMP_EQ, L_0FBA
    VMJump L_0FCC

L_0FBA:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 11, 0, 0
    VMJump L_0FF1

L_0FCC:
    WorkCmpConst EVENT_WORK_0x4192, 602
    VMJumpIf CMP_EQ, L_0FDF
    VMJump L_0FF1

L_0FDF:
    // "Sorry...[f000]븁\u0000\nWhen I saw you,\nI just started talking for some reason."
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_SorryWhenSawJust, 6, 0, 0
    VMJump L_0FF1

L_0FF1:
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x404c, 1
    WorkSetConst EVENT_WORK_0x4192, 0
    FlagSet EVENT_FLAG_0x03dd
    FlagSet EVENT_FLAG_0x098a
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_100F:
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    Plugin13_Cmd1005 0, 0x803a
    ActorCmdExec 255, Movement_10D0
    ActorCmdWait
    VMStackPush 0x803a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1052
    // "I'm sorry![f000]븁\u0000\nYour system's power was turned\noff during a Random Matchup![f000]븁\u0000\nYou will not be able to participate\nin a Random Matchup[f000]븀\u0000\nor Wi-Fi Competition for an hour.[f000]븁\u0000\nPlease come back later.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global2100_Text_ImSorrySystemsPower, 0x8011, 4, 0
    ActorMsgClose
    VMJump L_108F

L_1052:
    Plugin13_Cmd1005 3, 0x803b
    VMStackPush 0x803b
    VMStackPushConst 0
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_108F
    Plugin13_Cmd1009 0x803b
    WordSetPlayerName 0
    // "In honor of your achievement in battle,\nI present you with these Battle Points![f000]븁\u0000"
    SystemMsg Global2100_Text_HonorAchievementBattlePresent, 2
    WordSetNumber 1, 0x803b, 2
    MEPlay SEQ_ME_BPGET
    // "[f000]Ā\u0001\u0000 received [f000]ȁ\u0001\u0001 BP!"
    SystemMsg Global2100_Text_ReceivedBp, 2
    MEWait
    MsgWaitAdvance
    InfoMsgClose

L_108F:
    Plugin13_Cmd1008 4, 0
    // "Saving...\nDon't turn off the power."
    SystemMsg Global2100_Text_SavingDontTurnOff, 2
    VMSleep 1
    MsgSetLoadingSpinner 0
    SaveDataWrite 0x803b
    VMStackPush 0x803a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_10C0
    Cmd_02ED 1, 0

L_10C0:
    MsgWinCloseAll
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    VMReturn

Movement_10D0:
    Move 0, 1
    MoveEnd
