#include "asm/field_script.inc"
#include "text/script/accumula_town.h"

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
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8028, 0

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Accumula Town\nThe Fast-Growing Town!"
    MsgPlaceSign AccumulaTown_Text_AccumulaTownFastGrowing, 1
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PokePartyGetCount 0x8020, 2

L_0094:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_01A9
    PokePartyGetSpecies 0x8022, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 495
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 496
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 497
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_00FF
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F9
    WorkSetConst 0x8023, 1

L_00F9:
    VMJump L_019D

L_00FF:
    VMStackPush 0x8022
    VMStackPushConst 498
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 499
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 500
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0151
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014B
    WorkSetConst 0x8023, 1

L_014B:
    VMJump L_019D

L_0151:
    VMStackPush 0x8022
    VMStackPushConst 501
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 502
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 503
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_019D
    VMStackPush EVENT_WORK_0x4030
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019D
    WorkSetConst 0x8023, 1

L_019D:
    WorkAdd 0x8021, 1
    VMJump L_0094

L_01A9:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D0
    // "What's this?[f000]븁\u0000\nYou and your Pokémon\nsomehow resemble one another![f000]븁\u0000\nI'm sure you came to look like\none another during all of the time[f000]븀\u0000\nyou spent together!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WhatsPokemonSomehowResemble, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_01DE

L_01D0:
    // "What's this?[f000]븁\u0000\nYou and your Pokémon\nsomehow resemble one another!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WhatsPokemonSomehowResemble_2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01DE:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Do you like Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_LikePokemon, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0221
    // "Yes, yes, you have Pokémon with you,\nso I'm sure you do!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_YesYesHavePokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_022F

L_0221:
    // "That so? Then why are you\non a journey with Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_ThenWhyJourneyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_022F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Now, I wonder which person\nwas the first person in the[f000]븀\u0000\nworld to get along with Pokémon..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_NowWonderWhichPerson, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "See my Audino?[f000]븁\u0000\nShe popped out of the\nswaying tall grass just to meet me!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_SeeAudinoShePopped, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Di chee! ♪"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_DiChee, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My cute little Foongus\nlooks just like a Poké Ball!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_CuteLittleFoongusLooks, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This is a Foongus?"
    InfoMsg AccumulaTown_Text_Foongus, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0ace
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0320
    VMStackPushFlag EVENT_FLAG_0x014d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0308
    // "Sorry to bother you, but I like low\nplaces like this and little tiny things.[f000]븁\u0000\nTiny Pokémon are the most interesting![f000]븁\u0000\nI particularly like Pokémon that have a\nheight of eight inches or less.[f000]븁\u0000\nBy the way...\nDo you have any tiny Pokémon?[f000]븁\u0000\nIf you do, could you show me?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_SorryBotherButLike, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x014d
    VMJump L_0314

L_0308:
    // "Will you show me a tiny Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WillShowTinyPokemon, 0, 0
    MsgWaitAdvance

L_0314:
    VMCall L_0334
    VMJump L_032E

L_0320:
    // "Thanks for showing me a tiny Pokémon!\nIt was super interesting![f000]븁\u0000\nI'd sure like it if you'd\nshow me one again tomorrow..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_ThanksShowingTinyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_032E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0334:
    PokePartyGetCount 0x8020, 0

L_033A:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_05BD
    PokePartyGetSpecies 0x8022, 0x8021
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 50
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_038D
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 50
    VMJump L_05B1

L_038D:
    VMStackPush 0x8022
    VMStackPushConst 177
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03C1
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 177
    VMJump L_05B1

L_03C1:
    VMStackPush 0x8022
    VMStackPushConst 298
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03F5
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 298
    VMJump L_05B1

L_03F5:
    VMStackPush 0x8022
    VMStackPushConst 406
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0429
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 406
    VMJump L_05B1

L_0429:
    VMStackPush 0x8022
    VMStackPushConst 412
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0498
    PokePartyGetParam 0x8027, 0x8021, 111
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8027
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0492
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 412

L_0492:
    VMJump L_05B1

L_0498:
    VMStackPush 0x8022
    VMStackPushConst 433
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04CC
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 433
    VMJump L_05B1

L_04CC:
    VMStackPush 0x8022
    VMStackPushConst 492
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_051B
    PokePartyGetParam 0x8027, 0x8021, 111
    VMStackPush 0x8027
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0515
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 492

L_0515:
    VMJump L_05B1

L_051B:
    VMStackPush 0x8022
    VMStackPushConst 590
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_054F
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 590
    VMJump L_05B1

L_054F:
    VMStackPush 0x8022
    VMStackPushConst 595
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0583
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 595
    VMJump L_05B1

L_0583:
    VMStackPush 0x8022
    VMStackPushConst 602
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05B1
    WorkSetConst 0x8024, 1
    WordSetPokeSpeciesWithArticle 0, 602

L_05B1:
    WorkAdd 0x8021, 1
    VMJump L_033A

L_05BD:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0616
    // "Wow! Amazing!\n[f000]ā\u0001\u0000 is so tiny![f000]븁\u0000\nI wonder what it would feel like\nto become [f000]ā\u0001\u0000...[f000]븁\u0000\nI bet I'd get a way different outlook\non life...[f000]븁\u0000\nJust imagining it gets me excited!\nTiny things are super interesting!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WowAmazingTinyWonder, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 63
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Thanks for showing me a tiny Pokémon!\nIt was super interesting![f000]븁\u0000\nI'd sure like it if you'd\nshow me one again tomorrow..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_ThanksShowingTinyPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0ace
    VMJump L_0624

L_0616:
    // "Hm...[f000]븁\u0000\nYou don't even have a teensy\nnumber of tiny Pokémon.[f000]븁\u0000\nI'd sure like to see a Pokémon\nthat's eight inches high or smaller..."
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_HmDontEvenHave, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0624:
    VMReturn

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_DAILY_0x0acf
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0682
    VMStackPushFlag EVENT_FLAG_0x014e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_066A
    // "You know what?![f000]븁\u0000\nI love high places like this,\nand really big things![f000]븁\u0000\nMy favorite Pokémon\nare big ones, too![f000]븁\u0000\nWhat I mean by big, you know,\nis being more than 17 feet tall![f000]븁\u0000\nC'mon! If you have a big\nPokémon, show it to me!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_KnowWhatLoveHigh, 0, 0
    MsgWaitAdvance
    FlagSet EVENT_FLAG_0x014e
    VMJump L_0676

L_066A:
    // "Have you come to show me a big Pokémon?"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_HaveComeShowBig, 0, 0
    MsgWaitAdvance

L_0676:
    VMCall L_0696
    VMJump L_0690

L_0682:
    // "Big thanks for showing me a big Pokémon![f000]븁\u0000\nI'd be even more happy if you'd show me\none tomorrow, too!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_BigThanksShowingBig, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0690:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_0696:
    PokePartyGetCount 0x8020, 0

L_069C:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_08B0
    PokePartyGetSpecies 0x8022, 0x8021
    PokePartyIsEgg 0x8028, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 95
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06EF
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 95
    VMJump L_08A4

L_06EF:
    VMStackPush 0x8022
    VMStackPushConst 130
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0723
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 130
    VMJump L_08A4

L_0723:
    VMStackPush 0x8022
    VMStackPushConst 208
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0757
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 208
    VMJump L_08A4

L_0757:
    VMStackPush 0x8022
    VMStackPushConst 249
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_078B
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 249
    VMJump L_08A4

L_078B:
    VMStackPush 0x8022
    VMStackPushConst 321
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07BF
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 321
    VMJump L_08A4

L_07BF:
    VMStackPush 0x8022
    VMStackPushConst 350
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_07F3
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 350
    VMJump L_08A4

L_07F3:
    VMStackPush 0x8022
    VMStackPushConst 384
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0827
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 384
    VMJump L_08A4

L_0827:
    VMStackPush 0x8022
    VMStackPushConst 483
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_085B
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 483
    VMJump L_08A4

L_085B:
    VMStackPush 0x8022
    VMStackPushConst 487
    VMStackCmp CMP_EQ
    VMStackPush 0x8028
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_08A4
    PokePartyGetParam 0x8027, 0x8021, 111
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_08A4
    WorkSetConst 0x8025, 1
    WordSetPokeSpeciesWithArticle 0, 487

L_08A4:
    WorkAdd 0x8021, 1
    VMJump L_069C

L_08B0:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0909
    // "Wow! Amazing!\n[f000]ā\u0001\u0000 is sooooo big![f000]븁\u0000\nRiding on top of [f000]ā\u0001\u0000\nwould be so much fun![f000]븁\u0000\nWhen you look from high places, you can\nsee rooftops and faraway mountains[f000]븀\u0000\nand other things you usually can't see.[f000]븁\u0000\nThat's why I like big things so much!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WowAmazingSoooooBig, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 64
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "Big thanks for showing me a big Pokémon![f000]븁\u0000\nI'd be even more happy if you'd show me\none tomorrow, too!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_BigThanksShowingBig, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet EVENT_FLAG_DAILY_0x0acf
    VMJump L_0917

L_0909:
    // "What?!\nNo big Pokémon for me?[f000]븁\u0000\nI wanna see a Pokémon\nbigger than 17 feet!"
    ParentActorMsg MSGFILE_SCRIPT, AccumulaTown_Text_WhatNoBigPokemon, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0917:
    VMReturn
    .balign 4, 0
