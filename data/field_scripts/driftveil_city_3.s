#include "asm/field_script.inc"
#include "text/script/driftveil_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Grand Hotel Driftveil.\nI'm sorry but we're completely full.[f000]븁\u0000\nBut please feel free to relax."
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_WelcomeGrandHotelDriftveil, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Did you know this?\nIt's from an article in Pokémon Pal.[f000]븁\u0000\n“Press the L Button while selecting\na move during battle to display[f000]븀\u0000\ndetailed information about that move!\""
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_DidKnowItsFrom, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Battling Pokémon stronger\nthan you gives you more Exp. Points!"
    ParentActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_BattlingPokemonStrongerThan, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    PokeDexGetCount 0, 0x8020
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x8020
    VMStackPushConst 70
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_00FE
    VMStackPushFlag EVENT_FLAG_0x013f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E8
    // "Ha hoo!\nYou have a Pokédex.[f000]븁\u0000\nHow many Pokémon have you found?[f000]븁\u0000\nHoo ha! You've found 70 or more!\nNow we're talking! This is for you![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_HaHooHavePokedex, 0, 0, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 253
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "A Pokémon holding a Shell Bell recovers\nits HP a little bit if it inflicts damage[f000]븀\u0000\nduring a battle.[f000]븁\u0000\nBut what's more important is this. Have\nyou shown the Pokédex to a professor?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_PokemonHoldingShellBell, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet EVENT_FLAG_0x013f
    VMJump L_00F8

L_00E8:
    // "A Pokémon holding a Shell Bell recovers\nits HP a little bit if it inflicts damage[f000]븀\u0000\nduring a battle.[f000]븁\u0000\nBut what's more important is this. Have\nyou shown the Pokédex to a professor?"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_PokemonHoldingShellBell, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_00F8:
    VMJump L_010E

L_00FE:
    // "Hoo ha!\nYou have a Pokédex.[f000]븁\u0000\nHow many Pokémon have you found?[f000]븁\u0000\nIf you find 70 or more, I'll give you\nsomething sure to delight!"
    ActorMsg MSGFILE_SCRIPT, DriftveilCity3_Text_HooHaHavePokedex, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_010E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
