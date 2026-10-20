#include "asm/field_script.inc"
#include "text/script/global_10440.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    RTCallGlobal 2004
    WorkSet 0x8010, 0x8000
    VMStackPop 0x8000
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0087
    // "See you again."
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_SeeAgain, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_008D

L_0087:
    VMCall L_0095

L_008D:
    Cmd_013C
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0095:
    TrialHouseWorkInit
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    TrialHouseCmd_01F2 0x8027
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CC
    WorkSetConst 0x8028, 16
    WorkSetConst 0x8024, 1
    VMJump L_00F7

L_00CC:
    VMStackPush 0x8027
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F1
    WorkSetConst 0x8028, 25
    WorkSetConst 0x8024, 1
    VMJump L_00F7

L_00F1:
    WorkSetConst 0x8024, 0

L_00F7:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0145
    ActorMsg MSGFILE_SCRIPT, 0x8028, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_013F
    WorkSetConst 0x8023, 1
    VMCall L_02EE
    VMJump L_0145

L_013F:
    WorkSetConst 0x8021, 1

L_0145:
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    WorkSetConst 0x8020, 1
    TrialHouseGetBattleTestRank 0x8026
    VMStackPush 0x8026
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0193
    // "The master returns! Would you like to\nchallenge a Battle Test?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_MasterReturnsWouldLike, 0x8011, 2, 0
    VMJump L_019F

L_0193:
    // "I will judge your battles in a\nBattle Test![f000]븁\u0000\nWhat would you like to do?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WillJudgeBattlesBattle, 0x8011, 2, 0

L_019F:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    WorkSetConst 0x8020, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32805
    ListMenuAdd 30, 65535, 0
    ListMenuAdd 31, 65535, 1
    ListMenuAdd 35, 65535, 2
    ListMenuAdd 32, 65535, 3
    ListMenuShow
    VMStackPush 0x8025
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0202
    WorkSetConst 0x8021, 1
    VMJump L_02B3

L_0202:
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_0215
    VMJump L_0250

L_0215:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023A
    WorkSetConst 0x8023, 0
    VMCall L_02EE
    VMJump L_024A

L_023A:
    // "I am sorry...[f000]븁\u0000\nIt may be too early for you to challenge\na Battle Test.[f000]븁\u0000\nPlease come back after you finish\nyour journey and build your strength."
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_AmSorryMayToo, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_024A:
    VMJump L_02B3

L_0250:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0263
    VMJump L_0275

L_0263:
    WorkSetConst 0x8020, 1
    VMCall L_02E0
    VMJump L_02B3

L_0275:
    WorkCmpConst 0x8025, 2
    VMJumpIf CMP_EQ, L_0288
    VMJump L_0294

L_0288:
    VMCall L_07B5
    VMJump L_02B3

L_0294:
    WorkCmpConst 0x8025, 3
    VMJumpIf CMP_EQ, L_02A7
    VMJump L_02B3

L_02A7:
    WorkSetConst 0x8021, 1
    VMJump L_02B3

L_02B3:
    VMJump L_019F

L_02B9:
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DC
    // "See you again."
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_SeeAgain, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_02DC:
    TrialHouseWorkDelete
    VMReturn

L_02E0:
    // "You will take a Battle Test using\nthree Pokémon for Single Battles[f000]븀\u0000\nand four Pokémon for Double Battles.[f000]븁\u0000\nYou may not use duplicate Pokémon or\nduplicate held items.[f000]븁\u0000\nFor these battles, all Pokémon will be\nset to Lv. 50.[f000]븁\u0000\nYou will battle against five Trainers in a\nrow, and I will be your judge.[f000]븁\u0000\nWhat would you like to do?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WillTakeBattleTest, 0x8011, 2, 0
    VMReturn

L_02EE:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    TrialHouseCmd_01ED 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E4
    // "Which challenge would you like,\nSingle or Double?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WhichChallengeWouldLike, 0x8011, 2, 0
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32809
    ListMenuAdd 33, 65535, 0
    ListMenuAdd 34, 65535, 1
    ListMenuAdd 32, 65535, 2
    ListMenuShow
    VMStackPush 0x8029
    VMStackPushConst 65534
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0373
    WorkSetConst 0x8021, 1
    VMReturn
    VMJump L_03DE

L_0373:
    WorkCmpConst 0x8029, 0
    VMJumpIf CMP_EQ, L_0386
    VMJump L_0398

L_0386:
    WorkSetConst 0x802b, 20
    WorkSetConst 0x802c, 0
    VMJump L_03DE

L_0398:
    WorkCmpConst 0x8029, 1
    VMJumpIf CMP_EQ, L_03AB
    VMJump L_03BD

L_03AB:
    WorkSetConst 0x802b, 21
    WorkSetConst 0x802c, 1
    VMJump L_03DE

L_03BD:
    WorkCmpConst 0x8029, 2
    VMJumpIf CMP_EQ, L_03D0
    VMJump L_03DE

L_03D0:
    WorkSetConst 0x8021, 1
    VMReturn
    VMJump L_03DE

L_03DE:
    VMJump L_044A

L_03E4:
    WorkSetConst 0x802e, 0
    TrialHouseCmd_01F2 0x802e
    VMStackPush 0x802e
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0413
    WorkSetConst 0x802b, 20
    WorkSetConst 0x802c, 0
    VMJump L_0444

L_0413:
    VMStackPush 0x802e
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0438
    WorkSetConst 0x802b, 21
    WorkSetConst 0x802c, 1
    VMJump L_0444

L_0438:
    WorkSetConst 0x802b, 20
    WorkSetConst 0x802c, 0

L_0444:
    WorkSetConst 0x802e, 0

L_044A:
    TrialHousePrepareParty 0x802c
    // "Then, let's begin.[f000]븁\u0000\nPlease bear in mind that once you start\nthe challenge, you will face five battles[f000]븀\u0000\nwithout a break.[f000]븁\u0000\nNow, please choose the Pokémon you would\nlike to battle with.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_ThenLetsBeginPlease, 0x8011, 2, 0
    ActorMsgClose
    Cmd_01B0 0x802b, 0x802a
    VMStackPush 0x802a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0483
    WorkSetConst 0x8021, 1
    VMReturn
    VMJump L_04B0

L_0483:
    VMStackPush 0x802a
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B0
    VMStackPush 0x8000
    WorkSet 0x8000, 0x802b
    RTCallGlobal 10260
    VMStackPop 0x8000
    WorkSetConst 0x8021, 1
    VMReturn

L_04B0:
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C5
    PokePartyRecoverAll

L_04C5:
    TrialHouseCallTeamSelect 0x802b, 0x802a, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E8
    WorkSetConst 0x8021, 1
    VMReturn

L_04E8:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0507
    // "Saving...\nDon't turn off the power."
    SystemMsgAsync Global10440_Text_SavingDontTurnOff, 2
    TrialHouseSaveData 1
    InfoMsgClose

L_0507:
    RecordAdd 121, 1
    Cmd_02C5 4
    FunfestBGMReturn
    // "Now, the Battle Test begins!\nPlease go inside![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_NowBattleTestBegins, 0x8011, 2, 0
    ActorMsgClose
    ActorCmdExec 0, Movement_09C8
    ActorCmdWait
    ActorCmdExec 255, Movement_09D8
    ActorCmdWait
    ActorCmdExec 0, Movement_09E8
    ActorCmdWait
    VMCall L_06E2
    ActorCmdExec 0, Movement_0A18
    ActorCmdWait
    ActorCmdExec 255, Movement_0A24
    ActorCmdWait
    ActorCmdExec 0, Movement_0A34
    ActorCmdWait
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06E0
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    TrialHouseGetBattleTestRank 0x8030
    // "All right.[f000]븁\u0000\nI will tell you the result of your\nBattle Test.[f000]븁\u0000\nThe test result is...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_AllRightWillTell, 0x8011, 2, 0
    ActorMsgClose
    Cmd_01DD 3, 0, 0
    TrialHouseCalcPointsStars 0x802f, 0x8031
    RecordGet 121, 0x802d
    VMStackPush 0x8030
    VMStackPush 0x802f
    VMStackCmp CMP_LT
    VMStackPush 0x802d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_05DF
    Cmd_01DD 4, 0x802f, 0

L_05DF:
    FadeOutBlackQ
    FadeWait
    FieldClose
    TrialHouseCmd_01F4 0x8023, 1
    FieldOpen
    FadeInBlackQ
    FadeWait
    WorkCmpConst 0x802f, 0
    VMJumpIf CMP_EQ, L_0604
    VMJump L_0610

L_0604:
    WorkSetConst 0x8032, 15
    VMJump L_06D0

L_0610:
    WorkCmpConst 0x802f, 1
    VMJumpIf CMP_EQ, L_0623
    VMJump L_062F

L_0623:
    WorkSetConst 0x8032, 14
    VMJump L_06D0

L_062F:
    WorkCmpConst 0x802f, 2
    VMJumpIf CMP_EQ, L_0642
    VMJump L_064E

L_0642:
    WorkSetConst 0x8032, 13
    VMJump L_06D0

L_064E:
    WorkCmpConst 0x802f, 3
    VMJumpIf CMP_EQ, L_0661
    VMJump L_066D

L_0661:
    WorkSetConst 0x8032, 12
    VMJump L_06D0

L_066D:
    WorkCmpConst 0x802f, 4
    VMJumpIf CMP_EQ, L_0680
    VMJump L_068C

L_0680:
    WorkSetConst 0x8032, 11
    VMJump L_06D0

L_068C:
    WorkCmpConst 0x802f, 5
    VMJumpIf CMP_EQ, L_069F
    VMJump L_06AB

L_069F:
    WorkSetConst 0x8032, 10
    VMJump L_06D0

L_06AB:
    WorkCmpConst 0x802f, 6
    VMJumpIf CMP_EQ, L_06BE
    VMJump L_06CA

L_06BE:
    WorkSetConst 0x8032, 9
    VMJump L_06D0

L_06CA:
    WorkSetConst 0x8032, 15

L_06D0:
    ActorMsg MSGFILE_SCRIPT, 0x8032, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_06E0:
    VMReturn

L_06E2:
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8022, 0

L_06EE:
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_075A
    TrialHouseCmd_01EA 0x8022, 0x8033
    ActorNew 7, 0, 1, 251, 0x8033, 0
    ActorCmdExec 251, Movement_09F4
    ActorCmdWait
    TrialHouseMsgDisp 0, 251
    TrialHouseStartBattle
    ActorCmdExec 251, Movement_0A08
    ActorCmdWait
    ActorDelete 251
    WorkAdd 0x8022, 1
    VMStackPush 0x8022
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0754
    VMCall L_0762

L_0754:
    VMJump L_06EE

L_075A:
    WorkSetConst 0x8010, 1
    VMReturn

L_0762:
    WorkSetConst 0x8034, 0
    ActorCmdExec 0, Movement_0A40
    ActorCmdExec 255, Movement_0A4C
    ActorCmdWait
    MEPlay SEQ_ME_ASA
    MEWait
    WorkGet 0x8034, 0x8022
    WorkAdd 0x8034, 1
    WordSetNumber 0, 0x8034, 1
    // "You will be facing opponent No. [f000]Ȁ\u0001\u0000.\nGood luck![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WillFacingOpponentNo, 0x8011, 2, 0
    ActorMsgClose
    ActorCmdExec 255, Movement_0A54
    ActorCmdExec 0, Movement_0A5C
    ActorCmdWait
    VMReturn

L_07B5:
    GameCommCheckDSiWiFi 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07DE
    ActorMsgClose
    RTCallGlobal 2005
    WorkSetConst 0x8021, 1
    VMJump L_0959

L_07DE:
    // "Would you like to download\na special Battle Test?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WouldLikeDownloadSpecial, 0x8011, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0953
    ActorMsgClose
    VMCall L_095B
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0824
    WorkSetConst 0x8021, 1
    VMReturn

L_0824:
    // "Communicating... Don't turn off the\npower. Press the B Button to cancel."
    SystemMsgAsync Global10440_Text_CommunicatingDontTurnOff, 2
    VMSleep 1
    MsgSetLoadingSpinner 0
    TrialHouseCmd_01F0 0x8010
    InfoMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0865
    // "Hmm... I don't see anything\nto download...[f000]븁\u0000\nWhat would you like to do?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_HmmDontSeeAnything, 0x8011, 2, 0
    WorkSetConst 0x8020, 1
    VMReturn
    VMJump L_08B9

L_0865:
    VMStackPush 0x8010
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0892
    // "You seem to have taken all of the\ncurrent Battle Tests already.[f000]븁\u0000\nWhat would you like to do?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_SeemHaveTakenAll, 0x8011, 2, 0
    WorkSetConst 0x8020, 1
    VMReturn
    VMJump L_08B9

L_0892:
    VMStackPush 0x8010
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08B9
    // "The download was canceled."
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_DownloadCanceled, 0x8011, 2, 0
    WorkSetConst 0x8020, 1
    VMReturn

L_08B9:
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    TrialHouseCmd_01F2 0x8035
    VMStackPush 0x8035
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08E8
    WorkSetConst 0x8036, 19
    VMJump L_08EE

L_08E8:
    WorkSetConst 0x8036, 20

L_08EE:
    ActorMsg MSGFILE_SCRIPT, 0x8036, 0x8011, 2, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    // "Would you like to challenge right away?"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_WouldLikeChallengeRight, 0x8011, 2, 0
    YesNoWin 0x8010
    ActorMsgClose
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_093D
    WorkSetConst 0x8023, 1
    VMCall L_02EE
    VMJump L_094D

L_093D:
    // "OK. See you later.\nI've been waiting for you!"
    ActorMsg MSGFILE_SCRIPT, Global10440_Text_OkSeeLaterIve, 0x8011, 2, 0
    LastKeyWait
    ActorMsgClose

L_094D:
    VMJump L_0959

L_0953:
    WorkSetConst 0x8021, 1

L_0959:
    VMReturn

L_095B:
    WorkSetConst 0x8037, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    VMStackPush 0x8005
    RTCallGlobal 2003
    WorkSet 0x8037, 0x8000
    VMStackPop 0x8005
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8037
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_09BA
    WorkSetConst 0x8010, 0
    VMJump L_09C0

L_09BA:
    WorkSetConst 0x8010, 1

L_09C0:
    WorkSetConst 0x8037, 0
    VMReturn

Movement_09C8:
    Move 12, 1
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_09D8:
    Move 12, 5
    Move 15, 2
    Move 34, 1
    MoveEnd

Movement_09E8:
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_09F4:
    Move 70, 1
    Move 13, 4
    Move 14, 2
    Move 35, 1
    MoveEnd

Movement_0A08:
    Move 15, 2
    Move 12, 4
    Move 69, 1
    MoveEnd

Movement_0A18:
    Move 14, 1
    Move 35, 1
    MoveEnd

Movement_0A24:
    Move 14, 2
    Move 13, 5
    Move 32, 1
    MoveEnd

Movement_0A34:
    Move 15, 1
    Move 13, 1
    MoveEnd

Movement_0A40:
    Move 15, 2
    Move 12, 2
    MoveEnd

Movement_0A4C:
    Move 33, 1
    MoveEnd

Movement_0A54:
    Move 34, 1
    MoveEnd

Movement_0A5C:
    Move 13, 2
    Move 14, 2
    Move 32, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    SEPlay SEQ_SE_MESSAGE
    Cmd_01F5 0x803a
    WorkSetConst 0x8039, 0
    VMStackPush 0x803a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AB1
    // "There is no test result to display now."
    SystemMsgAsync Global10440_Text_ThereNoTestResult, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0B70

L_0AB1:
    VMStackPush 0x803a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AD6
    WorkSetConst 0x8039, 1
    WorkSetConst 0x8038, 0
    VMJump L_0B70

L_0AD6:
    VMStackPush 0x803a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0AFB
    WorkSetConst 0x8039, 1
    WorkSetConst 0x8038, 1
    VMJump L_0B70

L_0AFB:
    // "Which test result would you like to see?"
    SystemMsgAsync Global10440_Text_WhichTestResultWould, 2
    ListMenu_AnchorTopRight 31, 1, 0, 1, 32784
    ListMenuAdd 36, 65535, 0
    ListMenuAdd 37, 65535, 1
    ListMenuAdd 32, 65535, 2
    ListMenuShow
    InfoMsgClose
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0B39
    VMJump L_0B4B

L_0B39:
    WorkSetConst 0x8039, 1
    WorkSetConst 0x8038, 0
    VMJump L_0B70

L_0B4B:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0B5E
    VMJump L_0B70

L_0B5E:
    WorkSetConst 0x8039, 1
    WorkSetConst 0x8038, 1
    VMJump L_0B70

L_0B70:
    VMStackPush 0x8039
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B95
    FadeOutBlackQ
    FadeWait
    FieldClose
    TrialHouseCmd_01F4 0x8038, 0
    FieldOpen
    FadeInBlackQ
    FadeWait

L_0B95:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
