#include "asm/field_script.inc"
#include "text/script/global_10380.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8026, 0

Script_1:
    WordSetPlayerName 0
    // "How is your Pokédex coming along?\nCan I see it?"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_HowPokedexComingAlong, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0102
    PokeDexGetEvaluationParams 2, 0x8020, 0x8021, 0x8022
    VMStackPushFlag EVENT_FLAG_0x008d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A1
    VMCall L_0142
    // "Let me see...[f000]븁\u0000\nOh, you've caught [f000]Ȃ\u0001\u0000 Pokémon in the\nUnova region!"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_LetSeeOhYouve_2, 0, 0
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    VMJump L_00C3

L_00A1:
    VMCall L_0112
    // "Let me see...[f000]븁\u0000\nOh, you've seen [f000]Ȃ\u0001\u0000 Pokémon in the\nUnova region!"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_LetSeeOhYouve, 0, 0
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0

L_00C3:
    VMCall L_02FC
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00EE
    // "It seems like you are meeting Pokémon at\na good clip![f000]븁\u0000\nHere! This is a gift to thank you for all\nyour hard work![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_SeemsLikeMeetingPokemon_2, 0, 0
    ActorMsgClose
    VMCall L_03FA

L_00EE:
    // "Keep up the good work!"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_KeepUpGoodWork, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0110

L_0102:
    // "This Pokédex is full of memories from your\njourney. It's a treasure you can cherish[f000]븀\u0000\nfor your entire life."
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_PokedexFullMemoriesFrom, 0, 0
    LastKeyWait
    ActorMsgClose

L_0110:
    RTEndGlobal

L_0112:
    PokeDexIsComplete 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012F
    FlagSet EVENT_FLAG_0x008d

L_012F:
    PokeDexGetEvaluationParams 2, 0x8020, 0x8021, 0x8022
    WordSetNumber 0, 0x8021, 3
    VMReturn

L_0142:
    PokeDexGetEvaluationParams 3, 0x8020, 0x8021, 0x8022
    WordSetNumber 0, 0x8021, 3
    VMReturn
    PokeDexIsComplete 0x8010, 2
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0178
    MEPlay SEQ_ME_HYOUKA6
    VMJump L_017E

L_0178:
    VMCall L_01AB

L_017E:
    VMReturn
    PokeDexIsComplete 0x8010, 3
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01A3
    MEPlay SEQ_ME_HYOUKA6
    VMJump L_01A9

L_01A3:
    VMCall L_01AB

L_01A9:
    VMReturn

L_01AB:
    VMStackPush 0x8021
    VMStackPushConst 39
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01C8
    MEPlay SEQ_ME_HYOUKA1
    VMJump L_0240

L_01C8:
    VMStackPush 0x8021
    VMStackPushConst 99
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_01E5
    MEPlay SEQ_ME_HYOUKA2
    VMJump L_0240

L_01E5:
    VMStackPush 0x8021
    VMStackPushConst 149
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0202
    MEPlay SEQ_ME_HYOUKA3
    VMJump L_0240

L_0202:
    VMStackPush 0x8021
    VMStackPushConst 199
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_021F
    MEPlay SEQ_ME_HYOUKA4
    VMJump L_0240

L_021F:
    VMStackPush 0x8021
    VMStackPushConst 249
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_023C
    MEPlay SEQ_ME_HYOUKA5
    VMJump L_0240

L_023C:
    MEPlay SEQ_ME_HYOUKA5

L_0240:
    VMReturn
    PokeDexIsComplete 0x8010, 1
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0265
    MEPlay SEQ_ME_HYOUKA6
    VMJump L_02FA

L_0265:
    VMStackPush 0x8021
    VMStackPushConst 159
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_0282
    MEPlay SEQ_ME_HYOUKA1
    VMJump L_02FA

L_0282:
    VMStackPush 0x8021
    VMStackPushConst 349
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_029F
    MEPlay SEQ_ME_HYOUKA2
    VMJump L_02FA

L_029F:
    VMStackPush 0x8021
    VMStackPushConst 449
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_02BC
    MEPlay SEQ_ME_HYOUKA3
    VMJump L_02FA

L_02BC:
    VMStackPush 0x8021
    VMStackPushConst 549
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_02D9
    MEPlay SEQ_ME_HYOUKA4
    VMJump L_02FA

L_02D9:
    VMStackPush 0x8021
    VMStackPushConst 633
    VMStackCmp CMP_LE
    VMJumpIf CMP_STACK, L_02F6
    MEPlay SEQ_ME_HYOUKA5
    VMJump L_02FA

L_02F6:
    MEPlay SEQ_ME_HYOUKA5

L_02FA:
    VMReturn

L_02FC:
    WorkSetConst 0x8023, 0
    PokeDexIsComplete 0x8010, 2
    VMStackPushFlag EVENT_FLAG_0x0088
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0337
    WorkSetConst 0x8024, 1
    WorkSetConst 0x8023, 1

L_0337:
    PokeDexIsComplete 0x8010, 3
    VMStackPushFlag EVENT_FLAG_0x0089
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_037C
    WorkSetConst 0x8025, 1
    WorkSetConst 0x8023, 1

L_037C:
    PokeDexIsComplete 0x8010, 1
    VMStackPushFlag EVENT_FLAG_0x008a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03C1
    WorkSetConst 0x8026, 1
    WorkSetConst 0x8023, 1

L_03C1:
    VMReturn

L_03C3:
    WorkSetConst 0x8023, 0
    PokeDexIsComplete 0x8010, 1
    VMStackPushFlag EVENT_FLAG_0x008a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_03F8
    WorkSetConst 0x8023, 1

L_03F8:
    VMReturn

L_03FA:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0452
    ItemCheckSpace ITEM_PERMIT, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0434
    WorkSetConst 0x8008, 630
    RTCallGlobal 2803
    VMReturn

L_0434:
    WorkSetConst 0x8000, 630
    WorkSetConst 0x8001, 1
    RTCallGlobal 2805
    // "If you have a Permit,\nyou can go to the Nature Preserve[f000]븀\u0000\nfrom Mistralton City by plane![f000]븁\u0000\nGo check it out to see\nwhat kind of place it is![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_IfHavePermitCan, 0, 0
    FlagSet EVENT_FLAG_0x0088

L_0452:
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_04AA
    ItemCheckSpace ITEM_OVAL_CHARM, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_048C
    WorkSetConst 0x8008, 631
    RTCallGlobal 2803
    VMReturn

L_048C:
    WorkSetConst 0x8000, 631
    WorkSetConst 0x8001, 1
    RTCallGlobal 2805
    // "If you have an Oval Charm,\nwe don't know why,[f000]븀\u0000\nbut you'll have a better chance to[f000]븀\u0000\nfind Eggs at the Pokémon Day Care![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_IfHaveOvalCharm, 0, 0
    FlagSet EVENT_FLAG_0x0089

L_04AA:
    VMStackPush 0x8026
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0502
    ItemCheckSpace ITEM_SHINY_CHARM, 1, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04E4
    WorkSetConst 0x8008, 632
    RTCallGlobal 2803
    VMReturn

L_04E4:
    WorkSetConst 0x8000, 632
    WorkSetConst 0x8001, 1
    RTCallGlobal 2805
    // "If you have a Shiny Charm,\nwe don't know why, but you'll have a[f000]븀\u0000\nbetter chance to find Shiny Pokémon![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_IfHaveShinyCharm, 0, 0
    FlagSet EVENT_FLAG_0x008a

L_0502:
    VMReturn

Script_2:
    WordSetPlayerName 0
    // "You're here to show me how your\nPokédex is coming along, right?"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_YoureHereShowHow, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0584
    PokeDexGetEvaluationParams 1, 0x8020, 0x8021, 0x8022
    WordSetNumber 0, 0x8021, 3
    // "I see![f000]븁\u0000\nYou've caught [f000]Ȃ\u0001\u0000 Pokémon, then..."
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_SeeYouveCaughtPokemon, 0, 0
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    ParentActorMsg MSGFILE_SCRIPT, 0x8020, 0, 0
    VMCall L_03C3
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_057A
    MsgWaitAdvance
    // "Oh! We have a gift for you.\nMy daughter has it.[f000]븀\u0000\nGo speak to her and get it!"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_OhWeHaveGift, 0, 0

L_057A:
    LastKeyWait
    ActorMsgClose
    VMJump L_0592

L_0584:
    // "Oh, really...[f000]븁\u0000\nIf you want me to have a look at\nyour Pokédex, come here again!"
    ParentActorMsg MSGFILE_SCRIPT, Global10380_Text_OhReallyIfWant, 0, 0
    LastKeyWait
    ActorMsgClose

L_0592:
    RTEndGlobal

Script_3:
    PokeDexHaveNational 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05F9
    PokeDexGetEvaluationParams 1, 0x8020, 0x8021, 0x8022
    WordSetNumber 0, 0x8021, 3
    // "I see![f000]븁\u0000\nYou've caught [f000]Ȃ\u0001\u0000 Pokémon, then..."
    SystemMsg Global10380_Text_SeeYouveCaughtPokemon, 2
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    SystemMsg 0x8020, 2
    MsgWaitAdvance
    VMCall L_03C3
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_05F3
    // "Oh! We have a gift for you.\nMy daughter has it.[f000]븀\u0000\nGo speak to her and get it!"
    SystemMsg Global10380_Text_OhWeHaveGift, 2
    MsgWaitAdvance

L_05F3:
    VMJump L_0667

L_05F9:
    VMStackPushFlag EVENT_FLAG_0x008d
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_062C
    VMCall L_0142
    // "Let me see...[f000]븁\u0000\nOh, you've caught [f000]Ȃ\u0001\u0000 Pokémon in the\nUnova region!"
    SystemMsg Global10380_Text_LetSeeOhYouve_2, 2
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    SystemMsg 0x8020, 2
    VMJump L_0646

L_062C:
    VMCall L_0112
    // "Let me see...[f000]븁\u0000\nOh, you've seen [f000]Ȃ\u0001\u0000 Pokémon in the\nUnova region!"
    SystemMsg Global10380_Text_LetSeeOhYouve, 2
    MEPlay 0x8022
    MEWait
    MsgWaitAdvance
    SystemMsg 0x8020, 2

L_0646:
    VMCall L_02FC
    VMStackPush 0x8023
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0667
    // "It seems like you are meeting Pokémon at\na good clip![f000]븁\u0000\nI have something I want to give you!\nPlease come to the lab when you can."
    SystemMsg Global10380_Text_SeemsLikeMeetingPokemon, 2
    MsgWaitAdvance

L_0667:
    RTEndGlobal
    .balign 4, 0
