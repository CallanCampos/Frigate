#include "asm/field_script.inc"
#include "text/script/nacrene_city_10.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    RTCGetWeekDay 0x8020
    WorkCmpConst 0x8020, 0
    VMJumpIf CMP_EQ, L_003F
    VMJump L_0051

L_003F:
    WorkSetConst EVENT_WORK_0x4020, 43
    WorkSetConst EVENT_WORK_0x4021, 309
    VMJump L_012F

L_0051:
    WorkCmpConst 0x8020, 1
    VMJumpIf CMP_EQ, L_0064
    VMJump L_0076

L_0064:
    WorkSetConst EVENT_WORK_0x4020, 52
    WorkSetConst EVENT_WORK_0x4021, 131
    VMJump L_012F

L_0076:
    WorkCmpConst 0x8020, 2
    VMJumpIf CMP_EQ, L_0089
    VMJump L_009B

L_0089:
    WorkSetConst EVENT_WORK_0x4020, 45
    WorkSetConst EVENT_WORK_0x4021, 306
    VMJump L_012F

L_009B:
    WorkCmpConst 0x8020, 3
    VMJumpIf CMP_EQ, L_00AE
    VMJump L_00C0

L_00AE:
    WorkSetConst EVENT_WORK_0x4020, 16
    WorkSetConst EVENT_WORK_0x4021, 126
    VMJump L_012F

L_00C0:
    WorkCmpConst 0x8020, 4
    VMJumpIf CMP_EQ, L_00D3
    VMJump L_00E5

L_00D3:
    WorkSetConst EVENT_WORK_0x4020, 20
    WorkSetConst EVENT_WORK_0x4021, 128
    VMJump L_012F

L_00E5:
    WorkCmpConst 0x8020, 5
    VMJumpIf CMP_EQ, L_00F8
    VMJump L_010A

L_00F8:
    WorkSetConst EVENT_WORK_0x4020, 64
    WorkSetConst EVENT_WORK_0x4021, 305
    VMJump L_012F

L_010A:
    WorkCmpConst 0x8020, 6
    VMJumpIf CMP_EQ, L_011D
    VMJump L_012F

L_011D:
    WorkSetConst EVENT_WORK_0x4020, 53
    WorkSetConst EVENT_WORK_0x4021, 314
    VMJump L_012F

L_012F:
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag EVENT_FLAG_DAILY_0x0aaf
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0160
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Enjoy the taste of our specialty!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_EnjoyTasteOurSpecialty, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_021A

L_0160:
    RTCGetWeekDay 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01B5
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Café Warehouse,\na stylish café in a rural area![f000]븁\u0000\nOur café has a special on Wednesdays!\nHere, have a Soda Pop![f000]븁\u0000\nEnjoy the taste of our specialty![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WelcomeCafeWarehouseStylish_2, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 31
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_DAILY_0x0aaf
    VMJump L_021A

L_01B5:
    VMStackPush 0x8020
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0206
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Café Warehouse,\nthe stylish café in the country![f000]븁\u0000\nWe have a special on Saturdays!\nHave a complimentary Lemonade![f000]븁\u0000\nWe're really proud of our original recipe![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WelcomeCafeWarehouseStylish_3, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 32
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    FlagSet EVENT_FLAG_DAILY_0x0aaf
    VMJump L_021A

L_0206:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Café Warehouse,\na stylish café in a rural area!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WelcomeCafeWarehouseStylish, 0, 0
    LastKeyWait
    ActorMsgClose

L_021A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4020, 43
    VMJumpIf CMP_EQ, L_023B
    VMJump L_0364

L_023B:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02BD
    // "I found a handkerchief.\nI think one of our regulars dropped it.[f000]븁\u0000\nI think that the customer had a brown\nPokémon that can use Super Fang...[f000]븁\u0000\nI'm the kind of person who instantly\nforgets about these kinds of things...[f000]븁\u0000\nIf you don't mind, would you look\nfor the owner of this handkerchief?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_FoundHandkerchiefThinkOne, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02A9
    // "Thank you!\nI'll let you hang on to it.[f000]븁\u0000\nEvery day, we get a lot of different\ncustomers, so all ya gotta do is ask 'em!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_ThankIllLetHang, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 634
    WorkSet 0x8001, 1
    RTCallGlobal 2806
    VMStackPop 0x8001
    VMStackPop 0x8000
    WorkSetConst EVENT_WORK_0x40dc, 1
    VMJump L_02B7

L_02A9:
    // "Sniff...[f000]븁\u0000\nI sure want to give this forgotten\nhandkerchief back to its owner..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_SniffSureWantGive, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02B7:
    VMJump L_035E

L_02BD:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02E4
    // "I think it might have been the customer\nwho had a brown Pokémon[f000]븀\u0000\nthat can use Super Fang..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_ThinkMightHaveBeen, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_035E

L_02E4:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_033D
    // "What? You found the customer\nwho lost his handkerchief?![f000]븁\u0000\nIt was the person with Patrat, you say?\nWhat a relief. Thank you so much![f000]븁\u0000\nHere, this is for you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WhatFoundCustomerWho, 0, 0
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 64
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Work is so much fun![f000]븁\u0000\nI mean, I get to meet so many\npeople and Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WorkMuchFunMean, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst EVENT_WORK_0x40dc, 3
    VMJump L_035E

L_033D:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035E
    // "Work is so much fun![f000]븁\u0000\nI mean, I get to meet so many\npeople and Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WorkMuchFunMean, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_035E:
    VMJump L_0584

L_0364:
    WorkCmpConst EVENT_WORK_0x4020, 52
    VMJumpIf CMP_EQ, L_0377
    VMJump L_03B0

L_0377:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039C
    // "Hm? Did I lose something?\nNo, I don't think so...[f000]븁\u0000\nI bet it was a customer on another day."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_HmDidLoseSomething, 0, 0
    LastKeyWait
    VMJump L_03A8

L_039C:
    // "It's Monday! I'm drinking a Moomoo Milk\nto get myself stoked up for the week![f000]븁\u0000\nIt makes me feel like Krokorok and I\nare both ready to take on another week!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_ItsMondayImDrinking, 0, 0
    LastKeyWait

L_03A8:
    MsgWinCloseAll
    VMJump L_0584

L_03B0:
    WorkCmpConst EVENT_WORK_0x4020, 45
    VMJumpIf CMP_EQ, L_03C3
    VMJump L_03FC

L_03C3:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E8
    // "What? No, I haven't lost anything.[f000]븁\u0000\nMy sweet Lillipup can't use\nSuper Fang, either.[f000]븁\u0000\nPerhaps it was another customer.[f000]븁\u0000\nMany people come to this café only\non a certain day."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WhatNoHaventLost, 0, 0
    LastKeyWait
    VMJump L_03F4

L_03E8:
    // "I always come here to relax with Lillipup\nand look at lovely Fossils in the museum.[f000]븁\u0000\nBy the way, my little Lillipup's\nspecialty is Roar."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_AlwaysComeHereRelax, 0, 0
    LastKeyWait

L_03F4:
    MsgWinCloseAll
    VMJump L_0584

L_03FC:
    WorkCmpConst EVENT_WORK_0x4020, 16
    VMJumpIf CMP_EQ, L_040F
    VMJump L_0448

L_040F:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0434
    // "You're looking for someone\nwho lost something? Aren't you nice?[f000]븁\u0000\nMunna isn't brown, though,\nso it must be a different customer.[f000]븁\u0000\nI think I might have seen someone with a\nbrown Pokémon on Thursday..."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_YoureLookingSomeoneWho, 0, 0
    LastKeyWait
    VMJump L_0440

L_0434:
    // "Don't you think my Munna would\nbe a superstar if it was in[f000]븀\u0000\na Pokémon Musical?[f000]븁\u0000\nIt's on the path to success, just like\nme and my career as a Pokémon Trainer!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_DontThinkMunnaWould, 0, 0
    LastKeyWait

L_0440:
    MsgWinCloseAll
    VMJump L_0584

L_0448:
    WorkCmpConst EVENT_WORK_0x4020, 20
    VMJumpIf CMP_EQ, L_045B
    VMJump L_04EC

L_045B:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B3
    // "Huh?\nDid I lose something?[f000]븁\u0000\nHey! That handkerchief!\nIt's mine![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_HuhDidLoseSomething, 0, 0
    MsgWinCloseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_ARDEMO_01
    // "[f000]Ā\u0001\u0000\nhanded over the Grubby Hanky!"
    SystemMsg NacreneCity10_Text_HandedOverGrubbyHanky, 0
    MsgWaitAdvance
    InfoMsgClose
    SEWait
    WorkSetConst 0x8021, 0
    ItemSub ITEM_GRUBBY_HANKY, 1, 0x8021
    // "Thank you so much for\nfinding that for me![f000]븁\u0000\nI need that handkerchief so I can wipe\nPatrat's cheeks after it eats Berries."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_ThankMuchFindingNeed, 0, 0
    LastKeyWait
    WorkSetConst EVENT_WORK_0x40dc, 2
    VMJump L_04E4

L_04B3:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04D8
    // "Thank you so much for\nfinding that for me![f000]븁\u0000\nI need that handkerchief so I can wipe\nPatrat's cheeks after it eats Berries."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_ThankMuchFindingNeed, 0, 0
    LastKeyWait
    VMJump L_04E4

L_04D8:
    // "I always come to Café Warehouse\nto relax after Patrat and I go[f000]븀\u0000\nfor a jog in Pinwheel Forest.[f000]븁\u0000\nGetting a refreshing drink after\nexercising is just the best! Right?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_AlwaysComeCafeWarehouse, 0, 0
    LastKeyWait

L_04E4:
    MsgWinCloseAll
    VMJump L_0584

L_04EC:
    WorkCmpConst EVENT_WORK_0x4020, 64
    VMJumpIf CMP_EQ, L_04FF
    VMJump L_0538

L_04FF:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0524
    // "I haven't lost anything.\nWatchog doesn't let anything get by it![f000]븁\u0000\nI'm sure whatever it is belongs\nto some other customer."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_HaventLostAnythingWatchog, 0, 0
    LastKeyWait
    VMJump L_0530

L_0524:
    // "Wa ha ha! My Watchog has nice fur!\nDoesn't it look impressive?[f000]븁\u0000\nI always come here to Café Warehouse\nevery week and brag myself silly about it![f000]븁\u0000\nIts Super Fang is super cool!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WaHaHaWatchog, 0, 0
    LastKeyWait

L_0530:
    MsgWinCloseAll
    VMJump L_0584

L_0538:
    WorkCmpConst EVENT_WORK_0x4020, 53
    VMJumpIf CMP_EQ, L_054B
    VMJump L_0584

L_054B:
    VMStackPush EVENT_WORK_0x40dc
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0570
    // "My! That handkerchief looks just like\nmine, but it belongs to someone else.[f000]븁\u0000\nIt does have a certain, hmm, ah,\nje ne sais quoi about it.[f000]븁\u0000\nWhat's with that look?[f000]븁\u0000\nOK. OK. It smells like a Patrat."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_HandkerchiefLooksJustLike, 0, 0
    LastKeyWait
    VMJump L_057C

L_0570:
    // "Taking it easy in the café on weekends\nmakes me forget the stress of workdays."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_TakingEasyCafeWeekends, 0, 0
    LastKeyWait

L_057C:
    MsgWinCloseAll
    VMJump L_0584

L_0584:
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    WorkCmpConst EVENT_WORK_0x4021, 309
    VMJumpIf CMP_EQ, L_05AB
    VMJump L_05CD

L_05AB:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 511, 0
    // "Sei jii!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_SeiJii, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_05CD:
    WorkCmpConst EVENT_WORK_0x4021, 131
    VMJumpIf CMP_EQ, L_05E0
    VMJump L_0602

L_05E0:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Bwaa!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_Bwaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_0602:
    WorkCmpConst EVENT_WORK_0x4021, 306
    VMJumpIf CMP_EQ, L_0615
    VMJump L_0637

L_0615:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Yap, yap! ♪"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_YapYap, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_0637:
    WorkCmpConst EVENT_WORK_0x4021, 126
    VMJumpIf CMP_EQ, L_064A
    VMJump L_066C

L_064A:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 517, 0
    // "Muuuuuuuuuun!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_Muuuuuuuuuun, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_066C:
    WorkCmpConst EVENT_WORK_0x4021, 128
    VMJumpIf CMP_EQ, L_067F
    VMJump L_06A1

L_067F:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Kee keeh!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_KeeKeeh, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_06A1:
    WorkCmpConst EVENT_WORK_0x4021, 305
    VMJumpIf CMP_EQ, L_06B4
    VMJump L_06D6

L_06B4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 505, 0
    // "Waach-ch-ch!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_WaachChCh, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_06D6:
    WorkCmpConst EVENT_WORK_0x4021, 314
    VMJumpIf CMP_EQ, L_06E9
    VMJump L_070B

L_06E9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 559, 0
    // "Skraaa!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_Skraaa, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_070B

L_070B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Some things may be famous and good,\nbut some good things aren't famous.[f000]븁\u0000\nWhat matters is whether you like them.\nDon't you agree?"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_SomeThingsMayFamous, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've been getting really, really heavy,\nbut I just can't stop getting seconds.[f000]븁\u0000\nWho was it?\nWho used the move Encore on me?!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_IveBeenGettingReally, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I like Nacrene's Café Warehouse\nand Striaton's restaurant, too.[f000]븁\u0000\nEach offers such different flavors!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_LikeNacrenesCafeWarehouse, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This café has a special on\nWednesdays and Saturdays!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity10_Text_CafeHasSpecialWednesdays, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
