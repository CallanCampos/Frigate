#include "asm/field_script.inc"
#include "text/script/humilau_city_6.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x000f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    PedometerGet EVENT_WORK_0x400e
    DebugPrint EVENT_WORK_0x400e

L_0051:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "But if you change your mind,\nI don't mind asking you to walk[f000]븀\u0000\nwith my Mienfoo again."
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_ButIfChangeMind, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_032A

L_007E:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x000f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01E3
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey, you!\nWould you walk with my dear Mienfoo?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_HeyWouldWalkDear, 1, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01CD
    WorkSetConst EVENT_WORK_0x400e, 0
    // "Oh my!\nYou're very understanding![f000]븁\u0000\nWonderful. Please walk a lot\nwith my cute Mienfoo![f000]븁\u0000\nBut...\nPlease don't go out of this house![f000]븁\u0000\nIt's dangerous outside.\nAll right. Take good care of my Mienfoo![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_OhYoureVeryUnderstanding, 1, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_00F5
    VMJump L_0111

L_00F5:
    ActorCmdExec 255, Movement_0A70
    ActorWalkRoute 0, 5, 4, 0, 8, 0
    VMJump L_016F

L_0111:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0124
    VMJump L_0140

L_0124:
    ActorCmdExec 255, Movement_0A70
    ActorWalkRoute 0, 5, 2, 0, 8, 0
    VMJump L_016F

L_0140:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0153
    VMJump L_016F

L_0153:
    ActorCmdExec 255, Movement_0A60
    ActorWalkRoute 0, 7, 2, 0, 8, 0
    VMJump L_016F

L_016F:
    ActorCmdWait
    VMStackPush 0x8000
    VMStackPush 0x8001
    VMStackPush 0x8002
    VMStackPush 0x8003
    VMStackPush 0x8004
    WorkSet 0x8000, 0
    WorkSet 0x8001, 4
    WorkSet 0x8002, 0
    WorkSet 0x8003, 0
    WorkSet 0x8004, 2
    RTCallGlobal 10535
    VMStackPop 0x8004
    VMStackPop 0x8003
    VMStackPop 0x8002
    VMStackPop 0x8001
    VMStackPop 0x8000
    PedometerStart
    WorkSetConst EVENT_WORK_0x400f, 1
    FlagSet EVENT_FLAG_0x000f
    VMJump L_01DD

L_01CD:
    // "Oh my![f000]븁\u0000\nYou turned down my request.\nYou're mean.[f000]븁\u0000\nSome people say that I should walk\nmy Mienfoo myself.[f000]븁\u0000\nBut, it's impossible, because I've never\ncarried anything heavier than[f000]븀\u0000\na Poké Ball![f000]븁\u0000\n...But if you change your mind,\nI don't mind asking you to walk[f000]븀\u0000\nmy Mienfoo again."
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_OhTurnedDownRequest, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01DD:
    VMJump L_032A

L_01E3:
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 365
    VMStackCmp CMP_LT
    VMStackPushFlag EVENT_FLAG_0x000f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_02AC
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8025, 0
    // "You've just started walking.\nPlease walk more![f000]븁\u0000\n...Whaaat?[f000]븁\u0000\nYou're not going to say\nyou will quit in the middle of[f000]븀\u0000\nwalking my cute Mienfoo, are you?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_YouveJustStartedWalking, 1, 4, 0
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32805
    ListMenuAdd 6, 65535, 0
    ListMenuAdd 7, 65535, 1
    ListMenuShow
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0296
    // "Oh my!\nWhat's the matter with you?[f000]븁\u0000\nIt looks like my cute Mienfoo still\nwants to walk![f000]븁\u0000\nIn that case, I can't give you a\nthank-you gift.[f000]븁\u0000\n...But if you change your mind,\nI don't mind asking you to walk[f000]븀\u0000\nwith my Mienfoo again.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_OhWhatsMatterLooks, 1, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMCall L_0330
    PedometerEnd
    WorkSetConst EVENT_WORK_0x400f, 0
    FlagSet EVENT_FLAG_DAILY_0x0ad0
    FlagReset EVENT_FLAG_0x000f
    VMJump L_02A6

L_0296:
    // "Of course![f000]븁\u0000\nPlease walk my cute Mienfoo\nuntil it is totally satisfied."
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_CoursePleaseWalkCute, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02A6:
    VMJump L_032A

L_02AC:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh my![f000]븁\u0000\nMy cute Mienfoo\nlooks very tough now.[f000]븁\u0000\nThank you very much\nfor walking my Mienfoo.[f000]븁\u0000\nI'll give this to you\nas a token of my appreciation.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_OhCuteMienfooLooks, 1, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 88
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 2
    WorkSet 0x8001, 0
    RTCallGlobal 10536
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMCall L_0330
    // "Please walk my cute Mienfoo again!"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_PleaseWalkCuteMienfoo, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    PedometerEnd
    WorkSetConst EVENT_WORK_0x400f, 0
    FlagSet EVENT_FLAG_DAILY_0x0ad0
    FlagSet EVENT_FLAG_DAILY_0x0adf
    FlagReset EVENT_FLAG_0x000f

L_032A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0330:
    PlayerGetGPos 0x8021, 0x8022
    PlayerGetDir 0x8020
    ActorGetGPos 0, 0x8023, 0x8024
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0383
    ActorCmdExec 0, Movement_0684
    VMJump L_0633

L_0383:
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03C4
    ActorCmdExec 0, Movement_0690
    VMJump L_0633

L_03C4:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0405
    ActorCmdExec 0, Movement_06A4
    VMJump L_0633

L_0405:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0446
    ActorCmdExec 0, Movement_06B4
    VMJump L_0633

L_0446:
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0487
    ActorCmdExec 0, Movement_06C8
    VMJump L_0633

L_0487:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04C8
    ActorCmdExec 0, Movement_06D8
    VMJump L_0633

L_04C8:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0509
    ActorCmdExec 0, Movement_06EC
    VMJump L_0633

L_0509:
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_054A
    ActorCmdExec 0, Movement_06FC
    VMJump L_0633

L_054A:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0593
    ActorCmdExec 0, Movement_0708
    ActorCmdExec 255, Movement_0660
    VMJump L_0633

L_0593:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05DC
    ActorCmdExec 0, Movement_0714
    ActorCmdExec 255, Movement_0674
    VMJump L_0633

L_05DC:
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0625
    ActorCmdExec 0, Movement_071C
    ActorCmdExec 255, Movement_0674
    VMJump L_0633

L_0625:
    ActorWalkRoute 0, 5, 3, 1, 8, 0

L_0633:
    ActorCmdWait
    VMReturn
    .balign 4, 0
    Move 0, 1
    Move 71, 1
    Move 13, 1
    Move 72, 1
    MoveEnd
    Move 0, 1
    Move 71, 1
    Move 12, 1
    Move 72, 1
    MoveEnd

Movement_0660:
    Move 14, 1
    Move 13, 1
    Move 15, 2
    Move 32, 1
    MoveEnd

Movement_0674:
    Move 13, 1
    Move 15, 1
    Move 32, 1
    MoveEnd

Movement_0684:
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_0690:
    Move 13, 2
    Move 14, 2
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_06A4:
    Move 14, 2
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_06B4:
    Move 13, 1
    Move 14, 3
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_06C8:
    Move 14, 2
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_06D8:
    Move 13, 1
    Move 14, 2
    Move 12, 2
    Move 35, 1
    MoveEnd

Movement_06EC:
    Move 14, 1
    Move 12, 2
    Move 35, 1
    MoveEnd

Movement_06FC:
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_0708:
    Move 12, 1
    Move 35, 1
    MoveEnd

Movement_0714:
    Move 15, 1
    MoveEnd

Movement_071C:
    Move 13, 1
    Move 35, 1
    MoveEnd

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_0x0966
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad0
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_076F
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 619, 0
    // "...Yeep?"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_Yeep, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_08B0

L_076F:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0adf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07B4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 619, 0
    // "Yeeeep. ♪"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_Yeeeep, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_08B0

L_07B4:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ad0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0adf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07F9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 619, 0
    // "Yeep!"
    ParentActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_Yeep_2, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_08B0

L_07F9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    DebugPrint EVENT_WORK_0x400e
    WordSetPlayerName 0
    PedometerGet EVENT_WORK_0x400e
    PVPlay 619, 0
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 99
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0835
    // "The steps of the Mienfoo walking\nwith [f000]Ā\u0001\u0000 are somewhat clumsy."
    SystemMsg HumilauCity6_Text_StepsMienfooWalkingSomewhat, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08B0

L_0835:
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 199
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_085A
    // "The steps of the Mienfoo walking\nwith [f000]Ā\u0001\u0000 are still clumsy."
    SystemMsg HumilauCity6_Text_StepsMienfooWalkingStill, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08B0

L_085A:
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 299
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_087F
    // "The steps of the Mienfoo walking\nwith [f000]Ā\u0001\u0000 are getting smooth."
    SystemMsg HumilauCity6_Text_StepsMienfooWalkingGetting, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08B0

L_087F:
    VMStackPush EVENT_WORK_0x400e
    VMStackPushConst 364
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_08A4
    // "The steps of the Mienfoo walking\nwith [f000]Ā\u0001\u0000 are light!"
    SystemMsg HumilauCity6_Text_StepsMienfooWalkingLight, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08B0

L_08A4:
    // "The steps of the Mienfoo walking\nwith [f000]Ā\u0001\u0000 are very light![f000]븁\u0000\nMienfoo seems to be\nsatisfied with the walk!"
    SystemMsg HumilauCity6_Text_StepsMienfooWalkingVery, 2
    PVWait
    LastKeyWait
    MsgWinCloseAll

L_08B0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x000f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08DF
    // "Is this a home video?\nMienfoo is in it!"
    SystemMsg HumilauCity6_Text_HomeVideoMienfoo, 2
    LastKeyWait
    MsgWinCloseAll
    VMJump L_091A

L_08DF:
    ActorCmdExec 1, Movement_0A38
    ActorCmdWait
    SEPlay SEQ_SE_SYS_58
    // "Hey, you!"
    ScreamMsg HumilauCity6_Text_Hey, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose_0039
    ActorCmdExec 255, Movement_0A60
    ActorCmdWait
    // "What are you doing?[f000]븁\u0000\nIn front of my very eyes,\nyou disrupt Mienfoo's walk...[f000]븁\u0000\nOn top of that, you got engrossed\nin watching TV.[f000]븀\u0000\nWhat nerve![f000]븁\u0000\nStop taking a break, and walk\nmy Mienfoo![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_WhatDoingFrontVery, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0A60
    ActorCmdWait

L_091A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    VMStackPushFlag EVENT_FLAG_0x000f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0949
    // "A somewhat expensive-looking...\nbut ordinary trash can."
    SystemMsg HumilauCity6_Text_SomewhatExpensiveLookingBut, 2
    LastKeyWait
    MsgWinCloseAll
    VMJump L_09A2

L_0949:
    ActorCmdExec 1, Movement_0A50
    ActorCmdWait
    SEPlay SEQ_SE_SYS_58
    // "Hey, you!"
    ScreamMsg HumilauCity6_Text_Hey, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose_0039
    ActorWalkRoute 1, 7, 5, 1, 4, 1
    ActorCmdExec 255, Movement_0A68
    ActorCmdWait
    // "What are you doing?[f000]븁\u0000\nYou have the audacity to check\nthe trash can in my house.[f000]븁\u0000\nIt's not good for the education of\nmy Mienfoo.[f000]븁\u0000\nNo matter how many times you check,\nthe trash can is empty![f000]븁\u0000\nPlease focus on walking![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_WhatDoingHaveAudacity, 1, 0, 0
    MsgWinCloseAll
    ActorWalkRoute 1, 6, 3, 1, 8, 0
    ActorCmdWait
    ActorCmdExec 1, Movement_0A60
    ActorCmdWait

L_09A2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_SYS_58
    ActorCmdExec 1, Movement_0A40
    ActorCmdWait
    // "Hey, you!"
    ScreamMsg HumilauCity6_Text_Hey, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose_0039
    ActorWalkRoute 1, 5, 6, 1, 4, 1
    ActorCmdExec 255, Movement_0A58
    ActorCmdWait
    // "What are you doing?[f000]븁\u0000\nI can understand very well\nthat my Mienfoo is so cute[f000]븀\u0000\nthat you want to take it out,[f000]븀\u0000\nbut you can't do that![f000]븁\u0000\nIt's dangerous outside![f000]븁\u0000\nWill you take responsibility\nif my Mienfoo gets hurt?[f000]븁\u0000\nPlease walk INSIDE the room![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, HumilauCity6_Text_WhatDoingCanUnderstand, 1, 0, 0
    MsgWinCloseAll
    ActorPairSetMoveEnable 1
    ActorWalkRoute 1, 6, 3, 1, 8, 0
    ActorCmdExec 255, Movement_0A20
    ActorCmdWait
    ActorCmdExec 1, Movement_0A60
    ActorCmdWait
    ActorPairSetMoveEnable 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd

Movement_0A20:
    Move 12, 1
    MoveEnd
    Move 15, 1
    MoveEnd
    Move 14, 1
    MoveEnd

Movement_0A38:
    Move 0, 1
    MoveEnd

Movement_0A40:
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0

Movement_0A50:
    Move 3, 1
    MoveEnd

Movement_0A58:
    Move 32, 1
    MoveEnd

Movement_0A60:
    Move 33, 1
    MoveEnd

Movement_0A68:
    Move 34, 1
    MoveEnd

Movement_0A70:
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
