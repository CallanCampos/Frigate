#include "asm/field_script.inc"
#include "text/script/global_10655.h"

// Script plugin 6, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd

Script_5:
    WbtCmd_IsCreated 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003D
    VMCall L_0045
    VMCall L_0119

L_003D:
    VMCall L_00A9
    VMHalt

L_0045:
    Plugin6_Cmd1016 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0074
    WorkSetConst 0x8011, 0
    ActorSetGPos 0x8011, 15, 0, 5, 2
    VMJump L_0099

L_0074:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0099
    WorkSetConst 0x8011, 8
    ActorSetGPos 0x8011, 15, 0, 5, 3

L_0099:
    VMReturn

Script_6:
    FlagSet EVENT_FLAG_0x0367
    FlagSet EVENT_FLAG_0x0368
    FlagSet EVENT_FLAG_0x0369
    VMHalt

L_00A9:
    VMStackPushFlag EVENT_FLAG_0x0989
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BE
    VMReturn

L_00BE:
    Plugin6_Cmd1023 EVENT_WORK_0x4020, EVENT_WORK_0x4021, EVENT_WORK_0x4022
    VMStackPush EVENT_WORK_0x4020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00E1
    FlagReset EVENT_FLAG_0x0367
    ActorAdd 9

L_00E1:
    VMStackPush EVENT_WORK_0x4021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00FC
    FlagReset EVENT_FLAG_0x0368
    ActorAdd 10

L_00FC:
    VMStackPush EVENT_WORK_0x4022
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0117
    FlagReset EVENT_FLAG_0x0369
    ActorAdd 11

L_0117:
    VMReturn

L_0119:
    VMStackPushFlag EVENT_FLAG_0x0989
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012E
    VMReturn

L_012E:
    WbtCmd_GetTournament 0x8010
    VMStackPush 0x8010
    VMStackPushConst 11
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0147
    VMReturn

L_0147:
    WbtCmd_GetRound 0x8010
    VMStackPush 0x8010
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0164
    WorkSetConst EVENT_WORK_0x4135, 1

L_0164:
    VMReturn

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4135
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0197
    // "We're preparing a special tournament.\nPlease be patient a little longer."
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WerePreparingSpecialTournament, 0x8011, 4, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01A3

L_0197:
    WbtCmd_Create
    Plugin6_Cmd1015 1
    VMCall L_01EC

L_01A3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D6
    WbtCmd_Create
    Plugin6_Cmd1015 2
    VMCall L_01EC
    VMJump L_01E6

L_01D6:
    // "We're getting special tournaments ready.\nIt may take a while, so please be patient!"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WereGettingSpecialTournaments, 0x8011, 4, 0
    LastKeyWait
    MsgWinCloseAll

L_01E6:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01EC:
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WbtCmd_SetRound 0
    // "Welcome to the\nPokémon World Tournament![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WelcomePokemonWorldTournament, 0x8011, 4, 0
    WorkSetConst 0x8020, 1

L_023E:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMStackPush 0x8020
    VMStackPushConst 8
    VMStackCmp CMP_NE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0540
    DebugPrint 0x8020
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0278
    VMJump L_0290

L_0278:
    // "Will you participate?"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WillParticipate, 0x8011, 4, 0
    VMCall L_056D
    VMJump L_053A

L_0290:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_02A3
    VMJump L_02CE

L_02A3:
    WorkSetConst 0x8024, 0
    VMCall L_0B68
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C8
    WorkSetConst 0x8020, 1

L_02C8:
    VMJump L_053A

L_02CE:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_02E1
    VMJump L_037D

L_02E1:
    VMCall L_05D6
    WbtCmd_SetTournament 0x8023
    WorkGet 0x8021, 0x8023
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_0304
    VMJump L_0310

L_0304:
    WorkSetConst 0x8020, 1
    VMJump L_0377

L_0310:
    WorkCmpConst 0x8021, 11
    VMJumpIf CMP_EQ, L_0323
    VMJump L_0333

L_0323:
    WbtCmd_SetStyle 0
    WorkSetConst 0x8020, 7
    VMJump L_0377

L_0333:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_0346
    VMJump L_0352

L_0346:
    WorkSetConst 0x8020, 4
    VMJump L_0377

L_0352:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0365
    VMJump L_0371

L_0365:
    WorkSetConst 0x8020, 5
    VMJump L_0377

L_0371:
    WorkSetConst 0x8020, 6

L_0377:
    VMJump L_053A

L_037D:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_0390
    VMJump L_03CF

L_0390:
    WorkSetConst 0x8023, 17
    WbtCmd_SetType 17
    VMCall L_0DED
    VMStackPush 0x8023
    VMStackPushConst 17
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03BF
    WorkSetConst 0x8020, 3
    VMJump L_03C9

L_03BF:
    WbtCmd_SetType 0x8023
    WorkSetConst 0x8020, 6

L_03C9:
    VMJump L_053A

L_03CF:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_03E2
    VMJump L_043C

L_03E2:
    WorkSetConst 0x8022, 0
    VMCall L_0EAF
    Plugin6_Cmd1026 0x8022
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0411
    WorkSetConst 0x8020, 0
    VMJump L_0436

L_0411:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0430
    WorkSetConst 0x8020, 3
    VMJump L_0436

L_0430:
    WorkSetConst 0x8020, 7

L_0436:
    VMJump L_053A

L_043C:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_044F
    VMJump L_04B3

L_044F:
    WorkSetConst 0x8025, 4
    VMCall L_0807
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0484
    WorkGet 0x8025, 0x8023
    WbtCmd_SetStyle 0x8025
    WorkSetConst 0x8020, 7
    VMJump L_04AD

L_0484:
    WbtCmd_GetTournament 0x8021
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04A7
    WorkSetConst 0x8020, 4
    VMJump L_04AD

L_04A7:
    WorkSetConst 0x8020, 3

L_04AD:
    VMJump L_053A

L_04B3:
    WorkCmpConst 0x8020, 7
    VMJumpIf CMP_EQ, L_04C6
    VMJump L_0534

L_04C6:
    WorkSetConst 0x8027, 0
    VMCall L_0861
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04F1
    WorkSetConst 0x8020, 0
    VMJump L_052E

L_04F1:
    WorkSetConst 0x8027, 0
    VMCall L_0978
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0528
    Plugin6_Cmd1005
    WbtCmd_SetRound 1
    VMCall L_0A17
    WorkSetConst 0x8020, 8
    VMJump L_052E

L_0528:
    WorkSetConst 0x8020, 3

L_052E:
    VMJump L_053A

L_0534:
    WorkSetConst 0x8020, 0

L_053A:
    VMJump L_023E

L_0540:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_056B
    // "We hope to see you again!"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WeHopeSeeAgain, 0x8011, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WbtCmd_Free
    VMJump L_056B

L_056B:
    VMReturn

L_056D:
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 2, 65535, 0
    ListMenuAdd 3, 65535, 1
    ListMenuAdd 4, 65535, 2
    ListMenuShow
    WorkCmpConst 0x8023, 0
    VMJumpIf CMP_EQ, L_05A3
    VMJump L_05AF

L_05A3:
    WorkSetConst 0x8020, 3
    VMJump L_05D4

L_05AF:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_05C2
    VMJump L_05CE

L_05C2:
    WorkSetConst 0x8020, 2
    VMJump L_05D4

L_05CE:
    WorkSetConst 0x8020, 0

L_05D4:
    VMReturn

L_05D6:
    // "Which tournament\nwill you participate in?"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhichTournamentWillParticipate, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    VMCall L_0616
    ListMenuAdd 38, 65535, 0
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0614
    WorkSetConst 0x8023, 0

L_0614:
    VMReturn

L_0616:
    Plugin6_Cmd1006 11, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0637
    ListMenuAdd 26, 65535, 11

L_0637:
    Plugin6_Cmd1006 4, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0658
    ListMenuAdd 26, 65535, 4

L_0658:
    Plugin6_Cmd1006 5, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0679
    ListMenuAdd 27, 65535, 5

L_0679:
    Plugin6_Cmd1006 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_069A
    ListMenuAdd 28, 65535, 6

L_069A:
    Plugin6_Cmd1006 7, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06BB
    ListMenuAdd 29, 65535, 7

L_06BB:
    Plugin6_Cmd1006 8, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06DC
    ListMenuAdd 30, 65535, 8

L_06DC:
    Plugin6_Cmd1006 9, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06FD
    ListMenuAdd 31, 65535, 9

L_06FD:
    Plugin6_Cmd1006 10, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_071E
    ListMenuAdd 32, 65535, 10

L_071E:
    Plugin6_Cmd1006 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_073F
    ListMenuAdd 23, 65535, 1

L_073F:
    Plugin6_Cmd1006 13, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0760
    ListMenuAdd 35, 65535, 13

L_0760:
    Plugin6_Cmd1006 15, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0781
    ListMenuAdd 36, 65535, 15

L_0781:
    Plugin6_Cmd1006 2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A2
    ListMenuAdd 24, 65535, 2

L_07A2:
    Plugin6_Cmd1006 12, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07C3
    ListMenuAdd 33, 65535, 12

L_07C3:
    Plugin6_Cmd1006 14, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07E4
    ListMenuAdd 34, 65535, 14

L_07E4:
    Plugin6_Cmd1006 3, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0805
    ListMenuAdd 25, 65535, 3

L_0805:
    VMReturn

L_0807:
    // "Which battle format\nwill you choose?"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhichBattleFormatWill, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 44, 65535, 0
    ListMenuAdd 45, 65535, 1
    ListMenuAdd 46, 65535, 2
    ListMenuAdd 47, 65535, 3
    ListMenuAdd 38, 65535, 4
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_085F
    WorkSetConst 0x8023, 4

L_085F:
    VMReturn

L_0861:
    WbtCmd_GetTournament 0x8021
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0890
    WorkSetConst 0x8027, 1
    VMReturn

L_0890:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_08A5
    MsgWinCloseAll

L_08A5:
    Plugin6_Cmd1046 0x8010
    Plugin6_Cmd1028 0x8010
    DebugPrint 0x8010
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08CC
    VMReturn
    VMJump L_0970

L_08CC:
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0970
    Plugin6_Cmd1058 0x8010
    WorkCmpConst 0x8010, 60
    VMJumpIf CMP_EQ, L_08F6
    VMJump L_0904

L_08F6:
    // "You don't meet the conditions,\nso you can't participate.[f000]븁\u0000\nRules differ for each tournament.[f000]븁\u0000\nPlease check the Download Tournament\nrules before participating.[f000]븁\u0000"
    SystemMsg Global10655_Text_DontMeetConditionsCant_11, 2
    MsgWinCloseAll
    VMJump L_096E

L_0904:
    WorkCmpConst 0x8010, 48
    VMJumpIf CMP_EQ, L_093E
    WorkCmpConst 0x8010, 49
    VMJumpIf CMP_EQ, L_093E
    WorkCmpConst 0x8010, 50
    VMJumpIf CMP_EQ, L_093E
    WorkCmpConst 0x8010, 51
    VMJumpIf CMP_EQ, L_093E
    VMJump L_0952

L_093E:
    SystemMsg 0x8010, 2
    // "By the way, Eggs cannot participate![f000]븁\u0000"
    SystemMsg Global10655_Text_ByWayEggsCannot, 2
    MsgWinCloseAll
    VMJump L_096E

L_0952:
    SystemMsg 0x8010, 2
    Plugin6_Cmd1057 0x8026
    VMStackPush 0x8000
    WorkSet 0x8000, 0x8026
    RTCallGlobal 10261
    VMStackPop 0x8000

L_096E:
    VMReturn

L_0970:
    WorkSetConst 0x8027, 1
    VMReturn

L_0978:
    WbtCmd_GetTournament 0x8021
    VMStackPush 0x8021
    VMStackPushConst 12
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 13
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_09E6
    // "A randomized draw will determine your\nPokémon team. The excitement builds!"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_RandomizedDrawWillDetermine, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32784
    ListMenuAdd 109, 65535, 0
    ListMenuAdd 110, 65535, 1
    ListMenuAdd 111, 65535, 2
    ListMenuShow
    // "The moment of truth!\nWhich Pokémon will you receive?[f000]븁\u0000\nCounting down...[f000]븁\u0000\n... ...\n... ...[f000]븁\u0000\nTa-da! Here they are![f000]븁\u0000\nThese are the Pokémon\navailable for rental![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_MomentTruthWhichPokemon, 0x8011, 4, 0
    MsgWinCloseAll
    WbtCmd_MakeRentalParty 0x8010
    VMJump L_09F4

L_09E6:
    // "Please choose the Pokémon that\nwill participate.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_PleaseChoosePokemonWill, 0x8011, 4, 0
    MsgWinCloseAll

L_09F4:
    PokePartyRecoverAll
    Plugin6_Cmd1048 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A0F
    VMReturn

L_0A0F:
    WorkSetConst 0x8027, 1
    VMReturn

L_0A17:
    FunfestBGMReturn
    Plugin6_Cmd1007 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A3E
    // "Your victory in the Unova Leaders\nTournament will enable you to participate[f000]븀\u0000\nin Leaders tournaments of more regions.[f000]븁\u0000\nGood luck to you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_VictoryUnovaLeadersTournament, 0x8011, 4, 0

L_0A3E:
    Plugin6_Cmd1007 10, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A63
    // "I have an announcement to make![f000]븁\u0000\nWin all the regional Leaders tournaments,\nand you'll be able to participate in the[f000]븀\u0000\nWorld Leaders Tournament and the[f000]븀\u0000\nType Expert Tournament.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_HaveAnnouncementMakeWin, 0x8011, 4, 0

L_0A63:
    Plugin6_Cmd1007 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A88
    // "One more win in the World Leaders\nTournament, and you'll be able to[f000]븀\u0000\nparticipate in the Champions Tournament.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_OneMoreWinWorld, 0x8011, 4, 0

L_0A88:
    Plugin6_Cmd1007 13, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AAD
    // "When you win all the regional Leaders\ntournaments as well as the Rental[f000]븀\u0000\nTournament, you'll be able to participate[f000]븀\u0000\nin the Rental Master Tournament![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhenWinAllRegional, 0x8011, 4, 0

L_0AAD:
    Plugin6_Cmd1007 15, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AD2
    // "When you win all the regional Leaders\ntournaments as well as[f000]븀\u0000\nthe Mix Tournament,[f000]븁\u0000\nyou'll be able to participate in the\nMix Master Tournament![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhenWinAllRegional_2, 0x8011, 4, 0

L_0AD2:
    // "Done!\nYour registration is complete.[f000]븀\u0000\nRight this way, please.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_DoneRegistrationCompleteRight, 0x8011, 4, 0
    MsgWinCloseAll
    Plugin6_Cmd1016 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B07
    ActorCmdExec 0x8011, Movement_0B40
    ActorCmdWait
    VMJump L_0B11

L_0B07:
    ActorCmdExec 0x8011, Movement_0B50
    ActorCmdWait

L_0B11:
    ActorCmdExec 255, Movement_0B60
    FadeOutBlack
    ActorCmdWait
    FadeWait
    MapChangeCore ZONE_PWT_3, 31, 0, 15, 2
    RTReserveScript 10665
    Cmd_02C5 23
    Cmd_01DD 9, 0x8021, 0
    VMReturn
    .balign 4, 0

Movement_0B40:
    Move 12, 1
    Move 15, 1
    Move 34, 1
    MoveEnd

Movement_0B50:
    Move 12, 1
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_0B60:
    Move 12, 5
    MoveEnd

L_0B68:
    // "Which tournament do you\nwant to know about?"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhichTournamentWantKnow, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 37, 65535, 100
    VMCall L_0616
    ListMenuAdd 38, 65535, 0
    ListMenuShow
    WorkCmpConst 0x8023, 100
    VMJumpIf CMP_EQ, L_0BA8
    VMJump L_0BBA

L_0BA8:
    // "There are four battle formats\nto a tournament:[f000]븀\u0000\nSingle, Double, Triple, and Rotation.[f000]븁\u0000\nSingle Battles need three Pokémon.[f000]븁\u0000\nDouble Battles need four Pokémon.[f000]븁\u0000\nTriple Battles need six Pokémon.[f000]븁\u0000\nRotation Battles need four Pokémon.[f000]븁\u0000\nTriple Battles and Rotation Battles\ngive you more points than other formats[f000]븀\u0000\nwhen you manage to win, so jump in![f000]븀\u0000\nIt's worthwhile to try these formats![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_ThereFourBattleFormats, 0x8011, 4, 0
    VMJump L_0DEB

L_0BBA:
    WorkCmpConst 0x8023, 11
    VMJumpIf CMP_EQ, L_0BCD
    VMJump L_0BDF

L_0BCD:
    // "At last, the Driftveil Tournament!\nIn this tournament, anything goes.[f000]븁\u0000\nAny Pokémon with any held item\ncan participate.[f000]븁\u0000\nThe winner will be rewarded with BP.\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_LastDriftveilTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0BDF:
    WorkCmpConst 0x8023, 4
    VMJumpIf CMP_EQ, L_0BF2
    VMJump L_0C04

L_0BF2:
    // "Driftveil Tournament is a tournament\nwhere anything goes.[f000]븁\u0000\nYou may use any Pokémon or\nheld items of your choice.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 25.[f000]븁\u0000\nThe winner will be rewarded\nwith BP.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_DriftveilTournamentTournamentWhere, 0x8011, 4, 0
    VMJump L_0DEB

L_0C04:
    WorkCmpConst 0x8023, 5
    VMJumpIf CMP_EQ, L_0C17
    VMJump L_0C29

L_0C17:
    // "Unova Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom Unova are participating.[f000]븁\u0000\nYou may use any Pokémon or\nheld items of your choice.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith BP.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_UnovaLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0C29:
    WorkCmpConst 0x8023, 6
    VMJumpIf CMP_EQ, L_0C3C
    VMJump L_0C4E

L_0C3C:
    // "Kanto Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom Kanto are participating.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_KantoLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0C4E:
    WorkCmpConst 0x8023, 7
    VMJumpIf CMP_EQ, L_0C61
    VMJump L_0C73

L_0C61:
    // "Johto Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom Johto are participating.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_JohtoLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0C73:
    WorkCmpConst 0x8023, 8
    VMJumpIf CMP_EQ, L_0C86
    VMJump L_0C98

L_0C86:
    // "Hoenn Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom Hoenn are participating.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_HoennLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0C98:
    WorkCmpConst 0x8023, 9
    VMJumpIf CMP_EQ, L_0CAB
    VMJump L_0CBD

L_0CAB:
    // "Sinnoh Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom Sinnoh are participating.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_SinnohLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0CBD:
    WorkCmpConst 0x8023, 10
    VMJumpIf CMP_EQ, L_0CD0
    VMJump L_0CE2

L_0CD0:
    // "World Leaders Tournament is a\ntournament in which Gym Leaders[f000]븀\u0000\nfrom various regions are participating.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner of will be rewarded with BP,\ndepending on how many[f000]븀\u0000\nPokémon are left standing.[f000]븁\u0000\nAim for total victory! Good luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WorldLeadersTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0CE2:
    WorkCmpConst 0x8023, 1
    VMJumpIf CMP_EQ, L_0CF5
    VMJump L_0D07

L_0CF5:
    // "Champions Tournament is a tournament\nin which only the Champions of the[f000]븀\u0000\nChampions can participate.[f000]븁\u0000\nYou may not have duplicate Pokémon\nor duplicate held items.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe winner will be rewarded with BP,\ndepending on how many Pokémon[f000]븀\u0000\nare left standing.[f000]븁\u0000\nAim for total victory! Good luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_ChampionsTournamentTournamentWhich, 0x8011, 4, 0
    VMJump L_0DEB

L_0D07:
    WorkCmpConst 0x8023, 13
    VMJumpIf CMP_EQ, L_0D1A
    VMJump L_0D2C

L_0D1A:
    // "Rental Master Tournament is a unique\ntournament where we prepare the[f000]븀\u0000\nPokémon with which you will battle.[f000]븁\u0000\nRental Pokémon will be randomly\nselected for you. They are more rare[f000]븀\u0000\nthan the ones in the Rental Tournament.[f000]븁\u0000\nTo make the battles more thrilling, the\nparticipants are Gym Leaders from[f000]븀\u0000\nvarious regions.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_RentalMasterTournamentUnique, 0x8011, 4, 0
    VMJump L_0DEB

L_0D2C:
    WorkCmpConst 0x8023, 15
    VMJumpIf CMP_EQ, L_0D3F
    VMJump L_0D51

L_0D3F:
    // "Mix Master Tournament is a unique\ntournament where you and your opponent[f000]븀\u0000\nswap Pokémon before battling.[f000]븁\u0000\nYou'll normally swap one Pokémon,\nbut you'll swap two in Triple Battles.[f000]븁\u0000\nSwapped Pokémon will be returned\nafter the battle, so don't be alarmed.[f000]븁\u0000\nFor these battles, all Pokémon\nwill be set to Level 50.[f000]븁\u0000\nThe participants in the Mix Master\nTournament are Unova Gym Leaders,[f000]븀\u0000\nso there will be rare Pokémon in the mix.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_MixMasterTournamentUnique, 0x8011, 4, 0
    VMJump L_0DEB

L_0D51:
    WorkCmpConst 0x8023, 2
    VMJumpIf CMP_EQ, L_0D64
    VMJump L_0D76

L_0D64:
    // "Type Expert Tournament is a tournament\nin which only Pokémon of a specific type[f000]븀\u0000\ncan participate.[f000]븁\u0000\nFor example, if you choose\nFire-type Expert Tournament, only[f000]븀\u0000\nFire-type Pokémon can participate.[f000]븁\u0000\nYou'll need different Pokémon\nof the same type.[f000]븁\u0000\nMake sure no two Pokémon are\nholding the same item.[f000]븁\u0000\nThe item Soul Dew and the\nmove Sky Drop are banned.[f000]븁\u0000\nLegendary or mythical Pokémon\nmay not participate, either.[f000]븁\u0000\nFor these battles, all Pokémon will be\nset to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_TypeExpertTournamentTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0D76:
    WorkCmpConst 0x8023, 12
    VMJumpIf CMP_EQ, L_0D89
    VMJump L_0D9B

L_0D89:
    // "Rental Tournament is a unique tournament\nwhere we prepare the Pokémon with which[f000]븀\u0000\nyou will battle.[f000]븁\u0000\nRental Pokémon will be randomly selected\nfor you. So luck plays a big part![f000]븁\u0000\nThe winner will be rewarded\nwith BP.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_RentalTournamentUniqueTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0D9B:
    WorkCmpConst 0x8023, 14
    VMJumpIf CMP_EQ, L_0DAE
    VMJump L_0DC0

L_0DAE:
    // "Mix Tournament is a unique tournament\nwhere you and your opponent swap[f000]븀\u0000\nPokémon before battling.[f000]븁\u0000\nYou'll normally swap one Pokémon,\nbut you'll swap two in Triple Battles.[f000]븁\u0000\nSwapped Pokémon will be returned\nafter the battle, so don't be alarmed.[f000]븁\u0000\nFor these battles, all Pokémon will be\nset to Level 50.[f000]븁\u0000\nThe winner will be rewarded\nwith BP.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_MixTournamentUniqueTournament, 0x8011, 4, 0
    VMJump L_0DEB

L_0DC0:
    WorkCmpConst 0x8023, 3
    VMJumpIf CMP_EQ, L_0DD3
    VMJump L_0DE5

L_0DD3:
    // "Download Tournament is a tournament\nthat you can download from the[f000]븀\u0000\nblack PC next to me.[f000]븁\u0000\nThe rules are different for\neach tournament.[f000]븁\u0000\nPlease check the detailed rules\nbefore participating.[f000]븁\u0000\nThe winner will be rewarded\nwith a little more BP than usual.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_DownloadTournamentTournamentCan, 0x8011, 4, 0
    VMJump L_0DEB

L_0DE5:
    WorkSetConst 0x8024, 1

L_0DEB:
    VMReturn

L_0DED:
    // "Which type of tournament\nwill you participate in?"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WhichTypeTournamentWill, 0x8011, 4, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32803
    ListMenuAdd 83, 65535, 0
    ListMenuAdd 84, 65535, 9
    ListMenuAdd 85, 65535, 10
    ListMenuAdd 86, 65535, 11
    ListMenuAdd 87, 65535, 12
    ListMenuAdd 88, 65535, 14
    ListMenuAdd 89, 65535, 1
    ListMenuAdd 90, 65535, 3
    ListMenuAdd 91, 65535, 4
    ListMenuAdd 92, 65535, 2
    ListMenuAdd 93, 65535, 13
    ListMenuAdd 94, 65535, 6
    ListMenuAdd 95, 65535, 5
    ListMenuAdd 96, 65535, 7
    ListMenuAdd 97, 65535, 15
    ListMenuAdd 98, 65535, 16
    ListMenuAdd 99, 65535, 8
    ListMenuAdd 38, 65535, 17
    ListMenuShow
    VMStackPush 0x8023
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0EAD
    WorkSetConst 0x8023, 17

L_0EAD:
    VMReturn

L_0EAF:
    Plugin6_Cmd1025 0x8028
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0ED8
    // "Huh?[f000]븁\u0000\nThere are no tournaments\nyou can participate in.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_HuhThereNoTournaments, 0x8011, 4, 0
    VMJump L_0EE0

L_0ED8:
    MsgWinCloseAll
    Plugin6_Cmd1049 0x8010, 0x8022

L_0EE0:
    VMReturn

Script_4:
    ActorsPauseAll
    Plugin6_Cmd1016 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F1B
    WorkSetConst 0x8011, 0
    ActorCmdExec 255, Movement_1038
    ActorCmdWait
    ActorCmdExec 0x8011, Movement_1050
    ActorCmdWait
    VMJump L_0F48

L_0F1B:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F48
    WorkSetConst 0x8011, 8
    ActorCmdExec 255, Movement_1044
    ActorCmdWait
    ActorCmdExec 0x8011, Movement_105C
    ActorCmdWait

L_0F48:
    WbtCmd_GetRound 0x8010
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0F5F
    VMJump L_0F77

L_0F5F:
    // "You lost in the first round.\nKeep trying, and better luck next time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_LostFirstRoundKeep, 0x8011, 4, 0
    VMCall L_10AC
    VMJump L_1009

L_0F77:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_0F8A
    VMJump L_0FA2

L_0F8A:
    // "One more before the final round...\nI'm sure you'll do better next time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_OneMoreBeforeFinal, 0x8011, 4, 0
    VMCall L_10AC
    VMJump L_1009

L_0FA2:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_0FB5
    VMJump L_0FCD

L_0FB5:
    // "That was close.\nI'm sure you'll be the winner next time![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CloseImSureYoull, 0x8011, 4, 0
    VMCall L_10AC
    VMJump L_1009

L_0FCD:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_0FE0
    VMJump L_1009

L_0FE0:
    WbtCmd_GetTournament 0x8021
    Plugin6_Cmd1044 0, 0x8021
    // "Congratulations on winning the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CongratulationsWinning, 0x8011, 4, 0
    VMCall L_1068
    VMCall L_115E
    WbtCmd_RecordWin
    VMJump L_1009

L_1009:
    // "We hope to see you again!"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WeHopeSeeAgain, 0x8011, 4, 0
    LastKeyWait
    MsgWinCloseAll
    WbtCmd_Free
    VMStackPush 0x8021
    VMStackPushConst 11
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1032
    RTReserveScript 12

L_1032:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_1038:
    Move 13, 7
    Move 32, 1
    MoveEnd

Movement_1044:
    Move 13, 7
    Move 32, 1
    MoveEnd

Movement_1050:
    Move 14, 1
    Move 13, 1
    MoveEnd

Movement_105C:
    Move 15, 1
    Move 13, 1
    MoveEnd

L_1068:
    WbtCmd_AwardBattlePoints 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_10AA
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000, as a result of your victory,\nyou will be awarded Battle Points![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_ResultVictoryWillAwarded, 0x8011, 4, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    WordSetNumber 1, 0x8010, 2
    // "[f000]Ā\u0001\u0000 received [f000]ȁ\u0001\u0001 BP!"
    SystemMsg Global10655_Text_ReceivedBp, 2
    MEPlay SEQ_ME_BPGET
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll

L_10AA:
    VMReturn

L_10AC:
    // "Thank you for your participation!\nPlease accept this consolation prize.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_ThankParticipationPleaseAccept, 0x8011, 4, 0
    MsgWinCloseAll
    Random 0x8010, 4
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_10D3
    VMJump L_10DF

L_10D3:
    WorkSetConst 0x8010, 72
    VMJump L_113C

L_10DF:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_10F2
    VMJump L_10FE

L_10F2:
    WorkSetConst 0x8010, 73
    VMJump L_113C

L_10FE:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_1111
    VMJump L_111D

L_1111:
    WorkSetConst 0x8010, 74
    VMJump L_113C

L_111D:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_1130
    VMJump L_113C

L_1130:
    WorkSetConst 0x8010, 75
    VMJump L_113C

L_113C:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x8010
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMReturn

L_115E:
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1173
    VMReturn

L_1173:
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1253
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WbtCmd_GetType 0x802a
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 1

L_11A8:
    VMStackPush 0x802b
    VMStackPushConst 17
    VMStackCmp CMP_NE
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_122A
    WbtCmd_GetWinCount 2, 0x802b, 0x8010
    VMStackPush 0x802b
    VMStackPush 0x802a
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1205
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_11FF
    WorkSetConst 0x802c, 0

L_11FF:
    VMJump L_121E

L_1205:
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_121E
    WorkSetConst 0x802c, 0

L_121E:
    WorkAdd 0x802b, 1
    VMJump L_11A8

L_122A:
    VMStackPush 0x802c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1241
    MedalGive 145

L_1241:
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0

L_1253:
    WbtCmd_GetWinCount 0x8021, 17, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_13DF
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_128E
    WorkCmpConst 0x8021, 11
    VMJumpIf CMP_EQ, L_128E
    VMJump L_12A0

L_128E:
    MedalDiscover 134
    MedalDiscover 135
    MedalGive 133
    VMJump L_13DF

L_12A0:
    WorkCmpConst 0x8021, 12
    VMJumpIf CMP_EQ, L_12B3
    VMJump L_12BD

L_12B3:
    MedalGive 134
    VMJump L_13DF

L_12BD:
    WorkCmpConst 0x8021, 14
    VMJumpIf CMP_EQ, L_12D0
    VMJump L_12DA

L_12D0:
    MedalGive 135
    VMJump L_13DF

L_12DA:
    WorkCmpConst 0x8021, 5
    VMJumpIf CMP_EQ, L_12ED
    VMJump L_12F7

L_12ED:
    MedalGive 136
    VMJump L_13DF

L_12F7:
    WorkCmpConst 0x8021, 6
    VMJumpIf CMP_EQ, L_130A
    VMJump L_1314

L_130A:
    MedalGive 137
    VMJump L_13DF

L_1314:
    WorkCmpConst 0x8021, 7
    VMJumpIf CMP_EQ, L_1327
    VMJump L_1331

L_1327:
    MedalGive 138
    VMJump L_13DF

L_1331:
    WorkCmpConst 0x8021, 8
    VMJumpIf CMP_EQ, L_1344
    VMJump L_134E

L_1344:
    MedalGive 139
    VMJump L_13DF

L_134E:
    WorkCmpConst 0x8021, 9
    VMJumpIf CMP_EQ, L_1361
    VMJump L_136B

L_1361:
    MedalGive 140
    VMJump L_13DF

L_136B:
    WorkCmpConst 0x8021, 10
    VMJumpIf CMP_EQ, L_137E
    VMJump L_1388

L_137E:
    MedalGive 141
    VMJump L_13DF

L_1388:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_139B
    VMJump L_13A5

L_139B:
    MedalGive 142
    VMJump L_13DF

L_13A5:
    WorkCmpConst 0x8021, 13
    VMJumpIf CMP_EQ, L_13B8
    VMJump L_13C2

L_13B8:
    MedalGive 143
    VMJump L_13DF

L_13C2:
    WorkCmpConst 0x8021, 15
    VMJumpIf CMP_EQ, L_13D5
    VMJump L_13DF

L_13D5:
    MedalGive 144
    VMJump L_13DF

L_13DF:
    Plugin6_Cmd1007 6, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1414
    MedalDiscover 137
    MedalDiscover 138
    MedalDiscover 139
    MedalDiscover 140
    // "Wow, you can now participate in\nthe Leaders Tournaments![f000]븁\u0000\nLeaders Tournaments have\ndifferent battle rules.[f000]븁\u0000\nThe participants are all famous\nGym Leaders.[f000]븁\u0000\nYou'll be facing tougher opposition\nthan usual in these tournaments.[f000]븀\u0000\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_WowCanNowParticipate, 0x8011, 4, 0

L_1414:
    Plugin6_Cmd1007 10, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1442
    MedalDiscover 141
    Plugin6_Cmd1044 0, 10
    // "You can now participate in the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CanNowParticipate, 0x8011, 4, 0

L_1442:
    Plugin6_Cmd1007 13, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1470
    MedalDiscover 143
    Plugin6_Cmd1044 0, 13
    // "You can now participate in the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CanNowParticipate, 0x8011, 4, 0

L_1470:
    Plugin6_Cmd1007 15, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_149E
    MedalDiscover 144
    Plugin6_Cmd1044 0, 15
    // "You can now participate in the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CanNowParticipate, 0x8011, 4, 0

L_149E:
    Plugin6_Cmd1007 2, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_14CC
    MedalDiscover 145
    Plugin6_Cmd1044 0, 2
    // "You can now participate in the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CanNowParticipate, 0x8011, 4, 0

L_14CC:
    Plugin6_Cmd1006 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_1550
    Plugin6_Cmd1007 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1529
    MedalDiscover 142
    Plugin6_Cmd1044 0, 1
    // "You can now participate in the\n[f000]Ļ\u0001\u0000![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_CanNowParticipate, 0x8011, 4, 0
    VMJump L_1550

L_1529:
    WbtCmd_GetWinCount 10, 17, 0x8010
    WorkSetConst 0x8029, 9
    WorkSub 0x8029, 0x8010
    WordSetNumber 0, 0x8029, 2
    // "Oh, I almost forgot to share this news.[f000]븁\u0000\nYou'll be able to participate in the\nChampions Tournament after you win[f000]븀\u0000\nthe World Leaders Tournament[f000]븀\u0000\na certain number of times: [f000]Ȁ\u0001\u0000 more![f000]븁\u0000\nBest of luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10655_Text_OhAlmostForgotShare, 0x8011, 4, 0

L_1550:
    VMReturn
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802e, 0

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WorkSetConst 0x802d, 0

L_156A:
    VMStackPush 0x802d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1645
    // ""
    SystemMsg Global10655_Text_Empty, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32814
    ListMenuAdd 100, 103, 100
    ListMenuAdd 101, 104, 101
    ListMenuAdd 102, 105, 102
    ListMenuShow
    VMSleep 3
    WorkCmpConst 0x802e, 100
    VMJumpIf CMP_EQ, L_15BD
    VMJump L_15C9

L_15BD:
    VMCall L_164B
    VMJump L_163F

L_15C9:
    WorkCmpConst 0x802e, 101
    VMJumpIf CMP_EQ, L_15DC
    VMJump L_1637

L_15DC:
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8010, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1631
    FunfestBGMReturn
    VMCall L_1672

L_1631:
    VMJump L_163F

L_1637:
    InfoMsgClose
    WorkSetConst 0x802d, 1

L_163F:
    VMJump L_156A

L_1645:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_164B:
    // "Tournament Records\nwere accessed.[f000]븁\u0000"
    SystemMsg Global10655_Text_TournamentRecordsWereAccessed, 2
    InfoMsgClose
    Plugin6_Cmd1050 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1670
    WorkSetConst 0x802d, 1

L_1670:
    VMReturn

L_1672:
    // "The Tournament Download System\nwas accessed.[f000]븁\u0000"
    SystemMsg Global10655_Text_TournamentDownloadSystemAccessed, 2
    InfoMsgClose
    WbtCmd_Download 0, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_1699
    WorkSetConst 0x802d, 1

L_1699:
    VMReturn
    .balign 4, 0
