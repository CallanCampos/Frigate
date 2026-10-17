#include "asm/field_script.inc"
#include "text/script/shopping_mall.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Shopping Mall Nine\nColorful and wonderful!"
    InfoMsg ShoppingMall_Text_ShoppingMallNineColorful, 2
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you want all the TMs, the\nShopping Mall is a must-visit![f000]븁\u0000\nThat's right! You should brag about\ncoming here to the TM Collector[f000]븀\u0000\nin Mistralton City!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_IfWantAllTms, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "On the first floor, they sell medicines.[f000]븁\u0000\nOn the second floor, they sell TMs\nand different kinds of Mail.[f000]븁\u0000\nOn the third floor, they sell items for\nraising stats and battle items.[f000]븁\u0000\nI really admire Mr. Clyde, the\nPokémon Gym guide, so I'm practicing[f000]븀\u0000\nbeing a guide, too!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_FirstFloorTheySell, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I was challenged to a Pokémon\nbattle inside the Shopping Mall![f000]븁\u0000\nThat means it's a place where\nonly the strong survive. Got it?"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ChallengedPokemonBattleInside, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The thing I buy most often is the item\nthat raises a Pokémon's HP, called HP Up.[f000]븁\u0000\nThere are also a lot of other items that\ncatch my eye."
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ThingBuyMostOften, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    RTCGetDayPart 0x8020
    RTCGetWeekDay 0x8021
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E1
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_0115
    VMJump L_0121

L_0115:
    VMCall L_02EC
    VMJump L_01DB

L_0121:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_0134
    VMJump L_0140

L_0134:
    VMCall L_01FB
    VMJump L_01DB

L_0140:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_0153
    VMJump L_015F

L_0153:
    VMCall L_02EC
    VMJump L_01DB

L_015F:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0172
    VMJump L_017E

L_0172:
    VMCall L_01FB
    VMJump L_01DB

L_017E:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_0191
    VMJump L_019D

L_0191:
    VMCall L_02EC
    VMJump L_01DB

L_019D:
    WorkCmpConst 0x8021, 5
    VMJumpIf CMP_EQ, L_01B0
    VMJump L_01BC

L_01B0:
    VMCall L_01FB
    VMJump L_01DB

L_01BC:
    WorkCmpConst 0x8021, 6
    VMJumpIf CMP_EQ, L_01CF
    VMJump L_01DB

L_01CF:
    VMCall L_02EC
    VMJump L_01DB

L_01DB:
    VMJump L_01F5

L_01E1:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I can't wait for evening!\nThat's right! Evening itself is a fever!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_CantWaitEveningThats, 0, 0
    LastKeyWait
    ActorMsgClose

L_01F5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01FB:
    WorkSetConst 0x8022, 180
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's tonight's special evening deal![f000]븁\u0000\nPoké Balls are on sale![f000]븁\u0000\nFor an a-m-a-z-i-n-g...[f000]븁\u0000\n10% off!\nThey're on sale now!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ItsTonightsSpecialEvening, 3, 2, 0
    MoneyWinDisp 31, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D8
    ItemCheckSpace ITEM_POKE_BALL, 1, 0x8024
    MoneyCheck 0x8023, 0x8022
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0269
    MoneyWinClose
    // "What? You don't have enough money,\neven when it's 10% off?!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhatDontHaveEnough, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D2

L_0269:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0294
    MoneyWinClose
    // "What? You can't hold any more\neven when you'd get 10% off?!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhatCantHoldAny, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02D2

L_0294:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8022
    MoneyWinUpdate
    SEWait
    // "Well, at 10% off, your\nPoké Ball purchase will be $180!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_Well10OffPoke, 3, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 4
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000

L_02D2:
    VMJump L_02EA

L_02D8:
    MoneyWinClose
    // "OK, then that will be $0 for no items![f000]븁\u0000\nThanks for nothing!\nPlease come again!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_OkThenWill0, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_02EA:
    VMReturn

L_02EC:
    WorkSetConst 0x8022, 270
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "It's tonight's special evening deal![f000]븁\u0000\nPotions are on sale![f000]븁\u0000\nFor an a-m-a-z-i-n-g...[f000]븁\u0000\n10% off!\nThey're on sale now!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ItsTonightsSpecialEvening_2, 3, 2, 0
    MoneyWinDisp 31, 1
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03C9
    ItemCheckSpace ITEM_POTION, 1, 0x8024
    MoneyCheck 0x8023, 0x8022
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035A
    MoneyWinClose
    // "What? You don't have enough money,\neven when it's 10% off?!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhatDontHaveEnough, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03C3

L_035A:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0385
    MoneyWinClose
    // "What? You can't hold any more\neven when you'd get 10% off?!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhatCantHoldAny, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_03C3

L_0385:
    SEPlay SEQ_SE_SYS_22
    MoneySub 0x8022
    MoneyWinUpdate
    SEWait
    // "Well, at 10% off, your\nPotion purchase will be $270!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_Well10OffPotion, 3, 2, 0
    MsgWaitAdvance
    MsgWinCloseAll
    MoneyWinClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000

L_03C3:
    VMJump L_03DB

L_03C9:
    MoneyWinClose
    // "OK, then that will be $0 for no items![f000]븁\u0000\nThanks for nothing!\nPlease come again!"
    ActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_OkThenWill0, 3, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_03DB:
    VMReturn

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The better Trainer you are,\nthe more you'll find yourself thinking![f000]븁\u0000\nThat's so you can help your\nPokémon partners win!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_BetterTrainerMoreYoull, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If I were to play hide-and-seek with\nmy Pokémon, they'd find me right away![f000]븁\u0000\nBut if I were to get lost,\nI'd be glad they could find me right away!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_IfWerePlayHide, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Shopping Mall Nine!\nIt's called Nine because it's on Route 9.[f000]븁\u0000\nYou can remember it by thinking of this:\n“Done shopping? Nein!\""
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WelcomeShoppingMallNine, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My Lampent's not good at\nPokémon battles,[f000]븀\u0000\nbut it's great at making toast!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_LampentsNotGoodPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush EVENT_WORK_0x4184
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04DC
    // "I'm a Lady...[f000]븁\u0000\nI've come to do some shopping,\nbut I'm having ever so much trouble[f000]븀\u0000\nfinding out what is sold where.[f000]븁\u0000\nWould you be so kind as to buy\na Hyper Potion for me?[f000]븀\u0000\nI'll give you the money for it."
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ImLadyIveCome, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04C8
    // "Why, thank you!\nHere is the money for it.[f000]븁\u0000\nJust a Hyper Potion, please.\nThank you ever so much for your help."
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhyThankHereMoney, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 received $1,200!"
    SystemMsg ShoppingMall_Text_Received1200, 0
    LastKeyWait
    MsgWinCloseAll
    MoneyAdd 1200
    FlagSet EVENT_FLAG_DAILY_0x0acc
    WorkSetConst EVENT_WORK_0x4184, 1
    VMJump L_04D6

L_04C8:
    // "Someone refusing a request from me?\nWhat a novel sensation![f000]븁\u0000\nNow, where could they be\nselling those Hyper Potions?"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_SomeoneRefusingRequestFrom, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_04D6:
    VMJump L_06A2

L_04DC:
    VMStackPush EVENT_WORK_0x4184
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acd
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0681
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0522
    // "I want a Hyper Potion!\nDid you already buy one for me?"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WantHyperPotionDid, 0, 0
    VMJump L_052C

L_0522:
    // "Did I not ask you to buy\na Hyper Potion for me?[f000]븁\u0000\nPlease! I'm waiting.[f000]븁\u0000\nOr did you already buy it, perhaps?"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_DidNotAskBuy, 0, 0

L_052C:
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_066D
    ItemCheckAmount ITEM_HYPER_POTION, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05D2
    // "It's a Hyper Potion!\nThank you for buying it so quickly![f000]븁\u0000\nThis isn't much, but please take it!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ItsHyperPotionThank, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst 0x8025, 0
    ItemSub ITEM_HYPER_POTION, 1, 0x8025
    WorkSetConst 0x8025, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 35
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Shopping is so much fun!\nI'm thrilled with all I've accomplished!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ShoppingMuchFunIm, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0acd
    FlagReset EVENT_FLAG_DAILY_0x0acc
    WorkSetConst EVENT_WORK_0x4184, 0
    VMJump L_0667

L_05D2:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0659
    // "It's a Hyper Potion! It feels as though\nI've been waiting ever so long for this.[f000]븁\u0000\nBut I am happy to finally have it.\nThis isn't much, but please take it."
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ItsHyperPotionFeels, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    WorkSetConst 0x8026, 0
    ItemSub ITEM_HYPER_POTION, 1, 0x8026
    WorkSetConst 0x8026, 0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 34
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Shopping is so much fun!\nI'm thrilled with all I've accomplished!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ShoppingMuchFunIm, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0acd
    FlagReset EVENT_FLAG_DAILY_0x0acc
    WorkSetConst EVENT_WORK_0x4184, 0
    VMJump L_0667

L_0659:
    // "Excuse me? It seems as though\nthere's no Hyper Potion in your Bag.[f000]븁\u0000\nPlease!\nBuy a Hyper Potion for me!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ExcuseSeemsThoughTheres, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0667:
    VMJump L_067B

L_066D:
    // "What I want is a Hyper Potion!\nPlease! I'm waiting..."
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_WhatWantHyperPotion, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_067B:
    VMJump L_06A2

L_0681:
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acd
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A2
    // "Shopping is so much fun!\nI'm thrilled with all I've accomplished!"
    ParentActorMsg MSGFILE_SCRIPT, ShoppingMall_Text_ShoppingMuchFunIm, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_06A2:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 17
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 18
    WorkSet 0x8001, 2
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 19
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 20
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 14
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
