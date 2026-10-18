#include "asm/field_script.inc"
#include "text/script/floccesy_town_4.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "As they wandered in search of Pokémon,\npeople began making homes in more places.[f000]븁\u0000\nThis ranch used to be a grassy meadow.[f000]븁\u0000\nIt's now become a place where people\nand Pokémon have homes together!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown4_Text_TheyWanderedSearchPokemon, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    PokePartyGetCount 0x8020, 1
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0065
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't really know much about\nPokémon types and stuff like that.[f000]븁\u0000\nI just focus on raising one Pokémon\nand charge ahead![f000]븁\u0000\nBut it's kinda the hard way to do things.[f000]븁\u0000\nWhen I run into a Pokémon that mine is\nnot good against, I can't win anymore!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown4_Text_DontReallyKnowMuch, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0079

L_0065:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, you have so many Pokémon![f000]븁\u0000\nSeems to me that thinking about how\nto raise each one would be a lot of fun!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown4_Text_OhHaveManyPokemon, 0, 0
    LastKeyWait
    ActorMsgClose

L_0079:
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Yip!"
    ParentActorMsg MSGFILE_SCRIPT, FloccesyTown4_Text_Yip, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
