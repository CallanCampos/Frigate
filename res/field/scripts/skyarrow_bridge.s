#include "asm/field_script.inc"
#include "text/script/skyarrow_bridge.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hello. Excuse me.[f000]븁\u0000\nI'm terribly sorry to ask, but\nwill you buy a bottle of Fresh Water[f000]븀\u0000\nfor $300?"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_HelloExcuseImTerribly, 0, 2, 0
    MoneyWinDisp 31, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0065
    VMCall L_0179
    VMJump L_0173

L_0065:
    MoneyWinClose
    // "Yes, I know.\nI know![f000]븁\u0000\n$300 a bottle is expensive![f000]븁\u0000\nBut, if I don't sell this,\nI'll be in trouble...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_YesKnowKnow300, 0, 2, 0
    Random EVENT_WORK_0x4000, 5
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_009E
    // "My wife wants to buy a Bag\nfrom a really expensive brand...[f000]븁\u0000\nIf I don't buy it for her,\nour relationship might go sour...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_WifeWantsBuyBag, 0, 2, 0
    VMJump L_012C

L_009E:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C3
    // "I have a daughter who's a Trainer.[f000]븁\u0000\nIt's natural for a father to want to buy\na lot of things like new Ultra Balls and[f000]븀\u0000\nHyper Potions for his daughter...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_HaveDaughterWhosTrainer, 0, 2, 0
    VMJump L_012C

L_00C3:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E8
    // "My son and his wife send me money,\nbut they can't send very much...[f000]븁\u0000\nI know I shouldn't rely on them,\nbut there is a lot going on.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_SonHisWifeSend, 0, 2, 0
    VMJump L_012C

L_00E8:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_010D
    // "I want to buy some toys\nfor my adorable grandchildren.[f000]븁\u0000\nTheir tastes are quite expensive.\nBut I want to see their happy faces...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_WantBuySomeToys, 0, 2, 0
    VMJump L_012C

L_010D:
    VMStackPush EVENT_WORK_0x4000
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012C
    // "I don't really have\nany special reasons...[f000]븁\u0000\nBut the full-course meal I ate\nyesterday was incredibly expensive![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_DontReallyHaveAny, 0, 2, 0

L_012C:
    // "So, to help this old man...[f000]븁\u0000\nPlease buy my Fresh Water\nfor $300. Please."
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_HelpOldManPlease, 0, 2, 0
    MoneyWinDisp 31, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0161
    VMCall L_0179
    VMJump L_0173

L_0161:
    MoneyWinClose
    // "Sigh...[f000]븁\u0000\nWell, since you're stubborn,\nI'll stop asking.[f000]븁\u0000\nBut when you change your mind,\nplease come back anytime.[f000]븁\u0000\nI'll be selling Fresh Water\nfor $300 right here!"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_SighWellSinceYoure, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0173:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0179:
    ItemCheckSpace ITEM_FRESH_WATER, 1, 0x8022
    MoneyCheck 0x8021, 300
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B2
    MoneyWinClose
    // "Y-you... $300...\nYou don't have it?![f000]븁\u0000\nI didn't say anything! Don't worry.\nYou don't have to buy Fresh Water."
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_Y300DontHave, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_021B

L_01B2:
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DD
    MoneyWinClose
    // "You... You can't carry any more\nFresh Water![f000]븁\u0000\nWow! You're a Fresh Water maniac!\nPlease buy it from me[f000]븀\u0000\nnext time you need some!"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_CantCarryAnyMore, 0, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_021B

L_01DD:
    SEPlay SEQ_SE_SYS_22
    MoneySub 300
    MoneyWinUpdate
    SEWait
    // "Thank you very much![f000]븁\u0000\nBuying Fresh Water for $300,\nwhich is more expensive than usual...[f000]븀\u0000\nWhat a generous person you are![f000]븁\u0000\nI'll never forget your kindness\nfor the rest of my life!"
    ActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_ThankVeryMuchBuying, 0, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 30
    WorkSet 0x8001, 1
    RTCallGlobal 2802
    VMStackPop 0x8001
    VMStackPop 0x8000

L_021B:
    VMReturn

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The curve here is like...errrrk...\nand then when you pass the curve,[f000]븀\u0000\nwhoooosh, zoooom![f000]븁\u0000\n...Do you understand the dialect\nof Goldenrod City?"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_CurveHereLikeErrrrk, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hmm! That man![f000]븁\u0000\nHe's standing right in the middle of\nthe Skyarrow Bridge... I think."
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_HmmManHesStanding, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Good-bye, Castelia City...\nGood-bye, old me..."
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_GoodByeCasteliaCity, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036F
    // "[f000]븉\u0001\u0002Oh... Oh...\nSo...thirsty...[f000]븁\u0000\nG-g-give me...\nFresh Water...?[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_OhOhThirstyG, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035B
    ItemSub ITEM_FRESH_WATER, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0347
    MsgWinCloseAll
    SEPlay SEQ_SE_ARDEMO_01
    SEWait
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PlayerGetRailPos 0x8023, 0x8024, 0x8025
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0319
    ActorCmdExec 4, Movement_0438
    VMJump L_0321

L_0319:
    ActorCmdExec 4, Movement_044C

L_0321:
    VMSleep 20
    ActorCmdExec 255, Movement_045C
    ActorCmdWait
    ActorDelete 4
    WorkSetConst EVENT_WORK_0x4108, 1
    FlagSet EVENT_FLAG_0x035a
    FlagReset EVENT_FLAG_0x035b
    VMJump L_0355

L_0347:
    // "[f000]븉\u0001\u0002But... You don't have Fresh Water...\nI appreciate the thought, though...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_ButDontHaveFresh, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0355:
    VMJump L_0369

L_035B:
    // "[f000]븉\u0001\u0002Thank...[f000]븁\u0000\nWhat?\nOh...[f000]븁\u0000\nWithout Fresh Water...\nI can't run on bridges anymore.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_ThankWhatOhWithout, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0369:
    VMJump L_0431

L_036F:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0396
    // "Refreshed!![f000]븁\u0000\nI'm 100% rehydrated!\nI feel better now! Thank you![f000]븁\u0000\nI'll dash to the next bridge!"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_RefreshedIm100Rehydrated, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0431

L_0396:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0410
    // "Yay! I ran on all the bridges\nin the Unova region![f000]븁\u0000\nIt's all thanks to you!\nPlease! Accept these![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_YayRanAllBridges, 0, 0
    ItemCheckSpace ITEM_MOOMOO_MILK, 12, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03FC
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 33
    WorkSet 0x8001, 12
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x4108, 7
    VMJump L_040A

L_03FC:
    // "Oh... Your Bag is full."
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_OhBagFull, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_040A:
    VMJump L_0431

L_0410:
    VMStackPush EVENT_WORK_0x4108
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0431
    // "The bridge I like best for running\nis the Tubeline Bridge.[f000]븁\u0000\nI get to race the trains!\nThat gives me power!"
    ParentActorMsg MSGFILE_SCRIPT, SkyarrowBridge_Text_BridgeLikeBestRunning, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0431:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_0438:
    Move 38, 4
    Move 18, 1
    Move 16, 8
    Move 20, 8
    MoveEnd

Movement_044C:
    Move 36, 4
    Move 16, 8
    Move 20, 8
    MoveEnd

Movement_045C:
    Move 0, 1
    MoveEnd
