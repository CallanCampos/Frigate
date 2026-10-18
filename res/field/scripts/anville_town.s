#include "asm/field_script.inc"
#include "text/script/anville_town.h"

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
    ScriptEntriesEnd

Script_3:
    FlagSet EVENT_FLAG_0x02a8
    FlagSet EVENT_FLAG_0x02a9
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00A4
    RTCGetWeekDay 0x8010
    VMStackPush 0x8010
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00A4
    FlagReset EVENT_FLAG_0x02a8
    FlagReset EVENT_FLAG_0x02a9

L_00A4:
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 9
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 10
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00CB
    FlagReset EVENT_FLAG_0x02a8

L_00CB:
    VMHalt

Script_4:
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F0
    BMSetVisible 8, 24, 25, 0
    VMJump L_0100

L_00F0:
    Cmd_0221 EVENT_WORK_0x4162, 0x8010
    BMChangeMdlID 8, 24, 25, 0x8010

L_0100:
    VMHalt

Script_5:
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0125
    BMSetVisible 8, 24, 25, 0
    VMJump L_0135

L_0125:
    Cmd_0221 EVENT_WORK_0x4162, 0x8010
    BMChangeMdlID 8, 24, 25, 0x8010

L_0135:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 5208, 0, 0xed000, 0x198000, 0x5004f, 0x250000, 15
    EvCameraWait
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0105
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0192
    // "Unfortunately, no train has\ncome here today.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_UnfortunatelyNoTrainHas, 0, 0
    MsgWinCloseAll
    VMJump L_0257

L_0192:
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01B5
    // "Oh, are you also curious about\nthat train?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhAlsoCuriousAbout, 0, 0
    VMJump L_01BF

L_01B5:
    // "See the train there?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_SeeTrainThere, 0, 0

L_01BF:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0228
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_01F9
    // "All righty!\nLet me explain![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_AllRightyLetExplain, 0, 0
    VMJump L_0203

L_01F9:
    // "All righty!\nAll aboard![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_AllRightyAllAboard, 0, 0

L_0203:
    MsgWinCloseAll
    VMCall L_0267
    VMStackPushFlag EVENT_FLAG_0x0105
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0222
    FlagSet EVENT_FLAG_0x0105

L_0222:
    VMJump L_0257

L_0228:
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_024B
    // "Oh, that's a shame.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhThatsShame, 0, 0
    VMJump L_0255

L_024B:
    // "Hahaha!\nYou're right.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_HahahaYoureRight, 0, 0

L_0255:
    MsgWinCloseAll

L_0257:
    EvCameraMoveToDefault 15
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0267:
    WorkSetConst 0x8020, 0
    MsgSetAutoscrolls 1
    FadeEx 3, 0, 16, 4
    FadeExWait
    EvCameraMoveTo 6616, 60032, 0xed000, 0x16f000, 0x47b1f, 0x1db000, 1
    EvCameraWait
    EvCameraMoveTo 6616, 3968, 0xed000, 0x1b6000, 0x47b1f, 0x1db000, 160
    FadeEx 3, 16, 0, 4
    FadeExWait
    WorkSetConst 0x8020, 14
    WorkAdd 0x8020, EVENT_WORK_0x4162
    InfoMsg 0x8020, 1
    WorkCmpConst EVENT_WORK_0x4162, 0
    VMJumpIf CMP_EQ, L_02DF
    VMJump L_02E9

L_02DF:
    VMSleep 30
    VMJump L_03F2

L_02E9:
    WorkCmpConst EVENT_WORK_0x4162, 1
    VMJumpIf CMP_EQ, L_02FC
    VMJump L_0306

L_02FC:
    VMSleep 25
    VMJump L_03F2

L_0306:
    WorkCmpConst EVENT_WORK_0x4162, 2
    VMJumpIf CMP_EQ, L_0319
    VMJump L_0323

L_0319:
    VMSleep 25
    VMJump L_03F2

L_0323:
    WorkCmpConst EVENT_WORK_0x4162, 3
    VMJumpIf CMP_EQ, L_0336
    VMJump L_0340

L_0336:
    VMSleep 30
    VMJump L_03F2

L_0340:
    WorkCmpConst EVENT_WORK_0x4162, 4
    VMJumpIf CMP_EQ, L_0353
    VMJump L_035D

L_0353:
    VMSleep 30
    VMJump L_03F2

L_035D:
    WorkCmpConst EVENT_WORK_0x4162, 5
    VMJumpIf CMP_EQ, L_0370
    VMJump L_037A

L_0370:
    VMSleep 30
    VMJump L_03F2

L_037A:
    WorkCmpConst EVENT_WORK_0x4162, 6
    VMJumpIf CMP_EQ, L_038D
    VMJump L_0397

L_038D:
    VMSleep 30
    VMJump L_03F2

L_0397:
    WorkCmpConst EVENT_WORK_0x4162, 7
    VMJumpIf CMP_EQ, L_03AA
    VMJump L_03B4

L_03AA:
    VMSleep 30
    VMJump L_03F2

L_03B4:
    WorkCmpConst EVENT_WORK_0x4162, 8
    VMJumpIf CMP_EQ, L_03C7
    VMJump L_03D1

L_03C7:
    VMSleep 45
    VMJump L_03F2

L_03D1:
    WorkCmpConst EVENT_WORK_0x4162, 9
    VMJumpIf CMP_EQ, L_03E4
    VMJump L_03EE

L_03E4:
    VMSleep 55
    VMJump L_03F2

L_03EE:
    VMSleep 55

L_03F2:
    FadeEx 3, 0, 16, 4
    FadeExWait
    MsgWinCloseAll
    EvCameraMoveTo 7000, 9600, 0xed000, 0x1b4000, 0x47b1f, 0x1da6a0, 1
    EvCameraWait
    EvCameraMoveTo 7000, 9600, 0xed000, 0x1b4000, 0x47b1f, 0x12a000, 180
    FadeEx 3, 16, 0, 4
    FadeExWait
    WorkSetConst 0x8020, 25
    WorkAdd 0x8020, EVENT_WORK_0x4162
    InfoMsg 0x8020, 1
    WorkCmpConst EVENT_WORK_0x4162, 0
    VMJumpIf CMP_EQ, L_0462
    VMJump L_046C

L_0462:
    VMSleep 30
    VMJump L_0575

L_046C:
    WorkCmpConst EVENT_WORK_0x4162, 1
    VMJumpIf CMP_EQ, L_047F
    VMJump L_0489

L_047F:
    VMSleep 80
    VMJump L_0575

L_0489:
    WorkCmpConst EVENT_WORK_0x4162, 2
    VMJumpIf CMP_EQ, L_049C
    VMJump L_04A6

L_049C:
    VMSleep 30
    VMJump L_0575

L_04A6:
    WorkCmpConst EVENT_WORK_0x4162, 3
    VMJumpIf CMP_EQ, L_04B9
    VMJump L_04C3

L_04B9:
    VMSleep 30
    VMJump L_0575

L_04C3:
    WorkCmpConst EVENT_WORK_0x4162, 4
    VMJumpIf CMP_EQ, L_04D6
    VMJump L_04E0

L_04D6:
    VMSleep 30
    VMJump L_0575

L_04E0:
    WorkCmpConst EVENT_WORK_0x4162, 5
    VMJumpIf CMP_EQ, L_04F3
    VMJump L_04FD

L_04F3:
    VMSleep 30
    VMJump L_0575

L_04FD:
    WorkCmpConst EVENT_WORK_0x4162, 6
    VMJumpIf CMP_EQ, L_0510
    VMJump L_051A

L_0510:
    VMSleep 30
    VMJump L_0575

L_051A:
    WorkCmpConst EVENT_WORK_0x4162, 7
    VMJumpIf CMP_EQ, L_052D
    VMJump L_0537

L_052D:
    VMSleep 30
    VMJump L_0575

L_0537:
    WorkCmpConst EVENT_WORK_0x4162, 8
    VMJumpIf CMP_EQ, L_054A
    VMJump L_0554

L_054A:
    VMSleep 35
    VMJump L_0575

L_0554:
    WorkCmpConst EVENT_WORK_0x4162, 9
    VMJumpIf CMP_EQ, L_0567
    VMJump L_0571

L_0567:
    VMSleep 35
    VMJump L_0575

L_0571:
    VMSleep 45

L_0575:
    WorkCmpConst EVENT_WORK_0x4162, 0
    VMJumpIf CMP_EQ, L_0588
    VMJump L_058E

L_0588:
    VMJump L_0697

L_058E:
    WorkCmpConst EVENT_WORK_0x4162, 1
    VMJumpIf CMP_EQ, L_05A1
    VMJump L_05AB

L_05A1:
    FlagSet EVENT_FLAG_0x0a76
    VMJump L_0697

L_05AB:
    WorkCmpConst EVENT_WORK_0x4162, 2
    VMJumpIf CMP_EQ, L_05BE
    VMJump L_05C8

L_05BE:
    FlagSet EVENT_FLAG_0x0a77
    VMJump L_0697

L_05C8:
    WorkCmpConst EVENT_WORK_0x4162, 3
    VMJumpIf CMP_EQ, L_05DB
    VMJump L_05E5

L_05DB:
    FlagSet EVENT_FLAG_0x0a78
    VMJump L_0697

L_05E5:
    WorkCmpConst EVENT_WORK_0x4162, 4
    VMJumpIf CMP_EQ, L_05F8
    VMJump L_0602

L_05F8:
    FlagSet EVENT_FLAG_0x0a79
    VMJump L_0697

L_0602:
    WorkCmpConst EVENT_WORK_0x4162, 5
    VMJumpIf CMP_EQ, L_0615
    VMJump L_061F

L_0615:
    FlagSet EVENT_FLAG_0x0a7a
    VMJump L_0697

L_061F:
    WorkCmpConst EVENT_WORK_0x4162, 6
    VMJumpIf CMP_EQ, L_0632
    VMJump L_063C

L_0632:
    FlagSet EVENT_FLAG_0x0a7b
    VMJump L_0697

L_063C:
    WorkCmpConst EVENT_WORK_0x4162, 7
    VMJumpIf CMP_EQ, L_064F
    VMJump L_0659

L_064F:
    FlagSet EVENT_FLAG_0x0a7c
    VMJump L_0697

L_0659:
    WorkCmpConst EVENT_WORK_0x4162, 8
    VMJumpIf CMP_EQ, L_066C
    VMJump L_0676

L_066C:
    FlagSet EVENT_FLAG_0x0a7d
    VMJump L_0697

L_0676:
    WorkCmpConst EVENT_WORK_0x4162, 9
    VMJumpIf CMP_EQ, L_0689
    VMJump L_0693

L_0689:
    FlagSet EVENT_FLAG_0x0a7e
    VMJump L_0697

L_0693:
    FlagSet EVENT_FLAG_0x0a7f

L_0697:
    FadeEx 3, 0, 16, 4
    FadeExWait
    MsgWinCloseAll
    EvCameraMoveTo 5208, 0, 0xed000, 0x198000, 0x5004f, 0x250000, 1
    EvCameraWait
    FadeEx 3, 16, 0, 4
    FadeExWait
    MsgSetAutoscrolls 0
    WorkSetConst 0x8020, 36
    WorkAdd 0x8020, EVENT_WORK_0x4162
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    MsgWinCloseAll
    WorkSetConst 0x8020, 0
    VMReturn
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0

Script_18:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8022, 78
    WorkSetConst 0x8024, 2
    WorkSetConst 0x8021, 28
    WorkSetConst 0x8023, 1
    WordSetItemNameEx 0, 0x8022, 0x8024, 0
    WordSetNumber 1, 0x8024, 2
    WordSetItemNameEx 2, 0x8021, 0x8023, 0
    WordSetNumber 3, 0x8023, 2
    VMStackPush EVENT_WORK_0x400b
    VMStackPushConst 111
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0774
    // "Ah... I'm in trouble.[f000]븁\u0000\nI was so engrossed in my trip that I ran\nout of [f000]ĉ\u0001\u0000.[f000]븁\u0000\nLet me see...\nWill you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000[f000]븀\u0000\nfor my [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_AhImTroubleEngrossed, 0, 0
    WorkSetConst EVENT_WORK_0x400b, 111
    VMJump L_077E

L_0774:
    // "Will you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000\nfor my [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WillTrade, 0, 0

L_077E:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0826
    VMCall L_083A
    WordSetItemName 0, 0x8022
    WordSetItemName 2, 0x8021
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_07B8
    VMJump L_07C8

L_07B8:
    // "...Uh-oh. You don't have\nenough [f000]ĉ\u0001\u0000s."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_UhOhDontHave, 0, 0
    VMJump L_0820

L_07C8:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_07DB
    VMJump L_07EB

L_07DB:
    // "...Uh-oh. You don't have enough room for\nthe [f000]ĉ\u0001\u0002."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_UhOhDontHave_2, 0, 0
    VMJump L_0820

L_07EB:
    // "Great, let's trade![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_GreatLetsTrade, 0, 0
    MsgWinCloseAll
    VMCall L_0898
    // "Hehe! It was a delightful trade,\nwasn't it?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_HeheDelightfulTradeWasnt, 0, 0
    VMStackPushFlag EVENT_FLAG_0x0995
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0820
    VMCall L_0B1E

L_0820:
    VMJump L_0830

L_0826:
    // "Oh, that's a shame."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhThatsShame_2, 0, 0

L_0830:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_083A:
    ItemCheckAmount 0x8022, 0x8024, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_085D
    WorkSetConst 0x8025, 0
    VMReturn

L_085D:
    ItemCheckSpace 0x8021, 0x8023, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0880
    WorkSetConst 0x8025, 1
    VMReturn

L_0880:
    ItemSub 0x8022, 0x8024, 0x8010
    ItemAdd 0x8021, 0x8023, 0x8010
    WorkSetConst 0x8025, 2
    VMReturn

L_0898:
    WordSetItemNameEx 0, 0x8022, 0x8024, 0
    WordSetItemNameEx 2, 0x8021, 0x8023, 0
    MEPlay SEQ_ME_ITEM
    // "Gave the [f000]ĉ\u0001\u0000 in exchange for\nthe [f000]ĉ\u0001\u0002!"
    SystemMsg AnvilleTown_Text_GaveExchange, 0
    MEWait
    MsgWaitAdvance
    MsgWinCloseAll
    WordSetItemName 0, 0x8022
    WordSetItemName 2, 0x8021
    VMReturn

Script_19:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8022, 91
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8021, 51
    WorkSetConst 0x8023, 1
    WordSetItemNameEx 0, 0x8022, 0x8024, 0
    WordSetNumber 1, 0x8024, 2
    WordSetItemNameEx 2, 0x8021, 0x8023, 0
    WordSetNumber 3, 0x8023, 2
    VMStackPush EVENT_WORK_0x400c
    VMStackPushConst 111
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_092B
    // "I am a [f000]ĉ\u0001\u0000 collector![f000]븁\u0000\nYou, over there! Let's have a\nbusinesslike exchange.[f000]븁\u0000\nWill you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000 for\nmy [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_AmCollectorOverThere, 0, 0
    WorkSetConst EVENT_WORK_0x400c, 111
    VMJump L_0935

L_092B:
    // "Will you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000\nfor my [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WillTrade_2, 0, 0

L_0935:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09DD
    VMCall L_083A
    WordSetItemName 0, 0x8022
    WordSetItemName 2, 0x8021
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_096F
    VMJump L_097F

L_096F:
    // "Hah! You don't have\nany [f000]ĉ\u0001\u0000s."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_HahDontHaveAny, 0, 0
    VMJump L_09D7

L_097F:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0992
    VMJump L_09A2

L_0992:
    // "Hah! You don't have enough room for\nthe [f000]ĉ\u0001\u0002."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_HahDontHaveEnough, 0, 0
    VMJump L_09D7

L_09A2:
    // "Excellent. Let's do business![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_ExcellentLetsBusiness, 0, 0
    MsgWinCloseAll
    VMCall L_0898
    // "Hah! It was a mutually beneficial trade!\nThis is a win-win relationship, right?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_HahMutuallyBeneficialTrade, 0, 0
    VMStackPushFlag EVENT_FLAG_0x0995
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_09D7
    VMCall L_0B1E

L_09D7:
    VMJump L_09E7

L_09DD:
    // "Why? Do you think this is a bad deal?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WhyThinkBadDeal, 0, 0

L_09E7:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8022, 4
    WorkSetConst 0x8024, 20
    WorkSetConst 0x8021, 23
    WorkSetConst 0x8023, 1
    WordSetItemNameEx 0, 0x8022, 0x8024, 0
    WordSetNumber 1, 0x8024, 2
    WordSetItemNameEx 2, 0x8021, 0x8023, 0
    WordSetNumber 3, 0x8023, 2
    VMStackPush EVENT_WORK_0x400d
    VMStackPushConst 111
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0A58
    // "With that face, I bet you have\nsomething I want.[f000]븁\u0000\nWill you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000 for\nmy [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_FaceBetHaveSomething, 0, 0
    WorkSetConst EVENT_WORK_0x400d, 111
    VMJump L_0A62

L_0A58:
    // "Will you trade your [f000]ȁ\u0001\u0001 [f000]ĉ\u0001\u0000 for\nmy [f000]ȁ\u0001\u0003 [f000]ĉ\u0001\u0002?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WillTrade_3, 0, 0

L_0A62:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B0A
    VMCall L_083A
    WordSetItemName 0, 0x8022
    WordSetItemName 2, 0x8021
    WorkCmpConst 0x8025, 0
    VMJumpIf CMP_EQ, L_0A9C
    VMJump L_0AAC

L_0A9C:
    // "Ooh-la-la!\nYou don't have enough [f000]ĉ\u0001\u0000s."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OohLaLaDont, 0, 0
    VMJump L_0B04

L_0AAC:
    WorkCmpConst 0x8025, 1
    VMJumpIf CMP_EQ, L_0ABF
    VMJump L_0ACF

L_0ABF:
    // "Ooh-la-la! You don't have room for\nthe [f000]ĉ\u0001\u0002."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OohLaLaDont_2, 0, 0
    VMJump L_0B04

L_0ACF:
    // "Yay!\nThen, let's trade![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_YayThenLetsTrade, 0, 0
    MsgWinCloseAll
    VMCall L_0898
    // "Ooh-la-la! This luster!\nI really like [f000]ĉ\u0001\u0000s.[f000]븁\u0000\nAn experienced person like me can tell\nthe difference of the luster of each one!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OohLaLaLuster, 0, 0
    VMStackPushFlag EVENT_FLAG_0x0995
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B04
    VMCall L_0B1E

L_0B04:
    VMJump L_0B14

L_0B0A:
    // "Oh, it seems I was mistaken..."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhSeemsMistaken, 0, 0

L_0B14:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0B1E:
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst 0x8026, 0
    GameGetVersion 0x8026
    SEPlay SEQ_SE_FLD_133
    VMStackPush 0x8026
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0B56
    Cmd_0275 0, 11, 0
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg AnvilleTown_Text_FunfestMissionHasBeen, 0
    VMJump L_0B63

L_0B56:
    Cmd_0275 0, 12, 0
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg AnvilleTown_Text_FunfestMissionHasBeen_2, 0

L_0B63:
    SEWait
    FlagSet EVENT_FLAG_0x0995
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A beautiful tune is spreading\nthroughout the town..."
    InfoMsg AnvilleTown_Text_BeautifulTuneSpreadingThroughout, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The girl on the bridge...[f000]븁\u0000\nShe's playing a lullaby for all the\nsleeping trains of this town."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_GirlBridgeShesPlaying, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Trains are so cooooool![f000]븁\u0000\nMy mom brought me, but now she's\ntaking pictures somewhere.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_TrainsCoooooolMomBrought, 0, 0
    MsgWinCloseAll
    ActorCmdExec 4, Movement_0F80
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wanted to get a picture from\nthis angle![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WantedGetPictureFrom, 0, 0
    MsgWinCloseAll
    ActorCmdExec 5, Movement_0F88
    ActorCmdWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's so lively on weekends!\nOh, me? I came here to watch the trains."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_ItsLivelyWeekendsOh, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "You can trade lots of items.\nIt was surprisingly fun when I tried!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_CanTradeLotsItems, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What delicious air!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WhatDeliciousAir, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The device that changes\nthe direction of trains[f000]븀\u0000\nis called a turntable!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_DeviceChangesDirectionTrains, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    RTCGetWeekDay 0x8010
    VMStackPush 0x8010
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0CB8
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CA8
    // "This is a rare weekend when there aren't\nany trains."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_RareWeekendWhenThere, 0, 0
    VMJump L_0CB2

L_0CA8:
    // "They are full of people today, too.\nDo you also trade items?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_TheyFullPeopleToday, 0, 0

L_0CB2:
    VMJump L_0CC2

L_0CB8:
    // "On weekends, a lot of people come here\nto watch the trains."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_WeekendsLotPeopleCome, 0, 0

L_0CC2:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4162
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0CF7
    // "Today all the train cars are resting!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_TodayAllTrainCars, 0, 0
    VMJump L_0D38

L_0CF7:
    RTCGetWeekDay 0x8010
    VMStackPush 0x8010
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0D2E
    // "If there are a lot of people,\nI feel that the trains are happy!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_IfThereLotPeople, 0, 0
    VMJump L_0D38

L_0D2E:
    // "A lot of train cars have a rest here."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_LotTrainCarsHave, 0, 0

L_0D38:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a subway map of the Unova region.[f000]븁\u0000"
    InfoMsg AnvilleTown_Text_ItsSubwayMapUnova, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallPokedexDiploma 2, 0
    FieldOpen
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Anville Town\nRolling Out the Steel Rails"
    MsgPlaceSign AnvilleTown_Text_AnvilleTownRollingOut, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A train to Nimbasa City is leaving\nthe station shortly.[f000]븁\u0000\nWould you like to board?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_TrainNimbasaCityLeaving, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E4C
    // "Then, please get on the train and wait.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_ThenPleaseGetTrain, 0, 0
    MsgWinCloseAll
    PlayerGetDir 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0DDD
    ActorCmdExec 255, Movement_0F90
    ActorCmdWait
    VMJump L_0E22

L_0DDD:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0E0C
    ActorCmdExec 255, Movement_0F9C
    VMSleep 24
    ActorCmdExec 2, Movement_0FB8
    ActorCmdWait
    VMJump L_0E22

L_0E0C:
    ActorCmdExec 255, Movement_0FAC
    VMSleep 8
    ActorCmdExec 2, Movement_0FB8
    ActorCmdWait

L_0E22:
    VMSleep 16
    RTReserveScript 10342
    FadeOutBlackQ
    FadeWait
    VMSleep 15
    SEPlay SEQ_SE_BDEMO_02
    VMSleep 30
    MapChangeCore ZONE_GEAR_STATION_9, 16, 0, 14, 1
    VMJump L_0E5A

L_0E4C:
    // "This is a quiet town!\nRelax and stay for a while."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_QuietTownRelaxStay, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0E5A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_21:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x016d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0362
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0ECA
    // "Oh, Pansage...\nWhere could you be?[f000]븁\u0000\nWe went to the amusement park\nand saw a musical.[f000]븁\u0000\nBut when we were going home,\nPansage got on a different train![f000]븁\u0000\nI wonder where it is now..."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhPansageWhereCould, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPushFlag EVENT_FLAG_0x0360
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0361
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0EC4
    FlagReset EVENT_FLAG_0x0360
    FlagReset EVENT_FLAG_0x0361

L_0EC4:
    VMJump L_0F54

L_0ECA:
    VMStackPushFlag EVENT_FLAG_0x016d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0362
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0F33
    // "Oh! Pokémon Trainer![f000]븁\u0000\nYou looked for my\nPansage, didn't you?[f000]븁\u0000\nThank you so much.\nThis isn't much, but please take it."
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhPokemonTrainerLooked, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 213
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Oh, my silly Pansage.\nA person with green hair told me that[f000]븀\u0000\nPansage's dream is to become a[f000]븀\u0000\nrailroad conductor![f000]븁\u0000\nBut... That guy...\nCan he talk with Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhSillyPansagePerson, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_0x016d
    VMJump L_0F54

L_0F33:
    VMStackPushFlag EVENT_FLAG_0x016d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0F54
    // "Oh, my silly Pansage.\nA person with green hair told me that[f000]븀\u0000\nPansage's dream is to become a[f000]븀\u0000\nrailroad conductor![f000]븁\u0000\nBut... That guy...\nCan he talk with Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_OhSillyPansagePerson, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0F54:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_22:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Ook!"
    ParentActorMsg MSGFILE_SCRIPT, AnvilleTown_Text_Ook, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0F80:
    Move 32, 1
    MoveEnd

Movement_0F88:
    Move 34, 1
    MoveEnd

Movement_0F90:
    Move 13, 1
    Move 35, 1
    MoveEnd

Movement_0F9C:
    Move 14, 1
    Move 13, 3
    Move 15, 1
    MoveEnd

Movement_0FAC:
    Move 13, 2
    Move 15, 1
    MoveEnd

Movement_0FB8:
    Move 33, 1
    MoveEnd
