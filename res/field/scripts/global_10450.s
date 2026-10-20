#include "asm/field_script.inc"
#include "text/script/global_10450.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    Cmd_0167 0, 0, 0, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Pokémon Musical![f000]븁\u0000\nHere you can participate in\na musical alone.[f000]븁\u0000\nWould you like to participate?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WelcomePokemonMusicalHere, 0x8011, 2, 0
    VMCall L_0851
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010E
    VMCall L_0A74
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BA
    VMCall L_0816
    VMJump L_0108

L_00BA:
    VMCall L_0AF1
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00DF
    VMCall L_0816
    VMJump L_0108

L_00DF:
    // "Great! Please walk this way![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_GreatPleaseWalkWay, 0x8011, 2, 0
    ActorMsgClose
    Cmd_02C5 5
    FunfestBGMReturn
    VMCall L_0B9C
    SEPlay SEQ_SE_KAIDAN
    FadeOutBlackQ
    SEWait
    FadeWait
    MusicalCmd_0163 0x8021, 0

L_0108:
    VMJump L_0114

L_010E:
    VMCall L_0816

L_0114:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    Cmd_0167 0, 0, 0, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMCall L_0206
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8022, 1
    Cmd_0167 0, 0, 0, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Pokémon Musical![f000]븁\u0000\nThis is a changing room for Dress Up only.\nWould you like to Dress Up your Pokémon?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WelcomePokemonMusicalChanging, 0x8011, 2, 0
    VMCall L_0851
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D0
    VMCall L_0A74
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D0
    // "This way, please![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WayPlease, 0x8011, 2, 0
    ActorMsgClose
    FunfestBGMReturn
    VMCall L_0BC8
    SEPlay SEQ_SE_KAIDAN
    FadeOutBlackQ
    BGMPush 6
    FadeWait
    SEWait
    FieldClose
    MusicalCmd_0164 0x8021
    FieldOpen
    VMCall L_0BDE
    FadeInWhiteQ
    BGMPop 0, 60
    FadeWait

L_01D0:
    VMCall L_0816
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    MsgSetAutoscrolls 0
    Cmd_0167 21, 0, 0, 0x8010
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    MsgSetAutoscrolls 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0206:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    // "Welcome to the Pokémon Musical![f000]븁\u0000\nHere you and your friends can\nperform together![f000]븁\u0000\nWould you like to participate?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WelcomePokemonMusicalHere_2, 0x8011, 2, 0
    VMCall L_0851
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0245
    VMCall L_0816
    VMReturn

L_0245:
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_026A
    ActorMsgClose
    RTCallGlobal 2005
    VMCall L_0816
    VMReturn

L_026A:
    VMCall L_0A74
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_028B
    VMCall L_0816
    VMReturn

L_028B:
    // "Would you like to use\nInfrared Communication or[f000]븀\u0000\nDS Wireless Communications?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WouldLikeUseInfrared, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32805
    ListMenuAdd 47, 65535, 0
    ListMenuAdd 48, 65535, 1
    ListMenuAdd 40, 65535, 2
    ListMenuShow
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8025
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_02E5
    VMCall L_0816
    VMReturn

L_02E5:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_030A
    // "Would you like to launch\nInfrared Communication and[f000]븀\u0000\nDS Wireless Communications?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WouldLikeLaunchInfrared, 0x8011, 2, 0
    VMJump L_0316

L_030A:
    // "Launch DS Wireless Communications?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_LaunchDsWirelessCommunications, 0x8011, 2, 0

L_0316:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0335
    VMCall L_0816
    VMReturn

L_0335:
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8023, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_038C
    VMCall L_0816
    VMReturn

L_038C:
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x8023, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BD
    WorkSetConst 0x8023, 0
    VMJump L_03E2

L_03BD:
    VMStackPush 0x8023
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DC
    WorkSetConst 0x8023, 0
    VMJump L_03E2

L_03DC:
    WorkSetConst 0x8023, 1

L_03E2:
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FD
    VMCall L_0816
    VMReturn

L_03FD:
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 1

L_040F:
    VMStackPush 0x8026
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0653
    // "One of you will become the leader.\nThat person will get to choose the show[f000]븀\u0000\nyou'll perform![f000]븁\u0000\nThe other members should select\n“Join group.\""
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_OneWillBecomeLeader, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32804
    ListMenuAdd 39, 65535, 1
    ListMenuAdd 38, 65535, 0
    ListMenuAdd 40, 65535, 2
    ListMenuShow
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0482
    VMCall L_0816
    VMReturn
    VMJump L_064D

L_0482:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A1
    VMCall L_0AF1
    VMJump L_04A7

L_04A1:
    WorkSetConst 0x8010, 1

L_04A7:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064D
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0556
    ActorMsgClose
    Cmd_0167 10, 1, 0, 0
    Cmd_0167 16, 1, 0, 0
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0502
    VMCall L_06BD
    VMJump L_0508

L_0502:
    VMCall L_0707

L_0508:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0550
    Cmd_0167 11, 1, 0, 0
    // "Would you like to launch\nInfrared Communication and[f000]븀\u0000\nDS Wireless Communications?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WouldLikeLaunchInfrared, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0550
    VMCall L_0816
    VMReturn

L_0550:
    VMJump L_060F

L_0556:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_060F
    ActorMsgClose
    Cmd_0167 10, 0, 0, 0
    Cmd_0167 16, 0, 0, 0
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_059E
    VMCall L_0751
    VMJump L_05A4

L_059E:
    VMCall L_07BA

L_05A4:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05F2
    Cmd_0167 11, 0, 0, 0
    // "Launch DS Wireless Communications?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_LaunchDsWirelessCommunications, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05EC
    VMCall L_0816
    VMReturn

L_05EC:
    VMJump L_060F

L_05F2:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_060F
    Cmd_0167 11, 0, 0, 0

L_060F:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0628
    WorkSetConst 0x8026, 0

L_0628:
    Cmd_0167 22, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_064D
    VMCall L_0804
    VMReturn

L_064D:
    VMJump L_040F

L_0653:
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    Cmd_0167 21, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0682
    VMJump L_06BB

L_0682:
    // "Thank you for waiting.\nThis way, please![f000]븂\u0001<"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_ThankWaitingWayPlease, 0x8011, 2, 0
    ActorMsgClose
    Cmd_0167 14, 10, 0, 0
    Cmd_02C5 5
    FunfestBGMReturn
    VMCall L_0BB2
    SEPlay SEQ_SE_KAIDAN
    FadeOutBlackQ
    SEWait
    FadeWait
    MusicalCmd_0163 0x8021, 1
    VMCall L_0804

L_06BB:
    VMReturn

L_06BD:
    Cmd_0167 18, 0, 0, 0x8010
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_06DA
    VMJump L_06E6

L_06DA:
    WorkSetConst 0x8010, 1
    VMJump L_0705

L_06E6:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_06F9
    VMJump L_0705

L_06F9:
    WorkSetConst 0x8010, 0
    VMJump L_0705

L_0705:
    VMReturn

L_0707:
    Cmd_0167 18, 1, 0, 0x8010
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_0724
    VMJump L_0730

L_0724:
    WorkSetConst 0x8010, 1
    VMJump L_074F

L_0730:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_0743
    VMJump L_074F

L_0743:
    WorkSetConst 0x8010, 0
    VMJump L_074F

L_074F:
    VMReturn

L_0751:
    Cmd_0167 12, 0, 0, 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_076E
    VMJump L_077A

L_076E:
    WorkSetConst 0x8010, 1
    VMJump L_07B8

L_077A:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_078D
    VMJump L_0799

L_078D:
    WorkSetConst 0x8010, 0
    VMJump L_07B8

L_0799:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_07AC
    VMJump L_07B8

L_07AC:
    WorkSetConst 0x8010, 2
    VMJump L_07B8

L_07B8:
    VMReturn

L_07BA:
    Cmd_0167 13, 0, 0, 0x8010
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_07D7
    VMJump L_07E3

L_07D7:
    WorkSetConst 0x8010, 1
    VMJump L_0802

L_07E3:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_07F6
    VMJump L_0802

L_07F6:
    WorkSetConst 0x8010, 0
    VMJump L_0802

L_0802:
    VMReturn

L_0804:
    Cmd_0167 11, 0, 0, 0
    VMCall L_0816
    VMReturn

L_0816:
    Cmd_0167 22, 0, 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0843
    // "Please visit us again."
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_PleaseVisitUsAgain, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_0843:
    Cmd_0167 1, 0, 0, 0
    Cmd_013C
    VMReturn

L_0851:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 1

L_0863:
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A72
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 35, 65535, 0
    ListMenuAdd 36, 65535, 1
    ListMenuAdd 37, 65535, 2
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08BE
    WorkSetConst 0x8010, 1
    WorkSetConst 0x8028, 0
    VMJump L_0A6C

L_08BE:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_08F3
    WorkSetConst 0x8010, 0
    WorkSetConst 0x8028, 0
    VMJump L_0A6C

L_08F3:
    WorkSetConst 0x8029, 1

L_08F9:
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A6C
    // "What would you like me to explain?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WhatWouldLikeExplain, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 60, 65535, 0
    ListMenuAdd 62, 65535, 1
    ListMenuAdd 64, 65535, 2
    ListMenuAdd 66, 65535, 3
    ListMenuAdd 68, 65535, 4
    ListMenuAdd 40, 65535, 5
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_09B3
    WorkSetConst 0x8029, 0
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09A1
    // "Would you like to Dress Up your Pokémon?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WouldLikeDressUp, 0x8011, 2, 0
    VMJump L_09AD

L_09A1:
    // "Participate in the musical?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_ParticipateMusical, 0x8011, 2, 0

L_09AD:
    VMJump L_0A66

L_09B3:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09D8
    // "The Pokémon Musical is a show where\nPokémon wearing Props perform on stage.[f000]븀\u0000\nAnyone can participate![f000]븁\u0000\nWe encourage all Trainers to show the\nworld how charming their Pokémon are![f000]븁\u0000\nThe audience is looking forward to seeing\nhow you Dress Up your Pokémon, and how[f000]븀\u0000\nyour Pokémon perform![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_PokemonMusicalShowWhere, 0x8011, 2, 0
    VMJump L_0A66

L_09D8:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09FD
    // "You can join the musical from any of the\nthree reception areas.[f000]븁\u0000\nThe reception area in the center is for\nparticipating alone.[f000]븁\u0000\nWhen you participate alone, you will\nbe joined by other Trainers from around[f000]븀\u0000\nthe Unova region.[f000]븁\u0000\nIf you want to put on a musical with your\nfriends, go to the left reception area.[f000]븁\u0000\nYou'll be asked to pick a Leader who\nwill choose which show to perform,[f000]븀\u0000\nand the others will join the group.[f000]븁\u0000\nThe reception area to the right is where\nyou go to Dress Up only.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_CanJoinMusicalFrom, 0x8011, 2, 0
    VMJump L_0A66

L_09FD:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A22
    // "The Pokémon that will participate in the\nmusical are chosen from your party at[f000]븀\u0000\nthe reception area.[f000]븁\u0000\nBecause of their shape, some Pokémon\nhave a hard time wearing certain Props.[f000]븁\u0000\nYou might want to try Dress Up in the\nchanging room first if you're worried![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_PokemonWillParticipateMusical, 0x8011, 2, 0
    VMJump L_0A66

L_0A22:
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A47
    // "Oh, Dress Up is my favorite part! That's\nwhere you put the Props you've collected[f000]븀\u0000\non the Pokémon that will be performing.[f000]븁\u0000\nDepending on the Pokémon, you can put\nthe Props in different places.[f000]븁\u0000\nWhen you use Props that fit the theme of\nthe show you've chosen, the audience[f000]븀\u0000\nwill notice your Pokémon more![f000]븁\u0000\nSometimes a Prop that doesn't fit the\ntheme will also make your Pokémon[f000]븀\u0000\nstand out.[f000]븁\u0000\nIf you want to get an idea of how it's\ndone, you can watch how other people[f000]븀\u0000\nDress Up and then try it yourself![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_OhDressUpFavorite, 0x8011, 2, 0
    VMJump L_0A66

L_0A47:
    VMStackPush 0x8010
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A66
    // "You can choose which show to perform\nwhen you participate in the musical.[f000]븁\u0000\nThere is no need for Trainers to give\ncommands, but if your Pokémon is carrying[f000]븀\u0000\na Prop in its arms, it can use that Prop[f000]븀\u0000\nto show off and appeal to the audience.[f000]븁\u0000\nDepending on how you Dress Up your\nPokémon, the reactions from the audience[f000]븀\u0000\nwill change![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_CanChooseWhichShow, 0x8011, 2, 0

L_0A66:
    VMJump L_08F9

L_0A6C:
    VMJump L_0863

L_0A72:
    VMReturn

L_0A74:
    WorkSetConst 0x802a, 0
    MusicalCmd_0165 11, 0, 0x802a
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AA8
    // "I'm sorry, but you don't have an\neligible Pokémon in your party.[f000]븁\u0000\nPlease come back again with\ndifferent Pokémon![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_ImSorryButDont, 0x8011, 2, 0
    WorkSetConst 0x8010, 0
    VMReturn

L_0AA8:
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    // "Please choose the Pokémon that\nwill participate.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_PleaseChoosePokemonWill, 0x8011, 2, 0
    ActorMsgClose
    Cmd_016A 0x802b, 0x8021
    VMStackPush 0x802b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AE3
    WorkSetConst 0x8010, 0
    VMReturn

L_0AE3:
    WorkSetConst 0x802b, 0
    WorkSetConst 0x8010, 1
    VMReturn

L_0AF1:
    WorkSetConst 0x802c, 0
    MusicalCmd_0165 14, 0, 0x802c
    // "Which show would you like\nto participate in?"
    ActorMsg MSGFILE_SCRIPT, Global10450_Text_WhichShowWouldLike, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 29, 65535, 0
    ListMenuAdd 30, 65535, 1
    ListMenuAdd 31, 65535, 2
    ListMenuAdd 32, 65535, 3
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B54
    WordSetMusicalInfo 0, 0, 0
    ListMenuAdd 33, 65535, 4

L_0B54:
    ListMenuAdd 34, 65535, 5
    ListMenuShow
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0B8D
    WorkSetConst 0x8010, 0
    VMJump L_0B9A

L_0B8D:
    MusicalCmd_0165 5, 0x8010, 0
    WorkSetConst 0x8010, 1

L_0B9A:
    VMReturn

L_0B9C:
    ActorCmdExec 0, Movement_0BF8
    ActorCmdWait
    ActorCmdExec 255, Movement_0C28
    ActorCmdWait
    VMReturn

L_0BB2:
    ActorCmdExec 15, Movement_0C10
    ActorCmdWait
    ActorCmdExec 255, Movement_0C44
    ActorCmdWait
    VMReturn

L_0BC8:
    ActorCmdExec 16, Movement_0BF8
    ActorCmdWait
    ActorCmdExec 255, Movement_0C60
    ActorCmdWait
    VMReturn

L_0BDE:
    ActorSetGPos 16, 18, 0, 11, 1
    ActorSetGPos 255, 18, 0, 12, 0
    VMReturn

Movement_0BF8:
    Move 0, 1
    Move 12, 1
    Move 3, 1
    Move 15, 1
    Move 2, 1
    MoveEnd

Movement_0C10:
    Move 0, 1
    Move 12, 1
    Move 2, 1
    Move 14, 1
    Move 3, 1
    MoveEnd

Movement_0C28:
    Move 0, 1
    Move 12, 2
    Move 2, 1
    Move 14, 2
    Move 0, 1
    Move 12, 2
    MoveEnd

Movement_0C44:
    Move 0, 1
    Move 12, 2
    Move 3, 1
    Move 15, 2
    Move 0, 1
    Move 12, 2
    MoveEnd

Movement_0C60:
    Move 0, 1
    Move 12, 4
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Which show would you like\nto participate in?"
    ParentActorMsg MSGFILE_SCRIPT, Global10450_Text_WhichShowWouldLike, 0, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 29, 65535, 0
    ListMenuAdd 30, 65535, 1
    ListMenuAdd 31, 65535, 2
    ListMenuAdd 32, 65535, 3
    ListMenuAdd 33, 65535, 4
    ListMenuAdd 34, 65535, 5
    ListMenuShow
    ActorMsgClose
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0CE4
    VMJump L_0CEB

L_0CE4:
    MusicalCmd_0165 5, 0x8010, 0

L_0CEB:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0CF1:
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    MusicalCmd_0166 0x8020, 0, 0x802d
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0D3D
    // "In fact, this person does not exist..."
    ParentActorMsg MSGFILE_SCRIPT, Global10450_Text_FactPersonDoesNot, 0, 0
    ABKeyWait
    ActorMsgClose
    VMJump L_0DE2

L_0D3D:
    MusicalCmd_0166 0x8020, 3, 0x802f
    MusicalCmd_0166 0x8020, 4, 0x8030
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_0D5E
    VMJump L_0D7C

L_0D5E:
    MusicalCmd_0166 0x8020, 1, 0x802e
    WordSetPlayerName 0
    ParentActorMsg MSGFILE_SCRIPT, 0x802e, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0DE2

L_0D7C:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_0D8F
    VMJump L_0DC5

L_0D8F:
    MusicalCmd_0166 0x8020, 2, 0x802e
    WordSetPlayerName 0
    WordSetMusicalInfo 1, 1, 0x8030
    ParentActorMsg MSGFILE_SCRIPT, 0x802e, 0, 0
    ActorMsgClose
    WorkGet 0x8008, 0x8030
    WorkSetConst 0x8009, 0
    RTCallGlobal 10466
    MusicalCmd_0169 0x8020
    VMJump L_0DE2

L_0DC5:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_0DD8
    VMJump L_0DE2

L_0DD8:
    MusicalCmd_0169 0x8020
    VMJump L_0DE2

L_0DE2:
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0
    VMReturn

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8020, 5
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8020, 6
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8020, 7
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8020, 8
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8020, 9
    VMCall L_0CF1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WordSetPlayerName 0
    WordSetMusicalInfo 1, 1, 0x8008
    MEPlay SEQ_ME_ACCE
    // "Received the [f000][ff00]\u0001\u0002[f000]Ċ\u0001\u0001[f000][ff00]\u0001\u0000!"
    SystemMsg Global10450_Text_Received, 0
    MEWait
    MsgWaitAdvance
    VMStackPush 0x8009
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EFC
    // "[f000]Ā\u0001\u0000 put the [f000]Ċ\u0001\u0001\nin the [f000][ff00]\u0001\u0002Prop Case[f000][ff00]\u0001\u0000![f000]븁\u0000"
    SystemMsg Global10450_Text_PutPropCase_2, 0
    VMJump L_0F04

L_0EFC:
    // "[f000]Ā\u0001\u0000 put the [f000]Ċ\u0001\u0001\nin the [f000][ff00]\u0001\u0002Prop Case[f000][ff00]\u0001\u0000!"
    SystemMsg Global10450_Text_PutPropCase, 0
    LastKeyWait

L_0F04:
    InfoMsgClose
    MusicalCmd_0168 0x8008
    RTEndGlobal
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
