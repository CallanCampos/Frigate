#include "asm/field_script.inc"
#include "text/script/nuvema_town_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca's Dad: I see! So you're\non a Pokémon journey, too![f000]븁\u0000\nI'm quite impressed with you.\nAnd with your parents, too.[f000]븁\u0000\nAfter always being together as a family,\nand now not having you around...[f000]븁\u0000\nI'll bet--no, I'm sure--they're worried\nabout you out on your own.[f000]븁\u0000\nBe sure to tell them all about what\nhappens in your travels."
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown4_Text_BiancasDadSeeYoure, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Bianca's Mom: Oh! So you also received\na Pokédex from Professor Juniper![f000]븁\u0000\nThat daughter of mine looks at her\nPokédex every chance she gets![f000]븁\u0000\nShe's always going on about the\nmany Pokémon and many[f000]븀\u0000\nmemories it contains..."
    ParentActorMsg MSGFILE_SCRIPT, NuvemaTown4_Text_BiancasMomOhAlso, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
