#include "asm/field_script.inc"
#include "text/script/opelucid_city_pokemon_center.h"

// Script plugin 13, from the zones that use this file

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 255
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 8
    WorkSet 0x8001, 1
    RTCallGlobal 10110
    VMStackPop 0x8001
    VMStackPop 0x8000
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D8
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C4
    // "The slow passage of time in\nOpelucid City...[f000]븀\u0000\nIt fits an old woman like me perfectly."
    // "In Opelucid City, you can spend your\ndays in leisure and opulence..."
    ActorMsgVersioned 1024, OpelucidCityPokemonCenter_Text_SlowPassageTimeOpelucid, OpelucidCityPokemonCenter_Text_OpelucidCityCanSpend, 11, 0, 0
    VMJump L_00D2

L_00C4:
    // "You never know what\nwill happen in life.[f000]븁\u0000\nSo maybe it's best to take care of\nwhat you should do while you can."
    // "You never know what\nwill happen in life.[f000]븁\u0000\nSo maybe it's best to take care of\nwhat you want to do while you can."
    ActorMsgVersioned 1024, OpelucidCityPokemonCenter_Text_NeverKnowWhatWill, OpelucidCityPokemonCenter_Text_NeverKnowWhatWill_2, 11, 0, 0

L_00D2:
    VMJump L_00E2

L_00D8:
    // "What was that?\nHow did whatever happen? Why now?"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_WhatHowDidWhatever, 0, 0

L_00E2:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0142
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012E
    // "Sometimes, I think this...[f000]븁\u0000\nThere could be another world, where\na person who looks just like me[f000]븀\u0000\nlives in a completely different way..."
    // "I imagine that even the exact same\nperson would change a lot by living in a[f000]븀\u0000\ndifferent world."
    ActorMsgVersioned 1024, OpelucidCityPokemonCenter_Text_SometimesThinkThereCould, OpelucidCityPokemonCenter_Text_ImagineEvenExactSame, 8, 0, 0
    VMJump L_013C

L_012E:
    // "No matter what the world is like,\nI want to live in it as myself.[f000]븀\u0000\nThat's the truth!"
    // "No matter what the world is like,\nI want to live in it as myself.[f000]븀\u0000\nThat's my ideal!"
    ActorMsgVersioned 1024, OpelucidCityPokemonCenter_Text_NoMatterWhatWorld, OpelucidCityPokemonCenter_Text_NoMatterWhatWorld_2, 8, 0, 0

L_013C:
    VMJump L_014C

L_0142:
    // "Oh...\nI thought it was chilly..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_OhThoughtChilly, 0, 0

L_014C:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A4
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0194
    // "I want to see how the Pokémon Gym has\nchanged, but I'm not a Trainer yet...[f000]븁\u0000\nI wonder if there's a Pokémon\nsomewhere that will travel with me..."
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_WantSeeHowPokemon, 0, 0
    VMJump L_019E

L_0194:
    // "Drayden said that if I want to\ntravel with Pokémon, I should[f000]븀\u0000\nfeel a Pokémon's pain as my own!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_DraydenSaidIfWant, 0, 0

L_019E:
    VMJump L_01AE

L_01A4:
    // "Know what I saw?\nA huuuge icicle fall from the sky!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_KnowWhatSawHuuuge, 0, 0

L_01AE:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag EVENT_FLAG_0x01bc
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01E3
    // "Oh!\nYour Medal Box...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_OhMedalBox, 0, 0
    VMJump L_01ED

L_01E3:
    // "Wh-what in the world?\nWhy is the city covered in ice?[f000]븁\u0000\nOh!\nYour Medal Box...[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_WhWhatWorldWhy, 0, 0

L_01ED:
    MedalGetCount 7, 0x8021
    WordSetMedalRank 0, 0x8021
    WorkCmpConst 0x8021, 0
    VMJumpIf CMP_EQ, L_020A
    VMJump L_021A

L_020A:
    // "The sky-blue color is as\nrefreshing as a clear fall day![f000]븁\u0000\nThat paint is for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_SkyBlueColorRefreshing, 0, 0
    VMJump L_02AA

L_021A:
    WorkCmpConst 0x8021, 1
    VMJumpIf CMP_EQ, L_022D
    VMJump L_023D

L_022D:
    // "The copper coating sparkles elegantly![f000]븁\u0000\nThat paint is for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_CopperCoatingSparklesElegantly, 0, 0
    VMJump L_02AA

L_023D:
    WorkCmpConst 0x8021, 2
    VMJumpIf CMP_EQ, L_0250
    VMJump L_0260

L_0250:
    // "That silver coating is so chic and cool![f000]븁\u0000\nThat paint is for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_SilverCoatingChicCool, 0, 0
    VMJump L_02AA

L_0260:
    WorkCmpConst 0x8021, 3
    VMJumpIf CMP_EQ, L_0273
    VMJump L_0283

L_0273:
    // "How luxurious! That gold coating\nis gorgeous![f000]븁\u0000\nThat paint is for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    ParentActorMsg MSGFILE_SCRIPT, OpelucidCityPokemonCenter_Text_HowLuxuriousGoldCoating, 0, 0
    VMJump L_02AA

L_0283:
    WorkCmpConst 0x8021, 4
    VMJumpIf CMP_EQ, L_0296
    VMJump L_02AA

L_0296:
    // "This stylish design has red\nhighlights on a white body, and the[f000]븀\u0000\nmotif is Reshiram, the legendary[f000]븀\u0000\nDragon-type Pokémon.[f000]븁\u0000\nIt's for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    // "This stylish design has blue\nhighlights on a jet-black body, and the[f000]븀\u0000\nmotif is Zekrom, the legendary[f000]븀\u0000\nDragon-type Pokémon.[f000]븁\u0000\nIt's for [f000]Ķ\u0001\u0000-rank\nmedalists only!"
    ActorMsgVersioned 1024, OpelucidCityPokemonCenter_Text_StylishDesignHasRed, OpelucidCityPokemonCenter_Text_StylishDesignHasBlue, 9, 0, 0
    VMJump L_02AA

L_02AA:
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
