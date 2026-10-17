#include "asm/field_script.inc"
#include "text/script/join_avenue.h"

// Script plugin 8, from the zones that use this file

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    VMHalt

Script_2:
    WorkSetConst 0x8021, 0
    VMCall L_0250
    Plugin8_Cmd1031 20, 0x8021
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0065
    WorkSetConst EVENT_WORK_0x4119, 1

L_0065:
    WorkCmpConst EVENT_WORK_0x4110, 1
    VMJumpIf CMP_EQ, L_0078
    VMJump L_00A2

L_0078:
    ActorDelete 0
    ActorSetGPos 1, 15, 0, 70, 1
    ActorSetGPos 2, 14, 0, 70, 1
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_00A2:
    WorkCmpConst EVENT_WORK_0x4110, 2
    VMJumpIf CMP_EQ, L_00B5
    VMJump L_00DF

L_00B5:
    ActorDelete 0
    ActorSetGPos 1, 15, 0, 70, 1
    ActorSetGPos 2, 14, 0, 70, 1
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_00DF:
    WorkCmpConst EVENT_WORK_0x4110, 3
    VMJumpIf CMP_EQ, L_00F2
    VMJump L_011C

L_00F2:
    ActorDelete 0
    ActorSetGPos 1, 15, 0, 70, 1
    ActorSetGPos 2, 14, 0, 70, 1
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_011C:
    WorkCmpConst EVENT_WORK_0x4110, 4
    VMJumpIf CMP_EQ, L_012F
    VMJump L_0159

L_012F:
    ActorDelete 0
    ActorSetGPos 1, 15, 0, 70, 1
    ActorSetGPos 2, 14, 0, 70, 1
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_0159:
    WorkCmpConst EVENT_WORK_0x4110, 5
    VMJumpIf CMP_EQ, L_016C
    VMJump L_0196

L_016C:
    ActorDelete 0
    ActorSetGPos 1, 15, 0, 70, 1
    ActorSetGPos 2, 14, 0, 70, 1
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_0196:
    WorkCmpConst EVENT_WORK_0x4110, 6
    VMJumpIf CMP_EQ, L_01A9
    VMJump L_01C3

L_01A9:
    ActorDelete 0
    ActorDelete 1
    ActorDelete 2
    ActorDelete 3
    ActorDelete 4
    VMJump L_01C3

L_01C3:
    VMStackPush EVENT_WORK_0x413a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_023A
    WorkSetConst 0x8022, 0
    ActorSetGPos 255, 15, 0, 70, 0
    Plugin8_Cmd1027 0, 0x8022
    ActorSetGPos 0x8022, 14, 0, 71, 0
    Plugin8_Cmd1027 1, 0x8022
    ActorSetGPos 0x8022, 16, 0, 71, 0
    Plugin8_Cmd1027 2, 0x8022
    ActorSetGPos 0x8022, 13, 0, 72, 0
    Plugin8_Cmd1027 3, 0x8022
    ActorSetGPos 0x8022, 17, 0, 72, 0
    Plugin8_Cmd1030 23, 1
    Plugin8_Cmd1037 1

L_023A:
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    VMHalt

Script_7:
    VMCall L_0250
    VMHalt

L_0250:
    GameGetVersion 0x8020
    VMStackPush 0x8020
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0277
    ObjInitWarpGPos 1, 0, 0, 0
    VMJump L_0281

L_0277:
    ObjInitWarpGPos 0, 0, 0, 0

L_0281:
    VMReturn

Script_3:
    ActorsPauseAll
    // "Hmmmm... What should I do?\nI can't possibly manage everything.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_HmmmmWhatShouldCant, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 0, Movement_04AC
    ActorCmdExec 3, Movement_04B4
    ActorCmdExec 4, Movement_04B4
    ActorCmdWait
    // "Hello there![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_HelloThere, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 255, Movement_04AC
    ActorCmdWait
    ActorWalkRoute 255, 15, 72, 0, 8, 0
    ActorCmdWait
    // "Welcome to Join Avenue![f000]븁\u0000\nWe don't have anything yet, as you see,\nso it's just an avenue at this point.[f000]븁\u0000\nOh, where are my manners?\nAllow me to introduce myself.[f000]븁\u0000\nI am the owner of Join Avenue.\nMy dream is to go around the world[f000]븀\u0000\nbuilding avenues that bustle with[f000]븀\u0000\nlots of people.[f000]븁\u0000\nThe problem is...I have no one\nI can trust to manage the avenue.[f000]븁\u0000\n...\n...[f000]븁\u0000\nSomething just struck me![f000]븁\u0000\nYou seem to be a Trainer traveling\naround, aren't you?[f000]븁\u0000\nYou naturally meet people from\nall over this region, don't you?[f000]븁\u0000\nI know it seems sudden, but will you\nmanage the avenue for me?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WelcomeJoinAvenueWe, 0, 0, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 1

L_02FB:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_034F
    YesNoWin 0x8023
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033D
    // "Thank you so much![f000]븁\u0000\nOK, tell me what kind of a person you are.[f000]븁\u0000\nWhat would be your favorite phrase\nthat you'd use to greet everyone?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_ThankMuchOkTell, 0, 0, 0
    WorkSetConst 0x8024, 0
    VMJump L_0349

L_033D:
    // "I really need you to help me.\nYou know what that means, don't you?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_ReallyNeedHelpKnow, 0, 0, 0

L_0349:
    VMJump L_02FB

L_034F:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    MsgWinCloseAll
    // "[f000][ff00]\u0001\u0002[f000]봂\u0000Warning![f000][ff00]\u0001\u0000[f000]븁\u0000\nThe words you are about to enter\nmay be sent to other players.[f000]븁\u0000\nPlease consider them carefully\nbefore you register them.[f000]븁\u0000"
    SystemMsg JoinAvenue_Text_WarningWordsAboutEnter, 2
    InfoMsgClose
    Plugin8_Cmd1023 2, 0
    // "That's a great line![f000]븁\u0000\nThen what would you say when\nsomething truly moves your heart?[f000]븁\u0000\nYou want to choose something universal,\na phrase that anyone will understand,[f000]븀\u0000\nyou know?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_ThatsGreatLineThen, 0, 0, 0
    MsgWinCloseAll
    // "[f000][ff00]\u0001\u0002[f000]봂\u0000Warning![f000][ff00]\u0001\u0000[f000]븁\u0000\nThe words you are about to enter\nmay be sent to other players.[f000]븁\u0000\nPlease consider them carefully\nbefore you register them.[f000]븁\u0000"
    SystemMsg JoinAvenue_Text_WarningWordsAboutEnter, 2
    InfoMsgClose
    Plugin8_Cmd1023 3, 0
    Plugin8_Cmd1007 19, 255, 0, 0
    Plugin8_Cmd1007 20, 255, 0, 1
    // "[f000]ķ\u0001\u0000\n[f000]ķ\u0001\u0001[f000]븁\u0000\nI knew it!\nYou are the one![f000]븁\u0000\nWho else could be so well suited\nto managing the avenue?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_KnewOneWhoElse, 0, 0, 0
    MsgWinCloseAll
    VMSleep 16
    ActorCmdExec 3, Movement_04AC
    ActorCmdWait
    ActorCmdExec 3, Movement_04C4
    ActorCmdWait
    // "Sir![f000]븁\u0000\nIt's almost time for\nyour next appointment...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_SirItsAlmostTime, 3, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_04BC
    ActorCmdWait
    // "Oh, I almost forgot.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_OhAlmostForgot, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_04CC
    ActorCmdWait
    // "My assistants![f000]븁\u0000\nYou heard me. I must leave now, so\nplease support our newest manager.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_AssistantsHeardMustLeave, 0, 0, 0
    MsgWinCloseAll
    // "Yes, sir! Please take care of yourself.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_YesSirPleaseTake, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 0, Movement_04F8
    ActorCmdExec 3, Movement_04D4
    ActorCmdExec 4, Movement_04E4
    ActorCmdExec 255, Movement_0504
    ActorCmdWait
    ActorDelete 3
    ActorDelete 4
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 0
    SEWait
    ActorCmdExec 1, Movement_0510
    ActorCmdExec 2, Movement_0518
    ActorCmdWait
    ActorCmdExec 255, Movement_04CC
    ActorCmdWait
    // "Pleased to meet you![f000]븁\u0000\nHow should we address you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_PleasedMeetHowShould, 1, 0, 0
    MsgWinCloseAll
    Plugin8_Cmd1023 1, 0
    Plugin8_Cmd1007 4, 255, 0, 0
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nNow that you are the one to manage\nthe development of the avenue,[f000]븀\u0000\nplease turn it into a wonderful[f000]븀\u0000\nattraction where many people visit.[f000]븁\u0000\nI'll explain how to develop the avenue,\nso please talk to me when you are ready."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_NowOneManageDevelopment, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    MedalDiscover 182
    MedalDiscover 186
    MedalDiscover 190
    WorkSetConst EVENT_WORK_0x410b, 1
    WorkSetConst EVENT_WORK_0x4110, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_04AC:
    Move 75, 1
    MoveEnd

Movement_04B4:
    Move 1, 1
    MoveEnd

Movement_04BC:
    Move 3, 1
    MoveEnd

Movement_04C4:
    Move 2, 1
    MoveEnd

Movement_04CC:
    Move 0, 1
    MoveEnd

Movement_04D4:
    Move 15, 1
    Move 14, 1
    Move 13, 9
    MoveEnd

Movement_04E4:
    Move 15, 1
    Move 63, 1
    Move 15, 1
    Move 13, 9
    MoveEnd

Movement_04F8:
    Move 15, 1
    Move 13, 9
    MoveEnd

Movement_0504:
    Move 63, 2
    Move 1, 1
    MoveEnd

Movement_0510:
    Move 13, 2
    MoveEnd

Movement_0518:
    Move 13, 2
    MoveEnd

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1007 4, 255, 0, 0
    WorkCmpConst EVENT_WORK_0x4110, 0
    VMJumpIf CMP_EQ, L_0545
    VMJump L_055B

L_0545:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0640

L_055B:
    WorkCmpConst EVENT_WORK_0x4110, 1
    VMJumpIf CMP_EQ, L_056E
    VMJump L_0590

L_056E:
    // "To make it more attractive,\nyou'll want useful city improvements.[f000]븁\u0000\nFor example...shops![f000]븁\u0000\nIn order for you to make a shop,\nyou'll need someone who has a dream.[f000]븀\u0000\nYou then have to “invite\" that person[f000]븀\u0000\nto join the avenue.[f000]븁\u0000\nSpeaking of invitation,\nsomeone is coming this way.[f000]븁\u0000\nWhy don't you “invite\" that person?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_MakeMoreAttractiveYoull, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    Plugin8_Cmd1028 1, 0
    WorkSetConst EVENT_WORK_0x4110, 2
    VMJump L_0640

L_0590:
    WorkCmpConst EVENT_WORK_0x4110, 2
    VMJumpIf CMP_EQ, L_05A3
    VMJump L_05B9

L_05A3:
    // "Let's build a shop first.[f000]븁\u0000\nYou need to talk to a person\nin the avenue to “invite\" that person[f000]븀\u0000\nso you can have the person build a shop."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_LetsBuildShopFirst, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0640

L_05B9:
    WorkCmpConst EVENT_WORK_0x4110, 3
    VMJumpIf CMP_EQ, L_05CC
    VMJump L_05EE

L_05CC:
    // "Congratulations!\nYou've just made your first shop![f000]븁\u0000\nBut having a shop means nothing\nif you don't have any customers.[f000]븁\u0000\nYou'll need to talk to a customer and\n“recommend\" the shop.[f000]븁\u0000\nSpeaking of recommendation,\nsomeone is coming this way.[f000]븁\u0000\nWhy don't you “recommend\" our shop?"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_CongratulationsYouveJustMade, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    Plugin8_Cmd1028 1, 1
    WorkSetConst EVENT_WORK_0x4110, 4
    VMJump L_0640

L_05EE:
    WorkCmpConst EVENT_WORK_0x4110, 4
    VMJumpIf CMP_EQ, L_0601
    VMJump L_0617

L_0601:
    // "Let's “recommend\" the shop to someone."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_LetsRecommendShopSomeone, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0640

L_0617:
    WorkCmpConst EVENT_WORK_0x4110, 5
    VMJumpIf CMP_EQ, L_062A
    VMJump L_0640

L_062A:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0640

L_0640:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0648:
    Move 63, 2
    Move 14, 4
    MoveEnd

Movement_0654:
    Move 14, 1
    MoveEnd

Movement_065C:
    Move 14, 4
    Move 63, 1
    Move 69, 1
    MoveEnd

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    Plugin8_Cmd1007 4, 255, 0, 0
    WorkCmpConst EVENT_WORK_0x4110, 0
    VMJumpIf CMP_EQ, L_0691
    VMJump L_06A3

L_0691:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_2, 2, 0, 0
    VMJump L_075C

L_06A3:
    WorkCmpConst EVENT_WORK_0x4110, 1
    VMJumpIf CMP_EQ, L_06B6
    VMJump L_06C8

L_06B6:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_2, 2, 0, 0
    VMJump L_075C

L_06C8:
    WorkCmpConst EVENT_WORK_0x4110, 2
    VMJumpIf CMP_EQ, L_06DB
    VMJump L_06ED

L_06DB:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_3, 2, 0, 0
    VMJump L_075C

L_06ED:
    WorkCmpConst EVENT_WORK_0x4110, 3
    VMJumpIf CMP_EQ, L_0700
    VMJump L_0712

L_0700:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_4, 2, 0, 0
    VMJump L_075C

L_0712:
    WorkCmpConst EVENT_WORK_0x4110, 4
    VMJumpIf CMP_EQ, L_0725
    VMJump L_0737

L_0725:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_5, 2, 0, 0
    VMJump L_075C

L_0737:
    WorkCmpConst EVENT_WORK_0x4110, 5
    VMJumpIf CMP_EQ, L_074A
    VMJump L_075C

L_074A:
    // "[f000]ĺ\u0001\u0000![f000]븁\u0000\nWe will explain what you need to know\nto become truly superior in this role."
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_WeWillExplainWhat_6, 2, 0, 0
    VMJump L_075C

L_075C:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    ActorCmdExec 1, Movement_04B4
    ActorCmdExec 2, Movement_04B4
    ActorCmdWait
    Plugin8_Cmd1007 4, 255, 0, 0
    // "Congratulations![f000]븁\u0000\nYou've just recommended the shop, and\nthat made the popularity of[f000]븀\u0000\nthe avenue go up![f000]븁\u0000\nRaise the popularities of shops and the\navenue to make it famous![f000]븁\u0000\nA good tip for bringing more customers\nis to use the communication features.[f000]븁\u0000\nYou should turn on the C-Gear to attract\nlots of passersby.[f000]븁\u0000\nYou should also try communication\nfacilities, such as the Union Room[f000]븀\u0000\nand the Global Terminal.[f000]븁\u0000\n[f000]ĺ\u0001\u0000,\nwe'll be serving as your assistants[f000]븀\u0000\nin the room over here.[f000]븁\u0000\nPlease come visit us.[f000]븁\u0000\nIf you'll excuse us...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_CongratulationsYouveJustRecommended, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_065C
    ActorCmdExec 1, Movement_0648
    ActorCmdWait
    ActorDelete 2
    ActorCmdExec 1, Movement_0654
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 1
    SEWait
    Plugin8_Cmd1028 3, 4
    Plugin8_Cmd1028 3, 5
    WorkSetConst EVENT_WORK_0x4110, 6
    Plugin8_Cmd1030 18, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    Plugin8_Cmd1007 8, 255, 0, 0
    Plugin8_Cmd1007 4, 255, 0, 1
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ĺ\u0001\u0000\n[f000]ĺ\u0001\u0001's office[f000]븁\u0000"
    MsgPlaceSign JoinAvenue_Text_SOffice, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    Plugin8_Cmd1007 8, 255, 0, 0
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ĺ\u0001\u0000[f000]븁\u0000\nAn avenue that grows as you deepen\nexchanges with other people.[f000]븁\u0000"
    MsgPlaceSign JoinAvenue_Text_AvenueGrowsDeepenExchanges, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    Plugin8_Cmd1007 8, 255, 0, 0
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ĺ\u0001\u0000[f000]븁\u0000\nAn avenue that grows as you deepen\nexchanges with other people.[f000]븁\u0000"
    MsgPlaceSign JoinAvenue_Text_AvenueGrowsDeepenExchanges, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    ActorFindByGPos 0x8025, 0x8029, 14, 0, 71
    ActorFindByGPos 0x8026, 0x8029, 16, 0, 71
    ActorFindByGPos 0x8027, 0x8029, 13, 0, 72
    ActorFindByGPos 0x8028, 0x8029, 17, 0, 72
    WorkSetConst 0x802a, 0

L_08B8:
    VMStackPush 0x802a
    VMStackPushConst 8
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0A6B
    Plugin8_Cmd1030 24, 1
    ActorCmdExec 255, Movement_0D20
    ActorCmdExec 0x8025, Movement_0D20
    ActorCmdExec 0x8026, Movement_0D20
    ActorCmdExec 0x8027, Movement_0D20
    ActorCmdExec 0x8028, Movement_0D20
    ActorCmdWait
    WorkGet 0x802c, 0x802a
    WorkAdd 0x802c, 25
    Plugin8_Cmd1002 0x802c, 0x8029
    VMStackPush 0x8029
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0A5F
    VMStackPush 0x802a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x802a
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0991
    ActorCmdExec 255, Movement_04BC
    ActorCmdExec 0x8025, Movement_04BC
    ActorCmdExec 0x8026, Movement_04BC
    ActorCmdExec 0x8027, Movement_04BC
    ActorCmdExec 0x8028, Movement_04BC
    VMJump L_09B9

L_0991:
    ActorCmdExec 255, Movement_04C4
    ActorCmdExec 0x8025, Movement_04C4
    ActorCmdExec 0x8026, Movement_04C4
    ActorCmdExec 0x8027, Movement_04C4
    ActorCmdExec 0x8028, Movement_04C4

L_09B9:
    ActorCmdWait
    WorkGet 0x802c, 0x802a
    WorkAdd 0x802c, 44
    Plugin8_Cmd1002 0x802c, 0x802b
    Plugin8_Cmd1030 24, 0
    Plugin8_Cmd1007 0, 0, 0x802a, 0
    Plugin8_Cmd1007 6, 0, 0x802a, 1
    Plugin8_Cmd1007 18, 0, 0x802a, 2
    Plugin8_Cmd1007 4, 255, 0, 3
    // "[f000]Ā\u0001\u0000: [f000]ķ\u0001\u0001\n[f000]ķ\u0001\u0002[f000]븀\u0000\n[f000]ĺ\u0001\u0003! Hurrah![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, JoinAvenue_Text_Hurrah_2, 0x802b, 2, 0
    ActorMsgClose
    Plugin8_Cmd1007 0, 0, 0x802a, 0
    Plugin8_Cmd1007 21, 0, 0x802a, 1
    Plugin8_Cmd1007 22, 0, 0x802a, 2
    Plugin8_Cmd1007 23, 0, 0x802a, 3
    Plugin8_Cmd1007 1, 0, 0x802a, 4
    Plugin8_Cmd1007 10, 0, 0x802a, 5
    // "[f000]Ā\u0001\u0000\nMet on [f000]ȁ\u0001\u0002/[f000]ȁ\u0001\u0003/20[f000]ȁ\u0001\u0001[f000]븁\u0000"
    SystemMsg JoinAvenue_Text_Met20, 2
    SEPlay SEQ_SE_SW_JA_01
    SEPlay SEQ_SE_SW_JA_02
    // "[f000]ĸ\u0001\u0004\nRank [f000]ȁ\u0001\u0005"
    SystemMsg JoinAvenue_Text_Rank, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose

L_0A5F:
    WorkAdd 0x802a, 1
    VMJump L_08B8

L_0A6B:
    Plugin8_Cmd1030 24, 1
    ActorCmdExec 255, Movement_0D28
    ActorCmdExec 0x8025, Movement_0D28
    ActorCmdExec 0x8026, Movement_0D28
    ActorCmdExec 0x8027, Movement_0D28
    ActorCmdExec 0x8028, Movement_0D28
    ActorCmdWait
    FadeEx 3, 0, 16, 2
    FadeExWait
    Plugin8_Cmd1039 1
    ActorSetGPos 255, 15, 0, 40, 1
    ActorSetGPos 0x8025, 14, 0, 39, 1
    ActorSetGPos 0x8026, 16, 0, 39, 1
    ActorSetGPos 0x8027, 13, 0, 38, 1
    ActorSetGPos 0x8028, 17, 0, 38, 1
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 64037, 0, 0xe1000, 0xf8000, 0x24000, 0x247000, 1
    EvCameraWait
    Plugin8_Cmd1037 0
    FadeEx 3, 16, 0, 2
    FadeExWait
    Plugin8_Cmd1030 24, 0
    WorkSetConst 0x802a, 0

L_0B21:
    VMStackPush 0x802a
    VMStackPushConst 4
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0C19
    WorkCmpConst 0x802a, 0
    VMJumpIf CMP_EQ, L_0B47
    VMJump L_0B53

L_0B47:
    WorkGet 0x802b, 0x8025
    VMJump L_0BB0

L_0B53:
    WorkCmpConst 0x802a, 1
    VMJumpIf CMP_EQ, L_0B66
    VMJump L_0B72

L_0B66:
    WorkGet 0x802b, 0x8026
    VMJump L_0BB0

L_0B72:
    WorkCmpConst 0x802a, 2
    VMJumpIf CMP_EQ, L_0B85
    VMJump L_0B91

L_0B85:
    WorkGet 0x802b, 0x8027
    VMJump L_0BB0

L_0B91:
    WorkCmpConst 0x802a, 3
    VMJumpIf CMP_EQ, L_0BA4
    VMJump L_0BB0

L_0BA4:
    WorkGet 0x802b, 0x8028
    VMJump L_0BB0

L_0BB0:
    VMSleep 10
    ActorCmdExec 0x802b, Movement_0D30
    ActorCmdWait
    WorkGet 0x802c, 0x802a
    WorkAdd 0x802c, 1
    WordSetNumber 4, 0x802c, 1
    Plugin8_Cmd1007 0, 3, 0x802a, 0
    Plugin8_Cmd1007 21, 3, 0x802a, 1
    Plugin8_Cmd1007 22, 3, 0x802a, 2
    Plugin8_Cmd1007 23, 3, 0x802a, 3
    SEPlay SEQ_SE_SW_JA_01
    SEPlay SEQ_SE_SW_JA_02
    // "Assistant [f000]Ȁ\u0001\u0004: [f000]Ā\u0001\u0000\nMet on [f000]ȁ\u0001\u0002/[f000]ȁ\u0001\u0003/20[f000]ȁ\u0001\u0001"
    SystemMsg JoinAvenue_Text_AssistantMet20, 1
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkAdd 0x802a, 1
    VMJump L_0B21

L_0C19:
    VMSleep 10
    Plugin8_Cmd1007 8, 255, 0, 0
    Plugin8_Cmd1007 11, 255, 0, 1
    Plugin8_Cmd1007 4, 255, 0, 2
    SEPlay SEQ_SE_SW_JA_01
    SEPlay SEQ_SE_SW_JA_02
    // "[f000]ĺ\u0001\u0002's [f000]Ĺ\u0001\u0000\nRank [f000]Ȃ\u0001\u0001"
    SystemMsg JoinAvenue_Text_SRank, 1
    SEWait
    MsgWaitAdvance
    RecordGet 127, 0x8029
    WordSetNumber 0, 0x8029, 7
    RecordGet 128, 0x8029
    WordSetNumber 1, 0x8029, 7
    Plugin8_Cmd1007 12, 255, 0, 2
    // "People recommended: [f000]Ȇ\u0001\u0000\nPeople invited: [f000]Ȇ\u0001\u0001[f000]븁\u0000\nPopularity of the avenue:\n“[f000]ł\u0001\u0002\"[f000]븁\u0000"
    SystemMsg JoinAvenue_Text_PeopleRecommendedPeopleInvited, 1
    Plugin8_Cmd1007 8, 255, 0, 0
    Plugin8_Cmd1002 63, 0x8029
    WorkAdd 0x8029, 36
    SystemMsg 0x8029, 1
    InfoMsgClose
    SEPlay SEQ_SE_MSCL_09
    FadeOutWhite
    FadeWait
    SEWait
    EvCameraMoveToDefault 1
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    Plugin8_Cmd1039 0
    Plugin8_Cmd1008 1, 3, 0
    Plugin8_Cmd1008 1, 3, 1
    Plugin8_Cmd1008 1, 3, 2
    Plugin8_Cmd1008 1, 3, 3
    WorkSetConst EVENT_WORK_0x413a, 2
    FlagReset EVENT_FLAG_0x09f1
    MapChangeCore ZONE_JOIN_AVENUE_2, 8, 0, 8, 1
    FadeInWhite
    FadeWait
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0D20:
    Move 12, 7
    MoveEnd

Movement_0D28:
    Move 12, 5
    MoveEnd

Movement_0D30:
    Move 1, 1
    Move 61, 1
    Move 2, 1
    Move 61, 1
    Move 0, 1
    Move 61, 1
    Move 3, 1
    Move 61, 1
    Move 1, 1
    MoveEnd

Script_12:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
