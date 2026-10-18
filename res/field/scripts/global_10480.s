#include "asm/field_script.inc"
#include "text/script/global_10480.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    VMStackPushFlag EVENT_FLAG_0x00e1
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0051
    // "Hi! I am a humble Maid![f000]븁\u0000\nMy master has a big heart to match his\nbig belly. He's a gourmet![f000]븁\u0000\nSo I am looking for ingredients that will\nmake his heart jump for joy![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HiAmHumbleMaid, 0, 0
    FlagSet EVENT_FLAG_0x00e1

L_0051:
    WorkSetConst 0x8023, 0
    ItemCollectorCheckGroup 0, 0x8023
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    // "Sniff, sniff... Sniff, sniff...[f000]븁\u0000\nOh, that aroma! It smells so gourmet![f000]븁\u0000\nDo you have a wonderful ingredient in\nyour Bag?[f000]븀\u0000\nWould you consider selling it to me?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffSniff, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallBag 1, 0x8021, 0x8020
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00B7
    // "Aww... Well, that's OK.[f000]븁\u0000\nIf you find a gourmet ingredient for me,\nplease let me know!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_AwwWellThatsOk, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_016C

L_00B7:
    ItemCollectorGetPrice 0x8020, 0, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E6
    // "Hmm... This is not quite what I'm\nlooking for.[f000]븁\u0000\nI don't think my master will be satisfied\nwith such a smell..."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HmmNotQuiteWhat, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_016C

L_00E6:
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x8020, 0, 1, 8
    WordSetItemName 0, 0x8020
    // "Oh, it smells so good![f000]븁\u0000\nThat [f000]ĉ\u0001\u0000 of yours is a very\nrare ingredient indeed![f000]븁\u0000\nWould you take $[f000]ȇ\u0001\u0001 for it?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_OhSmellsGoodYours, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_015C
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x8020, 0
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000 and\nreceived $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x8024, 0
    ItemSub 0x8020, 1, 0x8024
    MoneyWinClose
    // "Merci! Thank you!![f000]븁\u0000\nWith this, I can create a superb menu.\nMy master will be so surprised!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_MerciThankCanCreate, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_016C

L_015C:
    MoneyWinClose
    // "Aww... Well, that's OK.[f000]븁\u0000\nIf you find a gourmet ingredient for me,\nplease let me know!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_AwwWellThatsOk, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_016C:
    VMJump L_0180

L_0172:
    // "Sniff, sniff... Sniff, sniff...[f000]븁\u0000\nWell, I don't smell anything interesting\nin your Bag.[f000]븁\u0000\nBut if you find a gourmet ingredient,\nplease let me know!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffSniff_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0180:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    VMStackPushFlag EVENT_FLAG_0x00e2
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01DF
    // "Ore, ore, ore...\nOres that I adore![f000]븁\u0000\nAh, this is where you should laugh...[f000]븁\u0000\nAnyway... I will pay you well for\nrare ores.[f000]븁\u0000\nAnd not just ores! Stones, gems,\nshards...anything mineral! I love it all![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_OreOreOreOres, 0, 0
    FlagSet EVENT_FLAG_0x00e2

L_01DF:
    WorkSetConst 0x8028, 0
    ItemCollectorCheckGroup 1, 0x8028
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0300
    // "Don't you have an adorable ore\nthat shakes my core?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_DontHaveAdorableOre, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallBag 1, 0x8026, 0x8025
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0245
    // "Hmmm. My core won't change.\nI hope you sell it to me someday."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HmmmCoreWontChange, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02FA

L_0245:
    ItemCollectorGetPrice 0x8025, 1, 0x8027
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0274
    // "Huh? What on earth is this?[f000]븁\u0000\nYou can't shake my core\nwith an ore so poor!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HuhWhatEarthCant, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02FA

L_0274:
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x8025, 1, 1, 8
    WordSetItemName 0, 0x8025
    // "Oh, I do adore the ores![f000]븁\u0000\nThat [f000]ĉ\u0001\u0000, it's shaking me!\nWill you sell it for $[f000]ȇ\u0001\u0001?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_OhAdoreOresIts, 7, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EA
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x8025, 1
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000\nand received $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived_2, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x8029, 0
    ItemSub 0x8025, 1, 0x8029
    MoneyWinClose
    // "The deal is done!\nI've scored an ore I can adore!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_DealDoneIveScored, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02FA

L_02EA:
    MoneyWinClose
    // "Hmmm. My core won't change.\nI hope you sell it to me someday."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HmmmCoreWontChange, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02FA:
    VMJump L_0310

L_0300:
    MoneyWinClose
    // "Don't you have an adorable ore\nthat shakes my core?[f000]븁\u0000\nHmmmm. You don't seem to have it.\nCome back again."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_DontHaveAdorableOre_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0310:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8025, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x802a, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802d, 0
    VMStackPushFlag EVENT_FLAG_0x00e3
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0375
    // "As you can see,\nI am an ultrarich billionaire![f000]븁\u0000\nAnd as you can see,\nmy hobby is to collect rare items![f000]븁\u0000\nIn fact, this outfit is ultra expensive\nand rare.[f000]븀\u0000\nCan you see it? Can you?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_CanSeeAmUltrarich, 0, 0
    FlagSet EVENT_FLAG_0x00e3

L_0375:
    VMStackPushFlag EVENT_FLAG_0x00e9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_044D
    ItemCheckAmount ITEM_RELIC_CROWN, 1, 0x802d
    VMStackPush 0x802d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0441
    WorkSetConst 0x802a, 590
    ItemCollectorGetPrice 0x802a, 2, 0x802c
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x802a, 2, 1, 8
    WordSetItemName 0, 0x802a
    // "Actually, it's...\nIt's a rare treasure, Relic Crown![f000]븁\u0000\nI was told it was at the innermost part\nof the Abyssal Ruins![f000]븁\u0000\nI think it is understandable that you\nwant to treasure it, but will you sell it[f000]븀\u0000\nto me for $[f000]ȇ\u0001\u0001? Will you?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_ActuallyItsItsRare, 0, 2, 0
    YesNoWin 0x8010
    FlagSet EVENT_FLAG_0x00e9
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_042B
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x802a, 2
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000\nand received $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived_3, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x802e, 0
    ItemSub 0x802a, 1, 0x802e
    MoneyWinClose
    // "I got a very good item!\nDon't you think it's great? Don't you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_GotVeryGoodItem, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_043B

L_042B:
    MoneyWinClose
    // "If you ever change your mind,\nwill you sell me the rare item? Will you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_IfEverChangeMind, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_043B:
    VMJump L_0447

L_0441:
    VMCall L_0477

L_0447:
    VMJump L_0453

L_044D:
    VMCall L_0477

L_0453:
    WorkSetConst 0x802e, 0
    WorkSetConst 0x802d, 0
    WorkSetConst 0x802c, 0
    WorkSetConst 0x802b, 0
    WorkSetConst 0x802a, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0477:
    WorkSetConst 0x802f, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8032, 0
    ItemCollectorCheckGroup 2, 0x8032
    VMStackPush 0x8032
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05AA
    // "There's something that this billionaire\nwants to get his hands on, even if it[f000]븀\u0000\nmeans spending a lot of money.[f000]븁\u0000\nDo you have such a rare item?\nDo you?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_TheresSomethingBillionaireWants, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallBag 1, 0x8030, 0x802f
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x802f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04EF
    // "If you ever change your mind,\nwill you sell me the rare item? Will you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_IfEverChangeMind, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_05A4

L_04EF:
    ItemCollectorGetPrice 0x802f, 2, 0x8031
    VMStackPush 0x8031
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_051E
    // "This is slightly different.\nDon't you think so, too? Don't you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SlightlyDifferentDontThink, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_05A4

L_051E:
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x802f, 2, 1, 8
    WordSetItemName 0, 0x802f
    // "That [f000]ĉ\u0001\u0000!! Will you sell it to me\nfor $[f000]ȇ\u0001\u0001? Will you?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_WillSellWill, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0594
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x802f, 2
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000\nand received $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived_3, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x8033, 0
    ItemSub 0x802f, 1, 0x8033
    MoneyWinClose
    // "I got a very good item!\nDon't you think it's great? Don't you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_GotVeryGoodItem, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_05A4

L_0594:
    MoneyWinClose
    // "If you ever change your mind,\nwill you sell me the rare item? Will you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_IfEverChangeMind, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_05A4:
    VMJump L_05B8

L_05AA:
    // "There's something that this billionaire\nwants to get his hands on, even if it[f000]븀\u0000\nmeans spending a lot of money.[f000]븁\u0000\nDo you have such a rare item?\nDo you?[f000]븁\u0000\nGrrr! You don't have a rare item,\ndo you?"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_TheresSomethingBillionaireWants_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_05B8:
    WorkSetConst 0x8033, 0
    WorkSetConst 0x8032, 0
    WorkSetConst 0x8031, 0
    WorkSetConst 0x8030, 0
    WorkSetConst 0x802f, 0
    VMReturn

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8034, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8036, 0
    VMStackPushFlag EVENT_FLAG_0x00fb
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0613
    // "Hey, check it out. I've got a sweet deal\nfor ya![f000]븁\u0000\nYou know how Pokémon like to hide behind\nstones or rocks, yeah?[f000]븁\u0000\nSometimes they hide cool stuff, too.[f000]븁\u0000\nIf you find something cool behind a rock,\nyou should bring it to me.[f000]븁\u0000\nI'll make it worth your while.[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HeyCheckOutIve, 0, 0
    FlagSet EVENT_FLAG_0x00fb

L_0613:
    WorkSetConst 0x8037, 0
    ItemCollectorCheckGroup 3, 0x8037
    VMStackPush 0x8037
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0734
    // "What's up?\nDid you find something cool for me?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_WhatsUpDidFind, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallBag 1, 0x8035, 0x8034
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8034
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0679
    // "Hey, come see me if you find something\ncool, got it?[f000]븁\u0000\nI'll make it worth your while."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HeyComeSeeIf, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_072E

L_0679:
    ItemCollectorGetPrice 0x8034, 3, 0x8036
    VMStackPush 0x8036
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_06A8
    // "Nope, this is no good. Not worth my while."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_NopeNoGoodNot, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_072E

L_06A8:
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x8034, 3, 1, 8
    WordSetItemName 0, 0x8034
    // "Hey, nice! Let me check out\nthat [f000]ĉ\u0001\u0000...[f000]븁\u0000\nI'll give you $[f000]ȇ\u0001\u0001 for it.\nWhaddaya say?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_HeyNiceLetCheck, 0, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_071E
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x8034, 3
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000\nand received $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived_4, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x8038, 0
    ItemSub 0x8034, 1, 0x8038
    MoneyWinClose
    // "Hey, thanks![f000]븁\u0000\nI bet I can sell this to a collector for\nway more than I just gave you.[f000]븁\u0000\nHa! That's business for ya!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HeyThanksBetCan, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_072E

L_071E:
    MoneyWinClose
    // "Hey, come see me if you find something\ncool, got it?[f000]븁\u0000\nI'll make it worth your while."
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HeyComeSeeIf, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_072E:
    VMJump L_0742

L_0734:
    // "What's up?\nDid you find something cool for me?[f000]븁\u0000\n...Nope, nothing cool in your Bag.\nOh well. Maybe next time!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_WhatsUpDidFind_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0742:
    WorkSetConst 0x8038, 0
    WorkSetConst 0x8037, 0
    WorkSetConst 0x8036, 0
    WorkSetConst 0x8035, 0
    WorkSetConst 0x8034, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkSetConst 0x8039, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x803b, 0
    VMStackPushFlag EVENT_FLAG_0x014c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_07A1
    // "I work on a farm in Mistralton City.\nSo I need lots of Mulch! So much Mulch![f000]븁\u0000\nYou're a Pokémon Trainer, right? Do you\nknow much about Hidden Grottoes?[f000]븁\u0000\nYou can sometimes find Mulch\ndeep inside Hidden Grottoes![f000]븀\u0000\nLots of Mulch! So much Mulch![f000]븁\u0000\nMulch is great!\nI have much love for Mulch![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_WorkFarmMistraltonCity, 0, 0
    FlagSet EVENT_FLAG_0x014c

L_07A1:
    WorkSetConst 0x803c, 0
    ItemCollectorCheckGroup 4, 0x803c
    VMStackPush 0x803c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08C2
    // "You have Mulch, don't you?\nHow much Mulch?[f000]븀\u0000\nWill you show it to me?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_HaveMulchDontHow, 0, 0
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    FieldClose
    CallBag 1, 0x803a, 0x8039
    FieldOpen
    FadeInBlackQ
    FadeWait
    VMStackPush 0x8039
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0807
    // "Mulch is important for farms and fields!\nMulch helps things grow so much![f000]븁\u0000\nIf you find Mulch, even if it's not much,\nplease bring it to me!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_MulchImportantFarmsFields, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08BC

L_0807:
    ItemCollectorGetPrice 0x8039, 4, 0x803b
    VMStackPush 0x803b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0836
    // "Sniff, sniff, sniff...\nThis doesn't smell right![f000]븁\u0000\nIf it's not Mulch, I can't buy it.\nNothing excites me as much as Mulch!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffDoesnt, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08BC

L_0836:
    MoneyWinDisp 31, 1
    WordSetLoadItemCollectorPrice 0x8039, 4, 1, 8
    WordSetItemName 0, 0x8039
    // "Sniff, sniff, sniff... This smell...\nIt's [f000]ĉ\u0001\u0000, isn't it?[f000]븁\u0000\nI'll pay this much for that Mulch:\n$[f000]ȇ\u0001\u0001. How about it?"
    ActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffSmell, 7, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08AC
    MsgWinCloseAll
    SEPlay SEQ_SE_SYS_22
    ItemCollectorSell 0x8039, 4
    MoneyWinUpdate
    // "Turned over the [f000]ĉ\u0001\u0000\nand received $[f000]ȇ\u0001\u0001!"
    SystemMsg Global10480_Text_TurnedOverReceived_5, 2
    SEWait
    MsgWaitAdvance
    InfoMsgClose
    WorkSetConst 0x803d, 0
    ItemSub 0x8039, 1, 0x803d
    MoneyWinClose
    // "Sniff, sniff, sniff...\nThis Mulch has an exquisite smell![f000]븀\u0000\nSo much aroma! So much excitement!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffMulch, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_08BC

L_08AC:
    MoneyWinClose
    // "Mulch is important for farms and fields!\nMulch helps things grow so much![f000]븁\u0000\nIf you find Mulch, even if it's not much,\nplease bring it to me!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_MulchImportantFarmsFields, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_08BC:
    VMJump L_08D2

L_08C2:
    MoneyWinClose
    // "Sniff, sniff, sniff...\nI don't smell anything.[f000]븁\u0000\nYou don't have Mulch, do you?\nBring me some next time![f000]븀\u0000\nBring me a lot! I need so much Mulch!"
    ParentActorMsg MSGFILE_SCRIPT, Global10480_Text_SniffSniffSniffDont, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_08D2:
    WorkSetConst 0x803d, 0
    WorkSetConst 0x803c, 0
    WorkSetConst 0x803b, 0
    WorkSetConst 0x803a, 0
    WorkSetConst 0x8039, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
